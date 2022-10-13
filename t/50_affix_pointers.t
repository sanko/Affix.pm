use strict;
no warnings 'portable';
use Affix qw[:all];
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use File::Spec;
use t::lib::nativecall;
use experimental 'signatures';
$|++;
#
compile_test_lib('50_types_pointers');
{
    sub pointer_test : Native('t/50_types_pointers') :
        Signature([Pointer[Double], ArrayRef [ Int, 5 ], Int, CodeRef [ [ Int, Int ] => Double ] ] => Double);
    my $ptr = 99;
    is 900, pointer_test(
        \$ptr,
        [ 1 .. 5 ],
        5,
        sub {
            pass('our coderef was called');
            is_deeply \@_, [ 4, 8 ], '... and given correct arguments';
            50.25;
        }
        ),
        'making call to test various types of pointers';
    is $ptr, 100.5, 'Pointer[Double] was updated!';
}
{
    is pointer_test(
        undef,
        [ 1 .. 5 ],
        5,
        sub {
            pass('our coderef was called');
            is_deeply \@_, [ 4, 8 ], '... and given correct arguments';
            50.25;
        }
        ),
        -1, 'making call with an undef pointer passes a NULL';
}
#
done_testing;
