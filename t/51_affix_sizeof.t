use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
$|++;
#
TODO:
{
    local $TODO = 'Testing against hard coded sizes that might be incorrect for your platform';
    is sizeof(Char),  1, 'sizeof(Char) == 1';
    is sizeof(Float), 4, 'sizeof(Float) == 4';
    my $struct1 = Struct [ c => ArrayRef [ Char, 3 ] ];
    is sizeof($struct1), 3, 'sizeof($struct1) == 3';
    my $struct2 = Struct [ c => ArrayRef [ Int, 3 ] ];
    is sizeof($struct2), 12, 'sizeof($struct2) == 12';
    my $struct3 = Struct [ d => Double, c => ArrayRef [ Int, 3 ] ];
    is sizeof($struct3), 24, 'sizeof($struct3) == 24';
    my $struct4 = Struct [ y => $struct3 ];    # Make sure we are padding cached size data
    is sizeof($struct4), 24, 'sizeof($struct4) == 24';
    my $struct5 = Struct [ y => Struct [ d => Double, c => ArrayRef [ Int, 3 ] ] ];
    is sizeof($struct5), 24, 'sizeof($struct5) == 24';
    my $struct6 = Struct [ y => $struct3, s => ArrayRef [ $struct4, 4 ], c => Char ];
    is sizeof($struct6), 128, 'sizeof($struct6) == 128';
    my $array1 = ArrayRef [ Struct [ d => Double, c => ArrayRef [ Int, 3 ] ], 3 ];
    is sizeof($array1), 72, 'sizeof($array1) == 72';
    my $union1 = Union [ i => Int, d => Float ];
    is sizeof($union1), 4, 'sizeof($union1) == 4';
    my $union2 = Union [ i => Int, s => $struct1, d => Float ];
    is sizeof($union2), 4, 'sizeof($union2) == 4';
    my $union3 = Union [ i => Int, s => $struct3, d => Float ];
    is sizeof($union3), 24, 'sizeof($union3) == 24';
}
#
done_testing;
