use Test2::V0;
use lib '../lib', 'lib', '../blib/arch/auto/Affix', '../blib/lib';
use Affix;
#
$|++;
#
if ( Affix::Platform::OS() =~ /Win32/ ) {
    diag Affix::locate_lib('ntdll');
}
else {
    diag Affix::locate_lib('m');
}
#
pass 'yep';
#
done_testing;
