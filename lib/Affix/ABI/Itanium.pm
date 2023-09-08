package Affix::ABI::Itanium 1.0 {
    use strict;
    use warnings;
    use lib '../../lib', '../../blib/arch', '../../blib/lib';
    use Affix qw[:types :flags];
    use Data::Dump;
    #
    sub prefix () {'_Z'}
    #
    sub bare_function_type ($) {

        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.bare-function-type
        #~ <bare-function-type> ::= <signature type>+
        #~       types are possible return type, then parameter types
        my ($types) = @_;
        my @ret;
        for my $type (@$types) {
            my $type_id = int $type;
            CORE::state $builtin_types //= {
                ord VOID_FLAG()      => 'v',
                ord WCHAR_FLAG()     => 'w',
                ord BOOL_FLAG()      => 'b',
                ord CHAR_FLAG()      => 'c',
                ord SCHAR_FLAG()     => 'a',
                ord UCHAR_FLAG()     => 'h',
                ord SHORT_FLAG()     => 's',
                ord USHORT_FLAG()    => 't',
                ord INT_FLAG()       => 'i',
                ord UINT_FLAG()      => 'j',
                ord LONG_FLAG()      => 'l',
                ord ULONG_FLAG()     => 'm',
                ord LONGLONG_FLAG()  => 'x',
                ord ULONGLONG_FLAG() => 'y',

                # int i128 => 'n', # Unsupported by dyncall
                # int u128 => 'o', # Unsupported by dyncall
                ord FLOAT_FLAG()  => 'f',
                ord DOUBLE_FLAG() => 'd',

# int LongDouble() => 'e', # Unsupported by dyncall
# int float128 => 'g',  # Unsupported by dyncall
#~ int ellipsis() => 'z', # TODO: Add CC back
# THe following are all unsupported by dyncall
#~ ::= Dd # IEEE 754r decimal floating point (64 bits)
#~ ::= De # IEEE 754r decimal floating point (128 bits)
#~ ::= Df # IEEE 754r decimal floating point (32 bits)
#~ ::= Dh # IEEE 754r half-precision floating point (16 bits)
#~ ::= DF <number> _ # ISO/IEC TS 18661 binary floating point type _FloatN (N bits), C++23 std::floatN_t
#~ ::= DF <number> x # IEEE extended precision formats, C23 _FloatNx (N bits)
#~ ::= DF16b # C++23 std::bfloat16_t
#~ ::= DB <number> _        # C23 signed _BitInt(N)
#~ ::= DB <instantiation-dependent expression> _ # C23 signed _BitInt(N)
#~ ::= DU <number> _        # C23 unsigned _BitInt(N)
#~ ::= DU <instantiation-dependent expression> _ # C23 unsigned _BitInt(N)
#~ ::= Di # char32_t
#~ ::= Ds # char16_t
#~ ::= Du # char8_t
#~ ::= Da # auto
#~ ::= Dc # decltype(auto)
#~ ::= Dn # std::nullptr_t (i.e., decltype(nullptr))
#~ ::= u <source-name> [<template-args>] # vendor extended type
            };
            if ( exists $builtin_types->{$type_id} ) {
                push @ret, $builtin_types->{$type_id};
            }
            else {
                use Data::Dump;
                ddx $builtin_types;
                ddx $builtin_types->{$type_id};
                ddx $type;
            }
        }
        return join '', @ret;
    }

    sub nested_name (@) {

      #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.nested-name
      #~ <nested-name> ::= N [<CV-qualifiers>] [<ref-qualifier>] <prefix> <unqualified-name> E
      #~               ::= N [<CV-qualifiers>] [<ref-qualifier>] <template-prefix> <template-args> E
      #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.CV-qualifiers
      #~ <CV-qualifiers>      ::= [r] [V] [K] 	  # restrict (C99), volatile, const
        my @scope = @_;
        join '', 'N', ( map { length($_) . $_ } @scope ), 'E';
    }

    sub name($) {

        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.name
        #~  <name> ::= <nested-name>
        #~         ::= <unscoped-name>
        #~         ::= <unscoped-template-name> <template-args>
        #~         ::= <local-name>	# See Scope Encoding below
        #~
        #~ <unscoped-name> ::= <unqualified-name>
        #~                 ::= St <unqualified-name>   # ::std::
        #~
        #~ <unscoped-template-name> ::= <unscoped-name>
        #~ 		                    ::= <substitution>
        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.unqualified-name
        #~ <unqualified-name> ::= <operator-name> [<abi-tags>]
        #~                     ::= <ctor-dtor-name>
        #~                     ::= <source-name>
        #~                     ::= <unnamed-type-name>
        #~                     ::= DC <source-name>+ E      # structured binding declaration
        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.special-name
        #~ <ctor-dtor-name> ::= C1			# complete object constructor
        #~                  ::= C2			# base object constructor
        #~                  ::= C3			# complete object allocating constructor
        #~                  ::= CI1 <base class type>	# complete object inheriting constructor
        #~                  ::= CI2 <base class type>	# base object inheriting constructor
        #~                  ::= D0			# deleting destructor
        #~                  ::= D1			# complete object destructor
        #~                  ::= D2			# base object destructor
        my ($name) = @_;
        my @name   = split /::/, $name;
        scalar @name > 1 ? nested_name(@name) : length($name) . $name;
    }

    sub encoding($$) {

        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.encoding
        #~ <encoding> ::= <function name> <bare-function-type>
        #~            ::= <data name>
        #~            ::= <special-name>
        my ( $name, $args ) = @_;
        $args ? name($name) . bare_function_type($args) :    # function name/special name
            name($name);                                     # type name

        # TODO: special name https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.special-name
    }

    sub mangle ($;$$$) {
        my $affix = shift if ref $_[0];

        #~ https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-structure
        #~ <mangled-name> ::= _Z <encoding>
        #~                ::= _Z <encoding> . <vendor-specific suffix>
        my ( $name, $args, $ret ) = @_;
        $args = [Void] if defined($args) && !@$args;
        return prefix . encoding( $name, $args );
    }
}
1;

package main;
use strict;
use warnings;
use Affix qw[:all];
use Test::More;
use Data::Dump;
$|++;
ddx Affix::typedef bar => Int;
#
is Affix::ABI::Itanium::mangle( 'f', []     => Void ), '_Z1fv', 'Ret? f()';
is Affix::ABI::Itanium::mangle( 'f', [Void] => Void ), '_Z1fv', 'Ret? f(void)';
is Affix::ABI::Itanium::mangle( 'f', [Int]  => Void ), '_Z1fi', 'Ret? f(int)';
is Affix::ABI::Itanium::mangle( 'f', [ Affix::typedef bar => Int ] => Void ), '_Z3foo3bar',
    'Ret? foo(bar)';
#
is Affix::ABI::Itanium::mangle('N::f'), '_ZN1N1fE', 'Type? N::f';
is Affix::ABI::Itanium::mangle( 'System::Sound::beep', [] => Void ), '_ZN6System5Sound4beepEv',
    'Ret? System::Sound::beep()';
is Affix::ABI::Itanium::mangle('Arena::level'), '_ZN5Arena5levelE', 'Type? Arena::level';
#
done_testing;
__END__
=encoding utf-8

=head1 NAME

Affix::ABI::Itanium - ABI Support for C++

=head1 DESCRIPTION

The Itanium ABI specification standardizes (most) of the interfaces between
different user-provided C++ program fragments and between those fragments and
the implementation-provided runtime and libraries. The C++ standard itself does
not attempt to create a single mangling scheme but most popular compilers (GCC
3+, Clang 1+, Intel C++ 8+) use Itanium. Microsoft uses its own unique design
so check out L<Affix::ABI::Microsoft>.

=head2 Features

Affix::ABI::Itanium supports the following features right out of the box:

=over

=item L<name mangling|https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling>

=back

=head1 Unsupported

Itanium is large and this is still under development. For now, this is what I
know is unsupported:

=over

=item operators

=item Virtual Tables/RTTI

=item Virtual Override Thunks

=item Guard Variables

=item Livetime-Extended Temporaries

=item Transaction-Safe Function Entry Points

=item Type encoding

=over

=item templates

=item ref-qualifier (&, &&) as we have no Ref[...] type yet

=item l-value reference

=item r-value reference (C++11)

=item complex pair (C99)

=item imaginary floating types (C99)

=item CV-qualifiers (r, V, K) as we have no way to flag restrict(C99), volatile, or const flags yet

=back

=back

=head1 Examples

Very short examples might find their way into the L<Affix::Cookbook>. The best
example of use might be L<LibUI>. Brief examples will be found in C<eg/>.

=head1 See Also

https://www.agner.org/optimize/calling_conventions.pdf

=head1 LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.

=head1 AUTHOR

Sanko Robinson E<lt>sanko@cpan.orgE<gt>

=begin stopwords

=end stopwords

=cut