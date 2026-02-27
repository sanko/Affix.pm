use v5.40;
use Test2::Tools::Affix qw[:all];
use blib;
use Affix qw[:all];
#

subtest 'Recursive Liveness: LiveArray of Structs' => sub {
    my $Point = Struct[ x => Int(), y => Int() ];
    my $ptr = malloc(sizeof($Point) * 2);
    my $live_arr = cast($ptr, Pointer[Array[$Point, 2]]);

    # Set values
    $live_arr->[0]{x} = 10;
    $live_arr->[0]{y} = 20;

    # Accessing $live_arr->[0] returns an Affix::Live blessed hash
    my $p0 = $live_arr->[0];
    isa_ok $p0, ['Affix::Live'], 'Element of live array should be Affix::Live';

    $p0->{x} = 30;
    is $live_arr->[0]{x}, 30, 'Changes to live element reflect in original memory';

    free($ptr);
};

subtest 'Recursive Liveness: LiveStruct with Array' => sub {
    # Using typedef to ensure member names are preserved in the infix registry
    typedef ListStruct => Struct[ items => Array[Int(), 3] ];

    my $ptr = malloc(sizeof(ListStruct()));
    my $live_struct = cast($ptr, LiveStruct(ListStruct()));

    # Accessing $live_struct->{items} returns an Affix::Pointer object (LiveArray)
    my $items = $live_struct->{items};
    isa_ok $items, ['Affix::Pointer'], 'Array field of live struct should be Affix::Pointer';

    $items->[0] = 100;
    is $live_struct->{items}[0], 100, 'Changes to live array field reflect in struct';

    free($ptr);
};

subtest 'Unified Pointer/Struct Access' => sub {
    my $Point = Struct[ x => Int(), y => Int() ];
    my $ptr = malloc(sizeof($Point));
    my $p = cast($ptr, Pointer[$Point]);

    # Direct hash access via %{} overload
    try {
        $p->{x} = 42;
        is $p->{x}, 42, 'Direct field access on Pointer[Struct] works';
    } catch ($e) {
        fail("Direct field access failed: $e");
    }

    free($ptr);
};

subtest 'Custom Destructors' => sub {
    # Get library object for libc
    my $libc = load_library(libc());
    my $malloc_ptr = find_symbol($libc, 'malloc');
    my $free_ptr   = find_symbol($libc, 'free');

    {
        # Allocate memory using libc's malloc, not Affix's managed malloc
        my $malloc = wrap(undef, $malloc_ptr, [Size_t] => Pointer[Void]);
        my $p = $malloc->(16);

        # Attaching free() as a destructor.
        # When $p goes out of scope, Affix will call free($p).
        attach_destructor($p, $free_ptr, $libc);
    }

    pass "Pin with custom destructor went out of scope without crashing";
};

subtest 'ThisCall Sugar' => sub {
    my $cb = ThisCall( Callback( [ [Int()] => Void() ] ) );
    is $cb->signature, '*((*void,int)->void)', 'ThisCall prepends Pointer[Void] (*void)';

    my $str = ThisCall( '*((int)->void)' );
    is $str, '*((*void,int)->void)', 'ThisCall also works on signature strings';
};

subtest 'Enum String Marshalling' => sub {
    typedef Color => Enum[ [RED => 1], [GREEN => 2], [BLUE => 3] ];

    # Use 'abs' from libc as an identity function for positive integers
    *id = wrap(libc(), 'abs', [Color()] => Color());

    ok id("GREEN") == 2, 'Passing string "GREEN" to Enum works';
    ok id("BLUE")  == 3, 'Passing string "BLUE" to Enum works';
    ok id(1)       == 1, 'Passing integer 1 still works';
};

done_testing;
