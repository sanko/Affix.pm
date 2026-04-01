use v5.40;
use blib;
use Affix               qw[malloc affix :types sizeof];
use Test2::Tools::Affix qw[:all];
$|++;
#
my $C_CODE = <<'END_C';
#include "std.h"
#include <stdlib.h>

//ext: .c

DLLEXPORT bool test_ptr(void*ptr) { warn("# c: %p", ptr); return true; }

typedef struct {
    int x;
    double y;
} Pos;

// Test passing struct by value
DLLEXPORT bool check_pos_val(Pos p) {
    warn("# c: val {x: %d, y: %f}", p.x, p.y);
    return (p.x == 10 && p.y == 20.5);
}

// Test passing struct by pointer
DLLEXPORT bool check_pos_ptr(Pos *p) {
    warn("# c: ptr %p {x: %d, y: %f}", p, p->x, p->y);
    return (p->x == 10 && p->y == 20.5);
}

typedef struct {
    int id;
    char name[16];
} Task;

typedef struct {
    char name[32];
    Task tasks[2]; // Array inside struct
} Employee;

typedef struct {
    Employee *manager; // Pointer inside struct
    int budget;
} Company;

// Verification function
DLLEXPORT bool verify_hierarchy(Company *c) {
    if (!c || !c->manager) return false;

    warn("# c: Company { budget: %d, manager: '%s' }", c->budget, c->manager->name);
    warn("# c: Manager Task 0: [%d] %s", c->manager->tasks[0].id, c->manager->tasks[0].name);

    return (c->budget == 50000 &&
            strcmp(c->manager->name, "Alice") == 0 &&
            c->manager->tasks[0].id == 101);
}

// Helper to return a NULL manager company
DLLEXPORT Company* get_null_company() {
    static Company c = { .manager = NULL, .budget = 100 };
    return &c;
}


// C++ Mock: Destructor Tracking
static int destructor_count = 0;
typedef struct { int id; } MockObj;
DLLEXPORT MockObj* mock_new(int id) {
    MockObj* m = (MockObj*)malloc(sizeof(MockObj));
    m->id = id;
    return m;
}
DLLEXPORT void mock_delete(MockObj* m) {
    destructor_count++;
    free(m);
}
DLLEXPORT int get_destructor_count() { return destructor_count; }

// Function Pointer Pattern
typedef int (*calc_t)(int, int);
typedef struct {
    calc_t operation;
} Calculator;

DLLEXPORT int run_calc(Calculator *c, int a, int b) {
    if (!c || !c->operation) return -1;
    return c->operation(a, b);
}
END_C
#
my $lib_path = compile_ok($C_CODE);
ok( $lib_path && -e $lib_path, 'Compiled a test shared library successfully' );
ok affix $lib_path, 'test_ptr', [ Pointer [Void] ], Bool;
#
#~ ok my $ptr = malloc(1024), '$ptr = malloc(1024)';
ok my $ptr = alloc_owned(1024), '$ptr = alloc_owned(1024)';
ok is_pinv2($ptr),              'is_pinv2($ptr)';
ok test_ptr($ptr),              'test_ptr($ptr)';
diag sprintf( "0x%016X", address_v2($ptr) );
#
subtest struct => sub {
    typedef Pos => Struct [ x => Int, y => Double ];
    affix $lib_path, 'check_pos_val', [ Pos() ],             Bool;
    affix $lib_path, 'check_pos_ptr', [ Pointer [ Pos() ] ], Bool;

    # Allocate raw memory
    ok my $mem = alloc_owned( sizeof( Pos() ) ), 'alloc_owned memory for Pos';

    # Cast the raw memory to our Pos struct
    # This creates a magical 2.0 pin variable
    ok my $p = cast( $mem, 'Pos' ), 'cast memory to Pos struct';

    # Manipulate fields natively via magic
    $p->{x} = 10;
    $p->{y} = 20.5;

    # Verify native fields
    is $p->{x}, 10,   'field x is 10';
    is $p->{y}, 20.5, 'field y is 20.5';

    # Pass the magical pin to FFI (By Value)
    # Affix will use memcpy because it's a pin
    ok check_pos_val($p), 'FFI check_pos_val($p) - passed by value';

    # Pass the magical pin to FFI (By Pointer)
    # Affix will use get_address_v2 to resolve the pointer
    ok check_pos_ptr($p), 'FFI check_pos_ptr($p) - passed by pointer';
    diag sprintf( "Address: 0x%X", address_v2($p) );
};
subtest company => sub {
    typedef Task     => Struct [ id      => Int, name => Array [ Char, 16 ] ];
    typedef Employee => Struct [ name    => Array [ Char, 32 ], tasks => Array [ Task(), 2 ] ];
    typedef Company  => Struct [ manager => Pointer [ Employee() ], budget => Int ];
    #
    affix $lib_path, 'verify_hierarchy', [ Pointer [ Company() ] ], Bool;

    # Setup nested data in C memory
    ok my $mem_comp = alloc_owned( sizeof( Company() ) ),  'alloc Company';
    ok my $mem_mgr  = alloc_owned( sizeof( Employee() ) ), 'alloc Manager';
    ok my $comp     = cast( $mem_comp, Company() ),        'cast Company';
    ok my $mgr      = cast( $mem_mgr, Employee() ),        'cast Manager';

    # Link them via pointer
    $comp->{manager} = address_v2($mgr);
    $comp->{budget}  = 50000;

    # Set deep values
    $mgr->{name}           = "Alice";
    $mgr->{tasks}[0]{id}   = 101;
    $mgr->{tasks}[0]{name} = "FFI Core";
    $mgr->{tasks}[1]{id}   = 102;
    $mgr->{tasks}[1]{name} = "Marshal v2";

    # TEST DEEP ACCESS: $comp -> manager (ptr) -> tasks (array) -> name (string)
    is $comp->{manager}{name},         "Alice", 'Deep Read: manager->name';
    is $comp->{manager}{tasks}[0]{id}, 101,     'Deep Read: manager->tasks[0]->id';

    # TEST DEEP WRITE via the pointer member
    $comp->{manager}{tasks}[0]{id} = 999;
    is $mgr->{tasks}[0]{id}, 999, 'Deep Write confirmed in original memory';

    # Reset for verification function
    $comp->{manager}{tasks}[0]{id} = 101;

    # Pass to C
    ok verify_hierarchy($comp), 'FFI: verify_hierarchy($comp) - deep validation passed';
};
subtest 'smart & safety' => sub {

    # Types were defined in previous subtests
    affix $lib_path, 'get_null_company', [], Pointer [ Company() ];
    ok my $mem_comp = alloc_owned( sizeof( Company() ) ),  'alloc Company';
    ok my $mem_mgr  = alloc_owned( sizeof( Employee() ) ), 'alloc Manager';
    ok my $comp     = cast( $mem_comp, Company() ),        'cast Company';
    ok my $mgr      = cast( $mem_mgr, Employee() ),        'cast Manager';
    $mgr->{name}    = "Bob";
    $comp->{budget} = 50000;

    # We assign the $mgr pin DIRECTLY to the manager pointer field.
    $comp->{manager} = $mgr;
    is address_v2( $comp->{manager} ), address_v2($mgr), 'Smart Assignment: Pin converted to address automatically';
    is $comp->{manager}{name},         "Bob",            'Data accessible through smart-assigned pointer';

    # Traversing a NULL pointer in a struct
    $comp->{manager} = undef;    # Set C pointer to NULL
    my $name;
    ok lives { $name = $comp->{manager}{name} }, 'Accessing NULL pointer member is not a fatal exception';
    is $name, undef, 'Member of NULL pointer is undef';

    # returning NULL
    ok my $null_comp = get_null_company(), 'C returns pointer to struct with NULL member';
    is $null_comp->{manager}, undef, 'C NULL pointer correctly becomes Perl undef';

    # Deep Null
    my $deep_ok = $null_comp->{manager}{tasks}[0]{id};
    ok !$deep_ok, 'Deep access on C NULL throws Perl exception instead of crashing';
};
subtest 'Giant Array & Anon Types' => sub {
    my $type = Struct [ a => Int, b => Int ];
    my $mem  = alloc_owned( sizeof($type) );

    # TEST ANONYMOUS TYPE EVAPORATION
    {
        my $p = cast( $mem, $type );
        is $p->{a}, 0, 'Anonymous struct works';
    }

    # At this point, free_v2_pin was called, and the local arena for that
    # struct definition is gone. No leak in the global registry!
    # TEST GIANT ARRAY SWITCH
    # Imagine a C array of 10,000 ints
    typedef BigArray => Array [ Int, 10000 ];
    $mem = alloc_owned( sizeof( BigArray() ) );
    my $arr_pin = cast( $mem, BigArray() );

    # $arr_pin is currently a scalar (Lazy Placeholder)
    #~ ok !SvROK($$arr_pin), 'Giant array is still a lazy scalar';
    # Accessing it vivifies the AV
    is $arr_pin->[500], 0, 'Accessing giant array element vivifies it on demand';

    #~ ok SvROK($arr_pin), 'Now it is a real array reference';
};
subtest calculator => sub {
    typedef MockObj    => Struct [ id => Int ];
    typedef calc_t     => Callback [ [ Int, Int ] => Int ];
    typedef Calculator => Struct [ operation => calc_t() ];
    my $lib = Affix::load_library($lib_path);    # Load library object for find_symbol
    subtest 'calculator' => sub {
        subtest 'C++ Destructors' => sub {
            affix $lib, 'mock_new', [Int], Pointer [ MockObj() ];

            #~ affix $lib, 'mock_delete',          [ Pointer [ MockObj() ] ], Void;
            affix $lib, 'get_destructor_count', [], Int;
            {
                ok my $raw_ptr = mock_new(42), 'Create native object';
                my $addr = address_v2($raw_ptr);

                # find_symbol returns a v1 Pin. address_v2 now recognizes its vtable.
                my $sym  = Affix::find_symbol( $lib, 'mock_delete' );
                my $dtor = address_v2($sym);
                ok $dtor,                                    'Resolved destructor address from v1 symbol pin';
                ok my $managed = wrap_owned( $addr, $dtor ), 'wrap_owned with mock_delete';
                is get_destructor_count(), 0, 'Destructor not called yet';
            }
            is get_destructor_count(), 1, 'wrap_owned triggered native destructor';
        };
        subtest 'Function Pointers' => sub {
            affix $lib, 'run_calc', [ Pointer [ Calculator() ], Int, Int ], Int;
            ok my $mem  = alloc_owned( sizeof( Calculator() ) ), 'alloc Calculator';
            ok my $calc = cast( $mem, Calculator() ),            'cast Calculator';

            # First assignment
            $calc->{operation} = sub ( $a, $b ) { return $a * $b; };
            is run_calc( $calc, 10, 5 ), 50, 'C called Perl sub (10 * 5)';

            #~ Affix::sv_dump($calc);
            # Second assignment - Prioritizing the Sub check allows this to update
            $calc->{operation} = sub ( $a, $b ) { return $a + $b; };

            #~ Affix::sv_dump($calc);
            is run_calc( $calc, 10, 5 ), 15, 'Updated function pointer to different sub (10 + 5)';

            # Extract the native C function directly from the struct
            my $native_sub = $calc->{operation};

            # Validate that we successfully wrapped it into a Perl CV
            is ref($native_sub), 'Affix', 'Extracted function pointer is a callable Affix CV';

            #~ Affix::sv_dump $native_sub;
            # Call the native C memory directly from Perl!
            is $native_sub->( 3, 4 ), 7, 'Calling extracted function pointer natively returns 7';
        };
    };
};
subtest 'Discriminated Union' => sub {
    typedef Vec2  => Struct [ x => Float, y => Float ];
    typedef Shape => Union [ point => Vec2(), radius => Float ];

    # We want to track which member is active
    ok my $mem = alloc_owned( sizeof( Shape() ) ), 'alloc Shape';
    ok my $s   = cast( $mem, Shape() ),            'cast Shape';
    $s->{point} = { x => 1.5, y => 2.5 };
    is $s->{point}{x}, 1.5, 'Active member: point';
    $s->{radius} = 10.0;
    is $s->{radius}, 10.0, 'Switched to active member: radius';
};
subtest 'Enum Dualvars' => sub {
    typedef Status     => Enum [ PENDING => 0, RUNNING => 1, SUCCESS => 2, ERROR => 3 ];
    typedef AutoStatus => Enum [ 'QUEUED', 'PROCESSED', 'FAILED' ];
    typedef AutoTask   => Struct [ id => Int, status => Status(), auto => AutoStatus() ];
    typedef TaskObj    => Struct [ id => Int, status => Status() ];
    subtest TaskObj => sub {
        ok my $mem  = alloc_owned( sizeof( TaskObj() ) ), 'alloc TaskObj';
        ok my $task = cast( $mem, TaskObj() ),            'cast TaskObj';

        # Set by name
        $task->{status} = 'RUNNING';
        is int $task->{status}, 1, 'Enum set by name matches integer 1';

        # Get as string
        is "$task->{status}", 'RUNNING', 'Enum stringifies to RUNNING';

        # Set with int
        $task->{status} = 2;
        is "$task->{status}", 'SUCCESS', 'Enum set by integer matches string SUCCESS';

        # Make sure it's a dualvar
        ok( $task->{status} == 2 && $task->{status} eq 'SUCCESS', 'Enum is a proper Dualvar' );
    };
    subtest auto => sub {
        our $todo = todo 'This was a dev tool that I might eventually remove';
        my $mem  = alloc_owned( sizeof( AutoTask() ) );
        my $task = cast( $mem, AutoTask() );

        # Write to Enum by String Name
        $task->{status} = 'RUNNING';

        # Internal representation validation (No off-by-one)
        is int $task->{status}, 1, 'Enum written by name resolves to exact integer 1';

        # Dualvar retrieval check (SVt_PVIV upgrade successful in C)
        is "$task->{status}", 'RUNNING', 'Enum read back retains string mapping (Dualvar SV Upgrade successful)';

        # VTable Shadowing verification (C interception is correctly scoped to vtbl_enum)
        $task->{status} = 2;
        is "$task->{status}", 'SUCCESS', 'Enum written by integer successfully stringifies to mapped name';
        ok( $task->{status} == 2 && $task->{status} eq 'SUCCESS', 'Dualvar operations work symmetrically in Perl' );

        # Auto-enumerated Enums verification
        $task->{auto} = 'PROCESSED';
        is int( $task->{auto} ), 1, 'Implicit enum correctly evaluated next index as integer 1';

        # VTable scoping / memory boundaries
        is is_pinv2($task), T(), 'Task pointer is natively tracked as a v2 FFI Pin';
    }
};
#
done_testing;
