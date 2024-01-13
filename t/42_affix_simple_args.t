use Test2::V0;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::helper;
use experimental 'signatures';
$|++;

# rakudo/t/04-nativecall/02-simple-args.t
my $lib = compile_test_lib('42_affix_simple_args');
#
subtest 'Int' => sub {
    is wrap( $lib, 'TakeInt', [Int] => Int )->(-42),                                          1, '[Int] => Int';
    is wrap( $lib, 'TakeUInt', [UInt] => Int )->(42),                                         1, '[UInt] => Int';
    is wrap( $lib, 'TakeTwoShorts', [ Short, Short ] => Long )->( 10, 20 ),                   2, '[Short, Short] => Long';
    is wrap( $lib, 'AssortedIntArgs', [ Long, Short, Char ] => Long )->( 101, 102, chr 103 ), 3, '[Long, Short, Char] => Long';
};
subtest 'Float' => sub {
    is wrap( $lib, 'TakeADouble',    [Double] => Int )->(-6.9e0), 4, '[Double] => Int';
    is wrap( $lib, 'TakeADoubleNaN', [Double] => Int )->('NaN'),  4, '[Double] => Int (NaN)';
    is wrap( $lib, 'TakeAFloat',     [Float]  => Int )->(4.2e0),  5, '[Float] => Int';
    is wrap( $lib, 'TakeAFloatNaN',  [Float]  => Int )->('NaN'),  5, '[Float] => Int (NaN)';
};
subtest 'Str' => sub {
    is wrap( $lib, 'TakeAString', [Str] => Int )->('ok 6 - passed a string'), 6, '[Str] => Int';
    #
    affix $lib, 'TakeAStringThenNull', [ Long, Str ] => Int;

    # Loop is important to test the dispatcher!
    is TakeAStringThenNull( 0, $_ ), 6, 'defined/undefined works on the same callsite' for undef, 'ok 6 - passed a string';
    #
    affix( $lib, 'SetString',   [Str] => Int );
    affix( $lib, 'CheckString', []    => Int );
    subtest 'Explicitly managing strings' => sub {
        my $str = 'ok 7 - checked previously passed string';

        # https://docs.raku.org/language/nativecall.html#sub_explicitly-manage
        SetString($str);
        is CheckString(), 7, 'checked previously passed string';
    }
};

=pod

=begin future

# Make sure wrapped subs work
sub wrapped : Native('t/src/42_affix_simple_args') : Signature([Int]=>Int);
sub wrapper ($arg) { is wrapped($arg), 8, 'wrapped sub' }
wrapper(42);
TODO: {
    #local $TODO = 'Some platforms choke on 64bit math';
    # 64-bit integer
    sub TakeInt64 : Native('t/src/42_affix_simple_args') : Signature([LongLong]=>Int);
    {
        use Math::BigInt;
        is TakeInt64( Math::BigInt->new('0xFFFFFFFFFF') ), 9, 'passed int64 0xFFFFFFFFFF';
    }
}

# Unsigned integers.
sub TakeUint8 : Native('t/src/42_affix_simple_args') : Signature([UChar]=>Int);
sub TakeUint16 : Native('t/src/42_affix_simple_args') : Signature([UShort]=>Int);
sub TakeUint32 : Native('t/src/42_affix_simple_args') : Signature([ULong]=>Int);
SKIP: {
    #skip 'Cannot test TakeUint8(0xFE) on OS X with -O3', 1 if $^O eq 'darwin';
    #
    # For some reason, on OS X with clang, the following test fails with -O3
    # specified.  One can only assume this is some weird compiler issue (tested
    # on Apple LLVM version 6.1.0 (clang-602.0.49) (based on LLVM 3.6.0svn).
    #
    is TakeUint8( chr 0xFE ), 10, 'passed uint8 0xFE';
}

# R#2124 https://github.com/rakudo/rakudo/issues/2124
#skip("Cannot test TakeUint16(0xFFFE) with clang without -O0");
is TakeUint16(0xFFFE),     11, 'passed uint16 0xFFFE';
is TakeUint32(0xFFFFFFFE), 12, 'passed uint32 0xFFFFFFFE';
sub TakeSizeT : Native('t/src/42_affix_simple_args') : Signature([Int]=>ULong);
is TakeSizeT(42), 13, 'passed size_t 42';
sub TakeSSizeT : Native('t/src/42_affix_simple_args') : Signature([Int]=>ULong);
is TakeSSizeT(-42), 14, 'passed ssize_t -42';

# https://docs.raku.org/type/Proxy - Sort of like a magical tied hash?
#my $arg := Proxy.new(
#    FETCH => -> $ {
#        42
#    },
#    STORE => -> $, $val {
#        die "STORE NYI";
#    },
#);
#is TakeInt($arg), 1, 'Proxy works';

=end future

=cut

done_testing;
