use strictures 2;
$|++;
use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib';
use Dyn qw[:dc :dl :sugar];
use File::Find;
$|++;
#
package Dynamo {
    use strictures 2;
    use Data::Dump;
    use Keyword::Simple;
    use lib '../lib';
    use Dyn::Call;
    use Dyn::Load qw[dlLoadLibrary dlSymsInit dlSymsCount dlSymsName dlFindSymbol];
    use File::Find;
    #
    my @files;
    find(
        {   preprocess => sub {
                my $depth = $File::Find::dir =~ tr[/][];
                $depth > 2 ? () : @_;
            },
            wanted => sub {
                push @files, $File::Find::name
                    if ( -f $File::Find::name and /libm(-[\d\.]+)?\.so(?:\.\d)?/ );
            }
        },
        '/usr/lib'
    );
    warn join ', ', @files;
    my ( undef, $path ) = @files;    # pick one
    my $lib  = dlLoadLibrary($path);
    my $init = dlSymsInit($path);
    #
    CORE::say "Symbols in libm ($path): " . dlSymsCount($init);
    CORE::say 'All symbol names in libm:';

    #CORE::say sprintf '  %4d %s', $_, dlSymsName( $init, $_ ) for 0 .. dlSymsCount($init) - 1;
    CORE::say 'libm has sqrtf()? ' .       ( dlFindSymbol( $lib, 'sqrtf' )       ? 'yes' : 'no' );
    CORE::say 'libm has pow()? ' .         ( dlFindSymbol( $lib, 'pow' )         ? 'yes' : 'no' );
    CORE::say 'libm has not_in_libm()? ' . ( dlFindSymbol( $lib, 'not_in_libm' ) ? 'yes' : 'no' );
    #
    #CORE::say 'sqrtf(36.f) = ' . Dyn::load( $path, 'sqrtf', '(f)f' )->call(36.0);
    #CORE::say 'pow(2.0, 10.0) = ' . Dyn::load( $path, 'pow', 'dd)d' )->call( 2.0, 10.0 );
    my $ptr = dlFindSymbol( $lib, 'pow' );
    my $cvm = Dyn::Call::dcNewCallVM(1024);
    Dyn::Call::dcMode( $cvm, 0 );
    Dyn::Call::dcReset($cvm);
    Dyn::Call::dcArgInt( $cvm, 5 );
    Dyn::Call::dcArgInt( $cvm, 6 );
    Dyn::Call::dcCallInt( $cvm, $ptr );    #  '5 + 6 == 11';
    use Dyn qw[:sugar];
    no warnings('reserved');
    sub pow : native( $path ) : Signature(Double, Double => Double);
    warn pow( 3, 4 );

    #dlFreeSymbol( $lib );
    #
    sub bool () {'Dynamo::Type::bool'}
    sub i8()    {'Dynamo::Type::i8'}
    sub u8()    {'Dynamo::Type::u8'}
    sub i16 ()  {'Dynamo::Type::i16'}
    sub u16 ()  {'Dynamo::Type::u16'}
    sub i32 ()  {'Dynamo::Type::i32'}
    sub u32 ()  {'Dynamo::Type::u32'}
    sub i64 ()  {'Dynamo::Type::i64'}
    sub u64 ()  {'Dynamo::Type::u64'}
    sub f32 ()  {'Dynamo::Type::f32'}
    sub f64 ()  {'Dynamo::Type::f64'}
    sub str()   {'Dynamo::Type::str'}

    sub union(%) {
        my %opts    = @_;
        my $fields  = $opts{fields};
        my $package = $opts{name};
        my $anon    = defined $package ? 0 : 1;
        my %union   = (
            current => [ undef, undef ],
            types   => [ map { $fields->{$_} } sort keys %$fields ],
            fields  => [ sort keys %$fields ]
        );
        if ($anon) {
            CORE::state $u = 0;    # TODO: Make descriptive with keys of fields
            $package = 'Dynamo::Union::Anon_' . ++$u;
        }
        for my $i ( 0 .. $#{ $union{fields} } ) {
            my $key = $union{fields}[$i];
            no strict 'refs';
            *{ $package . '::' . $key } = sub () {    # autoviv mutators to match fields
                my ( $s, @etc ) = @_;
                if (@etc) {
                    my $value = ( $union{types}[$i] )->new(@etc);
                    $value // return;                 # TODO: carp
                    $s->[0] = $key;
                    $s->[1] = $value if $value;
                }
                $$s[0] && $$s[0] eq $key ? $$s[1] : ();
            };
        }

        # TODO: prevent defining a union twice
        {
            no strict 'refs';
            *{ $package . '::ISA' } = ['Dynamo::Union'];
            for my $key ( keys %union ) {
                *{ $package . '::' . $key } = $union{$key};
            }
        }
        return $anon ? $package->new() : !0;
    }

    #sub Pointer (){}


        package Dynamo::Type::void {    # nada

        sub new {
            my ( $package, $f, @etc ) = @_;

@etc && return ();
            return bless \0, $package;
        }

                    sub sizeof() {0}

        }

    package Dynamo::Type::bool {
        use overload '""' => sub {
            my $s = shift;
            !!$$s[0];
            },
            '0+' => sub {
            my $s = shift;
            !!$$s[0];
            },
            fallback => 1;

        sub _check {
            my $value = shift;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {1}
    }
    package Dynamo::Type::byte {    # char
                use parent -norequire, 'Dynamo::Type::u8';

    }
    package Dynamo::Type::i8 {    # char
        use overload
            '0+' => sub {
            my $s = shift;
            $$s[0];
            },
            fallback => 1;

        sub _check {
            my $value = shift;
            return if $value !~ m[^(?:[-\+])?\d+$];
            return if $value < -128 || $value > 127;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {1}
    }

    package Dynamo::Type::u8 {    # unsigned char
        use parent -norequire, 'Dynamo::Type::i8';

        sub _check {
            my $value = shift;
            return if $value !~ m[^\d+$];
            return if $value > 255;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
    }

    package Dynamo::Type::i16 {    # short
        use overload
            '""' => sub {
            my $s = shift;
            $$s[0];
            },
            '0+' => sub {
            my $s = shift;
            $$s[0];
            },
            fallback => 1;
        use parent -norequire, 'Dynamo::Type::i8';

        sub _check {
            my $value = shift;
            return if $value !~ m[^(?:[-\+])?\d+$];
            return if $value < -255 || $value > 256;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {2}
    }

    package Dynamo::Type::u16 {    # unsigned short
        use parent -norequire, 'Dynamo::Type::i16';

        sub _check {
            my $value = shift;
            return if $value !~ m[^\d+$];
            return if $value > 256;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
    }

    package Dynamo::Type::i32 {    # int
        use parent -norequire, 'Dynamo::Type::i16';

        sub _check {
            my $value = shift;
            return if $value !~ m[^(?:[-\+])?\d+$];
            return if $value < -2147483648 || $value > 2147483647;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {4}
    }

    package Dynamo::Type::u32 {    # unsigned int
        use parent -norequire, 'Dynamo::Type::i32';

        sub _check {
            my $value = shift;
            return if $value !~ m[^\d+$];
            return if $value > 4294967295;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            @etc && return ();
            return bless [$f], $package;
        }
    }

    package Dynamo::Type::i64 {    # long long
        use parent -norequire, 'Dynamo::Type::i32';

        sub _check {
            my $value = shift;
            return if $value !~ m[^(?:[-\+])?\d+$];
            return if $value < -9223372036854775808 || $value > 9223372036854775807;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {8}
    }

    package Dynamo::Type::u64 {    # unsigned long long
        use parent -norequire, 'Dynamo::Type::i64';

        sub _check {
            my $value = shift;
            return if $value !~ m[^\d+$];
            return if $value > 18446744073709551615;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            @etc && return ();
            return bless [$f], $package;
        }
    }

    package Dynamo::Type::f32 {    # float
        use parent -norequire, 'Dynamo::Type::i32';

        sub _check {
            my $value = shift;     # honestly, this should use Math::BitFloat
            return if $value !~ m[^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$];
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {4}
    }

    package Dynamo::Type::f64 {    # double
        use parent -norequire, 'Dynamo::Type::f32';

        sub _check {
            my $value = shift;     # honestly, this should use Math::BitFloat
            return if $value !~ m[^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$];
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof() {8}
    }

    package Dynamo::Type::str {
        use overload '""' => sub {
            my $s = shift;
            $$s[0];
            },
            fallback => 1;

        sub _check {
            my $value = shift;
            1;
        }

        sub new {
            my ( $package, $f, @etc ) = @_;
            return if !_check($f);
            @etc && return ();
            return bless [$f], $package;
        }
        sub sizeof { length shift->[0] }
    }

    package Dynamo::Union {
        use overload
            '""'     => 'value',
            bool     => 'value',
            fallback => 1;

        sub max (@) {    # From List::Util::PP
            return undef unless @_;
            my $max = shift;
            $_ > $max and $max = $_ foreach @_;
            return $max;
        }

        sub new {
            my $package = shift;
            Carp::confess 'Not a subclass' if $package eq __PACKAGE__;
            bless [ undef, undef ], $package;
        }

        sub value {
            my ($s) = @_;
            $$s[1];
        }

        sub type {
            my ($s) = @_;
            $$s[0];
        }

        sub sizeof {
            my ($s) = @_;
            no strict 'refs';
            max map { $_->sizeof } @{ *{ ref($s) . '::types' } };
        }
    }

    package Dynamo::Enum {

        sub new {
            my $package = shift;
            my %etc     = @_;
            if ( defined $etc{name} ) {
                for my $key ( keys %{ $etc{values} } ) {
                    no strict 'refs';
                    *{ $etc{name} . '::' . $key } = sub () { $etc{values}{$key} }
                }
            }
            return bless \%etc, $package;
        }

        sub check {
            my ( $s, $value ) = @_;
            !!grep { $_ == $value } $s->values;
        }

        sub value {
            my ( $s, $key ) = @_;
            $s->{values}{$key} // ();
        }

        sub key {
            my ( $s, $value ) = @_;
            my @ret = grep { $s->{values}{$_} == $value } keys %{ $s->{values} };
            @ret ? shift @ret : ();
        }

        sub values {
            my ( $s, $value ) = @_;
            values %{ $s->{values} };
        }

        sub keys {
            my ( $s, $value ) = @_;
            keys %{ $s->{values} };
        }
    }
    #
    sub _csv {
        my $text = shift // '';
        my @new  = ();
        push( @new, $+ ) while $text =~ m[
        \s*(?:"([^\"\\]*(?:\\.[^\"\\]*)*)",?
           | ([^,]+),?
           | ,)
       ]gx;
        push( @new, undef ) if substr( $text, -1, 1 ) eq ',';
        @new;
    }

    sub import {
        warn 'import';

        # create keyword 'provided', expand it to 'if' at parse time
        Keyword::Simple::define 'provided', sub {
            my ($ref) = @_;
            substr( $$ref, 0, 0 ) = 'if';    # inject 'if' at beginning of parse buffer
        };
        Keyword::Simple::define 'union', sub {
            my ($ref) = @_;
            warn 'UNION';

            #warn $$ref;
            $$ref =~ s[\s*(?<name>\S+?)\s+{\s*(?<types>.*?)\s*}\s*;][Dynamo::union(name   => $+{name}, fields => { i => Dynamo::i32(), f => Dynamo::f32(), c => Dynamo::u16() } );]s;
            #warn $1;
            #warn $2;
            warn $+{types};
            warn join '|', _csv( $+{types} );

            my @types= map {
              warn $_;
            }

            split ';', $+{types};
ddx
\@types;
Dynamo::union(name   => $+{name}, fields => { i => Dynamo::i32(), f => Dynamo::f32(), c => Dynamo::u16() } );


die $$ref;
        };
        Keyword::Simple::define 'enum', sub {
            my ($ref) = @_;
            warn 'ENUM';

            #warn $$ref;
            $$ref =~ s[\s*(?<name>\S+?)\s+(?<types>.*?);][]s;
            warn $1;
            warn $2;
            warn $+{types};
            warn join '|', _csv( $+{types} );
        };
        Keyword::Simple::define 'class', sub {
            my ($ref) = @_;
            warn 'CLASS';
        };
        Keyword::Simple::define 'field', sub {
            my ($ref) = @_;
            warn 'FIELD';
        };
        Keyword::Simple::define 'method', sub {
            my ($ref) = @_;
            warn 'METHOD';
        };
    }

    sub unimport {

        # lexically disable keyword again
        Keyword::Simple::undefine 'provided';
        Keyword::Simple::undefine 'class';
    }
}
#
BEGIN { Dynamo->import }
use Data::Dump;
use Test2::V0;
use Test2::Tools::Compare qw[string];
subtest 'Dynamo::Type::* subtests' => sub {
    #
    #::Void
    subtest 'Dynamo::Type' => sub {
        isa_ok +Dynamo::Type::bool->new(1), ['Dynamo::Type::bool'], '...(1) isa Dynamo::Type::bool';
        isa_ok +Dynamo::Type::bool->new(1.5), ['Dynamo::Type::bool'],
            '...(1.5) isa Dynamo::Type::bool';
        isa_ok +Dynamo::Type::bool->new(-1), ['Dynamo::Type::bool'],
            '...(-1) isa Dynamo::Type::bool';
        isa_ok +Dynamo::Type::bool->new(+1), ['Dynamo::Type::bool'],
            '...(+1) isa Dynamo::Type::bool';
        isa_ok +Dynamo::Type::bool->new(0), ['Dynamo::Type::bool'],
            '...(+1) isa Dynamo::Type::bool';
        is +Dynamo::Type::bool->new(+1),        T(), '...(+1) is true';
        is +Dynamo::Type::bool->new(-1),        T(), '...(-1) is true';
        is +Dynamo::Type::bool->new(0),         F(), '...(0) is false';
        is +Dynamo::Type::bool->new( !0 ),      T(), '...(!0) is true';
        is +Dynamo::Type::bool->new( !1 ),      F(), '...(!1) is false';
        is +Dynamo::Type::bool->new(100),       1,   '...(100) is 1';
        is +Dynamo::Type::bool->new(-100),      1,   '...(-100) is 1';
        is +Dynamo::Type::bool->new(0),         !1,  '...(0) is !1';
        is +Dynamo::Type::bool->new( !0 ),      !!1, '...(!0) is !!1';
        is +Dynamo::Type::bool->new( !1 ),      !1,  '...(!1) is !1';
        is +Dynamo::Type::bool->new('a'),       D(), '...("a") is undefined';
        is +Dynamo::Type::bool->new(1)->sizeof, 1,   'sizeof() is 1';
    };
    subtest 'Dynamo::Type::i8' => sub {
        isa_ok +Dynamo::Type::i8->new(100), ['Dynamo::Type::i8'], '...(100) isa Dynamo::Type::i8';
        is +Dynamo::Type::i8->new('a'),       U(),  '...("a") is undefined';
        is +Dynamo::Type::i8->new(-1),        -1,   '...(-1) is -1';
        is +Dynamo::Type::i8->new(-128),      -128, '...(-128) is -128';
        is +Dynamo::Type::i8->new(-129),      U(),  '...(-129) is undefined';
        is +Dynamo::Type::i8->new(127),       127,  '...(127) is 127';
        is +Dynamo::Type::i8->new(128),       U(),  '...(128) is undefined';
        is +Dynamo::Type::i8->new(1)->sizeof, 1,    'sizeof() is 1';
    };
    subtest 'Dynamo::Type::u8' => sub {
        isa_ok +Dynamo::Type::u8->new(100), ['Dynamo::Type::u8'], '...(100) isa Dynamo::Type::u8';
        is +Dynamo::Type::u8->new('a'),       U(), '...("a") is undefined';
        is +Dynamo::Type::u8->new(0),         D(), '...(0) is defined...';
        is +Dynamo::Type::u8->new(0),         0,   '   ...(0) is 0';
        is +Dynamo::Type::u8->new(-1),        U(), '...(-1) is undefined';
        is +Dynamo::Type::u8->new(255),       255, '...(255) is 255';
        is +Dynamo::Type::u8->new(256),       U(), '...(256) is undefined';
        is +Dynamo::Type::u8->new(1)->sizeof, 1,   'sizeof() is 1';
    };

    #::i16
    #::u16
    #::i16
    #::u16
    #::i32
    #::u32
    #::i64
    #::u64
    subtest 'Dynamo::Type::f32' => sub {
        isa_ok +Dynamo::Type::f32->new(1), ['Dynamo::Type::f32'], '...(1) isa Dynamo::Type::f32';
        isa_ok +Dynamo::Type::f32->new(1.5), ['Dynamo::Type::f32'],
            '...(1.5) isa Dynamo::Type::f32';
        isa_ok +Dynamo::Type::f32->new(-1), ['Dynamo::Type::f32'], '...(-1) isa Dynamo::Type::f32';
        isa_ok +Dynamo::Type::f32->new(+1), ['Dynamo::Type::f32'], '...(+1) isa Dynamo::Type::f32';
        is +Dynamo::Type::f32->new('a'),       U(), '...("a") is undefined';
        is +Dynamo::Type::f32->new(8)->sizeof, 4,   'sizeof() is 4';
    };
    subtest 'Dynamo::Type::str' => sub {
        isa_ok +Dynamo::Type::str->new('Hello, world!'), ['Dynamo::Type::str'],
            '...("Hello, world") isa Dynamo::Type::str';
        is +Dynamo::Type::str->new('two')->sizeof,  3, 'sizeof() is 3';
        is +Dynamo::Type::str->new('four')->sizeof, 4, 'sizeof() is 4';
    };

    #::Double
};
subtest 'Dynamo::Enum' => sub {
    my $enum = Dynamo::Enum->new(
        name   => 'My::Test::Enum',
        type   => 'Dynamo::Type::f32',
        values => { red => 0, green => 20, blue => 21, orange => 20.5 }
    );
    isa_ok $enum, [qw[Dynamo::Enum]], '$enum isa Dynamo::Enum';
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
subtest 'Dynamo::Union' => sub {
    isa_ok Dynamo::union(    # anon union returns object
        fields => { i => Dynamo::i32(), f => Dynamo::f32(), c => Dynamo::u16() }
        ),
        'Dynamo::Union::Anon_1';
    is Dynamo::union(        # package name is given so bool is returned
        name   => 'My::Union',
        fields => { i => Dynamo::i32(), f => Dynamo::f32(), c => Dynamo::u16() }
        ),
        1, 'define My::Union';
    my $union = My::Union->new();
    isa_ok $union, [qw[Dynamo::Union]], 'newly constructed $union isa Dynamo::Union';
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
done_testing;


__END__
union Test::Union {
    field $a: isa(i8);

    }

union 'Some::Union'


package Test::Union{
use Dynamo::Union
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

enum Person::Level :isa(f32) { # default :isa(i16)
	red,
	green = 20,
	blue,
	orange = green + .5
};

$guy->shirt(Person::Level::green()); # or import green directly

__END__
struct Person : version(v3.14.0) {
    field $name   : isa(string)         :param; # required
    field $cash   : isa(f32);
    field $credit : isa(f32);
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
    field @numbers : isa(f32);
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
sub power (u32 $num, i32 $exp) :Returns(f32) :Library('somelib.so', 1.0) :Symbol('pow');

