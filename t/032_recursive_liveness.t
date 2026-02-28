use v5.40;
use lib 'lib', 'blib/arch', 'blib/lib';
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];

# Prepare C library
my $C_CODE = <<'END_C';
#include "std.h"
//ext: .c
typedef struct {
    int x, y;
} Point;

typedef struct {
    Point top_left;
    Point bottom_right;
} Rect;

static Rect g_rect = { {10, 10}, {50, 50} };

DLLEXPORT void* get_rect_ptr() { return &g_rect; }
DLLEXPORT int get_tl_x() { return g_rect.top_left.x; }
END_C
my $lib_path = compile_ok($C_CODE);
typedef Point => Struct [ x => Int, y => Int ];
typedef Rect => Struct [ top_left => Point(), bottom_right => Point() ];
subtest 'Recursive Liveness (Struct in Struct)' => sub {

    # 1. Cast to LiveStruct
    affix $lib_path, 'get_rect_ptr', [] => Pointer [ Rect() ];
    my $ptr  = get_rect_ptr();
    my $live = cast( $ptr, LiveStruct( Rect() ) );

    # top_left should be a Live view (Hash)
    my $tl = $live->{top_left};
    isa_ok $tl, ['Affix::Live'], 'Nested struct member is a Live view (Hash)';
    is $tl->{x}, 10, 'Initial nested value ok via unified hash access';

    # Write to nested member
    $tl->{x} = 42;
    affix $lib_path, 'get_tl_x', [] => Int;
    is get_tl_x(), 42, 'Modified nested member via live view affected C memory';
};
subtest 'Recursive Liveness (Array in Struct)' => sub {
    typedef NestedArray => Struct [ id => Int, data => Array [ Int, 4 ] ];
    affix $lib_path, [ get_rect_ptr => 'get_arr_ptr' ], [] => Pointer [ NestedArray() ];
    my $ptr  = get_arr_ptr();
    my $live = cast( $ptr, LiveStruct( NestedArray() ) );
    my $arr  = $live->{data};
    isa_ok $arr, ['Affix::Pointer'], 'Nested array member is a Pointer (Live view)';
    $arr->[0] = 999;
    is $arr->[0], 999, 'Modified nested array via live view';
};
done_testing;
