#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix               qw[:all];
use Test2::Tools::Affix qw[:all];
use Test2::V0 -no_srand => 1;
use Config;
$|++;
#
my $max_iter = $ENV{FUZZ_MAX_ITER} // 1000;
my $timeout  = $ENV{FUZZ_TIMEOUT}  // 5;
my $verbose  = $ENV{FUZZ_VERBOSE}  // 0;
sub compile_source ($code) { compile_ok( <<~'' . $code ) }
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


# Type catalog: { c_type, infix_sig, perl_type, gen_value }
# Ranges are derived from sizeof() at runtime so they match the actual platform.
my @PRIMITIVES;
{
    my @signed_ints = (
        [ 'int8_t',    'sint8',    sub { SInt8() } ],
        [ 'int16_t',   'sint16',   sub { SInt16() } ],
        [ 'int32_t',   'sint32',   sub { SInt32() } ],
        [ 'int64_t',   'sint64',   sub { SInt64() } ],
        [ 'char',      'char',     sub { Char() } ],
        [ 'short',     'short',    sub { Short() } ],
        [ 'int',       'int',      sub { Int() } ],
        [ 'long',      'long',     sub { Long() } ],
        [ 'long long', 'longlong', sub { LongLong() } ],
    );
    my @unsigned_ints = (
        [ 'uint8_t',            'uint8',     sub { UInt8() } ],
        [ 'uint16_t',           'uint16',    sub { UInt16() } ],
        [ 'uint32_t',           'uint32',    sub { UInt32() } ],
        [ 'uint64_t',           'uint64',    sub { UInt64() } ],
        [ 'unsigned char',      'uchar',     sub { UChar() } ],
        [ 'unsigned short',     'ushort',    sub { UShort() } ],
        [ 'unsigned int',       'uint',      sub { UInt() } ],
        [ 'unsigned long',      'ulong',     sub { ULong() } ],
        [ 'unsigned long long', 'ulonglong', sub { ULongLong() } ],
    );
    my @floats = ( [ 'float', 'float', sub { Float() } ], [ 'double', 'double', sub { Double() } ], );
    my @bool   = ( [ 'bool',  'bool',  sub { Bool() } ], );
    for my $entry (@signed_ints) {
        my ( $c, $sig, $perl ) = @$entry;
        my $bits = sizeof( $perl->() ) * 8;
        my $half = 2**( $bits - 1 );
        push @PRIMITIVES, { c => $c, sig => $sig, perl => $perl, gen => sub { int( rand( $half * 2 ) ) - $half } };
    }
    for my $entry (@unsigned_ints) {
        my ( $c, $sig, $perl ) = @$entry;
        my $bits = sizeof( $perl->() ) * 8;
        my $max  = 2**$bits;
        push @PRIMITIVES, { c => $c, sig => $sig, perl => $perl, gen => sub { int( rand($max) ) } };
    }
    for my $entry (@floats) {
        my ( $c, $sig, $perl ) = @$entry;
        my $prec = sizeof( $perl->() ) == 4 ? 2 : 4;
        push @PRIMITIVES, { c => $c, sig => $sig, perl => $perl, gen => sub { sprintf( "%.*f", $prec, rand(100) - 50 ) } };
    }
    for my $entry (@bool) {
        my ( $c, $sig, $perl ) = @$entry;
        push @PRIMITIVES, { c => $c, sig => $sig, perl => $perl, gen => sub { int( rand(2) ) } };
    }
}
my @SPECIAL = (
    { c => 'size_t',    sig => 'size_t',  perl => sub { Size_t() },  gen => sub { int( rand(1000) ) } },
    { c => 'ssize_t',   sig => 'ssize_t', perl => sub { SSize_t() }, gen => sub { int( rand(1000) ) } },
    { c => 'ptrdiff_t', sig => 'long',    perl => sub { Long() },    gen => sub { int( rand(200) ) - 100 } }
);
sub pick (@list) { $list[ int( rand(@list) ) ] }

sub pick_n ( $n, @list ) {
    my @shuffled = sort { rand(1) <=> rand(1) } @list;
    return @shuffled[ 0 .. $n - 1 ] if $n <= @list;
    return @shuffled;
}

sub unique_name {
    state $counter = 0;
    return 'fuzz_fn_' . $counter++;
}

# Primitive echo function (identity)
sub generate_function {
    my ($fn_name)   = @_;
    my @all         = ( @PRIMITIVES, @SPECIAL );
    my $nparams     = 1 + int( rand(4) );
    my @param_types = pick_n( $nparams, @all );
    my $ret_type    = pick(@all);
    my ( @c_params, @perl_args, @gen_values, @sig_args );
    for my $pt (@param_types) {
        my $pname = 'p' . scalar(@c_params);
        push @c_params,  "$pt->{c} $pname";
        push @perl_args, $pt->{perl}->();
        push @sig_args,  $pt->{sig};
        my $val = $pt->{gen}->();
        push @gen_values, sub {$val};
    }
    my $params_str = join( ', ', @c_params );
    my $ret_sig    = $ret_type->{sig};
    my $ret_c      = $ret_type->{c};
    my $c_code     = <<"END_C";
$ret_c $fn_name($params_str) {
    return ($ret_c) p0;
}
END_C
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => \@sig_args,
        sig_ret    => $ret_sig,
        perl_args  => \@perl_args,
        perl_ret   => $ret_type->{perl}->(),
        gen_values => \@gen_values,
    };
}

# Struct by value
sub generate_struct_fn {
    my ($fn_name) = @_;
    my $nfields   = 1 + int( rand(3) );
    my @all       = grep {
        $_->{c} ne 'float'             &&
            $_->{c} ne 'double'        &&
            $_->{c} ne 'char'          &&
            $_->{c} ne 'unsigned char' &&
            $_->{c} ne 'bool'          &&
            sizeof( $_->{perl}->() ) * 8 <= 53
    } @PRIMITIVES;
    my @fields      = pick_n( $nfields, @all );
    my $struct_name = 'S' . int( rand(99999) );
    my ( @struct_members, @struct_perl_fields, @struct_sig_fields, @c_params, @perl_args, @sig_args, @gen_values );
    for my $f (@fields) {
        my $fname = 'm' . scalar(@struct_members);
        push @struct_members,     "$f->{c} $fname;";
        push @struct_perl_fields, $fname, $f->{perl}->();
        push @struct_sig_fields,  "$fname:" . $f->{sig};
        my $pname = 'p' . scalar(@c_params);
        push @c_params,  "$f->{c} $pname";
        push @perl_args, $f->{perl}->();
        push @sig_args,  $f->{sig};
        my $val = $f->{gen}->();
        push @gen_values, sub {$val};
    }
    my $struct_body = join( ' ', @struct_members );
    my $struct_def  = "typedef struct { $struct_body } $struct_name;";
    my $struct_sig  = '{' . join( ',', @struct_sig_fields ) . '}';
    my $params_str  = join( ', ', @c_params );
    my @inits       = map {"r.m$_ = p$_;"} 0 .. $#fields;
    my $init_block  = join( "\n    ", @inits );
    my $c_code      = <<"END_C";
$struct_def

$struct_name $fn_name($params_str) {
    $struct_name r = {0};
    $init_block
    return r;
}
END_C
    my @field_names = map { 'm' . $_ } 0 .. $#fields;
    my @check_gens  = @gen_values;
    my @check_names = @field_names;
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => \@sig_args,
        sig_ret    => $struct_sig,
        perl_args  => \@perl_args,
        perl_ret   => Struct [@struct_perl_fields],
        gen_values => \@gen_values,
        verify     => sub ($result) {
            my %expected;
            @expected{@check_names} = map { $_->() } @check_gens;
            local $ENV{TABLE_TERM_SIZE} = 200;
            my $ok = is( $result, \%expected, "struct fields match" );
            unless ($ok) {
                diag "GOT:     " . join( ", ", map {"$_=$result->{$_}"} sort keys %$result );
                diag "EXPECTED:" . join( ", ", map {"$_=$expected{$_}"} sort keys %expected );
            }
        },
    };
}

# Union (by pointer)
sub generate_union_fn {
    my ($fn_name)  = @_;
    my $union_name = 'U' . int( rand(99999) );
    my @candidates = ( @PRIMITIVES[ 0 .. 5 ] );
    my $nfields    = 2 + int( rand(3) );
    my @fields     = pick_n( $nfields, @candidates );
    my ( @union_members, @union_perl_fields, @union_sig_fields );
    for my $f (@fields) {
        my $fname = 'm' . scalar(@union_members);
        push @union_members,     "$f->{c} $fname;";
        push @union_perl_fields, $fname, $f->{perl}->();
        push @union_sig_fields,  "$fname:" . $f->{sig};
    }
    my $union_body = join( ' ', @union_members );
    my $union_sig  = '<' . join( ',', @union_sig_fields ) . '>';
    my $c_code     = <<"END_C";
typedef union { $union_body } $union_name;

int $fn_name($union_name *u) {
    return u ? (int)u->m0 : -1;
}
END_C
    my $val        = $fields[0]->{gen}->();
    my $union_type = Union [@union_perl_fields];
    my @keep_alive;
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => ["*$union_sig"],
        sig_ret    => 'int',
        perl_args  => [ Pointer [$union_type] ],
        perl_ret   => Int(),
        gen_values => [
            sub {
                my $mem = Affix::malloc( sizeof($union_type) );
                my $pin = cast( $mem, $union_type );
                $pin->{m0} = $val;
                push @keep_alive, $mem;
                return $pin;
            }
        ],
    };
}

# Enum
sub generate_enum_fn {
    my ($fn_name) = @_;
    my $nelems = 2 + int( rand(5) );
    my @elems;
    for my $i ( 0 .. $nelems - 1 ) {
        my $val = $i * 10 + int( rand(5) );
        push @elems, "    ${fn_name}_V${i} = $val,";
    }
    my $enum_def = "typedef enum {\n" . join( "\n", @elems ) . "\n} ${fn_name}_enum_t;";
    my $c_code   = <<"END_C";
$enum_def

int $fn_name(${fn_name}_enum_t e) {
    return (int)e;
}
END_C
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => ['int'],
        sig_ret    => 'int',
        perl_args  => [ Int() ],
        perl_ret   => Int(),
        gen_values => [ sub { pick( 0 .. $nelems - 1 ) * 10 } ],
    };
}

# Multi-argument stress (exceeds register count on x86_64)
sub generate_mega_arg_fn {
    my ($fn_name)   = @_;
    my $nparams     = 8 + int( rand(4) );
    my @param_types = pick_n( $nparams, @PRIMITIVES );
    my $ret_type    = pick(@PRIMITIVES);
    my ( @c_params, @perl_args, @sig_args, @gen_values, @cast_args );
    for my $pt (@param_types) {
        my $pname = 'p' . scalar(@c_params);
        push @c_params,  "$pt->{c} $pname";
        push @perl_args, $pt->{perl}->();
        push @sig_args,  $pt->{sig};
        my $val = $pt->{gen}->();
        push @gen_values, sub {$val};
        push @cast_args,  "($pt->{c})p$#c_params";
    }
    my $params_str = join( ', ',  @c_params );
    my $sum_expr   = join( ' + ', @cast_args );
    my $c_code     = <<"END_C";
$ret_type->{c} $fn_name($params_str) {
    return ($ret_type->{c})($sum_expr);
}
END_C
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => \@sig_args,
        sig_ret    => $ret_type->{sig},
        perl_args  => \@perl_args,
        perl_ret   => $ret_type->{perl}->(),
        gen_values => \@gen_values,
    };
}

# Pointer pass-through
my @POINTER_PRIMITIVES = grep { sizeof( $_->{perl}->() ) > 1 } @PRIMITIVES;

sub generate_pointer_fn {
    my ($fn_name) = @_;
    my $base      = pick(@POINTER_PRIMITIVES);
    my $c_code    = <<"END_C";
$base->{c} $fn_name($base->{c} *ptr) {
    return ptr ? *ptr : 0;
}
END_C
    my $affix_type = $base->{perl}->();
    my $ptr_type   = Pointer [$affix_type];
    my $is_float   = ( $base->{c} eq 'float' || $base->{c} eq 'double' );
    my $val        = $base->{gen}->();
    my @keep_alive;
    my $verify = $is_float ?
        sub ($result) {
        my $tol = $base->{c} eq 'float' ? 1e-5 : 1e-10;
        ok( abs( $result - $val ) <= $tol, "$fn_name ${\($base->{c})} roundtrip" );
        } :
        sub ($result) { ok( $result == $val, "$fn_name ${\($base->{c})} roundtrip" ) };
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => [ '*' . $base->{sig} ],
        sig_ret    => $base->{sig},
        perl_args  => [$ptr_type],
        perl_ret   => $affix_type,
        gen_values => [
            sub {
                my $mem = Affix::malloc( sizeof($affix_type) );
                my $pin = cast( $mem, $ptr_type );
                $$pin = $val;
                push @keep_alive, $mem;
                return $pin;
            }
        ],
        verify => $verify,
    };
}

# Callback (function pointer)
sub generate_callback_fn {
    my ($fn_name) = @_;
    my @small     = ( @PRIMITIVES[ 0 .. 2 ] );
    my $a1        = pick(@small);
    my $a2        = pick(@small);
    my $ret       = pick(@small);
    my $cb_name   = 'cb_' . int( rand(99999) );
    my $cb_sig    = "(*($a1->{sig},$a2->{sig})->$ret->{sig})";
    my $c_code    = <<"END_C";
typedef $ret->{c} (*$cb_name)($a1->{c}, $a2->{c});

$ret->{c} $fn_name($cb_name op, $a1->{c} x, $a2->{c} y) {
    return op(x, y);
}
END_C
    my $v1 = $a1->{gen}->();
    my $v2 = $a2->{gen}->();
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => [ $cb_sig, $a1->{sig}, $a2->{sig} ],
        sig_ret    => $ret->{sig},
        perl_args  => [ Callback [ [ $a1->{perl}->(), $a2->{perl}->() ] => $ret->{perl}->() ], $a1->{perl}->(), $a2->{perl}->() ],
        perl_ret   => $ret->{perl}->(),
        gen_values => [
            sub {
                sub ( $x, $y ) { $x + $y }
            },
            sub {$v1},
            sub {$v2}
        ],
    };
}

# String roundtrip
sub generate_string_fn {
    my ($fn_name) = @_;
    my $c_code = <<"END_C";
int $fn_name(const char *s) {
    return s ? (int)strlen(s) : -1;
}
END_C
    my $test_str = join '', map { chr( 65 + int( rand(26) ) ) } 1 .. ( 3 + int( rand(10) ) );
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => ['*char'],
        sig_ret    => 'int',
        perl_args  => [ String() ],
        perl_ret   => Int(),
        gen_values => [ sub {$test_str} ],
        verify     => sub ($result) { ok( $result == length($test_str), "string length roundtrip" ) },
    };
}

# Packed struct by value (tests #pragma pack + Packed() sizeof/layout)
sub generate_packed_struct_fn {
    my ($fn_name) = @_;
    my $nfields   = 2 + int( rand(3) );
    my @all       = grep {
        $_->{c} ne 'float'             &&
            $_->{c} ne 'double'        &&
            $_->{c} ne 'char'          &&
            $_->{c} ne 'unsigned char' &&
            $_->{c} ne 'bool'          &&
            sizeof( $_->{perl}->() ) * 8 <= 53
    } @PRIMITIVES;
    my @fields      = pick_n( $nfields, @all );
    my $struct_name = 'PS' . int( rand(99999) );
    my ( @struct_members, @struct_perl_fields, @struct_sig_fields, @c_params, @perl_args, @sig_args, @gen_values );
    for my $f (@fields) {
        my $fname = 'm' . scalar(@struct_members);
        push @struct_members,     "$f->{c} $fname;";
        push @struct_perl_fields, $fname, $f->{perl}->();
        push @struct_sig_fields,  "$fname:" . $f->{sig};
        my $pname = 'p' . scalar(@c_params);
        push @c_params,  "$f->{c} $pname";
        push @perl_args, $f->{perl}->();
        push @sig_args,  $f->{sig};
        my $val = $f->{gen}->();
        push @gen_values, sub {$val};
    }
    my $struct_body = join( ' ', @struct_members );
    my $struct_sig  = '!' . '{' . join( ',', @struct_sig_fields ) . '}';
    my $params_str  = join( ', ', @c_params );
    my @inits       = map {"r.m$_ = p$_;"} 0 .. $#fields;
    my $init_block  = join( "\n    ", @inits );
    my $c_code      = <<"END_C";
#pragma pack(push, 1)
typedef struct { $struct_body } $struct_name;
#pragma pack(pop)

$struct_name $fn_name($params_str) {
    $struct_name r = {0};
    $init_block
    return r;
}
END_C
    my @field_names = map { 'm' . $_ } 0 .. $#fields;
    my @check_gens  = @gen_values;
    my @check_names = @field_names;
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => \@sig_args,
        sig_ret    => $struct_sig,
        perl_args  => \@perl_args,
        perl_ret   => Packed( Struct [@struct_perl_fields] ),
        gen_values => \@gen_values,
        verify     => sub ($result) {
            my %expected;
            @expected{@check_names} = map { $_->() } @check_gens;
            local $ENV{TABLE_TERM_SIZE} = 200;
            my $ok = is( $result, \%expected, "packed struct fields match" );
            unless ($ok) {
                diag "GOT:     " . join( ", ", map {"$_=$result->{$_}"} sort keys %$result );
                diag "EXPECTED:" . join( ", ", map {"$_=$expected{$_}"} sort keys %expected );
            }
        },
    };
}

# Packed struct pointer roundtrip
sub generate_packed_struct_ptr_fn {
    my ($fn_name)   = @_;
    my $nfields     = 2 + int( rand(3) );
    my @all         = @PRIMITIVES;
    my @fields      = pick_n( $nfields, @all );
    my $struct_name = 'PP' . int( rand(99999) );
    my ( @struct_members, @struct_perl_fields, @struct_sig_fields );
    for my $f (@fields) {
        my $fname = 'm' . scalar(@struct_members);
        push @struct_members,     "$f->{c} $fname;";
        push @struct_perl_fields, $fname, $f->{perl}->();
        push @struct_sig_fields,  "$fname:" . $f->{sig};
    }
    my $struct_body = join( ' ', @struct_members );
    my $struct_sig  = '!' . '{' . join( ',', @struct_sig_fields ) . '}';
    my $first       = $fields[0];
    my $c_code      = <<"END_C";
#pragma pack(push, 1)
typedef struct { $struct_body } $struct_name;
#pragma pack(pop)

$first->{c} ${fn_name}($struct_name *s) {
    return s ? s->m0 : 0;
}
END_C
    my $val         = $first->{gen}->();
    my @perl        = @struct_perl_fields;
    my $struct_type = Packed( Struct [@perl] );
    my $is_float    = ( $first->{c} eq 'float' || $first->{c} eq 'double' );
    my @keep_alive;
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => ["*$struct_sig"],
        sig_ret    => $first->{sig},
        perl_args  => [ Pointer [$struct_type] ],
        perl_ret   => $first->{perl}->(),
        gen_values => [
            sub {
                my $mem = Affix::malloc( sizeof($struct_type) );
                my $pin = cast( $mem, $struct_type );
                $pin->{m0} = $val;
                push @keep_alive, $mem;
                return $pin;
            }
        ],
        verify => $is_float ?
            sub ($result) { ok( abs( $result - $val ) < 0.01, "roundtrip" ) }
        : sub ($result) { ok( $result == $val, "roundtrip" ) },
    };
}

# Struct pointer roundtrip
sub generate_struct_ptr_fn {
    my ($fn_name)   = @_;
    my $nfields     = 1 + int( rand(3) );
    my @fields      = pick_n( $nfields, @PRIMITIVES );
    my $struct_name = 'SP' . int( rand(99999) );
    my ( @struct_members, @struct_perl_fields, @struct_sig_fields );
    for my $f (@fields) {
        my $fname = 'm' . scalar(@struct_members);
        push @struct_members,     "$f->{c} $fname;";
        push @struct_perl_fields, $fname, $f->{perl}->();
        push @struct_sig_fields,  "$fname:" . $f->{sig};
    }
    my $struct_body = join( ' ', @struct_members );
    my $struct_sig  = '{' . join( ',', @struct_sig_fields ) . '}';
    my $first       = $fields[0];
    my $c_code      = <<"END_C";
typedef struct { $struct_body } $struct_name;

$first->{c} ${fn_name}($struct_name *s) {
    return s ? s->m0 : 0;
}
END_C
    my $val         = $first->{gen}->();
    my @perl        = @struct_perl_fields;
    my $struct_type = Struct [@perl];
    my $is_float    = ( $first->{c} eq 'float' || $first->{c} eq 'double' );
    my @keep_alive;
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => ["*$struct_sig"],
        sig_ret    => $first->{sig},
        perl_args  => [ Pointer [$struct_type] ],
        perl_ret   => $first->{perl}->(),
        gen_values => [
            sub {
                my $mem = Affix::malloc( sizeof($struct_type) );
                my $pin = cast( $mem, $struct_type );
                $pin->{m0} = $val;
                push @keep_alive, $mem;
                return $pin;
            }
        ],
        verify => $is_float ?
            sub ($result) { ok( abs( $result - $val ) < 0.01, "roundtrip" ) }
        : sub ($result) { ok( $result == $val, "roundtrip" ) },
    };
}

# Array roundtrip (tests Array[Type,N] marshalling)
sub generate_array_fn {
    my ($fn_name) = @_;
    my @ints      = grep { $_->{c} ne 'float' && $_->{c} ne 'double' && sizeof( $_->{perl}->() ) <= 2 } @PRIMITIVES;
    my $base      = pick(@ints);
    my $count     = 2 + int( rand(4) );                                                                                # 2..5 elements
    my @vals      = map { $base->{gen}->() } 1 .. $count;
    my $c_code    = <<"END_C";
int $fn_name($base->{c} arr[$count]) {
    int sum = 0;
    for (int i = 0; i < $count; i++) sum += (int)arr[i];
    return sum;
}
END_C
    my $array_type = Array [ $base->{perl}->(), $count ];
    my $expected   = 0;
    $expected += $_ for @vals;
    return {
        c_code     => $c_code,
        c_name     => $fn_name,
        sig_args   => [ "[$count:" . $base->{sig} . "]" ],
        sig_ret    => 'int',
        perl_args  => [$array_type],
        perl_ret   => Int(),
        gen_values => [ sub { \@vals } ],
        verify     => sub ($result) {
            my $ok = $result == $expected;
            unless ($ok) {
                diag "array sum: got=$result expected=$expected count=$count type=$base->{c}";
            }
            ok( $ok, "array sum roundtrip" );
        },
    };
}

# Build + verify one function
sub fuzz_one {
    my $fn_name = unique_name();
    my $variant = pick(qw[primitive struct union enum mega_arg pointer callback string struct_ptr packed_struct packed_struct_ptr array]);
    my $spec;
    if    ( $variant eq 'primitive' )         { $spec = generate_function($fn_name); }
    elsif ( $variant eq 'struct' )            { $spec = generate_struct_fn($fn_name); }
    elsif ( $variant eq 'union' )             { $spec = generate_union_fn($fn_name); }
    elsif ( $variant eq 'enum' )              { $spec = generate_enum_fn($fn_name); }
    elsif ( $variant eq 'mega_arg' )          { $spec = generate_mega_arg_fn($fn_name); }
    elsif ( $variant eq 'pointer' )           { $spec = generate_pointer_fn($fn_name); }
    elsif ( $variant eq 'callback' )          { $spec = generate_callback_fn($fn_name); }
    elsif ( $variant eq 'string' )            { $spec = generate_string_fn($fn_name); }
    elsif ( $variant eq 'struct_ptr' )        { $spec = generate_struct_ptr_fn($fn_name); }
    elsif ( $variant eq 'packed_struct' )     { $spec = generate_packed_struct_fn($fn_name); }
    elsif ( $variant eq 'packed_struct_ptr' ) { $spec = generate_packed_struct_ptr_fn($fn_name); }
    elsif ( $variant eq 'array' )             { $spec = generate_array_fn($fn_name); }
    return unless $spec;
    note "--- Variant: $variant ---"                                                             if $verbose;
    note "C Code:\n$spec->{c_code}"                                                              if $verbose;
    note sprintf( "infix sig: (%s)->%s", join( ',', @{ $spec->{sig_args} } ), $spec->{sig_ret} ) if $verbose;
    subtest "$variant $spec->{c_name}" => sub {

        # Compile
        my $lib = eval {
            local $SIG{ALRM} = sub { die "compile timeout\n" };
            alarm $timeout;
            my $result = compile_source( $spec->{c_code} );
            alarm 0;
            $result;
        };
        unless ($lib) {
            note "SKIP compile: " . substr( $@ // '', 0, 60 );
            skip_all 1, "$variant compile";
            return;
        }
        ok my $fn = wrap( $lib, $spec->{c_name}, $spec->{perl_args}, $spec->{perl_ret} ), 'wrap';

        # Generate call arguments
        my @call_args;
        my $gen_ok = eval {
            for my $gv ( @{ $spec->{gen_values} } ) {
                push @call_args, $gv->();
            }
            1;
        };
        unless ($gen_ok) {
            note $@;
        }
        ok $gen_ok, 'gen_values';

        # Call
        my $result = eval {
            local $SIG{ALRM} = sub { die "call timeout\n" };
            alarm $timeout;
            my $r = $fn->(@call_args);
            alarm 0;
            $r;
        };
        if ($@) {
            fail "CRASH variant=$variant name=$spec->{c_name}: $@";
            return;
        }

        # Verify
        if ( $spec->{verify} ) {
            $spec->{verify}->($result);
        }
        else {
            pass "$variant $spec->{c_name} survived (result=$result)";
        }
    };
}
#
note 'Fuzzing: Compile -> Load -> Affix -> Call -> Verify ABI';
note "  Iterations: $max_iter, Timeout: ${timeout}s";
fuzz_one() for 1 .. $max_iter;
#
done_testing;
