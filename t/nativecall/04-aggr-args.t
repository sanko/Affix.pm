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

=todo

my $field=Dyn::Call::Field->new(
    {size => 3, type => 'd'}
);

ddx ($field);
warn $field->size;
warn $field->size(4);
warn $field->size;
warn $field->type;

=cut

compile_test_lib('04-aggr-args');

=todo

my $iii = Dynamo::Types::Int->new();
warn $iii;
ddx $iii;
warn $iii->sizeof;
warn $iii->alignment;
ddx $iii->signature;
sub TakeADouble : Native('t/04-aggr-args') : Signature('(d)i')    {...}
sub TakeADoubleNaN : Native('t/04-aggr-args') : Signature('(d)i') {...}
sub TakeAFloat : Native('t/04-aggr-args') : Signature('(f)i')     {...}
sub TakeAFloatNaN : Native('t/04-aggr-args') : Signature('(f)i')  {...}

sub give4{
    return 4 if (-6.9 - $_[0] < 0.001) ;
    return 0;}
my $four;
for (1..500000){
$four = TakeADouble(-6.9e0);
$four = give4(-6.9e0);
}

is TakeADouble(-6.9e0), 4, 'passed a double';

#is TakeADoubleNaN('NaN'), 4, 'passed a NaN (double)';
#is TakeAFloat(4.2e0),     5, 'passed a float';
#is TakeAFloatNaN('NaN'),  5, 'passed a NaN (float)';







my $struct = Dynamo::Struct [
    i => Dynamo::Int,

    #j => Float,
    #k => I16
];

ddx $struct;
exit;
#
#package Int {}




my $int = Dynamo::Types::Int->new();
ddx $int;
#$int->[11] = 8;
warn $int->check(1);
warn $int->check(1.5);
warn $int->check('Test');
warn '----------------';
my $float = Dynamo::Types::Float->new();
warn $float->check(1);
warn $float->check(1.5);
warn $float->check('Test');



#ddx Dyn::Int[];
#
warn $int->sizeof;

die;
=cut

#sub TakeIntStruct : Native('t/04-aggr-args') : Signature(Struct[Int] => Double)         {...}

# Int related
sub TakeIntStruct : Native('t/04-aggr-args') : Signature('({i})i')     {...}
sub TakeIntIntStruct : Native('t/04-aggr-args') : Signature('({ii})i') {...}
#
is TakeIntStruct( [42] ),        1,  'passed struct with a single int';
is TakeIntIntStruct( [ 5, 9 ] ), 14, 'passed struct with a two ints';
done_testing;
__END__
sub TakeTwoShortsStruct : Native('t/04-aggr-args') : Signature('({ss})i')    {...}
sub IntShortChar : Native('t/04-aggr-args') : Signature('({isc})i') {...}
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

#my $ag = dcNewAggr( 1, 1 );
#warn $ag;
#dcAggrField( $ag, 'i', 0, 1 );
#dcCloseAggr($ag);
#
#die chr 43;
is TakeIntStruct([42]),                      1, 'passed struct with a single int';
#is TakeTwoShortsStruct( [10, 20] ),          3, 'passed struct with two shorts';




#is IntShortChar( [101, 102, 103] ), 3, 'passed an int, short and char struct';
done_testing;

exit 0;
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
