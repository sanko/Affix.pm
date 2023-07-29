use strict;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
use Config;
$|++;
#
use t::lib::helper;
#
my $lib = compile_test_lib('51_affix_sizeof_offsetof');
subtest 'fundamental types' => sub {

    #diag sizeof(Double);
    #die;
    is sizeof(Bool),     Affix::wrap( $lib, 's_bool',     [], Size_t )->(),  'sizeof(Bool)';
    is sizeof(Char),     Affix::wrap( $lib, 's_char',     [], Size_t )->(),  'sizeof(Char)';
    is sizeof(Short),    Affix::wrap( $lib, 's_short',    [], Size_t )->(),  'sizeof(Short)';
    is sizeof(Int),      Affix::wrap( $lib, 's_int',      [], Size_t )->(),  'sizeof(Int)';
    is sizeof(Long),     Affix::wrap( $lib, 's_long',     [], Size_t )->(),  'sizeof(Long)';
    is sizeof(LongLong), Affix::wrap( $lib, 's_longlong', [], Size_t )->(),  'sizeof(LongLong)';
    is sizeof(Float),    Affix::wrap( $lib, 's_float',    [], Size_t )->(),  'sizeof(Float)';
    is sizeof(Double),   Affix::wrap( $lib, 's_double',   [], Size_t )->(),  'sizeof(Double)';
    is sizeof(SSize_t),  Affix::wrap( $lib, 's_ssize_t',  [], SSize_t )->(), 'sizeof(SSize_t)';
    is sizeof(Size_t),   Affix::wrap( $lib, 's_size_t',   [], Size_t )->(),  'sizeof(Size_t)';
    is sizeof( Pointer [Void] ), Affix::wrap( $lib, 's_pointer', [], Size_t )->(),
        'sizeof(Pointer[Void])';
    is sizeof( Struct [ i => Int, f => Float, a => Array [ Int, 7 ], c => Char, z => Str ] ),
        Affix::wrap( $lib, 's_struct', [], Size_t )->(), 'sizeof(Struct[...])';
    is sizeof( Union [ i => Int, f => Float, a => Array [ Int, 7 ], c => Char, z => Str ] ),
        Affix::wrap( $lib, 's_union', [], Size_t )->(), 'sizeof(Union[...])';
};

#exit;
Affix::typedef massive => Struct [
    B => Bool,
    c => Char,
    C => UChar,
    s => Short,
    S => UShort,
    i => Int,
    I => UInt,
    j => Long,
    J => ULong,
    l => LongLong,
    L => ULongLong,
    f => Float,
    d => Double,
    p => Pointer [Int],
    Z => Str,
    A => Struct [ i => Int ],
    u => Union [ i => Int, structure => Struct [ ptr => Pointer [Void], l => Long ] ]
];

#use Data::Dumper;
#diag Dumper massive();
subtest 'array' => sub {
    is sizeof( Array [ Char, 3 ] ), 3, 'sizeof(Array [ Char, 3 ])';
    is sizeof( Array [ Pointer [Void], 1 ] ),
        Affix::wrap( $lib, 's_pointer_array', [], Size_t )->(),
        'sizeof(Array [ Pointer[Void], 1 ])';
    is sizeof( Array [ Str, 3 ] ), Affix::wrap( $lib, 's_string_array', [], Size_t )->(),
        'sizeof(Array [ Str, 3 ])';
};
subtest 'aggregates' => sub {
    is sizeof( Struct [] ), 0, 'empty struct is 0 bytes';
    my $struct1 = Struct [ c => Array [ Char, 3 ] ];
    my $struct2 = Struct [ c => Array [ Int,  3 ] ];
    my $struct3 = Struct [ d => Double, c => Array [ Int, 3 ] ];
    my $struct4 = Struct [ y => $struct3 ];    # Make sure we are padding cached size data
    my $struct5 = Struct [ y => Struct [ d => Double, c => Array [ Int, 3 ] ] ];
    my $struct6 = Struct [ y => $struct3, s => $struct4, c => Char ];
    my $struct7 = Struct [ i => Int,    Z => Str ];
    my $struct8 = Struct [ d => Double, c => Array [ Int, 4 ] ];
    subtest 'structs' => sub {

        #use Data::Dumper;
        #diag Dumper $struct1;
        is sizeof($struct1), Affix::wrap( $lib, 's_struct1', [], Size_t )->(), 'sizeof(struct1)';
        is sizeof($struct2), Affix::wrap( $lib, 's_struct2', [], Size_t )->(), 'sizeof(struct2)';
        is sizeof($struct3), Affix::wrap( $lib, 's_struct3', [], Size_t )->(), 'sizeof(struct3)';
        is sizeof($struct4), Affix::wrap( $lib, 's_struct4', [], Size_t )->(), 'sizeof(struct4)';
        is sizeof($struct5), Affix::wrap( $lib, 's_struct5', [], Size_t )->(), 'sizeof(struct5)';
    SKIP: {
            #skip 'perl defined bad var sizes with -Duselongdouble before 5.36.x', 2
            #    if ( $Config{uselongdouble} || $Config{usequadmath} ) && $^V lt v5.36.1;
            is sizeof($struct6), Affix::wrap( $lib, 's_struct6', [], Size_t )->(),
                'sizeof(struct6)';
            is sizeof( massive() ), Affix::wrap( $lib, 's_massive', [], Size_t )->(),
                'sizeof(massive)';
        }

        #diag Dumper $struct7;
        is sizeof($struct7), Affix::wrap( $lib, 's_struct7', [], Size_t )->(), 'sizeof(struct7)';
        is sizeof($struct8), Affix::wrap( $lib, 's_struct8', [], Size_t )->(), 'sizeof(struct8)';
    };
    subtest 'arrays' => sub {

        #die sizeof( Struct [ d => Double, c => Array [ Int, 4 ] ]);
        for my $length ( 1 .. 3 ) {
            my $array1 = Array [ Struct [ d => Double, c => Array [ Int, 4 ] ], $length ];

            #diag Dumper $array1;
            is sizeof($array1), Affix::wrap( $lib, 's_array1', [Int], Size_t )->($length),
                'sizeof(array1) [' . $length . ']';
        }
    };
    subtest 'unions' => sub {
        my $union1 = Union [ i => Int, d => Float ];
        my $union2 = Union [ i => Int, s => $struct1, d => Float ];
        my $union3 = Union [ i => Int, s => $struct3, d => Float ];
        my $union4 = Union [ i => Int, s => Array [ $struct1, 5 ], d => Float ];
        is sizeof($union1), Affix::wrap( $lib, 's_union1', [], Size_t )->(), 'sizeof(union1)';
        is sizeof($union2), Affix::wrap( $lib, 's_union2', [], Size_t )->(), 'sizeof(union2)';
    SKIP: {
            #skip 'perl defined bad var sizes with -Duselongdouble before 5.36.x', 2
            #    if ( $Config{uselongdouble} || $Config{usequadmath} ) && $^V lt v5.36.1;
            is sizeof($union3), Affix::wrap( $lib, 's_union3', [], Size_t )->(), 'sizeof(union3)';
            is sizeof($union4), Affix::wrap( $lib, 's_union4', [], Size_t )->(), 'sizeof(union4)';
        }
    };
    is sizeof( Pointer [Void] ), Affix::wrap( $lib, 's_voidptr', [], Size_t )->(), 'sizeof(void *)';
};
subtest 'offsetof' => sub {
    is offsetof( massive(), 'B' ), Affix::wrap( $lib, 'o_B', [], Size_t )->(), 'offsetof(..., "B")';
    is offsetof( massive(), 'c' ), Affix::wrap( $lib, 'o_c', [], Size_t )->(), 'offsetof(..., "c")';
    is offsetof( massive(), 'C' ), Affix::wrap( $lib, 'o_C', [], Size_t )->(), 'offsetof(..., "C")';
    is offsetof( massive(), 's' ), Affix::wrap( $lib, 'o_s', [], Size_t )->(), 'offsetof(..., "s")';
    is offsetof( massive(), 'S' ), Affix::wrap( $lib, 'o_S', [], Size_t )->(), 'offsetof(..., "S")';
    is offsetof( massive(), 'i' ), Affix::wrap( $lib, 'o_i', [], Size_t )->(), 'offsetof(..., "i")';
    is offsetof( massive(), 'I' ), Affix::wrap( $lib, 'o_I', [], Size_t )->(), 'offsetof(..., "I")';
    is offsetof( massive(), 'j' ), Affix::wrap( $lib, 'o_j', [], Size_t )->(), 'offsetof(..., "j")';
    is offsetof( massive(), 'J' ), Affix::wrap( $lib, 'o_J', [], Size_t )->(), 'offsetof(..., "J")';
    is offsetof( massive(), 'l' ), Affix::wrap( $lib, 'o_l', [], Size_t )->(), 'offsetof(..., "l")';
    is offsetof( massive(), 'L' ), Affix::wrap( $lib, 'o_L', [], Size_t )->(), 'offsetof(..., "L")';
    is offsetof( massive(), 'f' ), Affix::wrap( $lib, 'o_f', [], Size_t )->(), 'offsetof(..., "f")';
    is offsetof( massive(), 'd' ), Affix::wrap( $lib, 'o_d', [], Size_t )->(), 'offsetof(..., "d")';
    is offsetof( massive(), 'p' ), Affix::wrap( $lib, 'o_p', [], Size_t )->(), 'offsetof(..., "p")';
    is offsetof( massive(), 'Z' ), Affix::wrap( $lib, 'o_Z', [], Size_t )->(), 'offsetof(..., "Z")';
};

#diag Dumper massive();
#
done_testing;
