use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
use File::Spec;
use t::lib::helper;
use experimental 'signatures';
$|++;
plan skip_all => 'no support for aggregates by value' unless Affix::Platform::AggrByValue();
#
my $lib    = compile_test_lib('45_affix_aggr_returns');
my $struct = Affix::wrap( $lib, 'get_struct' => [] => Struct [ i => Int, Z => Str ] )->();
isa_ok $struct, 'HASH', 'get_struct()';
#
is $struct->{i}, 15,               '->{i}';
is $struct->{Z}, 'This is a test', '->{Z}';
done_testing;
