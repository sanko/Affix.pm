package Affix::Type 1.0 {
    use strict;
    use warnings;

    # Types: [ text, id, size, align, [ offset, etc. ] ]
    use overload '""' => sub { shift->[0] }, '0+' => sub { ord shift->[1] };
    sub sizeof { shift->[2] }
    sub align  { shift->[3] }
    sub offset { shift->[4] }
}
1;
