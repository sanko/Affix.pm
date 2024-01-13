use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Affix;
use Test2::V0;
use Config;
$|++;
#
my $libfile = $^O eq 'darwin' ? '/usr/lib/libSystem.dylib' : Affix::locate_lib( $^O eq 'MSWin32' ? 'ntdll' : 'm' );
SKIP: {
    #~ skip 'I known nothing about MacOS',       2 if $^O eq 'darwin';
    $libfile // skip 'Cannot find math lib: ' . $libfile, 2;
    diag 'Loading ' . $libfile . ' ...';
    my $sin = Affix::wrap( $libfile, 'sin', [Double], Double );
    isa_ok $sin, 'Affix', 'sin(...)';
    my $correct
        = $Config{usequadmath} ? -0.988031624092861826547107284568483 :
        $Config{uselongdouble} ? -0.988031624092861827 :
        -0.988031624092862;    # The real value of sin(30);
    is $sin->(30), $correct, '$sin->( 30 );';
}
done_testing;
