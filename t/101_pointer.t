use v5.40;
use lib '../lib', 'lib';
use blib;
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];
use Config;

# This C code will be compiled into a temporary library for the tests.
my $lib_path = compile_ok(<<'END_C');
#include "std.h"
//ext: .c
#include <stdint.h>
#include <string.h>

static void* g_ptr = NULL;

DLLEXPORT void set_ptr(void* ptr) { g_ptr = ptr; }
DLLEXPORT void* get_ptr(void) { return g_ptr; }
END_C
#
diag Affix::sizeof('char');
diag Affix::sizeof('int');
diag Affix::sizeof('int16');
diag Affix::sizeof('int32');
diag Affix::sizeof('int64');
diag Affix::sizeof('int128');
;    #

# Affix the C library functions
ok affix( $lib_path, 'set_ptr', '(*void)->void' ), 'affix set_ptr';
ok affix( $lib_path, 'get_ptr', '()->*void' ),     'affix get_ptr';
diag "--- Testing Affix::malloc and object methods ---";

# 1. Allocate a managed C buffer using the CORRECT array syntax.
my $ptr = Affix::malloc( Affix::sizeof('[50:char]') );

#~ my $ptr = Affix::calloc(50, 'char');
ok $ptr, 'malloc returns an Affix::Pointer object';
Affix::dump( $ptr, 32 );

# 2. Pass the object to a C function that expects a raw pointer.
#    Affix automatically extracts the raw C pointer from the blessed object.
set_ptr($ptr);
isnt get_ptr(), 0, 'C function received a non-null pointer';

# 3. Write data to C memory using Perl's dereferencing syntax.
#    This tests the "set" magic attached to the object.
my $message = "Hello from an OO pointer!";
$$ptr = $message;

# 4. Read data back from C memory by casting the return value.
my $ret_ptr = get_ptr();
warn $ret_ptr;

#~ ...;
done_testing;
exit;

# Use the CORRECT array syntax for the cast.
$ret_ptr->cast('[50:char]');
is $$ret_ptr, $message, 'C side correctly stored the string written from Perl';

# 5. Test the cast() method with a different array type.
# Use the CORRECT array syntax.
$ptr->cast('[4:uint32]');

# Write to the first integer slot. 0x4F4C4C48 is "OLLH" (Hello little-endian)
$$ptr = 0x4F4C4C48;

# Read it back from C by casting again.
$ret_ptr->cast('uint32');    # Casting to the element type to read the first element
is $$ret_ptr, 0x4F4C4C48, 'cast() works, can write and read an integer value';
diag "--- Testing Affix::calloc and Affix::free ---";

# The Affix::calloc API is correct: you provide the number of elements
# and the type of a SINGLE element. Affix constructs the array type internally.
my $another_ptr = Affix::calloc( 10, 'int' );
isa_ok $another_ptr, ['Affix::Pointer'], 'calloc returns an Affix::Pointer';

# The internal type of $another_ptr is now '[10:int]'
ok Affix::free($another_ptr), 'Affix::free() works as a function';
diag "--- Testing Affix::Pointer->free() ---";

# 6. Test the object method for freeing memory.
ok $ptr->free(), '->free() method returns true';

# Calling it again should be safe (idempotent).
ok $ptr->free(), '->free() can be called multiple times safely';

# Final note: If we had not called ->free(), when $ptr goes out of scope,
# Perl's GC would call the DESTROY handler for the magic attached to it,
# which triggers Affix_free_pin. Since pin->managed is true, it would have
# safefree'd the C memory automatically, preventing a leak.
done_testing;
