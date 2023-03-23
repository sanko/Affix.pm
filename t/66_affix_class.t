use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::nativecall;
use experimental 'signatures';
$|++;

# https://www.gnu.org/software/libunistring/manual/html_node/The-wchar_005ft-mess.html
#plan skip_all => 'wchar * is broken on *BSD and Solaris' if $^O =~ /(?:bsd|solaris)/i;
#
my $lib = compile_test_lib('66_affix_class');

#warn system 'nm -D t/src/66_affix_class.so';
#
typedef 'MyClass' => Struct [ myNum => Int, myString => Str ];
#
diag 'Itanium support is still early';

#~ affix [ $lib, 'C' ] => [ '_Z5setupv' => 'setup' ] => [] => MyClass();
affix [ $lib, 'I' ] => 'setup' => [] => MyClass();

#~ affix $lib, [ '_ZN7MyClass5speedEi' => 'speed' ] => [] => Void;
#~ warn affix $lib,          [ 'MyClass::speed' => 'speed' ] => [] => Void;
#use Data::Dump;
my $myclass = setup();

#ddx $myclass;
is $myclass->{myNum},    15,          '.myNum';
is $myclass->{myString}, 'Some text', '.myString';

#system 'nm -D ' . $lib;
#
done_testing;
