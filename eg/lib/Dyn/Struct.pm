package Dyn::Struct {
    use strictures 2;
    use Data::Dump;
    use Keyword::Simple;
    use Dyn::Types qw[:all];
    use Carp;
    use Exporter;
    our @ISA       = qw(Exporter);
    our @EXPORT_OK = qw[Int UInt Str Float UFloat Object ArrayRef Ref Undef Object];
    my @class;
    my %_data;
    sub dump { ddx \%_data }

    sub _class_info {
        my $struct = shift;
        $_data{$struct} // die qq[No struct $struct is defined];
        $_data{$struct};
    }

    package Dyn::Struct::Base {

        sub new {
            my $package = shift;
            warn 'NEW';
            use Data::Dump;
            ddx \Dyn::Struct::_class_info($package);
        }
    }

    package Dyn::Struct::Abstract {

        sub new {
            my $package = shift;
            Carp::croak qq[Abstract class $package must be subclassed];
        }
    }

    sub import {

        #my ($caller, @imports)=@_;
        ddx \@_;
        Keyword::Simple::define has => sub {
            @class || Carp::croak 'Use of keyword has outside of a class';
            warn 'has';
            my ($ref) = @_;
            if ( $$ref =~ s/\s*(\w+)\s*([\$\@])(\w+)// ) {
                my ( $type, $sigil, $name ) = ( $1, $2, $3 );
                warn $type;
                warn $sigil;
                warn $name;
                push @{ $_data{ $class[-1] }{fields} }, [ $type, $sigil, $name ];

                #warn $$ref;
                return;
            }

            #die '------'.$$ref.'----';
        };

        # create keyword 'provided', expand it to 'if' at parse time
        Keyword::Simple::define struct => sub {
            my ($ref) = @_;

            #substr($$ref, 0, 0) = 'if';  # inject 'if' at beginning of parse buffer
            if ( $$ref =~ s/(\w+)\s*?(?:\s+(\w+)\s*?)?(?^:((?:\{(?:(?>[^\{\}]+)|(?-1))*\})))?// ) {
                push @class, $1;
                my $block = $3 // '';
                warn 'new class: ' . $class[-1];
                warn $block;
                my $isa = $2 // $_data{ $class[-1] }{isa} // 'Dyn::Struct::Base';
                substr( $$ref, 0, 0 ) = qq[package $1 {use parent -norequire, qw[$isa]; $block}];
                $_data{ $class[-1] }{isa} //= $isa;

                # TODO: Define class as type
                #{{{do <${block}>}}};
                #die '------'.$$ref.'----';
            }
        };
        Keyword::Simple::define abstract => sub {
            my ($ref) = @_;
            if ( $$ref =~ /^\s+struct\s+(\w+)/ ) {
                warn 'ABSTRACT';
                $_data{$1}{isa} = 'Dyn::Struct::Abstract';
            }
        }
    }

    sub unimport {

        # lexically disable keyword again
        Keyword::Simple::undefine 'struct';
        Keyword::Simple::undefine 'has';
    }
}
1;
