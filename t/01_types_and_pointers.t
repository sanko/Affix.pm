use strict;
use Test::More 0.98;
use Test::Warnings qw[warning :no_end_test];
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', 'lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix;
use utf8;
use t::lib::helper;
$|++;

#~ diag unpack 'C', pack 'C', -ord 'A';
#~ diag unpack 'c', pack 'c', -ord 'A';
#~ diag unpack 'W', '赤';
#~ diag ord '赤';
#~ use Test2::V0;
#~ Test2::Tools::Encoding::set_encoding('utf8');
binmode $_, "encoding(UTF-8)" for Test::More->builder->output, Test::More->builder->failure_output;
my $lib = compile_test_lib('01_types_and_pointers');

#~ warn `nm -D $lib`;
#
use Data::Dump;
subtest types => sub {
    isa_ok $_, 'Affix::Type::Base'
        for Void, Bool, Char, UChar, WChar, Short, UShort, Int, UInt, Long, ULong, LongLong,
        ULongLong, SSize_t, Size_t, Float, Double, Str,
        #
        WStr, Pointer [Int],
        CodeRef [ [ Pointer [Void], Double, Str, Array [ Str, 10 ], Pointer [WStr] ] => Str ],
        Struct [ i => Str, j => Long ], Union [ u => Int, x => Double ], Array [ Int, 10 ],
        InstanceOf ['Test::Class'];
};
subtest 'bool test(bool)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [Bool] => Bool ), 'Affix', 'wrap ..., [Bool] => Bool';
    is $code->(1), !1, 'test(1)';
    is $code->(0), 1,  'test(0)';
};
subtest 'bool test(int, bool*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Bool] ] => Bool ), 'Affix',
        'wrap ..., [Int, Pointer[Bool]] => Bool';
    is $code->( 0, [ 1, 1, 1, 1 ] ), 1,  '->(0, [1, 1, 1, 1]) == true';
    is $code->( 1, [ 0, 0, 0, 0 ] ), !1, '->(1, [0, 0, 0, 0]) == false';
    is $code->( 2, [ 0, 0, 1, 0 ] ), 1,  '->(2, [0, 0, 1, 0]) == true';
    is $code->( 3, [ 1, 1, 1, 0 ] ), !1, '->(3, [1, 1, 1, 0]) == false';
};
subtest 'bool test(int, int, bool**)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Int, Pointer [ Pointer [Bool] ] ] => Bool ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Bool]]] => Bool';
    is $code->( 1, 0, [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ] ), 1,  '->(1, 0, ...) == true';
    is $code->( 0, 1, [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ] ), !1, '->(0, 1, ...) == false';
};
subtest 'bool test(int, int, int, bool***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test', [ Int, Int, Int, Pointer [ Pointer [ Pointer [Bool] ] ] ] => Bool ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[Bool]]]] => Bool';
    is $code->(
        1, 0, 0,
        [   [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1, '->(1, 0, 0, ...) == true';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 0, 1 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        !1, '->(2, 0, 1, ...) == false';
};
subtest 'bool* Ret_BoolPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_BoolPtr', [] => Pointer [Bool] ), 'Affix',
        'Ret_BoolPtr ..., [ ] => Pointer[Bool]';
    isa_ok my $type = Array [ Bool, 3 ], 'Affix::Type::Array', 'my $type = Array[Bool, 3]';
    isa_ok my $ret  = $code->(),         'Affix::Pointer',     'bool *';
    is_deeply $type->unmarshal($ret), [ 1, !1, 1 ], 'unmarshal bool *';
};
subtest 'bool** Ret_BoolPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_BoolPtrPtr', [] => Pointer [ Pointer [Bool] ] ), 'Affix',
        'Ret_BoolPtrPtr ..., [ ] => Pointer[Pointer[Bool]]';
    isa_ok my $type = Array [ Array [ Bool, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Bool, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'bool **';
    is_deeply $type->unmarshal($ret), [ [ 1, !1, !1 ], [ !1, 1, !1 ], [ !1, !1, 1 ] ],
        'unmarshal bool **';
};

# Char
subtest 'char test(char)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [Char] => Char ), 'Affix', 'wrap ..., [Char] => Char';
    is chr $code->('M'),       'Y', 'test("M")';
    is chr $code->('N'),       'N', 'test("N")';
    is chr $code->(undef),     'N', 'test(undef)';
    is chr $code->( ord 'M' ), 'Y', 'test(ord "M")';
};
subtest 'char test(int, char*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Char] ] => Char ), 'Affix',
        'wrap ..., [Int, Pointer[Char]] => Char';
    is chr $code->( 0, [ 'a', 'b', 'c', 'd' ] ), 'a', '->(0, ["a", "b", "c", "d"]) == "a"';
    is chr $code->( 1, [ 'a', 'b', 'c', 'd' ] ), 'b', '->(1, ["a", "b", "c", "d"]) == "b"';
    is chr $code->( 2, [ 'a', 'b', 'c', 'd' ] ), 'c', '->(2, ["a", "b", "c", "d"]) == "c"';
    is chr $code->( 3, [ 'a', 'b', 'c', 'd' ] ), 'd', '->(3, ["a", "b", "c", "d"]) == "d"';
    is chr $code->( 3, "Testing" ), 't', '->(3, "Testing") == "t';
};
subtest 'int test(char*, char**)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Pointer [Char], Pointer [ Pointer [Char] ] ], Int ),
        'Affix', 'wrap ..., [Pointer[Char], Pointer[Pointer[Char]]] => Int';
    is $code->( "Alex", [ "John", "Bill", "Sam", "Martin", "Jose", "Alex", "Paul" ] ), 6,
        '->("Alex", [ "John", "Bill", "Sam", "Martin", "Jose", "Alex", "Paul" ] ) == 6';
};
subtest 'char test(int, char**)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [ Pointer [Char] ] ] => Pointer [Char] ),
        'Affix', 'wrap ..., [Int, Pointer[Pointer[Char]]] => Pointer[Char]';
    is $code->(
        2, [ [ 'S', 'g', '<', 'd', \0 ], [ 'l', 'q', 's', 'm', \0 ], [ 'k', 'b', '9', 'p', \0 ] ]
        ),
        'kb9p', '->(2 ...) == "kb9p"';
    is $code->( 0, [ "This", "is",  "a", "test" ] ), 'This', '->(0, ...) == "This"';
    is $code->( 3, [ "That", undef, "a", "test" ] ), 'test', '->(3, ...) == "test"';
    is $code->( 1, [ "That", undef, "a", "test" ] ), undef,  '->(1, ...) == undef';
};
subtest 'char* test(int, int, char***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test',
        [ Int, Int, Pointer [ Pointer [ Pointer [Char] ] ] ] => Pointer [Char] ), 'Affix',
        'wrap ..., [Int, Int, Pointer[Pointer[Pointer[Char]]]] => Pointer[Char]';
    is $code->(
        2, 3,
        [   [ "The", "cat",  "sat", "on",   "the", "mat" ],
            [ "I",   "love", "to",  "eat",  "pizza" ],
            [ "She", "went", "to",  "the",  "store" ],
            [ "He",  "is",   "a",   "very", "good", "boy" ]
        ]
        ),
        'the', '->(2, 3, ...) == "the"';
};
subtest 'char*** Ret_CharPtrPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_CharPtrPtrPtr', [] => Pointer [Void] ), 'Affix',
        'wrap ..., [] => Pointer[Void]';
    isa_ok my $ptr = $code->(), 'Affix::Pointer', '$code->()';
    subtest 'Str' => sub {
        isa_ok my $type = Array [ Array [ Str, 3 ], 3 ], 'Affix::Type::Array',
            'Array [ Array [ Str, 3 ], 3 ]';
        is_deeply $type->unmarshal($ptr),
            [ [ 'abc', 'def', 'ghi' ], [ 'jkl', 'mno', 'pqr' ], [ 'stu', 'vwx', 'yz{' ] ],
            'unmarshal returned pointer';
    };
    subtest 'Pointer[Char]' => sub {
        isa_ok my $type = Array [ Array [ Array [ Char, 3 ], 3 ], 3 ], 'Affix::Type::Array',
            'Array [ Array [ Array[Char, 3], 3 ], 3 ]';
        is_deeply $type->unmarshal($ptr),
            [
            [ [ "a", "b", "c" ], [ "d", "e", "f" ], [ "g", "h", "i" ] ],
            [ [ "j", "k", "l" ], [ "m", "n", "o" ], [ "p", "q", "r" ] ],
            [ [ "s", "t", "u" ], [ "v", "w", "x" ], [ "y", "z", "{" ] ],
            ],
            'unmarshal returned pointer';
    };
};

# UChar
subtest 'unsigned char test(unsigned char)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [UChar] => UChar ), 'Affix', 'wrap ..., [UChar] => UChar';
    is $code->('M'), 'Y', 'test(100)';
    is $code->(0),   'N', 'test(0)';
};
subtest 'unsigned char test(int, unsigned char*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [UChar] ] => UChar ), 'Affix',
        'wrap ..., [Int, Pointer[UChar]] => UChar';
    is $code->( 1, [ ' ', 'A', 'B', 'C' ] ), 'a', q['->(1, [' ', 'A', 'B', 'C'] == a];
    is $code->( 0, [ ' ', 'A', 'B', 'C' ] ), ' ', q['->(0, [' ', 'A', 'B', 'C'] == ' '];
    is $code->( 2, [ ' ', 'A', 'B', 'C' ] ), 'B', q['->(2, [' ', 'A', 'B', 'C'] == 'B'];
};
subtest 'unsigned char*** Ret_UCharPtrPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_UCharPtrPtrPtr', [] => Pointer [Void] ), 'Affix',
        'wrap ..., [] => Pointer[Void]';
    isa_ok my $ptr = $code->(), 'Affix::Pointer', '$code->()';
    subtest 'Str' => sub {
        isa_ok my $type = Array [ Array [ Str, 3 ], 3 ], 'Affix::Type::Array',
            'Array [ Array [ Str, 3 ], 3 ]';
        is_deeply $type->unmarshal($ptr),
            [ [ 'abc', 'def', 'ghi' ], [ 'jkl', 'mno', 'pqr' ], [ 'stu', 'vwx', 'yz{' ] ],
            'unmarshal returned pointer';
    };
    subtest 'Pointer[UChar]' => sub {
        isa_ok my $type = Array [ Array [ Array [ UChar, 3 ], 3 ], 3 ], 'Affix::Type::Array',
            'Array [ Array [ Array[UChar, 3], 3 ], 3 ]';
        is_deeply $type->unmarshal($ptr),
            [
            [ [ "a", "b", "c" ], [ "d", "e", "f" ], [ "g", "h", "i" ] ],
            [ [ "j", "k", "l" ], [ "m", "n", "o" ], [ "p", "q", "r" ] ],
            [ [ "s", "t", "u" ], [ "v", "w", "x" ], [ "y", "z", "{" ] ],
            ],
            'unmarshal returned pointer';
    };
};

# Short
subtest 'short test(short)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [Short] => Short ), 'Affix', 'wrap ..., [Short] => Short';
    is $code->(100), -100, 'test(100)';
    is $code->(0),   0,    'test(0)';
};
subtest 'short test(int, short char*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Short] ] => Short ), 'Affix',
        'wrap ..., [Int, Pointer[Short]] => Short';
    is $code->( 1, [ map {ord} ' ', 'A', 'B', 'C' ] ), -65,
        q['->(1, [ map {ord} ' ', 'A', 'B', 'C'] == -65];
    is $code->( 0, [ map {ord} ' ', 'A', 'B', 'C' ] ), -32,
        q['->(0, [ map {ord} ' ', 'A', 'B', 'C'] == -32];
    is $code->( 2, [ map {ord} ' ', 'A', 'B', 'C' ] ), -66,
        q['->(2, [ map {ord} ' ', 'A', 'B', 'C'] == -66];
};
subtest 'unsigned short*** Ret_ShortPtrPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_ShortPtrPtrPtr', [] => Pointer [Void] ), 'Affix',
        'wrap ..., [] => Pointer[Void]';
    isa_ok my $ptr = $code->(), 'Affix::Pointer', '$code->()';
    subtest 'Pointer[Short]' => sub {
        isa_ok my $type = Array [ Array [ Array [ Short, 3 ], 3 ], 3 ], 'Affix::Type::Array',
            'Array [ Array [ Array[Short, 3], 3 ], 3 ]';
        is_deeply $type->unmarshal($ptr),
            [
            [ [ 97,   -98,  99 ],   [ -100, 101,  -102 ], [ 103,  -104, 105 ] ],
            [ [ -106, 107,  -108 ], [ 109,  -110, 111 ],  [ -112, 113,  -114 ] ],
            [ [ 115,  -116, 117 ],  [ -118, 119,  -120 ], [ 121,  -122, 123 ] ],
            ],
            'unmarshal returned pointer';
    };
};

# UShort
subtest 'unsigned short test(unsigned short)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [UShort] => UShort ), 'Affix',
        'wrap ..., [UShort] => UShort';
    is $code->(100), 100, 'test(100)';
    is $code->(0),   0,   'test(0)';
};
subtest 'unsigned short test(int, unsigned short char*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [UShort] ] => UShort ), 'Affix',
        'wrap ..., [Int, Pointer[UShort]] => UShort';
    is $code->( 1, [ map {ord} ' ', 'A', 'B', 'C' ] ), 65,
        q['->(1, [ map {ord} ' ', 'A', 'B', 'C'] == 65];
    is $code->( 0, [ map {ord} ' ', 'A', 'B', 'C' ] ), 32,
        q['->(0, [ map {ord} ' ', 'A', 'B', 'C'] == 32];
    is $code->( 2, [ map {ord} ' ', 'A', 'B', 'C' ] ), 66,
        q['->(2, [ map {ord} ' ', 'A', 'B', 'C'] == 66];
};
subtest 'unsigned short*** Ret_UShortPtrPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_UShortPtrPtrPtr', [] => Pointer [Void] ), 'Affix',
        'wrap ..., [] => Pointer[Void]';
    isa_ok my $ptr = $code->(), 'Affix::Pointer', '$code->()';
    subtest 'Pointer[UShort]' => sub {
        isa_ok my $type = Array [ Array [ Array [ UShort, 3 ], 3 ], 3 ], 'Affix::Type::Array',
            'Array [ Array [ Array[UShort, 3], 3 ], 3 ]';
        is_deeply $type->unmarshal($ptr),
            [
            [ [ 97,  98,  99 ],  [ 100, 101, 102 ], [ 103, 104, 105 ] ],
            [ [ 106, 107, 108 ], [ 109, 110, 111 ], [ 112, 113, 114 ] ],
            [ [ 115, 116, 117 ], [ 118, 119, 120 ], [ 121, 122, 123 ] ],
            ],
            'unmarshal returned pointer';
    };
};

# Int
subtest 'int test(int)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [Int] => Int ), 'Affix', 'wrap ..., [Int] => Int';
    is $code->(100), -100, 'test(100)';
    is $code->(0),   0,    'test(0)';
    is $code->(-10), 10,   'test(-10)';
};
subtest 'int test(int, int*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Int] ] => Int ), 'Affix',
        'wrap ..., [Int, Pointer[Int]] => Int';
    is $code->( 0, [ 1,   11,    12,  13 ] ), -1,   '->(0, [1, 11, 12, 13]) == -1';
    is $code->( 1, [ 50,  40,    30,  20 ] ), -40,  '->(1, [50, 40, 30, 20]) == -40';
    is $code->( 2, [ 10,  80,    61,  0 ] ),  -61,  '->(2, [10, 80, 61, 0]) == -61';
    is $code->( 1, [ 10,  -1380, 61,  0 ] ),  1380, '->(1, [10, -1380, 61, 0]) == 1380';
    is $code->( 3, [ 10,  -1380, 61,  0 ] ),  0,    '->(3, [10, -1380, 61, 0]) == 0';
    is $code->( 3, [ 211, 21,    143, 83 ] ), -83,  '->(3, [211, 21, 143, 83]) == -83';
};
subtest 'int test(int, int, int**)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Int, Pointer [ Pointer [Int] ] ] => Int ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Int]]] => Int';
    is $code->( 1, 0, [ [ 199, 30, 15 ], [ 1934, 1, 1 ], [ !1, !1, 1 ] ] ), 1934,
        '->(1, 0, ...) == 199';
    is $code->( 0, 1, [ [ 187, 10, 16 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ] ), 10, '->(0, 1, ...) == 10';
};
subtest 'int test(int, int, int, int***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test', [ Int, Int, Int, Pointer [ Pointer [ Pointer [Int] ] ] ] => Int ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[Int]]]] => Int';
    is $code->(
        1, 0, 0,
        [   [ [ 112,  512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1434, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,    0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1434, '->(1, 0, 0, ...) == 1434';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 22,    1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        22, '->(2, 0, 1, ...) == 22';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, -22,   1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        -22, '->(2, 0, 1, ...) == -22';
};
subtest 'int** Ret_IntPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_IntPtrPtr', [] => Pointer [ Pointer [Int] ] ), 'Affix',
        'Ret_IntPtrPtr ..., [ ] => Pointer[Pointer[Int]]';
    isa_ok my $type = Array [ Array [ Int, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Int, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'int **';
    is_deeply $type->unmarshal($ret), [ [ 0, 1, 2 ], [ 3, 4, 5 ], [ 6, 7, 8 ] ], 'unmarshal int **';
};
subtest 'int*** Ret_IntPtrPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_IntPtrPtrPtr', [] => Pointer [ Pointer [ Pointer [Int] ] ] ),
        'Affix', 'Ret_IntPtrPtrPtr ..., [ ] => Pointer[Pointer[Pointer[Int]]]';
    isa_ok my $type = Array [ Array [ Array [ Int, 3 ], 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Array[Int, 3], 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'int **';
    is_deeply $type->unmarshal($ret),
        [
        [ [ 0,  1,  2 ],  [ 3,  4,  5 ],  [ 6,  7,  8 ] ],
        [ [ 9,  10, 11 ], [ 12, 13, 14 ], [ 15, 16, 17 ] ],
        [ [ 18, 19, 20 ], [ 21, 22, 23 ], [ 24, 25, 26 ] ],
        ],
        'unmarshal int ***';
};

# UInt
subtest 'unsigned int test(unsigned int)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [UInt] => UInt ), 'Affix', 'wrap ..., [UInt] => UInt';
    is $code->(100), 100, 'test(100)';
    is $code->(0),   0,   'test(0)';
};
subtest 'unsigned int test(int, unsigned int*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [UInt] ] => UInt ), 'Affix',
        'wrap ..., [Int, Pointer[UInt]] => UInt';
    is $code->( 0, [ 1,   11, 12,  13 ] ), 1,  '->(0, [1, 11, 12, 13]) == 1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'int test(int, int, int**)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Int, Pointer [ Pointer [Int] ] ] => Int ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Int]]] => Int';
    is $code->( 1, 0, [ [ 199, 30, 15 ], [ 1934, 1, 1 ], [ !1, !1, 1 ] ] ), 1934,
        '->(1, 0, ...) == 199';
    is $code->( 0, 1, [ [ 187, 10, 16 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ] ), 10, '->(0, 1, ...) == 10';
};
subtest 'unsigned int test(int, int, int, unsigned int***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test', [ Int, Int, Int, Pointer [ Pointer [ Pointer [UInt] ] ] ] => UInt ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[UInt]]]] => UInt';
    is $code->(
        1, 0, 0,
        [   [ [ 112,  512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1434, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,    0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1434, '->(1, 0, 0, ...) == 1434';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 22,    1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        22, '->(2, 0, 1, ...) == 22';
};
subtest 'unsigned int** Ret_UIntPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_UIntPtrPtr', [] => Pointer [ Pointer [UInt] ] ), 'Affix',
        'Ret_UIntPtrPtr ..., [ ] => Pointer[Pointer[UInt]]';
    isa_ok my $type = Array [ Array [ UInt, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[UInt, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'unsigned int **';
    is_deeply $type->unmarshal($ret), [ [ 0, 1, 2 ], [ 3, 4, 5 ], [ 6, 7, 8 ] ], 'unmarshal int **';
};
subtest 'unsigned int*** Ret_UIntPtrPtrPtr()' => sub {
    isa_ok my $code
        = wrap( $lib, 'Ret_UIntPtrPtrPtr', [] => Pointer [ Pointer [ Pointer [UInt] ] ] ), 'Affix',
        'Ret_UIntPtrPtrPtr ..., [ ] => Pointer[Pointer[Pointer[UInt]]]';
    isa_ok my $type = Array [ Array [ Array [ UInt, 3 ], 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Array[UInt, 3], 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'unsigned int **';
    is_deeply $type->unmarshal($ret),
        [
        [ [ 0,  1,  2 ],  [ 3,  4,  5 ],  [ 6,  7,  8 ] ],
        [ [ 9,  10, 11 ], [ 12, 13, 14 ], [ 15, 16, 17 ] ],
        [ [ 18, 19, 20 ], [ 21, 22, 23 ], [ 24, 25, 26 ] ],
        ],
        'unmarshal unsigned int ***';
};

# Long
subtest 'long test(long)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [Long] => Long ), 'Affix', 'wrap ..., [Long] => Long';
    is $code->(100), -100, 'test(100)';
    is $code->(0),   0,    'test(0)';
};
subtest 'long test(int, long*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Long] ] => Long ), 'Affix',
        'wrap ..., [Int, Pointer[Long]] => Long';
    is $code->( 0, [ 1,   11, 12,  13 ] ), 1,  '->(0, [1, 11, 12, 13]) == 1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'long test(int, int, long**)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Int, Pointer [ Pointer [Long] ] ] => Long ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Long]]] => Long';
    is $code->( 1, 0, [ [ 199, 30, 15 ], [ 1934, 1, 1 ], [ !1, !1, 1 ] ] ), 1934,
        '->(1, 0, ...) == 199';
    is $code->( 0, 1, [ [ 187, 10, 16 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ] ), 10, '->(0, 1, ...) == 10';
};
subtest 'long test(int, int, int, long***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test', [ Int, Int, Int, Pointer [ Pointer [ Pointer [Long] ] ] ] => Long ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[Long]]]] => Long';
    is $code->(
        1, 0, 0,
        [   [ [ 112,  512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1434, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,    0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1434, '->(1, 0, 0, ...) == 1434';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 22,    1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        22, '->(2, 0, 1, ...) == 22';
};
subtest 'long** Ret_LongPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_LongPtrPtr', [] => Pointer [ Pointer [Long] ] ), 'Affix',
        'Ret_LongPtrPtr ..., [ ] => Pointer[Pointer[Long]]';
    isa_ok my $type = Array [ Array [ Long, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Long, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'long **';
    is_deeply $type->unmarshal($ret), [ [ 0, 1, 2 ], [ 3, 4, 5 ], [ 6, 7, 8 ] ],
        'unmarshal long **';
};
subtest 'long*** Ret_LongPtrPtrPtr()' => sub {
    isa_ok my $code
        = wrap( $lib, 'Ret_LongPtrPtrPtr', [] => Pointer [ Pointer [ Pointer [Long] ] ] ), 'Affix',
        'Ret_LongPtrPtrPtr ..., [ ] => Pointer[Pointer[Pointer[Long]]]';
    isa_ok my $type = Array [ Array [ Array [ Long, 3 ], 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Array[Long, 3], 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'long **';
    is_deeply $type->unmarshal($ret),
        [
        [ [ 0,  1,  2 ],  [ 3,  4,  5 ],  [ 6,  7,  8 ] ],
        [ [ 9,  10, 11 ], [ 12, 13, 14 ], [ 15, 16, 17 ] ],
        [ [ 18, 19, 20 ], [ 21, 22, 23 ], [ 24, 25, 26 ] ],
        ],
        'unmarshal long ***';
};

# ULong
subtest 'unsigned long test(int, unsigned long*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [ULong] ] => ULong ), 'Affix',
        'wrap ..., [Int, Pointer[ULong]] => Long';
    is $code->( 0, [ 1,   11, 12,  13 ] ), 1,  '->(0, [1, 11, 12, 13]) == 1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'unsigned long test(int, int, int, unsigned long***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test', [ Int, Int, Int, Pointer [ Pointer [ Pointer [ULong] ] ] ] => ULong ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[ULong]]]] => ULong';
    is $code->(
        1, 0, 0,
        [   [ [ 112,  512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1434, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,    0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1434, '->(1, 0, 0, ...) == 1434';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 22,    1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        22, '->(2, 0, 1, ...) == 22';
};
subtest 'unsigned long** Ret_ULongPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_ULongPtrPtr', [] => Pointer [ Pointer [ULong] ] ), 'Affix',
        'Ret_ULongPtrPtr ..., [ ] => Pointer[Pointer[ULong]]';
    isa_ok my $type = Array [ Array [ ULong, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[ULong, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'unsigned long **';
    is_deeply $type->unmarshal($ret), [ [ 0, 1, 2 ], [ 3, 4, 5 ], [ 6, 7, 8 ] ],
        'unmarshal unsigned long **';
};

# LongLong
subtest 'long long test(int, long long*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [LongLong] ] => LongLong ), 'Affix',
        'wrap ..., [Int, Pointer[LongLong]] => LongLong';
    is $code->( 0, [ 1,   11, 12,  13 ] ), 1,  '->(0, [1, 11, 12, 13]) == 1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'long long test(int, int, int, long long***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test',
        [ Int, Int, Int, Pointer [ Pointer [ Pointer [LongLong] ] ] ] => LongLong ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[LongLong]]]] => LongLong';
    is $code->(
        1, 0, 0,
        [   [ [ 112,  512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1434, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,    0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1434, '->(1, 0, 0, ...) == 1434';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 22,    1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        22, '->(2, 0, 1, ...) == 22';
};
subtest 'long long** Ret_LongLongPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_LongLongPtrPtr', [] => Pointer [ Pointer [LongLong] ] ),
        'Affix', 'Ret_LongLongPtrPtr ..., [ ] => Pointer[Pointer[LongLong]]';
    isa_ok my $type = Array [ Array [ LongLong, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[LongLong, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'long long **';
    is_deeply $type->unmarshal($ret), [ [ 0, 1, 2 ], [ 3, 4, 5 ], [ 6, 7, 8 ] ],
        'unmarshal long long **';
};

# ULongLong
subtest 'unsigned long long test(int, unsigned long long*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [ULongLong] ] => ULongLong ), 'Affix',
        'wrap ..., [Int, Pointer[ULongLong]] => ULongLong';
    is $code->( 0, [ 1,   11, 12,  13 ] ), 1,  '->(0, [1, 11, 12, 13]) == 1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'unsigned long long test(int, int, int, unsigned long long***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test',
        [ Int, Int, Int, Pointer [ Pointer [ Pointer [ULongLong] ] ] ] => ULongLong ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[ULongLong]]]] => ULongLong';
    is $code->(
        1, 0, 0,
        [   [ [ 112,  512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1434, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,    0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        1434, '->(1, 0, 0, ...) == 1434';
    is $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 22,    1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        22, '->(2, 0, 1, ...) == 22';
};
subtest 'unsigned long long** Ret_ULongLongPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_ULongLongPtrPtr', [] => Pointer [ Pointer [ULongLong] ] ),
        'Affix', 'Ret_ULongLongPtrPtr ..., [ ] => Pointer[Pointer[ULongLong]]';
    isa_ok my $type = Array [ Array [ ULongLong, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[ULongLong, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'unsigned long long **';
    is_deeply $type->unmarshal($ret), [ [ 0, 1, 2 ], [ 3, 4, 5 ], [ 6, 7, 8 ] ],
        'unmarshal unsigned long long **';
};

# Float
subtest 'float test(int, float*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Float] ] => Float ), 'Affix',
        'wrap ..., [Int, Pointer[Float]] => Float';
    is sprintf( '%.2f', $code->( 0, [ 1.1, 11, 12, 13 ] ) ), '1.10',
        '->(0, [1.1, 11, 12, 13]) == 1.1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'float test(int, int, int, float***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test', [ Int, Int, Int, Pointer [ Pointer [ Pointer [Float] ] ] ] => Float ),
        'Affix', 'wrap ..., [Int, Int, Pointer[Pointer[Pointer[Float]]]] => Float';
    is_approx $code->(
        1, 0, 0,
        [   [ [ 112,   512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 14.34, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,     0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        14.34, '->(1, 0, 0, ...) == 14.34';
    is_approx $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 2.2,   1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        2.20, '->(2, 0, 1, ...) == 2.20';
};
subtest 'float ** Ret_FloatPtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_FloatPtrPtr', [] => Pointer [ Pointer [Float] ] ), 'Affix',
        'Ret_FloatPtrPtr ..., [ ] => Pointer[Pointer[Float]]';
    isa_ok my $type = Array [ Array [ Float, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Float, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'float **';
    my ($data) = $type->unmarshal($ret);
    my $ideal = [                          # Float math ain't perfect
        [ 1.000000, 1.010000, 1.020000 ], [ 1.100000, 1.110000, 1.120000 ],
        [ 1.200000, 1.210000, 1.220000 ]
    ];
    subtest 'unmarshall float **' => sub {
        for my $i ( 0 .. 2 ) {
            for my $j ( 0 .. 2 ) {
                is_approx $data->[$i][$j], $ideal->[$i][$j], sprintf '->[%d][%d] == %f', $i, $j,
                    $data->[$i][$j];
            }
        }
    };
};

# Double
subtest 'double test(int, double*)' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Int, Pointer [Double] ] => Double ), 'Affix',
        'wrap ..., [Int, Pointer[Double]] => Double';
    is sprintf( '%.2f', $code->( 0, [ 1.1, 11, 12, 13 ] ) ), '1.10',
        '->(0, [1.1, 11, 12, 13]) == 1.1';
    is $code->( 1, [ 50,  40, 30,  20 ] ), 40, '->(1, [50, 40, 30, 20]) == 40';
    is $code->( 2, [ 10,  80, 61,  0 ] ),  61, '->(2, [10, 80, 61, 0]) == 61';
    is $code->( 3, [ 211, 21, 143, 83 ] ), 83, '->(3, [211, 21, 143, 83]) == 83';
};
subtest 'double test(int, int, int, double***)' => sub {
    isa_ok my $code
        = wrap( $lib, 'test',
        [ Int, Int, Int, Pointer [ Pointer [ Pointer [Double] ] ] ] => Double ), 'Affix',
        'wrap ..., [Int, Int, Pointer[Pointer[Pointer[Double]]]] => Double';
    is_approx $code->(
        1, 0, 0,
        [   [ [ 112,   512, 79021 ], [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 14.34, 870, 1091 ],  [ 1, 1, 1 ], [ !1, !1, 1 ] ],
            [ [ 1,     0,   1 ],     [ 1, 1, 1 ], [ !1, !1, 1 ] ]
        ]
        ),
        14.34, '->(1, 0, 0, ...) == 14.34';
    is_approx $code->(
        2, 0, 1,
        [   [ [ 1, 323,   1 ], [ 1, 222, 1 ], [ !1, !1, 1 ] ],
            [ [ 1, 89030, 1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ],
            [ [ 1, 2.2,   1 ], [ 1, 1,   1 ], [ !1, !1, 1 ] ]
        ]
        ),
        2.20, '->(2, 0, 1, ...) == 2.20';
};
subtest 'double ** Ret_DoublePtrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_DoublePtrPtr', [] => Pointer [ Pointer [Double] ] ),
        'Affix', 'Ret_DoublePtrPtr ..., [ ] => Pointer[Pointer[Double]]';
    isa_ok my $type = Array [ Array [ Double, 3 ], 3 ], 'Affix::Type::Array',
        'my $type = Array[Array[Double, 3], 3]';
    isa_ok my $ret = $code->(), 'Affix::Pointer', 'double **';
    my ($data) = $type->unmarshal($ret);
    my $ideal = [                          # Float math ain't perfect
        [ 1.000000, 1.010000, 1.020000 ], [ 1.100000, 1.110000, 1.120000 ],
        [ 1.200000, 1.210000, 1.220000 ]
    ];
    subtest 'unmarshall double **' => sub {
        for my $i ( 0 .. 2 ) {
            for my $j ( 0 .. 2 ) {
                is_approx $data->[$i][$j], $ideal->[$i][$j], sprintf '->[%d][%d] == %f', $i, $j,
                    $data->[$i][$j];
            }
        }
    };
};

# Str
diag 'I can reuse the char* tests here';
subtest 'int test(char *, char **) as [ [ Str, Array [Str] ] => Int ]' => sub {
    isa_ok my $code = wrap( $lib, 'test', [ Str, Array [Str] ], Int ), 'Affix',
        'wrap ..., [ Str, Array[Str] ] => Int';
    is $code->( "Alex", [ "John", "Bill", "Sam", "Martin", "Jose", "Alex", "Paul" ] ), 6,
        '->("Alex", [ "John", "Bill", "Sam", "Martin", "Jose", "Alex", "Paul" ] ) == 6';
};
subtest 'char*** Ret_ArrayStr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_ArrayStr', [], Array [ Array [ Str, 2 ], 10 ] ), 'Affix',
        'wrap ..., [] => Array [ Array [ Str, 2 ], 10 ]';
    is_deeply $code->(),
        [
        [ "John",      "Doe" ],
        [ "Mary",      "Smith" ],
        [ "Michael",   "Brown" ],
        [ "Susan",     "Williams" ],
        [ "David",     "Johnson" ],
        [ "Jennifer",  "Jones" ],
        [ "William",   "Davis" ],
        [ "Elizabeth", "Wilson" ],
        [ "Richard",   "Miller" ],
        [ "Linda",     "Moore" ],
        ],
        'list of names as a char ***';
};

# WChar
subtest 'wchar_t * reverse(wchar_t * str)' => sub {
    isa_ok my $code = wrap( $lib, 'reverse', [WStr] => WStr ), 'Affix',
        'reverse ..., [ WStr ] => WStr';
    is $code->('虚空'), '空虚', '虚空 => 空虚';
};
subtest 'wchar_t ** Ret_WStrPtr()' => sub {
    isa_ok my $code = wrap( $lib, 'Ret_WStrPtr', [] => Array [ WStr, 5 ] ), 'Affix',
        'Ret_WStrPtr ..., [ WStr ] => Pointer[WStr]';
    is_deeply $code->(),
        [
        "안녕하세요", "감사합니다",
        "미안합니다", "잘 부탁합니다",
        "안녕히 계세요"
        ],
        '5 korean phrases returned';
};
subtest 'WStr marshaling' => sub {
    my $type = Pointer [WStr];
    for my $str (
        '赤',                                     '時空',
        'こんにちは、世界',                'Привет, мир!',
        '안녕하세요, 세계!',                'مرحبا بالعالم!',
        'नमस्ते दुनिया! ', '',
        undef,                                     <<'END') {
気付かれないでトドメを刺す
どの時代も生き延びてきた
嘘みたいな空の下
恐いものなんて憶えちゃいない

街をそっと見下ろして
気紛れに踏んづけたり
そこら中に火をつけた
そう言えば何て名前だったっけ

悲しみを全部引き受けたって大丈夫
手加減なんていらない
どこでだって誰の前でだって
ただ自分でいたい

引っ張り出した影の影
染みこんでる孤独な日々
世界中が苛ついたって
デタラメに今日もわめいてみせる

そんなに見ないで
ヒントずらしたくらいでいい
裸みたいな気分
浮き足立った未来に不満でも
目を覚ましていたい

悲しみを全部引き受けたって大丈夫
手加減なんていらない
どこでだって誰の前でだって
ただ自分でいたい
END
        my $ptr = $type->marshal($str);
        is $type->unmarshal($ptr), $str, defined $str ? $str eq '' ? "''" : $str : 'undef';
    }
};

# StdStr
# Struct
# Array
# Union
# Enum
# CPPStruct / Class
# CodeRef
# Pointer[Void]
# Pointer[SV]
done_testing;
__END__
subtest 'Pointer[WChar]' => sub {
    my $ptr = ( Pointer [WChar] )->marshal('赤');
    $ptr->dump(16);
    is( ( Pointer [WChar] )->unmarshal($ptr), '赤', 'wide char in and out' );
};

subtest 'Pointer[Int]' => sub {
    {
        my $ptr = ( Pointer [Int] )->marshal(3939);
        is( ( Pointer [Int] )->unmarshal($ptr), 3939, '3939 in and out' );
    }
    {
        my $ptr = ( Pointer [Int] )->marshal(-9);
        is( ( Pointer [Int] )->unmarshal($ptr), -9, '-9 in and out' );
    }
};
subtest 'Pointer[UInt]' => sub {
    {
        my $ptr = ( Pointer [UInt] )->marshal(55);
        is( ( Pointer [UInt] )->unmarshal($ptr), 55, '55 in and out' );
    }
    {
        my $ptr = ( Pointer [UInt] )->marshal(0);
        is( ( Pointer [UInt] )->unmarshal($ptr), 0, '0 in and out' );
    }
};
subtest 'Pointer[Long]' => sub {
    {
        my $ptr = ( Pointer [Long] )->marshal(3939);
        is( ( Pointer [Long] )->unmarshal($ptr), 3939, '3939 in and out' );
    }
    {
        my $ptr = ( Pointer [Long] )->marshal(-9);
        is( ( Pointer [Long] )->unmarshal($ptr), -9, '-9 in and out' );
    }
};
subtest 'Pointer[ULong]' => sub {
    {
        my $ptr = ( Pointer [ULong] )->marshal(55);
        is( ( Pointer [ULong] )->unmarshal($ptr), 55, '55 in and out' );
    }
    {
        my $ptr = ( Pointer [ULong] )->marshal(0);
        is( ( Pointer [ULong] )->unmarshal($ptr), 0, '0 in and out' );
    }
};
subtest 'Pointer[LongLong]' => sub {
    {
        my $ptr = ( Pointer [LongLong] )->marshal(3939);
        is( ( Pointer [LongLong] )->unmarshal($ptr), 3939, '3939 in and out' );
    }
    {
        my $ptr = ( Pointer [LongLong] )->marshal(-9);
        is( ( Pointer [LongLong] )->unmarshal($ptr), -9, '-9 in and out' );
    }
};
subtest 'Pointer[ULongLong]' => sub {
    {
        my $ptr = ( Pointer [ULongLong] )->marshal(55);
        is( ( Pointer [ULongLong] )->unmarshal($ptr), 55, '55 in and out' );
    }
    {
        my $ptr = ( Pointer [ULongLong] )->marshal(0);
        is( ( Pointer [ULongLong] )->unmarshal($ptr), 0, '0 in and out' );
    }
};
subtest 'Pointer[SSize_t]' => sub {
    {
        my $ptr = ( Pointer [SSize_t] )->marshal(3939);
        is( ( Pointer [SSize_t] )->unmarshal($ptr), 3939, '3939 in and out' );
    }
    {
        my $ptr = ( Pointer [SSize_t] )->marshal(-9);
        is( ( Pointer [SSize_t] )->unmarshal($ptr), -9, '-9 in and out' );
    }
};
subtest 'Pointer[Size_t]' => sub {
    {
        my $ptr = ( Pointer [SSize_t] )->marshal(55);
        is( ( Pointer [Size_t] )->unmarshal($ptr), 55, '55 in and out' );
    }
    {
        my $ptr = ( Pointer [Size_t] )->marshal(0);
        is( ( Pointer [Size_t] )->unmarshal($ptr), 0, '0 in and out' );
    }
};
subtest 'Pointer[Float]' => sub {
    {
        my $ptr = ( Pointer [Float] )->marshal(393.9);
        is( sprintf( '%.1f', ( Pointer [Float] )->unmarshal($ptr) ), 393.9, '393.9 in and out' );
    }
    {
        my $ptr = ( Pointer [Float] )->marshal(-9.5);
        is( sprintf( '%.1f', ( Pointer [Float] )->unmarshal($ptr) ), -9.5, '-9.5 in and out' );
    }
};
subtest 'Pointer[Double]' => sub {
    {
        my $ptr = ( Pointer [Double] )->marshal(393.9);
        is( sprintf( '%.1f', ( Pointer [Double] )->unmarshal($ptr) ), 393.9, '393.9 in and out' );
    }
    {
        my $ptr = ( Pointer [Double] )->marshal(-9.5);
        is( sprintf( '%.1f', ( Pointer [Double] )->unmarshal($ptr) ), -9.5, '-9.5 in and out' );
    }
};
subtest 'Pointer[Str]' => sub {
    {
        my $ptr = ( Pointer [Str] )->marshal('Oh, this is a test.');
        is(
            ( Pointer [Str] )->unmarshal($ptr),
            'Oh, this is a test.',
            q['Oh, this is a test.' in and out]
        );
    }
    {
        my $ptr = ( Pointer [Str] )->marshal(undef);
        is( ( Pointer [Str] )->unmarshal($ptr), '', q[undef in and out] );
    }
    {
        my $ptr = ( Pointer [Str] )->marshal('');
        is( ( Pointer [Str] )->unmarshal($ptr), '', q['' in and out] );
    }
};
subtest 'Pointer[CodeRef[...]]' => sub {
    plan tests => 3;
    my $type = Pointer [ CodeRef [ [] => Int ] ];
    my $ptr  = $type->marshal( sub { pass 'in callback'; 55; } );
    my $ref  = $type->unmarshal($ptr);
    isa_ok $ref , 'CODE';
    is $ref->(), 55, 'return value from callback (55)';
};
subtest 'Pointer[Struct[...]]' => sub {
    pass 'dummy';
    my $type = Pointer [ Struct [ i => Str, j => Long ] ];
    diag Affix::sizeof(Long);
    diag Affix::sizeof( Struct [ i => Str, j => Long ] );
    my $ptr = $type->marshal( { i => 'String', j => 10000 } );
    $ptr->dump(31);
    my $ref = $type->unmarshal($ptr);
    isa_ok $ref , 'HASH';
    is_deeply $ref, { i => 'String', j => 10000 }, 'same hash is returned';
};
subtest 'Pointer[Union[...]]' => sub {
    {
        my $type = Pointer [ Union [ i => Str, j => Long ] ];
        {
            my $ptr = $type->marshal( { i => 'String' } );
            my $ref = $type->unmarshal($ptr);
            isa_ok $ref , 'HASH';
            is $ref->{i}, 'String', '->{i} value';
            diag $ref->{j};
        }
        {
            my $ptr = $type->marshal( { j => 500 } );
            my $ref = $type->unmarshal($ptr);
            isa_ok $ref , 'HASH';
            is_deeply $ref->{j}, 500, '->{j} value';

            # TODO: trying to access undef union slots crashes
            #diag $ref->{i};
        }
    }
    {
        my $type = Pointer [ Union [ i => Int, f => Float ] ];
        my $data = { f => 3.14 };
        my $ptr  = $type->marshal($data);
        is_deeply sprintf( '%1.2f', $type->unmarshal($ptr)->{f} ), 3.14, 'round trip is correct';
        #
        $data = { i => 6 };
        $ptr  = $type->marshal($data);
        is_deeply $type->unmarshal($ptr)->{i}, 6, 'round trip is correct';
    };
};
subtest 'Pointer[Array[...]]' => sub {
    my $type = Pointer [ Array [ Int, 10 ] ];
    subtest 'call something' => sub {
        isa_ok my $code = Affix::wrap( $lib, 'take_pointer_to_array_of_ints', [$type], Int ),
            'Affix', 'Int take_pointer_to_array_of_ints(Pointer [ Array [ Int, 10 ] ])';
        is $code->( [ 1 .. 10 ] ), 55, '$code->( [ 1 .. 10 ] )';
    };
    {
        my $ptr = $type->marshal( [ 1 .. 10 ] );
        my $ref = $type->unmarshal($ptr);
        isa_ok $ref, 'ARRAY', 'unmarshal of array';
        is_deeply $ref, [ 1 .. 10 ], 'same array is returned';
    }

=fdsaf

    {
        my $type = Pointer [ Array [ Struct [ alpha => Str, numeric => Int ], 3 ] ];
        my $data = [
            { alpha => 'Smooth',   numeric => 4 },
            { alpha => 'Move',     numeric => 2 },
            { alpha => 'Ferguson', numeric => 0 }
        ];
        warn;
        my $ptr = $type->marshal($data);
        warn;
        is_deeply [ $type->unmarshal($ptr) ], [$data], 'round trip is correct';
    }
    {
        my $type = Pointer [ Array [ CodeRef [ [Str] => Str ], 3 ] ];
        my $ptr  = $type->marshal(
            [   sub { is shift, 'one',   'proper args passed to 1st'; 'One' },
                sub { is shift, 'two',   'proper args passed to 2nd'; 'Two' },
                sub { is shift, 'three', 'proper args passed to 3rd'; 'Three' }
            ]
        );
        my $cv = $type->unmarshal($ptr);
        is scalar @$cv,         3,       '3 coderefs unpacked';
        is $cv->[0]->('one'),   'One',   'proper return value from 1st';
        is $cv->[1]->('two'),   'Two',   'proper return value from 2nd';
        is $cv->[2]->('three'), 'Three', 'proper return value from 3rd';
    };
=cut

};

subtest 'Pointer[Any]' => sub {
    my $type = Pointer [Any];
    my $ptr  = $type->marshal( { four => 'four' } );
    isa_ok $ptr, 'Affix::Pointer::Unmanaged';
    is_deeply [ $type->unmarshal($ptr) ], [ { four => 'four' } ], 'SV* cast to void*';

    #subtest 'affix' => sub {
    #ok Affix::wrap( $lib, 'set_sv_pointer', [Any], Void )->('Test'), '->("Test")';
    #is Affix::wrap( $lib, 'get_sv_pointer', [], Any )->(), 'Test', '->() => "Test"';
};
done_testing;
