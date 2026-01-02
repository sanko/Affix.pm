use v5.40;
use blib;
use Affix::Bind;
use Test2::Tools::Affix qw[:all];
#
my $CLANG_AVAIL = do {
    my ( undef, undef, $exit ) = Capture::Tiny::capture { system 'clang', '--version' };
    $exit == 0;
};

sub spew_files ( $dir, %files ) {
    $dir->child($_)->spew_utf8( $files{$_} ) for keys %files;
    $dir;
}

sub run_tests_for_driver ( $driver_class, $label ) {
    subtest 'Driver: ' . $label => sub {
        #
        subtest 'Preprocessor & Defines' => sub {
            my $dir = Path::Tiny->tempdir;
            spew_files(
                $dir,
                'defs.h' => <<'EOF',
/** @brief Buffer Size */
#define BUF_SIZE 1024
#define API_NAME "MyLib"
EOF
                'main.c' => '#include "defs.h"'
            );
            my $parser = $driver_class->new( project_files => [ $dir->child('defs.h')->stringify ] );
            my @objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($buf)  = grep { $_->name eq 'BUF_SIZE' } @objs;
            ok( $buf, 'Found BUF_SIZE' );
            if ($buf) {
                is( $buf->value, '1024', 'BUF_SIZE value' );
                like( $buf->doc, qr/Buffer Size/, 'BUF_SIZE doc' );
            }
        };
        #
        subtest 'Records (Structs & Unions)' => sub {
            my $dir = Path::Tiny->tempdir;
            spew_files(
                $dir,
                'structs.h' => <<'EOF',
/** @brief A Point */
typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int id;
    union {
        int i;
        float f;
    } payload;
    int flags;
} Packet;
EOF
                'main.c' => '#include "structs.h"'
            );
            my $parser = $driver_class->new( project_files => [ $dir->child('structs.h')->stringify ] );
            my @objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );

            # Check Point
            my ($pt) = grep { $_->name eq 'Point' } @objs;
            ok( $pt, 'Found Point' );
            if ($pt) {
                like( $pt->doc, qr/A Point/, 'Point doc' );
                is( $pt->members->[0]->name, 'x', 'Member x name' );
                isa_ok( $pt->members->[0]->type, ['Affix::Bind::Type'], 'Member x type object' );
                is( $pt->members->[0]->type->affix_type, 'Int', 'Member x is Int' );
            }

            # Check Packet
            my ($pkt) = grep { $_->name eq 'Packet' } @objs;
            ok( $pkt, 'Found Packet' );
            if ($pkt) {
                is( $pkt->members->[0]->name, 'id',      'Member 0: id' );
                is( $pkt->members->[1]->name, 'payload', 'Member 1: payload' );
                is( $pkt->members->[2]->name, 'flags',   'Member 2: flags' );
                my $u = $pkt->members->[1]->definition;
                ok $u, 'Payload has definition';
                if ($u) {
                    is( $u->tag,                            'union', 'Payload is union' );
                    is( $u->members->[0]->name,             'i',     'Union mem 0: i' );
                    is( $u->members->[1]->type->affix_type, 'Float', 'Union mem 1 is Float' );
                }
            }
        };
        #
        subtest Enums => sub {
            my $dir = Path::Tiny->tempdir;
            spew_files(
                $dir,
                'enums.h' => <<'EOF',
enum State {
    IDLE,
    RUNNING = 5,
    STOPPED
};
EOF
                'main.c' => '#include "enums.h"'
            );
            my $parser = $driver_class->new( project_files => [ $dir->child('enums.h')->stringify ] );
            my @objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($st)   = grep { $_->name eq 'State' } @objs;
            ok( $st, 'Found State enum' );
            if ($st) {
                my $c = $st->constants;
                is( $c->[0]{name},  'IDLE',    'IDLE' );
                is( $c->[0]{value}, 0,         'IDLE=0' );
                is( $c->[1]{name},  'RUNNING', 'RUNNING' );
                is( $c->[1]{value}, 5,         'RUNNING=5' );
            }
        };
        #
        subtest Functions => sub {
            my $dir = Path::Tiny->tempdir;
            spew_files(
                $dir,
                'funcs.cpp' => <<'EOF',
/** @brief Calc */
int calc(int a);
int calc(double d);
static void helper() {}
EOF
            );
            my $parser = $driver_class->new( project_files => [ $dir->child('funcs.cpp')->stringify ] );
            my @objs   = $parser->parse( $dir->child('funcs.cpp')->stringify, [] );

            # Sort calcs to ensure deterministic testing of overloads
            my @calcs = sort { $a->args->[0]->name cmp $b->args->[0]->name } grep { $_->name eq 'calc' } @objs;
            is( @calcs, 2, 'Found 2 calc overloads' );
            if ( @calcs == 2 ) {

                # calc(int a)
                my $f1 = $calcs[0];
                is( $f1->ret->affix_type, 'Int', 'Ret Int' );
                is( $f1->args->[0]->name, 'a',   'Arg name a' );

                # isa_ok($thing, $class) is fine without name, but being consistent
                isa_ok( $f1->args->[0], ['Affix::Bind::Argument'], 'Arg object check' );
                is( $f1->args->[0]->affix_type, 'Int', 'Arg type Int' );

                # calc(double d)
                my $f2 = $calcs[1];
                is( $f2->ret->affix_type,       'Int',    'Ret Int' );
                is( $f2->args->[0]->name,       'd',      'Arg name d' );
                is( $f2->args->[0]->affix_type, 'Double', 'Arg type Double' );
            }
        };
        #
        subtest 'Edge Cases' => sub {
            my $dir = Path::Tiny->tempdir;
            spew_files(
                $dir,
                'edge.h' => <<'EOF',
typedef struct {
    int data[16];
    char* name;
    float matrix[4][4];
} Buffer;
EOF
                'main.c' => '#include "edge.h"'
            );
            my $parser = $driver_class->new( project_files => [ $dir->child('edge.h')->stringify ] );
            my @objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($buf)  = grep { $_->name eq 'Buffer' } @objs;
            ok $buf, 'Found Buffer';
            if ($buf) {
                my $m0 = $buf->members->[0];    # int data[16]
                isa_ok( $m0->type, ['Affix::Bind::Type::Array'], 'Member 0 is Array' );
                if ( $m0->type->isa('Affix::Bind::Type::Array') ) {
                    is( $m0->type->count,      16,               'Array count 16' );
                    is( $m0->type->of->name,   'int',            'Array of int' );
                    is( $m0->type->affix_type, 'Array[Int, 16]', 'Affix Sig: Array[Int, 16]' );
                }
                my $m1 = $buf->members->[1];    # char* name
                isa_ok( $m1->type, ['Affix::Bind::Type::Pointer'], 'Member 1 is Pointer' );
                if ( $m1->type->isa('Affix::Bind::Type::Pointer') ) {
                    is( $m1->type->of->name,   'char',          'Pointer to char' );
                    is( $m1->type->affix_type, 'Pointer[Char]', 'Affix Sig: Pointer[Char]' );
                }
                my $m2 = $buf->members->[2];    # float matrix[4][4]
                isa_ok( $m2->type, ['Affix::Bind::Type::Array'], 'Member 2 is Array' );
                if ( $m2->type->isa('Affix::Bind::Type::Array') ) {
                    is( $m2->type->count, 4, 'Outer dim 4' );
                    isa_ok( $m2->type->of, ['Affix::Bind::Type::Array'], 'Inner type is Array' );
                    is( $m2->type->of->count,          4,                           'Inner dim 4' );
                    is( $m2->type->of->of->affix_type, 'Float',                     'Inner type Float' );
                    is( $m2->type->affix_type,         'Array[Array[Float, 4], 4]', '2D Array Affix Sig' );
                }
            }
        };
    };
    subtest 'System Header Filtering' => sub {
        my $dir = Path::Tiny->tempdir;
        spew_files(
            $dir,
            'mixed.h' => <<'EOF',
#include <stdlib.h>
#include <stdio.h>

void my_local_function(int x);
EOF
            'main.c' => '#include "mixed.h"'
        );
        my $parser  = $driver_class->new( project_files => [ $dir->child('mixed.h')->stringify ] );
        my @objs    = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
        my ($local) = grep { $_->name eq 'my_local_function' } @objs;
        ok( $local, 'Found local function "my_local_function"' );
        my ($sys) = grep { $_->name eq 'exit' || $_->name eq 'malloc' } @objs;
        ok( !$sys, 'System function (exit/malloc) was correctly filtered out' );
        my ($size_t) = grep { $_->name eq 'size_t' } @objs;
        ok( !$size_t, 'System typedef (size_t) was filtered out' );
    }
}
run_tests_for_driver( 'Affix::Bind::Driver::Clang', 'Clang System' ) if $CLANG_AVAIL;
run_tests_for_driver( 'Affix::Bind::Driver::Regex', 'Regex System (Fallback)' );
done_testing();
