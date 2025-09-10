# Based on Module::Build::Tiny which is copyright (c) 2011 by Leon Timmermans, David Golden.
# Module::Build::Tiny is free software; you can redistribute it and/or modify it under
# the same terms as the Perl 5 programming language system itself.
use v5.40;
use feature 'class';
no warnings 'experimental::class';
class    #
    Affix::Builder {
    use CPAN::Meta;
    use ExtUtils::Install qw[pm_to_blib install];
    use ExtUtils::InstallPaths 0.002;
    use File::Basename        qw[basename dirname];
    use File::Path            qw[mkpath rmtree];
    use File::Spec::Functions qw[catfile catdir rel2abs abs2rel splitdir curdir];
    use JSON::PP 2            qw[encode_json decode_json];

    # Not in CORE
    use Path::Tiny qw[path cwd];
    use ExtUtils::Helpers 0.028 qw[make_executable split_like_shell detildefy];

    # infix and Affix stuff
    use Config qw[%Config];
    field $force : param //= 0;
    field $debug : param = 1;
    field $libver;
    field $cflags = $^O =~ /bsd/ ? '' : '-fPIC ' . (
        $debug > 0 ?
            '-DDEBUG=' .
            $debug .
            ' -g3 -pthread -gdwarf-4 -fPIC ' .
            ' -Wno-deprecated -pipe ' .
            ' -Wall -Wextra -Wpedantic -Wvla -Wnull-dereference ' .
            ' -Wswitch-enum  -Wduplicated-cond ' .
            ' -Wduplicated-branches'

        # .
        # ( $Config{osname} eq 'darwin' ? '' : ' -fvar-tracking-assignments' )
        :

            # $Config{osname} eq 'MSWin32' ? '' :
            ' -DNDEBUG -DBOOST_DISABLE_ASSERTS -Ofast -fPIC -ftree-vectorize -ffast-math -fno-align-functions -fno-align-loops -fno-omit-frame-pointer -flto'
    );
    field $ldflags = $^O =~ /bsd/ ? '' : ' -flto ';
    field $cppver  = 'c++17';                         # https://en.wikipedia.org/wiki/C%2B%2B20#Compiler_support
    field $cver    = 'c17';                           # https://en.wikipedia.org/wiki/C17_(C_standard_revision)
    field $make : param //= $Config{make};
    #
    field $action : param //= 'build';
    field $meta : reader = CPAN::Meta->load_file('META.json');

    # Params to Build script
    field $install_base  : param    //= '';
    field $installdirs   : param    //= '';
    field $uninst        : param    //= 0;            # Make more sense to have a ./Build uninstall command but...
    field $install_paths : param    //= ExtUtils::InstallPaths->new( dist_name => $meta->name );
    field $verbose       : param(v) //= 0;
    field $dry_run       : param    //= 0;
    field $pureperl      : param    //= 0;
    field $jobs          : param    //= 1;
    field $destdir       : param    //= '';
    field $prefix        : param    //= '';
    #
    ADJUST {
        -e 'META.json' or die "No META information provided\n";
    }
    method write_file( $filename, $content ) { path($filename)->spew_raw($content) or die "Could not open $filename: $!\n" }
    method read_file ($filename)             { path($filename)->slurp_utf8         or die "Could not open $filename: $!\n" }

    method step_build() {
        $self->step_affix;
        #~ ...;

        #~ for my $pl_file ( find( qr/\.PL$/i, 'lib' ) ) {
        #~ use Data::Dump;
        #~ ddx $pl_file;
        #~ ( my $pm = $pl_file ) =~ s/\.PL$//;
        #~ warn 'HERE';
        #~ system $^X, $pl_file, $pm and die "$pl_file returned $?\n";
        #~ }
        my %modules       = map { $_ => catfile( 'blib', $_ ) } find( qr/\.pm$/,  'lib' );
        my %docs          = map { $_ => catfile( 'blib', $_ ) } find( qr/\.pod$/, 'lib' );
        my %scripts       = map { $_ => catfile( 'blib', $_ ) } find( qr/(?:)/,   'script' );
        my %sdocs         = map { $_ => delete $scripts{$_} } grep {/.pod$/} keys %scripts;
        my %dist_shared   = map { $_ => catfile( qw[blib lib auto share dist],   $meta->name, abs2rel( $_, 'share' ) ) } find( qr/(?:)/, 'share' );
        my %module_shared = map { $_ => catfile( qw[blib lib auto share module], abs2rel( $_, 'module-share' ) ) } find( qr/(?:)/, 'module-share' );
        pm_to_blib( { %modules, %docs, %scripts, %dist_shared, %module_shared }, catdir(qw[blib lib auto]) );
        make_executable($_) for values %scripts;
        mkpath( catdir(qw[blib arch]), $verbose );
        0;
    }
    method step_clean() { rmtree( $_, $verbose ) for qw[blib temp]; 0 }

    method step_install() {
        $self->step_build() unless -d 'blib';
        install( $install_paths->install_map, $verbose, $dry_run, $uninst );
        0;
    }
    method step_realclean () { rmtree( $_, $verbose ) for qw[blib temp Build _build_params MYMETA.yml MYMETA.json]; 0 }

    method step_test() {
        $self->step_build() unless -d 'blib';
        require TAP::Harness::Env;
        my %test_args = (
            ( verbosity => $verbose ),
            ( jobs  => $jobs ),
            ( color => -t STDOUT ),
            lib => [ map { rel2abs( catdir( 'blib', $_ ) ) } qw[arch lib] ],
        );
        TAP::Harness::Env->create( \%test_args )->runtests( sort map { $_->stringify } find( qr/\.t$/, 't' ) )->has_errors;
    }

    method get_arguments (@sources) {
        $_ = detildefy($_) for grep {defined} $install_base, $destdir, $prefix, values %{$install_paths};
        $install_paths = ExtUtils::InstallPaths->new( dist_name => $meta->name );
        return;
    }

    method Build(@args) {
        my $method = $self->can( 'step_' . $action );
        $method // die "No such action '$action'\n";
        exit $method->($self);
    }

    method Build_PL() {
        die "Can't build Affix under --pureperl-only\n" if $pureperl;
        say sprintf 'Creating new Build script for %s %s', $meta->name, $meta->version;
        $self->write_file( 'Build', sprintf <<'', $^X, __PACKAGE__, __PACKAGE__ );
#!%s
use lib 'builder';
use %s;
%s->new( @ARGV && $ARGV[0] =~ /\A\w+\z/ ? ( action => shift @ARGV ) : (),
    map { /^--/ ? ( shift(@ARGV) =~ s[^--][]r => 1 ) : /^-/ ? ( shift(@ARGV) =~ s[^-][]r => shift @ARGV ) : () } @ARGV )->Build();

        make_executable('Build');
        my @env = defined $ENV{PERL_MB_OPT} ? split_like_shell( $ENV{PERL_MB_OPT} ) : ();
        $self->write_file( '_build_params', encode_json( [ \@env, \@ARGV ] ) );
        if ( my $dynamic = $meta->custom('x_dynamic_prereqs') ) {
            my %meta = ( %{ $meta->as_struct }, dynamic_config => 1 );
            $self->get_arguments( \@env, \@ARGV );
            require CPAN::Requirements::Dynamic;
            my $dynamic_parser = CPAN::Requirements::Dynamic->new();
            my $prereq         = $dynamic_parser->evaluate($dynamic);
            $meta{prereqs} = $meta->effective_prereqs->with_merged_prereqs($prereq)->as_string_hash;
            $meta = CPAN::Meta->new( \%meta );
        }
        $meta->save(@$_) for ['MYMETA.json'];
    }

    sub find ( $pattern, $base ) {
        $base = path($base) unless builtin::blessed $base;
        my $blah = $base->visit(
            sub ( $path, $state ) {
                $state->{$path} = $path if $path =~ $pattern;

                #~ return \0 if keys %$state == 10;
            },
            { recurse => 1 }
        );
        values %$blah;
    }

    # infix builder
    method step_clone_infix() {
        return                      if cwd->absolute->child('infix')->exists;
        die 'Failed to clone infix' if system 'git clone --verbose https://github.com/sanko/infix/';
    }

    method step_infix () {
        $self->step_clone_infix();
return
        my $pre = cwd->absolute->child( qw[blib arch auto], $meta->name );
        return 0 if -d $pre;
        $pre->child('lib')->mkdir;
        my $cwd       = cwd->absolute;
        my $build_dir = $cwd->child('infix')->absolute;
        chdir $build_dir;
        `gmake`;
        return 0;
        warn `xmake -P .`;

        #~ die "Can't build xs files under --pureperl-only\n" if $opt{'pureperl-only'};
        if ( $^O eq 'MSWin32' ) {
            for my $exe ( $make, qw[gmake nmake mingw32-make] ) {
                next unless `$exe --version`;
                $make = $exe;
                if ( $exe eq 'nmake' ) {
                    $make .= ' -f Nmakefile' . ( $verbose ? '' : ' /S' );
                }
                else {
                    $make = 'gmake AS="gcc -c " CC=gcc VPATH=. PREFIX="' . $pre . '"' . ( $verbose ? '' : ' -s' );
                }
                last;
            }
            ( ( $verbose && CORE::say($_) ) || 1 ) && system($_) for $make;
            $pre->child('include')->mkdir;
            $pre->child('lib')->mkdir;
        }
        else {
            $make .= ( $verbose ? '' : ' -s' );
            ( ( $verbose && CORE::say($_) ) || 1 ) && system($_) for $make, $make . ' install';
        }
        chdir $cwd;
    }

    method step_affix {
        $self->step_infix;
        my $cwd = cwd->absolute;
        my @objs;
        require ExtUtils::CBuilder;
        my $builder = ExtUtils::CBuilder->new(
            quiet  => !$verbose,
            config => {

                #~ (
                #~ $opt{config}->get('osname') !~ /bsd/ &&
                #~ $opt{config}->get('ld') eq 'cc' ? ( ld => 'g++' ) : ()
                #~ ),
                #~ %{ $opt{config}->values_set }
            }
        );
        my $pre = $cwd->child(qw[blib arch auto])->absolute;
        my $source;
        require DynaLoader;
        my $mod2fname = defined &DynaLoader::mod2fname ? \&DynaLoader::mod2fname : sub { return $_[0][-1] };
        my @parts     = ('Affix');
        my $archdir   = catdir( qw/blib arch auto/, @parts );
        mkpath( $archdir, $verbose, oct '755' ) unless -d $archdir;
        my $lib_file = catfile( $archdir, $mod2fname->( \@parts ) . '.' . $Config{dlext} );
        my @dirs;
        push @dirs, '../';
        my $has_cxx = !1;
        warn $cwd;


       if(0) {
            use ExtUtils::ParseXS;
            my $pxs = ExtUtils::ParseXS->new;

      $pxs->process_file( filename => 'lib/Affix.xs',
                          output => 'lib/Affix.c',
                          'C++' => 0,
                          typemap => 'typemap',
                          hiertype => 1,
                          except => 1,
                          versioncheck => 1,
                          linenumbers => 1,
                          optimize => 1,
                          prototypes => 1,
                          die_on_error => 0,
                        );
                    }


        for my $source (
            find( qr/\.c$/, $cwd->child('lib') ),
            find( qr/\.c$/, $cwd->child('infix/src/core') )
        ) {
            warn $source;

            my $cxx       = $source =~ /cx+$/;
            my $file_base = $source->basename(qr[.c$]);
            my $tempdir   = path('lib');
            $tempdir->mkdir( {verbose => $verbose, mode => oct '755'} );
            my $version = $meta->version;
            my $obj     = $builder->object_file($source);
            push @dirs, $source->dirname();
            #~ warn sprintf '%d vs %d (%d)',
            #~ $source->stat->mtime, path($obj)->stat->mtime,
            #~ $source->stat->mtime - path($obj)->stat->mtime;

            #~ use Data::Dump;
            #~ ddx {
            #~ 'C++'        => $cxx,
            #~ source       => $source->stringify,
            #~ defines      => { VERSION => qq/"$version"/, XS_VERSION => qq/"$version"/ },
            #~ include_dirs => [
            #~ cwd->stringify, cwd->child('dyncall')->realpath->stringify, $source->dirname, $pre->child( $meta->name, 'include' )->stringify
            #~ ],
            #~ extra_compiler_flags =>
            #~ ( '-fPIC -std=' . ( $cxx ? $cppver : $cver ) . ' ' . $cflags . ( $debug ? ' -ggdb3 -g -Wall -Wextra -pedantic' : '' ) )
            #~ };
            $has_cxx = 1 if $cxx;


          warn cwd->child('infix')->child('src')->realpath->stringify;

            push @objs,    # misses headers but that's okay
                ( $force ||
                    ( !-f $obj ) ||
                    ( $source->stat->mtime >= path($obj)->stat->mtime ) ||
                    ( path(__FILE__)->stat->mtime > path($obj)->stat->mtime ) ) ?
                $builder->compile(
                'C++'        => $cxx,
                source       => $source->stringify,
                defines      => { VERSION => qq/"$version"/, XS_VERSION => qq/"$version"/ },
                include_dirs =>
                    [ cwd->stringify, cwd->child('infix')->realpath->stringify,
                    cwd->child('infix')->child('include')->realpath->stringify,
                    cwd->child('infix')->child('src')->realpath->stringify,
                    cwd->child('infix')->child('src/core')->realpath->stringify,
                    cwd->child('infix')->child('src/arch/x64')->realpath->stringify,
                    cwd->child('infix')->child('src/arch/aarch64')->realpath->stringify,

                    $source->dirname,
                    $pre->child( $meta->name, 'include'
                    )->stringify ],
                extra_compiler_flags =>
                    ( '-fPIC -std=' . ( $cxx ? $cppver : $cver ) . ' ' . $cflags . ( $debug ? ' -ggdb3 -g -Wall -Wextra -pedantic' : '' ) )
                ) :
                $obj;
        }

        #~ warn join ', ', @dirs;
        #~ warn join ', ', @parts;
        #~ warn $lib_file;

        return (
            ( $force || ( !-f $lib_file ) || grep { path($_)->stat->mtime > path($lib_file)->stat->mtime } @objs ) ?
                $builder->link(
                extra_linker_flags => (
                    $ldflags .
                        ( join ' ', map { ' -L' . $_ } @dirs ) . ' -L' .
                        $pre->child( $meta->name, 'lib' )->stringify .
                        ( $has_cxx ? '' : ' -lstdc++ ' ) .
                        ' -linfix'
                ),
                objects     => [@objs],
                lib_file    => $lib_file,
                module_name => join '::',
                @parts
                ) :
                $lib_file
        );
    }
    };
1;
