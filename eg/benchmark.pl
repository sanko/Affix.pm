use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Affix     qw[:all];
use Dyn::Load qw[:all];
use Dyn::Call qw[:all];
use FFI::Platypus 1.58;
use Config;
use Benchmark qw[cmpthese timethese :hireswallclock];

# arbitrary benchmarks
$|++;
our $libfile
    = $^O eq 'MSWin32' ? 'ntdll.dll' :
    $^O eq 'darwin'    ? '/usr/lib/libm.dylib' :
    $^O eq 'bsd'       ? '/usr/lib/libm.so' :
    $Config{archname} =~ /64/ ?
    -e '/lib64/libm.so.6' ?
    '/lib64/libm.so.6' :
        '/lib/x86_64-linux-gnu/libm.so.6' :
    '/lib/libm.so.6';
#
sub sin_ : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin');
sub sin_var : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin') :
    Mode(DC_SIGCHAR_CC_ELLIPSIS_VARARGS);
sub sin_ell : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin') :
    Mode(DC_SIGCHAR_CC_ELLIPSIS);
sub sin_cdecl : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin') :
    Mode(DC_SIGCHAR_CC_CDECL);
sub sin_std : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin') :
    Mode(DC_SIGCHAR_CC_STDCALL);
sub sin_fc : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin') :
    Mode(DC_SIGCHAR_CC_FASTCALL_GNU);
sub sin_tc : Native($main::libfile) : Signature([Double]=>Double) : Symbol('sin') :
    Mode(DC_SIGCHAR_CC_THISCALL_GNU);
#
my $sin_default  = wrap( $libfile, 'sin', [Double] => Double );
my $sin_vararg   = wrap( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_ELLIPSIS_VARARGS );
my $sin_ellipsis = wrap( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_ELLIPSIS );
my $sin_cdecl    = wrap( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_CDECL );
my $sin_stdcall  = wrap( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_STDCALL );
my $sin_fastcall = wrap( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_FASTCALL_GNU );
my $sin_thiscall = wrap( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_THISCALL_GNU );
#
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_DEFAULT, '_attach_sin_default' );
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_ELLIPSIS_VARARGS, '_attach_sin_var' );
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_ELLIPSIS,     '_attach_sin_ellipse' );
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_CDECL,        '_attach_sin_cdecl' );
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_STDCALL,      '_attach_sin_std' );
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_FASTCALL_GNU, '_attach_sin_fc' );
attach( $libfile, 'sin', [Double] => Double, DC_SIGCHAR_CC_THISCALL_GNU, '_attach_sin_tc' );
#
my $ffi = FFI::Platypus->new( api => 1 );
$ffi->lib($libfile);
my $ffi_func = $ffi->function( sin => ['double'] => 'double' );
$ffi->attach( [ sin => 'ffi_sin' ] => ['double'] => 'double' );

# prime the pump
my $sin = sin 500;
{
    die 'oops' if sin_(500) != $sin;
    die 'oops' if sin_var(500) != $sin;
    die 'oops' if sin_ell(500) != $sin;
    die 'oops' if sin_std(500) != $sin;
    die 'oops' if sin_fc(500) != $sin;
    die 'oops' if sin_tc(500) != $sin;
    die 'oops' if $sin_default->(500) != $sin;
    die 'oops' if $sin_vararg->(500) != $sin;
    die 'oops' if $sin_ellipsis->(500) != $sin;
    die 'oops' if $sin_cdecl->(500) != $sin;
    die 'oops' if $sin_stdcall->(500) != $sin;
    die 'oops' if $sin_fastcall->(500) != $sin;
    die 'oops' if $sin_thiscall->(500) != $sin;
    die 'oops' if _attach_sin_default(500) != $sin;
    die 'oops' if _attach_sin_var(500) != $sin;
    die 'oops' if _attach_sin_ellipse(500) != $sin;
    die 'oops' if _attach_sin_cdecl(500) != $sin;
    die 'oops' if _attach_sin_std(500) != $sin;
    die 'oops' if _attach_sin_fc(500) != $sin;
    die 'oops' if _attach_sin_tc(500) != $sin;
    die 'oops' if ffi_sin(500) != $sin;
    die 'oops' if $ffi_func->(500) != $sin;
    die 'oops' if $ffi_func->call(500) != $sin;
}
#
my $depth = 1000;
cmpthese(
    timethese(
        -10,
        {   perl => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin($x); $x++ }
            },
            attach_sin_default => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_default($x); $x++ }
            },
            attach_sin_var => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_var($x); $x++ }
            },
            attach_sin_ellipse => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_ellipse($x); $x++ }
            },
            attach_sin_cdecl => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_cdecl($x); $x++ }
            },
            attach_sin_std => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_std($x); $x++ }
            },
            attach_sin_fc => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_fc($x); $x++ }
            },
            attach_sin_tc => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = _attach_sin_tc($x); $x++ }
            },
            sub_sin_default => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_($x); $x++ }
            },
            sub_sin_var => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_var($x); $x++ }
            },
            sub_sin_ell => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_ell($x); $x++ }
            },
            sub_sin_cdecl => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_cdecl($x); $x++ }
            },
            sub_sin_std => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_std($x); $x++ }
            },
            sub_sin_fc => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_fc($x); $x++ }
            },
            sub_sin_tc => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_tc($x); $x++ }
            },
            call_default => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_default->($x); $x++ }
            },
            call_vararg => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_vararg->($x); $x++ }
            },
            call_ellipsis => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_ellipsis->($x); $x++ }
            },
            call_cdecl => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_cdecl->($x); $x++ }
            },
            call_stdcall => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_stdcall->($x); $x++ }
            },
            call_fastcall => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_fastcall->($x); $x++ }
            },
            call_thiscall => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $sin_thiscall->($x); $x++ }
            },
            ffi_attach => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = ffi_sin($x); $x++ }
            },
            ffi_function => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $ffi_func->($x); $x++ }
            },
            ffi_function_call => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = $ffi_func->call($x); $x++ }
            }
        }
    )
);
