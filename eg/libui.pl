#!perl
use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib';
use Dyn qw[:sugar :dl];
use Dyn::Load qw[:all];
use Dyn::Call qw[:all];
$|++;
#
my $path = '/home/sanko/Downloads/libui-ng-master/build/meson-out/libui.so.0';
my $lib  = dlLoadLibrary($path);
my $init = dlSymsInit($path);
#
#CORE::say "Symbols in user32 ($path): " . dlSymsCount($init);
#CORE::say 'All symbol names in user32:';
for my $i ( 0 .. dlSymsCount($init) - 1 ) {
    my $name = dlSymsName( $init, $i );
    CORE::say sprintf '  %4d %s', $i, $name if $name =~ m[^ui];
}

#CORE::say 'user32 has MessageBoxA()? ' . ( dlFindSymbol( $lib, 'MessageBoxA' ) ? 'yes' : 'no' );
#CORE::say 'user32 has NonExistant()? ' . ( dlFindSymbol( $lib, 'NonExistant' ) ? 'yes' : 'no' );
#
#CORE::say 'MessageBoxA(...) = ' .
#   Dyn::wrap( $lib, 'MessageBoxA', '(IZZI)i' )->( 0, 'JAPH!', 'Hello, World', 0 );
my $init = Dyn::wrap( $lib, 'uiInit',        '(p)Z' );
my $main = Dyn::wrap( $lib, 'uiMain',        '()v' );
my $quit = Dyn::wrap( $lib, 'uiQuit',        '()' );
my $win  = Dyn::wrap( $lib, 'uiNewWindow',   '(Ziii)p' );
my $show = Dyn::wrap( $lib, 'uiControlShow', 'p)v' );
#####warn Dyn::Call::getopts();
#die;
my $s = dcNewStruct( 1, 0 );    # DEFAULT_STRUCT_ALIGNMENT
dcStructField( $s, DC_SIGCHAR_DOUBLE, DEFAULT_ALIGNMENT, 1 );
dcCloseStruct($s);
warn dcStructSize($s);
Dyn::Call::newStruct(1);
exit;
warn $s;
my $o = Dyn::Call::dcAllocMem( dcStructSize($s) );
warn $o;
warn $o;
exit Dyn::Call::letsgo();

#warn $init->(\$o);
warn $win;
my $w = $win->( 'Hi, this is a test', 100, 100, 1 );
use Data::Dump;
ddx $w;
$show->($w);
$main->();
$quit->();
dcFreeStruct($s);

#/home/sanko/Downloads/libui-ng-master/build/meson-out/libui.so.0
