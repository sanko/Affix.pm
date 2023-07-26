use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
use File::Spec;
use t::lib::helper;
use experimental 'signatures';
$|++;
plan skip_all => 'no support for aggregates by value' unless Affix::Platform::AggrByVal();
#
my $lib = compile_test_lib('44_affix_aggr_args');
#
is Affix::wrap( $lib, TakeIntStruct => [ Struct [ int => Int ] ] => Int )->( { int => 42 } ), 1,
    'passed struct with a single int';
is Affix::wrap( $lib, TakeIntIntStruct => [ Struct [ a => Int, b => Int ] ] => Int )
    ->( { a => 5, b => 9 } ), 14, 'passed struct with a two ints';
is Affix::wrap( $lib, TakeIntArray => [ ArrayRef [ Int, 3 ] ] => Int )->( [ 1, 2, 3 ] ), 6,
    'passed array with a three ints';
#
done_testing;
