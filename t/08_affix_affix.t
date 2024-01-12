use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use Test::Fatal    qw[exception];
use Test::Warnings qw[warning :no_end_test];
use t::lib::helper;
$|++;
#
my $lib = compile_test_lib('08_affix_affix');
#
subtest 'Bad args' => sub {
    like exception { affix $lib, 'Nothing', Int, Void }, qr[Expected a list of argument types ], q[affix ..., ..., Int, ... throws exception];
    like exception { affix $lib, 'Nothing', undef, Void }, qr[Expected a list of argument types ], q[affix ..., ..., undef, ... throws exception];
    like exception { affix $lib, 'Nothing', 100,   Void }, qr[Expected a list of argument types ], q[affix ..., ..., 100, ... throws exception];
    like exception { affix $lib, 'Nothing', 'Int', Void }, qr[Expected a list of argument types ], q[affix ..., ..., 'Int', ... throws exception];
};
subtest 'Bad name' => sub {
    like exception { wrap $lib, [ 'Nothing', 'Something' ], [Int], Void }, qr[isn't expecting a name],
        q[wrap ..., ['Nothing', 'Something'] ..., ... throws exception];
    like exception { wrap $lib, undef,     [Int], Void }, qr[symbol name],                         q[wrap ..., undef ..., ... throws exception];
    like exception { wrap $lib, 'Missing', [Int], Void }, qr[Failed to find symbol named Missing], q[wrap ..., 'Missing' ..., ... throws exception];
};
subtest 'Good stuff' => sub {
    ok affix( $lib, [ 'Nothing', 'Nope' ], [], Int ), qq[affix '$lib', ['Nothing', 'Nope'], [], Int];
    is Nope(), 99, 'Nope()';
    ok affix( $lib, 'Nothing_I', [Int], Int ), qq[affix '$lib', 'Nothing_I', [Int], Int];
    is Nothing_I(50), 150, 'Nothing_I(50)';
};
done_testing;
__END__
die;
subtest 'bad abi' => sub {
    exception { wrap( [ $lib, 'GCC' ], 'Nothing', [Int], Void ) },
    qr[Unknown ABI],
        "wrap( [ $lib, 'GCC' ], 'Nothing', [Int], Void ) throws exception on malformed lib list";
    die;
    exception { wrap( [$lib], 'Nothing', [Int], Void ) },
    qr[Expected a library and ABI],
        "wrap( [ $lib ], 'Nothing', [Int], Void ) throws exception on missing ABI in lib list";

    #~ exception { wrap( $lib, 'Nothing', Int ) }
    #~ qr[Expected arguments should be passed as a list],
    #~ "wrap($lib, 'Nothing', Int) throws exception on malformed arg list";
    #~ exception { wrap( $lib, 'Nothing', [10] ) }
    #~ qr[Unexpected or unknown type in argument list position 1],
    #~ "wrap($lib, 'Nothing', [10]) throws exception on unknown arg type";
};
subtest 'bad arg list' => sub {
    exception { wrap( $lib, 'Nothing', 10 ) },
    qr[Usage], "wrap($lib, 'Nothing', 10) throws exception on malformed arg list";
    exception { wrap( $lib, 'Nothing', Int ) }
    qr[Usage], "wrap($lib, 'Nothing', Int) throws exception on malformed arg list";
    exception { wrap( $lib, 'Nothing', [10] ) }
    qr[Usage], "wrap($lib, 'Nothing', [10]) throws exception on unknown arg type";
    exception { wrap( $lib, 'Nothing', 10, Affix::Void() ) }
    qr[Expected arguments list as a list],
        "wrap($lib, 'Nothing', 10, Void) throws exception on malformed arg list";
    exception { wrap( $lib, 'Nothing', Int, Affix::Void() ) }
    qr[Expected arguments list as a list],
        "wrap($lib, 'Nothing', Int, Void) throws exception on malformed arg list";
    diag __LINE__;
    exception { wrap( $lib, 'Nothing', [10], Affix::Void() ) }
    qr[Unknown or unsupported argument type in slot 1],
        "wrap($lib, 'Nothing', [10], Void) throws exception on unknown arg type";
    diag __LINE__;
    exception { wrap( $lib, 'Nothing', [ Affix::Int(), 10 ], Affix::Void() ) }
    qr[Unknown or unsupported argument type in slot 2],
        "wrap($lib, 'Nothing', [Int, 10], Void) throws exception on unknown arg type";
    diag __LINE__;
    exception { wrap( $lib, 'Nothing', [ Affix::CC_ELLIPSIS(), Affix::Int(), 10 ], Affix::Void() ) }
    qr[Unknown or unsupported argument type in slot 3],
        "wrap($lib, 'Nothing', [CC_ELLIPSIS, Int, 10], Void) throws exception on unknown arg type";
};
#
subtest 'do the right thing' => sub {
    {
        my $Nothing = wrap( $lib, 'Nothing', [], Affix::Void() );
        diag __LINE__;
        isa_ok $Nothing , 'Affix', "my \$Nothing = wrap( $lib, 'Nothing', [Int] )";
        diag __LINE__;
        is $Nothing->(), undef, '$Nothing->() returns undef';
        diag __LINE__;
    }
    {
        my $Nothing = wrap( [ $lib, Affix::ABI_C() ], 'Nothing', [], Affix::Void() );
        isa_ok $Nothing , 'Affix', "my \$Nothing = wrap( $lib, 'Nothing', [Int] )";
        is $Nothing->(), undef, '$Nothing->() returns undef';
    }

    package t::Test1 {
        Affix::affix( $lib, 'Nothing', [], Affix::Void() );
        Test::More::can_ok __PACKAGE__, 'Nothing';
        Test::More::is( Nothing(), undef, 'Nothing() returns undef' );
    }

    package t::Test2 {
        Affix::affix( $lib, 'Nothing_I', [ Affix::Int() ], Affix::Void() );
        Test::More::can_ok __PACKAGE__, 'Nothing_I';
        Test::More::is( Nothing_I(5), undef, 'Nothing_I() returns undef' );
    }
};
#
done_testing;
__END__
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
    my sub Argless : Native('t/src/08_affix_affix') : Signature([]=>Int) { }
    is Argless(), 2, 'called argless closure';
}

#test_native_closure();
#test_native_closure(); # again cause we may have created an optimized version to run
done_testing;
