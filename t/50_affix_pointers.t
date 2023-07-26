use strict;
no warnings 'portable';
use Affix qw[:memory :default :types];
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use File::Spec;
use t::lib::helper;
use Config;
$|++;
#
#~ plan skip_all => 'no support for aggregates by value' unless Affix::Platform::AggrByVal();
my $lib = compile_test_lib('50_affix_pointers');
#
affix $lib, pointer_test =>
    [ Pointer [Double], ArrayRef [ Int, 5 ], Int, CodeRef [ [ Int, Int ] => Double ] ] => Double;
affix $lib, dbl_ptr => [ Pointer [Double] ] => Str;
#
diag __LINE__;
subtest 'marshalled pointer' => sub {
    my $type = Pointer [Double];
    diag __LINE__;
    my $ptr = $type->marshal(100);
    is dbl_ptr($ptr), 'one hundred', 'dbl_ptr($ptr) where $ptr == 100';
    diag __LINE__;
    is $type->unmarshal($ptr), 1000, '$ptr was changed to 1000';
    diag __LINE__;
};
subtest 'scalar ref' => sub {
    diag __LINE__;
    my $ptr = 100;
    is dbl_ptr($ptr), 'one hundred', 'dbl_ptr($ptr) where $ptr == 100';
    diag __LINE__;
    is $ptr, 1000, '$ptr was changed to 1000';
    diag __LINE__;
};
diag __LINE__;
subtest 'undefined scalar ref' => sub {
    diag __LINE__;
    my $ptr;
    is dbl_ptr($ptr), 'empty', 'dbl_ptr($ptr) where $ptr == undef';
    diag __LINE__;
    is $ptr, 1000, '$ptr was changed to 1000';
    diag __LINE__;
};
diag __LINE__;
subtest 'Affix::Pointer with a double' => sub {
    diag __LINE__;
    my $ptr = calloc( 1, 16 );
    {
        diag __LINE__;
        my $data = pack 'd', 100.04;
        memcpy( $ptr, $data, length $data );
        $ptr->dump(16);
        warn( ( Pointer [Double] )->unmarshal($ptr) );
    }
    is dbl_ptr($ptr), 'one hundred and change', 'dbl_ptr($ptr) where $ptr == malloc(...)';
    $ptr->dump(16);
    diag __LINE__;
    my $raw = $ptr->raw(16);
    is unpack( 'd', $raw ), 10000, '$ptr was changed to 10000';
    free $ptr;
    diag __LINE__;
};
diag __LINE__;
subtest 'ref Affix::Pointer with a double (should croak)' => sub {
    diag __LINE__;
    my $ptr = calloc( 1, 16 );
    {
        diag __LINE__;
        my $data = pack 'd', 9;
        memcpy( $ptr, $data, length $data );
    }
    diag __LINE__;
    is dbl_ptr($ptr), 'nine', 'dbl_ptr($ptr) where $ptr == malloc(...)';
    is unpack( 'd', $ptr->raw(16) ),
        ( $Config{usequadmath} ? 9876.54299999999966530594974756241 :
            $Config{uselongdouble} ? 9876.54299999999967 :
            9876.543 ), '$ptr is still 9';
    diag __LINE__;
    Affix::DumpHex( $ptr, 16 );
    diag __LINE__;
    free $ptr;
};
diag __LINE__;
{
    my $ptr = 99;
    diag __LINE__;
    is pointer_test(
        $ptr,
        [ 1 .. 5 ],
        5,
        sub {
            diag __LINE__;
            pass('our coderef was called');
            is_deeply \@_, [ 4, 8 ], '... and given correct arguments';
            diag __LINE__;
            50.25;
        }
        ),
        900, 'making call to test various types of pointers';
    diag __LINE__;
    diag ref $ptr;
    is $ptr, 100.5, 'Pointer[Double] was updated!';
}
diag __LINE__;
{
    is pointer_test(
        undef,
        [ 1 .. 5 ],
        5,
        sub {
            diag __LINE__;
            pass('our coderef was called');
            is_deeply \@_, [ 4, 8 ], '... and given correct arguments';
            diag __LINE__;
            50.25;
        }
        ),
        -1, 'making call with an undef pointer passes a NULL';
}
diag __LINE__;
{
    my $data = pack 'd', 590343.12351;    # Test pumping raw, packed data into memory
    diag __LINE__;
    my $ptr = malloc length($data);
    diag __LINE__;
    memmove $ptr, $data, length $data;
    diag __LINE__;
    diag 'allocated ' . length($data) . ' bytes';
    diag __LINE__;
    is pointer_test(
        $ptr,
        [ 1 .. 5 ],
        5,
        sub {
            diag __LINE__;
            pass('our coderef was called');
            is_deeply \@_, [ 4, 8 ], '... and given correct arguments';
            50.25;
        }
        ),
        ( $Config{usequadmath} ? 18.3382499999999986073362379102036 :
            $Config{uselongdouble} ? 18.3382499999999986 :
            18.33825 ), 'making call with Affix::Pointer object with packed data';
    is unpack( 'd', $ptr ),
        ( $Config{usequadmath} ? 3.49299999999999988276044859958347 :
            $Config{uselongdouble} ? 3.49299999999999988 :
            3.493 ), 'Affix::Pointer updated';
    diag __LINE__;
    free $ptr;
    diag __LINE__;
}
diag __LINE__;

#~ subtest struct => sub {
#~ diag __LINE__;
#~ typedef massive => Struct [
#~ B => Bool,
#~ c => Char,
#~ C => UChar,
#~ s => Short,
#~ S => UShort,
#~ i => Int,
#~ I => UInt,
#~ j => Long,
#~ J => ULong,
#~ l => LongLong,
#~ L => ULongLong,
#~ f => Float,
#~ d => Double,
#~ p => Pointer [Int],
#~ Z => Str,
#~ A => Struct [ i => Int ],
#~ u => Union [ i => Int, structure => Struct [ ptr => Pointer [Void], l => Long ] ]
#~ ];
#~ diag __LINE__;
#~ diag 'sizeof in perl: ' . sizeof( massive() );
#~ sub massive_ptr : Native('t/src/50_affix_pointers') : Signature([] => Pointer[massive()]);
#~ sub sptr : Native('t/src/50_affix_pointers') : Signature([Pointer[massive()]] => Bool);
#~ ok sptr( { Z => 'Works!' } );
#~ my $ptr = massive_ptr();
#~ my $sv  = unmarshal( $ptr, Pointer [ massive() ] );
#~ is $sv->{A}{i}, 50,                   'parsed pointer to sv and got .A.i [nested structs]';
#~ is $sv->{Z},    'Just a little test', 'parsed pointer to sv and got .Z';
#~ };
diag __LINE__;
#
done_testing;
