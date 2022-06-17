use strictures;
use Data::Dump;
sub I8(;$)      { Dyn::Type::I8->new( @_     ? @_ : 0 ); }
sub U8(;$)      { Dyn::Type::U8->new( @_     ? @_ : 0 ); }
sub Int(;$)     { Dyn::Type::I16->new( @_    ? @_ : 0 ); }
sub UInt(;$)    { Dyn::Type::U16->new( @_    ? @_ : 0 ); }
sub Float(;$)   { Dyn::Type::Float->new( @_  ? @_ : 0 ); }
sub Double(;$)  { Dyn::Type::Double->new( @_ ? @_ : 0 ); }
sub Void(;$)    { Dyn::Type::Void->new(@_) }
sub Struct(%)   { Dyn::Type::Struct->new( @_ ? @_ : [] ) }
sub Pointer($)  { Dyn::Type::Pointer->new(@_) }
sub Union($$)   { Dyn::Type::Union->new(@_) }
sub Optional($) { Dyn::Type::Optional->new(@_) }

package Dyn::Type {
    use overload '|' => sub {
        Dyn::Type::Union->new(@_);
        },
        '""' => sub {
        my $s = shift;
        $$s[0];
        },
        '0+' => sub {
        my $s = shift;
        $$s[0];
        },
        fallback => 0;
    use Carp;
    sub new {...}

    sub set($$) {
        my $s = shift;
        Carp::confess 'Value failed to check out for ' . ref $s if !$s->check(@_);
        use Data::Dump;
        ddx $s;
        $s->[0] = shift;
        ddx $s;
    }
};

package Dyn::Type::Pointer {
    use parent -norequire, 'Dyn::Type';

    sub check($$) {
        1;
    }
    sub new { my ( $package, $obj ) = @_; bless \$obj, $package; }
};

package Dyn::Type::Optional {
    use parent -norequire, 'Dyn::Type';

    sub check($$) {
        1;
    }
    sub new { my ( $package, $obj ) = @_; bless \$obj, $package; }
};

package Dyn::Type::Union {
    use parent -norequire, 'Dyn::Type';

    sub check($$) {
        ...;
    }

    sub new {
        my ( $package, $left, $right ) = @_;
        bless [ $left, $right ], $package;
    }
};

package Dyn::Type::Bool {
    use overload '|' => sub {
        warn 'bitwise or';
        use Data::Dump;
        ddx \@_;
        },
        '""' => sub {
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
        return if defined $f && !_check($f);
        @etc && return ();
        return bless [$f], $package;
    }
    sub sizeof() {1}
}

package Dyn::Type::Byte {    # char
    use parent -norequire, 'Dyn::Type::U8';
}

package Dyn::Type::I8 {    # char
    use overload
        '|' => sub {
        warn 'bitwise or';
        use Data::Dump;
        ddx \@_;
        },
        '0+' => sub {
        my $s = shift;
        $$s[0];
        },
        '""'     => 'value',
        fallback => 1;

    sub _check {
        my $value = shift;
        return if $value !~ m[^(?:[-\+])?\d+$];
        return if $value < -128 || $value > 127;
        1;
    }

    sub new {
        my ( $package, $f, @etc ) = @_;
        return if defined $f && !_check($f);
        @etc && return ();
        return bless [$f], $package;
    }
    sub sizeof($) {1}

    sub value($) {
        my $s = shift;
        $$s[0];
    }
}

package Dyn::Type::I16 {
    use parent -norequire, 'Dyn::Type::I8';
}

package Dyn::Type::U16 {
    use parent -norequire, 'Dyn::Type::U8';
}

package Dyn::Type::Float {
    use parent -norequire, 'Dyn::Type';
};

package Dyn::Type::Double {
    use parent -norequire, 'Dyn::Type::Float';
};

package Dyn::Type::Struct {
    use parent -norequire, 'Dyn::Type';

    sub new {
        my $package = shift;
        bless shift, $package;
    }
};
$|++;
my $int = I8(100);
ddx $int;

#warn $int->set(100);
ddx $int;

#warn $int;
#ddx Pointer [ Int | Float ];
my $struct = Struct [ x => Optional [Int], y => Pointer [Int] ];
ddx unwrap($int);
ddx unwrap($struct);

sub unwrap {
    my $this = shift;
    my $type = ref $this;
    CORE::state $chain;
    use Data::Dump;
    $chain //= {
        'Dyn::Type::I8'     => sub { shift->value },
        'Dyn::Type::Struct' => sub {
            my $s = shift;
            ddx $s;
            ddx {@$s};
            ...;
        }
    };
    $chain->{$type} // die 'Unknown type: ' . $type;
    return $chain->{$type}->($this);
    ...;
}
