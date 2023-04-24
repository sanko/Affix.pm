use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Affix     qw[:all];
use Dyn::Load qw[:all];
use Dyn::Call qw[/dc/];
use FFI::Platypus 1.58;
use Config;
use Benchmark qw[cmpthese timethese :hireswallclock];

# arbitrary benchmarks
$|++;
our $libfile;

BEGIN {
    $libfile
        = $^O eq 'MSWin32' ? 'ntdll.dll' :
        $^O eq 'darwin'    ? '/usr/lib/libm.dylib' :
        $^O eq 'bsd'       ? '/usr/lib/libm.so' :
        $Config{archname} =~ /64/ ?
        -e '/lib64/libm.so.6' ?
        '/lib64/libm.so.6' :
            '/lib/x86_64-linux-gnu/libm.so.6' :
        '/lib/libm.so.6';
}

sub libfile {
    return undef;
    $libfile;
}
use Inline C => config => libs => '-lm';
use Inline C => <<'...';
#include <math.h>

double inline_c_sin(double in) {
  return sin(in);
}
...

package DynFFI {
    use strictures 2;
    use feature 'signatures';
    use Config;
    use File::Spec;
    use File::Basename qw[dirname];
    use File::Find     qw[find];
    use Dyn            qw[:all];
    my $cvm = dcNewCallVM(1024);
    dcMode( $cvm, DC_CALL_C_DEFAULT );
    END { dcFree($cvm) if $cvm }

    sub obj ( $package, $ptr, $signature ) {
        my @funcs;
        my $ret = \&dcCallVoid;
        CORE::state $arg_dispatch //= {
            DC_SIGCHAR_BOOL()     => \&dcArgBool,
            DC_SIGCHAR_CHAR()     => \&dcArgChar,
            DC_SIGCHAR_SHORT()    => \&dcArgShort,
            DC_SIGCHAR_INT()      => \&dcArgInt,
            DC_SIGCHAR_LONG()     => \&dcArgLong,
            DC_SIGCHAR_LONGLONG() => \&dcArgLongLong,
            DC_SIGCHAR_FLOAT()    => \&dcArgFloat,
            DC_SIGCHAR_DOUBLE()   => \&dcArgDouble,
            DC_SIGCHAR_POINTER()  => \&dcArgPointer,
            DC_SIGCHAR_STRING()   => \&dcArgString
        };
        CORE::state $ret_dispatch //= {
            DC_SIGCHAR_VOID()     => \&dcCallVoid,
            DC_SIGCHAR_BOOL()     => \&dcCallBool,
            DC_SIGCHAR_CHAR()     => \&dcCallChar,
            DC_SIGCHAR_SHORT()    => \&dcCallShort,
            DC_SIGCHAR_INT()      => \&dcCallInt,
            DC_SIGCHAR_LONG()     => \&dcCallLong,
            DC_SIGCHAR_LONGLONG() => \&dcCallLongLong,
            DC_SIGCHAR_FLOAT()    => \&dcCallFloat,
            DC_SIGCHAR_DOUBLE()   => \&dcCallDouble,
            DC_SIGCHAR_POINTER()  => \&dcCallPointer,
            DC_SIGCHAR_STRING()   => \&dcCallString
        };
        my @chars = split //, $signature;
        while ( my $sigchar = shift @chars ) {
            if ( $sigchar eq ')' ) {
                $ret = $ret_dispatch->{ shift @chars };
                last;
            }
            push @funcs, $arg_dispatch->{$sigchar};
        }
        my $thing = sub {
            dcReset($cvm);
            $funcs[$_]->( $cvm, $_[$_] ) for 0 .. $#funcs;
            $ret->( $cvm, $ptr );
        };
        bless $thing, $package;
    }

    sub load ( $path, $version = () ) {
        dlLoadLibrary( guess_library_name( $path, $version ) );
    }
    sub func ( $lib, $symbol ) { dlFindSymbol( $lib, $symbol ); }
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
}
my $hand_rolled = DynFFI->obj( DynFFI::func( DynFFI::load($libfile), 'sin' ), 'd)d' );
#
my $sin_default = wrap( $libfile, 'sin', [Double] => Double );
affix( $libfile, [ 'sin', '_affix_sin_default' ], [Double] => Double );

#~ Affix::affix_2( $libfile, [ 'sin', '_affix2_sin_default' ], [Double] => Double );
#
my $ffi = FFI::Platypus->new( api => 1 );
$ffi->lib($libfile);
my $ffi_func = $ffi->function( sin => ['double'] => 'double' );
$ffi->attach( [ sin => 'ffi_sin' ] => ['double'] => 'double' );

# prime the pump
my $sin = sin 500;
{
    die 'oops' if $hand_rolled->(500) != $sin;
    die 'oops' if $sin_default->(500) != $sin;
    die 'oops' if _affix_sin_default(500) != $sin;

    #~ die 'oops' if _affix2_sin_default(500) != $sin;
    die 'oops' if ffi_sin(500) != $sin;
    die 'oops' if $ffi_func->(500) != $sin;
    die 'oops' if inline_c_sin(500) != $sin;
}
#
my $depth = 1000;
cmpthese(
    timethese(
        -30,
        {   dyn_hand_rolled => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $hand_rolled->($x); $x++ }
            },
            perl => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin($x); $x++ }
            },
            affix_sub => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _affix_sin_default($x); $x++ }
            },

            #~ affix2_sub => sub {
            #~ my $x = 0;
            #~ while ( $x < $depth ) { my $n = _affix2_sin_default($x); $x++ }
            #~ },
            affix_coderef => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_default->($x); $x++ }
            },
            ffi_sub => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = ffi_sin($x); $x++ }
            },
            ffi_coderef => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $ffi_func->($x); $x++ }
            },
            inline_c_sin => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = inline_c_sin($x); $x++ }
            }
        }
    )
);
__END__
                  Rate ffi_coderef dyn_hand_rolled ffi_sub affix_sub affix_coderef inline_c_sin perl
ffi_coderef      601/s          --            -36%    -80%      -83%          -86%         -87% -92%
dyn_hand_rolled  944/s         57%              --    -69%      -73%          -79%         -79% -87%
ffi_sub         3037/s        405%            222%      --      -15%          -31%         -33% -58%
affix_sub       3556/s        492%            277%     17%        --          -20%         -22% -51%
affix_coderef   4425/s        637%            369%     46%       24%            --          -3% -39%
inline_c_sin    4561/s        659%            383%     50%       28%            3%           -- -37%
perl            7211/s       1100%            664%    137%      103%           63%          58%   --
