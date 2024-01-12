use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use t::lib::helper;

#~ use experimental 'signatures';
use Devel::CheckBin;
use Config;
$|++;

# Tests taken from Affix::ABI::Fortran
#~ plan tests => 6;
# https://fortran-lang.org/learn/building_programs/managing_libraries/
SKIP: {
    my $compiler = can_run('gfortran');
    my $gnu      = !!$compiler;
    $compiler = can_run('ifort') unless $compiler;    # intel
    skip 'test requires GNUFortran', 6 unless $compiler;
    my $lib  = './t/src/86_affix_abi_fortran/' . ( $^O eq 'MSWin32' ? '' : 'lib' ) . 'affix_fortran.' . $Config{so};
    my $line = sprintf '%s t/src/86_affix_abi_fortran/hello.f90 -fPIC %s -o %s', $compiler,
        ( $gnu ? '-shared' : ( $^O eq 'MSWin32' ? '/libs:dll' : $^O eq 'darwin' ? '-dynamiclib' : '-shared' ) ), $lib;
    diag $line;
    diag `$line`;
    subtest 'function func(i) result(j)' => sub {
        ok affix( $lib, 'func', [Int], Int ), 'affix ..., "func", [Int], Int';
        is func(3), 36, 'func( 3 ) == 36';
    };
    subtest 'recursive function fact(i) result(j)' => sub {
        ok affix( $lib, 'fact', [ Pointer [Int] ], Int ), 'affix ..., "fact", [Int], Int';
        is fact(5), 120, 'fact( 5 ) == 120';
    };
    subtest 'function sum_v( x, y ) result(z)' => sub {
        diag 'by value args test';
        ok affix( $lib, 'sum_v', [ Int, Int ], Int ), 'affix ..., "sum_v", [Int, Int], Int';
        is sum_v( 5, 10 ), 15, 'sum_v( 5, 10 ) == 15';
    };
    subtest 'function sum_r( x, y ) result(z)' => sub {
        diag 'by reference args test';
        ok affix( $lib, 'sum_r', [ Pointer [Int], Pointer [Int] ], Int ), 'affix ..., "sum_r", [Pointer[Int], Pointer[Int]], Int';
        is sum_r( 5, 10 ), 15, 'sum_r( 5, 10 ) == 15';
    };
    subtest 'subroutine square_cube(i, isquare, icube)' => sub {
        ok affix( $lib, 'square_cube', [ Pointer [Int], Pointer [Int], Pointer [Int] ], Void ),
            'affix ..., "square_cube", [Pointer[Int], Pointer[Int], Pointer[Int]], Void';
        diag 'square_cube( 4, \my $square, \my $cube );';
        square_cube( 4, \my $square, \my $cube );
        diag sprintf 'i=%d, i^2=%d, i^3=%d', 4, $square, $cube;
        is $square, 16, '4^2 == 16';
        is $cube,   64, '4^3 == 64';

        #~ is sum_r( 5, 10 ), 15, 'square_cube( 5, 10 ) == 15';
    };
    subtest 'four forms for fortran functions' => sub {
        ok affix( $lib, 'f1', [ Pointer [Int] ], Int ), 'function f1(i) result (j)';
        ok affix( $lib, 'f2', [ Pointer [Int] ], Int ), 'integer function f2(i) result (j)';
        ok affix( $lib, 'f3', [ Pointer [Int] ], Int ), 'integer function f3(i)';
        ok affix( $lib, 'f4', [ Pointer [Int] ], Int ), 'function f4(i)';
        is f1(0), 1, 'f1(0)';
        is f2(0), 2, 'f2(0)';
        is f3(0), 3, 'f3(0)';
        is f4(0), 4, 'f4(0)';
    };
    subtest 'optional arguments' => sub {
        ok affix( $lib, 'tester', [ Varargs, Pointer [Float] ], Float ), 'affix ..., "tester", [ Varargs, Pointer[Float] ], Float';
        is sprintf( '%1.2f', tester() ),    '0.00', 'tester() == 0.00';
        is sprintf( '%1.2f', tester(1.0) ), '1.00', 'tester(1.0) == 1.00';
        is sprintf( '%1.2f', tester() ),    '0.00', 'tester() == 0.00';
        is sprintf( '%1.2f', tester() ),    '0.00', 'tester() == 0.00';
        use Data::Dump;
        ddx( ( Pointer [Float] )->cast(5) );
        tester( ( Pointer [Float] )->cast(5) );
    };
    done_testing;
    exit;
    #
    subtest 'integer function f_add(x, y)' => sub {
        ok affix( $lib, 'f_add', [ Int, Int ], Int ), 'affix ..., "f_add", [Int, Int], Int';
        is f_add( 3, 4 ), 7, 'f_add(3, 4) == 7';
    };
    subtest 'subroutine s_add(res, x, y)' => sub {
        ok affix( $lib, 's_add', [ Pointer [Int], Pointer [Int], Pointer [Int] ], Void ),
            'affix ..., "s_add", [Pointer[Int], Pointer[Int], Pointer[Int]], Void';
        s_add( \my $res, 5, 4 );
        is $res, 9, 's_add(\my $res, 5, 4); $res == 7';
    };
    done_testing;
    exit;

    # add.pl
    use Affix;
    affix $lib, 'f_add', [ Int, Int ], Int;
    CORE::say 'Sum of 3 and 4 = ' . f_add( 3, 4 );    # Sum of 3 and 4 = 7

    #
    use Affix;
    affix $lib, 's_add', [ Pointer [Int], Pointer [Int], Pointer [Int] ], Void;
    my $value = 0;
    s_add( \$value, 3, 4 );
    warn $value;
    CORE::say 'Sum of 3 and 4 = ' . $value;           # Sum of 3 and 4 = 7
    die;

    #~ diag( ( -s $lib ) . ' bytes' );
    diag `nm -D $lib`;
    ok affix( $lib, 'fstringlen', [String] => Int ), 'bound fortran function `fstringlen`';
    is fstringlen('Hello, World!'), 13, 'fstringlen("Hello, World!") == 13';
    ok affix( $lib, 'math::add', [ Int, Int ], Int ), 'bound fortran function `math::add` (module as namespace)';
    is math::add( 5, 4 ), 9, 'math::add(5, 4) == 9';
    ok affix( $lib, 'add', [ Int, Int ] => Int ), 'bound fortran function `add` (top level namespace)';
    is add( 5, 4 ), 9, 'add(5, 4) == 9';
    ok affix( $lib, 'sum_arr', [ Array [ Int, 3 ], Int ] => Int ), 'bound fortran function `sum_arr`';
    my $type = Array [ Int, 3 ];
    warn ref $type;
    my $array = $type->marshal( [ 1 .. 3 ] );
    warn $array;
    warn ref $array;
    use Data::Dump;
    ddx $array;
    $array->dump(16);
    diag sum_arr( $array, 3 );
    {
        ok affix( $lib, 'addc', [ Pointer [Int], Pointer [Int], Pointer [Int] ] => Void ), 'bound fortran subroutine `addc` (top level namespace)';
        my $var;
        is addc( $var, 5, 4 ), 9, 'add(5, 4) == 9';
        warn $var;
    }
    diag 'might fail to clean up on Win32 because we have not released the lib yet... this is fine' if $^O eq 'MSWin32';
    unlink $lib;
}
#
done_testing;
