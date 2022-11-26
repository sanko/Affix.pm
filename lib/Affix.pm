package Affix 0.04 {    # 'FFI' is my middle name!
    use strict;
    use warnings;
    no warnings 'redefine';
    use File::Spec;
    use File::Spec::Functions qw[rel2abs];
    use File::Basename        qw[dirname];
    use File::Find            qw[find];
    use Config;
    use Sub::Util qw[subname];
    use Text::ParseWords;
    use Carp      qw[];
    use vars      qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use Dyn::Call qw[:sigchar];

    #use Attribute::Handlers;
    #no warnings 'redefine';
    use XSLoader;
    XSLoader::load( __PACKAGE__, our $VERSION );
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

            #ddx [ $lib, $_delay{$sub}[3], $sig, $ret, $_delay{$sub}[6], $_delay{$sub}[7] ];
            my $cv
                = attach( $lib, $_delay{$sub}[3], $sig, $ret, $_delay{$sub}[6], $_delay{$sub}[7] );
            delete $_delay{$sub};
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
    sub MODIFY_CODE_ATTRIBUTES {
        my ( $package, $code, @attributes ) = @_;

        #use Data::Dump;
        #ddx \@_;
        my ( $library, $library_version, $signature, $return, $symbol, $full_name, $mode );
        for my $attribute (@attributes) {
            if ( $attribute =~ m[^Native(?:\(\s*(.+)\s*\)\s*)?$] ) {
                ( $library, $library_version ) = Text::ParseWords::parse_line( '\s*,\s*', 1, $1 );
                $library //= ();

                #warn $library;
                #warn $library_version;
                $library_version //= 0;
            }
            elsif ( $attribute =~ m[^Symbol\(\s*(['"])?\s*(.+)\s*\1\s*\)$] ) {
                $symbol = $2;
            }
            elsif ( $attribute =~ m[^Mode\(\s*(DC_SIGCHAR_CC_.+?)\s*\)$] ) {
                $mode    # Don't wait for Dyn::Call::DC_SIGCHAR...
                    = $1 eq 'DC_SIGCHAR_CC_DEFAULT'        ? DC_SIGCHAR_CC_DEFAULT :
                    $1 eq 'DC_SIGCHAR_CC_THISCALL'         ? DC_SIGCHAR_CC_THISCALL :
                    $1 eq 'DC_SIGCHAR_CC_ELLIPSIS'         ? DC_SIGCHAR_CC_ELLIPSIS :
                    $1 eq 'DC_SIGCHAR_CC_ELLIPSIS_VARARGS' ? DC_SIGCHAR_CC_ELLIPSIS_VARARGS :
                    $1 eq 'DC_SIGCHAR_CC_CDECL'            ? DC_SIGCHAR_CC_CDECL :
                    $1 eq 'DC_SIGCHAR_CC_STDCALL'          ? DC_SIGCHAR_CC_STDCALL :
                    $1 eq 'DC_SIGCHAR_CC_FASTCALL_MS'      ? DC_SIGCHAR_CC_FASTCALL_MS :
                    $1 eq 'DC_SIGCHAR_CC_FASTCALL_GNU'     ? DC_SIGCHAR_CC_FASTCALL_GNU :
                    $1 eq 'DC_SIGCHAR_CC_THISCALL_MS'      ? DC_SIGCHAR_CC_THISCALL_MS :
                    $1 eq 'DC_SIGCHAR_CC_THISCALL_GNU'     ? DC_SIGCHAR_CC_THISCALL_GNU :
                    $1 eq 'DC_SIGCHAR_CC_ARM_ARM'          ? DC_SIGCHAR_CC_ARM_ARM :
                    $1 eq 'DC_SIGCHAR_CC_ARM_THUMB'        ? DC_SIGCHAR_CC_ARM_THUMB :
                    $1 eq 'DC_SIGCHAR_CC_SYSCALL'          ? DC_SIGCHAR_CC_SYSCALL :
                    length($1) == 1                        ? $1 :
                    return $attribute;
                $mode = ord $mode if $mode =~ /\D/;
            }

           #elsif ( $attribute =~ m[^Signature\s*?\(\s*(.+?)?(?:\s*=>\s*(\w+)?)?\s*\)$] ) { # pretty
            elsif ( $attribute =~ m[^Signature\(\s*(\[.*\])\s*=>\s*(.*)\)$] ) {    # pretty
                $signature = $1;
                $return    = $2;
            }
            else { return $attribute }
        }
        $mode      //= DC_SIGCHAR_CC_DEFAULT();
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
            #    $signature, $return,  $mode,            $full_name
            #];
            if ( defined &{$full_name} ) {    #no strict 'refs';

                # TODO: call this defined sub and pass the wrapped symbol and then the passed args
                ...;
                return attach( locate_lib( $library, $library_version ),
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

    sub locate_lib {
        my ( $name, $version ) = @_;
        CORE::state $_lib_cache;
        ( $name, $version ) = @$name if ref $name eq 'ARRAY';
        $name // return ();    # NULL
        return $name if -e $name;
        {
            my $i   = -1;
            my $pkg = __PACKAGE__;
            ($pkg) = caller( ++$i ) while $pkg eq __PACKAGE__;    # Dig out of the hole first
            my $ok = $pkg->can($name);
            $name = $ok->() if $ok;
        }

        #$name = eval $name;
        $name =~ s[['"]][]g;
        #
        my @retval;
        ($version) = version->parse($version)->stringify =~ m[^v?(.+)$];

        # warn $version;
        $version = $version ? qr[\.${version}] : qr/([\.\d]*)?/;
        if ( !defined $_lib_cache->{ $name . chr(0) . ( $version // '' ) } ) {
            if ( $OS eq 'MSWin32' ) {
                $name =~ s[\.dll$][];

                #return $name . '.dll'     if -f $name . '.dll';
                return $_lib_cache->{ $name . chr(0) . ( $version // '' ) }
                    = File::Spec->canonpath( File::Spec->rel2abs( $name . '.dll' ) )
                    if -e $name . '.dll';
                require Win32;

# https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order#search-order-for-desktop-applications
                my @dirs = grep {-d} (
                    dirname( File::Spec->rel2abs($^X) ),                    # 1. exe dir
                    Win32::GetFolderPath( Win32::CSIDL_SYSTEM() ),          # 2. sys dir
                    Win32::GetFolderPath( Win32::CSIDL_WINDOWS() ),         # 4. win dir
                    File::Spec->rel2abs( File::Spec->curdir ),              # 5. cwd
                    File::Spec->path(),                                     # 6. $ENV{PATH}
                    map { split /[:;]/, ( $ENV{$_} ) } grep { $ENV{$_} }    # X. User defined
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                );

                #warn $_ for sort { lc $a cmp lc $b } @dirs;
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
                    map      { split /[:;]/, ( $ENV{$_} ) }
                        grep { $ENV{$_} }
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                );

                #use Test::More;
                #warn join ', ', @dirs;
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
                $_lib_cache->{ $name . chr(0) . ( $version // '' ) } = rel2abs pop @retval;

                #diag join ', ', @retval;
            }
            else {
                return $_lib_cache->{ $name . chr(0) . ( $version // '' ) } = rel2abs $name
                    if -f $name;
                return $_lib_cache->{ $name . chr(0) . ( $version // '' ) } = rel2abs $name . '.so'
                    if -f $name . '.so';
                my $ext = $Config{so};
                my @libs;

               # warn $name . '.' . $ext . $version;
               #\b(?:lib)?${name}(?:-[\d\.]+)?\.${ext}${version}
               #my @lines = map { [/^\t(.+)\s\((.+)\)\s+=>\s+(.+)$/] }
               #    grep {/\b(?:lib)?${name}(?:-[\d\.]+)?\.${ext}(?:\.${version})?$/} `ldconfig -p`;
               #push @retval, map { $_->[2] } grep { -f $_->[2] } @lines;
                my @dirs = grep { -d $_ } (
                    dirname( File::Spec->rel2abs($^X) ),          # 0. exe dir
                    File::Spec->rel2abs( File::Spec->curdir ),    # 0. cwd
                    File::Spec->path(),                           # 0. $ENV{PATH}
                    map { File::Spec->rel2abs($_) }
                        qw[. ./lib/ ~/lib /usr/local/lib /usr/lib /lib64/ /lib/],
                    map      { split /[:;]/, ( $ENV{$_} ) }
                        grep { $ENV{$_} }
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                );
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
            $_lib_cache->{ $name . chr(0) . ( $version // '' ) } = rel2abs pop @retval;
        }

        # TODO: Make a test with a bad lib name
        $_lib_cache->{ $name . chr(0) . ( $version // '' ) }
            // Carp::croak( 'Cannot locate symbol: ' . $name );
    }

    # define packages that are otherwise XS-only so PAUSE will find them in META.json
    {

        package Affix::Type::Base 0.04;

        package Affix::Type::Void 0.04;

        package Affix::Type::Bool 0.04;

        package Affix::Type::Char 0.04;

        package Affix::Type::UChar 0.04;

        package Affix::Type::Short 0.04;

        package Affix::Type::UShort 0.04;

        package Affix::Type::Int 0.04;

        package Affix::Type::UInt 0.04;

        package Affix::Type::Long 0.04;

        package Affix::Type::ULong 0.04;

        package Affix::Type::LongLong 0.04;

        package Affix::Type::ULongLong 0.04;

        package Affix::Type::Float 0.04;

        package Affix::Type::Double 0.04;

        package Affix::Type::Pointer 0.04;

        package Affix::Type::Str 0.04;

        package Affix::Type::Aggregate 0.04;    # Reserved

        package Affix::Type::Struct 0.04;

        package Affix::Type::ArrayRef 0.04;

        package Affix::Type::Union 0.04;

        package Affix::Type::CodeRef 0.04;

        package Affix::Type::InstanceOf 0.04;

        package Affix::Type::Any 0.04;

        package Affix::Type::SSize_t 0.04;

        package Affix::Type::Size_t 0.04;

        package Affix::Type::Enum 0.04;

        package Affix::Type::IntEnum 0.04;

        package Affix::Type::UIntEnum 0.04;

        package Affix::Type::CharEnum 0.04;
    }
};
1;
__END__

=encoding utf-8

=head1 NAME

Affix - A Foreign Function Interface eXtension

=head1 SYNOPSIS

    use Affix;
    my $lib
        = $^O eq 'MSWin32'    ? 'ntdll' :
        $^O eq 'darwin'       ? '/usr/lib/libm.dylib' :
        $^O eq 'bsd'          ? '/usr/lib/libm.so' :
        -e '/lib64/libm.so.6' ? '/lib64/libm.so.6' :
        '/lib/x86_64-linux-gnu/libm.so.6';
    attach( $lib, 'pow', [ Double, Double ] => Double );
    print pow( 2, 10 );    # 1024

=head1 DESCRIPTION

Affix is a wrapper around L<dyncall|https://dyncall.org/>. If you're looking to
design your own low level wrapper, see L<Dyn.pm|Dyn>.

=head1 C<:Native> CODE attribute

While most of the upstream API is covered in the L<Dyn::Call>,
L<Dyn::Callback>, and L<Dyn::Load> packages, all the sugar is right here in
C<Affix>. The simplest but least flexible use of C<Affix> would look something
like this:

    use Affix;
    sub some_iiZ_func : Native('somelib.so') : Signature([Int, Long, Str] => Void);
    some_iiZ_func( 100, time, 'Hello!' );

Let's step through what's here...

The second line above looks like a normal Perl sub declaration but includes our
CODE attributes:

=over

=item C<:Native>

Here, we're specifying that the sub is actually defined in a native library.
This is inspired by Raku's C<native> trait.

=item C<:Signature>

Perl's L<signatures|perlsub/Signatures> and L<prototypes|perlsub/Prototypes>
obviously don't contain type info so we use this attribute to define advisory
argument and return types.

=back

Finally, we just call our affixed function. Positional parameters are passed
through and any result is returned according to the given type. Here, we return
nothing because our signature claims the function returns C<Void>.

To avoid banging your head on a built-in function, you may name your sub
anything else and let Affix know what symbol to attach:

    sub my_abs : Native('my_lib.dll') : Signature([Double] => Double) : Symbol('abs');
    CORE::say my_abs( -75 ); # Should print 75 if your abs is something that makes sense

This is by far the fastest way to work with this distribution but it's not by
any means the only way.

All of the following methods may be imported by name or with the C<:sugar> tag.

Note that everything here is subject to change before v1.0.

=head1 C<attach( ... )>

Wraps a given symbol in a named perl sub.

    Dyn::attach('C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );

=head1 C<wrap( ... )>

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

=head1 Signatures

C<dyncall> uses an almost C<pack>-like syntax to define signatures which is
simple and powerful but Affix is inspired by L<Type::Standard>. See
L<Affix::Types> for more.

=head1 Library paths and names

The C<Native> attribute, C<attach( ... )>, and C<wrap( ... )> all accept the
library name, the full path, or a subroutine returning either of the two. When
using the library name, the name is assumed to be prepended with lib and
appended with C<.so> (or just appended with C<.dll> on Windows), and will be
searched for in the paths in the C<LD_LIBRARY_PATH> (C<PATH> on Windows)
environment variable.

    use Affix;
    use constant LIBMYSQL => 'mysqlclient';
    use constant LIBFOO   => '/usr/lib/libfoo.so.1';
    sub LIBBAR {
        my $opt = $^O =~ /bsd/ ? 'r' : 'p';
        my ($path) = qx[ldconfig -$opt | grep libbar];
        return $1;
    }
    # and later
    sub mysql_affected_rows :Native(LIBMYSQL);
    sub bar :Native(LIBFOO);
    sub baz :Native(LIBBAR);

You can also put an incomplete path like C<'./foo'> and Affix will
automatically put the right extension according to the platform specification.
If you wish to suppress this expansion, simply pass the string as the body of a
block.

###### TODO: disable expansion with a block!

    sub bar :Native({ './lib/Non Standard Naming Scheme' });

B<BE CAREFUL>: the C<:Native> attribute and constant are evaluated at compile
time. Don't write a constant that depends on a dynamic variable like:

    # WRONG:
    use constant LIBMYSQL => $ENV{LIB_MYSQLCLIENT} // 'mysqlclient';

=head2 ABI/API version

If you write C<:Native('foo')>, Affix will search C<libfoo.so> under Unix like
system (C<libfoo.dynlib> on macOS, C<foo.dll> on Windows). In most modern
system it will require you or the user of your module to install the
development package because it's recommended to always provide an API/ABI
version to a shared library, so C<libfoo.so> ends often being a symbolic link
provided only by a development package.

To avoid that, the native trait allows you to specify the API/ABI version. It
can be a full version or just a part of it. (Try to stick to Major version,
some BSD code does not care for Minor.)

    use Affix;
    sub foo1 :Native('foo', v1); # Will try to load libfoo.so.1
    sub foo2 :Native('foo', v1.2.3); # Will try to load libfoo.so.1.2.3

    my $lib = ['foo', 'v1'];
    sub foo3 :Native($lib);

=head2 Calling into the standard library

If you want to call a function that's already loaded, either from the standard
library or from your own program, you can omit the library value or pass and
explicit C<undef>.

For example on a UNIX-like operating system, you could use the following code
to print the home directory of the current user:


    use Affix;
    typedef PwStruct => Struct [
        name  => Str,     # username
        pass  => Str,     # hashed pass if shadow db isn't in use
        uuid  => UInt,    # user
        guid  => UInt,    # group
        gecos => Str,     # real name
        dir   => Str,     # ~/
        shell => Str      # bash, etc.
    ];
    sub getuid : Native : Signature([]=>Int);
    sub getpwuid : Native : Signature([Int]=>Pointer[PwStruct]);
    my $data = main::getpwuid( getuid() );
    use Data::Dumper;
    print Dumper( Affix::ptr2sv( PwStruct(), $data ) );

=head1 Memory Functions

To help toss raw data around, some standard memory related functions are
exposed here. You may import them by name or with the C<:memory> or C<:all>
tags.

=head2 C<malloc( ... )>

    my $ptr = malloc( $size );

Allocates L<$size> bytes of uninitialized storage.

=head2 C<calloc( ... )>

    my $ptr = calloc( $num, $size );

Allocates memory for an array of C<$num> objects of C<$size> and initializes
all bytes in the allocated storage to zero.

=head2 C<realloc( ... )>

    $ptr = realloc( $ptr, $new_size );

Reallocates the given area of memory. It must be previously allocated by
C<malloc( ... )>, C<calloc( ... )>, or C<realloc( ... )> and not yet freed with
a call to C<free( ... )> or C<realloc( ... )>. Otherwise, the results are
undefined.

=head2 C<free( ... )>

    free( $ptr );

Deallocates the space previously allocated by C<malloc( ... )>, C<calloc( ...
)>, or C<realloc( ... )>.

=head2 C<memchr( ... )>

    memchr( $ptr, $ch, $count );

Finds the first occurrence of C<$ch> in the initial C<$count> bytes (each
interpreted as unsigned char) of the object pointed to by C<$ptr>.

=head2 C<memcmp( ... )>

    my $cmp = memcmp( $lhs, $rhs, $count );

Compares the first C<$count> bytes of the objects pointed to by C<$lhs> and
C<$rhs>. The comparison is done lexicographically.

=head2 C<memset( ... )>

    memset( $dest, $ch, $count );

Copies the value C<$ch> into each of the first C<$count> characters of the
object pointed to by C<$dest>.

=head2 C<memcpy( ... )>

    memcpy( $dest, $src, $count );

Copies C<$count> characters from the object pointed to by C<$src> to the object
pointed to by C<$dest>.

=head2 C<memmove( ... )>

    memmove( $dest, $src, $count );

Copies C<$count> characters from the object pointed to by C<$src> to the object
pointed to by C<$dest>.

=head2 C<sizeof( ... )>

    my $size = sizeof( Int );
    my $size1 = sizeof( Struct[ name => Str, age => Int ] );

Returns the size, in bytes, of the L<type|/Types> passed to it.

=head1 Types

While Raku offers a set of native types with a fixed, and known, representation
in memory but this is Perl so we need to do the work ourselves and design and
build a pseudo-type system. Affix supports the fundamental types (void, int,
etc.) and aggregates (struct, array, union).

=head2 Fundamental Types with Native Representation


    Affix       C99/C++     Rust    C#          pack()  Raku
    -----------------------------------------------------------------------
    Void        void/NULL   ->()    void/NULL   -
    Bool        _Bool       bool    bool        -       bool
    Char        int8_t      i8      sbyte       c       int8
    UChar       uint8_t     u8      byte        C       byte, uint8
    Short       int16_t     i16     short       s       int16
    UShort      uint16_t    u16     ushort      S       uint16
    Int         int32_t     i32     int         i       int32
    UInt        uint32_t    u32     uint        I       uint32
    Long        int64_t     i64     long        l       int64, long
    ULong       uint64_t    u64     ulong       L       uint64, ulong
    LongLong    -           i128                q       longlong
    ULongLong   -           u128                Q       ulonglong
    Float       float       f32                 f       num32
    Double      double      f64                 d       num64
    SSize_t     SSize_t                                 SSize_t
    Size_t      size_t                                  size_t
    Str         char *

Given sizes are minimums measured in bits

=head3 C<Void>

The C<Void> type corresponds to the C C<void> type. It is generally found in
typed pointers representing the equivalent to the C<void *> pointer in C.

    sub malloc :Native :Signature([Size_t] => Pointer[Void]);
    my $data = malloc( 32 );

As the example shows, it's represented by a parameterized C<Pointer[ ... ]>
type, using as parameter whatever the original pointer is pointing to (in this
case, C<void>). This role represents native pointers, and can be used wherever
they need to be represented in a Perl script.

In addition, you may place a C<Void> in your signature to skip a passed
argument.

=head3 C<Bool>

Boolean type may only have room for one of two values: C<true> or C<false>.

=head3 C<Char>

Signed character. It's guaranteed to have a width of at least 8 bits.

Pointers (C<Pointer[Char]>) might be better expressed with a C<Str>.

=head3 C<UChar>

Unsigned character. It's guaranteed to have a width of at least 8 bits.

=head3 C<Short>

Signed short integer. It's guaranteed to have a width of at least 16 bits.

=head3 C<UShort>

Unsigned short integer. It's guaranteed to have a width of at least 16 bits.

=head3 C<Int>

Basic signed integer type.

It's guaranteed to have a width of at least 16 bits. However, on 32/64 bit
systems it is almost exclusively guaranteed to have width of at least 32 bits.

=head3 C<UInt>

Basic unsigned integer type.

It's guaranteed to have a width of at least 16 bits. However, on 32/64 bit
systems it is almost exclusively guaranteed to have width of at least 32 bits.

=head3 C<Long>

Signed long integer type. It's guaranteed to have a width of at least 32 bits.

=head3 C<ULong>

Unsigned long integer type. It's guaranteed to have a width of at least 32
bits.

=head3 C<LongLong>

Signed long long integer type. It's guaranteed to have a width of at least 64
bits.

=head3 C<ULongLong>

Unsigned long long integer type. It's guaranteed to have a width of at least 64
bits.

=head3 C<Float>

L<Single precision floating-point
type|https://en.wikipedia.org/wiki/Single-precision_floating-point_format>.

=head3 C<Double>

L<Double precision floating-point
type|https://en.wikipedia.org/wiki/Double-precision_floating-point_format>.

=head3 C<SSize_t>

=head3 C<Size_t>

=head2 C<Str>

Automatically handle null terminated character pointers with this rather than
trying to defined a parameterized C<Pointer[...]> type like as C<Pointer[Char]>
and doing it yourself.

You'll learn a bit more about parameterized types in the next section.

=head1 Parameterized Types

Some types must be provided with more context data.

=head2 C<Pointer[ ... ]>

Create pointers to (almost) all other defined types including C<Struct> and
C<Void>.

To handle a pointer to an object, see L<InstanceOf>.

Void pointers (C<Pointer[Void]>) might be created with C<malloc> and other
memory related functions.

=head2 C<Aggregate>

This is currently undefined and reserved for possible future use.

=head2 C<Struct[ ... ]>

A struct is a type consisting of a sequence of members whose storage is
allocated in an ordered sequence (as opposed to C<Union>, which is a type
consisting of a sequence of members whose storage overlaps).

A C struct that looks like this:

    struct {
        char *make;
        char *model;
        int   year;
    };

...would be defined this way:

    Struct[
        make  => Str,
        model => Str,
        year  => Int
    ];

=head2 C<ArrayRef[ ... ]>

The elements of the array must pass the additional constraint. For example
C<ArrayRef[Int]> should be a reference to an array of numbers.

An array length must be given:

    ArrayRef[Int, 5];   # int arr[5]
    ArrayRef[Any, 20];  # SV * arr[20]
    ArrayRef[Char, 5];  # char arr[5]
    ArrayRef[Str, 10];  # char *arr[10]

=head2 C<Union[ ... ]>

A union is a type consisting of a sequence of members whose storage overlaps
(as opposed to C<Struct>, which is a type consisting of a sequence of members
whose storage is allocated in an ordered sequence).

The value of at most one of the members can be stored in a union at any one
time and the union is only as big as necessary to hold its largest member
(additional unnamed trailing padding may also be added). The other members are
allocated in the same bytes as part of that largest member.

A C union that looks like this:

    union {
        char  c[5];
        float f;
    };

...would be defined this way:

    Union[
        c => ArrayRef[Char, 5],
        f => Float
    ];

=head2 C<CodeRef[ ... ]>

A value where C<ref($value)> equals C<CODE>.

The argument list and return value must pass the additional constraint. For
example, C<CodeRef[[Int, Int]=>Int]> C<typedef int (*fuc)(int a, int b);>; that
is function that accepts two integers and returns an integer.

    CodeRef[[] => Void]; # typedef void (*function)();
    CodeRef[[Pointer[Int]] => Int]; # typedef Int (*function)(int * a);
    CodeRef[[Str, Int] => Struct[...]]; # typedef struct Person (*function)(chat * name, int age);

=head2 C<InstanceOf[ ... ]>

=head2 C<Any>

Anything you dump here will be passed along unmodified. We hand off whatever
C<SV*> perl gives us without copying it.

=head2 C<Enum[ ... ]>

The value of an C<Enum> is defined by its underlying type which includes
C<Int>, C<Char>, etc.

This type is declared with an list of strings.

    Enum[ 'ALPHA', 'BETA' ];
    # ALPHA = 0
    # BETA  = 1

Unless an enumeration constant is defined in an array reference, its value is
the value one greater than the value of the previous enumerator in the same
enumeration. The value of the first enumerator (if it is not defined) is zero.

    Enum[ 'A', 'B', [C => 10], 'D', [E => 1], 'F', [G => 'F + C'] ];
    # A = 0
    # B = 1
    # C = 10
    # D = 11
    # E = 1
    # F = 2
    # G = 12

    Enum[ [ one => 'a' ], 'two', [ 'three' => 'one' ] ]
    # one   = a
    # two   = b
    # three = a

Additionally, if you C<typedef> the enum into a given namespace, you may refer
to elements by name:

    typedef color => Enum[ 'RED', 'GREEN', 'BLUE' ];
    print color::RED();     # RED
    print int color::RED(); # 0

=head2 C<IntEnum[ ... ]>

Same as C<Enum>.

=head2 C<UIntEnum[ ... ]>

C<Enum> but with unsigned integers.

=head2 C<CharEnum[ ... ]>

C<Enum> but with signed chars.

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
ppc64 sparc sparc64 co-existing varargs variadic struct enum eXtension

=end stopwords

=cut
