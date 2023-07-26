[![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/linux.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/windows.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/osx.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/freebsd.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![MetaCPAN Release](https://badge.fury.io/pl/Affix.svg)](https://metacpan.org/release/Affix)
# NAME

Affix - A Foreign Function Interface eXtension

# SYNOPSIS

```perl
use Affix;

# bind to exported function
affix( 'libfoo', 'bar', [Str, Float] => Double );
print bar( 'Baz', 3.14 );

# bind to exported function but with sugar
sub bar : Native('libfoo') : Signature([Str, Float] => Double);
print bar( 'Baz', 10.9 );

# wrap an exported function in a code reference
my $bar = wrap( 'libfoo', 'bar', [Str, Float] => Double );
print $bar->( 'Baz', 3.14 );

# bind an exported value to a Perl value
pin( my $ver, 'libfoo', 'VERSION', Int );
```

# DESCRIPTION

Affix is an [FFI](https://en.wikipedia.org/wiki/Foreign_function_interface) to
wrap libraries developed in other languages (C, C++, Rust, etc.) without XS.

## Features

TODO

# Basics

TODO

# The API

The basic API is rather simple but not lacking in power. It's likely what
you'll decide to use in your projects.

## `affix( ... )`

```perl
affix( 'C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );
warn pow( 3, 5 );

affix( 'foo', ['foo', 'foobar'] => [ Str ] );
foobar( 'Hello' );

affix( ['foo_dylib', RUST], ['foo', 'foobar'] => [ Str ] );
foobar( 'Hello' );
```

Attaches a given symbol in a named perl sub.

Parameters include:

- `$lib`

    path or name of the library or an explicit `undef` to load functions from the
    main executable

    Optionally, you may provide an array reference with the library and a version
    number if the library was built with such a value as part of its filename.

- `$symbol_name`

    the name of the symbol to call

    Optionally, you may provide an array reference with the symbol's name and the
    name of the subroutine

- `$parameters`

    signature defining argument types in an array

- `$return`

    optional return type

    default is `Void`

Returns a code reference on success.

## `wrap( ... )`

Creates a wrapper around a given symbol in a given library.

```perl
my $pow = wrap( 'C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );
warn $pow->(5, 10); # 5**10
```

Parameters include:

- `$lib`

    path or name of the library or an explicit `undef` to load functions from the
    main executable

    Optionally, you may provide an array reference with the library and a version
    number if the library was built with such a value as part of its filename.

- `$symbol_name`

    the name of the symbol to call

- `$parameters`

    signature defining argument types in an array

- `$return`

    return type

`wrap( ... )` behaves exactly like `affix( ... )` but returns an anonymous
subroutine and does not pollute the namespace.

## `pin( ... )`

```perl
my $errno;
pin( $errno, 'libc', 'errno', Int );
print $errno;
$errno = 0;
```

Variables exported by a library - also names "global" or "extern" variables -
can be accessed using `pin( ... )`. The above example code applies magic to
`$error` that binds it to the integer variable named "errno" as exported by
the [libc](https://metacpan.org/pod/libc) library.

Expected parameters include:

- `$var`

    Perl scalar that will be bound to the exported variable.

- `$lib`

    name or path of the symbol

- `$symbol_name`

    the name of the exported variable name

- `$type`

    type that data will be coerced in or out of as required

This is likely broken on BSD but patches are welcome.

## `:Native` CODE attribute

All the sugar is right here in the :Native code attribute. This API is inspired
by [Raku's `native` trait](https://docs.raku.org/language/nativecall).

A simple example would look like this:

```perl
use Affix;
sub some_argless_function :Native('something');
some_argless_function();
```

The first line imports various code attributes and types. The next line looks
like a relatively ordinary Perl sub declaration--with a twist. We use the
`:Native` attribute in order to specify that the sub is actually defined in a
native library. The platform-specific extension (e.g., .so or .dll), as well as
any customary prefixes (e.g., lib) will be added for you.

The first time you call "some\_argless\_function", the "libsomething" will be
loaded and the "some\_argless\_function" will be located in it. A call will then
be made. Subsequent calls will be faster, since the symbol handle is retained.

Of course, most functions take arguments or return values--but everything else
that you can do is just adding to this simple pattern of declaring a Perl sub,
naming it after the symbol you want to call and marking it with the
`:Native`-related attributes.

Except in the case you are using your own compiled libraries, or any other kind
of bundled library, shared libraries are versioned, i.e., they will be in a
file `libfoo.so.x.y.z`, and this shared library will be symlinked to
`libfoo.so.x`. By default, Affix will pick up that file if it's the only
existing one. This is why it's safer, and advisable, to always include a
version, this way:

```perl
sub some_argless_function :Native('foo', v1.2.3)
```

Please check [the section on the ABI/API version](#abi-api-version) for
more information.

## Changing names

Sometimes you want the name of your Perl subroutine to be different from the
name used in the library you're loading. Maybe the name is long or has
different casing or is otherwise cumbersome within the context of the module
you are trying to create.

Affix provides the `:Symbol` attribute for you to specify the name of the
native routine in your library that may be different from your Perl subroutine
name.

```perl
package Foo;
use Affix;
sub init :Native('foo') :Symbol('FOO_INIT');
```

Inside of `libfoo` there is a routine called `FOO_INIT` but, since we're
creating a module called `Foo` and we'd rather call the routine as
`Foo::init` (instead of `Foo::FOO_INIT`), we use the symbol trait to specify
the name of the symbol in `libfoo` and call the subroutine whatever we want
(`init` in this case).

## Signatures

Normal Perl signatures do not convey the type of arguments a native function
expects and what it returns so you must define them with our final attribute:
`:Signature`

```perl
use Affix;
sub add :Native("calculator") :Signature([Int, Int] => Int);
```

Here, we have declared that the function takes two 32-bit integers and returns
a 32-bit integer. You can find the other types that you may pass [further down
this page](#types).

## ABI/API version

If you write `:Native('foo')`, Affix will search `libfoo.so` under Unix like
system (`libfoo.dynlib` on macOS, `foo.dll` on Windows). In most modern
system it will require you or the user of your module to install the
development package because it's recommended to always provide an API/ABI
version to a shared library, so `libfoo.so` ends often being a symbolic link
provided only by a development package.

To avoid that, the `:Native` attribute allows you to specify the API/ABI
version. It can be a full version or just a part of it. (Try to stick to Major
version, some BSD code does not care for Minor.)

```perl
use Affix;
sub foo1 :Native('foo', v1); # Will try to load libfoo.so.1
sub foo2 :Native('foo', v1.2.3); # Will try to load libfoo.so.1.2.3

sub pow : Native('m', v6) : Signature([Double, Double] => Double);
```

## Library Paths and Names

The `:Native` attribute, `affix( ... )`, and `wrap( ... )` all accept the
library name, the full path, or a subroutine returning either of the two. When
using the library name, the name is assumed to be prepended with lib and
appended with `.so` (or just appended with `.dll` on Windows), and will be
searched for in the paths in the `LD_LIBRARY_PATH` (`PATH` on Windows)
environment variable.

You can also put an incomplete path like `'./foo'` and Affix will
automatically put the right extension according to the platform specification.
If you wish to suppress this expansion, simply pass the string as the body of a
block.

```perl
sub bar :Native({ './lib/Non Standard Naming Scheme' });
```

**BE CAREFUL**: the `:Native` attribute and constant might be evaluated at
compile time.

## Calling into the standard library

If you want to call a function that's already loaded, either from the standard
library or from your own program, you can omit the library value or pass and
explicit `undef`.

For example on a UNIX-like operating system, you could use the following code
to print the home directory of the current user:

```perl
use Affix;
use Data::Dumper;
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
print Dumper( ptr2sv( $data, Pointer [ PwStruct() ] ) );
```

## Memory Functions

To help toss raw data around, some standard memory related functions are
exposed here. You may import them by name or with the `:memory` or `:all`
tags.

## `malloc( ... )`

```perl
my $ptr = malloc( $size );
```

Allocates `$size` bytes of uninitialized storage.

## `calloc( ... )`

```perl
my $ptr = calloc( $num, $size );
```

Allocates memory for an array of `$num` objects of `$size` and initializes
all bytes in the allocated storage to zero.

## `realloc( ... )`

```
$ptr = realloc( $ptr, $new_size );
```

Reallocates the given area of memory. It must be previously allocated by
`malloc( ... )`, `calloc( ... )`, or `realloc( ... )` and not yet freed with
a call to `free( ... )` or `realloc( ... )`. Otherwise, the results are
undefined.

## `free( ... )`

```
free( $ptr );
```

Deallocates the space previously allocated by `malloc( ... )`, `calloc( ...
)`, or `realloc( ... )`.

## `memchr( ... )`

```
memchr( $ptr, $ch, $count );
```

Finds the first occurrence of `$ch` in the initial `$count` bytes (each
interpreted as unsigned char) of the object pointed to by `$ptr`.

## `memcmp( ... )`

```perl
my $cmp = memcmp( $lhs, $rhs, $count );
```

Compares the first `$count` bytes of the objects pointed to by `$lhs` and
`$rhs`. The comparison is done lexicographically.

## `memset( ... )`

```
memset( $dest, $ch, $count );
```

Copies the value `$ch` into each of the first `$count` characters of the
object pointed to by `$dest`.

## `memcpy( ... )`

```
memcpy( $dest, $src, $count );
```

Copies `$count` characters from the object pointed to by `$src` to the object
pointed to by `$dest`.

## `memmove( ... )`

```
memmove( $dest, $src, $count );
```

Copies `$count` characters from the object pointed to by `$src` to the object
pointed to by `$dest`.

## `sizeof( ... )`

```perl
my $size = sizeof( Int );
my $size1 = sizeof( Struct[ name => Str, age => Int ] );
```

Returns the size, in bytes, of the [type](#types) passed to it.

## `offsetof( ... )`

```perl
my $struct = Struct[ name => Str, age => Int ];
my $offset = offsetof( $struct, 'age' );
```

Returns the offset, in bytes, from the beginning of a structure including
padding, if any.

## Utility Functions

Here's some thin cushions for the rougher edges of wrapping libraries.

They may be imported by name for now but might be renamed, removed, or changed
in the future.

## `cast( ... )`

```perl
my $hash = cast( $ptr, Struct[i => Int, ... ] );
```

This function will parse a pointer into a given target type.

The source pointer would have normally been obtained from a call to a native
subroutine that returned a pointer, a lvalue pointer to a native subroutine,
or, as part of a `Struct[ ... ]`.

## `DumpHex( ... )`

```
DumpHex( $ptr, $length );
```

Dumps `$length` bytes of raw data from a given point in memory.

This is a debugging function that probably shouldn't find its way into your
code and might not be public in the future.

# Types

Affix supports the fundamental types (void, int, etc.) and aggregates (struct,
array, union).

## Fundamental Types

<div>
      <table>
        <thead>
          <tr>
            <th>Affix</th> <th>C99</th> <th>Rust</th> <th>C#</th> <th>pack()</th> <th>Raku</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>Void</td> <td>void</td> <td>-&gt;()</td> <td>void/NULL</td> <td>-</td> <td></td>
          </tr>

          <tr>
            <td>Bool</td> <td>_Bool</td> <td>bool</td> <td>bool</td> <td>-</td> <td>bool</td>
          </tr>

          <tr>
            <td>Char</td>

            <td>int8_t</td>

            <td>i8</td>

            <td>sbyte</td>

            <td>c</td>

            <td>int8</td>
          </tr>

          <tr>
            <td>UChar</td>

            <td>uint8_t</td>

            <td>u8</td>

            <td>byte</td>

            <td>C</td>

            <td>byte, uint8</td>
          </tr>

          <tr>
            <td>Short</td>

            <td>int16_t</td>

            <td>i16</td>

            <td>short</td>

            <td>s</td>

            <td>int16</td>
          </tr>

          <tr>
            <td>UShort</td>

            <td>uint16_t</td>

            <td>u16</td>

            <td>ushort</td>

            <td>S</td>

            <td>uint16</td>
          </tr>

          <tr>
            <td>Int</td>

            <td>int32_t</td>

            <td>i32</td>

            <td>int</td>

            <td>i</td>

            <td>int32</td>
          </tr>

          <tr>
            <td>UInt</td>

            <td>uint32_t</td>

            <td>u32</td>

            <td>uint</td>

            <td>I</td>

            <td>uint32</td>
          </tr>

          <tr>
            <td>Long</td>

            <td>int64_t</td>

            <td>i64</td>

            <td>long</td>

            <td>l</td>

            <td>int64, long</td>
          </tr>

          <tr>
            <td>ULong</td>

            <td>uint64_t</td>

            <td>u64</td>

            <td>ulong</td>

            <td>L</td>

            <td>uint64, ulong</td>
          </tr>

          <tr>
            <td>LongLong</td>

            <td>-/long long</td>

            <td>i128</td>

            <td>q</td>

            <td>longlong</td>

            <td></td>
          </tr>

          <tr>
            <td>ULongLong</td>

            <td>-/unsigned long long</td>

            <td>u128</td>

            <td>Q</td>

            <td>ulonglong</td>

            <td></td>
          </tr>

          <tr>
            <td>Float</td>

            <td>float</td>

            <td>f32</td>

            <td>f</td>

            <td>num32</td>

            <td></td>
          </tr>

          <tr>
            <td>Double</td>

            <td>double</td>

            <td>f64</td>

            <td>d</td>

            <td>num64</td>

            <td></td>
          </tr>

          <tr>
            <td>SSize_t</td>

            <td>SSize_t</td>

            <td>SSize_t</td>

            <td></td>

            <td></td>

            <td></td>
          </tr>

          <tr>
            <td>Size_t</td>

            <td>size_t</td>

            <td>size_t</td>

            <td></td>

            <td></td>

            <td></td>
          </tr>

          <tr>
            <td>Str</td>

            <td>char *</td>

            <td></td>

            <td></td>

            <td></td>

            <td></td>
          </tr>

          <tr>
            <td>WStr</td>

            <td>wchar_t</td>

            <td></td>

            <td></td>

            <td></td>
          </tr>
        </tbody>
      </table>
</div>

Given sizes are minimums measured in bits

### `Void`

The `Void` type corresponds to the C `void` type. It is generally found in
typed pointers representing the equivalent to the `void *` pointer in C.

```perl
affix undef, 'malloc', [Size_t] => Pointer[Void];
my $data = malloc( 32 );
```

As the example above shows, it's represented by a parameterized `Pointer[ ...
]` type, using as parameter whatever the original pointer is pointing to (in
this case, `void`). This role represents native pointers, and can be used
wherever they need to be represented in a Perl script.

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

Signed integer type.

### `Size_t`

Unsigned integer type often expected as the result of `sizeof` or `offsetof`
but can be found elsewhere.

## `Str`

Automatically handle null terminated character pointers with this rather than
trying using `Pointer[Char]` and doing it yourself.

You'll learn a bit more about `Pointer[...]` and other parameterized types in
the next section.

## `WStr`

A null-terminated wide string is a sequence of valid wide characters, ending
with a null character.

## Parameterized Types

Some types must be provided with more context data.

## `Pointer[ ... ]`

```
Pointer[Int]  ~~ int *
Pointer[Void] ~~ void *
```

Create pointers to (almost) all other defined types including `Struct` and
`Void`.

To handle a pointer to an object, see [InstanceOf](https://metacpan.org/pod/InstanceOf).

Void pointers (`Pointer[Void]`) might be created with `malloc` and other
memory related functions.

## `Struct[ ... ]`

```perl
Struct[                    struct {
    dob => Struct[              struct {
        year  => Int,               int year;
        month => Int,   ~~          int month;
        day   => Int                int day;
    ],                          } dob;
    name => Str              char *name;
    wId  => Long                long wId;
];                          };
```

A struct consists of a sequence of members with storage allocated in an ordered
sequence (as opposed to `Union`, which is a type consisting of a sequence of
members where storage overlaps).

A C struct that looks like this:

```
struct {
    char *make;
    char *model;
    int   year;
};
```

...would be defined this way:

```perl
Struct[
    make  => Str,
    model => Str,
    year  => Int
];
```

All fundamental and aggregate types may be found inside of a `Struct`.

## `ArrayRef[ ... ]`

The elements of the array must pass the additional size constraint.

An array length must be given:

```
ArrayRef[Int, 5];   # int arr[5]
ArrayRef[Char, 5];  # char arr[5]
ArrayRef[Str, 10];  # char *arr[10]
```

## `Union[ ... ]`

A union is a type consisting of a sequence of members with overlapping storage
(as opposed to `Struct`, which is a type consisting of a sequence of members
whose storage is allocated in an ordered sequence).

The value of at most one of the members can be stored in a union at any one
time and the union is only as big as necessary to hold its largest member
(additional unnamed trailing padding may also be added). The other members are
allocated in the same bytes as part of that largest member.

A C union that looks like this:

```
union {
    char  c[5];
    float f;
};
```

...would be defined this way:

```perl
Union[
    c => ArrayRef[Char, 5],
    f => Float
];
```

## `CodeRef[ ... ]`

A value where `ref($value)` equals `CODE`. This would be how callbacks are
defined.

The argument list and return value must be defined. For example,
`CodeRef[[Int, Int]=`Int\]> ~~ `typedef int (*fuc)(int a, int b);`; that is to
say our function accepts two integers and returns an integer.

```perl
CodeRef[[] => Void];                   # typedef void (*function)();
CodeRef[[Pointer[Int]] => Int];        # typedef Int (*function)(int * a);
CodeRef[[Str, Int] => Struct[...]]; # typedef struct Person (*function)(chat * name, int age);
```

## `InstanceOf[ ... ]`

```
InstanceOf['Some::Class']
```

A blessed object of a certain type. When used as an lvalue, the result is
properly blessed. As an rvalue, the reference is checked to be a subclass of
the given package.

Note: This "type" is in a state of development flux and might be made complete
with [issue #32](https://github.com/sanko/Affix.pm/issues/32)

## `Any`

Anything you dump here will be passed along unmodified. We hand off a pointer
to the `SV*` perl gives us without copying it.

## `Enum[ ... ]`

The value of an `Enum` is defined by its underlying type which includes
`Int`, `Char`, etc.

This type is declared with an list of strings.

```
Enum[ 'ALPHA', 'BETA' ];
# ALPHA = 0
# BETA  = 1
```

Unless an enumeration constant is defined in an array reference, its value is
the value one greater than the value of the previous enumerator in the same
enumeration. The value of the first enumerator (if it is not defined) is zero.

```perl
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
```

As you can see, enum values may allude to earlier defined values and even basic
arithmetic is supported.

Additionally, if you `typedef` the enum into a given namespace, you may refer
to elements by name. They are defined as dualvars so that works:

```perl
typedef color => Enum[ 'RED', 'GREEN', 'BLUE' ];
print color::RED();     # RED
print int color::RED(); # 0
```

## `IntEnum[ ... ]`

Same as `Enum`.

## `UIntEnum[ ... ]`

`Enum` but with unsigned integers.

## `CharEnum[ ... ]`

`Enum` but with signed chars.

# Signatures

Affix's advisory signatures are required to give us a little hint about what we
should expect.

```perl
[ Int, ArrayRef[ Int, 100 ], Str ] => Int
```

Arguments are defined in a list: `[ Int, ArrayRef[ Char, 5 ], Str ]`

The return value comes next: `Int`

To call the function with such a signature, your Perl would look like this:

```
mh $int = func( 500, [ 'a', 'b', 'x', '4', 'H' ], 'Test');
```

See the aptly named sections entitled [Types](#types) for more on the possible
types and ["Calling Conventions" in Calling Conventions](https://metacpan.org/pod/Calling%20Conventions#Calling-Conventions) for flags that may also be
defined as part of your signature.

# Calling Conventions

Handle with care! Using these without understanding them can break your code!

Refer to [the dyncall manual](https://dyncall.org/docs/manual/manualse11.html),
[http://www.angelcode.com/dev/callconv/callconv.html](http://www.angelcode.com/dev/callconv/callconv.html),
[https://en.wikipedia.org/wiki/Calling\_convention](https://en.wikipedia.org/wiki/Calling_convention), and your local
university's Comp Sci department for a deeper explanation.

Anyway, here are the current options:

- `CC_DEFAULT`
- `CC_THISCALL`
- `CC_ELLIPSIS`
- `CC_ELLIPSIS_VARARGS`
- `CC_CDECL`
- `CC_STDCALL`
- `CC_FASTCALL_MS`
- `CC_FASTCALL_GNU`
- `CC_THISCALL_MS`
- `CC_THISCALL_GNU`
- `CC_ARM_ARM`
- `CC_ARM_THUMB`
- `CC_SYSCALL`

When used in ["Signatures" in signatures](https://metacpan.org/pod/signatures#Signatures), most of these cause the internal
argument stack to be reset. The exception is `CC_ELLIPSIS_VARARGS` which is
used prior to binding varargs of variadic functions.

# Platform Support

Not all features of dyncall are supported on all platforms, for those, the
underlying library defines macros you can use to detect support. These values
are exposed under the `Affix::Platform` package:

- `Affix::Platform::Syscall()`

    If true, your platform supports a syscall calling conventions.

- `Affix::Platform::AggrByVal()`

    If true, your platform supports passing around aggregates (struct, union) by
    value.

# Stack Size

You may control the max size of the internal stack that will be allocated and
used to bind the arguments to by setting the `$VMSize` variable before using
Affix.

```
BEGIN{ $Affix::VMSize = 2 ** 16; }
```

This value is `4096` by default.

# Examples

The best example of use might be [LibUI](https://metacpan.org/pod/LibUI). Brief examples will be found in
`eg/`. Very short examples might find their way here.

# Why not XS?

Deciding why your next project should be backed by Affix or even FFI::Platypus
is a personal one. However, I can explain my reasons for looking to FFI for my
projects and why I wrote Affix.

- FFI extensions do not need compilation.
- Your users do not need a functioning compiler on their system to be able to run FFI extensions.

    And unlike many XS based modules, the developer versions of libs which include
    headers are not necessary. The common runtime variations of libraries will do.

- FFI extensions are not platform specific.

    An FFI extension can be written to work on any version of Perl supported by
    Affix, for example. Developers who want to target older versions of perl can
    focus their efforts on the pure Perl side.

- FFI extensions are easy to write.

    Affix's API makes powerful things very simple. Take the following example code:

    ```perl
    affix 'libc' => 'puts', [ Str ] => Int;
    ```

- FFI extensions are easy to maintain.

    Simple, declarative code is easier to read and write, making it easier to
    maintain. Changes to core perl's internals are also no longer your headache.

# See Also

All the heavy lifting is done by [dyncall](https://dyncall.org/).

Check out [FFI::Platypus](https://metacpan.org/pod/FFI%3A%3APlatypus) for a more robust and mature FFI

[LibUI](https://metacpan.org/pod/LibUI) for a larger demo project based on Affix

[Types::Standard](https://metacpan.org/pod/Types%3A%3AStandard) for the inspiration of the advisory types system

# LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.

# AUTHOR

Sanko Robinson <sanko@cpan.org>
