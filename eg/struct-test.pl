#!perl
use Data::Dump;
use strictures 2;
use experimental 'signatures';
#
use lib './lib', '../lib', '../blib/arch', '../blib/lib';
use Dyn qw[:dc :dl :sugar];
use File::Find;
$|++;
#
my $path = '/home/sanko/Downloads/libui-ng-master/build/meson-out/libui.so.0';
warn $path;
my $lib = dlLoadLibrary($path);
warn;
my $init = dlSymsInit($path);
warn;
#
if (0) {
    CORE::say "Symbols in libm ($path): " . dlSymsCount($init);
    CORE::say 'All symbol names in libm:';
    CORE::say sprintf '  %4d %s', $_, dlSymsName( $init, $_ ) for 0 .. dlSymsCount($init) - 1;
    CORE::say 'libm has sqrtf()?5 ' .      ( dlFindSymbol( $lib, 'sqrtf' )       ? 'yes' : 'no' );
    CORE::say 'libm has pow()? ' .         ( dlFindSymbol( $lib, 'pow' )         ? 'yes' : 'no' );
    CORE::say 'libm has not_in_libm()? ' . ( dlFindSymbol( $lib, 'not_in_libm' ) ? 'yes' : 'no' );
    #
    CORE::say 'sqrtf(36.f) = ' . Dyn::wrap( $path, 'sqrtf', '(f)f' )->(36.0);
    CORE::say 'pow(2.0, 10.0) = ' . Dyn::wrap( $path, 'pow', 'dd)d' )->( 2.0, 10.0 );
}
use Dyn::Struct qw[Int UInt Object Str];
use Regexp::Common;
warn $RE{balanced}{ -begin => "{" }{ -end => "}" };
#
union Person::Level Int, Float;
enum Person::Level Int, Char;
class Person : version(v3.14.0) {
    field $name : isa(String);
    field $cash, $credit : isa(Float);
};
class Customer : isa( Person 3.14 ) : version(v2.1.0) {
    field $size : isa(Int) : reader;
    field @numbers : isa(UFloat);
    field $next : isa(Customer);
    method serve( Int $first, Person::Level $level);
    method reject( String $reason);
    method insert() : common;
}

#union
#
abstract struct Fire;
struct Wildfire : isa(Fire);

#field Int $i;
struct Access {
    has Int $j;
};
Dyn::Struct::dump();
Access->new();
Fire->new;
warn 'DONE';
__END__
class Account {
    has Int $size;
    has Fire::Place $fireplace;
    has ArrayRef [Int] @numbers;

    #has Int $i;
    has Access $!signatories;
has Str $.name;
    has Int $.ID;
    has UInt $!pwd;
    has Str @.history;
        # etc.
}


    struct Ui::Options {
            field $blah;
    }

sub struct ( $name, %fields ) { warn $name; ddx \%fields; }
struct Opt => ( size => Int );
my $o = Dyn::Call::newStruct(0);
warn $o;
warn Dyn::wrap( $path, 'uiInit', '(p)Z' )->($o);
Dyn::Call::letsgo($o);
