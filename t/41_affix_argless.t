use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::helper;
#
diag compile_test_lib('41_argless');
#
sub Nothing : Native('t/src/41_argless');
sub Argless : Native('t/src/41_argless') : Signature([]=>Int);
sub ArglessChar : Native('t/src/41_argless') : Signature([]=>Char);
sub ArglessLongLong : Native('t/src/41_argless') : Signature([]=>LongLong);
sub ArglessIntPointer : Native('t/src/41_argless') : Signature([]=>Pointer[Int]) :
    Symbol('ArglessPointer');    # Pointer[int32]
sub ArglessVoidPointer : Native('t/src/41_argless') : Signature([]=>Pointer[Void]) :
    Symbol('ArglessPointer');    # Pointer[int32]
sub ArglessUTF8String : Native('t/src/41_argless') : Signature([]=>Str);
sub short : Native('t/src/41_argless') : Signature([]=>Short) : Symbol('long_and_complicated_name');
#
Nothing();
pass 'survived the call';
#
is Argless(),                2, 'called argless function returning int32';
is ArglessChar(),            2, 'called argless function returning char';
is ArglessLongLong(),        2, 'called argless function returning long long';
is ${ ArglessIntPointer() }, 2, 'called argless function returning int pointer';
isa_ok ArglessVoidPointer(), 'Affix::Pointer', 'called argless function returning void pointer';
is ArglessUTF8String(), 'Just a string', 'called argless function returning string';
is short(),             3,               'called long_and_complicated_name';

sub test_native_closure() {
    my sub Argless : Native('t/src/41_argless') : Signature([]=>Int) { }
    is Argless(), 2, 'called argless closure';
}

#test_native_closure();
#test_native_closure(); # again cause we may have created an optimized version to run
done_testing;
