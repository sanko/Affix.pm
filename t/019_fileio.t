use v5.40;
use lib '../lib', 'lib';
use blib;
use Test2::Tools::Affix qw[:all];
use Affix               qw[:all];
use File::Temp          qw[tempfile];
$|++;

# Define C code to compile
# We include functions that operate on standard FILE* and generic pointers (for PerlIO tests)
my $C_CODE = <<'END_C';
#include "std.h"
//ext: .c

#include <stdio.h>
#include <string.h>

typedef struct {
    FILE *file_ptr;
    int record_count;
} FileHandler;

// Write to a FILE* passed from Perl
DLLEXPORT int c_write_to_file(FILE* fp, const char* text) {
    if (!fp) return -1;
    return fprintf(fp, "%s", text);
}

// Read from a FILE* passed from Perl
DLLEXPORT int c_read_char(FILE* fp) {
    if (!fp) return -2;
    return fgetc(fp);
}

// Return a new FILE* created in C
DLLEXPORT FILE* c_create_tmpfile(void) {
    FILE* fp = tmpfile();
    if (fp) {
        fprintf(fp, "Content from C");
        fflush(fp);
        rewind(fp);
    }
    return fp;
}

// Identity function to test round-tripping a PerlIO pointer.
// Since we don't link against libperl here, we treat PerlIO* as void*.
DLLEXPORT void* c_perlio_identity(void* p) {
    return p;
}

// Check if FILE* is NULL (to verify failure cases)
DLLEXPORT int c_is_null_file(FILE* fp) {
    return fp == NULL;
}
END_C

# Compile the library
my $lib_path = compile_ok($C_CODE);
subtest 'Standard C FILE* (Affix::File)' => sub {
    affix $lib_path, 'c_write_to_file',  [ File, String ] => Int;
    affix $lib_path, 'c_read_char',      [File]           => Int;
    affix $lib_path, 'c_create_tmpfile', []               => File;
    affix $lib_path, 'c_is_null_file',   [File]           => Int;
    #
    subtest 'Writing to a Perl filehandle from C' => sub {
        my ( $fh, $filename ) = tempfile();

        # Note: We use a real file because PerlIO_findFILE (used internally)
        # requires a valid C-level FILE* which scalar handles (\$) might not provide.
        # Turn off buffering to ensure C sees the file state immediately
        my $old_fh = select($fh);
        $| = 1;
        select($old_fh);
        my $bytes = c_write_to_file( $fh, 'Hello from C' );
        ok $bytes > 0, 'C function returned success count';
        close $fh;

        # Verify content
        open my $check, '<', $filename or die $!;
        my $content = <$check>;
        is $content, 'Hello from C', 'Data written by C appears in file';
        unlink $filename;
    };
    subtest 'Reading from a Perl filehandle in C' => sub {
        my ( $fh, $filename ) = tempfile();
        print $fh 'ABC';
        close $fh;
        open my $read_fh, '<', $filename or die $!;
        my $char_code = c_read_char($read_fh);
        is chr($char_code), 'A', 'C function read first character correctly';
        $char_code = c_read_char($read_fh);
        is chr($char_code), 'B', 'C function read second character correctly';
        close $read_fh;
        unlink $filename;
    };
    subtest 'Returning a FILE* from C to Perl' => sub {
        my $fh = c_create_tmpfile();
        ok $fh, 'Received a filehandle from C';

        # Affix returns a Glob reference for files
        is ref($fh), 'GLOB', 'Returned handle is a Glob reference';
        my $line = <$fh>;
        is $line, 'Content from C', 'Perl can read from the C-created FILE*';

        # C-created tmpfiles usually disappear on close, simply ensure no crash
        close $fh;
    };
    subtest 'Passing invalid handles' => sub {

        # Passing undef/closed handle should result in NULL on C side
        is c_is_null_file(undef), 1, 'Passing undef results in NULL FILE*';
    }
};
subtest 'PerlIO* Streams (Affix::PerlIO)' => sub {

    # Bind the identity function using PerlIO type
    affix $lib_path, 'c_perlio_identity', [PerlIO] => PerlIO;

    # Test Roundtrip
    # Note: PerlIO* handles are generally strictly tied to the Perl layer.
    # When passed to C, we extract the PerlIO*, pass it, and wrap it in a new Glob on return.
    my ( $fh, $filename ) = tempfile();
    print $fh 'Test Data';
    seek( $fh, 0, 0 );
    my $new_fh = c_perlio_identity($fh);
    ok $new_fh, 'Received handle back from C';
    is ref($new_fh), 'GLOB', 'Returned handle is a Glob reference';

    # Since it's the same underlying stream, reading from one should advance the other
    # or at least access the same data source.
    my $line = <$new_fh>;
    is $line, 'Test Data', 'Round-tripped PerlIO handle is readable';
    close $fh;
    close $new_fh;    # Should be safe to close the wrapper
    unlink $filename;
};
done_testing;
