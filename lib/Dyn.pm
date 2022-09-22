package Dyn 0.03 {
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

    sub wrap ( $lib, $symbol_name, $args, $return, $mode = DC_SIGCHAR_CC_DEFAULT ) {
        attach( $lib, $symbol_name, $args, $return, $mode );
    }

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
                    = sprintf( 'package %s{sub{sub{Dyn::guess_library_name(%s,%s)}}->()->();};',
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
        elsif ( $sub eq 'DESTROY' ) {
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

    sub _guess_library_name ( $lib = undef, $version = undef ) {
        $lib // return;
        return locate_lib( $lib, $version );
        CORE::state %_lib_cache;
        if ( !defined $_lib_cache{ $lib . chr(0) . ( $version // '' ) } ) {

            #use Data::Dump;
            #ddx \@_;
            if ( $^O eq 'MSWin32' ) {

                # .dll and no version info
            }
            else {
                my ( $volume, $directories, $file ) = File::Spec->splitpath($lib);

                #ddx [$volume, $directories, $file];
                my $so = $Config{so};    # TODO: might be .dynlib or .bundle
                $file = 'lib' . $file
                    if $file !~ m[^lib.+$] &&
                    $file !~ m[\.(?:${so}|bundle|dylib)$] &&
                    !defined $directories;

                #ddx [$volume, $directories, $file];
                $file = $file . '.' . $so if $file !~ m[\.$so(\.[\d\.])?$];
                $file .= '.' . $1 if defined $version && $^O !~ /bsd/ && $^O ne 'darwin' &&

                    #!defined $Config{ignore_versioned_solibs} &&
                    version->parse($version)->stringify =~ m[^v?(.+)$] && $1 > 0;
                $lib = File::Spec->catpath( $volume, $directories, $file );

                #ddx [$volume, $directories, $file];
            }

            #use Data::Dump;
            #warn $ENV{LD_LIBRARY_PATH};
            $_lib_cache{ $lib . chr(0) . ( $version // '' ) } = $lib;
        }
        warn $lib;
        $_lib_cache{ $lib . chr(0) . ( $version // '' ) };
    }
    our $OS = $^O;

    sub guess_library_name ( $name = (), $version = () ) {
        $name // return ();    # NULL
        return $name if -f $name;
        CORE::state $_lib_cache;
        my @retval;
        ($version) = version->parse($version)->stringify =~ m[^v?(.+)$];

        # warn $version;
        $version = $version ? qr[\.${version}] : qr/([\.\d]*)?/;
        if ( !defined $_lib_cache->{ $name . chr(0) . ( $version // '' ) } ) {
            if ( $OS eq 'MSWin32' ) {

                #return $name . '.dll'     if -f $name . '.dll';
                return File::Spec->canonpath( File::Spec->rel2abs( $name . '.dll' ) )
                    if -e $name . '.dll';
                require Win32;

# https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order#search-order-for-desktop-applications
                my @dirs = (
                    dirname( File::Spec->rel2abs($^X) ),               # 1. exe dir
                    Win32::GetFolderPath( Win32::CSIDL_SYSTEM() ),     # 2. sys dir
                    Win32::GetFolderPath( Win32::CSIDL_WINDOWS() ),    # 4. win dir
                    File::Spec->rel2abs( File::Spec->curdir ),         # 5. cwd
                    File::Spec->path()                                 # 6. $ENV{PATH}
                );
                find(
                    {   wanted => sub {
                            $File::Find::prune = 1
                                if !grep { $_ eq $File::Find::name } @dirs;    # no depth
                            push @retval, $_ if /\/${name}(-${version})?\.dll$/;
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

Dyn - dyncall Backed FFI

=head1 SYNOPSIS

    use Dyn qw[:sugar];
    sub pow
        : Native( $^O eq 'MSWin32' ? 'ntdll.dll' : ('libm', v6) )
        : Signature(Double, Double => Double);

    print pow( 2, 10 );    # 1024

=head1 DESCRIPTION

Dyn is a wrapper around L<dyncall|https://dyncall.org/>.

This distribution includes...

=over

=item L<Dyn::Call>

An encapsulation of architecture-, OS- and compiler-specific function call
semantics.

Functions can be imported with the C<:dc> tag.

=item L<Dyn::Callback>

Callback interface of C<dyncall> located in C<dyncallback>.

Functions can be imported with the C<:dcb> tag.

=item L<Dyn::Load>

Facilitates portable library symbol loading and access to functions in foreign
dynamic libraries and code modules.

Functions can be imported with the C<:dl> tag.

=back

Honestly, you should be using one of the above packages rather than this one as
they provide clean wrappers of dyncall's C functions. This package contains the
sugary API.

=head1 C<:Native> CODE attribute

While most of the upstream API is covered in the L<Dyn::Call>,
L<Dyn::Callback>, and L<Dyn::Load> packages, all the sugar is right here in
C<Dyn>. The most simple use of C<Dyn> would look something like this:

    use Dyn ':sugar';
    sub some_argless_function() : Native('somelib.so') : Signature('()v');
    some_argless_function();

Be aware that this will look a lot more like L<NativeCall from
Raku|https://docs.raku.org/language/nativecall> before v1.0!

The second line above looks like a normal Perl sub declaration but includes the
C<:Native> attribute to specify that the sub is actually defined in a native
library.

To avoid banging your head on a built-in function, you may name your sub
anything else and let Dyn know what symbol to attach:

    sub my_abs : Native('my_lib.dll') : Signature('(d)d') : Symbol('abs');
    CORE::say my_abs( -75 ); # Should print 75 if your abs is something that makes sense

This is by far the fastest way to work with this distribution but it's not by
any means the only way.

All of the following methods may be imported by name or with the C<:sugar> tag.

Note that everything here is subject to change before v1.0.

=head1 Functions

The less

=head2 C<wrap( ... )>

Creates a wrapper around a given symbol in a given library.

    my $pow = Dyn::wrap( 'C:\Windows\System32\user32.dll', 'pow', 'dd)d' );
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

    Dyn::attach('C:\Windows\System32\user32.dll', 'pow', '(dd)d' );

=head1 Signatures

C<dyncall> uses an almost C<pack>-like syntax to define signatures. A signature
is a character string that represents a function's arguments and return value
types. This is an essential part of mapping the more flexible and often
abstract data types provided in scripting languages to the strict machine-level
data types used by C-libraries.


=for future The high-level C interface functions L<<
C<dcCallF( ... )>|Dyn::Call/C<dcCallF( ... )> >>, L<< C<dcVCallF( ...
)>|Dyn::Call/C<dcVCallF( ... )> >>, L<< C<dcArgF( ... )>|Dyn::Call/C<dcArgF(
... )> >> and L<< C<dcVArgF( ... )>|Dyn::Call/C<dcVArgF( ... )> >> of the
dyncall library also make use of this signature string format.

Here are some signature examples along with their equivalent C function
prototypes:

    dyncall signature    C function prototype
    --------------------------------------------
    )v                   void      f1 ( )
    ii)i                 int       f2 ( int, int )
    p)L                  long long f3 ( void * )
    p)v                  void      f4 ( int ** )
    iBcdZ)d              double    f5 ( int, bool, char, double, const char * )
    _esl_.di)v           void      f6 ( short a, long long b, ... ) (for (promoted) varargs: double, int)
    (Zi)i                int       f7 ( const char *, int )
    (iiid)v              void      f8 ( int, int, int, double )

The following types are supported:

    Signature character     C/C++ data type
    ----------------------------------------------------
    v                       void
    B                       _Bool, bool
    c                       char
    C                       unsigned char
    s                       short
    S                       unsigned short
    i                       int
    I                       unsigned int
    j                       long
    J                       unsigned long
    l                       long long, int64_t
    L                       unsigned long long, uint64_t
    f                       float
    d                       double
    p                       void *
    Z                       const char * (pointer to a C string)

Please note that using a C<(> at the beginning of a signature string is
possible, although not required. The character doesn't have any meaning and
will simply be ignored. However, using it prevents annoying syntax highlighting
problems with some code editors.

Calling convention modes can be switched using the signature string, as well.
An C<_> in the signature string is followed by a character specifying what
calling convention to use, as this effects how arguments are passed. This makes
only sense if there are multiple co-existing calling conventions on a single
platform. Usually, this is done at the beginning of the string, except in
special cases, like specifying where the varargs part of a variadic function
begins. The following signature characters exist:

    Signature character   Calling Convention
    ------------------------------------------------------
    :                     platform's default calling convention
    e                     vararg function
    .                     vararg function's variadic/ellipsis part (...), to be specified before first vararg
    c                     only on x86: cdecl
    s                     only on x86: stdcall
    F                     only on x86: fastcall (MS)
    f                     only on x86: fastcall (GNU)
    +                     only on x86: thiscall (MS)
    #                     only on x86: thiscall (GNU)
    A                     only on ARM: ARM mode
    a                     only on ARM: THUMB mode
    $                     syscall

=head1 Platform Support

The dyncall library runs on many different platforms and operating systems
(including Windows, Linux, OpenBSD, FreeBSD, macOS, DragonFlyBSD, NetBSD,
Plan9, iOS, Haiku, Nintendo DS, Playstation Portable, Solaris, Minix, Raspberry
Pi, ReactOS, etc.) and processors (x86, x64, arm (arm & thumb mode), arm64,
mips, mips64, ppc32, ppc64, sparc, sparc64, etc.).

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
