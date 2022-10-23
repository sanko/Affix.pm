package Affix::Enum;
use strict;
use warnings;

package Affix::Type::Enum {
    use strict;
    use warnings;
    our @ISA = qw[Affix::Type::Base];
    use Scalar::Util qw[looks_like_number];

    sub id ($$) {
        my ( $s, $value ) = @_;
        my @ret = map { $_->[0] } grep {
            ( looks_like_number( $_->[1] ) && looks_like_number($value) ) ? ( $_->[1] == $value ) :
                ( $_->[1] eq $value )
        } @{ $s->{values} };
        return () unless @ret;
        return shift @ret;
    }

    sub ids($;$) {
        my ( $s, $value ) = @_;

        #my %seen;
        my @ret = map { $_->[0] } grep {
            defined $value ?
                ( ( looks_like_number( $_->[1] ) && looks_like_number($value) ) ?
                    ( $_->[1] == $value ) :
                    ( $_->[1] eq $value ) ) :
                1
        } @{ $s->{values} };
        return () unless @ret;
        return @ret;
    }

    sub value ($$) {
        my ( $s, $id ) = @_;
        my @ret = map { $_->[1] } grep { $_->[0] eq $id } @{ $s->{values} };
        return () unless @ret;
        return shift @ret;
    }

    sub values ($) {
        my ( $s, $value ) = @_;
        my %seen;
        my @ret = grep { !$seen{$_}++ } map { $_->[1] } @{ $s->{values} };
        return () unless @ret;
        return @ret;
    }
}
package    #
    Affix {
    sub Enum($)       { Affix::Type::Enum::Int(@_); }
    sub Enum::Char($) { warn 'HERE'; Affix::Type::Enum::Char(@_); }

    package Affix::Enum {

        sub Char($) {
            Affix::Type::Enum::Base( 'Affix::Type::Enum::Char', shift, Affix::Char() );
        }
    }

    package Affix::Type::Enum::Int { our @ISA = (qw[Affix::Type::Int Affix::Type::Enum]); }

    package Affix::Type::Enum::Char { our @ISA = (qw[Affix::Type::Char Affix::Type::Enum]); }

    package Affix::Type::Enum::Float { our @ISA = (qw[Affix::Type::Float Affix::Type::Enum]); }

    package Affix::Type::Enum::Double { our @ISA = (qw[Affix::Type::Double Affix::Type::Enum]); }

    package Affix::Type::Enum::UInt { our @ISA = (qw[Affix::Type::UInt Affix::Type::Enum]); }

    package Affix::Type::Enum {

        sub Base($$$) {
            my ( $pkg, $vals, $type ) = @_;
            my @values;
            my $next = 0;
            my %values;
            for my $value (@$vals) {
                my ( $k, $v );
                if ( ref $value eq 'ARRAY' ) {
                    $k = $value->[0];
                    $v = $value->[1];
                    if ( defined $values{$v} ) {
                        $v = $values{$v};
                    }
                    elsif ( $v =~ m![\/\+\-\*]!g ) {    # TODO: support basic math like C
                        no warnings 'redefine';
                        my ( undef, $file, $line ) = caller(1);
                        my $eval = '';
                        $eval .= sprintf 'sub %s(){q[%s]}', $_, $values{$_} for keys %values;
                        $v = eval qq!{\n#line $line "$file"\n$eval;$v;}!;
                        die $@ if $@;
                    }
                }
                else {
                    $k = $value;
                    $v = $next++;
                }
                $values{$k} = $v;
                push @values, [ $k, $v ];
                $next = ++$v;
            }
            bless { type => $type, values => \@values }, $pkg;
        }

        sub UChar($) {
            Base( 'Affix::Type::Enum::UChar', shift, Affix::UChar() );
        }

        sub Short ($) {
            Base( 'Affix::Type::Enum::Short', shift, Affix::UShort() );
        }

        sub Int ($) {
            Base( 'Affix::Type::Enum::Int', shift, Affix::Int() );
        }

        sub UInt ($) {
            Base( 'Affix::Type::Enum::UInt', shift, Affix::UInt() );
        }

        sub Long($) {
            Base( 'Affix::Type::Enum::Long', shift, Affix::Long() );
        }

        sub ULong($) {
            Base( 'Affix::Type::Enum::ULong', shift, Affix::ULong() );
        }

        sub LongLong($) {
            Base( 'Affix::Type::Enum::LongLong', shift, Affix::LongLong() );
        }

        sub ULongLong($) {
            Base( 'Affix::Type::Enum::ULongLong', shift, Affix::ULongLong() );
        }

        sub Float ($) {
            Base( 'Affix::Type::Enum::Float', shift, Affix::Float() );
        }

        sub Double($) {
            Base( 'Affix::Type::Enum::Double', shift, Affix::Double() );
        }
    }
}
1;
