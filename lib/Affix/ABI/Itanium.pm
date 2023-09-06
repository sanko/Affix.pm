package Affix::ABI::Itanium 1.0 {
    use strict;
    use warnings;
    use Affix qw[:types :flags];
    use Data::Dump;
    sub encoding () {'_Z'}

    sub mangle {
        my ( $name, $args ) = @_;
        $args = [Void] unless @$args;
        ddx $args;
        my $symbol = encoding . length($name) . $name;
        $symbol;
    }
}
1;

package main;
use strict;
use warnings;
use Affix qw[:all];
use Data::Dump;
warn Affix::ABI::Itanium::mangle( 'test', [] => Int );
my $void1 = Void;
my $void2 = Void;
$void1->[4] = 'hey';
ddx $void1;
ddx $void2;
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

Affix::ABI::Itanim supports the following features right out of the box:

=over

=item L<name mangling|https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling>

=back

=head1 Basics

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
