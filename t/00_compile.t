use Test2::V0;
use lib '../lib', 'lib', '../blib/arch/auto/Affix', '../blib/lib';
diag 'perl ' . $^V . ' @ ' . $^X;
use Affix;
ok $Affix::VERSION, 'Affix v' . $Affix::VERSION;
#
diag 'Platform info:';
diag '  dyncall ver: ' . Affix::Platform::DC_Version();
diag '     features: aggrbyval: ' . ( Affix::Platform::AggrByValue() ? 'yes' : 'no' );
diag '                 syscall: ' . ( Affix::Platform::Syscall()     ? 'yes' : 'no' );
diag '     compiler: ' . Affix::Platform::Compiler();
diag ' architecture: ' . Affix::Platform::Architecture();
diag '           os: ' . Affix::Platform::OS();

if ( Affix::Platform::OS() =~ m'Win32' ) {
    diag '               Cygwin: ' . ( Affix::Platform::MS_Cygwin() ? 'yes' : 'no' );
    diag '                MinGW: ' . ( Affix::Platform::MS_MinGW()  ? 'yes' : 'no' );
    diag '               MSVCRT: ' . ( Affix::Platform::MS_CRT()    ? 'yes' : 'no' );
}
if ( Affix::Platform::Architecture() =~ m'ARM' ) {
    diag '                    Thumb: ' . ( Affix::Platform::ARM_Thumb() ? 'yes' : 'no' );
    diag '               Hard Float: ' . ( Affix::Platform::HardFloat() ? 'yes' : 'no' );
    diag '                     EABI: ' . ( Affix::Platform::ARM_EABI()  ? 'yes' : 'no' );
    diag '                     OABI: ' . ( Affix::Platform::ARM_OABI()  ? 'yes' : 'no' );
}
elsif ( Affix::Platform::Architecture() =~ m'MIPS' ) {
    diag '                   o32 CC: ' . ( Affix::Platform::MIPS_O32()  ? 'yes' : 'no' );
    diag '               Hard Float: ' . ( Affix::Platform::HardFloat() ? 'yes' : 'no' );
    diag '                     EABI: ' . ( Affix::Platform::MIPS_EABI() ? 'yes' : 'no' );
}
pass 'okay';
#
done_testing;
