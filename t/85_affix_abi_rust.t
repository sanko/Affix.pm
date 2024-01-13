use Test2::V0;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use t::lib::helper;
use experimental 'signatures';
use Devel::CheckBin;
use Config;
$|++;
#
#~ my $lib = 't/src/85_affix_mangle_rust/target/release/' .
#~ ( $^O eq 'MSWin32' ? '' : 'lib' ) . 'affix_rust.' . $Config{so};
#~ warn $lib;
#~ system 'nm -D --demangle ' . $lib;
SKIP: {
    skip 'test requires rust/cargo', 4 unless can_run('cargo');
    my $lib = 't/src/85_affix_mangle_rust/target/release/' . ( $^O eq 'MSWin32' ? '' : 'lib' ) . 'affix_rust.' . $Config{so};
    diag 'Building dylib ' . $lib;
    system 'cargo build --manifest-path=t/src/85_affix_mangle_rust/Cargo.toml --release --quiet';
    ok affix( $lib, 'add', [ Size_t, Size_t ], Size_t ), 'bound rust function with #[no_mangle]';
    is add( 5, 4 ), 9, 'add(5, 4) == 9';
    #
    ok affix( $lib, 'mod' => [ Int, Int ] => Int ), 'bound mangled rust function';
    is mod( 5, 3 ), 2, 'mod(5, 3) == 2';
    #
    diag 'might fail to clean up on Win32 because we have not released the lib yet... this is fine' if $^O eq 'MSWin32';
    system 'cargo clean --manifest-path=t/src/85_affix_mangle_rust/Cargo.toml --quiet';
}
#
done_testing;
