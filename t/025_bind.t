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

sub run_tests_for_driver {
    my ( $driver_class, $label ) = @_;
    subtest "Driver: $label" => sub {
        #
        subtest "Preprocessor & Defines" => sub {
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
            my $objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($buf)  = grep { $_->name eq 'BUF_SIZE' } @$objs;
            ok( $buf, "Found BUF_SIZE" );
            if ($buf) {
                is( $buf->value, '1024', "BUF_SIZE value" );
                like( $buf->doc, qr/Buffer Size/, "BUF_SIZE doc" );
            }
        };
        #
        subtest "Records (Structs & Unions)" => sub {
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
            my $objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($pt)   = grep { $_->name eq 'Point' } @$objs;
            ok( $pt, "Found Point" );
            if ($pt) {
                like( $pt->doc, qr/A Point/, "Point doc" );
                is( $pt->members->[0]->name, 'x', "Member x" );
            }
            my ($pkt) = grep { $_->name eq 'Packet' } @$objs;
            ok( $pkt, "Found Packet" );
            if ($pkt) {
                is( $pkt->members->[0]->name, 'id',      "Member 0: id" );
                is( $pkt->members->[1]->name, 'payload', "Member 1: payload" );
                is( $pkt->members->[2]->name, 'flags',   "Member 2: flags" );
                my $u = $pkt->members->[1]->definition;
                ok $u, 'Payload has definition';
                if ($u) {
                    is( $u->tag,                'union', "Payload is union" );
                    is( $u->members->[0]->name, 'i',     "Union mem 0: i" );
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
            my $objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($st)   = grep { $_->name eq 'State' } @$objs;
            ok( $st, "Found State enum" );
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
            my $objs   = $parser->parse( $dir->child('funcs.cpp')->stringify, [] );
            my @calcs  = grep { $_->name eq 'calc' } @$objs;
            is( @calcs, 2, "Found 2 calc overloads" );
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
            my $objs   = $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] );
            my ($buf)  = grep { $_->name eq 'Buffer' } @$objs;
            ok( $buf, "Found Buffer" );
            if ($buf) {
                like( $buf->members->[0]->type, qr/int\s*\[16\]/,       "1D Array detected" );
                like( $buf->members->[1]->type, qr/char\s*\*/,          "Pointer detected" );
                like( $buf->members->[2]->type, qr/float\s*\[4\]\[4\]/, "2D Array detected" );
            }
        };
    };
}
#
run_tests_for_driver( 'Affix::Bind::Driver::Clang', 'Clang System' ) if $CLANG_AVAIL;
run_tests_for_driver( 'Affix::Bind::Driver::Regex', 'Regex System (Fallback)' );
#
done_testing();
