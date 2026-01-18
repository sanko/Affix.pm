#! perl

use strict;
use warnings;

use File::Spec::Functions qw/catdir catfile/;

load_extension('Dist::Build::XS');
load_extension('Dist::Build::XS::Conf');

create_node(
	target => 'infix',
	actions => [ command(qw{git clone --verbose https://github.com/sanko/infix.git}) ],
);

my $infix_c = catfile('infix', 'src', 'infix.c');
create_node(target => $infix_c, dependencies => [ 'infix' ]);

try_find_libraries_for(source => <<'END_C', libs => [[], ['rt']]);
#include <sys/mman.h>
#include <fcntl.h>
int main(void) { shm_open("/test", O_RDONLY, 0); return 0; }
END_C

if (is_os_type('Unix')) {
	push_extra_compiler_flags(qw/
		-pthread -DNDEBUG -DBOOST_DISABLE_ASSERTS -Ofast -ftree-vectorize -ffast-math
		-fno-align-functions -fno-align-loops -fno-omit-frame-pointer -flto=auto
	/);
	push_extra_linker_flags('-pthread');
}

if ($::DEBUG) {
	push_extra_compiler_flags("-DDEBUG=$::DEBUG", qw/
		-g3 -gdwarf-4 -Wno-deprecated -Wall -Wextra -Wpedantic -Wvla
		-Wnull-dereference -Wswitch-enum -Wduplicated-cond -Wduplicated-branches
	/);
	push_extra_compiler_flags('-fvar-tracking-assignments') unless is_os('darwin');
}

add_xs(
	file          => 'lib/Affix.c',
	dependencies  => [ 'infix' ],
	include_dirs  => [ catdir('infix', 'include'), catdir('infix', 'src') ],
	extra_sources => [ $infix_c ],
	standard      => 'c17',
);
