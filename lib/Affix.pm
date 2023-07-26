package Affix 0.12 {    # 'FFI' is my middle name!

    # ABSTRACT: A Foreign Function Interface eXtension
    use strict;
    use warnings;
    no warnings 'redefine';
    use File::Spec::Functions qw[rel2abs canonpath curdir path catdir];
    use File::Basename        qw[basename dirname];
    use File::Find            qw[find];
    use Config;
    use Sub::Util qw[subname];
    use Carp      qw[];
    use vars      qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use XSLoader;

    #~ our $VMSize = 1024; # defaults to 8192; passed to dcNewCallVM( ... )
    my $ok = XSLoader::load();
    #
    use parent 'Exporter';
    $EXPORT_TAGS{sugar} = [qw[MODIFY_CODE_ATTRIBUTES AUTOLOAD]];
    @EXPORT             = ( @{ $EXPORT_TAGS{types} }, @{ $EXPORT_TAGS{default} } );
    @EXPORT_OK          = sort map { @$_ = sort @$_; @$_ } values %EXPORT_TAGS;
    $EXPORT_TAGS{'all'} = \@EXPORT_OK;

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
            my $lib
                = defined $_delay{$sub}[1] ?
                scalar locate_lib( $_delay{$sub}[1], $_delay{$sub}[2] ) :
                undef;

            #~ use Data::Dump;
            #~ ddx [
            #~     $lib, (
            #~         $_delay{$sub}[3] eq $_delay{$sub}[6] ? $_delay{$sub}[3] :
            #~             [ $_delay{$sub}[3], $_delay{$sub}[6] ]
            #~     ),
            #~     $sig, $ret
            #~ ];
            my $cv = affix(
                $lib, (
                    $_delay{$sub}[3] eq $_delay{$sub}[6] ? $_delay{$sub}[3] :
                        [ $_delay{$sub}[3], $_delay{$sub}[6] ]
                ),
                $sig, $ret
            );
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
            if (
                $attribute =~ m[^Native\((["']?)(.+?)\1(?:,\s*(.+))?\)$]

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
            $_delay{$full_name}
                = [ $package, $library, $library_version, $symbol, $signature, $return,
                $full_name ];
        }
        return;
    }
    our $OS = $^O;
    my $is_win = $OS eq 'MSWin32';
    my $is_mac = $OS eq 'darwin';
    my $is_bsd = $OS =~ /bsd/;
    my $is_sun = $OS =~ /(solaris|sunos)/;

    sub locate_libs {
        my ( $lib, $version ) = @_;

        #~ warn $lib;
        #~ warn $version;
        #~ warn "Win: $is_win";
        #~ warn "Mac: $is_mac";
        #~ warn "BSD: $is_bsd";
        #~ warn "Sun: $is_sun";
        CORE::state $libdirs;
        if ( !defined $libdirs ) {
            if ($is_win) {
                require Win32;
                $libdirs = [
                    Win32::GetFolderPath( Win32::CSIDL_SYSTEM() ) . '/',
                    Win32::GetFolderPath( Win32::CSIDL_WINDOWS() ) . '/',
                ];
            }
            else {
                $libdirs = [
                    ( split ' ', $Config{libsdirs} ),
                    map      { warn $ENV{$_}; split /[:;]/, ( $ENV{$_} ) }
                        grep { $ENV{$_} }
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                ];
            }
            no warnings qw[once];
            require DynaLoader;
            $libdirs = [
                grep    { -d $_ }
                    map { rel2abs($_) }
                    qw[. ./lib ~/lib /usr/local/lib /usr/lib /lib /usr/lib/system],
                @DynaLoader::dl_library_path, @$libdirs
            ];
        }
        CORE::state $regex;
        if ( !defined $regex ) {
            $regex = $is_win ?
                qr/^
        (?:lib)?(?<name>\w+?)
        (?:[_-](?<version>[0-9\-\._]+))?_*
        \.$Config{so}
        $/ix :
                $is_mac ?
                qr/^
        (?:lib)?(?<name>\w+?)
        (?:\.(?<version>[0-9]+(?:\.[0-9]+)*))?
        \.(?:so|dylib|bundle)
        $/x :    # assume *BSD or linux
                qr/^
        (?:lib)?(?<name>\w+?)
        \.$Config{so}
        (?:\.(?<version>[0-9]+(?:\.[0-9]+)*))?
        $/x;
        }
        my @store;

        #~ warn join ', ', @$libdirs;
        find(
            sub {
                $File::Find::prune = 1
                    if !grep { canonpath $_ eq canonpath $File::Find::name } @$libdirs;

                #~ return                 if -d $_;
                return unless $_ =~ $regex;

                #~ use Data::Dump;
                #~ warn $File::Find::name;
                #~ ddx %+;
                push @store, { %+, path => $File::Find::name }
                    if defined $+{name}                                                          &&
                    ( $+{name} eq $lib )                                                         &&
                    ( defined $version ? defined( $+{version} ) && $version == $+{version} : 1 ) &&
                    -B $File::Find::name;
            },
            @$libdirs
        );
        @store;
    }

    sub locate_lib {
        my ( $name, $version ) = @_;
        return $name if $name && -e $name;
        CORE::state $cache;
        return $cache->{$name}{ $version // 0 }->{path} if defined $cache->{$name}{ $version // 0 };
        if ( !$version ) {
            return $cache->{$name}{0}{path} = rel2abs($name) if -e rel2abs($name);
            return $cache->{$name}{0}{path} = rel2abs( $name . '.' . $Config{so} )
                if -e rel2abs( $name . '.' . $Config{so} );
        }
        my @libs = locate_libs( $name, $version );

        #~ warn;
        #~ use Data::Dump;
        #~ warn join ', ', @_;
        #~ ddx \@_;
        #~ ddx $cache;
        #~ ddx \@libs;
        if (@libs) {
            ( $cache->{$name}{ $version // 0 } ) = @libs;
            return $cache->{$name}{ $version // 0 }->{path};
        }
        ();
    }
    {
        #~ ✅ https://gcc.gnu.org/git?p=gcc.git;a=blob_plain;f=gcc/cp/mangle.cc;hb=HEAD
        #~ ✅ https://rust-lang.github.io/rfcs/2603-rust-symbol-name-mangling-v0.html
        #~ ✅ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
        #~ ☑️ https://github.com/apple/swift/blob/main/docs/ABI/Mangling.rst#identifiers
        #~ ☑️ https://mikeash.com/pyblog/friday-qa-2014-08-15-swift-name-mangling.html
        #~ ☑️ https://dlang.org/spec/abi.html#name_mangling
        my @cache;
        my $vp = 0;    # void *
        my %symbol_cache;

        sub _mangle_name ($$) {
            my ( $func, $name ) = @_;
            if ( grep { $_ eq $name } @cache ) {
                return join '', 'S', ( grep { $cache[$_] eq $name } 0 .. $#cache ), '_';
            }
            push @cache, $name;
            $name =~ s[^$func][S0_];
            sprintf $name =~ '::' ? 'N%sE' : '%s',
                join( '', ( map { length($_) . $_ } split '::', $name ) );
        }

        sub _mangle_type {
            my ( $func, $type ) = @_;
            return    #'A'
                'P' . _mangle_type( $func, $type->{type} ) if $type->isa('Affix::Type::ArrayRef');
            if ( $type->isa('Affix::Type::Pointer') && $type->{type}->isa('Affix::Type::Void') ) {
                return $vp++ ? 'S_' : 'Pv';
            }

            #~ warn $type;
            #~ warn ref $type;
            return 'Pv' if $type->isa('Affix::Type::InstanceOf');
            return 'P' . _mangle_type( $func, $type->{type} ) if $type->isa('Affix::Type::Pointer');
            return _mangle_name( $func, $type->{typedef} )    if $type->isa('Affix::Type::Struct');
            CORE::state $types;
            $types //= {
                Char(),  'c',    # Note: signed char == 'a'
                Bool(),  'b', Double(), 'd', Long(),  'e', Float(), 'f', UChar(),  'h', Int(),  'i',
                UInt(),  'j', Long(),   'l', ULong(), 'm', Short(), 's', UShort(), 't', Void(), 'v',
                WChar(), 'w', LongLong(), 'x', ULongLong(), 'y', Str(), 'Pc', ord '_', '', Any(),
                'v'
            };
            $types->{$type} // die sprintf 'Unknown type: %s (%d)', chr($type), $type;
        }

        sub Itanium_mangle {
            my ( $lib, $name, $affix ) = @_;
            @cache = ();
            $vp    = 0;
            my $ret = '_Z' . sprintf $name =~ '::' ? 'N%sE' : '%s',
                join( '', ( map { length($_) . $_ } split '::', $name ) );

            #~ for my $arg ( scalar @{ $affix->{args} } ? @{ $affix->{args} } : Void() ) {
            my @args = scalar @{$affix} ? @{$affix} : Void();
            while (@args) {
                my $arg = shift @args;
                $ret .= _mangle_type( $name, $arg );
                if ( "$arg" == ord '_' ) {
                    shift @args;
                    push @args, Void() if !@args;    # skip calling conventions
                }
            }
            $ret;
        }

        # legacy
        sub Rust_legacy_mangle {
            my ( $lib, $name, $affix ) = @_;
            $symbol_cache{$lib} //= $lib->list_symbols();
            @cache = ();
            $vp    = 0;
            return $name if grep { $name eq $_ } grep { defined $_ } @{ $symbol_cache{$lib} };
            my $ret = qr'^_ZN(?:\d+\w+?)?' . sprintf $name =~ '::' ? '%sE' : '%s17h\w{16}E$',
                join( '', ( map { length($_) . $_ } split '::', $name ) );
            my @symbols = grep { $_ =~ $ret } grep { defined $_ } @{ $symbol_cache{$lib} };
            return shift @symbols;
        }
    }
};
1;
__END__
Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.
