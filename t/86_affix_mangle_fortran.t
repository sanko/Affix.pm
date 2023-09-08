use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use t::lib::helper;
use experimental 'signatures';
use Devel::CheckBin;
use Config;
$|++;
plan tests => 4;
# https://fortran-lang.org/learn/building_programs/managing_libraries/
SKIP: {
    skip 'test requires GNUFortran', 4 unless can_run('gfortran');
    my $lib
        = './t/src/86_affix_mangle_fortran/' .
        ( $^O eq 'MSWin32' ? '' : 'lib' ) .
        'affix_fortran.' .
        $Config{so};
    my $line
        = 'gfortran t/src/86_affix_mangle_fortran/hello.f90 -fPIC -fcheck=all -shared -o ' . $lib;
    diag $line;
    diag `$line`;

    #~ diag( ( -s $lib ) . ' bytes' );
    #~ warn `nm -D $lib`;
    ok affix( $lib, 'fstringlen', [String] => Int ), 'bound fortran function `fstringlen`';
    is fstringlen('Hello, World!'), 13, 'fstringlen("Hello, World!") == 13';
    ok affix( $lib, 'math::add', [ Int, Int ], Int ), 'bound fortran function `math::add` (module)';
    is math::add( 5, 4 ), 9, 'math::add(5, 4) == 9';
    diag 'might fail to clean up on Win32 because we have not released the lib yet... this is fine'
        if $^O eq 'MSWin32';
    unlink $lib;
}
#
done_testing;
