use strict;
use warnings;
use lib './lib', '../lib', '../blib/arch/', 'blib/arch', '../', '.';
use Affix               qw[wrap affix libm Double direct_wrap direct_affix];
use Test2::Tools::Affix qw[:all];
use Config;

use Test2::Require::AuthorTesting;
use Benchmark qw[:all];
$|++;

# Conditionally load FFI::Platypus and Inline::C
my ( $has_platypus, $has_inline_c );

BEGIN {
    {
        eval 'use FFI::Platypus';
        unless ($@) {
            $has_platypus = 1;
            diag 'FFI::Platypus found, including it in benchmarks.';
        }
        else {
            diag 'FFI::Platypus not found, skipping its benchmarks.';
        }
    }
    {
        eval 'use Inline;';
        unless ($@) {
            $has_inline_c = 1;
            diag 'Inline::C found, including it in benchmarks.';
        }
        else {
            diag 'Inline::C not found, skipping its benchmarks.';
        }
    }
}
my $libm = '' . libm();
diag 'libm: ' . $libm;

# FFI Setup
# Affix / Wrap setup
my $wrap_sin = wrap( $libm, 'sin', '(double)->double' );
affix( $libm, [ sin => 'affix_sin' ], '(double)->double' );
my $direct = direct_wrap( $libm, 'sin', '(double)->double' );
direct_affix( $libm, [ 'sin', 'direct_sin' ], '(double)->double' );

# FFI::Platypus setup (only if available)
my $platypus_sin;
if ($has_platypus) {
    my $ffi = FFI::Platypus->new( api => 2, lib => $libm );

    # Use find_lib with a named argument
    $ffi->attach( [ sin => 'platypus_sin' ], ['double'] => 'double' );
    $platypus_sin = $ffi->function( 'sin', ['double'] => 'double' );
}
my $inline_c_sin;
if ($has_inline_c) {
    use Inline C => <<END_OF_C;
#include <math.h>
double inline_sin(double x) { return sin(x); }
END_OF_C
}

# Verification
my $num = rand(time);
my $sin = sin $num;
diag sprintf 'sin( %f ) = %f', $num, $sin;
subtest verify => sub {
    is direct_sin($num),  float( $sin, tolerance => 0.000001 ), 'direct affix correctly calculates sin';
    is $direct->($num),   float( $sin, tolerance => 0.000001 ), 'direct wrap correctly calculates sin';
    is $wrap_sin->($num), float( $sin, tolerance => 0.000001 ), 'wrap correctly calculates sin';
    is affix_sin($num),   float( $sin, tolerance => 0.000001 ), 'affix correctly calculates sin';
    is sin($num),         float( $sin, tolerance => 0.000001 ), 'pure perl correctly calculates sin';

    # Conditionally run Platypus verification
    if ($has_platypus) {
        is $platypus_sin->($num), float( $sin, tolerance => 0.000001 ), 'platypus [function] correctly calculates sin';
        is platypus_sin($num),    float( $sin, tolerance => 0.000001 ), 'platypus [attach] correctly calculates sin';
    }
    if ($has_inline_c) {
        is inline_sin($num), float( $sin, tolerance => 0.000001 ), 'inline correctly calculates sin';
    }
};

# Benchmarks
my $depth = 20;
subtest benchmarks => sub {
    my $todo       = todo 'these are fun but not important; we will not be beating opcodes';
    my %benchmarks = (
        direct_affix => sub {
            my $x = 0;
            while ( $x < $depth ) { my $n = direct_sin($x); $x++; }
        },
        direct_wrap => sub {
            my $x = 0;
            while ( $x < $depth ) { my $n = $direct->($x); $x++; }
        },
        pure => sub {
            my $x = 0;
            while ( $x < $depth ) { my $n = sin($x); $x++ }
        },
        wrap => sub {
            my $x = 0;
            while ( $x < $depth ) { my $n = $wrap_sin->($x); $x++ }
        },
        affix => sub {
            my $x = 0;
            while ( $x < $depth ) { my $n = affix_sin($x); $x++ }
        }, (
            $has_platypus ?

                # Conditionally add Platypus benchmark
                (
                plat_f => sub {
                    my $x = 0;
                    while ( $x < $depth ) { my $n = $platypus_sin->($x); $x++ }
                },
                plat_a => sub {
                    my $x = 0;
                    while ( $x < $depth ) { my $n = platypus_sin($x); $x++ }
                }
                ) :
                ()
        ), (
            $has_inline_c ?

                # Conditionally add Inline::C benchmark
                (
                inline_c => sub {
                    my $x = 0;
                    while ( $x < $depth ) { my $n = inline_sin($x); $x++ }
                }
                ) :
                ()
        )
    );
    isnt fastest( -10, %benchmarks ), 'pure', 'The fastest method should not be pure Perl';
};

# Helper Function
# Cribbed from Test::Benchmark
sub fastest {
    my ( $times, %marks ) = @_;
    diag sprintf 'running %s for %s seconds each', join( ', ', keys %marks ), abs($times);
    my @marks;
    my $len = [ map { length $_ } keys %marks ]->[-1];
    for my $name ( sort keys %marks ) {
        my $res = timethis( $times, $marks{$name}, '', 'none' );
        my ( $r, $pu, $ps, $cu, $cs, $n ) = @$res;
        push @marks, { name => $name, res => $res, n => $n, s => ( $pu + $ps ) };
        diag sprintf '%' . ( $len + 1 ) . 's - %s', $name, timestr($res);
    }
    my $results = cmpthese {
        map { $_->{name} => $_->{res} } @marks
    }, 'none';
    my $len_1 = [ map { length $_->[1] } @$results ]->[-1];
    diag sprintf '%-' . ( $len + 1 ) . 's %' . ( $len_1 + 1 ) . 's' . ( ' %5s' x scalar keys %marks ), @$_ for @$results;
    [ sort { $b->{n} * $a->{s} <=> $a->{n} * $b->{s} } @marks ]->[0]->{name};
}
done_testing;

# Current results:
# WSL:
# running inline_c, pure, affix, direct_affix, wrap, plat_f, plat_a, direct_wrap for 10 seconds each
#        affix - 12 wallclock secs (10.61 usr +  0.00 sys = 10.61 CPU) @ 749167.39/s (n=7948666)
# direct_affix -  9 wallclock secs (10.51 usr +  0.00 sys = 10.51 CPU) @ 785234.63/s (n=8252816)
#  direct_wrap - 11 wallclock secs (10.49 usr +  0.00 sys = 10.49 CPU) @ 743024.12/s (n=7794323)
#     inline_c -  8 wallclock secs (10.41 usr +  0.00 sys = 10.41 CPU) @ 785154.85/s (n=8173462)
#       plat_a - 12 wallclock secs (10.56 usr +  0.00 sys = 10.56 CPU) @ 452524.24/s (n=4778656)
#       plat_f - 11 wallclock secs (10.46 usr +  0.00 sys = 10.46 CPU) @ 99935.28/s (n=1045323)
#         pure - 12 wallclock secs (10.67 usr +  0.00 sys = 10.67 CPU) @ 992181.82/s (n=10586580)
#         wrap - 12 wallclock secs (10.52 usr +  0.00 sys = 10.52 CPU) @ 657243.54/s (n=6914202)
#                   Rate plat_f plat_a  wrap direct_wrap affix inline_c direct_affix  pure
# plat_f         99935/s    --  -78%  -85%  -87%  -87%  -87%  -87%  -90%
# plat_a        452524/s  353%    --  -31%  -39%  -40%  -42%  -42%  -54%
# wrap          657244/s  558%   45%    --  -12%  -12%  -16%  -16%  -34%
# direct_wrap   743024/s  644%   64%   13%    --   -1%   -5%   -5%  -25%
# affix         749167/s  650%   66%   14%    1%    --   -5%   -5%  -24%
# inline_c      785155/s  686%   74%   19%    6%    5%    --   -0%  -21%
# direct_affix  785235/s  686%   74%   19%    6%    5%    0%    --  -21%
# pure          992182/s  893%  119%   51%   34%   32%   26%   26%    --
# Windows:
# running wrap, direct_wrap, inline_c, plat_f, direct_affix, plat_a, pure, affix for 10 seconds each
#  affix - 12 wallclock secs (11.06 usr +  0.00 sys = 11.06 CPU) @ 572285.00/s (n=6331189)
# direct_affix - 11 wallclock secs (10.92 usr +  0.03 sys = 10.95 CPU) @ 599752.10/s (n=6568485)
# direct_wrap -  9 wallclock secs (10.03 usr +  0.02 sys = 10.05 CPU) @ 554099.23/s (n=5567035)
# inline_c - 11 wallclock secs (10.97 usr +  0.02 sys = 10.98 CPU) @ 279653.31/s (n=3071712)
# plat_a - 11 wallclock secs (10.25 usr +  0.00 sys = 10.25 CPU) @ 165828.88/s (n=1699746)
# plat_f - 11 wallclock secs (10.87 usr +  0.00 sys = 10.87 CPU) @ 36889.84/s (n=401177)
#   pure - 11 wallclock secs (10.89 usr +  0.00 sys = 10.89 CPU) @ 485180.07/s (n=5283611)
#   wrap -  9 wallclock secs (10.11 usr +  0.00 sys = 10.11 CPU) @ 562106.04/s (n=5682330)
#             Rate plat_f plat_a inline_c  pure direct_wrap  wrap affix direct_affix
# plat_f   36890/s    --  -78%  -87%  -92%  -93%  -93%  -94%  -94%
# plat_a  165829/s  350%    --  -41%  -66%  -70%  -70%  -71%  -72%
# inline_c  279653/s  658%   69%    --  -42%  -50%  -50%  -51%  -53%
# pure    485180/s 1215%  193%   73%    --  -12%  -14%  -15%  -19%
# direct_wrap  554099/s 1402%  234%   98%   14%    --   -1%   -3%   -8%
# wrap    562106/s 1424%  239%  101%   16%    1%    --   -2%   -6%
# affix   572285/s 1451%  245%  105%   18%    3%    2%    --   -5%
# direct_affix  599752/s 1526%  262%  114%   24%    8%    7%    5%    --
