package Affix::Native 0.12 {
    use strict;
    use warnings;
    no warnings 'redefine';
    use Sub::Util qw[subname];
    use Carp      qw[];
    use vars      qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use Affix;
    #
    use parent 'Exporter';
    $EXPORT_TAGS{default} = [qw[MODIFY_CODE_ATTRIBUTES AUTOLOAD]];
    @EXPORT_OK = @EXPORT = @{ $EXPORT_TAGS{all} } = sort @{ $EXPORT_TAGS{default} };

    #~ use Data::Dump;
    #~ ddx \@EXPORT_OK;
    #~ ddx \@EXPORT;
    #~ ddx \%EXPORT_TAGS;
    #
    my %_delay;

    sub AUTOLOAD {
        my $self = $_[0];           # Not shift, using goto.
        my $sub  = our $AUTOLOAD;
        if ( defined $_delay{$sub} ) {

            #warn 'Wrapping ' . $sub;
            #use Data::Dump;
            #ddx $_delay{$sub};
            my $template = qq'package %s {use Affix qw[:types]; sub{%s}->(); }';
            my $sig      = eval sprintf $template, $_delay{$sub}[0], $_delay{$sub}[4];
            Carp::croak $@ if $@;
            my $ret = eval sprintf $template, $_delay{$sub}[0], $_delay{$sub}[5];
            Carp::croak $@ if $@;

            #use Data::Dump;
            #ddx $_delay{$sub};
            #~ ddx locate_lib( $_delay{$sub}[1], $_delay{$sub}[2] );
            my $lib = defined $_delay{$sub}[1] ? scalar Affix::locate_lib( $_delay{$sub}[1], $_delay{$sub}[2] ) : undef;

            #~ use Data::Dump;
            #~ ddx [
            #~ $lib, (
            #~ $_delay{$sub}[3] eq $_delay{$sub}[6] ? $_delay{$sub}[3] :
            #~ [ $_delay{$sub}[3], $_delay{$sub}[6] ]
            #~ ),
            #~ $sig, $ret
            #~ ];
            my $cv = Affix::affix( $lib, ( $_delay{$sub}[3] eq $_delay{$sub}[6] ? $_delay{$sub}[3] : [ $_delay{$sub}[3], $_delay{$sub}[6] ] ), $sig,
                $ret );
            Carp::croak 'Undefined subroutine &' . $_delay{$sub}[6] unless $cv;
            delete $_delay{$sub} if defined $_delay{$sub};
            return &$cv;
        }

        #~ elsif ( my $code = $self->can('SUPER::AUTOLOAD') ) {
        #~ return goto &$code;
        #~ }
        elsif ( $sub =~ /DESTROY$/ ) {
            return;
        }
        Carp::croak("Undefined subroutine &$sub called");
    }
    #
    #~ use Attribute::Handlers;
    #~ my %name;
    #~ sub cache {
    #~ return $name{$_[2]}||*{$_[1]}{NAME};
    #~ }
    #~ sub UNIVERSAL::Name :ATTR {
    #~ $name{$_[2]} = $_[4];
    #~ }
    #~ sub UNIVERSAL::Purpose :ATTR {
    #~ print STDERR "Purpose of ", &name, " is $_[4]\n";
    #~ }
    #~ sub UNIVERSAL::Unit :ATTR {
    #~ print STDERR &name, " measured in $_[4]\n";
    #~ }
    #~ sub dump_cache{use Data::Dump; ddx \%name;}
    sub MODIFY_CODE_ATTRIBUTES {
        my ( $package, $code, @attributes ) = @_;

        #~ use Data::Dump;
        #~ ddx \@_;
        my ( $library, $library_version, $signature, $return, $symbol, $full_name );
        for my $attribute (@attributes) {

            #~ warn $attribute;
            if (
                $attribute =~ m[^Native(?:\((["']?)(.+?)\1(?:,\s*(.+))?\))?$]

                # m[/^\bNative\s+(?:(\w+)\s*,\s*(\d+))?$/]
            ) {
                $library = $2 // ();

                #~ warn $library;
                #~ warn $library_version;
                $library_version = $3 // 0;
            }
            elsif ( $attribute =~ m[^Symbol\(\s*(['"])?\s*(.+)\s*\1\s*\)$] ) {
                $symbol = $2;
            }

            #elsif ( $attribute =~ m[^Signature\s*?\(\s*(.+?)?(?:\s*=>\s*(\w+)?)?\s*\)$] ) { # pretty
            elsif ( $attribute =~ m[^Signature\(\s*(\[.*\])\s*=>\s*(.*)\)$] ) {    # pretty
                $signature = $1;
                $return    = $2;
            }
            else { return $attribute }
        }
        $signature //= '[]';
        $return    //= 'Void';
        $full_name = subname $code;    #$library, $library_version,
        if ( !grep { !defined } $full_name ) {
            if ( !defined $symbol ) {
                $full_name =~ m[::(.*?)$];
                $symbol = $1;
            }

            #use Data::Dump;
            #ddx [
            #    $package,   $library, $library_version, $symbol,
            #    $signature, $return,  $full_name
            #];
            if ( defined &{$full_name} ) {    #no strict 'refs';

                # TODO: call this defined sub and pass the wrapped symbol and then the passed args
                #...;
                return affix(
                    locate_lib( $library, $library_version ),
                    ( $symbol eq $full_name ? $symbol : [ $symbol, $full_name ] ),
                    $signature, $return
                );
            }
            $_delay{$full_name} = [ $package, $library, $library_version, $symbol, $signature, $return, $full_name ];
        }
        return;
    }
};
1;

=encoding utf-8

=head1 NAME

Affix::Native - Syntactic Sugar for Affix

=head1 SYNOPSIS

    use Affix::Native;

    sub pow : Native('m', v6) : Signature([Double, Double] => Double);
    warn pow(2, 5);

    sub puts : Native : Signature([Str]=>Int);
    puts("Wow!");

=head1 DESCRIPTION

Affix is nice. But let's add a little Raku flavored sugar. This API is inspired by L<Raku's
C<native>trait|https://docs.raku.org/language/nativecall>.

A simple example would look like this under Affix:

    use Affix::Native;
    sub some_argless_function :Native('something');
    some_argless_function();

=head1 C<:Native> CODE attribute

We use the C<:Native> attribute in order to specify that the sub is actually defined in a native library.

The first time you call "some_argless_function", the "libsomething" will be loaded and the "some_argless_function" will
be located in it. A call will then be made. Subsequent calls will be faster, since the symbol handle is retained.

Of course, most functions take arguments or return values--but everything else that you can do is just adding to this
simple pattern of declaring a Perl sub, naming it after the symbol you want to call and marking it with the
C<:Native>-related attributes.

To include a version number, your code should look like this:

    sub some_argless_function :Native('foo', v1.2.3)

=head1 C<:Symbol>

Affix provides the C<:Symbol> attribute for you to specify the name of the native routine in your library that may be
different from your Perl subroutine name.

    package Foo;
    sub init :Native('foo') :Symbol('FOO_INIT');

Inside of C<libfoo> there is a routine called C<FOO_INIT> but, since we're creating a module called C<Foo> and we'd
rather call the routine as C<Foo::init> (instead of C<Foo::FOO_INIT>), we use the symbol trait to specify the name of
the symbol in C<libfoo> and call the subroutine whatever we want (C<init> in this case).

=head1 C<:Signature>

    sub add :Native("calculator") :Signature([Int, Int] => Int);

Here, we have declared that the function takes two 32-bit integers and returns a 32-bit integer.

=head1 See Also

L<Raku's C<native>trait|https://docs.raku.org/language/nativecall>

=head1 LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under the terms found in the Artistic License
2. Other copyrights, terms, and conditions may apply to data transmitted through this module.

=head1 AUTHOR

Sanko Robinson E<lt>sanko@cpan.orgE<gt>

=begin stopwords

dyncall

=end stopwords

=cut
