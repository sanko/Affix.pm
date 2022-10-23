use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use Affix::Enum;    # TODO: Include this in Affix;
use t::lib::nativecall;
#
compile_test_lib('55_affix_enum');
#
{
    my $ab = Affix::Enum [ 'alpha', 'beta' ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 0 ], [ 'beta', 1 ] ], qq![ 'alpha', 'beta' ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Int', 'default enum type is Int';
}
{
    my $ab = Affix::Enum::Char [ 'alpha', [ 'beta' => 5 ] ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 0 ], [ 'beta', 5 ] ],
        qq![ 'alpha', ['beta'=> 5] ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Char', 'Char enum type is Int';
}
{
    my $ab = Affix::Enum [ 'alpha', [ 'beta' => 5 ], 'gamma' ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 0 ], [ 'beta', 5 ], [ 'gamma', 6 ] ],
        qq![ 'alpha', [ 'beta' => 5 ], 'gamma' ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Int', 'default enum type is Int';
}
{
    my $ab = Affix::Enum [ 'alpha', [ 'beta' => 5 ], [ 'gamma' => 'alpha' ] ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 0 ], [ 'beta', 5 ], [ 'gamma', 0 ] ],
        qq![ 'alpha', [ 'beta' => 5 ], [ 'gamma' => 'alpha' ] ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Int', 'default enum type is Int';
}
{
    my $ab = Affix::Enum [ 'alpha', [ 'beta' => 5 ], [ 'gamma' => 'alpha-beta' ] ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 0 ], [ 'beta', 5 ], [ 'gamma', -5 ] ],
        qq![ 'alpha', [ 'beta' => 5 ], [ 'gamma' => 'alpha-beta' ] ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Int', 'default enum type is Int';
}
{
    my $ab = Affix::Enum [ 'alpha', [ 'beta' => 5 ], [ 'gamma' => 'beta*beta' ] ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 0 ], [ 'beta', 5 ], [ 'gamma', 25 ] ],
        qq![ 'alpha', [ 'beta' => 5 ], [ 'gamma' => 'beta*beta' ] ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Int', 'default enum type is Int';
}
{
    my $ab = Affix::Enum [ [ 'alpha' => 'a' ], 'beta' ];
    isa_ok $ab, 'Affix::Type::Enum';
    is_deeply $ab->{values}, [ [ 'alpha', 'a' ], [ 'beta', 'b' ] ],
        qq![ ['alpha'=> 'a'], 'beta' ] values!;
    isa_ok $ab->{type}, 'Affix::Type::Int', 'default enum type is Int';
}
{
    my $char_enum = Affix::Enum::Char [ [ one => 'a' ], 'two', [ 'three' => 'one' ] ];
    subtest 'ids' => sub {
        is $char_enum->id('a'), 'one', 'enum has an id with value "a"';
        is_deeply [ $char_enum->ids('a') ], [ 'one', 'three' ], 'enum has an id with value "a"';
        is_deeply [ $char_enum->ids('b') ], ['two'],            'enum has an id with value "b"';
        is_deeply [ $char_enum->ids() ], [ 'one', 'two', 'three' ],
            'enum has ids of one, two, and three';
        ok !$char_enum->id('c'),  'enum does not have an id with value "c"';
        ok !$char_enum->ids('c'), 'enum does not have an id with value "c"';
    };
    subtest 'values' => sub {
        is $char_enum->value('one'), 'a', 'enum has a value with id "one"';
        is_deeply [ $char_enum->values() ], [ 'a', 'b' ], 'enum has values of a, b';
        ok !$char_enum->value('omega'), 'enum does not have an value with id "omega"';
    }
}
subtest 'typedef' => sub {
    use Affix::Enum;
    typedef TV => Affix::Enum [
        [ FOX   => 11 ],
        [ CNN   => 25 ],
        [ ESPN  => 15 ],
        [ HBO   => 22 ],
        [ MAX   => 30 ],
        [ NBC   => 32 ],
        [ MSN   => 45 ],
        [ MSNBC => 'MSN + NBC' ]
    ];
    sub TakeEnum : Native('t/55_affix_enum') : Signature([TV]=>Int);
    is TakeEnum( TV::FOX() ),  -11, 'FOX';
    is TakeEnum( TV::ESPN() ), -1,  'ESPN';
    my $cv = wrap 't/55_affix_enum', 'TakeEnum', [ TV() ], TV();
    isa_ok( TV(), 'Affix::Type::Enum', 'typedef TV' );
    is TV::FOX(),       'FOX',                 'typedef Enum results in dualvars [FOX string]';
    is TV::FOX() + 0,   11,                    'typedef Enum results in dualvars [FOX numeric]';
    is TV::MSNBC(),     'MSNBC',               'typedef Enum results in dualvars [MSNBC string]';
    is TV::MSNBC() + 0, TV::NBC() + TV::MSN(), 'typedef Enum results in dualvars [MSNBC numeric]';
    is TakeEnum(11),    -11,                   'FOX used in attached function';
    is TakeEnum(15),    -1,                    'ESPN used in attached function';
    is $cv->( TV::FOX() ),   -TV::FOX(),       'TV::FOX() used in wrapped function';
    is $cv->( TV::CNN() ),   -1,               'TV::CNN() used in wrapped function';
    is $cv->( TV::MSNBC() ), -1,               'TV::MSNBC() used in wrapped function';
};
done_testing;
