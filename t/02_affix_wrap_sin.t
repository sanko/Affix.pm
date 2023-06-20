use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Affix;
use Test::More;
use Config;
$|++;
#
my $libfile = Affix::locate_lib( $^O eq 'MSWin32' ? 'ntdll' : 'm' );
skip 'Cannot find math lib: ' . $libfile, 8 if $^O ne 'MSWin32' && !-f $libfile;
diag 'Loading ' . $libfile . ' ...';
my $sin = Affix::wrap( $libfile, 'sin', [Double], Double );
my $correct
    = $Config{usequadmath} ? -0.988031624092861826547107284568483 :
    $Config{uselongdouble} ? -0.988031624092861827 :
    -0.988031624092862;    # The real value of sin(30);
is $sin->(30), $correct, '$sin->( 30 );';
done_testing;
