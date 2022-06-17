use strictures 2;
$|++;
use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'lib';

#use Dyn qw[:dc :dl :sugar];
use File::Find;
$|++;
#
#
use Dynamo qw[:Types];
use Data::Dump;
use Test2::V0;
use Test2::Tools::Compare qw[string];
subtest 'Dyn::Type::* subtests' => sub {
    #
    #::Void
    subtest 'Dyn::Type' => sub {
        isa_ok +Dyn::Type::bool->new(1),   ['Dyn::Type::bool'], '...(1) isa Dyn::Type::bool';
        isa_ok +Dyn::Type::bool->new(1.5), ['Dyn::Type::bool'], '...(1.5) isa Dyn::Type::bool';
        isa_ok +Dyn::Type::bool->new(-1),  ['Dyn::Type::bool'], '...(-1) isa Dyn::Type::bool';
        isa_ok +Dyn::Type::bool->new(+1),  ['Dyn::Type::bool'], '...(+1) isa Dyn::Type::bool';
        isa_ok +Dyn::Type::bool->new(0),   ['Dyn::Type::bool'], '...(+1) isa Dyn::Type::bool';
        is +Dyn::Type::bool->new(+1),        T(), '...(+1) is true';
        is +Dyn::Type::bool->new(-1),        T(), '...(-1) is true';
        is +Dyn::Type::bool->new(0),         F(), '...(0) is false';
        is +Dyn::Type::bool->new( !0 ),      T(), '...(!0) is true';
        is +Dyn::Type::bool->new( !1 ),      F(), '...(!1) is false';
        is +Dyn::Type::bool->new(100),       1,   '...(100) is 1';
        is +Dyn::Type::bool->new(-100),      1,   '...(-100) is 1';
        is +Dyn::Type::bool->new(0),         !1,  '...(0) is !1';
        is +Dyn::Type::bool->new( !0 ),      !!1, '...(!0) is !!1';
        is +Dyn::Type::bool->new( !1 ),      !1,  '...(!1) is !1';
        is +Dyn::Type::bool->new('a'),       D(), '...("a") is undefined';
        is +Dyn::Type::bool->new(1)->sizeof, 1,   'sizeof() is 1';
    };
    subtest 'Dyn::Type::i8' => sub {
        isa_ok +Dyn::Type::i8->new(100), ['Dyn::Type::i8'], '...(100) isa Dyn::Type::i8';
        is +Dyn::Type::i8->new('a'),       U(),  '...("a") is undefined';
        is +Dyn::Type::i8->new(-1),        -1,   '...(-1) is -1';
        is +Dyn::Type::i8->new(-128),      -128, '...(-128) is -128';
        is +Dyn::Type::i8->new(-129),      U(),  '...(-129) is undefined';
        is +Dyn::Type::i8->new(127),       127,  '...(127) is 127';
        is +Dyn::Type::i8->new(128),       U(),  '...(128) is undefined';
        is +Dyn::Type::i8->new(1)->sizeof, 1,    'sizeof() is 1';
    };
    subtest 'Dyn::Type::u8' => sub {
        isa_ok +Dyn::Type::u8->new(100), ['Dyn::Type::u8'], '...(100) isa Dyn::Type::u8';
        is +Dyn::Type::u8->new('a'),       U(), '...("a") is undefined';
        is +Dyn::Type::u8->new(0),         D(), '...(0) is defined...';
        is +Dyn::Type::u8->new(0),         0,   '   ...(0) is 0';
        is +Dyn::Type::u8->new(-1),        U(), '...(-1) is undefined';
        is +Dyn::Type::u8->new(255),       255, '...(255) is 255';
        is +Dyn::Type::u8->new(256),       U(), '...(256) is undefined';
        is +Dyn::Type::u8->new(1)->sizeof, 1,   'sizeof() is 1';
    };

    #::i16
    #::u16
    #::i16
    #::u16
    #::i32
    #::u32
    #::i64
    #::u64
    subtest 'Dyn::Type::float' => sub {
        isa_ok +Dyn::Type::float->new(1),   ['Dyn::Type::float'], '...(1) isa Dyn::Type::float';
        isa_ok +Dyn::Type::float->new(1.5), ['Dyn::Type::float'], '...(1.5) isa Dyn::Type::float';
        isa_ok +Dyn::Type::float->new(-1),  ['Dyn::Type::float'], '...(-1) isa Dyn::Type::float';
        isa_ok +Dyn::Type::float->new(+1),  ['Dyn::Type::float'], '...(+1) isa Dyn::Type::float';
        is +Dyn::Type::float->new('a'),       U(), '...("a") is undefined';
        is +Dyn::Type::float->new(8)->sizeof, 4,   'sizeof() is 4';
    };
    subtest 'Dyn::Type::str' => sub {
        isa_ok +Dyn::Type::str->new('Hello, world!'), ['Dyn::Type::str'],
            '...("Hello, world") isa Dyn::Type::str';
        is +Dyn::Type::str->new('two')->sizeof,  3, 'sizeof() is 3';
        is +Dyn::Type::str->new('four')->sizeof, 4, 'sizeof() is 4';
    };

    #::Double
};
subtest 'Dyn::Enum' => sub {
    my $enum = Dyn::Enum->new(
        name   => 'My::Test::Enum',
        type   => 'Dyn::Type::float',
        values => { red => 0, green => 20, blue => 21, orange => 20.5 }
    );
    isa_ok $enum, [qw[Dyn::Enum]], '$enum isa Dyn::Enum';
    ok $enum->check(0),    '0 checks out';
    ok !$enum->check(10),  '10 does not check out';
    ok $enum->check(20),   '20 checks out';
    ok $enum->check(20.5), '20.5 checks out';
    ok $enum->check(21),   '21 checks out';
    is $enum->key(20.5),         'orange',                    '20.5 => orange [key]';
    is $enum->value('green'),    20,                          'green => 20 [value]';
    is My::Test::Enum::green(),  20,                          'green => 20';
    is My::Test::Enum::red(),    0,                           'red => 0';
    is My::Test::Enum::blue(),   21,                          'blue => blue';
    is My::Test::Enum::orange(), 20.5,                        'orange => 20.5';
    is [ sort $enum->keys ],     [qw[blue green orange red]], 'correct keys (unsorted)';
    is [ sort $enum->values ],   [ 0, 20, 20.5, 21 ],         'correct values (unsorted)';
};
#
ddx Union(    # anon union returns object
    fields => { i => I32, f => Float, c => U16 }
);
subtest 'Dyn::Union' => sub {
    isa_ok Union(    # anon union returns object
        fields => { i => I32, f => Float, c => U16 }
        ),
        'Dyn::Union::Anon_1';
    is Union(        # package name is given so bool is returned
        name   => 'My::Union',
        fields => { i => I32, f => Float, c => U16 }
        ),
        1, 'define My::Union';
    my $union = My::Union->new();
    isa_ok $union, [qw[Dyn::Union]], 'newly constructed $union isa Dyn::Union';
    is $union->sizeof, 4, '$union->sizeof is 4';
    #
    is $union->value, U(), '$union->value begins undefined';
    is $union->type,  U(), '$union->type begins undefined';
    is $union->i,     U(), '$union->i begins undefined';
    is $union->f,     U(), '$union->f begins undefined';
    is $union->c,     U(), '$union->c begins undefined';
    is $union,        F(), '$union begins false';
    #
    is $union->i(5),  5,   '$union->i(5) returns 5';
    is $union->value, 5,   '$union->value now returns 5';
    is $union->type,  'i', '$union->type is now "i"';
    is $union->i,     5,   '$union->i returns 5';
    is $union,        5,   '$union returns 5';
    #
    is $union->f(5.3), 5.3, '$union->f(5.3) returns 5.3';
    is $union->value,  5.3, '$union->value now returns 5.3';
    is $union->type,   'f', '$union->type is now "f"';
    is $union->f,      5.3, '$union->f returns 5.3';
    is $union,         5.3, '$union returns 5.3';
    #
    is $union->c(127), 127, '$union->c(127) returns 127';
    is $union->value,  127, '$union->value now returns 127';
    is $union->type,   'c', '$union->type is now "c"';
    is $union->c,      127, '$union->c returns 127';
    is $union,         127, '$union returns 127';
    #
    is $union->i, U(), '$union->i is now undefined';
    is $union->f, U(), '$union->f is now undefined';
};
subtest 'Type API' => sub {
    {

        package Dyn;
        use Data::Dump;

        #ddx Pointer [ U8 | I16 ];
    };
    pass 'remove';

    #  isa_ok +Dyn::Type::str->new('Hello, world!'), ['Dyn::Type::str'],
    #      '...("Hello, world") isa Dyn::Type::str';
    #  is +Dyn::Type::str->new('two')->sizeof,  3, 'sizeof() is 3';
    #  is +Dyn::Type::str->new('four')->sizeof, 4, 'sizeof() is 4';
};
done_testing;
__END__
union Test::Union {
    field $a: isa(i8);

    }

union 'Some::Union'


package Test::Union{
use Dyn::Union
  field :isa();
  field $test:isa()


}

;
#class Hi{
#    field $.test;


#    }

__END__

union Person::Level {
	i: isa(i32),    # 4 bytes
	f: isa(u32[2]), # 4 bytes
	c: isa(u8)      # 1 byte
}; # entire union occupies 4 bytes

my $guy = Person::Level->new(f => 1.0);

union MyUnion {
    has int32 $.flags32;
    has int64 $.flags64;
}




__END__

enum Person::Level :isa(float) { # default :isa(i16)
	red,
	green = 20,
	blue,
	orange = green + .5
};

$guy->shirt(Person::Level::green()); # or import green directly

__END__
struct Person : version(v3.14.0) {
    field $name   : isa(string)         :param; # required
    field $cash   : isa(float);
    field $credit : isa(float);
    field $level  : isa(Persion::Level);
};
my $john = Person->new(
  level => $guy,
  cash  => 100.00,
  credit => 5000.00,
  name  => 'John'
);

__END__
class Customer :isa(Person 3.14) :version(v2.1.0) {
    field $size :isa(i16) :reader;
    field @numbers : isa(float);
    field $next : isa(Customer);
    method serve( i16 $first, Person::Level $level);
    method reject( String $reason);
    method insert() : common;
}

# import init from a lib already loaded/found in libpath
# function returns void
sub init ( );

# import sum from a lib already loaded/found in libpath
# function expects two unsigned 32 bit integers
# function returns a 32 bit float
sub sum (u32 $lhs, u32: $rhs) :Returns(u32);

# load pow from somelib.so with a version of 1.0 (libsomelib.1.0.so, etc.) as power
# function expects an unsigned 32 integer and a signed 32 bit integer
# function returns a 32 bit float
sub power (u32 $num, i32 $exp) :Returns(float) :Library('somelib.so', 1.0) :Symbol('pow');

