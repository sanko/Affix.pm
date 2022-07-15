use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Dyn qw[:all];
use File::Spec;
use t::lib::nativecall;
$|++;
#
compile_test_lib('40_misc');
#
package misc {
    use Dyn qw[:sugar];
    sub NCstrlen : Native('t/40_misc') : Signature('(Z)i') {...}
    sub test ()                                            { NCstrlen(123) }
}
is misc::test(), 3, 'body of a native sub declared in a package replaced';

BEGIN {
    sub NCstrlen : Native('t/40_misc') : Signature('(Z)i') {...}
}
is NCstrlen(123), 3, 'body of a native sub declared in a BEGIN block replaced';

# There is no inline algo for native in perl 5 so the other tests are pointless
done_testing;
__END__
use strict;
use Test::More 0.98;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Dyn qw[:sugar];
use File::Spec;
use experimental 'signatures';
$|++;
#
#my $lib;

chdir '..' unless -d './t';

# Build a library
use ExtUtils::CBuilder;
use File::Spec;
my ( $source_file, $object_file, $lib_file );
my $b = ExtUtils::CBuilder->new( quiet => 0 );
ok $b, 'created EU::CB object';

sub _build ( $source, $name, @exports ) {
    subtest 'build ' . $name => sub {

        $source_file
            = File::Spec->rel2abs( File::Spec->catfile( ( -d 't' ? 't' : '.' ), $name . '.cpp' ) );
        {
            open my $FH, '>', $source_file or die "Can't create $source_file: $!";
            diag '$source_file == ' . $source_file;
            printf $FH <<'END', $source; close $FH;
#if defined(_WIN32) || defined(__WIN32__)
#  define LIB_EXPORT extern "C" __declspec(dllexport)
#else
#  define LIB_EXPORT extern "C"
#endif

%s
END
        }
        ok -e $source_file, "generated '$source_file'";

        # Compile
        eval { $object_file = $b->compile( source => $source_file, 'C++' => 1 ) };
        is $@, q{}, 'no exception from compilation';
        ok -e $object_file, 'found object file';

        # Link
    SKIP: {
            plan skip_all => 'error compiling source' unless -e $object_file;
            $lib_file = $b->lib_file($object_file);
            $lib_file =~ s[\.xs\.][.];
            diag $lib_file;
            my @temps;
            eval {
                #$b->prelink(  );
                diag 'Exporting: ' . join ', ', @exports;
                ( $lib_file, @temps ) = $b->link(
                    lib_file     => $lib_file,
                    objects      => $object_file,
                    module_name  => 't::hello',
                    dl_func_list => [@exports]
                );
            };
            is $@, q{}, 'no exception from linking';
            ok -e $lib_file, 'found library';
            diag '$lib_file: ' . $lib_file;
            if ( $^O eq 'os2' ) {    # Analogue of LDLOADPATH...

                # Actually, not needed now, since we do not link with the generated DLL
                my $old = OS2::extLibpath();    # [builtin function]
                $old = ";$old" if defined $old and length $old;

                # To pass the sanity check, components must have backslashes...
                OS2::extLibpath_set(".\\$old");
            }
        }
    };
    return $lib_file;
}

sub _cleanup {return;
    subtest 'cleanup' => sub {
        for ( grep { defined && -e } $source_file, $object_file, $lib_file ) {
            tr/"'//d;
            1 while unlink;
            pass 'deleted ' . $_;
        }
        if ( $^O eq 'VMS' ) {
            1 while unlink 'LINKT.LIS';
            1 while unlink 'LINKT.OPT';
            pass 'VMS cleanup';
        }
    }
}
subtest '01-argless' => sub {
    my $lib = _build <<'END', 'argless', qw[Nothing]; #qw[Nothing Argless ArglessChar ArglessLongLong ArglessPointer ArglessUTF8String long_and_complicated_name];
#include <stdio.h>
#include <string.h>

LIB_EXPORT void Nothing( ) { return; }
/*
LIB_EXPORT int Argless( ) { return 2; }
LIB_EXPORT char ArglessChar( ) { return 2; }
LIB_EXPORT long long ArglessLongLong( ) { return 2; }
int my_int = 2;
LIB_EXPORT int * ArglessPointer( ) { return &my_int; }
char * my_str = "Just a string";
LIB_EXPORT char * ArglessUTF8String( ) { return my_str; }
LIB_EXPORT int long_and_complicated_name( ) { return 3; }
*/
END

    diag $lib;

    sub Nothing : Native('t/argless');
    #sub Argless : Native('t/argless') : Signature('()i');
    #sub ArglessChar : Native('t/argless') : Signature('()c');
    #sub ArglessLongLong : Native('t/argless') : Signature('()l');
    #sub ArglessPointer : Native('t/argless') : Signature('()p');    #returns Pointer[int32] { * }
    #sub ArglessUTF8String : Native('t/argless') : Signature('()Z');
    #sub short : Native('t/argless') : Symbol('long_and_complicated_name') : Signature('()i');
    #
    diag $lib;
    diag -s $lib;
    diag Dyn::guess_library_name('t/argless');
    diag `nm t/argless.o` if $^O eq 'MSWin32';

    Nothing();
    pass 'survived the call';
    #is Argless(),           2,               'called argless function returning int32';
    #is ArglessChar(),       2,               'called argless function returning char';
    #is ArglessLongLong(),   2,               'called argless function returning long long';
    #is ArglessPointer(),    pack( 'C', 2 ),  'called argless function returning pointer';
    #is ArglessUTF8String(), 'Just a string', 'called argless function returning string';
    #
    #is short(), 3, 'called long_and_complicated_name';

    #sub test_native_closure() {
    #    my sub Argless : Native('t/argless') : Signature('()i') : Symbol('Argless');
    #    is Argless(), 2, 'called argless closure';
    #}
    #test_native_closure();
    #
    _cleanup();
    #
};
subtest '02-simple-args' => sub {
    my @exports = qw[
        TakeInt AssortedIntArgs TakeTwoShorts AssortedIntArgs TakeADouble TakeAFloat TakeAString SetString CheckString wrapped
        TakeInt64 TakeUint8 TakeUint16 TakeUint32 TakeSizeT
        TakeSSizeT];
    my $lib = _build <<'END', 'simple_args', @exports;
#include <stdio.h>
#include <string.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
typedef signed __int64 int64_t;
#else
#include <inttypes.h>
#include <sys/types.h>
#endif

LIB_EXPORT int TakeInt(int x) {
    if (x == 42) return 1;
    return 0;
}

LIB_EXPORT int TakeTwoShorts(short x, short y) {
    if (x == 10 && y == 20) return 2;
    return 0;
}

LIB_EXPORT int AssortedIntArgs(int x, short y, char z) {
    if (x == 101 && y == 102 && z == 103) return 3;
    return 0;
}

LIB_EXPORT int TakeADouble(double x) {
    if (-6.9 - x < 0.001) return 4;
    return 0;
}

LIB_EXPORT int TakeAFloat(float x) {
    if (4.2 - x < 0.001) return 5;
    return 0;
}

LIB_EXPORT int TakeAString(char *pass_msg) {
    if (0 == strcmp(pass_msg, "ok 6 - passed a string")) return 6;
    return 0;
}

static char *cached_str = NULL;
LIB_EXPORT void SetString(char *str) { cached_str = str; }

LIB_EXPORT int CheckString() {
    if (0 == strcmp(cached_str, "ok 7 - checked previously passed string")) return 7;
    return 0;
}

LIB_EXPORT int wrapped(int n) {
    if (n == 42) return 8;
    return 0;
}

LIB_EXPORT int TakeInt64(int64_t x) {
    if (x == 0xFFFFFFFFFF) return 9;
    return 0;
}

LIB_EXPORT int TakeUint8(unsigned char x) {
    if (x == 0xFE) return 10;
    return 0;
}

LIB_EXPORT int TakeUint16(unsigned short x) {
    if (x == 0xFFFE) return 11;
    return 0;
}

LIB_EXPORT int TakeUint32(unsigned int x) {
    if (x == 0xFFFFFFFE) return 12;
    return 0;
}

LIB_EXPORT int TakeSizeT(size_t x) {
    if (x == 42) return 13;
    return 0;
}

LIB_EXPORT int TakeSSizeT(ssize_t x) {
    if (x == -42) return 14;
    return 0;
}
END

    # Int related
    sub TakeInt : Native('t/simple_args') : Signature('(i)i');
    sub TakeTwoShorts : Native('t/simple_args') : Signature('(ss)i');
    sub AssortedIntArgs : Native('t/simple_args') : Signature('(jsc)i');
    is TakeInt(42),                      1, 'passed int 42';
    is TakeTwoShorts( 10, 20 ),          2, 'passed two shorts';
    is AssortedIntArgs( 101, 102, 103 ), 3, 'passed an int32, int16 and int8';

    # Float related
    sub TakeADouble : Native('t/simple_args') : Signature('(d)j');
    sub TakeAFloat : Native('t/simple_args') : Signature('(f)j');
    is TakeADouble(-6.9e0), 4, 'passed a double';
    is TakeAFloat(4.2e0),   5, 'passed a float';

    # String related
    sub TakeAString : Native('t/simple_args') : Signature('(Z)j');
    is TakeAString('ok 6 - passed a string'), 6, 'passed a string';

    # Explicitly managing strings
    sub SetString : Native('t/simple_args') : Signature('(Z)j');
    sub CheckString : Native('t/simple_args') : Signature('()j');
    my $str = 'ok 7 - checked previously passed string';

    #explicitly_manage($str); # https://docs.raku.org/routine/explicitly-manage
    SetString($str);
    is CheckString(), 7, 'checked previously passed string';

    # Make sure wrapped subs work
    sub wrapped : Native('t/simple_args') : Signature('(j)j');
    sub wrapper ($arg) { is wrapped($arg), 8, 'wrapped sub' }
    wrapper(42);

    # 64-bit integer
    sub TakeInt64 : Native('t/simple_args') : Signature('(l)l');
    {
        no warnings 'portable';
        is TakeInt64(0xFFFFFFFFFF), 9, 'passed int64 0xFFFFFFFFFF';
    }

    # Unsigned integers.
    sub TakeUint8 : Native('t/simple_args') : Signature('(C)j');
    sub TakeUint16 : Native('t/simple_args') : Signature('(S)j');
    sub TakeUint32 : Native('t/simple_args') : Signature('(L)j');
SKIP: {
        skip 'Cannot test TakeUint8(0xFE) on OS X with -O3', 1 if $^O eq 'darwin';
        #
        # For some reason, on OS X with clang, the following test fails with -O3
        # specified.  One can only assume this is some weird compiler issue (tested
        # on Apple LLVM version 6.1.0 (clang-602.0.49) (based on LLVM 3.6.0svn).
        #
        is TakeUint8(0xFE), 10, 'passed uint8 0xFE';
    }

    # R#2124 https://github.com/rakudo/rakudo/issues/2124
    #skip("Cannot test TakeUint16(0xFFFE) with clang without -O0");
    is TakeUint16(0xFFFE),     11, 'passed uint16 0xFFFE';
    is TakeUint32(0xFFFFFFFE), 12, 'passed uint32 0xFFFFFFFE';
    sub TakeSizeT : Native('t/simple_args') : Signature('(i)j');
    is TakeSizeT(42), 13, 'passed size_t 42';
    sub TakeSSizeT : Native('t/simple_args') : Signature('(I)j');
    is TakeSSizeT(-42), 14, 'passed ssize_t -42';
    #
    _cleanup();
    #
};
done_testing;
