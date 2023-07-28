[![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/linux.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/windows.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/osx.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![Actions Status](https://github.com/sanko/Affix.pm/actions/workflows/freebsd.yaml/badge.svg)](https://github.com/sanko/Affix.pm/actions) [![MetaCPAN Release](https://badge.fury.io/pl/Affix.svg)](https://metacpan.org/release/Affix)
# NAME

Affix - A Foreign Function Interface eXtension

# SYNOPSIS

```perl
use Affix;

# bind to exported function
affix 'm', 'floor', [Double], Double;
warn floor(3.14159); # 3

# wrap an exported function in a code reference
my $getpid = wrap 'c', 'getpid', [], Int;
warn $getpid->(); # $$

# bind an exported value to a Perl value
pin( my $ver, 'libfoo', 'VERSION', Int );
```

# DESCRIPTION

Affix is an [FFI](https://en.wikipedia.org/wiki/Foreign_function_interface) to
wrap libraries developed in other languages (C, C++, Rust, etc.) without having
to write or maintain XS.

## Features

Affix supports the following features right out of the box:

- Works on Windows, macOS, Linux, BSD, and more
- Callbacks
- Pointers
- Typedefs
- Global/Exported variables
- Libraries developed in C, C++, and Rust (and more to come!) even those with mangled symbol names
- Aggregates such as structs, unions, and arrays
- Passing aggregates by value
- Nested aggregates
- 'Smart' enums
- Tested to work all the way down to Perl 5.026 (which is ancient in my book)

# Basics

Affix's basic API is rather simple but not lacking in power. Let's start at the
beginning with the eponymous `affix( ... )` function.

## `affix( ... )`

Attach a given symbol in a named perl sub.

```perl
affix 'C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double;
warn pow( 3, 5 );

affix 'c', 'puts', [Str], Int;
puts( 'Hello' );

affix 'c', ['puts', 'write'], [Str], Int; # renamed function
write( 'Hello' );

affix undef, [ 'rint', 'round' ], [Double], Double; # use current process
warn round(3.14);

affix [ 'xxhash', '0.8.2' ], [ 'XXH_versionNumber', 'xxHash::version' ], [], UInt;
warn xxHash::version();
```

Expected parameters include:

- `lib`

    File path or name of the library to load symbols from. Pass an explicit
    `undef` to pull functions from the main executable.

    Optionally, you may provide an array reference with the library and a version
    number if the library was built with such a value as part of its filename.

- `symbol_name`

    Name of the symbol to wrap.

    Optionally, you may provide an array reference with the symbol's name and the
    name of the wrapping subroutine.

- `parameters`

    Provide the argument types in an array reference.

- `return`

    A single return type for the function.

On success, `affix( ... )` returns the generated code reference which may be
called directly but you'll likely use the name you provided.

## `wrap( ... )`

Creates a wrapper around a given symbol in a given library.

```perl
my $pow = wrap( 'C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );
warn $pow->(5, 10); # 5**10
```

Parameters include:

- `lib`

    File path or name of the library to load symbols from. Pass an explicit
    `undef` to pull functions from the main executable.

    Optionally, you may provide an array reference with the library and a version
    number if the library was built with such a value as part of its filename.

- `symbol_name`

    Name of the symbol to wrap.

- `parameters`

    Provide the argument types in an array reference.

- `return`

    A single return type for the function.

`wrap( ... )` behaves exactly like `affix( ... )` but returns an anonymous
subroutine and does not pollute the namespace.

## `pin( ... )`

```perl
my $errno;
pin $errno, 'libc', 'errno', Int;
print $errno;
$errno = 0;
```

Variables exported by a library - also referred to as "global" or "extern"
variables - can be accessed using `pin( ... )`. The above example code applies
magic to `$error` that binds it to the integer variable named "errno" as
exported by the [libc](https://metacpan.org/pod/libc) library.

Expected parameters include:

- `var`

    Perl scalar that will be bound to the exported variable.

- `lib`

    File path or name of the library to load symbols from. Pass an explicit
    `undef` to pull functions from the main executable.

    Optionally, you may provide an array reference with the library and a version
    number if the library was built with such a value as part of its filename.

- `symbol_name`

    Name of the exported variable to wrap.

- `$type`

    Indicate to Affix what type of data the variable contains.

This is likely broken on BSD but patches are welcome.

## `:Native` CODE attribute

All the sugar is right here in the :Native code attribute. This part of the API
is inspired by [Raku's `native`
trait](https://docs.raku.org/language/nativecall).

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

### Changing names

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

### Signatures

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

### ABI/API version

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

### Library Paths and Names

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

### Calling into the standard library

If you want to call a function that's already loaded, either from the standard
library or from your own program, you can omit the library value or pass and
explicit `undef`.

For example, on Unix, you could use the following code to gather the home
directory and other info about the current user:

```perl
use Affix qw[:all];
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
sub getuid : Native : Signature([]=>Int);
sub getpwuid : Native : Signature([Int]=>Pointer[PwStruct()]);
p( ( Pointer [ PwStruct() ] )->unmarshal( main::getpwuid( getuid() ) ) );
```

# Memory Functions

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

# Utility Functions

Here's some thin cushions for the rougher edges of wrapping libraries.

They may be imported by name for now but might be renamed, removed, or changed
in the future.

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
    <table>     <thead>       <tr>         <th>Affix</th> <th>C99</th>
    <th>Rust</th> <th>C#</th> <th>pack()</th> <th>Raku</th>       </tr> </thead>   
     <tbody>       <tr>         <td>Void</td> <td>void</td> <td>-&gt;()</td>
    <td>void/NULL</td> <td>-</td> <td></td>       </tr>       <tr>        
    <td>Bool</td> <td>_Bool</td> <td>bool</td> <td>bool</td> <td>-</td>
    <td>bool</td>       </tr>       <tr>         <td>Char</td> <td>int8_t</td>     
       <td>i8</td>         <td>sbyte</td>         <td>c</td>       <td>int8</td>   
       </tr>       <tr>         <td>UChar</td> <td>uint8_t</td>         <td>u8</td>
            <td>byte</td>         <td>C</td>       <td>byte, uint8</td>       </tr>
          <tr>         <td>Short</td>  <td>int16_t</td>         <td>i16</td>       
     <td>short</td> <td>s</td>         <td>int16</td>       </tr>       <tr>
    <td>UShort</td>         <td>uint16_t</td>         <td>u16</td> <td>ushort</td> 
           <td>S</td>         <td>uint16</td>       </tr> <tr>         <td>Int</td>
            <td>int32_t</td>         <td>i32</td> <td>int</td>         <td>i</td>  
          <td>int32</td>       </tr>       <tr>       <td>UInt</td>        
    <td>uint32_t</td>         <td>u32</td> <td>uint</td>         <td>I</td>        
    <td>uint32</td>       </tr>       <tr>         <td>Long</td>        
    <td>int64_t</td>         <td>i64</td> <td>long</td>         <td>l</td>        
    <td>int64, long</td>       </tr> <tr>         <td>ULong</td>        
    <td>uint64_t</td>         <td>u64</td>    <td>ulong</td>         <td>L</td>    
        <td>uint64, ulong</td>       </tr>       <tr>         <td>LongLong</td>    
        <td>-/long long</td> <td>i128</td>         <td>q</td>        
    <td>longlong</td>         <td></td>    </tr>       <tr>        
    <td>ULongLong</td>         <td>-/unsigned long long</td>         <td>u128</td> 
           <td>Q</td>         <td>ulonglong</td>       <td></td>       </tr>      
    <tr>         <td>Float</td> <td>float</td>         <td>f32</td>        
    <td>f</td>         <td>num32</td>       <td></td>       </tr>       <tr>       
     <td>Double</td> <td>double</td>         <td>f64</td>         <td>d</td>       
     <td>num64</td>        <td></td>       </tr>       <tr>        
    <td>SSize_t</td> <td>SSize_t</td>         <td>SSize_t</td>         <td></td>   
         <td></td>       <td></td>       </tr>       <tr>         <td>Size_t</td>
    <td>size_t</td>         <td>size_t</td>         <td></td>         <td></td>    
    <td></td>       </tr>       <tr>         <td>Str</td>         <td>char *</td>  
          <td></td>         <td></td>         <td></td>         <td></td>     
    </tr>       <tr>         <td>WStr</td>         <td>wchar_t</td> <td></td>      
      <td></td>         <td></td>       </tr>     </tbody> </table>
</div>

Given sizes are minimums measured in bits

Other types are also defined according to the system's platform. See
[Affix::Types](https://metacpan.org/pod/Affix%3A%3ATypes).

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
