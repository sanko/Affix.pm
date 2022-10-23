use strict;
use Test::More 0.98;
use lib '../lib', 'lib';
use Affix;
#
sub get_lib {
    return 'ntdll'               if $^O eq 'MSWin32';
    return '/usr/lib/libm.dylib' if $^O eq 'darwin';
    my $opt = $^O =~ /bsd/ ? 'r' : 'p';
    my ($path) = qx[ldconfig -$opt | grep libm.so];
    $path =~ m!(\S*?)$!;
    diag $1;
    $1;
}
sub pow : Native(get_lib) : Signature([Double, Double]=>Double);
is pow( 2, 10 ), 1024, 'pow( 2, 10 ) == 1024';
done_testing;

# /lib/x86_64-linux-gnu/libm.so.6

