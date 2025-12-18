use v5.40;
use lib '../lib', 'lib';
use blib;
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];
#
$|++;
#
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
#
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
done_testing;
