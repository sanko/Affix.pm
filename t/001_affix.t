use v5.40;
use lib '../lib', 'lib';
use blib;
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];
use Config;
#
#~ Affix::test_internal_lifecycle();
#~ Affix::test_callback_lifecycle();
$|++;
#
subtest import => sub {
    imported_ok qw[affix wrap pin unpin];
    imported_ok qw[libm libc];
    imported_ok qw[sizeof alignof offsetof];
};
subtest types => sub {
    imported_ok qw[
        Void Bool Char UChar Short UShort Int UInt Long ULong LongLong ULongLong Float Double LongDouble
        Size_t
        String WString
        Pointer
        SV
        SChar WChar
        SInt8
    ];
    subtest abstract => sub {
        is Void,       'void',       'Void';
        is Bool,       'bool',       'Bool';
        is Char,       'char',       'Char';
        is UChar,      'uchar',      'UChar';
        is Short,      'short',      'Short';
        is UShort,     'ushort',     'UShort';
        is Int,        'int',        'Int';
        is UInt,       'uint',       'UInt';
        is Long,       'long',       'Long';
        is ULong,      'ulong',      'ULong';
        is LongLong,   'longlong',   'LongLong';
        is ULongLong,  'ulonglong',  'ULongLong';
        is Float,      'float',      'Float';
        is Double,     'double',     'Double';
        is LongDouble, 'longdouble', 'LongDouble';
        is SChar,      'char',       'SChar';
    };
    subtest explicit => sub {
        is SInt8,   'sint8',   'SInt8';
        is SInt16,  'sint16',  'SInt16';
        is SInt32,  'sint32',  'SInt32';
        is SInt64,  'sint64',  'SInt64';
        is SInt128, 'sint128', 'SInt128';
    };
    subtest SIMD => sub {
        skip_all 'TODO';
    };
    subtest composite => sub {
        is Pointer [Void],             '*void',  'Pointer[Void]';
        is Pointer [Char],             '*char',  'Pointer[Char]';
        is Pointer [ Pointer [Void] ], '**void', 'Pointer[Pointer[Void]]';
        #
        is Struct [ name => Pointer [Char] ], '{name:*char}', 'Struct[ name => ... ]';
        is Struct [ name => Pointer [Char], dob => Struct [ y => Int, m => Int, d => Int ] ], '{name:*char,dob:{y:int,m:int,d:int}}',
            'Struct[ name => ..., dob => ...]';
        #
        is Union [ i => Int, f => Float ], '<i:int,f:float>', 'Union[...]';
        #
    };

    #~ use Data::Dump;
    #~ ddx [ Int, Void, Pointer [Int], Int ];
};

# This C code will be compiled into a temporary library for many of the tests.
my $C_CODE = <<'END_C';
#include "std.h"
//ext: .c

#include <stdint.h>
#include <stdbool.h>
#include <string.h> // For strcmp
#include <stdlib.h> // For malloc

/* Basic Primitives */
DLLEXPORT int add(int a, int b) { return a + b; }
DLLEXPORT unsigned int u_add(unsigned int a, unsigned int b) { return a + b; }

// Functions to test every supported primitive type
DLLEXPORT int8_t   echo_int8   (int8_t   v) { return v; }
DLLEXPORT uint8_t  echo_uint8  (uint8_t  v) { return v; }
DLLEXPORT int16_t  echo_int16  (int16_t  v) { return v; }
DLLEXPORT uint16_t echo_uint16 (uint16_t v) { return v; }
DLLEXPORT int32_t  echo_int32  (int32_t  v) { return v; }
DLLEXPORT uint32_t echo_uint32 (uint32_t v) { return v; }
DLLEXPORT int64_t  echo_int64  (int64_t  v) { return v; }
DLLEXPORT uint64_t echo_uint64 (uint64_t v) { return v; }
DLLEXPORT float    echo_float  (float    v) { return v; }
DLLEXPORT double   echo_double (double   v) { return v; }
DLLEXPORT bool     echo_bool   (bool     v) { return v; }

/* Pointers and References */
DLLEXPORT const char* get_hello_string() { return "Hello from C"; }
DLLEXPORT bool set_hello_string(const char * hi) { return strcmp(hi, "Hello from Perl")==0; }

// Dereferences a pointer and returns its value + 10.
DLLEXPORT int deref_and_add(int* p) {
    if (!p) return -1;
    return *p + 10;
}

// Modifies the integer pointed to by the argument.
DLLEXPORT void modify_int_ptr(int* p, int new_val) {
    if (p) *p = new_val + 1;
}

// Takes a pointer to a pointer and verifies the string.
DLLEXPORT int check_string_ptr_ptr(char** s) {
    if (s && *s && strcmp(*s, "perl") == 0) {
        // Modify the inner pointer to prove we can
        *s = "C changed me";
        return 1; // success
    }
    return 0; // failure
}

/* Structs and Arrays */
typedef struct {
    int32_t id;
    double value;
    const char* label;
} MyStruct;

// "Constructor" for the struct.
DLLEXPORT void init_struct(MyStruct* s, int32_t id, double value, const char* label) {
    if (s) {
        s->id = id;
        s->value = value;
        s->label = label;
    }
}

// "Getter" for a struct member.
DLLEXPORT int32_t get_struct_id(MyStruct* s) {
    return s ? s->id : -1;
}

// Sums an array of 64-bit integers.
DLLEXPORT int64_t sum_s64_array(int64_t* arr, int len) {
    int64_t total = 0;
    for (int i = 0; i < len; i++)
        total += arr[i];
    return total;
}

// Returns a pointer to a static internal struct
MyStruct g_struct = { 99, -1.0, "Global" };
DLLEXPORT MyStruct* get_static_struct_ptr() {
    return &g_struct;
}

/* Nested Structs */
typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point top_left;
    Point bottom_right;
    const char* name;
} Rectangle;

DLLEXPORT int get_rect_width(Rectangle* r) {
    if (!r) return -1;
    return r->bottom_right.x - r->top_left.x;
}

// Return a struct by value
DLLEXPORT Point create_point(int x, int y) {
    Point p = {x, y};
    return p;
}

/* Advanced Pointers */
DLLEXPORT bool check_is_null(void* p) {
    return (p == NULL);
}
// Takes a void* and casts it to an int*
DLLEXPORT int read_int_from_void_ptr(void* p) {
    if (!p) return -999;
    return *(int*)p;
}

/* Enums and Unions */
typedef enum { RED, GREEN, BLUE } Color;

DLLEXPORT int check_color(Color c) {
    if (c == GREEN) return 1;
    return 0;
}

typedef union {
    int i;
    float f;
    char c[8];
} MyUnion;

DLLEXPORT float process_union_float(MyUnion u) {
    return u.f * 10.0;
}

/* Advanced Callbacks */
// Takes a callback that processes a struct
DLLEXPORT double process_struct_with_cb(MyStruct* s, double (*cb)(MyStruct*)) {
    return cb(s);
}

// Takes a callback that returns a struct
DLLEXPORT int check_returned_struct_from_cb(Point (*cb)(void)) {
    Point p = cb();
    return p.x + p.y;
}

// A callback with many arguments to test register/stack passing
typedef void (*kitchen_sink_cb)(
    int a, double b, int c, double d, int e, double f, int g, double h,
    const char* i, int* j
);
DLLEXPORT int call_kitchen_sink(kitchen_sink_cb cb) {
    int j_val = 100;
    cb(1, 2.0, 3, 4.0, 5, 6.0, 7, 8.0, "kitchen sink", &j_val);
    return j_val + 1;
}

/* Functions with many arguments */
DLLEXPORT long long multi_arg_sum(
    long long a, long long b, long long c, long long d,
    long long e, long long f, long long g, long long h, long long i
) {
    return a + b + c + d + e + f + g + h + i;
}

/* Simple Callback Harness */
DLLEXPORT int call_int_cb(int (*cb)(int), int val) {
    return cb(val);
}

DLLEXPORT double call_math_cb(double (*cb)(double, int), double d, int i) {
    return cb(d, i);
}

DLLEXPORT int sum_int_array(int* arr, int count) {
    int total = 0;
    for (int i = 0; i < count; i++)
        total += arr[i];
    return total;
}

DLLEXPORT int sum_point_by_val(Point p) {
    return p.x + p.y;
}

DLLEXPORT char get_char_at(char s[20], int index) {
    warn("# get_char_at('%s', %d);", s, index);
    if (index >= 20 || index < 0) return '!';
    return s[index];
}

DLLEXPORT int read_union_int(MyUnion u) {
    return u.i;
}

DLLEXPORT float sum_float_array(float* arr, int len) {
    float total = 0.0f;
    for (int i = 0; i < len; i++)
        total += arr[i];
    return total;
}

#if !(defined(__FreeBSD__) && defined(__aarch64__))
/* Long Double */
DLLEXPORT long double add_ld(long double a, long double b) {
    return a + b;
}

DLLEXPORT double ld_to_d(long double a) {
    return (double)a;
}
#endif

END_C

# Compile the library once for all subtests that need it.
my $lib_path = compile_ok($C_CODE);
ok( $lib_path && -e $lib_path, 'Compiled a test shared library successfully' );
subtest 'Library Loading and Lifecycle' => sub {
    note 'Testing load_library(), Affix::Lib objects, and reference counting.';
    my $lib1 = load_library($lib_path);
    isa_ok $lib1, ['Affix::Lib'], 'load_library returns an Affix::Lib object';
    my $lib2 = load_library($lib_path);
    is int $lib1, int $lib2, 'Loading the same library returns a handle to the same underlying object (singleton behavior)';
    my $bad_lib = load_library('non_existent_library_12345.so');
    is $bad_lib,                 undef, 'load_library returns undef for a non-existent library';
    is get_last_error_message(), D(),   'get_last_error_message provides a useful error on failed load';
};
subtest 'Symbol Finding' => sub {
    ok my $lib    = load_library($lib_path),    'load_library returns a pointer';
    ok my $symbol = find_symbol( $lib, 'add' ), 'find_symbol returns a pointer';
    is find_symbol( $lib, 'non_existent_symbol_12345' ), U(), 'find_symbol returns undef for a non-existent symbol';
};
subtest 'Forward Calls: Comprehensive Primitives' => sub {
    for my ( $type, $value )(
        bool  => false,                                       #
        int8  => -100,           uint8  => 100,               #
        int16 => -30000,         uint16 => 60000,             #
        int32 => -2_000_000_000, uint32 => 4_000_000_000,     #
        int64 => -5_000_000_000, uint64 => 10_000_000_000,    #
        float =>  1.23,          double => -4.56              #
    ) {
        my $name = "echo_$type";
        my $sig  = "($type)->$type";
        isa_ok my $fn = wrap( $lib_path, $name, $sig ), ['Affix'], $sig;
        is $fn->($value), $value == int $value ? $value : float( $value, tolerance => 0.01 ), "Correctly passed and returned type '$type'";
    }
};
subtest 'Forward Call with Many Arguments' => sub {
    note 'Testing a C function with more arguments than available registers.';
    my $sig = '(int64, int64, int64, int64, int64, int64, int64, int64, int64)->int64';
    isa_ok my $summer = wrap( $lib_path, 'multi_arg_sum', $sig ), ['Affix'];
    my $result = $summer->( 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 );
    is $result, 111111111, 'Correctly passed 9 arguments to a C function';
};
subtest 'Parser Error Reporting' => sub {
    note 'Testing that malformed signatures produce helpful error messages.';
    like warning { Affix::wrap( $lib_path, 'add', '(int, ^, int)->int' ) }, qr[parse signature], 'wrap() warning on invalid signature';
    like warning { Affix::sizeof('{int, double') },                         qr[parse signature], 'sizeof() warning on unterminated aggregate';
};
subtest '"Kitchen Sink" Callback' => sub {
    note 'Testing a callback with 10 mixed arguments passed as a direct coderef.';
    isa_ok my $harness
        = wrap( $lib_path, 'call_kitchen_sink',
        [ Callback [ [ SInt32, Float64, SInt32, Float64, SInt32, Float64, SInt32, Float64, Pointer [Char], Pointer [SInt32] ] => Void ] ] => SInt32 ),
        ['Affix'];
    my $callback_sub = sub( $a, $b, $c, $d, $e, $f, $g, $h, $i, $j_ref ) {
        is $a,      1,              'Callback arg 1 (int)';
        is $b,      2.0,            'Callback arg 2 (double)';
        is $c,      3,              'Callback arg 3 (int)';
        is $d,      4.0,            'Callback arg 4 (double)';
        is $e,      5,              'Callback arg 5 (int)';
        is $f,      6.0,            'Callback arg 6 (double)';
        is $g,      7,              'Callback arg 7 (int)';
        is $h,      8.0,            'Callback arg 8 (double)';
        is $i,      'kitchen sink', 'Callback arg 9 (string)';
        is $$j_ref, 100,            'Callback arg 10 (int*)';
        $$j_ref = 200;
    };
    is $harness->($callback_sub), 201, 'return value';
};
subtest 'Memory Management (malloc, calloc, free)' => sub {
    my $ptr = malloc(32);
    ok $ptr, 'malloc returns a pinned SV*';

    #~ use Data::Printer;
    #~ p $ptr;
    #~ diag length $ptr;
    #~ diag Affix::dump( $ptr, 32 );
    ok my $array_ptr = calloc( 4, Int ), 'calloc returns an array';

    #~ diag Affix::dump( $array_ptr, 32 );
    ok $array_ptr, 'calloc returns an Affix::Pointer object';
    ok affix $lib_path, 'sum_int_array', '(*int, int)->int';
    is sum_int_array( $array_ptr, 4 ), 0, 'Memory from calloc is zero-initialized';
    ok free($array_ptr), 'Explicitly calling free() returns true';

    # Note: Double-free would crash, so we assume it worked.
    like( warning { free( find_symbol( load_library($lib_path), 'sum_int_array' ) ) },
        qr/unmanaged/, 'free() croaks when called on an unmanaged pointer' );

    # Test that auto-freeing via garbage collection doesn't crash
    subtest 'GC of managed pointers' => sub {
        ok my $scoped_ptr = malloc(16), 'malloc(16)';

        #~ ok cast( $scoped_ptr, '*int'), 'cast void pointer to int pointer';
        #substr $$scoped_ptr, 0, 1, 'a';
        #~ diag '[' . ($$scoped_ptr) . ']';
        my $values = $$scoped_ptr;
        substr( $values, 4 ) = 'hi';
        $$scoped_ptr = $values;

        #~ Affix::dump( $scoped_ptr, 32 );
        #~ diag '[' . ($$scoped_ptr) . ']';
        # When $scoped_ptr goes out of scope here, its DESTROY method is called.
    };
    pass('Managed pointer went out of scope without crashing');
};
subtest 'Advanced Arrays' => sub {
    affix $lib_path, 'get_char_at', '([20:char], int)->char';
    my $str = "Perl";
    is( chr( get_char_at( $str, 0 ) ), 'P', 'Passing string to char[N] works (char 0)' );
    is( get_char_at( $str, 4 ),        0,   'Passing string to char[N] is null-terminated' );
    my $long_str = "This is a very long string that will be truncated";
    is( chr( get_char_at( $long_str, 18 ) ), 'g', 'Truncated string char 18 is correct' );
    is( get_char_at( $long_str, 19 ),        0,   'Truncated string is null-terminated at the boundary' );
    ok affix( $lib_path, 'sum_float_array', '(*float, int)->float' ), 'affix sum_float_array';
    my $floats = [ 1.1, 2.2, 3.3 ];
    is( sum_float_array( $floats, 3 ), float( 6.6, tolerance => 0.01 ), 'Correctly summed an array of floats' );
};
subtest 'These are called under valgrind in 900_leak' => sub {
    subtest 'use Affix' => sub {
        use Affix qw[];
        pass 'loaded';
    };
    subtest 'affix($$$$)' => sub {
        no warnings 'redefine';
        ok affix( libm, 'pow', [ Double, Double ], Double ), 'affix pow( Double, Double )';
        is pow( 5, 2 ), 25, 'pow(5, 2)';
    };
    subtest 'wrap($$$$)' => sub {
        isa_ok my $pow = wrap( libm, 'pow', [ Double, Double ], Double ), ['Affix'], 'double pow(double, double)';
        is $pow->( 5, 2 ), 25, '$pow->(5, 2)';
    };
    subtest 'return pointer' => sub {
        my $lib = compile_ok(<<'');
#include "std.h"
// ext: .c
void * test( ) { void * ret = "Testing"; return ret; }

        ok my $fn         = wrap( $lib, 'test', [] => Pointer [Void] ), 'affix';
        ok my $string_ptr = $fn->(),                                    'call';

        # Casting a pointer to String should return the Value "Testing"
        is Affix::cast( $string_ptr, String ), 'Testing', 'cast($ptr, String) returns value';
    };
    subtest 'return malloc\'d pointer' => sub {
        ok my $lib = compile_ok(<<'');
#include "std.h"
// ext: .c
#include <stdlib.h>
#include <string.h>
void * test() {
  void * ret = malloc(8);
  if ( ret == NULL ) { }
  else { strcpy(ret, "Testing"); }
  return ret;
}
void c_free(void* p) { free(p); }

        ok affix( $lib, 'test', [] => Pointer [Void] ), 'affix test()';

        # We MUST bind C's free, because Affix::free uses Perl's allocator.
        # Mixing them causes crashes on Windows.
        ok affix( $lib, 'c_free', [ Pointer [Void] ] => Void ), 'affix c_free()';
        ok my $string = test(),                                 'test()';
        is Affix::cast( $string, String ), 'Testing', 'read C string';

        # Correct cleanup: Use the allocator that created it.
        c_free($string);
        pass('freed via c_free');
    };
};
subtest enum => sub {
    subtest raw => sub {
        my $enum = Affix::Type::Enum->new(
            elements => [
                'SDL_FLIP_NONE',                                                   # = 0
                'SDL_FLIP_HORIZONTAL',                                             # = 1
                'SDL_FLIP_VERTICAL',                                               # = 2
                [ SDL_FLIP_BOTH => 'SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL' ],    # = 3
                [ SDL_MATH_TEST => 'SDL_FLIP_VERTICAL + 10' ]                      # = 12
            ]
        );
        my ( $consts, $vals ) = $enum->resolve();
        is $consts->{SDL_FLIP_BOTH}, 3,  'SDL_FLIP_BOTH';
        is $consts->{SDL_MATH_TEST}, 12, 'SDL_MATH_TEST';
    };
    my $c_source = <<'END_C';
#include "std.h"
//ext: .c

typedef enum {
    STATE_START     = 0,
    STATE_RUNNING   = 10,
    STATE_PAUSED    = 11,
    STATE_STOPPED   = 20,
    STATE_ERROR     = 99
} MachineState;

DLLEXPORT int check_state(MachineState s) {
    if (s == STATE_RUNNING) return 1;
    if (s == STATE_PAUSED)  return 2;
    return 0;
}

DLLEXPORT MachineState get_next_state(MachineState s) {
    if (s == STATE_START)   return STATE_RUNNING;
    if (s == STATE_RUNNING) return STATE_PAUSED;
    if (s == STATE_PAUSED)  return STATE_STOPPED;
    return STATE_ERROR;
}
END_C
    my $lib = compile_ok( $c_source, 'Compiled Enum test library' );

    # ============================================================================
    # 2. Pure Perl Logic Tests (Constants & Generation)
    # ============================================================================
    subtest 'Enum Definition & Constants' => sub {

        # Define an Enum in the current package
        # This tests:
        # 1. Explicit values ([KEY => VAL])
        # 2. Auto-increment ('KEY')
        # 3. Back-references ([KEY => 'PREV_KEY'])
        ok typedef(
            TestState => Enum [
                [ TEST_A => 0 ], [ TEST_B => 10 ], 'TEST_C',    # Should be 11
                [ TEST_D => 0x20 ],                             # Should be 32
                [ TEST_E => 'TEST_B' ]                          # Should be 10
            ]
            ),
            'typedef Enum executed';

        # Verify constants were exported to this namespace
        ok defined &TEST_A, 'Constant TEST_A exported';
        ok defined &TEST_B, 'Constant TEST_B exported';
        ok defined &TEST_C, 'Constant TEST_C exported';

        # Verify values
        is TEST_A(), 0,  'Explicit value (0)';
        is TEST_B(), 10, 'Explicit value (10)';
        is TEST_C(), 11, 'Auto-increment value (10 + 1 = 11)';
        is TEST_D(), 32, 'Hex value (0x20)';
        is TEST_E(), 10, 'Back-reference value (== TEST_B)';
    };
    subtest 'C Integration & Dualvars' => sub {

        # Define the Enum matching the C library
        typedef MachineState => Enum [
            [ STATE_START   => 0 ],  [ STATE_RUNNING => 10 ], 'STATE_PAUSED',    # 11
            [ STATE_STOPPED => 20 ], [ STATE_ERROR   => 99 ]
        ];

        # Bind functions
        isa_ok my $check = wrap( $lib, 'check_state',    ['@MachineState'] => Int ),             ['Affix'];
        isa_ok my $next  = wrap( $lib, 'get_next_state', ['@MachineState'] => '@MachineState' ), ['Affix'];
        subtest 'Passing Constants to C' => sub {

            # Pass the bareword constant
            is $check->( STATE_RUNNING() ), 1, 'Passed constant STATE_RUNNING (10) ok';
            is $check->( STATE_PAUSED() ),  2, 'Passed constant STATE_PAUSED (11) ok';

            # Pass raw integer
            is $check->(10), 1, 'Passed raw integer 10 ok';
        };
        subtest 'Returning Dualvars from C' => sub {

            # Case 1: START -> RUNNING
            my $val = $next->( STATE_START() );

            # Check Numeric Value
            is 0 + $val, 10, 'Numeric value is 10';
            ok $val == STATE_RUNNING(), 'Numeric equality with constant';

            # Check String Value (Dualvar)
            is "$val", 'STATE_RUNNING', 'String value is "STATE_RUNNING"';
            ok $val eq 'STATE_RUNNING', 'String equality';

            # Case 2: RUNNING -> PAUSED
            my $val2 = $next->( STATE_RUNNING() );
            is 0 + $val2, 11,             'Numeric value is 11';
            is "$val2",   'STATE_PAUSED', 'String value is "STATE_PAUSED"';
        };
        subtest 'Unknown Values' => sub {

            # Create a function that returns a value NOT in our enum definition
            # (Simulating C library adding a new flag we don't know about yet)
            my $raw_lib = compile_ok(<<'END_RAW');
#include "std.h"
//ext: .c
DLLEXPORT int get_unknown() { return 555; }
END_RAW
            my $get_unknown = wrap( $raw_lib, 'get_unknown', [] => MachineState() );
            my $val         = $get_unknown->();
            is 0 + $val, 555, 'Unknown integer value preserved';

            # Behavior for unknown strings depends on impl, usually just the number as string
            # or undef string slot. Usually sv_setiv sets the IV, sv_setpv is skipped.
            # So "$val" should be "555".
            is "$val", "555", 'Stringification of unknown enum value falls back to number';
        };
    };
    {

        package Other::Scope;
        use Affix;
        use Test2::Tools::Affix qw[ok is];

        sub run_test {
            Affix::typedef( ScopedEnum => Affix::Enum( [ [ SCOPED_A => 99 ] ] ) );
            ok defined &SCOPED_A, 'Constant exported to Other::Scope';
            is SCOPED_A(), 99, 'Constant value correct in Other::Scope';
        }
    }
    subtest 'Namespace Isolation' => sub {
        Other::Scope::run_test();
        ok !defined &SCOPED_A, 'Constant NOT leaked to main package';
    };
};
subtest 'Pointer Arithmetic and String Utils' => sub {
    imported_ok qw[ptr_add ptr_diff strdup strnlen is_null];
    subtest 'ptr_add and ptr_diff' => sub {
        my $buf = calloc( 10, Int );    # 40 bytes
        ok !is_null($buf), 'buffer is not null';
        my $p2 = ptr_add( $buf, 8 );    # width of 2 ints
        is ptr_diff( $p2,  $buf ),  8, 'ptr_diff calculates 8 bytes';
        is ptr_diff( $buf, $p2 ),  -8, 'ptr_diff calculates -8 bytes';
        $$p2 = 999;                     # Write to offset

        # Verify via original array pointer
        my $arr = cast( $buf, Array [ Int, 10 ] );
        is $$arr->[2], 999, 'ptr_add moved to index 2 correctly';
        free($buf);
    };
    subtest 'strdup and strnlen' => sub {
        my $str = "Hello World";
        my $dup = strdup($str);
        ok !is_null($dup), 'strdup returned non-null';
        is cast( $dup, String ), $str, 'strdup content matches';
        is strnlen( $dup, 5 ),   5,  'strnlen capped at max';
        is strnlen( $dup, 100 ), 11, 'strnlen found true length';

        # Ensure it's managed memory that we can free
        ok free($dup), 'free(dup) worked';
    };
};
subtest 'unions passed to callbacks' => sub {

    # compile a fresh library for this specific test case
    my $lib = compile_ok( <<'END_C', 'union lib' );
#include "std.h"
//ext: .c
typedef union {
    int i;
    float f;
    char c[8];
} MyUnion;

DLLEXPORT int invoke_union_cb(void (*cb)(MyUnion*)) {
    MyUnion u;
    u.i = 42;
    cb(&u);
    return u.i;
}
END_C
    ok typedef( MyUnion => Union [ i => SInt32, f => Float32, c => Array [ Char, 8 ] ] ), 'typedef @MyUnion';
    isa_ok my $invoke = wrap( $lib, 'invoke_union_cb', [ Callback [ [ Pointer [ MyUnion() ] ] => Void ] ] => Int ), ['Affix'];
    my $cb = sub($pin) {

        # Dereference the pin
        my $u = $$pin;
        is $u->{i}, 42, 'Read integer member from union pointer directly';    # magical

        # IEEE 754 2.0f is 0x40000000 (1073741824 decimal)
        $u->{f} = 2.0;
    };
    my $ret = $invoke->($cb);

    # Verify the write inside the callback persisted to the C caller
    is $ret, 1073741824, 'Callback modifications persisted to C (Union write-back)';
};
#
done_testing;
