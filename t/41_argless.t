use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Dyn qw[:all];
use File::Spec;
use t::lib::nativecall;
#
compile_test_lib('41_argless');
#
sub Nothing : Native('t/41_argless')                              {...}
sub Argless : Native('t/41_argless') : Signature('()i')           {...}
sub ArglessChar : Native('t/41_argless') : Signature('()c')       {...}
sub ArglessLongLong : Native('t/41_argless') : Signature('()l')   {...}
sub ArglessPointer : Native('t/41_argless') : Signature('()p')    {...}    # Pointer[int32]
sub ArglessUTF8String : Native('t/41_argless') : Signature('()Z') {...}
sub short : Native('t/41_argless') : Signature('()i') : Symbol('long_and_complicated_name') {...}
#
Nothing();
pass 'survived the call';
#
is Argless(),         2, 'called argless function returning int32';
is ArglessChar(),     2, 'called argless function returning char';
is ArglessLongLong(), 2, 'called argless function returning long long';
isa_ok ArglessPointer(), 'Dyn::pointer', 'called argless function returning pointer';
is ArglessUTF8String(), 'Just a string', 'called argless function returning string';
is short(),             3,               'called long_and_complicated_name';

#sub test_native_closure() {
#    my sub Argless :Native('t/41_argless') : Signature('()i') { ... }
#    is Argless(), 2, 'called argless closure';
#}
#test_native_closure();
#test_native_closure(); # again cause we may have created an optimized version to run
done_testing;
