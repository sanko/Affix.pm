use v5.40;
use blib;
use Affix               qw[malloc affix :types sizeof];
use Test2::Tools::Affix qw[:all];
#
my $C_CODE = <<'END_C';
#include "std.h"
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
#
done_testing;
