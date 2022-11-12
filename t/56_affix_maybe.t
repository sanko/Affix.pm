use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
use Dyn::Call qw[malloc];
$|++;
use t::lib::nativecall;
#
compile_test_lib('56_affix_maybe');
#
sub CheckNullPtr : Native('t/56_affix_maybe') : Signature([Maybe[Int]]=>Bool);

#sub CheckNullPtr : Native('t/56_affix_maybe') : Signature([Maybe[Pointer[Int]]]=>Bool);
my $hi = undef;
is CheckNullPtr($hi),   !1, 'CheckNullPtr($hi = undef) == -1';
is CheckNullPtr(undef), !1, 'CheckNullPtr(undef) == -1';
is CheckNullPtr(),      !1, 'CheckNullPtr() == -1';
is CheckNullPtr(10),    1,  'CheckNullPtr(10) == 1';
done_testing;
