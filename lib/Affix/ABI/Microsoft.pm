package Affix::ABI::Microsoft 1.0 {
    use strict;
    use warnings;
    use Affix qw[:types :flags];

    sub mangle {
        my ( $name, $args ) = @_;
        $args = [Void] unless @$args;
        ...;
    }
}
1;
__END__
=encoding utf-8

=head1 NAME

Affix::ABI::Microsoft - ABI Support for Microsoft's Visual C++ Compilers

=head1 DESCRIPTION




=head2 Features

Affix::ABI::Microsoft supports the following features right out of the box:

=over

=item L<name mangling|https://en.wikiversity.org/wiki/Visual_C%2B%2B_name_mangling>

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
