use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
use Config;
$|++;
#
use t::lib::helper;
#
my $lib = compile_test_lib('70_affix_varargs');
#
subtest 'calling conventions' => sub {
    Test::More::can_ok( 'Affix', $_ ) for qw[
        CC_DEFAULT
        CC_THISCALL
        CC_ELLIPSIS     CC_ELLIPSIS_VARARGS
        CC_CDECL
        CC_STDCALL
        CC_FASTCALL_MS  CC_FASTCALL_GNU
        CC_THISCALL_MS  CC_THISCALL_GNU
        CC_ARM_ARM      CC_ARM_THUMB
        CC_SYSCALL];
};
subtest 'ellipsis varargs' => sub {
    is Affix::wrap( $lib, 'average', [ Int, CC_ELLIPSIS_VARARGS, Int, Int ], Int )->( 2, 3, 4 ), 3,
        'average( 2, 3, 4 )';
    is Affix::wrap( $lib, 'average', [ Int, CC_ELLIPSIS, Int, Int, Int ], Int )->( 3, 5, 10, 15 ),
        10, 'average( 3, 5, 10, 15 )';
};
#
done_testing;
