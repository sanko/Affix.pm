[![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/linux.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/windows.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/osx.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/freebsd.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![MetaCPAN Release](https://badge.fury.io/pl/Affix.svg)](https://metacpan.org/release/Affix)
# NAME

Affix - A Foreign Function Interface eXtension

# SYNOPSIS

    use Affix;
    my $lib
        = $^O eq 'MSWin32'    ? 'ntdll' :
        $^O eq 'darwin'       ? '/usr/lib/libm.dylib' :
        $^O eq 'bsd'          ? '/usr/lib/libm.so' :
        -e '/lib64/libm.so.6' ? '/lib64/libm.so.6' :
        '/lib/x86_64-linux-gnu/libm.so.6';
    attach( $lib, 'pow', [ Double, Double ] => Double );
    print pow( 2, 10 );    # 1024

# DESCRIPTION

Affix is a wrapper around [dyncall](https://dyncall.org/). If you're looking to
design your own low level wrapper, see [Dyn.pm](https://metacpan.org/pod/Dyn).

# `:Native` CODE attribute

While most of the upstream API is covered in the [Dyn::Call](https://metacpan.org/pod/Dyn%3A%3ACall),
[Dyn::Callback](https://metacpan.org/pod/Dyn%3A%3ACallback), and [Dyn::Load](https://metacpan.org/pod/Dyn%3A%3ALoad) packages, all the sugar is right here in
`Affix`. The simplest but least flexible use of `Affix` would look something
like this:

    use Affix;
    sub some_iiZ_func : Native('somelib.so') : Signature([Int, Long, Str] => Void);
    some_iiZ_func( 100, time, 'Hello!' );

Let's step through what's here...

The second line above looks like a normal Perl sub declaration but includes our
CODE attributes:

- `:Native`

    Here, we're specifying that the sub is actually defined in a native library.
    This is inspired by Raku's `native` trait.

- `:Signature`

    Perl's [signatures](https://metacpan.org/pod/perlsub#Signatures) and [prototypes](https://metacpan.org/pod/perlsub#Prototypes)
    obviously don't contain type info so we use this attribute to define advisory
    argument and return types.

Finally, we just call our affixed function. Positional parameters are passed
through and any result is returned according to the given type. Here, we return
nothing because our signature claims the function returns `Void`.

To avoid banging your head on a built-in function, you may name your sub
anything else and let Affix know what symbol to attach:

    sub my_abs : Native('my_lib.dll') : Signature([Double] => Double) : Symbol('abs');
    CORE::say my_abs( -75 ); # Should print 75 if your abs is something that makes sense

This is by far the fastest way to work with this distribution but it's not by
any means the only way.

All of the following methods may be imported by name or with the `:sugar` tag.

Note that everything here is subject to change before v1.0.

# `attach( ... )`

Wraps a given symbol in a named perl sub.

    Dyn::attach('C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );

# `wrap( ... )`

Creates a wrapper around a given symbol in a given library.

    my $pow = Dyn::wrap( 'C:\Windows\System32\user32.dll', 'pow', [Double, Double]=>Double );
    warn $pow->(5, 10); # 5**10

Expected parameters include:

- `lib` - pointer returned by [`dlLoadLibrary( ... )`](https://metacpan.org/pod/Dyn%3A%3ALoad#dlLoadLibrary) or the path of the library as a string
- `symbol_name` - the name of the symbol to call
- `signature` - signature defining argument types, return type, and optionally the calling convention used

Returns a code reference.

# Signatures

`dyncall` uses an almost `pack`-like syntax to define signatures which is
simple and powerful but Affix is inspired by [Type::Standard](https://metacpan.org/pod/Type%3A%3AStandard). See
[Affix::Types](https://metacpan.org/pod/Affix%3A%3ATypes) for more.

# Library paths and names

The `Native` attribute, `attach( ... )`, and `wrap( ... )` all accept the
library name, the full path, or a subroutine returning either of the two. When
using the library name, the name is assumed to be prepended with lib and
appended with `.so` (or just appended with `.dll` on Windows), and will be
searched for in the paths in the `LD_LIBRARY_PATH` (`PATH` on Windows)
environment variable.

    use Affix;
    use constant LIBMYSQL => 'mysqlclient';
    use constant LIBFOO   => '/usr/lib/libfoo.so.1';
    sub LIBBAR {
        my $opt = $^O =~ /bsd/ ? 'r' : 'p';
        my ($path) = qx[ldconfig -$opt | grep libbar];
        return $1;
    }
    # and later
    sub mysql_affected_rows :Native(LIBMYSQL);
    sub bar :Native(LIBFOO);
    sub baz :Native(LIBBAR);

You can also put an incomplete path like `'./foo'` and Affix will
automatically put the right extension according to the platform specification.
If you wish to suppress this expansion, simply pass the string as the body of a
block.

\###### TODO: disable expansion with a block!

    sub bar :Native({ './lib/Non Standard Naming Scheme' });

**BE CAREFUL**: the `:Native` attribute and constant are evaluated at compile
time. Don't write a constant that depends on a dynamic variable like:

    # WRONG:
    use constant LIBMYSQL => $ENV{LIB_MYSQLCLIENT} // 'mysqlclient';

## ABI/API version

If you write `:Native('foo')`, Affix will search `libfoo.so` under Unix like
system (`libfoo.dynlib` on macOS, `foo.dll` on Windows). In most modern
system it will require you or the user of your module to install the
development package because it's recommended to always provide an API/ABI
version to a shared library, so `libfoo.so` ends often being a symbolic link
provided only by a development package.

To avoid that, the native trait allows you to specify the API/ABI version. It
can be a full version or just a part of it. (Try to stick to Major version,
some BSD code does not care for Minor.)

    use Affix;
    sub foo1 :Native('foo', v1); # Will try to load libfoo.so.1
    sub foo2 :Native('foo', v1.2.3); # Will try to load libfoo.so.1.2.3

    my $lib = ['foo', 'v1'];
    sub foo3 :Native($lib);

## Calling into the standard library

If you want to call a function that's already loaded, either from the standard
library or from your own program, you can omit the library value or pass and
explicit `undef`.

For example on a UNIX-like operating system, you could use the following code
to print the home directory of the current user:

    use Affix;
    typedef PwStruct => Struct [
        name  => Str,     # username
        pass  => Str,     # hashed pass if shadow db isn't in use
        uuid  => UInt,    # user
        guid  => UInt,    # group
        gecos => Str,     # real name
        dir   => Str,     # ~/
        shell => Str      # bash, etc.
    ];
    sub getuid : Native : Signature([]=>Int);
    sub getpwuid : Native : Signature([Int]=>Pointer[PwStruct]);
    my $data = main::getpwuid( getuid() );
    use Data::Dumper;
    print Dumper( Affix::ptr2sv( PwStruct(), $data ) );

# Memory Functions

To help toss raw data around, some standard memory related functions are
exposed here. You may import them by name or with the `:memory` or `:all`
tags.

## `malloc( ... )`

    my $ptr = malloc( $size );

Allocates [$size](https://metacpan.org/pod/%24size) bytes of uninitialized storage.

## `calloc( ... )`

    my $ptr = calloc( $num, $size );

Allocates memory for an array of `$num` objects of `$size` and initializes
all bytes in the allocated storage to zero.

## `realloc( ... )`

    $ptr = realloc( $ptr, $new_size );

Reallocates the given area of memory. It must be previously allocated by
`malloc( ... )`, `calloc( ... )`, or `realloc( ... )` and not yet freed with
a call to `free( ... )` or `realloc( ... )`. Otherwise, the results are
undefined.

## `free( ... )`

    free( $ptr );

Deallocates the space previously allocated by `malloc( ... )`, `calloc( ...
)`, or `realloc( ... )`.

## `memchr( ... )`

    memchr( $ptr, $ch, $count );

Finds the first occurrence of `$ch` in the initial `$count` bytes (each
interpreted as unsigned char) of the object pointed to by `$ptr`.

## `memcmp( ... )`

    my $cmp = memcmp( $lhs, $rhs, $count );

Compares the first `$count` bytes of the objects pointed to by `$lhs` and
`$rhs`. The comparison is done lexicographically.

## `memset( ... )`

    memset( $dest, $ch, $count );

Copies the value `$ch` into each of the first `$count` characters of the
object pointed to by `$dest`.

## `memcpy( ... )`

    memcpy( $dest, $src, $count );

Copies `$count` characters from the object pointed to by `$src` to the object
pointed to by `$dest`.

## `memmove( ... )`

    memmove( $dest, $src, $count );

Copies `$count` characters from the object pointed to by `$src` to the object
pointed to by `$dest`.

## `sizeof( ... )`

    my $size = sizeof( Int );
    my $size1 = sizeof( Struct[ name => Str, age => Int ] );

Returns the size, in bytes, of the [type](#types) passed to it.

# Types

While Raku offers a set of native types with a fixed, and known, representation
in memory but this is Perl so we need to do the work ourselves and design and
build a pseudo-type system. Affix supports the fundamental types (void, int,
etc.) and aggregates (struct, array, union).

## Fundamental Types with Native Representation

    Affix       C99/C++     Rust    C#          pack()  Raku
    -----------------------------------------------------------------------
    Void        void/NULL   ->()    void/NULL   -
    Bool        _Bool       bool    bool        -       bool
    Char        int8_t      i8      sbyte       c       int8
    UChar       uint8_t     u8      byte        C       byte, uint8
    Short       int16_t     i16     short       s       int16
    UShort      uint16_t    u16     ushort      S       uint16
    Int         int32_t     i32     int         i       int32
    UInt        uint32_t    u32     uint        I       uint32
    Long        int64_t     i64     long        l       int64, long
    ULong       uint64_t    u64     ulong       L       uint64, ulong
    LongLong    -           i128                q       longlong
    ULongLong   -           u128                Q       ulonglong
    Float       float       f32                 f       num32
    Double      double      f64                 d       num64
    SSize_t     SSize_t                                 SSize_t
    Size_t      size_t                                  size_t
    Str         char *

Given sizes are minimums measured in bits

### `Void`

The `Void` type corresponds to the C `void` type. It is generally found in
typed pointers representing the equivalent to the `void *` pointer in C.

    sub malloc :Native :Signature([Size_t] => Pointer[Void]);
    my $data = malloc( 32 );

As the example shows, it's represented by a parameterized `Pointer[ ... ]`
type, using as parameter whatever the original pointer is pointing to (in this
case, `void`). This role represents native pointers, and can be used wherever
they need to be represented in a Perl script.

In addition, you may place a `Void` in your signature to skip a passed
argument.

### `Bool`

Boolean type may only have room for one of two values: `true` or `false`.

### `Char`

Signed character. It's guaranteed to have a width of at least 8 bits.

Pointers (`Pointer[Char]`) might be better expressed with a `Str`.

### `UChar`

Unsigned character. It's guaranteed to have a width of at least 8 bits.

### `Short`

Signed short integer. It's guaranteed to have a width of at least 16 bits.

### `UShort`

Unsigned short integer. It's guaranteed to have a width of at least 16 bits.

### `Int`

Basic signed integer type.

It's guaranteed to have a width of at least 16 bits. However, on 32/64 bit
systems it is almost exclusively guaranteed to have width of at least 32 bits.

### `UInt`

Basic unsigned integer type.

It's guaranteed to have a width of at least 16 bits. However, on 32/64 bit
systems it is almost exclusively guaranteed to have width of at least 32 bits.

### `Long`

Signed long integer type. It's guaranteed to have a width of at least 32 bits.

### `ULong`

Unsigned long integer type. It's guaranteed to have a width of at least 32
bits.

### `LongLong`

Signed long long integer type. It's guaranteed to have a width of at least 64
bits.

### `ULongLong`

Unsigned long long integer type. It's guaranteed to have a width of at least 64
bits.

### `Float`

[Single precision floating-point
type](https://en.wikipedia.org/wiki/Single-precision_floating-point_format).

### `Double`

[Double precision floating-point
type](https://en.wikipedia.org/wiki/Double-precision_floating-point_format).

### `SSize_t`

### `Size_t`

## `Str`

Automatically handle null terminated character pointers with this rather than
trying to defined a parameterized `Pointer[...]` type like as `Pointer[Char]`
and doing it yourself.

You'll learn a bit more about parameterized types in the next section.

# Parameterized Types

Some types must be provided with more context data.

## `Pointer[ ... ]`

Create pointers to (almost) all other defined types including `Struct` and
`Void`.

To handle a pointer to an object, see [InstanceOf](https://metacpan.org/pod/InstanceOf).

Void pointers (`Pointer[Void]`) might be created with `malloc` and other
memory related functions.

## `Aggregate`

This is currently undefined and reserved for possible future use.

## `Struct[ ... ]`

A struct is a type consisting of a sequence of members whose storage is
allocated in an ordered sequence (as opposed to `Union`, which is a type
consisting of a sequence of members whose storage overlaps).

A C struct that looks like this:

    struct {
        char *make;
        char *model;
        int   year;
    };

...would be defined this way:

    Struct[
        make  => Str,
        model => Str,
        year  => Int
    ];

## `ArrayRef[ ... ]`

The elements of the array must pass the additional constraint. For example
`ArrayRef[Int]` should be a reference to an array of numbers.

An array length must be given:

    ArrayRef[Int, 5];   # int arr[5]
    ArrayRef[Any, 20];  # SV * arr[20]
    ArrayRef[Char, 5];  # char arr[5]
    ArrayRef[Str, 10];  # char *arr[10]

## `Union[ ... ]`

A union is a type consisting of a sequence of members whose storage overlaps
(as opposed to `Struct`, which is a type consisting of a sequence of members
whose storage is allocated in an ordered sequence).

The value of at most one of the members can be stored in a union at any one
time and the union is only as big as necessary to hold its largest member
(additional unnamed trailing padding may also be added). The other members are
allocated in the same bytes as part of that largest member.

A C union that looks like this:

    union {
        char  c[5];
        float f;
    };

...would be defined this way:

    Union[
        c => ArrayRef[Char, 5],
        f => Float
    ];

## `CodeRef[ ... ]`

A value where `ref($value)` equals `CODE`.

The argument list and return value must pass the additional constraint. For
example, `CodeRef[[Int, Int]=`Int\]> `typedef int (*fuc)(int a, int b);`; that
is function that accepts two integers and returns an integer.

    CodeRef[[] => Void]; # typedef void (*function)();
    CodeRef[[Pointer[Int]] => Int]; # typedef Int (*function)(int * a);
    CodeRef[[Str, Int] => Struct[...]]; # typedef struct Person (*function)(chat * name, int age);

## `InstanceOf[ ... ]`

## `Any`

Anything you dump here will be passed along unmodified. We hand off whatever
`SV*` perl gives us without copying it.

## `Enum[ ... ]`

The value of an `Enum` is defined by its underlying type which includes
`Int`, `Char`, etc.

This type is declared with an list of strings.

    Enum[ 'ALPHA', 'BETA' ];
    # ALPHA = 0
    # BETA  = 1

Unless an enumeration constant is defined in an array reference, its value is
the value one greater than the value of the previous enumerator in the same
enumeration. The value of the first enumerator (if it is not defined) is zero.

    Enum[ 'A', 'B', [C => 10], 'D', [E => 1], 'F', [G => 'F + C'] ];
    # A = 0
    # B = 1
    # C = 10
    # D = 11
    # E = 1
    # F = 2
    # G = 12

    Enum[ [ one => 'a' ], 'two', [ 'three' => 'one' ] ]
    # one   = a
    # two   = b
    # three = a

Additionally, if you `typedef` the enum into a given namespace, you may refer
to elements by name:

    typedef color => Enum[ 'RED', 'GREEN', 'BLUE' ];
    print color::RED();     # RED
    print int color::RED(); # 0

## `IntEnum[ ... ]`

Same as `Enum`.

## `UIntEnum[ ... ]`

`Enum` but with unsigned integers.

## `CharEnum[ ... ]`

`Enum` but with signed chars.

# See Also

Check out [FFI::Platypus](https://metacpan.org/pod/FFI%3A%3APlatypus) for a more robust and mature FFI.

Examples found in `eg/`.

# LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.

# AUTHOR

Sanko Robinson <sanko@cpan.org>
