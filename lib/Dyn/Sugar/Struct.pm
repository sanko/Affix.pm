package Dyn::Sugar::Aggregate 0.03;

use v5.14;
use warnings;

use Object::Pad 0.56;

require XSLoader;
XSLoader::load( __PACKAGE__, our $VERSION );


sub import
{
   $^H{"Dyn::Sugar::Aggregate/Aggregate"}++;
}

sub new
{bless \[], shift}

0x55AA;