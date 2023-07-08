use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
#
affix get_lib(), 'puts', [Str], Int;
is puts("Wow!"), 5, 'puts("Wow!")';

affix get_lib(), ['getlogin', 'login'], [], Str;
warn login();
# https://learn.microsoft.com/en-us/cpp/porting/visual-cpp-change-history-2003-2015?view=msvc-170#mathh
#~ Affix::affix( get_lib(), 'pow', [ Double, Double ], Double );
#~ is pow( 2, 10 ), 1024, 'pow( 2, 10 ) == 1024';
done_testing;

sub get_lib {
    return 'ntdll'                    if $^O eq 'MSWin32';
    return '/usr/lib/libSystem.dylib' if $^O eq 'darwin';
    ( 'm', 6 );
}
