use strict;
use Test::More 0.98;
use lib '../lib', 'lib';
diag 'perl ' . $^V . ' @ ' . $^X;
use_ok $_ for qw[Affix];
#
diag 'Platform info:';
diag '  dyncall ver: ' . Affix::Platform::Dyncall_Version();
diag '           OS: ' . Affix::Platform::OS();
if ( Affix::Platform::OS() eq 'Win32' || Affix::Platform::OS() eq 'Win64' ) {
    diag '               Cygwin: ' . ( Affix::Platform::Win32::Cygwin() ? 'yes' : 'no' );
    diag '                MinGW: ' . ( Affix::Platform::Win32::MinGW()  ? 'yes' : 'no' );
    diag '               MSVCRT: ' . ( Affix::Platform::MSVCRT()        ? 'yes' : 'no' );
}
diag '      syscall: ' . ( Affix::Platform::Syscall()   ? 'yes' : 'no' );
diag '    aggrbyval: ' . ( Affix::Platform::AggrByVal() ? 'yes' : 'no' );
diag '     compiler: ' . Affix::Platform::Compiler();
diag ' architecture: ' . Affix::Platform::Architecture();
if ( Affix::Platform::Architecture() eq 'ARM' || Affix::Platform::Architecture() eq 'ARM64' ) {
    diag '                    Thumb: ' . ( Affix::Platform::ARM::Thumb() ? 'yes' : 'no' );
    diag '               Hard Float: ' . ( Affix::Platform::ARM::HF()    ? 'yes' : 'no' );
    diag '                     EABI: ' . ( Affix::Platform::ARM::EABI()  ? 'yes' : 'no' );
    diag '                     OABI: ' . ( Affix::Platform::ARM::OABI()  ? 'yes' : 'no' );
}
elsif ( Affix::Platform::Architecture() eq 'MIPS' || Affix::Platform::Architecture() eq 'MIPS64' ) {
    diag '                   o32 CC: ' . ( Affix::Platform::MIPS::O32()  ? 'yes' : 'no' );
    diag '               Hard Float: ' . ( Affix::Platform::MIPS::HF()   ? 'yes' : 'no' );
    diag '                     EABI: ' . ( Affix::Platform::MIPS::EABI() ? 'yes' : 'no' );
}
#
done_testing;
