use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::nativecall;
use experimental 'signatures';
$|++;
#
compile_test_lib('58_affix_import_vars');
#
sub get_integer : Native('t/58_affix_import_vars') : Signature([]=>Int);
sub get_string : Native('t/58_affix_import_vars') : Signature([]=>Str);
subtest 'integer' => sub {
    my ( $integer, $string );
    is get_integer(), 5, 'correct lib value returned';
    Affix::global( $integer, Affix::locate_lib('t/58_affix_import_vars'), 'integer', Int );
    is $integer, 5, 'correct initial value returned';
    ok $integer = 90, 'set value via magic';
    is get_integer(), 90, 'correct new lib value returned';
};
subtest 'string' => sub {
    my ( $integer, $string );
    is get_string(), 'Hi!', 'correct initial lib value returned';
    Affix::global( $string, Affix::locate_lib('t/58_affix_import_vars'), 'string', Str );
    is $string, 'Hi!', 'correct initial value returned';
    ok $string = 'Testing', 'set value via magic';
    is get_string(), 'Testing', 'correct new lib value returned';
};
done_testing;
