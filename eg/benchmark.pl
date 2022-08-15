use strict;
use warnings;
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib';
use Dyn qw[:sugar];
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
sub sin_override : Native({$libfile}) : Signature('(d)d') : Symbol('sin') {...}
sub sin_ : Native({$libfile}) : Signature('(d)d') : Symbol('sin');
sub sin_var : Native({$libfile}) : Signature('(_:d)d') : Symbol('sin');
sub sin_ell : Native({$libfile}) : Signature('(_.d)d') : Symbol('sin');
sub sin_cdecl : Native({$libfile}) : Signature('(_cd)d') : Symbol('sin');
sub sin_std : Native({$libfile}) : Signature('(_sd)d') : Symbol('sin');
sub sin_fc : Native({$libfile}) : Signature('(_fd)d') : Symbol('sin');
sub sin_tc : Native({$libfile}) : Signature('(_#d)d') : Symbol('sin');
#
my $sin_default  = Dyn::wrap( $libfile, 'sin', 'd)d' );
my $sin_vararg   = Dyn::wrap( $libfile, 'sin', '_:d)d' );
my $sin_ellipsis = Dyn::wrap( $libfile, 'sin', '_.d)d' );
my $sin_cdecl    = Dyn::wrap( $libfile, 'sin', '_cd)d' );
my $sin_stdcall  = Dyn::wrap( $libfile, 'sin', '_sd)d' );
my $sin_fastcall = Dyn::wrap( $libfile, 'sin', '_fd)d' );
my $sin_thiscall = Dyn::wrap( $libfile, 'sin', '_#d)d' );
#
Dyn::attach( $libfile, 'sin', '(d)d',   '_attach_sin_default' );
Dyn::attach( $libfile, 'sin', '(_:d)d', '_attach_sin_var' );
Dyn::attach( $libfile, 'sin', '(_.d)d', '_attach_sin_ellipse' );
Dyn::attach( $libfile, 'sin', '(_cd)d', '_attach_sin_cdecl' );
Dyn::attach( $libfile, 'sin', '(_sd)d', '_attach_sin_std' );
Dyn::attach( $libfile, 'sin', '(_fd)d', '_attach_sin_fc' );
Dyn::attach( $libfile, 'sin', '(_#d)d', '_attach_sin_tc' );
#
my $ffi = FFI::Platypus->new( api => 1 );
$ffi->lib($libfile);
my $ffi_func = $ffi->function( sin => ['double'] => 'double' );
$ffi->attach( [ sin => 'ffi_sin' ] => ['double'] => 'double' );

# prime the pump
{
    my $sin = sin 500;
    die 'oops' if sin_override(500) != $sin;
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
            sub_sin_override => sub {
                my $x = 0;
                while ( $x < $depth ) { my $n = sin_override($x); $x++ }
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
