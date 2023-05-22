use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:sugar];
#
sub pow : Native(get_lib) : Signature([Double, Double]=>Double);
plan skip_all => 'I know nothing about macOS' if $^O eq 'darwin';
is pow( 2, 10 ), 1024, 'pow( 2, 10 ) == 1024';
done_testing;

sub get_lib {
    return 'ntdll'                    if $^O eq 'MSWin32';
    return '/usr/lib/libSystem.dylib' if $^O eq 'darwin';
    ( 'm', 6 );
}
