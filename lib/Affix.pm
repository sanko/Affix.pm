package Affix 0.50 {    # 'FFI' is my middle name!
    use v5.38;
    use feature 'class';
    no warnings 'experimental::class';
    use Carp qw[];
    use vars qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use XSLoader;
    use Exporter 'import';
    @EXPORT_OK   = qw[Void Bool Int Struct Pointer affix wrap];    # symbols to export on request
    %EXPORT_TAGS = ( all => \@EXPORT_OK );
    #
    my $ok = XSLoader::load();
    #
    class Affix::Type { }

    class Affix::Type::Void : isa(Affix::Type) { }

    class Affix::Type::Bool : isa(Affix::Type) { }

    class Affix::Type::Int : isa(Affix::Type) { }

    class Affix::Type::Struct : isa(Affix::Type) {
        field $fields : param;
    }

    class Affix::Type::Pointer : isa(Affix::Type) {
        field $type : param;
    }
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
    {
        # Perl isn't parsing this as a sub with a single arg when signatures are enabled. So that...
        # affix( Pointer[Int], Int, Int ) parses as affix( Pointer(Int, Int, Int) ); which is... wrong
        no experimental 'signatures';
        sub Pointer ( $) { Affix::Type::Pointer->new( type => shift ) }
    }
    #
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
}
1;
