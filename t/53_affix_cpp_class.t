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
my $lib = compile_test_lib('53_affix_cpp_class');
#
#~ use Data::Dump;
#~ warn `nm -D $lib`;
isa_ok my $Box = typedef( Box => CPPStruct [ w => Int, l => Int, h => Int ] ), 'Affix::Type::CPPStruct', 'typedef ...';
isa_ok affix( $lib, 'Box::new' => [ CC_THISCALL, Int, Int, Int ] => $Box ),    'Affix',                  'Box::new';
isa_ok affix( $lib, [ 'Box::Volume' => 'Box::volume' ] => [] => Int ),         'Affix',                  'Box::volume';
isa_ok my $box = Box->new( 1, 2, 3 ),                                          'Box',                    '$box = Box->new(1, 2, 3)';
is $box->volume, 6, '$box->volume == 6';
#
done_testing;
__END__
#~ sub Class {
#~ my @args = shift;
#~ ddx \@args;
#~ }
#~ sub Public  { }
#~ sub Private { }
#~ my $class = Class [ Public [], Private [] ];
ddx Struct [ age => Float ];
ddx typedef 'Animal' => CPPStruct [ age => Int, name => Str ];
ddx CPPStruct [ age => Int, name => Str ];
ddx Animal();

#~ ...;
#
warn `nm -D $lib`;
warn '_ZN6AnimalC1Ei';
my $affix = affix $lib, 'Animal::new' => [Int] => Animal();
warn $affix;

#~ ddx Affix::args($affix);
ddx $affix->args;
ddx $affix->retval;
warn $affix->cpp_struct;
warn $affix->cpp_const;

#~ ...;
warn '_ZN6Animal8set_nameEPc';
affix $lib, 'Animal::set_name' => [ Animal(), Str ] => Int;
warn '_ZN6Animal8get_nameEv';
affix $lib, 'Animal::get_name' => [ Animal() ] => Str;
warn '_ZN6Animal5matchES_';
affix $lib, 'Animal::match'   => [ Animal(), Animal() ] => Bool;
affix $lib, 'Animal::DESTROY' => [ Animal() ]           => Void;
#
warn;
isa_ok my $ptr = Animal::new(1), 'Animal', 'Animal::new()';
warn $ptr;
warn ref $ptr;
ddx $ptr;
...;

#~ ddx $ptr;
warn 'x' x 30;
is $ptr->set_name('Jack'), 4,      q[$ptr->set_name('Jack')];
is $ptr->get_name,         'Jack', '$ptr->get_name eq "Jack"';
like exception { Animal::get_name( bless {}, 'Broken' ) }, qr[subclass of Animal],
    q[Animal::get_name(bless {}, 'Broken') throws exception];
done_testing;
