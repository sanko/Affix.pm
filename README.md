[![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/linux.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/windows.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/osx.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/freebsd.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![MetaCPAN Release](https://badge.fury.io/pl/Affix.svg)](https://metacpan.org/release/Affix)
# NAME

Affix - 'FFI' is my middle name!

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

Dyn is a wrapper around [dyncall](https://dyncall.org/). If you're looking to
design your own low level wrapper, see [Dyn.pm](https://metacpan.org/pod/Dyn).

# `:Native` CODE attribute

While most of the upstream API is covered in the [Dyn::Call](https://metacpan.org/pod/Dyn%3A%3ACall),
[Dyn::Callback](https://metacpan.org/pod/Dyn%3A%3ACallback), and [Dyn::Load](https://metacpan.org/pod/Dyn%3A%3ALoad) packages, all the sugar is right here in
`Affix`. The most simple use of `Affix` would look something like this:

    use Affix ':all';
    sub some_argless_function : Native('somelib.so') : Signature([]=>Void);
    some_argless_function();

The second line above looks like a normal Perl sub declaration but includes the
`:Native` CODE attribute which specifies that the sub is actually defined in a
native library.

To avoid banging your head on a built-in function, you may name your sub
anything else and let Affix know what symbol to attach:

    sub my_abs : Native('my_lib.dll') : Signature([Double]=>Double) : Symbol('abs');
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

If you want to call a C function that's already loaded, either from the
standard library or from your own program, you can omit the value, so is
native.

For example on a UNIX-like operating system, you could use the following code
to print the home directory of the current user:

    use Affix;

    typedef PwStruct => Struct[
        name => Str,
        pass => Str,
        uuid => UInt,
        guid => UInt,
        gecos => Str,
        dir   => Str,
        shell => Str
    ];

    sub getuid:Native :Signature([]=>UInt);
    sub getuid:Native :Signature([UInt]=>PwStruct);
    CORE::say getpwuid(getuid())->{pw_dir};

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
