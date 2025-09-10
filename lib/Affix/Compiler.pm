use v5.40;
use feature qw[class];
no warnings qw[experimental::class experimental::try];
use Data::Printer;

#~ use Carp::Always;
$|++;
#
class Affix::Compiler {
    use Config     qw[%Config];
    use Path::Tiny qw[path tempdir];
    use File::Spec;
    use ExtUtils::MakeMaker;
    #
    field $os : param : reader        //= $^O;
    field $cleanup : param : reader   //= 0;
    field $version : param : reader   //= ();
    field $build_dir : param : reader //= tempdir( CLEANUP => $cleanup );
    field $name : param : reader;
    field $libname : reader
        = $build_dir->child( ( ( $os eq 'MSWin32' || $name =~ /^lib/ ) ? '' : 'lib' ) .
            $name . '.' .
            $Config{so} .
            ( $os eq 'MSWin32' || !defined $version ? '' : '.' . $version ) )->absolute;
    field $platform : reader = ();    # ADJUST
    field $source : param : reader;
    field $flags : param : reader //= {

        #~ ldflags => $Config{ldflags},
        cflags   => $Config{cflags},
        cppflags => $Config{cxxflags}
    };
    field @objs : reader = [];
    ADJUST {
        $source = [ map { _filemap($_) } @$source ];
    }
    #
    sub _can_run(@cmd) {
        state $paths //= [ map { $_->realpath } grep { $_->exists } map { path($_) } File::Spec->path ];
        for my $exe (@cmd) {
            grep { return path($_) if $_ = MM->maybe_command($_) } $exe, map { $_->child($exe) } @$paths;
        }
    }
    #
    field $linker : reader : param //= _can_run qw[g++ ld];

    #~ https://gcc.gnu.org/onlinedocs/gcc-3.4.0/gnat_ug_unx/Creating-an-Ada-Library.html
    field $ada : reader : param //= _can_run qw[gnatmake];

    #~ https://fasterthanli.me/series/making-our-own-executable-packer/part-5
    #~ https://stackoverflow.com/questions/71704813/writing-and-linking-shared-libraries-in-assembly-32-bit
    #~ https://github.com/therealdreg/nasm_linux_x86_64_pure_sharedlib
    field $asm : reader : param //= _can_run qw[nasm as];
    field $c : reader : param   //= _can_run qw[gcc clang cc icc icpx cl eccp];
    field $cpp : reader : param //= _can_run qw[g++ clang++ c++ icpc icpx cl eccp];

    #~ https://c3-lang.org/build-your-project/build-commands/
    field $c3 : reader : param //= _can_run qw[c3c];

    #~ https://www.circle-lang.org/site/index.html
    field $circle : reader : param //= _can_run qw[circle];

    #~ https://mazeez.dev/posts/writing-native-libraries-in-csharp
    #~ https://medium.com/@sixpeteunder/how-to-build-a-shared-library-in-c-sharp-and-call-it-from-java-code-6931260d01e5
    field $csharp : reader : param //= _can_run qw[dotnet];

    # cobc: https://gnucobol.sourceforge.io/
    field $cobol : reader : param //= _can_run qw[cobc cobol cob cob2];

    #~ https://github.com/crystal-lang/crystal/issues/921#issuecomment-2413541412
    field $crystal : reader : param //= _can_run qw[crystal];

    #~ https://wiki.liberty-eiffel.org/index.php/Compile
    #~ https://svn.eiffel.com/eiffelstudio-public/branches/Eiffel_54/Delivery/docs/papers/dll.html
    field $eiffel : reader : param //= _can_run qw[se];

    #~ https://dlang.org/articles/dll-linux.html#dso9
    #~ dmd -c dll.d -fPIC
    #~ dmd -oflibdll.so dll.o -shared -defaultlib=libphobos2.so -L-rpath=/path/to/where/shared/library/is
    field $d : reader : param       //= _can_run qw[dmd];
    field $fortran : reader : param //= _can_run qw[gfortran ifx ifort];

    #~ https://github.com/secana/Native-FSharp-Library
    #~ https://secanablog.wordpress.com/2020/02/01/writing-a-native-library-in-f-which-can-be-called-from-c/
    field $fsharp : reader : param //= _can_run qw[dotnet];

    #~ https://futhark.readthedocs.io/en/stable/usage.html
    field $futhark : reader : param //= _can_run qw[futhark];    # .fut => .c

    #~ https://medium.com/@walkert/fun-building-shared-libraries-in-go-639500a6a669
    #~ https://github.com/vladimirvivien/go-cshared-examples
    field $go : reader : param //= _can_run qw[go];

    #~ https://github.com/bennoleslie/haskell-shared-example
    #~ https://www.hobson.space/posts/haskell-foreign-library/
    field $haskell : reader : param //= _can_run qw[ghc cabal];

    #~ https://peterme.net/dynamic-libraries-in-nim.html
    field $nim : reader : param //= _can_run qw[nim];    # .nim => .c

    #~ https://odin-lang.org/news/calling-odin-from-python/
    field $odin : reader : param //= _can_run qw[odin];

    #~ https://p-org.github.io/P/getstarted/install/#step-4-recommended-ide-optional
    #~ https://p-org.github.io/P/getstarted/usingP/#compiling-a-p-program
    field $p : reader : param //= _can_run qw[p];    # .p => C#

    #~ https://blog.asleson.org/2021/02/23/how-to-writing-a-c-shared-library-in-rust/
    field $rust : reader : param //= _can_run qw[cargo];

    #~ swiftc point.swift -emit-module -emit-library
    #~ https://forums.swift.org/t/creating-a-c-accessible-shared-library-in-swift/45329/5
    #~ https://theswiftdev.com/building-static-and-dynamic-swift-libraries-using-the-swift-compiler/#should-i-choose-dynamic-or-static-linking
    field $swift : reader : param //= _can_run qw[swiftc];

    #~ https://www.rangakrish.com/index.php/2023/04/02/building-v-language-dll/
    #~ https://dev.to/piterweb/how-to-create-and-use-dlls-on-vlang-1p13
    field $v : reader : param //= _can_run qw[v];

    #~ https://ziglang.org/documentation/0.13.0/#Exporting-a-C-Library
    #~ zig build-lib mathtest.zig -dynamic
    field $zig : reader : param //= _can_run qw[zig];
    #
    ADJUST {
    }

    sub _filemap( $file, $language //= () ) {
        #
        ($_) = $file =~ m[\.(?=[^.]*\z)([^.]+)\z]i;
        $language //=                                                     #
            /^(?:ada|adb|ads|ali)$/i                  ? 'Ada' :           #
            /^(?:asm|s|a)$/i                          ? 'Assembly' :      #
            /^(?:c(?:c|pp|xx))$/i                     ? 'CPP' :           #
            /^c$/i                                    ? 'C' :             #
            /^c3$/i                                   ? 'C3' :            #
            /^d$/i                                    ? 'D' :             #
            /^cobol$/i                                ? 'Cobol' :         #
            /^csharp$/i                               ? 'CSharp' :        #
            /^crystal$/i                              ? 'Crystal' :       #
            /^futhark$/i                              ? 'Futhark' :       #
            /^go$/i                                   ? 'Go' :            #
            /^haskell$/i                              ? 'Haskell' :       #
            /^nim$/i                                  ? 'Nim' :           #
            /^odin$/i                                 ? 'Odin' :          #
            /^ace$/i                                  ? 'Eiffel' :        #
            /^(?:f(?:or)?|f(?:77|90|95|0[38]|18)?)$/i ? 'Fortran' :       #
            /^m+$/i                                   ? 'ObjectiveC' :    #
            /^p$/i                                    ? 'P' :             #
            /^v$/i                                    ? 'VLang' :         #
            ();
        ( 'Affix::Compiler::File::' . ${language} )->new( path => $file );
    }
    #
    method compile() {
        @objs = grep {defined} map { $_->compile($flags) } @$source;
    }

    method link() {
        return () unless grep { $_->exists } @objs;
        system( $linker, $flags->{ldflags} // (), '-shared', '-o', $libname->stringify, ( map { $_->absolute->stringify } @objs ) ) ? () : $libname;
    }

    #~ field $cxx;
    #~ field $d;
    #~ field $crystal;
};

class Affix::Compiler::File {
    use Config     qw[%Config];
    use Path::Tiny qw[];
    field $path : reader : param;
    field $flags : reader : param //= ();
    field $obj : reader : param   //= ();
    ADJUST {
        $path = Path::Tiny::path($path) unless builtin::blessed $path;
        $obj //= $path->sibling( $path->basename(qr/\..+?$/) . $Config{_o} );
    }
    method compile() {...}
}

class Affix::Compiler::File::CPP : isa(Affix::Compiler::File) {

    # https://learn.microsoft.com/en-us/cpp
    # https://gcc.gnu.org/
    # https://clang.llvm.org/
    #~ https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-compiler.html
    #~ https://www.ibm.com/products/c-and-c-plus-plus-compiler-family
    #~ https://docs.oracle.com/cd/E37069_01/html/E37073/gkobs.html
    #~ https://www.edg.com/c
    #~ https://www.circle-lang.org/site/index.html
    field $compiler : reader : param //= Affix::Compiler::_can_run qw[g++]

        #~ clang++ cl icpx ibm-clang++ CC eccp circle]
        ;

    method compile($flags) {
        system( $compiler, '-g', '-c', '-fPIC', $flags->{cxxflags} // (), $self->path, '-o', $self->obj ) ? () : $self->obj;
    }
}

class Affix::Compiler::File::C : isa(Affix::Compiler::File) {
    use Config qw[%Config];
    field $compiler : reader : param //= Affix::Compiler::_can_run $Config{cc}, qw[gcc]

        #~ clang cl icx ibm-clang CC eccp circle]
        ;

    method compile($flags) {
        system( $compiler, '-g', '-c', '-Wall', '-fPIC', $flags->{cflags} // (), $self->path, '-o', $self->obj ) ? () : $self->obj;
    }
}

class Affix::Compiler::File::Fortran : isa(Affix::Compiler::File) {

    # GNU, Intel, Intel Classic
    my $compiler = Affix::Compiler::_can_run qw[gfortran ifx ifort];

    method compile($flags) {
        my $obj = $self->obj;
        my $src = $self->path;
        warn qq`gfortran -shared -o $obj $src`;
        `gfortran -shared -o $obj $src`;
        $obj;

        #~ $self->obj
        #~ unless system grep {defined} $compiler, '-shared', ( Affix::Platform::Windows() ? () : '-fPIC' ), $flags->{fflags} // (), $self->path,
        #~ '-o', $self->obj;
    }
}

class Affix::Compiler::File::D : isa(Affix::Compiler::File) {
    use Config qw[%Config];
    field $compiler : reader : param //= Affix::Compiler::_can_run qw[dmd];

    method compile($flags) {
        system( $compiler, '-c', ( Affix::Platform::Windows() ? () : '-fPIC' ), $flags->{dflags} // (), $self->path, '-of=' . $self->obj ) ? () :
            $self->obj;
    }
}

class Affix::Compiler::FortranXXXXXX : isa(Affix::Compiler) {
    use Config     qw[%Config];
    use IPC::Cmd   qw[can_run];
    use Path::Tiny qw[path];
    field $exe : reader;
    field $compiler : reader;
    field $linker : reader;
    #
    ADJUST {
        if ( $exe = can_run('gfortran') ) {
            $compiler = method( $file, $obj, $flags ) {
                system $self->exe, qw[-c -fPIC], $file;
                die "failed to execute: $!\n"                                                                           if $? == -1;
                die sprintf "child died with signal %d, %s coredump\n", ( $? & 127 ), ( $? & 128 ) ? 'with' : 'without' if $? & 127;
                $obj
            };
            $linker = method($objs) {
                system $self->exe, qw[-shared], ( map { $_->stringify } @$objs ), '-o blah.so';
                die "failed to execute: $!\n"                                                                           if $? == -1;
                die sprintf "child died with signal %d, %s coredump\n", ( $? & 127 ), ( $? & 128 ) ? 'with' : 'without' if $? & 127;
                'ok!'
            };
        }
        elsif ( $exe = can_run('ifx') )   { }
        elsif ( $exe = can_run('ifort') ) { }
    }
    #
    method compile( $file, $obj //= (), $flags //= '' ) {
        $file = path($file)->absolute unless builtin::blessed $file;
        $obj //= $file->sibling( $file->basename(qr/\..+?$/) . $Config{_o} );
        try {
            return $compiler->( $self, $file, $obj, $flags );
        }
        catch ($err) { warn $err; }
    }

    method link($objs) {
        $objs = [ map { builtin::blessed $_ ? $_ : path($_)->absolute } @$objs ];
        return () unless @$objs;
        try {
            return $linker->( $self, $objs );
        }
        catch ($err) { warn $err; }
    }
}

class Affix::Compiler::File::Dxxx {
    use Config     qw[%Config];
    use IPC::Cmd   qw[can_run];
    use Path::Tiny qw[];
    field $exe : reader;
    field $compiler : reader;
    field $linker : reader;
    field $path : reader : param;
    #
    ADJUST {
        if ( $exe = can_run('dmd') ) {
            $compiler = method( $file, $obj, $flags ) {
                system $self->exe, qw[-c -fPIC], $file->stringify;
                die "failed to execute: $!\n"                                                                           if $? == -1;
                die sprintf "child died with signal %d, %s coredump\n", ( $? & 127 ), ( $? & 128 ) ? 'with' : 'without' if $? & 127;
                $obj
            };
            $linker = method($objs) {
                system $self->exe, qw[-shared], ( map { $_->stringify } @$objs ), '-o blah.so';
                die "failed to execute: $!\n"                                                                           if $? == -1;
                die sprintf "child died with signal %d, %s coredump\n", ( $? & 127 ), ( $? & 128 ) ? 'with' : 'without' if $? & 127;
                'ok!'
            };
        }
    }
    #
    method compile( $file, $obj //= (), $flags //= '' ) {
        $file = Path::Tiny::path($file)->absolute unless builtin::blessed $file;
        $obj //= $file->sibling( $file->basename(qr/\..+?$/) . $Config{_o} );
        try {
            return $compiler->( $self, $file->stringify, $obj, $flags );
        }
        catch ($err) { warn $err; }
    }

    method link($objs) {
        $objs = [ map { builtin::blessed $_ ? $_ : Path::Tiny::path($_)->absolute } @$objs ];
        return () unless @$objs;
        try {
            return $linker->( $self, $objs );
        }
        catch ($err) { warn $err; }
    }
}
1;
__END__
use v5.40;
use feature qw[class];
no warnings qw[experimental::class experimental::try];
use Data::Printer;
use Path::Tiny qw[path]; # All path and file handling should be done with Path::Tiny
#~ use Carp::Always;
$|++;
#
class My::Compiler {
    use Config     qw[%Config];
    use Path::Tiny qw[path tempdir];
    use File::Spec;
    use ExtUtils::MakeMaker;
    #
    field $source : param : reader;
    field $libname: param: reader;
    #
    field $linker:reader;
    #
    ADJUST{
        # if $source is a list, make sure they're all ABI compatible
        # convert files in $source to My::Compiler::File subclass objects based on file extension unless they are blessed
        # locate system linker for c compatible ABI (ld, etc.)
    }
    #
    method build(){
        # Compile all files by calling the compile() method on files in $source
        # Link objects into a shared library
        # Return the linked shared library if everything built okay
    }
}
class My::Compiler::File {
    field $path : param: reader;

#
    sub _can_run(@cmd) {
        state $paths //= [ map { $_->realpath } grep { $_->exists } map { path($_) } File::Spec->path ];
        for my $exe (@cmd) {
            grep { return path($_) if $_ = MM->maybe_command($_) } $exe, map { $_->child($exe) } @$paths;
        }
    }
    #
    field $linker : reader : param //= _can_run qw[g++ ld];
    #
}
class My::Compiler::File::C :isa(My::Compiler::File) {
    field $c : reader : param   //= _can_run qw[gcc clang cc icc icpx cl eccp];
    field $abi : reader = 'GNU_C';
}
class My::Compiler::File::CPP :isa(My::Compiler::File)  {     field $cpp : reader : param //= _can_run qw[g++ clang++ c++ icpc icpx cl eccp];
 }
class My::Compiler::File::Fortran :isa(My::Compiler::File)  {     #~ https://dlang.org/articles/dll-linux.html#dso9
    #~ dmd -c dll.d -fPIC
    #~ dmd -oflibdll.so dll.o -shared -defaultlib=libphobos2.so -L-rpath=/path/to/where/shared/library/is
    field $d : reader : param       //= _can_run qw[dmd];
    field $fortran : reader : param //= _can_run qw[gfortran ifx ifort];
 }
class My::Compiler::File::DLang :isa(My::Compiler::File)  { ... }
class My::Compiler::File::Crystal :isa(My::Compiler::File)  {     #~ https://github.com/crystal-lang/crystal/issues/921#issuecomment-2413541412
    field $crystal : reader : param //= _can_run qw[crystal];
 }
class My::Compiler::File::COBOL :isa(My::Compiler::File)  {     # cobc: https://gnucobol.sourceforge.io/
    field $cobol : reader : param //= _can_run qw[cobc cobol cob cob2];
 }
class My::Compiler::File::CSharp :isa(My::Compiler::File)  {     #~ https://mazeez.dev/posts/writing-native-libraries-in-csharp
    #~ https://medium.com/@sixpeteunder/how-to-build-a-shared-library-in-c-sharp-and-call-it-from-java-code-6931260d01e5
    field $csharp : reader : param //= _can_run qw[dotnet];
 }
class My::Compiler::File::Circle :isa(My::Compiler::File)  {  #~ https://www.circle-lang.org/site/index.html
    field $circle : reader : param //= _can_run qw[circle];
 }
class My::Compiler::File::C3 :isa(My::Compiler::File)  {
    #~ https://c3-lang.org/build-your-project/build-commands/
    field $c3 : reader : param //= _can_run qw[c3c];
 }
class My::Compiler::File::ASM :isa(My::Compiler::File)  {     #~ https://fasterthanli.me/series/making-our-own-executable-packer/part-5
    #~ https://stackoverflow.com/questions/71704813/writing-and-linking-shared-libraries-in-assembly-32-bit
    #~ https://github.com/therealdreg/nasm_linux_x86_64_pure_sharedlib
    field $asm : reader : param //= _can_run qw[nasm as];
 }
class My::Compiler::File::ADA :isa(My::Compiler::File)  {     #~ https://gcc.gnu.org/onlinedocs/gcc-3.4.0/gnat_ug_unx/Creating-an-Ada-Library.html
    field $ada : reader : param //= _can_run qw[gnatmake];
 }
class My::Compiler::File::Eiffel :isa(My::Compiler::File)  {
    #~ https://wiki.liberty-eiffel.org/index.php/Compile
    #~ https://svn.eiffel.com/eiffelstudio-public/branches/Eiffel_54/Delivery/docs/papers/dll.html
    field $eiffel : reader : param //= _can_run qw[se];
 }
class My::Compiler::File::FSharp :isa(My::Compiler::File)  {
    #~ https://github.com/secana/Native-FSharp-Library
    #~ https://secanablog.wordpress.com/2020/02/01/writing-a-native-library-in-f-which-can-be-called-from-c/
    field $fsharp : reader : param //= _can_run qw[dotnet];
 }
class My::Compiler::File::Futhark :isa(My::Compiler::File)  {     #~ https://futhark.readthedocs.io/en/stable/usage.html
    field $futhark : reader : param //= _can_run qw[futhark];    # .fut => .c
 }
class My::Compiler::File::Go :isa(My::Compiler::File)  {     #~ https://medium.com/@walkert/fun-building-shared-libraries-in-go-639500a6a669
    #~ https://github.com/vladimirvivien/go-cshared-examples
    field $go : reader : param //= _can_run qw[go];
 }
class My::Compiler::File::Haskell :isa(My::Compiler::File)  {
    #~ https://github.com/bennoleslie/haskell-shared-example
    #~ https://www.hobson.space/posts/haskell-foreign-library/
    field $haskell : reader : param //= _can_run qw[ghc cabal];
    }
class My::Compiler::File::Nim :isa(My::Compiler::File)  {     #~ https://peterme.net/dynamic-libraries-in-nim.html
    field $nim : reader : param //= _can_run qw[nim];    # .nim => .c
 }
class My::Compiler::File::Odin :isa(My::Compiler::File)  {   #~ https://odin-lang.org/news/calling-odin-from-python/
    field $odin : reader : param //= _can_run qw[odin];
 }
class My::Compiler::File::PLang :isa(My::Compiler::File)  {     #~ https://p-org.github.io/P/getstarted/install/#step-4-recommended-ide-optional
    #~ https://p-org.github.io/P/getstarted/usingP/#compiling-a-p-program
    field $p : reader : param //= _can_run qw[p];    # .p => C#
 }
class My::Compiler::File::Rust :isa(My::Compiler::File)  {
    #~ https://blog.asleson.org/2021/02/23/how-to-writing-a-c-shared-library-in-rust/
    field $rust : reader : param //= _can_run qw[cargo]; }
class My::Compiler::File::Swift :isa(My::Compiler::File)  {
    #~ swiftc point.swift -emit-module -emit-library
    #~ https://forums.swift.org/t/creating-a-c-accessible-shared-library-in-swift/45329/5
    #~ https://theswiftdev.com/building-static-and-dynamic-swift-libraries-using-the-swift-compiler/#should-i-choose-dynamic-or-static-linking
    field $swift : reader : param //= _can_run qw[swiftc];
    }
class My::Compiler::File::VLang :isa(My::Compiler::File)  {
    #~ https://www.rangakrish.com/index.php/2023/04/02/building-v-language-dll/
    #~ https://dev.to/piterweb/how-to-create-and-use-dlls-on-vlang-1p13
    field $v : reader : param //= _can_run qw[v];
 }
class My::Compiler::File::Zig :isa(My::Compiler::File)  {
    #~ https://ziglang.org/documentation/0.13.0/#Exporting-a-C-Library
    #~ zig build-lib mathtest.zig -dynamic
    field $exe : reader : param //= _can_run qw[zig];
    # Fill in the blanks
}




