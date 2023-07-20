use strict;
no warnings 'portable';
use Test::More 0.98;
use Test::Fatal qw[exception];
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:memory :default :types :cc];
use File::Spec;
use t::lib::helper;
use Config;
$|++;
#
my $lib = compile_test_lib('53_affix_instanceof');
#
#~ warn `nm -D $lib`;
affix $lib, get_class           => []                              => InstanceOf ['MyClass'];
affix $lib, 'MyClass::set_name' => [ InstanceOf ['MyClass'], Str ] => Int;
affix $lib, 'MyClass::get_name' => [ InstanceOf ['MyClass'] ]      => Str;
affix $lib, 'MyClass::DESTROY'  => [ InstanceOf ['MyClass'] ]      => Void;
#
isa_ok my $ptr = get_class(), 'MyClass', '$ptr = get_class() ';
is $ptr->set_name('Jack'), 4,      q[$ptr->set_name('Jack')];
is $ptr->get_name,         'Jack', '$ptr->get_name eq "Jack"';
like exception { MyClass::get_name( bless {}, 'Broken' ) }, qr[subclass of MyClass],
    q[MyClass::get_name(bless {}, 'Broken') throws exception];
done_testing;
