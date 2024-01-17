package Affix 0.12 {    # 'FFI' is my middle name!
    use v5.38;
    use feature 'class';
    no warnings 'experimental::class';
    use Carp qw[];
    use vars qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use DynaLoader;
    my $okay = 0;    # True on load
    use Exporter 'import';
    @EXPORT_OK = qw[
        Void
        Bool
        Char UChar SChar WChar
        Short UShort
        Int UInt
        Long ULong
        LongLong ULongLong
        Float Double
        Size_t SSize_t
        String WString
        Struct Array
        Pointer
        Callback
        SV
        affix wrap pin unpin
        malloc calloc realloc free memchr memcmp memset memcpy sizeof offsetof
        raw hexdump
    ];
    %EXPORT_TAGS = ( all => \@EXPORT_OK );
    #
    class Affix::Lib {
        field $path : param;
        field $abi : param //= Affix::Platform::ABI_Itanium->new();
        field $ver : param //= ();
    }
    #
    class Affix::Type {
        method check ($value)          {...}
        method cast  ( $from, $value ) {...}
        method sizeof() {...}
    }

    class Affix::Type::Void : isa(Affix::Type) { }

    class Affix::Type::Bool : isa(Affix::Type) { }

    class Affix::Type::Char : isa(Affix::Type) { }

    class Affix::Type::UChar : isa(Affix::Type) { }

    class Affix::Type::SChar : isa(Affix::Type) { }

    class Affix::Type::WChar : isa(Affix::Type) { }

    class Affix::Type::Short : isa(Affix::Type) { }

    class Affix::Type::UShort : isa(Affix::Type) { }

    class Affix::Type::Int : isa(Affix::Type) { }

    class Affix::Type::UInt : isa(Affix::Type) { }

    class Affix::Type::Long : isa(Affix::Type) { }

    class Affix::Type::ULong : isa(Affix::Type) { }

    class Affix::Type::LongLong : isa(Affix::Type) { }

    class Affix::Type::ULongLong : isa(Affix::Type) { }

    class Affix::Type::Float : isa(Affix::Type) { }

    class Affix::Type::Double : isa(Affix::Type) { }

    class Affix::Type::Size_t : isa(Affix::Type) { }

    class Affix::Type::SSize_t : isa(Affix::Type) { }

    class Affix::Type::String : isa(Affix::Type) { }

    class Affix::Type::WString : isa(Affix::Type) { }

    class Affix::Type::Struct : isa(Affix::Type) {
        field $fields : param;
    }

    class Affix::Type::Array : isa(Affix::Type) {
        field $type : param;
        field $size : param //= ();
    }

    class Affix::Type::Pointer : isa(Affix::Type) {
        field $type : param;
    }

    class Affix::Type::Callback : isa(Affix::Type) {
        field $args : param;
        field $returns : param;
    }

    class Affix::Type::SV : isa(Affix::Type) { }
    #
    class Affix::Function 1 {
        field $lib : param;
        field $symbol : param;
        field $args : param    //= [];
        field $returns : param //= Void();
        #
        ADJUST { warn 'adjust' }
        method DESTROY ( $global = 0 ) { warn 'destroy ', ref $self; }
        #
        method call (@args) { warn 'call' }
    }

    # ABI system
    class Affix::ABI::D {
        method mangle ( $name, $args, $ret ) {...}
    }

    class Affix::ABI::Fortran {
        method mangle ( $name, $args, $ret ) {...}
    }

    class Affix::ABI::Itanium {
        method mangle ( $name, $args, $ret ) {...}
    }

    class Affix::ABI::Microsoft {
        method mangle ( $name, $args, $ret ) {...}
    }

    class Affix::ABI::Rust : isa(Affix::ABI::Itanium) {
        method mangle ( $name, $args, $ret ) {...}
    }

    class Affix::ABI::Swift {
        method mangle ( $name, $args, $ret ) {...}
    }

    # Functions with signatures must follow classes until https://github.com/Perl/perl5/pull/21159
    # Type system
    sub Void()      { Affix::Type::Void->new() }
    sub Bool()      { Affix::Type::Bool->new() }
    sub Char()      { Affix::Type::Char->new() }
    sub UChar()     { Affix::Type::UChar->new() }
    sub SChar()     { Affix::Type::SChar->new() }
    sub WChar()     { Affix::Type::WChar->new() }
    sub Short()     { Affix::Type::Short->new() }
    sub UShort()    { Affix::Type::UShort->new() }
    sub Int()       { Affix::Type::Int->new() }
    sub UInt()      { Affix::Type::UInt->new() }
    sub Long()      { Affix::Type::Long->new() }
    sub ULong()     { Affix::Type::ULong->new() }
    sub LongLong()  { Affix::Type::LongLong->new() }
    sub ULongLong() { Affix::Type::ULongLong->new() }
    sub Float()     { Affix::Type::Float->new() }
    sub Double()    { Affix::Type::Double->new() }
    sub Size_t()    { Affix::Type::Size_t->new() }
    sub SSize_t()   { Affix::Type::SSize_t->new() }
    sub String()    { Affix::Type::String->new() }
    sub WString()   { Affix::Type::WString->new() }
    sub Struct   (%fields) { Affix::Type::Struct->new( fields => \%fields ) }
    sub Array    ( $type, $size    //= () )   { Affix::Type::Array->new( type => $type, size => $size ) }
    sub Callback ( $args, $returns //= Void ) { Affix::Type::Callback->new( args => $args, returns => $returns ) }
    sub SV() { Affix::Type::SV->new() }
    {
        # Perl isn't parsing this as a sub with a single arg when signatures are enabled. So that...
        # affix( Pointer[Int], Int, Int ) parses as affix( Pointer(Int, Int, Int) ); which is... wrong
        no experimental 'signatures';
        sub Pointer ( $) { Affix::Type::Pointer->new( type => shift ) }
    }

    # Core
    sub affix ( $lib, $symbol, $args //= [], $returns //= Void ) {
        my $affix = Affix::Function->new( lib => $lib, symbol => $symbol, args => $args, returns => $returns );
        my ($pkg) = caller(0);
        {
            no strict 'refs';
            *{ $pkg . '::' . $symbol } = sub { $affix->call(@_) };
        }
        $affix;
    }

    sub wrap ( $lib, $symbol, $args //= [], $returns //= Void ) {
        Affix::Function->new( lib => $lib, symbol => $symbol, args => $args, returns => $returns );
    }
    sub pin   ( $lib, $symbol, $type, $variable ) {...}
    sub unpin ($variable)                         {...}

    # Memory functions
    sub malloc   ($size)                 {...}
    sub calloc   ( $num, $size )         {...}
    sub realloc  ( $ptr, $size )         {...}
    sub free     ($ptr)                  {...}
    sub memchr   ( $ptr, $chr, $count )  {...}
    sub memcmp   ( $lhs, $rhs, $count )  {...}
    sub memset   ( $dest, $src, $count ) {...}
    sub memcpy   ( $dest, $src, $count ) {...}
    sub sizeof   ($type)                 {...}
    sub offsetof ( $type, $field )       {...}
    sub raw      ( $ptr, $size )         {...}
    sub hexdump  ( $ptr, $size )         {...}

    # Internals
    sub locate_lib    ( $lib, @dirs ) { DynaLoader::dl_findfile($lib); }
    sub locate_symbol ($name)         {...}

    # Let's go
    sub dl_load_flags ($modulename) {0}
    $okay = DynaLoader::bootstrap(__PACKAGE__);
}
1;
