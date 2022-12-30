package t::lib::nativecall {
    use strict;
    use warnings;
    use Test::More;
    use experimental 'signatures';
    use ExtUtils::CBuilder;
    use Path::Tiny;
    use Exporter 'import';
    our @EXPORT = qw[compile_test_lib compile_cpp_test_lib is_approx];
    use Config;
    #
    my $OS = $^O;
    my @cleanup;
    #
    my $compiler = ExtUtils::CBuilder->new();
    #
    sub compile_test_lib ( $name, $file = "$name.c" ) {
        plan skip_all => 'Tests require a C compiler' unless $compiler->have_compiler;
        diag sprintf 'Compiling test lib t/src/%s...', $file;
        my $obj = $compiler->compile( source => path("t/src/$file") );
        diag sprintf 'Linking %s...', $obj;
        my $lib = $compiler->link( objects => $obj );
        diag sprintf 'Built %s', $lib;
        push @cleanup, $obj, $lib;
        $lib;
    }

    sub compile_cpp_test_lib ( $name, $file = "$name.cpp" ) {
        plan skip_all => 'Tests require a C++ compiler' unless $compiler->have_cplusplus;
        diag sprintf 'Compiling test lib t/src/%s...', $file;
        my $obj = $compiler->compile( source => path("t/src/$file") );
        diag sprintf 'Linking %s...', $obj;
        my $lib = $compiler->link( objects => $obj, 'C++' => 1 );
        diag sprintf 'Built %s', $lib;
        push @cleanup, $obj, $lib;
        $lib;
    }

    END {
        for my $file (@cleanup) {
            diag 'Removing ' . $file;
            unlink $file;
        }
    }

    sub is_approx ( $actual, $expected, $desc ) {    # https://docs.raku.org/routine/is-approx
        ok abs( $actual - $expected ) < 1e-6, $desc;
    }
};
1;
