use v5.40;
use lib '../lib', 'lib';
use blib;
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];
use Config;
#
$|++;
my $C_CODE = <<'END_C';
#include "std.h"
//ext: .c

#include <stdint.h>
#include <stdbool.h>
#include <string.h> // For strcmp
#include <stdlib.h> // For malloc

/* Expose global vars */
DLLEXPORT int global_counter = 42;
DLLEXPORT void set_global_counter(int value) { global_counter = value;}
DLLEXPORT int get_global_counter(void) { return global_counter;}

END_C

# Compile the library once for all subtests that need it.
my $lib_path = compile_ok($C_CODE);
ok( $lib_path && -e $lib_path, 'Compiled a test shared library successfully' );

# TODO: loads more pinning and marshalling tests
subtest SInt32 => sub {
    my $pin_int;
    isa_ok affix( $lib_path, 'get_global_counter', '()->int32' );
    isa_ok affix( $lib_path, 'set_global_counter', '(int32)->void' );
    ok pin( $pin_int, $lib_path, 'global_counter', 'int32' ), 'pin(...)';
    is $pin_int, 42, 'pinned scalar equals 42';
    diag 'setting pinned scalar to 100';
    $pin_int = 100;
    is get_global_counter(), 100, 'checking value from inside the shared lib';
    diag 'setting value from inside the shared lib';
    set_global_counter(200);
    is $pin_int, 200, 'checking value from perl';
    diag 'unpinning scalar';
    ok unpin($pin_int), 'unpin() returns true';
    diag 'setting unpinned scalar to 25';
    $pin_int = 25;
    is get_global_counter(), 200, 'value is unchanged inside the shared lib';
    is $pin_int,             25,  'verify that value is local to perl';
};
done_testing;
