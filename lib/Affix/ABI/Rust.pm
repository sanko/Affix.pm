package Affix::ABI::Rust 1.0 {
    use strict;
    use warnings;
    use Affix qw[:types :flags];

    sub mangle ($;$$$) {
        my $affix = shift if ref $_[0];
        my ( $name, $args, $ret ) = @_;
        $args = [Void] if defined($args) && !@$args;
        return $name;
        ...;
    }
}
1;
__END__
=encoding utf-8

=head1 NAME

Affix::ABI::Rust - ABI Support for the Rust Programming Language

=head1 DESCRIPTION



=head2 Features

Affix::ABI::Rust supports the following features right out of the box:

=over

=item L<name mangling|https://internals.rust-lang.org/t/pre-rfc-a-new-symbol-mangling-scheme/8501>

=back

=head1 Basics

=head1 Examples

Very short examples might find their way into the L<Affix::Cookbook>. The best example of use might be L<LibUI>. Brief
examples will be found in C<eg/>.

=head1 See Also

TODO

=head1 LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under the terms found in the Artistic License
2. Other copyrights, terms, and conditions may apply to data transmitted through this module.

=head1 AUTHOR

Sanko Robinson E<lt>sanko@cpan.orgE<gt>

=begin stopwords


=end stopwords

=cut
