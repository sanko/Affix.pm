use v5.40;
use lib '../lib';
use Affix::Bind;
use Path::Tiny;
use Data::Printer;
$|++;
#
my $driver_class = 'Affix::Bind::Driver::Clang';
#
sub spew_files ( $dir, %files ) {
    $dir->child($_)->spew_utf8( $files{$_} ) for keys %files;
    $dir;
}
#
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
for my $obj ( $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] ) ) {
    p $obj;
    warn $obj->name;
    warn $obj->doc // '';
    warn $obj->describe;
    p $obj->members;
    if ( $obj isa 'Affix::Bind::Record' ) {
        for my $member ( @{ $obj->members } ) {
            p $member;
            say '     ' . $member->name . ' ' . $parser->_affix_type( $member->type );
        }
        ...;
    }
}
