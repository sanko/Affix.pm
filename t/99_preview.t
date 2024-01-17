use Test2::V0;
use lib '../lib', 'lib', '../blib/arch/auto/Affix', '../blib/lib';
use Affix qw[:all];
warn $0;
warn "$^X";
#
$|++;
#
my $affix = affix 'm', 'pow', Struct [], Void;
#
pow();
$affix->call();
isa_ok Pointer [Int], [ 'Affix::Type', 'Affix::Type::Pointer' ];

#~ Affix::args( Pointer [Int] );
#
done_testing;
