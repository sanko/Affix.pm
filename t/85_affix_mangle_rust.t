use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use t::lib::nativecall;
use experimental 'signatures';
use Devel::CheckBin;
use Config;
$|++;
#
#my $lib = compile_test_lib('85_affix_mangle_rust');
SKIP: {
    skip 'test requires rust/cargo', 1 unless can_run('cargo');
    system 'cargo build --manifest-path=t/src/85_affix_mangle_rust/Cargo.toml --release';
    my $lib = 't/src/85_affix_mangle_rust/target/release/' .
        ( $^O eq 'MSWin32' ? '' : 'lib' ) . 'affix_rust.' . $Config{so};

    #system 'nm ' . $lib;
    affix $lib, 'add', [ Size_t, Size_t ], Size_t;    #[no_mangle]
    is add( 5, 4 ), 9, 'add(5, 4) == 9';
    system 'cargo clean --manifest-path=t/src/85_affix_mangle_rust/Cargo.toml';
}

# _ZN10affix_rust3pow17h1d4d2ee71344ca5aE
#
done_testing;
