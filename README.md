# NAME

Affix - A Foreign Function Interface eXtension

# SYNOPSIS

```perl
use Affix qw[:all];

# bind to exported function
affix libm, 'floor', [Double], Double;
warn floor(3.14159);    # 3

# wrap an exported function in a code reference
my $getpid = wrap libc, 'getpid', [], Int;    # '_getpid' on Win32
warn $getpid->();                             # $$

# bind an exported value to a Perl value
pin( my $ver, 'libfoo', 'VERSION', Int );
```

# DESCRIPTION

Affix is an [FFI](https://en.wikipedia.org/wiki/Foreign_function_interface) to wrap libraries developed in other
languages (C, C++, Rust, etc.) without having to write or maintain XS.

## Features

Affix includes the following features right out of the box:

- Works on Windows, macOS, Linux, BSD, and more.
- Callbacks
- Pointers
- Typedefs
- Global/Exported variables
- Libraries developed in C, C++, and Rust (and more to come!) even those with mangled symbol names
- Aggregates such as structs, unions, and arrays
- Passing aggregates by value on many platforms
- Nested aggregates
- 'Smart' [enums](https://metacpan.org/pod/Affix%3A%3AEnum)
- Tested to work all the way down to Perl 5.026 (which is ancient in my book)

# The Basics

Affix's API is rather simple but not lacking in power. Let's start at the very beginning, with the eponymous `affix(
... )` function.

## `affix( ... )`

Attaches a given symbol to a named perl sub.

```perl
affix libm, 'pow', [Double, Double] => Double;
warn pow( 3, 5 );

affix libc, 'puts', [Str], Int;
puts( 'Hello' );

affix './mylib.dll', ['output', 'write'], [Str], Int; # renamed function
write( 'Hello' );

affix undef, [ 'rint', 'round' ], [Double], Double; # use current process
warn round(3.14);

affix find_library('xxhash'), [ 'XXH_versionNumber', 'xxHash::version' ], [], UInt;
warn xxHash::version();
```

Expected parameters include:

- `lib` - required

    File path or name of the library to load symbols from. Pass an explicit `undef` to pull functions from the main
    executable.

- `symbol_name` - required

    Name of the symbol to wrap.

    Optionally, you may provide an array reference with the symbol's name and the name of the wrapping subroutine.

- `parameters` - required

    Provide the argument types in an array reference.

- `return` - required

    A single return type for the function.

On success, `affix( ... )` returns the generated code reference which may be called directly but you'll likely use the
name you provided.

## `wrap( ... )`

Creates a wrapper around a given symbol in a given library.

```perl
my $pow = wrap libm, 'pow', [Double, Double] => Double;
warn $pow->(5, 10); # 5**10
```

Parameters include:

- `lib` - required

    File path or name of the library to load symbols from. Pass an explicit `undef` to pull functions from the main
    executable.

- `symbol_name` - required

    Name of the symbol to wrap.

- `parameters` - required

    Provide the argument types in an array reference.

- `return` - required

    A single return type for the function.

`wrap( ... )` behaves exactly like `affix( ... )` but returns an anonymous subroutine and does not pollute the
namespace with a named function.

## `pin( ... )`

```perl
my $errno;
pin $errno, libc, 'errno', Int;
print $errno;
$errno = 0;
```

Variables exported by a library - also referred to as "global" or "extern" variables - can be accessed using `pin( ...
)`. The above example code applies magic to `$error` that binds it to the integer variable named "errno" as exported
by the [libc](https://metacpan.org/pod/libc) library.

Expected parameters include:

- `var` - required

    Perl scalar that will be bound to the exported variable.

- `lib` - required

    File path or name of the library to load symbols from. Pass an explicit `undef` to pull functions from the main
    executable.

- `symbol_name` - required

    Name of the exported variable to wrap.

- `$type` - required

    Indicate to Affix what type of data the variable contains.

This is likely broken on BSD but patches are welcome.

# Library Functions

Locating libraries on different platforms can be a little tricky. These are utilities to help you out.

They are exported by default but may be imported by name or with the `:lib` tag.

## `find_library( ... )`

```perl
my $libm = find_library( 'm' ); # /usr/lib/libm.so.6, etc.
my $libc = find_library( 'c' ); # /usr/lib/libc.so.6, etc.
my $bz2 = find_library( 'bz2' ); # /usr/lib/libbz2.so.1.0.8
my $ntdll = find_library( 'ntdll' ); # C:\Windows\system32\ntdll.dll
```

Locates a library close to the way the compiler or platform-dependant runtime loader does. Where multiple versions of
the same shared library exists, the most recent should be returned.

## `load_library( ... )`

## `free_library( ... )`

## `list_symbols( ... )`

## `find_symbol( ... )`

## `free_symbol( ... )`

## `dlerror( )`

```perl
my $err = dlerror( );
say $err if $err;
```

Returns a human readable string describing the most recent error that occurred from `load_library( ... )`,
`free_library( ... )`, etc. since the last call to `dlerror( )`.

An undefined value is returned if no errors have occurred.

## `libc()`

Returns the path to the platform-dependant equivalent of the standard C library.

This may be something like `/usr/lib/libc.so.6` (Linux), `/lib/libc.so.7` (FreeBSD), `/usr/lib/libc.dylib` (macOS),
`C:\Windows\system32\msvcrt.dll` (Windows), etc.

## `libm()`

Returns the path to the platform-dependant equivalent of the standard C math library.

This may be something like `/usr/lib/libm.so.6` (Linux), `/lib/libm.so.5` (FreeBSD), `/usr/lib/libm.dylib` (macOS),
`C:\Windows\system32\msvcrt.dll` (Windows), etc.

# Memory Functions

To help toss raw data around, some standard memory related functions are exposed here. You may import them by name or
with the `:memory` or `:all` tags.

## `malloc( ... )`

```perl
my $ptr = malloc( $size );
```

Allocates `$size` bytes of uninitialized storage.

## `calloc( ... )`

```perl
my $ptr = calloc( $num, $size );
```

Allocates memory for an array of `$num` objects of `$size` and initializes all bytes in the allocated storage to
zero.

## `realloc( ... )`

```
$ptr = realloc( $ptr, $new_size );
```

Reallocates the given area of memory. It must be previously allocated by `malloc( ... )`, `calloc( ... )`, or
`realloc( ... )` and not yet freed with a call to `free( ... )` or `realloc( ... )`. Otherwise, the results are
undefined.

## `free( ... )`

```
free( $ptr );
```

Deallocates the space previously allocated by `malloc( ... )`, `calloc( ... )`, or `realloc( ... )`.

## `memchr( ... )`

```
memchr( $ptr, $ch, $count );
```

Finds the first occurrence of `$ch` in the initial `$count` bytes (each interpreted as unsigned char) of the object
pointed to by `$ptr`.

## `memcmp( ... )`

```perl
my $cmp = memcmp( $lhs, $rhs, $count );
```

Compares the first `$count` bytes of the objects pointed to by `$lhs` and `$rhs`. The comparison is done
lexicographically.

## `memset( ... )`

```
memset( $dest, $ch, $count );
```

Copies the value `$ch` into each of the first `$count` characters of the object pointed to by `$dest`.

## `memcpy( ... )`

```
memcpy( $dest, $src, $count );
```

Copies `$count` characters from the object pointed to by `$src` to the object pointed to by `$dest`.

## `memmove( ... )`

```
memmove( $dest, $src, $count );
```

Copies `$count` characters from the object pointed to by `$src` to the object pointed to by `$dest`.

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

Returns the offset, in bytes, from the beginning of a structure including padding, if any.

# Utilities

Here's some thin cushions for the rougher edges of wrapping libraries.

They may be imported by name for now but might be renamed, removed, or changed in the future.

## `DumpHex( ... )`

```
DumpHex( $ptr, $length );
```

Dumps `$length` bytes of raw data from a given point in memory.

This is a debugging function that probably shouldn't find its way into your code and might not be public in the future.

## `sv_dump( ... )`

# Signatures

You must provide Affix with a signature which may include types and calling conventions. Let's start with an example in
C:

```
bool report( const char * name, int grades[5] );
```

The signature telling Affix how to call this function would look like this:

```perl
affix 'libschool', 'report', [ Str, Array[Int, 5] ] => Bool;
```

Incoming arguments are defined in a list: `[ Str, Array[Int, 5] ]`

The return value follows: `Bool`

To call the function, your Perl would look like this:

```perl
my $promote = report( 'Alex Smithe', [ 90, 82, 70, 76, 98 ] );
```

See the subsections entitled [Types](#types) for more on the possible types and ["Calling
Conventions" in Calling Conventions](https://metacpan.org/pod/Calling%20Conventions#Calling-Conventions) for advanced flags that may also be defined as part of your signature.

# Types

Affix supports the fundamental types (void, int, etc.) as well as aggregates (struct, array, union). Please note that
types given are advisory only! No type checking is done at compile or runtime.

See [Affix::Type](https://metacpan.org/pod/Affix%3A%3AType).

## Calling Conventions

Handle with care! Using these without understanding them can break your code!

Refer to [the dyncall manual](https://dyncall.org/docs/manual/manualse11.html),
[http://www.angelcode.com/dev/callconv/callconv.html](http://www.angelcode.com/dev/callconv/callconv.html), [https://en.wikipedia.org/wiki/Calling\_convention](https://en.wikipedia.org/wiki/Calling_convention), and your
local university's Comp Sci department for a deeper explanation.

After having done that, feel free to use or misuse any of the current options:

- `This`

    Platform native C++ this calls

- `Ellipsis`
- `Varargs`
- `CDecl`

    x86 specific

- `STDCall`

    x86 specific

- `MSFastcall`

    x86 specific

- `GNUFastcall`

    x86 specific

- `MSThis`

    x86 specific, MS C++ this calls

- `GNUThis`

    x86 specific, GNU C++ `this` calls are `cdecl`, but this is defined for clarity

- `Arm`
- `Thumb`
- `Syscall`

When used in ["Signatures" in signatures](https://metacpan.org/pod/signatures#Signatures), most of these cause the internal argument stack to be reset. The exceptions are
`Ellipsis` and `Varargs`.

# Calling into the Standard Library

If you want to call a function that's already loaded, either from the standard library or from your own program, you
can omit the library value or pass and explicit `undef`.

For example, on Unix, you could use the following code to gather the home directory and other info about the current
user:

```perl
use Affix;
use Data::Printer;
typedef PwStruct => Struct [
    name  => Str,     # username
    pass  => Str,     # hashed pass if shadow db isn't in use
    uuid  => UInt,    # user
    guid  => UInt,    # group
    gecos => Str,     # real name
    dir   => Str,     # ~/
    shell => Str      # bash, etc.
];
affix undef, 'getuid',   []    => Int;
affix undef, 'getpwuid', [Int] => Pointer [ PwStruct() ];
p( ( Pointer [ PwStruct() ] )->unmarshal( main::getpwuid( getuid() ) ) );
```

# ABI/API versions

If you ask Affix to load symbols from 'foo', we'll will look for `libfoo.so` under Unix, (`libfoo.dynlib` on macOS,
and `foo.dll` on Windows.

Most modern system require you or the user of your module to install the development package because it's recommended
to always provide an API/ABI version to a shared library, so `libfoo.so` ends often being a symbolic link provided
only by a development package.

To avoid that, the Affix allows you to specify the API/ABI version. It can be a full version or just a part of it. (Try
to stick to Major version, some BSD code does not care for Minor.)

```perl
use Affix;
affix ['foo', v1], ...;       # Will try to load libfoo.so.1 on Unix
affix ['foo', v1.2.3], ...;   # Will try to load libfoo.so.1.2.3 on Unix
```

# Stack Size

You may control the max size of the internal stack that will be allocated and used to bind the arguments to by setting
the `$VMSize` variable before using Affix.

```
BEGIN{ $Affix::VMSize = 2 ** 16; }
```

This value is `4096` by default and probably should not be changed.

# See Also

All the heavy lifting is done by [dyncall](https://dyncall.org/).

Check out [FFI::Platypus](https://metacpan.org/pod/FFI%3A%3APlatypus) for a more robust and mature FFI

[LibUI](https://metacpan.org/pod/LibUI) for a larger demo project based on Affix

[Types::Standard](https://metacpan.org/pod/Types%3A%3AStandard) for the inspiration of the advisory types system

# LICENSE

This software is Copyright (c) 2024 by Sanko Robinson <sanko@cpan.org>.

This is free software, licensed under:

```
The Artistic License 2.0 (GPL Compatible)
```

See [http://www.perlfoundation.org/artistic\_license\_2\_0](http://www.perlfoundation.org/artistic_license_2_0).

# AUTHOR

Sanko Robinson <sanko@cpan.org>
