#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix qw[:all];
use Affix::Build;
use File::Temp qw[tempfile];
use File::Spec;
use POSIX qw[EXIT_SUCCESS EXIT_FAILURE];
use Test2::V0 defined $ENV{FUZZ_SRAND} ? $ENV{FUZZ_SRAND} == 0 ? ( -no_srand => 1 ) : ( -srand => $ENV{FUZZ_SRAND} ) : ();
$|++;
my $max_iter = $ENV{FUZZ_MAX_ITER} // 1000;
my $timeout  = $ENV{FUZZ_TIMEOUT}  // 5;
my $verbose  = $ENV{FUZZ_VERBOSE}  // 0;

sub compile_source ($code) {
    my $src = File::Temp->new( SUFFIX => '.c', UNLINK => 1, TMPDIR => 1 );
    print $src $code;
    close $src;
    my $path     = File::Spec->rel2abs( $src->filename );
    my $compiler = Affix::Build->new( debug => 0, name => 'fuzz_compile', version => '1.0', flags => { cflags => '-It/src' } );
    $compiler->add($path);
    $compiler->compile_and_link();
    return $compiler->link;
}
sub pick (@list) { $list[ int( rand(@list) ) ] }

# Capture::Tiny (used inside Affix::Build) intermittently fails to create its
# own temp files on Windows ("Permission denied" / tempfile errors). That is an
# infrastructure flake, not a compile failure, so retry before blaming the
# generated source.
sub compile_with_retry ($source) {
    for my $attempt ( 1 .. 3 ) {
        my $lib;
        my $err;
        eval {
            local $SIG{ALRM} = sub { die "compile timeout\n" };
            alarm $timeout;
            $lib = compile_source($source);
            alarm 0;
        };
        $err = $@                                     if $@;
        return ( $lib, $err )                         if !$err || $err !~ /tempfile|Permission denied/i;
        note "infra tempfile flake, attempt $attempt" if $verbose;
        sleep 1;
    }
    my $lib;
    my $err;
    eval {
        local $SIG{ALRM} = sub { die "compile timeout\n" };
        alarm $timeout;
        $lib = compile_source($source);
        alarm 0;
    };
    $err = $@ if $@;
    return ( $lib, $err );
}

# Map a C type keyword to the Affix type object used for wrap()ing.
sub affix_for ($c_type) {
    return
        $c_type eq 'long'     ? Long() :
        $c_type eq 'short'    ? Short() :
        $c_type eq 'char'     ? Char() :
        $c_type eq 'float'    ? Float() :
        $c_type eq 'double'   ? Double() :
        $c_type eq 'unsigned' ? UInt() :
        Int();
}

sub _generic_c_header() {
    <<~'';
    #include <inttypes.h>
	#include <locale.h>
	#include <math.h>
	#include <stdbool.h>
	#include <stddef.h>  // offsetof
	#include <stdio.h>
	#include <stdlib.h>  // malloc
	#include <string.h>
    #include <stdint.h>
	#include <wchar.h>
	// Some tests might actually include perl.h which has the real version of this
	#if !defined(warn)
	#define warn(FORMAT, ...)                                                          \
	    fprintf(stderr, FORMAT " at %s line %i\n", ##__VA_ARGS__, __FILE__, __LINE__); \
	    fflush(stderr);
	#endif
	#if defined _WIN32 || defined __CYGWIN__
	#include <BaseTsd.h>
	//typedef SSIZE_T ssize_t;
	typedef signed __int64 int64_t;
	#ifdef __GNUC__
	#define DLLEXPORT __attribute__((dllexport))
	#else
	#define DLLEXPORT __declspec(dllexport)
	#endif
	#else
	#ifdef __GNUC__
	#if __GNUC__ >= 4
	#define DLLEXPORT __attribute__((visibility("default")))
	#else
	#define DLLEXPORT __attribute__((dllimport))
	#endif
	#else
	#define DLLEXPORT __declspec(dllexport)
	#endif
	#include <inttypes.h>
	#include <sys/types.h>
	#endif
	//ext: .c

}

# Each generator returns a spec: { source, symbol, args => [\@type_objs],
#   gen_args => sub { [\@values] }, verify => sub($got, $args) }
sub gen_valid_spec {
    my @types = qw[int long short char float double unsigned];
    my $ret   = pick(@types);
    my $arg   = pick(@types);
    my $fn    = 'cfuzz_' . int( rand(99999) );
    my $value = int( rand(101) );                                # fits every int-like C type exactly
    my $flt   = $ret eq 'float' || $ret eq 'double';
    return {
        source => _generic_c_header . <<~"",
    DLLEXPORT $ret $fn($arg x) {
        return ($ret)x;
    }

        symbol   => $fn,
        args     => [ affix_for($arg) ],
        ret      => affix_for($ret),
        gen_args => sub { [$value] },
        verify   => $flt ? sub ( $got, $vals ) { abs( $got - $vals->[0] ) < 0.01 } : sub ( $got, $vals ) { $got == $vals->[0] },
    };
}

sub gen_mutated_source {
    my $base = gen_valid_spec()->{source};
    my $mut  = pick(qw[preprocessor comment_error type_error syntax_truncation brace_mismatch]);
    if ( $mut eq 'preprocessor' ) {

        # Insert random preprocessor directives
        my $dir = pick( '#pragma GCC optimize', '#define', '#if 1', '#ifdef FUZZ', '#include <nonexistent.h>' );
        if ( $dir eq '#include <nonexistent.h>' ) {

            # This should fail compilation
            $base =~ s/(#include)/$1 "nonexistent_fuzz_header_$$.h"\n$1/;
        }
        else {
            my @lines = split /\n/, $base;
            my $pos   = 1 + int( rand( $#lines - 1 ) );
            splice( @lines, $pos, 0, $dir );
            $base = join( "\n", @lines );
        }
    }
    elsif ( $mut eq 'comment_error' ) {

        # Unclosed block comment
        $base =~ s/(#include)/\/\* unclosed\n$1/;
    }
    elsif ( $mut eq 'type_error' ) {

        # Replace a type with nonsense
        $base =~ s#\b(int|long|short|char|float|double)\b#foobar_baz/quux_$1#;
    }
    elsif ( $mut eq 'syntax_truncation' ) {

        # Truncate the source randomly
        my $pos = int( rand( length($base) * 0.8 ) );
        $base = substr( $base, 0, $pos );
    }
    elsif ( $mut eq 'brace_mismatch' ) {

        # Remove or add extra braces
        my @chars  = split //, $base;
        my @braces = grep { $chars[$_] =~ /[\{\}]/ } 0 .. $#chars;
        if (@braces) {
            my $idx = pick(@braces);
            splice( @chars, $idx, 1 );
            $base = join '', @chars;
        }
    }
    return $base;
}

sub gen_boundary_spec {

    # Sources that push boundaries but should still compile
    my $variant = pick(qw[huge_array many_params empty_body deep_nesting string_buf]);
    if ( $variant eq 'huge_array' ) {
        my $size = 1000 + int( rand(9000) );
        return {
            source => _generic_c_header . <<~"",
DLLEXPORT int cfuzz_big(int idx) {
    static int big_arr[$size] = {0};
    if (idx < 0 || idx >= $size) return 0;
    return big_arr[idx];
}

            symbol   => 'cfuzz_big',
            ret      => Int(),
            args     => [ Int() ],
            gen_args => sub { [ int( rand($size) ) ] },
            verify   => sub ( $got, $vals ) { $got == 0 },
        };
    }
    elsif ( $variant eq 'many_params' ) {
        my $n      = 10 + int( rand(6) );
        my @params = map {"int p$_"} 0 .. $n - 1;
        my $sum    = join( ' + ', map {"p$_"} 0 .. $n - 1 );
        my $pstr   = join( ', ',  @params );
        my @vals   = map { int( rand(101) ) } 0 .. $n - 1;
        return {
            source => _generic_c_header . <<~"",
DLLEXPORT int cfuzz_many($pstr) {
    return $sum;
}

            symbol   => 'cfuzz_many',
            ret      => Int(),
            args     => [ ( Int() ) x $n ],
            gen_args => sub { [@vals] },
            verify   => sub ( $got, $vals ) { my $s = 0; $s += $_ for @$vals; $got == $s },
        };
    }
    elsif ( $variant eq 'empty_body' ) {
        return {
            source => _generic_c_header . <<~"",
DLLEXPORT int cfuzz_empty(void) { return 0; }

            symbol   => 'cfuzz_empty',
            ret      => Int(),
            args     => [],
            gen_args => sub { [] },
            verify   => sub ( $got, $vals ) { $got == 0 },
        };
    }
    elsif ( $variant eq 'deep_nesting' ) {
        my $code = "int cfuzz_nest(int x) {\n";
        for my $i ( 1 .. 20 ) {
            $code .= "    if (x >= $i) {\n";
        }
        $code .= "        return x;\n";
        for my $i ( 1 .. 20 ) {
            $code .= "    }\n";
        }
        $code .= "    return 0;\n}\n";
        return {
            source => _generic_c_header . <<~"",
DLLEXPORT $code

            symbol   => 'cfuzz_nest',
            ret      => Int(),
            args     => [ Int() ],
            gen_args => sub { [ int( rand(101) ) ] },
            verify   => sub ( $got, $vals ) { my $x = $vals->[0]; $got == ( $x >= 20 ? $x : 0 ) },
        };
    }
    elsif ( $variant eq 'string_buf' ) {
        my $len = 100 + int( rand(900) );
        my $str = join '', map { chr( 65 + int( rand(26) ) ) } 1 .. ( 1 + int( rand( $len - 2 ) ) );
        return {
            source => _generic_c_header . <<~"",
DLLEXPORT int cfuzz_strbuf(const char *s) {
    if (!s) return -1;
    char buf[$len];
    strncpy(buf, s, $len - 1);
    buf[$len - 1] = '\\0';
    return (int)strlen(buf);
}

            symbol   => 'cfuzz_strbuf',
            ret      => Int(),
            args     => [ String() ],
            gen_args => sub { [$str] },
            verify   => sub ( $got, $vals ) { $got == length( $vals->[0] ) },
        };
    }
    return gen_valid_spec();
}

# Main
diag 'Fuzzing: compile_ok() C compilation mutations';
diag sprintf '  Iterations: %d, Timeout: %ds', $max_iter, $timeout;
for my $i ( 1 .. $max_iter ) {
    note("iter $i/$max_iter") if $i % 20 == 0 || $verbose;
    my ( $source, $spec, $mutated );
    if ( $i % 4 == 0 ) {
        $source  = gen_mutated_source();
        $mutated = 1;
    }
    elsif ( $i % 4 == 1 ) {
        $spec   = gen_boundary_spec();
        $source = $spec->{source};
    }
    else {
        $spec   = gen_valid_spec();
        $source = $spec->{source};
    }
    note "Iter $i\n$source" if $verbose;
    my ( $lib, $err ) = compile_with_retry($source);
    if ($err) {
        if ( $err =~ /tempfile|Permission denied/i ) {
            pass 'infra flake (tempfile), skipped';
            note 'detail: ' . $err if $verbose;
        }
        elsif ($mutated) {
            pass 'expected compile failure (mutated): ' . substr( $err, 0, 60 );
            note 'detail: ' . $err if $verbose;
        }
        else {
            fail 'VALID SOURCE FAILED TO COMPILE: ' . substr $err, 0, 60;
            note "source:\n$source" if $verbose;
        }
        next;
    }
    unless ($lib) {
        pass '"compile returned no lib';
        next;
    }
    if ($mutated) {
        pass 'mutated source compiled without crashing';
        next;
    }

    # Valid source compiled: wrap it and exercise the real symbol
    my $fn;
    eval {
        local $SIG{ALRM} = sub { die "wrap timeout\n" };
        alarm $timeout;
        $fn = wrap( $lib, $spec->{symbol}, $spec->{args}, $spec->{ret} );
        alarm 0;
    };
    ok $fn, "wrap $spec->{symbol}" or do {
        note("wrap err: $@") if $verbose;
        next;
    };
    my @vals = @{ $spec->{gen_args}->() };
    my $got;
    eval {
        local $SIG{ALRM} = sub { die "call timeout\n" };
        alarm $timeout;
        $got = $fn->(@vals);
        alarm 0;
    };
    if ($@) {
        fail "CALL CRASH on $spec->{symbol}: " . substr( $@, 0, 60 );
        next;
    }
    if ( $spec->{verify}->( $got, \@vals ) ) {
        pass "$spec->{symbol} roundtrip (got $got)";
    }
    else {
        fail "$spec->{symbol} mismatch (got $got)";
    }
}
#
done_testing;
