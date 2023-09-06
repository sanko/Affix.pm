package Affix 0.12 {    # 'FFI' is my middle name!

    # ABSTRACT: A Foreign Function Interface eXtension
    use strict;
    use warnings;

    #~ no warnings 'redefine';
    use File::Spec::Functions qw[rel2abs canonpath curdir path catdir];
    use File::Basename        qw[basename dirname];
    use File::Find            qw[find];
    use Config;
    use Carp qw[];
    use vars qw[@EXPORT_OK @EXPORT %EXPORT_TAGS];
    use XSLoader;
    use Affix::Type;

    #~ our $VMSize = 1024; # defaults to 8192; passed to dcNewCallVM( ... )
    my $ok = XSLoader::load();
    #
    use parent 'Exporter';
    $EXPORT_TAGS{types} = [
        'Void', 'SV',

        # Aggregates
        'Struct', 'Array', 'Union',

        # Numerics
        'Bool',     'Char', 'UChar', 'WChar', 'Short', 'UShort', 'Int', 'UInt', 'Long', 'ULong',
        'LongLong', 'ULongLong', 'Float', 'Double', 'Size_t', 'SSize_t',

        # Enumerations
        'Enum', 'IntEnum', 'UIntEnum', 'CharEnum',

        # Pointers
        'String', 'WString', 'StdString', 'Pointer', 'CodeRef'
    ];

    #~ use Data::Dump;
    #~ ddx \@EXPORT_OK;
    #~ ddx \@EXPORT;
    #~ ddx \%EXPORT_TAGS;
    #
    our $OS = $^O;
    my $is_win = $OS eq 'MSWin32';
    my $is_mac = $OS eq 'darwin';
    my $is_bsd = $OS =~ /bsd/;
    my $is_sun = $OS =~ /(solaris|sunos)/;

    sub locate_libs {
        my ( $lib, $version ) = @_;
        $lib =~ s[^lib][];
        my $ver;
        if ( defined $version ) {
            require version;
            $ver = version->parse($version);
        }

        #~ warn $lib;
        #~ warn $version;
        #~ warn "Win: $is_win";
        #~ warn "Mac: $is_mac";
        #~ warn "BSD: $is_bsd";
        #~ warn "Sun: $is_sun";
        CORE::state $libdirs;
        if ( !defined $libdirs ) {
            if ($is_win) {
                require Win32;
                $libdirs = [
                    Win32::GetFolderPath( Win32::CSIDL_SYSTEM() ) . '/',
                    Win32::GetFolderPath( Win32::CSIDL_WINDOWS() ) . '/',
                ];
            }
            else {
                $libdirs = [
                    ( split ' ', $Config{libsdirs} ),
                    map { warn $ENV{$_}; split /[:;]/, ( $ENV{$_} ) }
                        grep { $ENV{$_} }
                        qw[LD_LIBRARY_PATH DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH]
                ];
            }
            no warnings qw[once];
            require DynaLoader;
            $libdirs = [
                grep { -d $_ }
                    map { rel2abs($_) }
                    qw[. ./lib ~/lib /usr/local/lib /usr/lib /lib /usr/lib/system],
                @DynaLoader::dl_library_path, @$libdirs
            ];
        }
        CORE::state $regex;
        if ( !defined $regex ) {
            $regex = $is_win ?
                qr/^
        (?:lib)?(?<name>\w+)
        (?:[_-](?<version>[0-9\-\._]+))?_*
        \.$Config{so}
        $/ix :
                $is_mac ?
                qr/^
        (?:lib)?(?<name>\w+)
        (?:\.(?<version>[0-9]+(?:\.[0-9]+)*))?
        \.(?:so|dylib|bundle)
        $/x :    # assume *BSD or linux
                qr/^
        (?:lib)?(?<name>\w+)
        \.$Config{so}
        (?:\.(?<version>[0-9]+(?:\.[0-9]+)*))?
        $/x;
        }
        my %store;

        #~ warn join ', ', @$libdirs;
        my %_seen;
        find(
            0 ?
                sub {    # This is rather slow...
                warn $File::Find::name;
                return if $store{ basename $File::Find::name};

                #~ return if $_seen{basename $File::Find::name}++;
                return if !-e $File::Find::name;
                warn basename $File::Find::name;
                warn;
                $File::Find::prune = 1
                    if !grep { canonpath $_ eq canonpath $File::Find::name } @$libdirs;
                /$regex/ or return;
                warn;
                $+{name} eq $lib or return;
                warn;
                my $lib_ver;
                $lib_ver = version->parse( $+{version} ) if defined $+{version};
                $store{ canonpath $File::Find::name} = {
                    %+,
                    path => $File::Find::name,
                    ( defined $lib_ver ? ( version => $lib_ver ) : () )
                    }
                    if ( defined($ver) && defined($lib_ver) ? $lib_ver == $ver : 1 );
                } :
                sub {
                $File::Find::prune = 1
                    if !grep { canonpath $_ eq canonpath $File::Find::name } @$libdirs;

                #~ return                 if -d $_;
                return unless $_ =~ $regex;
                return unless defined $+{name};
                return unless $+{name} eq $lib;
                return unless -B $File::Find::name;
                my $lib_ver;
                $lib_ver = version->parse( $+{version} ) if defined $+{version};
                return unless ( defined $lib_ver && defined($ver) ? $ver == $lib_ver : 1 );

                #~ use Data::Dump;
                #~ warn $File::Find::name;
                #~ ddx %+;
                $store{ canonpath $File::Find::name} //= {
                    %+,
                    path => $File::Find::name,
                    ( defined $lib_ver ? ( version => $lib_ver ) : () )
                };
                },
            @$libdirs
        );
        values %store;
    }

    sub locate_lib {
        my ( $name, $version ) = @_;
        return $name if $name && -B $name;
        CORE::state $cache //= {};
        return $cache->{$name}{ $version // '' }->{path}
            if defined $cache->{$name}{ $version // '' };
        if ( !$version ) {
            return $cache->{$name}{''}{path} = rel2abs($name) if -B rel2abs($name);
            return $cache->{$name}{''}{path} = rel2abs( $name . '.' . $Config{so} )
                if -B rel2abs( $name . '.' . $Config{so} );
        }
        my $libname = basename $name;
        $libname =~ s/^lib//;
        $libname =~ s/\..*$//;
        return $cache->{$libname}{ $version // '' }->{path}
            if defined $cache->{$libname}{ $version // '' };
        my @libs = locate_libs( $name, $version );

        #~ warn;
        #~ use Data::Dump;
        #~ warn join ', ', @_;
        #~ ddx \@_;
        #~ ddx $cache;
        if (@libs) {
            ( $cache->{$name}{ $version // '' } ) = @libs;
            return $cache->{$name}{ $version // '' }->{path};
        }
        ();
    }
    {
        # ctypes util
        sub padding_needed_for {
            my ( $offset, $alignment ) = @_;
            return $alignment unless $offset;
            return 0          unless $alignment;
            my $misalignment = $offset % $alignment;
            return $alignment - $misalignment
                if $misalignment;    # round to the next multiple of $alignment
            return 0;                # already a multiple of $alignment
        }
        #
        sub typedef {
            my ( $new, $original ) = @_;
            no strict 'refs';
            no warnings 'once';

            #~ use diagnostics;
            *{$new} = sub {$original};
            @{ 'Affix::Type::' . $new . '::ISA' } = 'Affix::Type::' . $original;
            push @{ $EXPORT_TAGS{types} }, $new;
        }

        sub Void() {    # could use state var if we didn't use the objects to store offset, etc.
            bless( [ 'Void', VOID_FLAG(), 0, 0, undef ], 'Affix::Type::Void' );
        }

        sub Bool() {
            bless( [ 'Bool', BOOL_FLAG(), BOOL_SIZE(), BOOL_ALIGN(), undef ], 'Affix::Type::Bool' );
        }

        sub Char() {
            bless( [ 'Char', CHAR_FLAG(), CHAR_SIZE(), CHAR_ALIGN(), undef ], 'Affix::Type::Char' );
        }

        sub SChar() {
            bless( [ 'SChar', SCHAR_FLAG(), CHAR_SIZE(), CHAR_ALIGN(), undef ],
                'Affix::Type::SChar' );
        }

        sub UChar() {
            bless( [ 'UChar', UCHAR_FLAG(), UCHAR_SIZE(), UCHAR_ALIGN(), undef ],
                'Affix::Type::UChar' );
        }

        sub WChar() {
            bless( [ 'WChar', WCHAR_FLAG(), WCHAR_SIZE(), WCHAR_ALIGN(), undef ],
                'Affix::Type::WChar' );
        }

        sub Short() {
            bless( [ 'Short', SHORT_FLAG(), SHORT_SIZE(), SHORT_ALIGN(), undef ],
                'Affix::Type::Short' );
        }

        sub UShort() {
            bless( [ 'UShort', USHORT_FLAG(), USHORT_SIZE(), USHORT_ALIGN(), undef ],
                'Affix::Type::UShort' );
        }

        sub Int () {
            bless( [ 'Int', INT_FLAG(), INT_SIZE(), INT_ALIGN(), undef ], 'Affix::Type::Int' );
        }

        sub UInt () {
            bless( [ 'UInt', UINT_FLAG(), UINT_SIZE(), UINT_ALIGN(), undef ], 'Affix::Type::UInt' );
        }

        sub Long () {
            bless( [ 'Long', LONG_FLAG(), LONG_SIZE(), LONG_ALIGN(), undef ], 'Affix::Type::Long' );
        }

        sub ULong () {
            bless( [ 'ULong', ULONG_FLAG(), ULONG_SIZE(), ULONG_ALIGN(), undef ],
                'Affix::Type::ULong' );
        }

        sub LongLong () {
            bless( [ 'LongLong', LONGLONG_FLAG(), LONGLONG_SIZE(), LONGLONG_ALIGN(), undef ],
                'Affix::Type::LongLong' );
        }

        sub ULongLong () {
            bless( [ 'ULongLong', ULONGLONG_FLAG(), ULONGLONG_SIZE(), ULONGLONG_ALIGN(), undef ],
                'Affix::Type::ULongLong' );
        }

        sub Float () {
            bless( [ 'Float', FLOAT_FLAG(), FLOAT_SIZE(), FLOAT_ALIGN(), undef ],
                'Affix::Type::Float' );
        }

        sub Double () {
            bless( [ 'Double', DOUBLE_FLAG(), DOUBLE_SIZE(), DOUBLE_ALIGN(), undef ],
                'Affix::Type::Double' );
        }

        sub Size_t () {
            bless( [ 'Size_t', SSIZE_T_FLAG(), SIZE_T_SIZE(), SIZE_T_ALIGN(), undef ],
                'Affix::Type::Size_t' );
        }

        sub SSize_t () {
            bless( [ 'SSize_t', SSIZE_T_FLAG(), SSIZE_T_SIZE(), SSIZE_T_ALIGN(), undef ],
                'Affix::Type::SSize_t' );
        }

        sub String () {
            bless( [ 'String', STRING_FLAG(), INTPTR_T_SIZE(), INTPTR_T_ALIGN(), undef ],
                'Affix::Type::String' );
        }

        sub WString () {
            bless( [ 'String', WSTRING_FLAG(), INTPTR_T_SIZE(), INTPTR_T_ALIGN(), undef ],
                'Affix::Type::WString' );
        }

        sub StdString () {
            bless( [ 'StdString', STD_STRING_FLAG(), INTPTR_T_SIZE(), INTPTR_T_ALIGN(), undef ],
                'Affix::Type::StdString' );
        }

        sub Struct ($) {
            my (@types) = @{ +shift };
            my @fields;
            my $sizeof = 0;
            my $packed = 0;

            #~ for my ( $field, $type )(@types) { # Perl 5.36
            for ( my $i = 0; $i < $#types; $i += 2 ) {
                my $field = $types[$i];
                my $type  = $types[ $i + 1 ];
                push @fields, sprintf '%s => %s', $field, $type;
                my $__sizeof = $type->sizeof;
                my $__align  = $type->align;
                $sizeof += $packed ? 0 :
                    padding_needed_for( $sizeof, $__align > $__sizeof ? $__sizeof : $__align );
                $type->[4] = $sizeof;    # offset
                $sizeof += $__sizeof;
            }
            bless(
                [   sprintf( 'Struct[ %s ]', join ', ', @fields ),
                    STRUCT_FLAG(), $sizeof, $sizeof + padding_needed_for( $sizeof, BYTE_ALIGN() ),
                    \@types
                ],
                'Affix::Type::Struct'
            );
        }

        # TODO: CPPStruct
        sub Union ($) {
            my (@types) = @{ +shift };
            my @fields;
            my $sizeof    = 0;
            my $alignment = 0;
            my $packed    = 0;

            #~ for my ( $field, $type )(@types) { # Perl 5.36
            for ( my $i = 0; $i < $#types; $i += 2 ) {
                my $field = $types[$i];
                my $type  = $types[ $i + 1 ];
                push @fields, sprintf '%s => %s', $field, $type;
                my $__sizeof = $type->sizeof;
                if ( $sizeof < $__sizeof ) {
                    $sizeof    = $__sizeof;
                    $alignment = $type->align;
                }
            }
            bless(
                [   sprintf( 'Union[ %s ]', join ', ', @fields ),
                    UNION_FLAG(), $sizeof, $alignment, \@types
                ],
                'Affix::Type::Union'
            );
        }

        sub Array ($) {    # [ text, id, size, align, offset, subtype, length, aggregate ]
            my ( $type, $size ) = @{ +shift };
            my $sizeof = undef;
            my $packed = 0;
            if ($size) {
                my $__sizeof = $type->sizeof;
                my $__align  = $type->align;
                for ( 0 ... $size ) {
                    $sizeof += $packed ? 0 :
                        padding_needed_for( $sizeof, $__align > $__sizeof ? $__sizeof : $__align );
                    $sizeof += $__sizeof;
                }
            }
            bless(
                [   sprintf( 'Array[ %s%s ]', $type, defined($size) ? ', ' . $size : '' ),
                    ARRAY_FLAG(), $sizeof, undef, undef, $type, $size, undef
                ],
                'Affix::Type::Array'
            );
        }

        sub CodeRef($) {
            my (@elements) = @{ +shift };
            my ( $args, $ret ) = @elements;
            bless(
                [   sprintf( 'CodeRef[ [ %s ] => %s ]', join( ', ', @$args ), $ret ),
                    CODEREF_FLAG(), INTPTR_T_SIZE(), INTPTR_T_ALIGN(), $args, $ret
                ],
                'Affix::Type::CodeRef'
            );
        }

        sub Pointer (;$) {    # [ text, id, size, align, offset, subtype, sizeof, package ]
            my $sizeof  = 0;
            my $packed  = 0;
            my $subtype = undef;
            if (@_) {
                ($subtype) = @{ +shift };
                my $__sizeof = $subtype->sizeof;
                my $__align  = $subtype->align;
                $sizeof += $packed ? 0 :
                    padding_needed_for( $sizeof, $__align > $__sizeof ? $__sizeof : $__align );
                $sizeof += $__sizeof;
            }
            else {
                $subtype = Void();    # Defaults to Pointer[Void]
            }
            bless(
                [   'Pointer[ ' . $subtype . ' ]',
                    POINTER_FLAG(), INTPTR_T_SIZE(), INTPTR_T_ALIGN(), undef, $subtype, $sizeof,
                    undef
                ],
                'Affix::Type::Pointer'
            );
        }

        sub SV () {    # Should only be used inside of a Pointer[]
            bless( [ 'SV', SV_FLAG(), 0, 0, undef ], 'Affix::Type::SV' );
        }

        sub _Enum($) {
            my (@elements) = @{ +shift };
            my $fields;
            my $index = 0;
            my $enum;
            for my $element (@elements) {
                ( $element, $index ) = @$element if ref $element eq 'ARRAY';
                if ( $index =~ /[+|-|\*|\/|^|%|\D]/ ) {
                    $index =~ s[(\w+)][$enum->{$1}//$1]xeg;
                    $index = eval $index;
                }
                $enum->{$element} = $index;
                push @$fields, sprintf '%s => %s', $element, $index++;
            }
            return $fields, $enum;
        }

        sub Enum($) {
            my ( $fields, $enum ) = &_Enum;
            bless(
                [   sprintf( 'Enum[ %s ]', join ', ', @$fields ),
                    'i', Int->sizeof, Int->sizeof, $enum
                ],
                'Affix::Type::Enum'
            );
        }

        sub IntEnum($) {
            my ( $fields, $enum ) = &_Enum;
            bless(
                [   sprintf( 'IntEnum[ %s ]', join ', ', @$fields ),
                    'i', Int->sizeof, Int->sizeof, $enum
                ],
                'Affix::Type::IntEnum'
            );
        }

        sub UIntEnum($) {
            my ( $fields, $enum ) = &_Enum;
            bless(
                [   sprintf( 'UIntEnum[ %s ]', join ', ', @$fields ),
                    'j', UInt->sizeof, UInt->sizeof, $enum
                ],
                'Affix::Type::UIntEnum'
            );
        }

        sub CharEnum($) {
            my (@elements) = @{ +shift };
            my @fields;
            my $index = 0;
            my %enum;
            for my $element (@elements) {
                ( $element, $index ) = @$element if ref $element eq 'ARRAY';
                if ( $index =~ /[+|-|\*|\/|^|%]/ ) {
                    $index =~ s[(\w+)][$enum{$1}//$1]xeg;
                    $index =~ s[\b(\D)\b][ord $1]xeg;
                    $index = eval $index;
                }
                $enum{$element} = $index =~ /\D/ ? ord $index : $index;
                push @fields, sprintf '%s => %s', $element, $enum{$element};
                $index++;
            }
            bless(
                [   sprintf( 'CharEnum[ %s ]', join ', ', @fields ),
                    'c', UInt->sizeof, UInt->sizeof, \%enum
                ],
                'Affix::Type::CharEnum'
            );
        }
        #
        typedef ShortInt     => Short;
        typedef SShort       => Short;
        typedef SShortInt    => Short;
        typedef UShortInt    => UShort;
        typedef Signed       => Int;
        typedef SInt         => Int;
        typedef Unsigned     => UInt;
        typedef LongInt      => Long;
        typedef SLongInt     => Long;
        typedef LongLongInt  => LongLong;
        typedef SLongLong    => LongLong;
        typedef SLongLongInt => LongLong;
        typedef ULongLongInt => ULongLong;
        typedef Str          => String;
        typedef WStr         => WString;
        #
        typedef i8  => Char;
        typedef u8  => UChar;
        typedef i16 => Short;
        typedef u16 => UShort;
        typedef i32 => Int;
        typedef u32 => UInt;
        typedef i64 => LongLong;
        typedef u64 => ULongLong;
        #
        typedef wchar_t => WChar;
        #
        @Affix::Type::Void::ISA = @Affix::Type::SV::ISA

            # Aggregates
            = @Affix::Type::Struct::ISA = @Affix::Type::Array::ISA = @Affix::Type::Union::ISA

            # Numerics
            = @Affix::Type::Bool::ISA  = @Affix::Type::Char::ISA     = @Affix::Type::UChar::ISA
            = @Affix::Type::WChar::ISA = @Affix::Type::Short::ISA    = @Affix::Type::UShort::ISA
            = @Affix::Type::Int::ISA   = @Affix::Type::UInt::ISA     = @Affix::Type::Long::ISA
            = @Affix::Type::ULong::ISA = @Affix::Type::LongLong::ISA = @Affix::Type::ULongLong::ISA
            = @Affix::Type::Float::ISA = @Affix::Type::Double::ISA   = @Affix::Type::Size_t::ISA
            = @Affix::Type::SSize_t::ISA

            # Enumerations
            = @Affix::Type::Enum::ISA = @Affix::Type::IntEnum::ISA = @Affix::Type::UIntEnum::ISA
            = @Affix::Type::CharEnum::ISA

            # Pointers
            = @Affix::Type::String::ISA = @Affix::Type::WString::ISA = @Affix::Type::StdString::ISA
            = @Affix::Type::Pointer::ISA = @Affix::Type::CodeRef::ISA

            # Typedef'd aliases
            = @Affix::Type::Str::ISA
            #
            = 'Affix::Type';
    }
    {
        #~ ✅ https://gcc.gnu.org/git?p=gcc.git;a=blob_plain;f=gcc/cp/mangle.cc;hb=HEAD
        #~ ✅ https://rust-lang.github.io/rfcs/2603-rust-symbol-name-mangling-v0.html
        #~ ✅ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
        #~ ☑️ https://github.com/apple/swift/blob/main/docs/ABI/Mangling.rst#identifiers
        #~ ☑️ https://mikeash.com/pyblog/friday-qa-2014-08-15-swift-name-mangling.html
        #~ ☑️ https://dlang.org/spec/abi.html#name_mangling
        #~ ☑️ https://github.com/gcc-mirror/gcc/blob/master/libiberty/rust-demangle.c
        my $operators = {
            '&&'                => 'aa',    # operator
            '&'                 => 'ad',    # (unary)
            '&'                 => 'an',
            '&='                => 'aN',
            '='                 => 'aS',
            'alignof_type'      => 'at',    # alignof of a type (C++11)
            'co_await'          => 'aw',    # co_await (C++2a)
            'alignof_expr'      => 'az',    # alignof of an expression (C++11)
            '()'                => 'cl',    # operator
            ','                 => 'cm',    # operator
            '~',                => 'co',    # operator
            '(cast)'            => 'cv',    # operator
            'delete[]'          => 'da',    # operator delete[]
            '*'                 => 'de',    # operator * (unary)
            'delete'            => 'dl',    # operator delete
            '.'                 => 'dt',    # member access (.)
            '/'                 => 'dv',    # operator /
            '/='                => 'dV',    # operator /=
            '^'                 => 'eo',    # operator ^
            '^='                => 'eO',    # operator ^=
            '=='                => 'eq',    # operator ==
            '>='                => 'ge',    # operator >=
            '>'                 => 'gt',    # operator >
            '[]'                => 'ix',    # operator []
            '<='                => 'le',    # operator <=
            '<<'                => 'ls',    # operator <<
            '<<='               => 'lS',    # operator <<=
            '<'                 => 'lt',    # operator <
            '-'                 => 'mi',    # operator -
            '-='                => 'mI',    # operator -=
            '*'                 => 'ml',    # operator *
            '*='                => 'mL',    # operator *=
            '--'                => 'mm',    # operator --
            'new[]'             => 'na',    # operator new[]
            '!='                => 'ne',    # operator !=
            '-(unary)'          => 'ng',    # operator - (unary)
            '!'                 => 'nt',    # operator !
            'new'               => 'nw',    # operator new
            '||'                => 'oo',    # operator ||
            '|'                 => 'or',    # operator |
            '|='                => 'oR',    # operator |=
            '+'                 => 'pl',    # operator +
            '+='                => 'pL',    # operator +=
            '->*'               => 'pm',    # operator ->*
            '++'                => 'pp',    # operator ++
            '+(unary)'          => 'ps',    # operator + (unary)
            '->'                => 'pt',    # operator ->
            '?'                 => 'qu',    # operator ?
            '%'                 => 'rm',    # operator %
            '%='                => 'rM',    # operator %=
            '>>'                => 'rs',    # operator >>
            '>>='               => 'rS',    # operator >>=
            'expansion'         => 'sp',    # Expression pack expansion operator
            '::'                => 'sr',    # Scope resolution operator
            '<=>'               => 'ss',    # operator <=> (C++2a "spaceship")
            'sizeof_type'       => 'st',    # operator sizeof (a type)
            'sizeof_expression' => 'sz',    # operator sizeof (an expression)
            'sizeof_expansion'  => 'sZ',    # operator sizeof (a pack expansion)
            'ext_operator'      => 'v\d'    # vendor extended operator
        };
        my $object = {
            'complete_new'        => 'C1',    # complete object (in-charge) constructor
            'incomplete_new'      => 'C2',    # base object (not-in-charge) constructor
            'complete_new_alloc'  => 'C3',    # complete object (in-charge) allocating constructor
            'bind'                => 'DC',    # structured binding declaration (C++1z)
            'delete'              => 'D0',    # deleting destructor
            'complete_destroy'    => 'D1',    # Complete object (in-charge) destructor
            'incomplete_destroy'  => 'D2',    # Base object (not-in-charge) destructor
            'func'                => 'F',     # function
            'sentry'              => 'GV',    # Sentry object for 1-time init
            'literal'             => 'L',     # literal, external name prefix
            'virtual_covariant'   => 'Tc',    # virtual function covariant override thunk
            'typeinfo'            => 'TD',    #	typeinfo common proxy
            'virtual_non_virtual' => 'Th',    # virtual function non-virtual override thunk
            'typeinfo'            => 'TI',    # typeinfo structure
            'RTTI'                => 'TS',    # RTTI name (NTBS)
            'VTT'                 => 'TT',    # VTT table
            'virtual_override'    => 'Tv',    # virtual function virtual override thunk
            'virtual_table'       => 'TV',    # virtual table
            'local'               => 'Z',     # local name prefix
            'name'                => '\d'     # name (length followed by name)
        };
        my $syntax = {
            'field_init'  => 'di',            # designated field initilizer
            'array_init'  => 'dx',            # designated array initilizer
            'range_init'  => 'dX',            # designated array range initilizer
            'braced_init' => 'il'             # braced-init-list
        };
        my $deliminator = {
            'end_of_list'       => 'E',       # End of argument list
            'template_arg_list' => 'I',       # Template argument list
            'name_list'         => 'N',       # dependent/qualifier name list
            'expression'        => 'X'        # expression prefix
        };
        my $name = {
            'local' => 's'                    # local string prefix
        };
        my $abbreviation = {
            'substitution'          => 'S_',     #substituted name
            'substitution_digit'    => 'S\d',    #substituted name (repeated)
            'substitution_upper'    => 'S\U',    #substituted name (repeated)
            'substitution_lower'    => 'S\u',    #substituted name (standard abbreviation)
            '::std::'               => 'St',     # ::std::
            'template_param'        => 'T_',     #template parameter
            'template_param_repeat' => 'T\d'     #template parameter (repeated)
        };

        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-builtin
        my $types = {
            Void()      => 'v',
            Bool()      => 'b',
            Char()      => 'c',
            SChar()     => 'a',
            UChar()     => 'h',
            Short()     => 's',
            UShort()    => 't',
            Int()       => 'i',
            UInt()      => 'j',
            Long()      => 'l',
            ULong()     => 'm',
            LongLong()  => 'x',
            ULongLong() => 'y',

            #I128 => 'n',
            #U128 => 'o',
            Float()  => 'f',    # Num32
            Double() => 'd',    # Num64

            #LongDouble => 'e',
            #Float128   => 'g'
            #Ellipsis   => 'z',
            #
            #~ Str() => 'Pc',
            #Pointer(Void()) => 'P',
            WChar() => 'w',

            #~ InstanceOf(Void()) => ''
            'const'               => 'K',
            Pointer( [ Void() ] ) => 'P',
            WString()             => 'Pw',
            Array( [ Void() ] )   => 'P',

            #CPPStruct([]) => '???'
        };

        sub Itanium_mangle_name {
            my ( $affix, $data, $name ) = @_;
            my @parts    = split '::', $name;
            my $_mangled = '';
            $_mangled .= $deliminator->{name_list} if @parts > 1;
            $_mangled .= $types->{const}           if $affix->cpp_const;
            #
            if ( scalar @parts >= 2 ) {
                push @{ $data->{subs} }, join '::', @parts[ 0 .. $#parts - 1 ];
            }
            if ( scalar(@parts) >= 2 && ( $parts[-1] eq 'new' || $parts[-2] eq $parts[-1] ) ) {
                $affix->cpp_constructor(1);
                $_mangled .= join '', map { length($_) . $_ } @parts[ 0 .. $#parts - 1 ];
                $_mangled .= $object->{complete_new};
            }
            elsif ( scalar(@parts) >= 2 && ( $parts[-1] eq "DESTROY" || $parts[-2] eq $parts[-1] ) )
            {
                $_mangled .= join '', map { length($_) . $_ } @parts[ 0 .. $#parts - 1 ];
                $_mangled .= $object->{complete_destroy};
            }
            else {
                $_mangled .= join '', map { length($_) . $_ } @parts;
            }
            $_mangled .= $deliminator->{end_of_list} if @parts > 1;

            #~ $object->{complete_new} if
            return $_mangled;
            my $ret = scalar @parts == 1 ? $_mangled :
                $deliminator->{name_list} . $_mangled . $deliminator->{end_of_list};
            $data->{class} = scalar @parts > 1;
            return $ret;
        }

        sub Itanium_check_substitution {
            my ( $affix, $data, $type ) = @_;
            for my $pos ( 0 .. $#{ $data->{subs} } ) {
                if ( $data->{subs}[$pos] eq $type ) {
                    $type = 'S';
                    if   ( $pos == 0 ) { $type .= '_' }
                    else               { $type .= ( $pos - 1 ) . '_' }
                }
            }
            $type;
        }

        sub Itanium_mangle_type {
            my ( $affix, $data, $type ) = @_;
            use Data::Dump;
            ddx $data;
            ddx $type;
            my $ret = '';
            if ( $type->isa('Affix::Type::Pointer') || $type->isa('Affix::Type::Array') ) {
                warn chr int $type;
                warn $types->{$type};
                $ret = $types->{ chr int $type } . Itanium_mangle_type( $affix, $data, $type->[5] );
                if ( grep { $ret eq $_ } @{ $data->{subs} } ) {
                    $ret = Itanium_check_substitution( $affix, $data, $ret );
                }
                else {
                    push @{ $data->{subs} }, $ret unless grep { $ret eq $_ } @{ $data->{subs} };
                }
            }
            elsif ( $type->isa('Affix::Type::InstanceOf') ) {
                $ret = '';    # TODO: Only return 'v' if function is a class method!!!!!!
            }
            elsif ( $type->isa('Affix::Type::Struct') || $type->isa('Affix::Type::Union') ) {

                # TODO: croak if not a typedef'd becuase we don't yet handle anon struct/union
                $ret = length( $type->{class} ) . $type->{class};
                my $_ret = Itanium_check_substitution( $affix, $data, $ret );
                if ( $ret eq $_ret ) {
                    push @{ $data->{subs} }, $ret unless grep { $ret eq $_ } @{ $data->{subs} };
                }
                $ret = $_ret;
            }
            elsif ( $type->isa('Affix::Type::CC') )
            {    # TODO: some call conv. are reflected in mangled symbol
                $ret = '';
            }
            elsif ( $type->isa('Affix::Type::Str') ) {
                $ret = $types->{ Pointer( [ Char() ] ) } . $types->{ Char() };
                if ( grep { $ret eq $_ } @{ $data->{subs} } ) {
                    $ret = Itanium_check_substitution( $affix, $data, $ret );
                }
                else {
                    push @{ $data->{subs} }, $ret unless grep { $ret eq $_ } @{ $data->{subs} };
                }
            }
            elsif ( defined $types->{$type} ) {
                $ret = $types->{$type};
            }
            else {
                my $_ret = Itanium_check_substitution( $affix, $data, $type->{class} );
                if ( defined $_ret ) { $ret = $_ret }
                else {
                    warn 'Unknown type in mangler: ' . chr scalar $type;

                    #~ require Data::Dump;
                    #~ ddx $type;
                    $ret = '';
                }
            }
            return $ret;
        }

        sub Itanium_mangle {

            #~ use Data::Dump;
            #~ ddx \@_;
            my ( $affix, $name, $types, $prefix ) = @_;
            my $data = {};

            #~ use Data::Dump;
            #~ ddx $types;
            $prefix //= '_Z';
            my $ret = join '', $prefix, Itanium_mangle_name( $affix, $data, $name );

            #~ ddx $types;
            if ( $affix->cpp_struct ) {
                my ( $S_, @etc ) = split '::', $name;
                shift @$types
                    if $affix->cpp_constructor &&
                    scalar @$types &&
                    $types->[0]->isa('Affix::Type::CPPStruct');

                #~ $ret .= $abbreviation->{substitution} unless $etc[-1] eq 'new';
            }

            #~ push @$types, Void() if $affix->cpp_constructor;
            $types = [ Void() ] unless scalar @$types;

            #~ ddx $types;
            #~ warn $affix->cpp_struct;
            #~ && !;
            $ret .= join '', map { Itanium_mangle_type( $affix, $data, $_ ) } @$types;

            #~ ddx $data;
            #~ warn $ret;
            return $ret;
        }

        # legacy
        sub Rust_legacy_mangle {
            my ( $affix, $name, $types, $prefix ) = @_;
            my ($symbol_cache) = $affix->lib->list_symbols();
            my @cache          = ();
            my $vp             = 0;
            return $name if grep { $name eq $_ } grep { defined $_ } @$symbol_cache;
            my $itanium = Itanium_mangle( $affix, $name . ( 'x' x 17 ), $types, '' );
            my $ret     = qr'^_ZN?.+?' . sprintf $name =~ '::' ? '%sE' : '%s17h\w{16}E$',
                join( '', ( map { length($_) . $_ } split '::', $name ) );
            my @symbols = grep { $_ =~ /$ret/ } grep { defined $_ } @$symbol_cache;
            return shift @symbols;
        }
    }

    # Dumb but easy in pure perl
    sub Type($) { $Affix::Type::_registry{ $_[0][0] } }
    {    # remove

        package Affix::Aggregate { };

        package Affix::Cache { };

        package Affix::Cache::Symbols { };

        package Affix::Cache::Libs { };

        package Affix::Type { };

        package Affix::Lib { };

        package Affix::Platform { };

        package Affix::Type { };

        package Affix::Type::Base { };

        package Affix::Type::Bool { };

        package Affix::Type::Any { };

        package Affix::Type::Array { };

        package Affix::Type::Base { };

        package Affix::Type::Bool { };

        package Affix::Type::CC { };

        package Affix::Type::Char { };

        package Affix::Type::CharEnum { };

        package Affix::Type::CodeRef { };

        package Affix::Type::Double { };

        package Affix::Type::Enum { };

        package Affix::Type::Float { };

        package Affix::Type::InstanceOf { };

        package Affix::Type::Int { };

        package Affix::Type::IntEnum { };

        package Affix::Type::Long { };

        package Affix::Type::LongLong { };

        package Affix::Type::Pointer { };

        package Affix::Type::Ref { };

        package Affix::Type::SSize_t { };

        package Affix::Type::Short { };

        package Affix::Type::Size_t { };

        package Affix::Type::StdStr { };

        package Affix::Type::Str { };

        package Affix::Type::Struct { };

        package Affix::Type::UChar { };

        package Affix::Type::UInt { };

        package Affix::Type::UIntEnum { };

        package Affix::Type::ULong { };

        package Affix::Type::ULongLong { };

        package Affix::Type::UShort { };

        package Affix::Type::Union { };

        package Affix::Type::Void { };

        package Affix::Type::WChar { };

        package Affix::Type::WStr { };
    }
    {
        my %seen;
        push @{ $EXPORT_TAGS{default} }, grep { !$seen{$_}++ } @{ $EXPORT_TAGS{$_} }
            foreach qw[base types cc];
    }
    {
        my %seen;
        push @{ $EXPORT_TAGS{all} }, grep { !$seen{$_}++ } @{ $EXPORT_TAGS{$_} }
            for keys %EXPORT_TAGS;
    }
    @EXPORT    = sort @{ $EXPORT_TAGS{default} };
    @EXPORT_OK = sort @{ $EXPORT_TAGS{all} };
};
1;
__END__
Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.
