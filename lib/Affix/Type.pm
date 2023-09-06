package Affix::Type 1.0 {
    use strict;
    use warnings;
    use Affix qw[];

    # Types: // [ text, id, size, align, offset, subtype, length, aggregate, typedef ]
    use overload
        '""' => sub { shift->[ Affix::SLOT_STRINGIFY() ] },
        '0+' => sub { ord shift->[ Affix::SLOT_NUMERICAL() ] };
    sub sizeof { shift->[ Affix::SLOT_SIZEOF() ] }
    sub align  { shift->[ Affix::SLOT_ALIGNMENT() ] }
    sub offset { shift->[ Affix::SLOT_OFFSET() ] }
}
1;
