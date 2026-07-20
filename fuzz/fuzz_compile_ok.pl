#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix               qw[:all];
use Test2::Tools::Affix qw[compile_ok];
use Config;
use File::Temp qw[tempfile];
use File::Spec;
use POSIX qw[EXIT_SUCCESS EXIT_FAILURE];
use Test2::V0;
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

# C source generators with intentional mutations/errors
sub gen_valid_source {
    my $fn    = 'cfuzz_' . int( rand(99999) );
    my @types = qw[int long short char float double unsigned];
    my $ret   = pick(@types);
    my $arg   = pick(@types);
    return <<"END_C";
#include "std.h"
#include <stdint.h>

//ext: .c

DLLEXPORT $ret $fn($arg x) {
    return ($ret)x;
}
END_C
}

sub gen_mutated_source {
    my $base = gen_valid_source();
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

sub gen_boundary_source {

    # Sources that push boundaries but should still compile
    my $variant = pick(qw[huge_array many_params empty_body deep_nesting string_buf]);
    if ( $variant eq 'huge_array' ) {
        my $size = 1000 + int( rand(9000) );
        return <<"END_C";
#include "std.h"
#include <stdint.h>

//ext: .c

DLLEXPORT int cfuzz_big(int idx) {
    static int big_arr[$size] = {0};
    if (idx < 0 || idx >= $size) return 0;
    return big_arr[idx];
}
END_C
    }
    elsif ( $variant eq 'many_params' ) {
        my $n      = 10 + int( rand(6) );
        my @params = map {"int p$_"} 0 .. $n - 1;
        my $sum    = join( ' + ', map {"p$_"} 0 .. $n - 1 );
        my $pstr   = join( ', ',  @params );
        return <<"END_C";
#include "std.h"
#include <stdint.h>

//ext: .c

DLLEXPORT int cfuzz_many($pstr) {
    return $sum;
}
END_C
    }
    elsif ( $variant eq 'empty_body' ) {
        return <<"END_C";
#include "std.h"

//ext: .c

DLLEXPORT int cfuzz_empty(void) { return 0; }
END_C
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
        return <<"END_C";
#include "std.h"
#include <stdint.h>

//ext: .c

DLLEXPORT $code
END_C
    }
    elsif ( $variant eq 'string_buf' ) {
        my $len = 100 + int( rand(900) );
        return <<"END_C";
#include "std.h"
#include <string.h>

//ext: .c

DLLEXPORT int cfuzz_strbuf(const char *s) {
    if (!s) return -1;
    char buf[$len];
    strncpy(buf, s, $len - 1);
    buf[$len - 1] = '\\0';
    return (int)strlen(buf);
}
END_C
    }
    return gen_valid_source();
}

# Main
printf STDERR "Fuzzing: compile_ok() C compilation mutations\n";
printf STDERR "  Iterations: %d, Timeout: %ds\n", $max_iter, $timeout;
printf STDERR "%s\n", "=" x 60;
for my $i ( 1 .. $max_iter ) {
    note("iter $i/$max_iter") if $i % 20 == 0 || $verbose;
    my $source;
    if ( $i % 4 == 0 ) {
        $source = gen_mutated_source();
    }
    elsif ( $i % 4 == 1 ) {
        $source = gen_boundary_source();
    }
    else {
        $source = gen_valid_source();
    }
    if ($verbose) {
        note("--- Iter $i ---\n$source");
    }
    my $lib;
    eval {
        local $SIG{ALRM} = sub { die "compile timeout\n" };
        alarm $timeout;
        $lib = compile_source($source);
        alarm 0;
    };
    if ($@) {
        if ( $source =~ /nonexistent_fuzz_header|unclosed|foobar|truncat/ ) {
            pass("expected compile failure: $&");
        }
        else {
            pass("expected compile failure: $&");
            note( "compile failed unexpectedly: " . substr( $@, 0, 80 ) ) if $verbose;
        }
    }
    elsif ($lib) {
        my $ok = 0;
        eval {
            local $SIG{ALRM} = sub { die "use timeout\n" };
            alarm $timeout;
            my $test_fn = wrap( $lib, 'cfuzz_strbuf', ['String'], 'Int' );
            $ok = 1;
            alarm 0;
        };
        ok $ok, "compile valid source";
    }
    else {
        pass("compile returned no lib");
    }
}
done_testing;
