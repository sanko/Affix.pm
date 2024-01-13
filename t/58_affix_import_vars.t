use Test2::V0;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::helper;
use experimental 'signatures';
$|++;
#
compile_test_lib( '58_affix_import_vars', '', 1 );
#
affix 't/src/58_affix_import_vars', 'get_integer' => [] => Int;
affix 't/src/58_affix_import_vars', 'get_string'  => [] => Str;
#
subtest 'integer' => sub {
    is get_integer(), 5, 'correct lib value returned';
    pin( my $integer, 't/src/58_affix_import_vars', 'integer', Int );
    is $integer, 5, 'correct initial value returned';
    ok $integer = 90, 'set value via magic';
    is get_integer(), 90, 'correct new lib value returned';
};
#
subtest 'string' => sub {
    is get_string(), 'Hi!', 'correct initial lib value returned';
    pin( my $string, Affix::locate_lib('t/src/58_affix_import_vars'), 'string', Str );
    is $string, 'Hi!', 'correct initial value returned';
    ok $string = 'Testing', 'set value via magic';
    is get_string(), 'Testing', 'correct new lib value returned';
};
#
done_testing;
