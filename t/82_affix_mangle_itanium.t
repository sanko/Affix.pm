use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use t::lib::helper;
use experimental 'signatures';
$|++;
#
my $lib = compile_test_lib('82_affix_mangle_itanium');

#~ system 'nm -D ' . $lib;
#
typedef 'MyClass' => Struct [ myNum => Int, myString => Str ];
#
subtest 'setup()' => sub {
    my $myclass = wrap( $lib => 'setup' => [] => MyClass() )->();
    is $myclass->{myNum},    15,          '.myNum == 15';
    is $myclass->{myString}, 'Some text', '.myString eq "Some text"';
};
subtest 'setup(int i)' => sub {
    my $myclass = wrap( $lib => 'setup' => [Int] => MyClass() )->(3);
    is $myclass->{myNum},    3,                     '.myNum == 3';
    is $myclass->{myString}, 'Some different text', '.myString eq "Some different text"';
    subtest 'MyClass::speed(...)' => sub {

        #my $set_speed = wrap( $lib  => 'MyClass::speed' => [Int] => Int )->(300);
        # ddx wrap( $lib, '_ZN7MyClass5speedEi' => [MyClass(), Int] => Int );
        # TODO: package MyClass{sub speed{...} } $obj->speed;
        my $ptr = ( Pointer [ MyClass() ] )->marshal( { myNum => 3 } );
        is wrap( $lib, 'MyClass::speed' => [ CC_THISCALL, Pointer [Void] ] => Int )->($ptr), 3,
            'this->speed() == 3';
        wrap( $lib, 'MyClass::speed' => [ CC_THISCALL, Pointer [Void], Int ] => Void )
            ->( $ptr, 400 );
        diag 'this->speed(400)';
        is wrap( $lib, 'MyClass::speed' => [ CC_THISCALL, Pointer [Void] ] => Int )->($ptr), 400,
            'this->speed() == 400';
    }
};
#
done_testing;
