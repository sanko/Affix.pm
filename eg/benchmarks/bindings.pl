use v5.40;
use blib;
use Benchmark qw[cmpthese];
use Config;
use ExtUtils::CBuilder ();
use ExtUtils::ParseXS  ();
use File::Path         qw[make_path];
use File::Temp         qw[tempdir];
use Affix              qw[:all];
$|++;

=pod

=head1 NAME

eg/benchmarks/bindings.pl - compare six ways to call C from Perl

=head1 SYNOPSIS

    perl Build.PL && ./Build
    perl -Mblib eg/benchmarks/bindings.pl

Set C<AFFIX_BENCH_SECONDS> to change the time spent per implementation (default: five CPU seconds).

=head1 DESCRIPTION

Calls C<sin(double) -E<gt> double> from libm/libc six ways:

=over 4

=item * Affix C<affix(...)> and C<wrap(...)>

=item * Inline::C

=item * a hand-written XSUB compiled with L<ExtUtils::ParseXS> and L<ExtUtils::CBuilder>

=item * FFI::Platypus C<attach(...)> and C<function(...)>

=back

The XSUB and Inline::C cases are compiled into a temporary directory at startup. Both inherit Perl's own
C<$Config{optimize}> (normally C<-O2>) through L<ExtUtils::CBuilder> and Inline::C, so neither native path gets
special optimization flags. All six results are checked against Perl's built-in C<sin> before timing, so a broken
binding can't silently skew the numbers.

=cut

my $SECONDS = $ENV{AFFIX_BENCH_SECONDS} // 5;
die "AFFIX_BENCH_SECONDS must be a positive integer\n" unless $SECONDS =~ /^\d+$/ && $SECONDS > 0;
require Inline;
require Inline::C;
require FFI::Platypus;

#~ Windows keeps the generated DLL mapped until process exit, so leave the
#~ directory behind there rather than warning during File::Temp's cleanup.
my $tmp = tempdir( CLEANUP => ( $^O eq 'MSWin32' ? 0 : 1 ) );

sub build_xs ($dir) {
    my $xs = "$dir/BenchXS.xs";
    open my $fh, '>', $xs or die "Cannot write $xs: $!";
    print {$fh} <<'END_XS';
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <math.h>

MODULE = BenchXS    PACKAGE = BenchXS

PROTOTYPES: DISABLE

double
xs_sin(x)
        double x
    CODE:
        RETVAL = sin(x);
    OUTPUT:
        RETVAL
END_XS
    close $fh;
    my $c = "$dir/BenchXS.c";
    ExtUtils::ParseXS::process_file( filename => $xs, output => $c, module_name => 'BenchXS' );
    my @inc  = grep {-d} map {"$_/CORE"} @Config{qw[archlib sitearch]};
    my $cb   = ExtUtils::CBuilder->new( quiet => 1 );
    my $obj  = $cb->compile( source => $c, include_dirs => \@inc );
    my $auto = "$dir/auto/BenchXS";
    make_path($auto);
    $cb->link(
        objects     => [$obj],
        lib_file    => "$auto/BenchXS.$Config{dlext}",
        module_name => 'BenchXS',

        #~ libc provides the math symbols on Windows; Unix needs libm.
        extra_link_flags => ( $^O eq 'MSWin32' ? '' : '-lm' )
    );
    unshift @INC, $dir;
    require DynaLoader;
    {

        package BenchXS;
        our @ISA = ('DynaLoader');
        sub dl_load_flags {0}
    }
    BenchXS->bootstrap;
    return \&BenchXS::xs_sin;
}
my $affix_sin = wrap( libm, 'sin', [Double] => Double );
affix libm, [ sin => 'affix_sin' ], [Double] => Double;
my $xs_sin = build_xs($tmp);
Inline->bind( C => <<'END_C' );
#include <math.h>
double inline_sin(double x) { return sin(x); }
END_C
my $inline_sin = \&main::inline_sin;

#~ undef searches the running perl executable and the C library it is
#~ linked against, so this works without a platform-specific libm name.
my $ffi = FFI::Platypus->new( api => 2, lib => undef );
$ffi->attach( [ sin => 'platypus_sin' ] => ['double'] => 'double' );
my $platypus_function = $ffi->function( sin => ['double'] => 'double' );
my %impl              = (
    Affix_wrap               => $affix_sin,
    Affix                    => \&main::affix_sin,
    XS                       => $xs_sin,
    'Inline::C'              => $inline_sin,
    'FFI::Platypus'          => \&main::platypus_sin,
    'FFI::Platypus function' => sub { $platypus_function->call( $_[0] ) }
);
say "sin(double) -> double, $SECONDS CPU seconds per implementation";
say "perl $^V ($Config{archname}), $^O";
say "Affix $Affix::VERSION";
say "Inline $Inline::VERSION / Inline::C $Inline::C::VERSION";
say "FFI::Platypus $FFI::Platypus::VERSION";
say '';

for my $x ( 0, 0.5, 1, -2.25 ) {
    my $expected = CORE::sin($x);
    for my $name ( sort keys %impl ) {
        my $got = $impl{$name}->($x);
        die "$name: sin($x) returned $got, expected $expected\n" if abs( $got - $expected ) > 1e-12;
    }
}
say 'All six implementations agree.';
say 'We can just run benchmarks now...';
cmpthese(
    -$SECONDS,
    {   Affix              => sub { $impl{Affix}->(0.5) },
        'Affix wrap'       => sub { $impl{Affix_wrap}->(0.5) },
        XS                 => sub { $impl{XS}->(0.5) },
        'Inline::C'        => sub { $impl{'Inline::C'}->(0.5) },
        'FFI::Platypus'    => sub { $impl{'FFI::Platypus'}->(0.5) },
        'FFI::Platypus fn' => sub { $impl{'FFI::Platypus function'}->(0.5) }
    }
);
