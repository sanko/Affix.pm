use Test2::V0;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
$|++;
use t::lib::helper;

# Cribbed from FFI::Platypus example
my $lib = compile_test_lib('56_affix_arrayref');
#
affix $lib, array_reverse10 => [ Array [ Int, 10 ] ], Int;
affix $lib, array_reverse   => [ Array [Int], Int ], Int;
affix $lib, array_sum       => [ Array [Int] ], Int;
#
subtest 'Array[Int, 10]' => sub {
    my @a = ( 1 .. 10 );
    is array_reverse10( \@a ), 55, 'array_reverse10( \@a )';
    is_deeply \@a, [ 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 ], 'contents reversed';
};
subtest 'Array[Int]' => sub {
    {
        my @a = ( 1 .. 20 );
        is array_reverse( \@a, 20 ), 210, 'array_reverse( \@a, 20 )';
        is_deeply \@a, [ 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 ], 'contents reversed';
    }
    {
        my $a = [ 1 .. 20 ];
        is array_reverse( $a, 20 ), 210, 'array_reverse( $a, 20 )';
        is_deeply $a, [ 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 ], 'contents reversed';
    }
};
subtest 'array_sum' => sub {
    is array_sum(undef),            -1, 'explicit undef';
    is array_sum( [0] ),            0,  '[0]';
    is array_sum( [ 1, 2, 3, 0 ] ), 6,  '[ 1, 2, 3, 0 ]';
};
#
done_testing;
