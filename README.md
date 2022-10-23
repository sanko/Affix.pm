[![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/linux.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/windows.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/osx.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![Actions Status](https://github.com/sanko/Dyn.pm/actions/workflows/freebsd.yaml/badge.svg)](https://github.com/sanko/Dyn.pm/actions) [![MetaCPAN Release](https://badge.fury.io/pl/Affix.svg)](https://metacpan.org/release/Affix)
# NAME

Affix - 'FFI' is my middle name!

# SYNOPSIS

    use Affix;
    attach( ( $^O eq 'MSWin32' ? 'ntdll' : 'libm' ), 'pow', [ Double, Double ] => Double );
    print pow( 2, 10 );    # 1024

# DESCRIPTION

Dyn is a wrapper around [dyncall](https://dyncall.org/). If you're looking to
design your own low level wrapper, see [Dyn.pm](https://metacpan.org/pod/Dyn).

# `:Native` CODE attribute

While most of the upstream API is covered in the [Dyn::Call](https://metacpan.org/pod/Dyn%3A%3ACall),
[Dyn::Callback](https://metacpan.org/pod/Dyn%3A%3ACallback), and [Dyn::Load](https://metacpan.org/pod/Dyn%3A%3ALoad) packages, all the sugar is right here in
`Affix`. The most simple use of `Affix` would look something like this:

    use Affix ':all';
    sub some_argless_function() : Native('somelib.so') : Signature([]=>Void);
    some_argless_function();

Be aware that this will look a lot more like [NativeCall from
Raku](https://docs.raku.org/language/nativecall) before v1.0!

The second line above looks like a normal Perl sub declaration but includes the
`:Native` attribute to specify that the sub is actually defined in a native
library.

To avoid banging your head on a built-in function, you may name your sub
anything else and let Dyn know what symbol to attach:

    sub my_abs : Native('my_lib.dll') : Signature([Double]=>Double) : Symbol('abs');
    CORE::say my_abs( -75 ); # Should print 75 if your abs is something that makes sense

This is by far the fastest way to work with this distribution but it's not by
any means the only way.

All of the following methods may be imported by name or with the `:sugar` tag.

Note that everything here is subject to change before v1.0.

# Functions

The less

## `wrap( ... )`

Creates a wrapper around a given symbol in a given library.

    my $pow = Dyn::wrap( 'C:\Windows\System32\user32.dll', 'pow', [Double, Double]=>Double );
    warn $pow->(5, 10); # 5**10

Expected parameters include:

- `lib` - pointer returned by [`dlLoadLibrary( ... )`](https://metacpan.org/pod/Dyn%3A%3ALoad#dlLoadLibrary) or the path of the library as a string
- `symbol_name` - the name of the symbol to call
- `signature` - signature defining argument types, return type, and optionally the calling convention used

Returns a code reference.

## `attach( ... )`

Wraps a given symbol in a named perl sub.

    Dyn::attach('C:\Windows\System32\user32.dll', 'pow', [Double, Double] => Double );

# Signatures

`dyncall` uses an almost `pack`-like syntax to define signatures. Affix is
inspired by [Type::Standard](https://metacpan.org/pod/Type%3A%3AStandard):

- `Void`
- `Int`

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
