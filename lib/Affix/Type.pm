package Affix::Type 1.0 {
    use strict;
    use warnings;
    use Affix qw[];

    # Types: // [ text, id, size, align, offset, subtype, length, aggregate, typedef, cast ]
    use overload
        '""' => sub { shift->[ Affix::SLOT_STRINGIFY() ] },
        '0+' => sub { shift->[ Affix::SLOT_NUMERIC() ] };
    sub sizeof { shift->[ Affix::SLOT_SIZEOF() ] }
    sub align  { shift->[ Affix::SLOT_ALIGNMENT() ] }
    sub offset { shift->[ Affix::SLOT_OFFSET() ] }
    sub cast   { $_[0]->[ Affix::SLOT_CAST() ] = $_[1]; $_[0] }
}
1;
