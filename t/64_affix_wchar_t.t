use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::nativecall;
use experimental 'signatures';
$|++;

# https://www.gnu.org/software/libunistring/manual/html_node/The-wchar_005ft-mess.html
plan skip_all => 'wchar * is broken on *BSD and Solaris' if $^O =~ /(?:bsd|solaris)/i;
#
compile_test_lib('64_affix_wchar_t');
sub check_string : Native('t/src/64_affix_wchar_t') : Signature([WStr]=>Int);
sub get_string : Native('t/src/64_affix_wchar_t') : Signature([]=>WStr);
#
subtest 'sv2ptr=>ptr2sv round trip' => sub {
    is check_string('時空'), 0, '[WStr]=>Int';

    #is get_string(),           '時空', '[]=>WStr';
};
#
done_testing;
