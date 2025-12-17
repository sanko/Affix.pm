use v5.40;
use lib '../lib', 'lib';
use blib;
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];
use Config;
#
$|++;
#
# This C code will be compiled into a temporary library for many of the tests.
my $C_CODE = <<'END_C';
#include "std.h"
//ext: .c

DLLEXPORT const char* get_hello_string() { return "Hello from C"; }
DLLEXPORT bool set_hello_string(const char * hi) { return strcmp(hi, "Hello from Perl")==0; }

DLLEXPORT int read_int_from_void_ptr(void* p) {
    if (!p) return -999;
    return *(int*)p;
}

DLLEXPORT int sum_int_array(int* arr, int count) {
    int total = 0;
    for (int i = 0; i < count; i++)
        total += arr[i];
    return total;
}

DLLEXPORT bool check_is_null(void* p) {
    return (p == NULL);
}

// Dereferences a pointer and returns its value + 10.
DLLEXPORT int deref_and_add(int* p) {
    if (!p) return -1;
    return *p + 10;
}

DLLEXPORT void modify_int_ptr(int* p, int new_val) {
    if (p) *p = new_val + 1;
}

DLLEXPORT int check_string_ptr_ptr(char** s) {
    if (s && *s && strcmp(*s, "perl") == 0) {
        // Modify the inner pointer to prove we can
        *s = "C changed me";
        return 1; // success
    }
    return 0; // failure
}

typedef struct {
    int32_t id;
    double value;
    const char* label;
} MyStruct;

MyStruct g_struct = { 99, -1.0, "Global" };

DLLEXPORT void init_struct(MyStruct* s, int32_t id, double value, const char* label) {
    if (s) {
        s->id = id;
        s->value = value;
        s->label = label;
    }
}
DLLEXPORT MyStruct* get_static_struct_ptr() {
    return &g_struct;
}

DLLEXPORT int32_t get_struct_id(MyStruct* s) {
    return s ? s->id : -1;
}

DLLEXPORT int call_int_cb(int (*cb)(int), int val) {
    return cb(val);
}
END_C

# Compile the library once for all subtests that need it.
my $lib_path = compile_ok($C_CODE);
ok( $lib_path && -e $lib_path, 'Compiled a test shared library successfully' );
#
affix $lib_path, 'read_int_from_void_ptr', [ Pointer [Void] ], Int;
my $mem = malloc(8);

# Cast returns a new pin. We must assign it or use the returned object.
# Also, we keep $mem alive to ensure the memory isn't freed if $int_ptr assumes
# $mem owns it (though cast usually creates unmanaged aliases, so we need $mem to stay alive).
my $int_ptr = Affix::cast( $mem, Pointer [Int] );

# Test magical 'set' via dereferencing
# $$int_ptr is a scalar magic that writes to the address
$$int_ptr = 42;

# Use the original $mem pointer for reading (verifying they point to the same place)
is( read_int_from_void_ptr($mem), 42, 'Magical set via deref wrote to C memory' );

# Test cast again
my $long_ptr = Affix::cast( $mem, Pointer [LongLong] );
$$long_ptr = 1234567890123;
is $$long_ptr, 1234567890123, 'Magical get after casting to a new type works';

# Test realloc
my $r_ptr = calloc( 2, Int );

# realloc updates the pointer inside $r_ptr in-place.
Affix::realloc( $r_ptr, 32 );    # Reallocate to hold 8 ints

# But $r_ptr still thinks it's [2:int]. We must cast to update the type view.
my $arr_ptr = Affix::cast( $r_ptr, Array [ Int, 8 ] );

# Read the entire array from C into a Perl variable
my $array_values = $$arr_ptr;

# Modify perl's copy
$array_values->[0] = 10;
$array_values->[7] = 80;

# Write the entire modified array ref back to the C pointer
$$arr_ptr = $array_values;

# Visual evidence that the memory has actually been updated
#~ Affix::dump( $arr_ptr, 32 );
# sum_int_array takes *int, so passing [8:int] (array ref) works as pointer
ok affix( $lib_path, 'sum_int_array', [ Pointer [Int], Int ], Int ), 'affix ... "sum_int_array", ...';
is sum_int_array( $arr_ptr, 8 ), 90, 'realloc successfully resized memory';
#
isa_ok my $check_is_null = wrap( $lib_path, 'check_is_null', '(*void)->bool' ), ['Affix'];
ok $check_is_null->(undef), 'Passing undef to a *void argument is received as NULL';
subtest 'char*' => sub {
    isa_ok my $get_string = wrap( $lib_path, 'get_hello_string', '()->*char' ), ['Affix'];
    is $get_string->(), 'Hello from C', 'Correctly returned a C string';
    isa_ok my $set_string = wrap( $lib_path, 'set_hello_string', '(*char)->bool' ), ['Affix'];
    ok $set_string->('Hello from Perl'), 'Correctly passed a string to C';
};
subtest 'int32*' => sub {
    isa_ok my $deref  = wrap( $lib_path, 'deref_and_add',  '(*int32)->int32' ),       ['Affix'];
    isa_ok my $modify = wrap( $lib_path, 'modify_int_ptr', '(*int32, int32)->void' ), ['Affix'];
    my $int_var = 50;
    is $deref->( \$int_var ), 60, 'Passing a scalar ref as an "in" pointer works';
    $modify->( \$int_var, 999 );
    is $int_var, 1000, 'C function correctly modified the value in our scalar ref ("out" param)';
};
subtest 'void*' => sub {
    isa_ok my $read_void = wrap( $lib_path, 'read_int_from_void_ptr', '(*void)->int32' ), ['Affix'];
    my $int_val = 12345;
    is $read_void->( \$int_val ), 12345, 'Correctly passed a scalar ref as a void* and read its value';
};
subtest 'char**' => sub {
    isa_ok my $check_ptr_ptr = wrap( $lib_path, 'check_string_ptr_ptr', '(**char)->int32' ), ['Affix'];
    my $string = 'perl';
    ok $check_ptr_ptr->( \$string ), 'Correctly passed a reference to a string as char**';
    is $string, 'C changed me', 'C function was able to modify the inner pointer';
};
subtest 'Struct Pointers (*@My::Struct)' => sub {
    ok typedef( 'My::Struct' => Struct [ id => SInt32, value => Float64, label => Pointer [Char] ] ), q[typedef('My::Struct' = ...)];
    isa_ok my $init_struct = wrap( $lib_path, 'init_struct', '(*@My::Struct, int32, float64, *char)->void' ), ['Affix'];
    my %struct_hash;
    $init_struct->( \%struct_hash, 101, 9.9, "Initialized" );
    is \%struct_hash, { id => 101, value => float(9.9), label => "Initialized" }, 'Correctly initialized a Perl hash via a struct pointer';
    isa_ok my $get_ptr = wrap( $lib_path, 'get_static_struct_ptr', '()->*@My::Struct' ), ['Affix'];
    my $struct_ptr = $get_ptr->();

    # Struct pointer now returns a Pin (Scalar Ref). Dereference it to check contents.
    is $$struct_ptr, { id => 99, value => float(-1.0), label => 'Global' }, 'Dereferencing a returned struct pointer works';
};
subtest 'Function Pointers (*(int->int))' => sub {
    isa_ok my $harness = wrap( $lib_path, 'call_int_cb', '(*((int32)->int32), int32)->int32' ), ['Affix'];
    my $result = $harness->( sub { $_[0] * 10 }, 7 );
    is $result, 70, 'Correctly passed a simple coderef as a function pointer';
    ok $check_is_null->(undef), 'Passing undef as a function pointer is received as NULL';
};
#
done_testing;
__END__


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



// Takes a pointer to a pointer and verifies the string.


/* Structs and Arrays */


// Sums an array of 64-bit integers.
DLLEXPORT int64_t sum_s64_array(int64_t* arr, int len) {
    int64_t total = 0;
    for (int i = 0; i < len; i++)
        total += arr[i];
    return total;
}

// Returns a pointer to a static internal struct


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


DLLEXPORT double call_math_cb(double (*cb)(double, int), double d, int i) {
    return cb(d, i);
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
