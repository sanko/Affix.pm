use strict;
use Test::More 0.98;
BEGIN { chdir '../../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Dyn qw[:all];
use File::Spec;
use t::nativecall;
use experimental 'signatures';
$|++;
#
compile_test_lib('04-aggr-args');

# Int related
sub TakeInt : Native('t/04-aggr-args') : Signature('({i})i')         {...}
sub TakeTwoShorts : Native('t/04-aggr-args') : Signature('(ss)i')    {...}
sub AssortedIntArgs : Native('t/04-aggr-args') : Signature('(jsc)j') {...}
#

=fdsa
DCaggr *
dcNewAggr( DCsize maxFieldCount, DCsize size )

void
dcFreeAggr( DCaggr * ag )
CODE:
    dcFreeAggr(ag);
    SV* sv = (SV*) &PL_sv_undef;
    sv_setsv(ST(0), sv);

void
dcAggrField( DCaggr * ag, DCsigchar type, DCint offset, DCsize arrayLength, ... )

void
dcCloseAggr( DCaggr * ag )
=cut

my $ag = dcNewAggr( 1, 1 );
warn $ag;
dcAggrField( $ag, 'i', 0, 1 );
dcCloseAggr($ag);
is TakeInt(42),                      1, 'passed int 42';
is TakeTwoShorts( 10, 20 ),          2, 'passed two shorts';
is AssortedIntArgs( 101, 102, 103 ), 3, 'passed an int32, int16 and int8';

# Float related
sub TakeADouble : Native('t/04-aggr-args') : Signature('(d)i')    {...}
sub TakeADoubleNaN : Native('t/04-aggr-args') : Signature('(d)i') {...}
sub TakeAFloat : Native('t/04-aggr-args') : Signature('(f)i')     {...}
sub TakeAFloatNaN : Native('t/04-aggr-args') : Signature('(f)i')  {...}
is TakeADouble(-6.9e0),   4, 'passed a double';
is TakeADoubleNaN('NaN'), 4, 'passed a NaN (double)';
is TakeAFloat(4.2e0),     5, 'passed a float';
is TakeAFloatNaN('NaN'),  5, 'passed a NaN (float)';

# String related
sub TakeAString : Native('t/04-aggr-args') : Signature('(Z)i') {...}
is TakeAString('ok 6 - passed a string'), 6, 'passed a string';

# Explicitly managing strings
sub SetString : Native('t/04-aggr-args') : Signature('(Z)i')  {...}
sub CheckString : Native('t/04-aggr-args') : Signature('()i') {...}
my $str = 'ok 7 - checked previously passed string';

#explicitly-manage($str); # https://docs.raku.org/routine/explicitly-manage
SetString($str);
is CheckString(), 7, 'checked previously passed string';

# Make sure wrapped subs work
sub wrapped : Native('t/04-aggr-args') : Signature('(i)i') {...}
sub wrapper ($arg)                                         { is wrapped($arg), 8, 'wrapped sub' }
wrapper(42);

# 64-bit integer
sub TakeInt64 : Native('t/04-aggr-args') : Signature('(l)i') {...}
{
    no warnings 'portable';
    is TakeInt64(0xFFFFFFFFFF), 9, 'passed int64 0xFFFFFFFFFF';
}

# Unsigned integers.
sub TakeUint8 : Native('t/04-aggr-args') : Signature('(C)i')  {...}
sub TakeUint16 : Native('t/04-aggr-args') : Signature('(S)i') {...}
sub TakeUint32 : Native('t/04-aggr-args') : Signature('(J)i') {...}
SKIP: {
    #skip 'Cannot test TakeUint8(0xFE) on OS X with -O3', 1 if $^O eq 'darwin';
    #
    # For some reason, on OS X with clang, the following test fails with -O3
    # specified.  One can only assume this is some weird compiler issue (tested
    # on Apple LLVM version 6.1.0 (clang-602.0.49) (based on LLVM 3.6.0svn).
    #
    is TakeUint8(0xFE), 10, 'passed uint8 0xFE';
}

# R#2124 https://github.com/rakudo/rakudo/issues/2124
#skip("Cannot test TakeUint16(0xFFFE) with clang without -O0");
is TakeUint16(0xFFFE),     11, 'passed uint16 0xFFFE';
is TakeUint32(0xFFFFFFFE), 12, 'passed uint32 0xFFFFFFFE';
sub TakeSizeT : Native('t/04-aggr-args') : Signature('(i)j');
is TakeSizeT(42), 13, 'passed size_t 42';
sub TakeSSizeT : Native('t/04-aggr-args') : Signature('(I)j');
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
done_testing;
