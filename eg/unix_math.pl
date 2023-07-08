use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib';
use Affix;
use Config;
$|++;
my $libfile = Affix::locate_lib( $^O eq 'MSWin32' ? 'ntdll.dll' : ( 'm', 6 ) );
#
CORE::say 'sqrtf(36.f) = ' . Affix::wrap( $libfile, 'sqrtf', [Float] => Float )->(36.0);
CORE::say 'pow(2.0, 10.0) = ' .
    Affix::wrap( $libfile, 'pow', [ Double, Double ] => Double )->( 2.0, 10.0 );
