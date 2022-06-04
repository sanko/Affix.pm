package Dyn::Types {
    use strictures 2;
    use Data::Dump;
    use Types::Standard qw[ArrayRef Int Undef Str Ref Object];
	use Types::Common::Numeric;
    use Type::Library -base;
    #
    __PACKAGE__->add_type(
        name       => 'UInt',
        parent     => Types::Common::Numeric::PositiveOrZeroInt(),

    );
    __PACKAGE__->add_type(
        name       => 'UFloat',
        parent     => Types::Common::Numeric::PositiveOrZeroNum()


    );
}
