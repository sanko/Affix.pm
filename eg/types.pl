use strictures;
use Data::Dump;
sub i8()       { bless \[], 'Dyn::Type::i8'; }
sub u8()       { bless \[], 'Dyn::Type::u8' }
sub Int()      { bless \[], 'Dyn::Type::i16'; }
sub UInt()     { bless \[], 'Dyn::Type::u16'; }
sub Float()    { bless \[], 'Dyn::Type::float'; }
sub Double()   { bless \[], 'Dyn::Type::double'; }
sub void()     {...}
sub Struct(%)  { ddx \@_ }
sub Pointer($) { Dyn::Type::Pointer->new(@_) }

package Dyn::Type::Pointer {
    use overload '|' => sub {
        warn 'bitwise or';
        use Data::Dump;
        ddx \@_;
        Dyn::Type::Union->new(@_);
        },
        fallback => 0;

    sub check($$) {
        1;
    }
    sub new { my ( $package, $obj ) = @_; bless \$obj, $package; }
};
sub Union($$) { Dyn::Type::Union->new(@_) }

package Dyn::Type::Union {
    use overload '|' => sub {
         use Data::Dump;
        ddx \@_;
        Dyn::Type::Union->new(@_);
        },
        fallback => 0;

    sub check($$) {
        1;
    }

    sub new {
        my ( $package, $left, $right ) = @_;
        bless \[ $left, $right ], $package;
    }
};

package Dyn::Type::i8 {
    use overload '|' => sub {
        Dyn::Type::Union->new(@_);
        },
        fallback => 0;

    sub check($$) {
        $_[1] >= -128 && $_[1] <= 127;
    }
};

package Dyn::Type::u16 { }

package Dyn::Type::float {
    use overload '|' => sub {
        Dyn::Type::Union->new(@_);
    }
};

package Dyn::Type::double {
    use overload '|' => sub {
        Dyn::Type::Union->new(@_);
    }
};

package Dyn::Type::struct {
    use overload '|' => sub { warn 'bitwise or' }
};
$|++;
my $int = i8;
ddx $int;
warn $int->check(32);
ddx Pointer [ Int | Float ];
ddx Struct [ x => Int, y => Pointer [Int] ];
