use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Dyn qw[wrap :dl];
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
        sin_default  => Dyn::wrap( $libfile, 'sin', '(d)d' ),
        sin_vararg   => Dyn::wrap( $libfile, 'sin', '(_:d)d' ),
        sin_ellipsis => Dyn::wrap( $libfile, 'sin', '(_.d)d' ),
        sin_cdecl    => Dyn::wrap( $libfile, 'sin', '(_cd)d' ),
        sin_stdcall  => Dyn::wrap( $libfile, 'sin', '(_sd)d' ),
        sin_fastcall => Dyn::wrap( $libfile, 'sin', '(_fd)d' ),
        sin_thiscall => Dyn::wrap( $libfile, 'sin', '(_#d)d' )
    );
    my $correct = -0.988031624092862;    # The real value of sin(30);
    is sin(30), $correct, 'sin(30) [perl]';
    for my $fptr ( keys %loaders ) {
        if ( !$loaders{$fptr} ) {
            diag 'Failed to attach ' . $fptr;
        }
        else {
            diag 'Attached ' . $fptr;
            eval { is $loaders{$fptr}->(30), $correct, sprintf '$loaders{%s}->( 30 );', $fptr };
            skip sprintf( '$loaders{%s}->( 30 ) failed: %s', $fptr, $@ ), 1 if $@;
        }
    }
}
done_testing;
