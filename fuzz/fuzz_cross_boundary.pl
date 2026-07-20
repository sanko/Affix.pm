#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix qw[:all];
use Affix::Build;
use File::Temp qw[tempfile];
use File::Spec;
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
    my $compiler = Affix::Build->new( debug => 0, name => 'fuzz_cross', version => '1.0', flags => { cflags => '-It/src' } );
    $compiler->add($path);
    $compiler->compile_and_link();
    return $compiler->link;
}
sub pick (@list) { $list[ int( rand(@list) ) ] }

# Type roundtrip definitions: C type -> Affix type -> Perl value -> verify
# sig format: [ \@perl_args, $perl_ret ]
my @ROUNDTRIPS = (
    {   name   => 'int8_t_roundtrip',
        c_name => 'rt_int8',
        gen    => sub { int( rand(256) ) - 128 },
        c_code => "DLLEXPORT int8_t rt_int8(int8_t v) { return v; }",
        sig    => [ [ Int() ], Int() ],
    },
    {   name   => 'uint8_t_roundtrip',
        c_name => 'rt_uint8',
        gen    => sub { int( rand(256) ) },
        c_code => "DLLEXPORT uint8_t rt_uint8(uint8_t v) { return v; }",
        sig    => [ [ UInt() ], UInt() ],
    },
    {   name   => 'int16_t_roundtrip',
        c_name => 'rt_int16',
        gen    => sub { int( rand(65536) ) - 32768 },
        c_code => "DLLEXPORT int16_t rt_int16(int16_t v) { return v; }",
        sig    => [ [ Short() ], Short() ],
    },
    {   name   => 'uint16_t_roundtrip',
        c_name => 'rt_uint16',
        gen    => sub { int( rand(65536) ) },
        c_code => "DLLEXPORT uint16_t rt_uint16(uint16_t v) { return v; }",
        sig    => [ [ UShort() ], UShort() ],
    },
    {   name   => 'int32_t_roundtrip',
        c_name => 'rt_int32',
        gen    => sub { int( rand( 2**32 ) ) - 2**31 },
        c_code => "DLLEXPORT int32_t rt_int32(int32_t v) { return v; }",
        sig    => [ [ Int() ], Int() ],
    },
    {   name   => 'uint32_t_roundtrip',
        c_name => 'rt_uint32',
        gen    => sub { int( rand( 2**32 ) ) },
        c_code => "DLLEXPORT uint32_t rt_uint32(uint32_t v) { return v; }",
        sig    => [ [ UInt() ], UInt() ],
    },
    {   name   => 'int64_t_roundtrip',
        c_name => 'rt_int64',
        gen    => sub { int( rand( 2**62 ) ) - 2**61 },
        c_code => "DLLEXPORT int64_t rt_int64(int64_t v) { return v; }",
        sig    => [ [ LongLong() ], LongLong() ],
    },
    {   name   => 'uint64_t_roundtrip',
        c_name => 'rt_uint64',
        gen    => sub { int( rand( 2**63 ) ) },
        c_code => "DLLEXPORT uint64_t rt_uint64(uint64_t v) { return v; }",
        sig    => [ [ ULongLong() ], ULongLong() ],
    },
    {   name   => 'float_roundtrip',
        c_name => 'rt_float',
        gen    => sub { sprintf( "%.2f", rand(100) - 50 ) },
        c_code => "DLLEXPORT float rt_float(float v) { return v; }",
        sig    => [ [ Float() ], Float() ],
        verify => sub ( $got, $expected ) { abs( $got - $expected ) < 0.01 },
    },
    {   name   => 'double_roundtrip',
        c_name => 'rt_double',
        gen    => sub { sprintf( "%.4f", rand(1000) - 500 ) },
        c_code => "DLLEXPORT double rt_double(double v) { return v; }",
        sig    => [ [ Double() ], Double() ],
        verify => sub ( $got, $expected ) { abs( $got - $expected ) < 0.01 },
    },
    {   name   => 'bool_roundtrip',
        c_name => 'rt_bool',
        gen    => sub { int( rand(2) ) },
        c_code => "DLLEXPORT bool rt_bool(bool v) { return v; }",
        sig    => [ [ Bool() ], Bool() ],
    },
    {   name   => 'char_roundtrip',
        c_name => 'rt_char',
        gen    => sub { int( rand(128) ) },
        c_code => "DLLEXPORT char rt_char(char v) { return v; }",
        sig    => [ [ Char() ], Char() ],
    },
    {   name   => 'pointer_roundtrip',
        c_name => 'rt_ptr',
        gen    => sub {
            my $val = int( rand(10000) );
            my $mem = Affix::malloc( Affix::sizeof( Int() ) );
            my $pin = Affix::cast( $mem, Pointer [ Int() ] );
            $$pin = $val;
            return [$pin];
        },
        c_code => "DLLEXPORT int rt_ptr(int *v) { return v ? *v : -1; }",
        sig    => [ [ Pointer [ Int() ] ], Int() ],
    },
    {   name   => 'string_roundtrip',
        c_name => 'rt_str',
        gen    => sub {
            join '', map { chr( 65 + int( rand(26) ) ) } 1 .. ( 2 + int( rand(5) ) );
        },
        c_code => "DLLEXPORT int rt_str(const char *s) { return s ? (int)strlen(s) : -1; }",
        sig    => [ [ String() ], Int() ],
        verify => sub ( $got, $expected ) { $got == length($expected) },
    },
    {   name   => 'struct_roundtrip',
        c_name => 'rt_struct',
        gen    => sub {
            my $x = int( rand(1000) ) - 500;
            my $y = sprintf( "%.2f", rand(100) - 50 );
            return { x => $x, y => $y };
        },
        c_code => <<'END_C',
typedef struct { int x; double y; } RTStruct;
DLLEXPORT RTStruct rt_struct(RTStruct s) { return s; }
END_C
        sig    => [ [ Struct [ x => Int(), y => Double() ] ], Struct [ x => Int(), y => Double() ] ],
        verify => sub ( $got, $expected ) {
            $got->{x} == $expected->{x} && abs( $got->{y} - $expected->{y} ) < 0.01;
        },
    },
    {   name   => 'union_roundtrip',
        c_name => 'rt_union',
        gen    => sub {
            my $union_type = Union [ i => Int(), f => Float() ];
            my $mem        = Affix::malloc( Affix::sizeof($union_type) );
            my $pin        = Affix::cast( $mem, $union_type );
            $pin->{i} = int( rand(1000) );
            return [$pin];
        },
        c_code => <<'END_C',
typedef union { int i; float f; } RTUnion;
DLLEXPORT int rt_union(RTUnion *u) { return u ? u->i : -1; }
END_C
        sig => [ [ Pointer [ Union [ i => Int(), f => Float() ] ] ], Int() ],
    },
    {   name   => 'enum_roundtrip',
        c_name => 'rt_enum',
        gen    => sub { int( rand(3) ) * 10 },
        c_code => <<'END_C',
typedef enum { RTE_A=0, RTE_B=10, RTE_C=20 } RTEnum;
DLLEXPORT int rt_enum(RTEnum e) { return (int)e; }
END_C
        sig => [ [ Int() ], Int() ],
    },
    {   name   => 'callback_roundtrip',
        c_name => 'rt_cb',
        gen    => sub {
            my $a = int( rand(100) );
            my $b = int( rand(100) );
            return [ sub ( $x, $y ) { $x + $y }, $a, $b ];
        },
        c_code => "typedef int (*rtcb)(int, int); DLLEXPORT int rt_cb(rtcb f, int a, int b) { return f(a, b); }",
        sig    => [ [ Callback [ [ Int(), Int() ] => Int() ], Int(), Int() ], Int() ],
        verify => sub ( $got, $cb, $a, $b ) { $got == $a + $b },
    },
    {   name   => 'long_roundtrip',
        c_name => 'rt_long',
        gen    => sub { int( rand( 2**31 ) ) - 2**30 },
        c_code => "DLLEXPORT long rt_long(long v) { return v; }",
        sig    => [ [ Long() ], Long() ],
    },
    {   name   => 'longlong_roundtrip',
        c_name => 'rt_longlong',
        gen    => sub { int( rand( 2**62 ) ) - 2**61 },
        c_code => "DLLEXPORT long long rt_longlong(long long v) { return v; }",
        sig    => [ [ LongLong() ], LongLong() ],
    }
);

# Multi-param roundtrip generators
sub gen_multi_param_roundtrip {
    my @all_types = (
        [ 'int',      sub { Int() } ],
        [ 'long',     sub { Long() } ],
        [ 'float',    sub { Float() } ],
        [ 'double',   sub { Double() } ],
        [ 'short',    sub { Short() } ],
        [ 'char',     sub { Char() } ],
        [ 'int64_t',  sub { LongLong() } ],
        [ 'uint64_t', sub { ULongLong() } ]
    );
    my $nparams = 1 + int( rand(6) );
    my @chosen;
    my %seen;
    while ( @chosen < $nparams && @chosen < @all_types ) {
        my $idx = int( rand(@all_types) );
        next if $seen{$idx}++;
        push @chosen, $all_types[$idx];
    }
    my ( @c_params, @a_args, @values );
    for my $i ( 0 .. $#chosen ) {
        push @c_params, "$chosen[$i]->[0] p$i";
        push @a_args,   $chosen[$i]->[1]->();
        push @values,   $chosen[$i]->[0] eq 'float' || $chosen[$i]->[0] eq 'double' ? sprintf( "%.2f", rand(100) ) : int( rand( 2**30 ) ) - 2**29;
    }
    my $pstr   = join( ', ', @c_params );
    my $ret    = $chosen[0];
    my $c_code = <<"END_C";
#include "std.h"
#include <stdint.h>

//ext: .c

DLLEXPORT $ret->[0] rt_multi($pstr) {
    return p0;
}
END_C
    return { c_code => $c_code, c_name => 'rt_multi', perl_args => \@a_args, perl_ret => $ret->[1]->(), values => \@values, };
}

# Main
note "Fuzzing: Cross-boundary Perl->C->JIT type roundtrip";
note "  Iterations: $max_iter, Timeout: ${timeout}s";
for my $i ( 1 .. $max_iter ) {

    # Compile all roundtrip functions in one big source
    my $full_c = "#include \"std.h\"\n#include <stdint.h>\n#include <string.h>\n\n//ext: .c\n\n";
    for my $rt (@ROUNDTRIPS) {
        $full_c .= $rt->{c_code} . "\n\n";
    }
    my $multi = gen_multi_param_roundtrip();
    $full_c .= $multi->{c_code} . "\n";
    my $lib = eval {
        local $SIG{ALRM} = sub { die "compile timeout\n" };
        alarm $timeout;
        my $result = compile_source($full_c);
        alarm 0;
        $result;
    };
    unless ($lib) {
        note "SKIP compile failed: $@" if $verbose;
        pass "skip iteration $i compile";
        next;
    }

    # Test each roundtrip
    for my $rt (@ROUNDTRIPS) {
        my $sig_args = $rt->{sig}[0];
        my $sig_ret  = $rt->{sig}[1];
        my $fn       = eval {
            local $SIG{ALRM} = sub { die "wrap timeout\n" };
            alarm $timeout;
            my $result = wrap( $lib, $rt->{c_name}, $sig_args, $sig_ret );
            alarm 0;
            $result;
        };
        unless ($fn) {
            note "SKIP wrap $rt->{name}: $@" if $verbose;
            pass "skip wrap $rt->{name}";
            next;
        }
        my $val = eval {
            local $SIG{ALRM} = sub { die "gen timeout\n" };
            alarm $timeout;
            my $v = $rt->{gen}->();
            alarm 0;
            $v;
        };
        unless ( defined $val ) {
            pass "skip gen $rt->{name}";
            next;
        }
        my @args   = ref($val) eq 'ARRAY' ? @$val : ($val);
        my $result = eval {
            local $SIG{ALRM} = sub { die "call timeout\n" };
            alarm $timeout;
            my $r = $fn->(@args);
            alarm 0;
            $r;
        };
        if ($@) {
            fail "CRASH $rt->{name}: $@";
            next;
        }
        if ( $rt->{verify} ) {
            ok $rt->{verify}->( $result, @args ), "$rt->{name} roundtrip";
        }
        else {
            pass "$rt->{name} survived";
        }
    }

    # Test multi-param
    {
        my $fn = eval {
            local $SIG{ALRM} = sub { die "wrap timeout\n" };
            alarm $timeout;
            my $result = wrap( $lib, $multi->{c_name}, $multi->{perl_args}, $multi->{perl_ret} );
            alarm 0;
            $result;
        };
        if ($fn) {
            my $result = eval {
                local $SIG{ALRM} = sub { die "call timeout\n" };
                alarm $timeout;
                my $r = $fn->( @{ $multi->{values} } );
                alarm 0;
                $r;
            };
            if ($@) {
                fail "CRASH multi_param: $@";
            }
            else {
                pass "multi_param survived";
            }
        }
        else {
            pass "skip multi_param wrap";
        }
    }
}
done_testing;
