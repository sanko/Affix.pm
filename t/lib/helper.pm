package t::lib::helper {
    use strict;
    use warnings;
    use Test2::V0;
    use experimental 'signatures';
    use Path::Tiny;
    use Exporter 'import';
    our @EXPORT = qw[compile_test_lib compile_cpp_test_lib is_approx];
    use Config;
    use Affix qw[];
    #
    my $OS = $^O;

    #~ Affix::Platform::OS();
    my @cleanup;
    #
    #~ diag $Config{cc};
    #~ diag $Config{cccdlflags};
    #~ diag $Config{ccdlflags};
    #~ diag $Config{ccflags};
    #~ diag $Config{ccname};
    #~ diag $Config{ccsymbols};
    sub compile_test_lib ( $name, $aggs = '', $keep = 0 ) {
        my $opt = path( grep { -f $_ } "t/src/$name.cxx", "t/src/$name.c" );
        return plan skip_all => 'Failed to build test lib' if !$opt;
        my $c_file = $opt->canonpath;
        my $o_file = path( "t/src/$name" . $Config{_o} )->canonpath;
        my $l_file = path( "t/src/$name." . $Config{so} )->canonpath;
        diag sprintf 'Building %s into %s', $c_file, $l_file;
        my $compiler = $Config{cc};
        if ( $c_file =~ /\.cxx$/ ) {
            if ( Affix::Platform::Compiler() eq 'Clang' ) {
                $compiler = 'c++';
            }
            elsif ( Affix::Platform::Compiler() eq 'GNU' ) {
                $compiler = 'g++';
            }
        }
        my @cmds = (
            "$compiler -Wall --shared -fPIC -DBUILD_LIB $aggs -o $l_file $c_file",

            #~ (
            #~ $OS eq 'MSWin32' ? "cl /LD /EHsc /Fe$l_file $c_file" :
            #~ "clang -stdlib=libc --shared -fPIC -o $l_file $c_file"
            #~ )
        );
        my ( @fails, $succeeded );
        my $ok;
        for my $cmd (@cmds) {
            diag $cmd;
            system $cmd;
            if ( $? == -1 ) {
                diag 'failed to execute: ' . $!;
            }
            elsif ( $? & 127 ) {
                diag sprintf "child died with signal %d, %s coredump\n", ( $? & 127 ), ( $? & 128 ) ? 'with' : 'without';
            }
            else {
                # diag 'child exited with value ' . ( $? >> 8 );
                $ok++;
                last;
            }
        }
        plan skip_all => 'Failed to build test lib' if !-f $l_file;
        push @cleanup, $o_file, $l_file unless $keep;
        $l_file;
    }

    END {
        for my $file ( grep {-f} @cleanup ) {
            diag 'Removing ' . $file;
            unlink $file;
        }
    }

    sub is_approx ( $actual, $expected, $desc ) {    # https://docs.raku.org/routine/is-approx
        ok abs( $actual - $expected ) < 1e-6, $desc;
    }
};
1;
