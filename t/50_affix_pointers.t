use strict;
no warnings 'portable';
use Affix     qw[:all];
use Dyn::Call qw[malloc memmove free];
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use File::Spec;
use t::lib::nativecall;
use experimental 'signatures';
$|++;
#
compile_test_lib('50_affix_pointers');
{
    sub pointer_test : Native('t/50_affix_pointers') :
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
{
    my $data = pack 'd', 590343.12351;    # Test pumping raw, packed data into memory
    my $ptr  = malloc length($data);
    memmove $ptr, $data, length $data;
    diag 'allocated ' . length($data) . ' bytes';
    is pointer_test(
        $ptr,
        [ 1 .. 5 ],
        5,
        sub {
            pass('our coderef was called');
            is_deeply \@_, [ 4, 8 ], '... and given correct arguments';
            50.25;
        }
        ),
        18.33825, 'making call with Dyn::Call::Pointer object with packed data';
    is unpack( 'd', $ptr ), 3.493, 'Dyn::Call::Pointer updated';
    free $ptr;
}
#
done_testing;
