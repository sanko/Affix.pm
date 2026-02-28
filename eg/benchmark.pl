use v5.40;
no warnings 'experimental';
use blib;
use Affix               qw[:all];
use Test2::Tools::Affix qw[:all];
use Config;
use Benchmark qw[:all];
$|++;

# Conditionally load FFI::Platypus and Inline::C
my ( $has_platypus, $has_inline_c );

BEGIN {
    try {
        builtin::load_module 'FFI::Platypus';
        FFI::Platypus->import();
        $has_platypus = 1;
        diag 'FFI::Platypus found, including it in benchmarks.';
    }
    catch ($e) {
        diag 'FFI::Platypus not found, skipping its benchmarks.';
    }
    try {
        builtin::load_module 'Inline';
        Inline->import();
        $has_inline_c = 1;
        diag 'Inline::C found, including it in benchmarks.';
    }
    catch ($e) {
        diag 'Inline::C not found, skipping its benchmarks.';
    }
}

# Prepare a C library for benchmarking
my $C_CODE = <<'END_C';
#include "std.h"
//ext: .c
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef struct {
    int32_t a, b, c, d;
} BenchStruct;

typedef struct {
    BenchStruct nested;
    int32_t id;
} OuterStruct;

typedef enum {
    RED = 1,
    GREEN = 2,
    BLUE = 3
} Color;

DLLEXPORT double fast_pow(double x, double y) { return x + y; }
DLLEXPORT double benchmark_sin(double x) { return sin(x); }
DLLEXPORT void* get_ptr() { static int32_t dummy = 42; return &dummy; }
DLLEXPORT int32_t take_ptr(int32_t* p) { return *p; }
DLLEXPORT BenchStruct get_struct() { static BenchStruct s = {1, 2, 3, 4}; return s; }
DLLEXPORT OuterStruct get_outer() { static OuterStruct o = {{10, 20, 30, 40}, 99}; return o; }
DLLEXPORT int32_t take_enum(Color c) { return (int32_t)c; }
DLLEXPORT Color return_enum(int32_t v) { return (Color)v; }

#if defined(__SIZEOF_INT128__)
typedef __int128 int128_t;
DLLEXPORT int128_t add128(int128_t a, int128_t b) { return a + b; }
#endif
END_C
my $lib  = compile_ok($C_CODE);
my $libm = '' . libm();

# affix Bindings
affix $lib, 'fast_pow',      [ Double, Double ] => Double;
affix $lib, 'benchmark_sin', [Double]           => Double;
affix $lib, 'get_ptr',       []                 => Pointer [Int];
affix $lib, 'take_ptr',      [ Pointer [Int] ]  => Int;
affix $lib, 'get_struct',    []                 => Struct [ a => Int, b => Int, c => Int, d => Int ];
affix $lib, 'get_outer',     []                 => Struct [ nested => Struct [ a => Int, b => Int, c => Int, d => Int ], id => Int ];
typedef Color => Enum [ [ RED => 1 ], [ GREEN => 2 ], [ BLUE => 3 ] ];
affix $lib, 'take_enum',   [ Color() ] => Int;
affix $lib, 'return_enum', [Int]       => Color();

# Affix wrap Variants
my $wrap_sin   = wrap $lib,        'benchmark_sin', [Double], Double;
my $direct_sin = direct_wrap $lib, 'benchmark_sin', [Double], Double;

# FFI::Platypus Bindings
my ( $ffi, $platypus_sin_func );
if ($has_platypus) {
    $ffi = FFI::Platypus->new( api => 2, lib => "$lib" );
    $ffi->attach( [ fast_pow      => 'platypus_fast_pow' ],    [ 'double', 'double' ] => 'double' );
    $ffi->attach( [ benchmark_sin => 'platypus_sin' ],         ['double']             => 'double' );
    $ffi->attach( [ get_ptr       => 'platypus_get_ptr' ],     []                     => 'opaque' );
    $ffi->attach( [ take_ptr      => 'platypus_take_ptr' ],    ['opaque']             => 'int' );
    $ffi->attach( [ take_enum     => 'platypus_take_enum' ],   ['int']                => 'int' );
    $ffi->attach( [ return_enum   => 'platypus_return_enum' ], ['int']                => 'int' );
    $platypus_sin_func = $ffi->function( 'benchmark_sin', ['double'] => 'double' );

    package BenchStruct {
        use FFI::Platypus::Record;
        record_layout( sint32 => 'a', sint32 => 'b', sint32 => 'c', sint32 => 'd', );
    }
    $ffi->type( 'record(BenchStruct)' => 'BenchStruct' );
    $ffi->attach( [ get_struct => 'platypus_get_struct' ], [] => 'BenchStruct' );
}

# Inline::C Bindings
Inline->import( C => <<'END_OF_C' ) if $has_inline_c;
#include <math.h>
double inline_sin(double x) { return sin(x); }
END_OF_C

# Verification
my $num = 1.5;
subtest verify => sub {
    is benchmark_sin($num), float( sin($num), tolerance => 0.000001 ), 'affix correctly calculates sin';
    is $wrap_sin->($num),   float( sin($num), tolerance => 0.000001 ), 'wrap correctly calculates sin';
    is $direct_sin->($num), float( sin($num), tolerance => 0.000001 ), 'direct_wrap correctly calculates sin';
    if ($has_platypus) {
        is platypus_sin($num),         float( sin($num), tolerance => 0.000001 ), 'platypus [attach] correctly calculates sin';
        is $platypus_sin_func->($num), float( sin($num), tolerance => 0.000001 ), 'platypus [function] correctly calculates sin';
    }
    if ($has_inline_c) {
        is inline_sin($num), float( sin($num), tolerance => 0.000001 ), 'inline correctly calculates sin';
    }
};

# Benchmarks
my $bench_count = -5;
my $ptr         = malloc(4);
$$ptr = 123;
subtest 'Core Overhead: Primitives' => sub {
    diag 'Comparing Primitive Call Overhead (double + double -> double)...';
    my %marks = ( 'Affix' => sub { fast_pow( 1.5, 2.5 ) }, );
    $marks{'Platypus'} = sub { platypus_fast_pow( 1.5, 2.5 ) }
        if $has_platypus;
    cmpthese( $bench_count, \%marks );
    pass 'finished';
};
subtest 'Math Functions (sin)' => sub {
    diag 'Comparing Math Function Overhead (sin)...';
    my %marks = (
        'Affix: affix'       => sub { benchmark_sin(0.5) },
        'Affix: wrap'        => sub { $wrap_sin->(0.5) },
        'Affix: direct_wrap' => sub { $direct_sin->(0.5) },
        'Pure Perl'          => sub { sin(0.5) },
    );
    if ($has_platypus) {
        $marks{'Platypus: attach'}   = sub { platypus_sin(0.5) };
        $marks{'Platypus: function'} = sub { $platypus_sin_func->(0.5) };
    }
    if ($has_inline_c) {
        $marks{'Inline::C'} = sub { inline_sin(0.5) };
    }
    cmpthese( $bench_count, \%marks );
    pass 'finished';
};
subtest 'Pointer Marshalling' => sub {
    diag 'Comparing Pointer Marshalling (Return pointer, Pass pointer)...';
    my %marks = ( 'Affix: return' => sub { get_ptr() }, 'Affix: pass' => sub { take_ptr($ptr) }, );
    if ($has_platypus) {
        $marks{'Platypus: return'} = sub { platypus_get_ptr() };
        $marks{'Platypus: pass'}   = sub { platypus_take_ptr($ptr) };
    }
    cmpthese( $bench_count, \%marks );
    pass 'finished';
};
subtest 'Aggregate Marshalling (Copying)' => sub {
    diag 'Comparing Struct Marshalling (Returned by value, deep-copied to Hash/Record)...';
    my %marks = ( 'Affix: struct_copy' => sub { get_struct() }, );
    if ($has_platypus) {
        $marks{'Platypus: struct_copy'} = sub { platypus_get_struct() };
    }
    cmpthese( $bench_count, \%marks );
    pass 'finished';
};
subtest 'Enum Marshalling' => sub {
    diag 'Comparing Enum Marshalling (Dualvars vs Integers)...';
    my %marks
        = ( 'Affix: pass_int' => sub { take_enum(2) }, 'Affix: pass_str' => sub { take_enum('GREEN') }, 'Affix: return' => sub { return_enum(3) }, );
    if ($has_platypus) {
        $marks{'Platypus: pass_int'} = sub { platypus_take_enum(2) };
        $marks{'Platypus: return'}   = sub { platypus_return_enum(3) };
    }
    cmpthese( $bench_count, \%marks );
    pass 'finished';
};
subtest 'Affix-Specific: "Live" vs "Copy" Aggregates' => sub {
    my $has_live = eval { LiveStruct( [ a => Int ] ); 1 };
    if ($has_live) {
        diag 'Benchmarking Affix-only Features: "Live" vs "Copy" Aggregates...';
        my $Inner = Struct [ a => Int, b => Int, c => Int, d => Int ];
        diag 'LiveStruct signature: ' . LiveStruct($Inner);
        eval 'affix $lib, [ get_struct => "get_struct_live" ], [] => LiveStruct $Inner';
        eval 'affix $lib, [ get_outer  => "get_outer_live"  ], [] => LiveStruct [ nested => $Inner, id => Int ]';
        my $live_struct = get_struct_live();
        my $copy_struct = get_struct();
        cmpthese(
            $bench_count,
            {   'struct_copy_pull'   => sub { get_struct() },
                'struct_live_pull'   => sub { get_struct_live() },
                'member_access_copy' => sub { my $v             = $copy_struct->{a} },
                'member_access_live' => sub { my $v             = $live_struct->{a} },
                'member_write_live'  => sub { $live_struct->{a} = 100 },
            }
        );
        diag 'Benchmarking Affix-only Features: Recursive Liveness...';
        my $outer_live = get_outer_live();
        cmpthese(
            $bench_count,
            {   'nested_member_access' => sub { my $v = $outer_live->{nested}{a} }
            }
        );
        pass 'finished';
    }
    else {
        pass 'LiveStruct not supported in this build';
    }
};
subtest 'Affix-Specific: Pointer and Array Indexing' => sub {
    diag 'Benchmarking Affix-only Features: Pointer and Array Indexing...';
    my $live_ptr   = get_ptr();
    my $live_array = calloc( 10, Int );
    cmpthese(
        $bench_count,
        {   'ptr_deref_magic' => sub { my $v = $$live_ptr },
            'ptr_indexing_r'  => sub {
                eval { my $v = $live_ptr->[0] }
            },
            'ptr_indexing_w' => sub {
                eval { $live_ptr->[0] = 42 }
            },
            'array_indexing_r' => sub {
                eval { my $v = $live_array->[5] }
            },
        }
    );
    pass 'finished';
};
subtest 'Affix-Specific: 128-bit Integers' => sub {
    my $has_128 = 0;
    if ( eval { Int128(); 1 } ) {
        eval {
            affix $lib, 'add128', [ Int128, Int128 ] => Int128;
            $has_128 = 1;
        };
    }
    if ($has_128) {
        diag 'Benchmarking Affix-only Features: 128-bit Integers (passed as strings)...';
        my $a = '170141183460469231731687303715884105727';    # max int128
        my $b = '1';
        cmpthese(
            $bench_count / 5,
            {   'add128' => sub { add128( $a, $b ) }
            }
        );
        pass 'finished';
    }
    else {
        pass '128-bit integers not supported in this build';
    }
};
done_testing;
