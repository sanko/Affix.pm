use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Dyn qw[wrap :dl :dc :types];
use Test::More;
use Config;
$|++;
#
my $libfile
    = $^O eq 'MSWin32' ? 'msvcrt.dll' :
    $^O eq 'darwin'    ? '/usr/lib/libm.dylib' :
    $^O eq 'bsd'       ? '/usr/lib/libm.so' :
    $Config{archname} =~ /64/ ?
    -e '/lib64/libm.so.6' ?
    '/lib64/libm.so.6' :
        '/lib/x86_64-linux-gnu/libm.so.6' :
    '/lib/libm.so.6';

#  "/usr/lib/system/libsystem_c.dylib", /* macos - note: not on fs w/ macos >= 11.0.1 */
#    "/usr/lib/libc.dylib",
#    "/boot/system/lib/libroot.so",       /* Haiku */
#    "\\ReactOS\\system32\\msvcrt.dll",   /* ReactOS */
#    "C:\\ReactOS\\system32\\msvcrt.dll",
#    "\\Windows\\system32\\msvcrt.dll",   /* Windows */
#    "C:\\Windows\\system32\\msvcrt.dll"
SKIP: {
    skip 'Cannot find math lib: ' . $libfile, 8 if $^O ne 'MSWin32' && !-f $libfile;
    diag 'Loading ' . $libfile . ' ...';
    my %loaders = (
        sin_default => Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_DEFAULT() ),
        sin_vararg  =>
            Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_ELLIPSIS_VARARGS() ),
        sin_ellipsis => Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_ELLIPSIS() ),
        sin_cdecl    => Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_CDECL() ),
        sin_stdcall  => Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_STDCALL() ),
        sin_fastcall =>
            Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_FASTCALL_GNU() ),
        sin_thiscall =>
            Dyn::attach( $libfile, 'sin', [Double], Double, DC_SIGCHAR_CC_THISCALL_GNU() )
    );
    my $correct = -0.988031624092862;    # The real value of sin(30);
    is sin(30), $correct, 'sin(30) [perl]';
    for my $fptr ( sort keys %loaders ) {
        if ( !defined $loaders{$fptr} ) {
            diag 'Failed to attach ' . $fptr;
        }
        else {
            is $loaders{$fptr}->(30), $correct, sprintf '$loaders{%s}->( 30 );', $fptr;

            #skip sprintf( '$loaders{%s}->( 30 ) failed: %s', $fptr, $@ ), 1 if $@;
        }
    }
}
done_testing;
