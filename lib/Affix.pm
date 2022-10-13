package Affix 0.03 {
    use strict;
    use warnings;
    no warnings 'redefine';
    use File::Spec;
    use File::Basename qw[dirname];
    use File::Find     qw[find];
    use Config;
    use Sub::Util qw[subname];
    use Text::ParseWords;
    use Carp      qw[];
    use vars      qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use Dyn::Call qw[DC_SIGCHAR_CC_DEFAULT];

    #use Attribute::Handlers;
    no warnings 'redefine';
    use 5.030;
    use XSLoader;
    XSLoader::load( __PACKAGE__, our $VERSION );
    #
    use experimental 'signatures';
    #
    use parent 'Exporter';
    @EXPORT_OK          = sort map { @$_ = sort @$_; @$_ } values %EXPORT_TAGS;
    $EXPORT_TAGS{'all'} = \@EXPORT_OK;    # When you want to import everything

    #@{ $EXPORT_TAGS{'enum'} }             # Merge these under a single tag
    #    = sort map { defined $EXPORT_TAGS{$_} ? @{ $EXPORT_TAGS{$_} } : () }
    #    qw[types?]
    #    if 1 < scalar keys %EXPORT_TAGS;
    @EXPORT    # Export these tags (if prepended w/ ':') or functions by default
        = sort map { m[^:(.+)] ? @{ $EXPORT_TAGS{$1} } : $_ } qw[:default :types]
        if keys %EXPORT_TAGS > 1;
    @{ $EXPORT_TAGS{all} } = our @EXPORT_OK = map { @{ $EXPORT_TAGS{$_} } } keys %EXPORT_TAGS;
    #
    my %_delay;

    sub go() {

        #warn "HOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO";
        #use Data::Dump;
        #ddx \%_delay;
        for my $sub ( keys %_delay ) {
            if ( defined $_delay{$sub} ) {

                #use Data::Dump;
                #ddx \%_delay;
                #warn $_delay{$sub}[3];
                my $line
                    = sprintf( 'package %s{sub{sub{Affix::guess_library_name(%s,%s)}}->()->();};',
                    $_delay{$sub}[0], $_delay{$sub}[1], $_delay{$sub}[2] );

                #warn $line;
                #warn eval $line;
                #warn $main::lib_file;
                my $cv = attach( guess_library_name( eval( $_delay{$sub}[1] ), $_delay{$sub}[2] ),
                    $_delay{$sub}[3], $_delay{$sub}[4], $_delay{$sub}[5], $_delay{$sub}[6] );

                #{
                #    no strict 'refs';
                #    *{$sub} = $cv;
                # }
                #*{$full_name} = $cv
                delete $_delay{$sub};
                return goto &$cv;
            }
        }
    }

    sub AUTOLOAD {
        my $self = $_[0];           # Not shift, using goto.
        my $sub  = our $AUTOLOAD;
        if ( defined $_delay{$sub} ) {

            #warn 'Wrapping ' . $sub;
            #use Data::Dump;
            #ddx $_delay{$sub};
            my $cv = attach(
                guess_library_name( eval( $_delay{$sub}[1] ), $_delay{$sub}[2] ),
                $_delay{$sub}[3], $_delay{$sub}[4], $_delay{$sub}[5],
                $_delay{$sub}[6], $_delay{$sub}[7]
            );
            delete $_delay{$sub};
            return goto &$cv;
        }

        #~ elsif ( my $code = $self->can('SUPER::AUTOLOAD') ) {
        #~ return goto &$code;
        #~ }
        elsif ( $sub =~ /DESTROY$/ ) {
            return;
        }
        Carp::croak("No method $sub ...");
    }
    #
    sub MODIFY_CODE_ATTRIBUTES ( $package, $code, @attributes ) {

        #use Data::Dump;
        #ddx \@_;
        my ( $library, $library_version, $signature, $return, $symbol, $full_name, $mode );
        for my $attribute (@attributes) {
            if ( $attribute =~ m[^Native\(\s*(.+)\s*\)\s*$] ) {
                ( $library, $library_version ) = Text::ParseWords::parse_line( '\s*,\s*', 1, $1 );

                #warn $library;
                #warn $library_version;
                $library_version //= 0;
            }
            elsif ( $attribute =~ m[^Symbol\(\s*(['"])?\s*(.+)\s*\1\s*\)$] ) {
                $symbol = $2;
            }
            elsif ( $attribute =~ m[^Mode\(\s*(DC_SIGCHAR_CC_.+?)\s*\)$] ) {
                $mode    # Don't wait for Dyn::Call::DC_SIGCHAR...
                    = $1 eq 'DC_SIGCHAR_CC_DEFAULT'        ? ':' :
                    $1 eq 'DC_SIGCHAR_CC_THISCALL'         ? '*' :
                    $1 eq 'DC_SIGCHAR_CC_ELLIPSIS'         ? 'e' :
                    $1 eq 'DC_SIGCHAR_CC_ELLIPSIS_VARARGS' ? '.' :
                    $1 eq 'DC_SIGCHAR_CC_CDECL'            ? 'c' :
                    $1 eq 'DC_SIGCHAR_CC_STDCALL'          ? 's' :
                    $1 eq 'DC_SIGCHAR_CC_FASTCALL_MS'      ? 'F' :
                    $1 eq 'DC_SIGCHAR_CC_FASTCALL_GNU'     ? 'f' :
                    $1 eq 'DC_SIGCHAR_CC_THISCALL_MS'      ? '+' :
                    $1 eq 'DC_SIGCHAR_CC_THISCALL_GNU'     ? '#' :
                    $1 eq 'DC_SIGCHAR_CC_ARM_ARM'          ? 'A' :
                    $1 eq 'DC_SIGCHAR_CC_ARM_THUMB'        ? 'a' :
                    $1 eq 'DC_SIGCHAR_CC_SYSCALL'          ? '$' :
                    length($1) == 1                        ? $1 :
                    return $attribute;
                $mode = ord $mode if $mode =~ /\D/;
            }

           #elsif ( $attribute =~ m[^Signature\s*?\(\s*(.+?)?(?:\s*=>\s*(\w+)?)?\s*\)$] ) { # pretty
            elsif ( $attribute =~ m[^Signature\(\s*(\[.*\])\s*=>\s*(.*)\)$] ) {    # pretty
                my $args = $1;
                my $ret  = $2;

                #my ($ret, $args) = $1 =~ m[^(\w+)(?:\s*=>\s*(.+))?$];
                $ret //= 'Void';

                #warn $args;
                #warn $ret;
                $signature = eval($args);
                $return    = eval($ret);
            }
            else { return $attribute }
        }
        $mode      //= DC_SIGCHAR_CC_DEFAULT();
        $signature //= [];
        $return    //= Void();
        $full_name = subname $code;
        if ( !grep { !defined } $library, $library_version, $full_name ) {
            if ( !defined $symbol ) {
                $full_name =~ m[::(.*?)$];
                $symbol = $1;
            }

            #use Data::Dump;
            #ddx [
            #    $package,   $library, $library_version, $symbol,
            #    $signature, $return,  $mode,            $full_name
            #];
            if ( defined &{$full_name} ) {    #no strict 'refs';
                return attach( guess_library_name( eval($library), $library_version ),
                    $symbol, $signature, $return, $mode, $full_name );
            }
            $_delay{$full_name} = [
                $package,   $library, $library_version, $symbol,
                $signature, $return,  $mode,            $full_name
            ];
        }
        return;
    }
    our $OS = $^O;

    sub guess_library_name ( $name = (), $version = () ) {
        ( $name, $version ) = @$name if ref $name eq 'ARRAY';
        $name // return ();    # NULL
        return $name if -f $name;
        CORE::state $_lib_cache;
        my @retval;
        ($version) = version->parse($version)->stringify =~ m[^v?(.+)$];

        # warn $version;
        $version = $version ? qr[\.${version}] : qr/([\.\d]*)?/;
        if ( !defined $_lib_cache->{ $name . chr(0) . ( $version // '' ) } ) {
            if ( $OS eq 'MSWin32' ) {
                $name =~ s[\.dll$][];

                #return $name . '.dll'     if -f $name . '.dll';
                return File::Spec->canonpath( File::Spec->rel2abs( $name . '.dll' ) )
                    if -e $name . '.dll';
                require Win32;

# https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order#search-order-for-desktop-applications
                my @dirs = grep {-d} (
                    dirname( File::Spec->rel2abs($^X) ),               # 1. exe dir
                    Win32::GetFolderPath( Win32::CSIDL_SYSTEM() ),     # 2. sys dir
                    Win32::GetFolderPath( Win32::CSIDL_WINDOWS() ),    # 4. win dir
                    File::Spec->rel2abs( File::Spec->curdir ),         # 5. cwd
                    File::Spec->path(),                                # 6. $ENV{PATH}

                    #'C:\Program Files'
                );
                warn $_ for sort { lc $a cmp lc $b } @dirs;
                find(
                    {   wanted => sub {
                            $File::Find::prune = 1
                                if !grep { $_ eq $File::Find::name } @dirs;    # no depth
                            push @retval, $_ if m{[/\\]${name}(-${version})?\.dll$}i;
                        },
                        no_chdir => 1
                    },
                    @dirs
                );
            }
            elsif ( $OS eq 'darwin' ) {
                return $name . '.so'     if -f $name . '.so';
                return $name . '.dylib'  if -f $name . '.dylib';
                return $name . '.bundle' if -f $name . '.bundle';
                return $name             if $name =~ /\.so$/;
                return $name;    # Let 'em work it out

# https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/UsingDynamicLibraries.html
                my @dirs = (
                    dirname( File::Spec->rel2abs($^X) ),          # 0. exe dir
                    File::Spec->rel2abs( File::Spec->curdir ),    # 0. cwd
                    File::Spec->path(),                           # 0. $ENV{PATH}
                    map      { File::Spec->rel2abs($_) }
                        grep { -d $_ } qw[~/lib /usr/local/lib /usr/lib],
                    map { $ENV{$_} // () }
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                );

                #use Test::More;
                #diag join ', ', @dirs;
                #warn;
                find(
                    {   wanted => sub {
                            $File::Find::prune = 1
                                if !grep { $_ eq $File::Find::name } @dirs;    # no depth
                            push @retval, $_ if /\b(?:lib)?${name}${version}\.(so|bundle|dylib)$/;
                        },
                        no_chdir => 1
                    },
                    @dirs
                );

                #diag join ', ', @retval;
            }
            else {
                return $name . '.so' if -f $name . '.so';
                return $name         if -f $name;
                my $ext = $Config{so};
                my @libs;

               # warn $name . '.' . $ext . $version;
               #\b(?:lib)?${name}(?:-[\d\.]+)?\.${ext}${version}
               #my @lines = map { [/^\t(.+)\s\((.+)\)\s+=>\s+(.+)$/] }
               #    grep {/\b(?:lib)?${name}(?:-[\d\.]+)?\.${ext}(?:\.${version})?$/} `ldconfig -p`;
               #push @retval, map { $_->[2] } grep { -f $_->[2] } @lines;
                my @dirs = (
                    dirname( File::Spec->rel2abs($^X) ),          # 0. exe dir
                    File::Spec->rel2abs( File::Spec->curdir ),    # 0. cwd
                    File::Spec->path(),                           # 0. $ENV{PATH}
                    map      { File::Spec->rel2abs($_) }
                        grep { -d $_ } qw[~/lib /usr/local/lib /usr/lib],
                    map { $ENV{$_} // () }
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                );

                #use Data::Dump;
                #ddx \@dirs;
                find(
                    {   wanted => sub {
                            $File::Find::prune = 1
                                if !grep { $_ eq $File::Find::name } @dirs;    # no depth
                            push @retval, $_ if /\b(?:lib)?${name}(?:-[\d\.]+)?\.${ext}${version}$/;
                            push @retval, $_ if /\b(?:lib)?${name}(?:-[\d\.]+)?\.${ext}$/;
                        },
                        no_chdir => 1
                    },
                    @dirs
                );
            }
            $_lib_cache->{ $name . chr(0) . ( $version // '' ) } = pop @retval;
        }

        # TODO: Make a test with a bad lib name
        $_lib_cache->{ $name . chr(0) . ( $version // '' ) }
            // Carp::croak( 'Cannot locate symbol: ' . $name );
    }
};
1;
__END__

=encoding utf-8

=head1 NAME

Affix - 'FFI' is my middle name!

=head1 SYNOPSIS

    use Affix;
    attach( ( $^O eq 'MSWin32' ? 'ntdll' : 'libm' ), 'pow', [ Double, Double ] => Double );
    print pow( 2, 10 );    # 1024

=head1 DESCRIPTION

Dyn is a wrapper around L<dyncall|https://dyncall.org/>. If you're looking to
design your own low level wrapper, see L<Dyn.pm|Dyn>.

=head1 C<:Native> CODE attribute

While most of the upstream API is covered in the L<Dyn::Call>,
L<Dyn::Callback>, and L<Dyn::Load> packages, all the sugar is right here in
C<Affix>. The most simple use of C<Affix> would look something like this:

    use Affix ':all';
    sub some_argless_function() : Native('somelib.so') : Signature([]=>Void);
    some_argless_function();

Be aware that this will look a lot more like L<NativeCall from
Raku|https://docs.raku.org/language/nativecall> before v1.0!

The second line above looks like a normal Perl sub declaration but includes the
C<:Native> attribute to specify that the sub is actually defined in a native
library.

To avoid banging your head on a built-in function, you may name your sub
anything else and let Dyn know what symbol to attach:

    sub my_abs : Native('my_lib.dll') : Signature([Double]=>Double) : Symbol('abs');
    CORE::say my_abs( -75 ); # Should print 75 if your abs is something that makes sense

This is by far the fastest way to work with this distribution but it's not by
any means the only way.

All of the following methods may be imported by name or with the C<:sugar> tag.

Note that everything here is subject to change before v1.0.

=head1 Functions

The less

=head2 C<wrap( ... )>

Creates a wrapper around a given symbol in a given library.

    my $pow = Dyn::wrap( 'C:\Windows\System32\user32.dll', 'pow', [Double, Double]=>Double );
    warn $pow->(5, 10); # 5**10

Expected parameters include:

=over

=item C<lib> - pointer returned by L<< C<dlLoadLibrary( ... )>|Dyn::Load/C<dlLoadLibrary( ... )> >> or the path of the library as a string

=item C<symbol_name> - the name of the symbol to call

=item C<signature> - signature defining argument types, return type, and optionally the calling convention used

=back

Returns a code reference.

=head2 C<attach( ... )>

Wraps a given symbol in a named perl sub.

    Dyn::attach('C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );

=head1 Signatures

C<dyncall> uses an almost C<pack>-like syntax to define signatures. Affix is
inspired by L<Type::Standard>:

=over

=item C<Void>

=item C<Int>



=back

=head1 See Also

Check out L<FFI::Platypus> for a more robust and mature FFI.

Examples found in C<eg/>.

=head1 LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.

=head1 AUTHOR

Sanko Robinson E<lt>sanko@cpan.orgE<gt>

=begin stopwords

dyncall OpenBSD FreeBSD macOS DragonFlyBSD NetBSD iOS ReactOS mips mips64 ppc32
ppc64 sparc sparc64 co-existing varargs variadic

=end stopwords

=cut