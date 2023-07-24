use strict;
use Test::More 0.98;
use Test::Warnings qw[warning :no_end_test];
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
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
#
subtest types => sub {
    isa_ok $_, 'Affix::Type::Base'
        for Void, Bool, Char, UChar, WChar, Short, UShort, Int, UInt, Long, ULong, LongLong,
        ULongLong, SSize_t, Size_t, Float, Double, Str,
        #
        WStr, Pointer [Int],
        CodeRef [ [ Pointer [Void], Double, Str, ArrayRef [ Str, 10 ], Pointer [WStr] ] => Str ],
        Struct [ i => Str, j => Long ], Union [ u => Int, x => Double ], ArrayRef [ Int, 10 ],
        InstanceOf ['Test::Class'];
};
subtest 'Pointer[Int]' => sub {
    my $type = Pointer [Int];
    my $code = Affix::wrap( $lib, 'testIntPointer', [ Int, $type ], Bool );
    subtest 'marshal=>unmarshal' => sub {
        {
            my $ptr = $type->marshal(3939);
            is( $type->unmarshal($ptr), 3939, '3939 in and out' );
        }
        {
            my $ptr = $type->marshal(-9);
            is( $type->unmarshal($ptr), -9, '-9 in and out' );
        }
    };
    subtest 'marshal=>code=>unmarshal' => sub {
        isa_ok my $ptr = $type->marshal(100), 'Affix::Pointer', '$type->marshal(100)';
        ok $code->( 100, $ptr ), '$code->(100, $ptr)';
        is $type->unmarshal($ptr), 100, '$ptr->unmarshal';
        isa_ok $ptr = $type->marshal(1000), 'Affix::Pointer', '$type->marshal(1000)';
        ok !$code->( 100, $ptr ), '!$code->(100, $ptr)';
    };
    subtest 'passing Pointer[Int] to a function' => sub {
        ok $code->( 1000, 1000 ), '$code->(1000, 1000)';
        ok !$code->( 100, 1000 ), '!$code->(100, 1000)';
        ok $code->( 1,    1.1 ),  '$code->(1, 1.1) [float is cast to int]';
        ok $code->( "1",  1 ),    '$code->("1", 1)';

        #~ like(
        #~ warning {
        ok !$code->( 1, "This should break" ), '!$code->(1, "This should break")';

        #~ },
        #~ qr/numeric/,
        #~ '...btw, strings are not all numeric'
        #~ );
    };
};
subtest 'Pointer[Pointer[Int]]' => sub {
    my $type = Pointer [ Pointer [Int] ];
    my $code = Affix::wrap( $lib, 'testIntPointerPointer', [ Int, $type ], Bool );
    subtest 'marshal=>unmarshal' => sub {
        {
            my $ptr = $type->marshal(3939);
            is( $type->unmarshal($ptr), 3939, '3939 in and out' );
        }
        {
            my $ptr = $type->marshal(-9);
            is( $type->unmarshal($ptr), -9, '-9 in and out' );
        }
    };
    subtest 'marshal=>code=>unmarshal' => sub {
        isa_ok my $ptr = $type->marshal(100), 'Affix::Pointer', '$type->marshal(100)';
        ok $code->( 100, $ptr ), '$code->(100, $ptr)';
        is $type->unmarshal($ptr), 100, '$ptr->unmarshal';
        isa_ok $ptr = $type->marshal(1000), 'Affix::Pointer', '$type->marshal(1000)';
        ok !$code->( 100, $ptr ), '!$code->(100, $ptr)';
    };
    subtest 'passing Pointer[Pointer[Int]] to a function' => sub {
        ok $code->( 1000, 1000 ), '$code->(1000, 1000)';
        ok !$code->( 100, 1000 ), '!$code->(100, 1000)';
        ok $code->( 1,    1.1 ),  '$code->(1, 1.1) [float is cast to int]';
        ok $code->( "1",  1 ),    '$code->("1", 1)';

        #~ like(    # Affix built for debugging throws a ton of warnings
        #~ warning {
        ok !$code->( 1, "This should break" ), '!$code->(1, "This should break")';

        #~ },
        #~ qr/numeric/,
        #~ '...btw, strings are not all numeric'
        #~ );
    };
};
subtest 'Pointer[Bool]' => sub {
    my $type = Pointer [Bool];
    my $code = Affix::wrap( $lib, 'testBoolPointer', [ Bool, $type ], Bool );
    subtest 'marshal=>unmarshal' => sub {
        {
            my $ptr = $type->marshal(1);
            is( $type->unmarshal($ptr), 1, '1 in and out' );
        }
        {
            my $ptr = $type->marshal(0);
            is( $type->unmarshal($ptr), !1, '!1 in and out' );
        }
    };
    subtest 'marshal=>code=>unmarshal' => sub {
        isa_ok my $ptr = $type->marshal(1), 'Affix::Pointer', '$type->marshal(1)';
        ok $code->( 1, $ptr ), '$code->(1, $ptr)';
        is $type->unmarshal($ptr), 1, '$ptr->unmarshal';
        isa_ok $ptr = $type->marshal(0), 'Affix::Pointer', '$type->marshal(0)';
        ok !$code->( 1, $ptr ), '!$code->(1, $ptr)';
    };
    subtest 'passing Pointer[Bool] to a function' => sub {
        ok $code->( 0,  0 ),                   '$code->(0, 0)';
        ok !$code->( 1, 0 ),                   '!$code->(1, 0)';
        ok $code->( 1,  "This should break" ), '$code->(1, "This should break")';
        diag '...btw, strings can be boolean';
    };
};
subtest 'Pointer[Pointer[Bool]]' => sub {
    my $type = Pointer [ Pointer [Bool] ];
    my $code = Affix::wrap( $lib, 'testBoolPointerPointer', [ Bool, $type ], Bool );
    subtest 'marshal=>unmarshal' => sub {
        {
            my $ptr = $type->marshal(1);
            is( $type->unmarshal($ptr), 1, '1 in and out' );
        }
        {
            my $ptr = $type->marshal(0);
            is( $type->unmarshal($ptr), !1, '!1 in and out' );
        }
    };
    subtest 'marshal=>code=>unmarshal' => sub {
        isa_ok my $ptr = $type->marshal(1), 'Affix::Pointer', '$type->marshal(1)';
        ok $code->( 1, $ptr ), '$code->(1, $ptr)';
        is $type->unmarshal($ptr), 1, '$ptr->unmarshal';
        isa_ok $ptr = $type->marshal(0), 'Affix::Pointer', '$type->marshal(0)';
        ok !$code->( 1, $ptr ), '!$code->(1, $ptr)';
    };
    subtest 'passing Pointer[Pointer[Bool]] to a function' => sub {
        ok $code->( 0,  0 ),                   '$code->(0, 0)';
        ok !$code->( 1, 0 ),                   '!$code->(1, 0)';
        ok $code->( 1,  "This should break" ), '$code->(1, "This should break")';
        diag '...btw, strings can be boolean';
    };
};
subtest 'Pointer[Void]' => sub {
    my $type = Pointer [Void];

    #~ my $ptr = Affix::malloc(5);
    #$$ptr = 'test';
    my $ptr = $type->marshal('test');
    isa_ok $ptr, 'Affix::Pointer::Unmanaged';

    #~ $ptr->dump(16);
    diag __LINE__;

    #~ diag $ptr->deref_scalar;
    diag $type->unmarshal($ptr);
    is $ptr->raw(4), 'test', '->raw is correct';
    $ptr->free;
};
subtest 'Pointer[Void]' => sub {
    my $type = Pointer [Void];

    #~ my $ptr = Affix::malloc(5);
    #$$ptr = 'test';
    my $ptr = $type->marshal('test');
    isa_ok $ptr, 'Affix::Pointer::Unmanaged';

    #~ $ptr->dump(16);
    diag __LINE__;

    #~ diag $ptr->deref_scalar;
    diag $type->unmarshal($ptr);
    is $ptr->raw(4), 'test', '->raw is correct';
    $ptr->free;
};
diag __LINE__;
subtest 'Pointer[Char]' => sub {
    my $type = Pointer [Char];
    {
        my $ptr = $type->marshal('Abcd');
        $ptr->dump(5);
        is $type->unmarshal($ptr),        'Abcd',   'Abcd in and out';
        is int( $type->unmarshal($ptr) ), ord('A'), 'Abcd in and out (int)';
    }
    {
        my $ptr = $type->marshal( 'Abcd' x 64 );
        $ptr->dump(257);
        diag $type->unmarshal($ptr);

        #~ die;
        is $type->unmarshal($ptr),        'Abcd' x 64, 'Abcd in and out';
        is int( $type->unmarshal($ptr) ), ord('A'),    'Abcd in and out (int)';
    }
    {
        my $ptr = $type->marshal( ord 'A' );
        is int( $type->unmarshal($ptr) ), ord('A'), 'A in and out (int)';
    }
    {
        my $ptr = $type->marshal( -ord 'A' );
        is int( $type->unmarshal($ptr) ), -ord('A'), '-A in and out (int)';
    }
};
subtest 'Pointer[UChar]' => sub {
    {
        my $ptr = ( Pointer [UChar] )->marshal('Abcd');
        $ptr->dump(30);
        is( ( Pointer [UChar] )->unmarshal($ptr),        'Abcd',   'Abcd in and out' );
        is( int( ( Pointer [UChar] )->unmarshal($ptr) ), ord('A'), 'Abcd in and out (int)' );
    }
    {
        my $ptr = ( Pointer [UChar] )->marshal( ord 'A' );
        is( int( ( Pointer [UChar] )->unmarshal($ptr) ), ord('A'), 'A in and out (int)' );
    }
    {
        my $ptr = ( Pointer [UChar] )->marshal( -ord 'A' );
        is( int( ( Pointer [UChar] )->unmarshal($ptr) ), -ord('A'), '-A in and out (int)' );
    }
};
subtest 'Pointer[WChar]' => sub {
    my $ptr = ( Pointer [WChar] )->marshal('赤');
    pass 'dummy';
    $ptr->dump(16);
    is( ( Pointer [WChar] )->unmarshal($ptr), '赤', 'wide char in and out' );
};
subtest 'Pointer[Short]' => sub {
    {
        my $ptr = ( Pointer [Short] )->marshal(3939);
        is( ( Pointer [Short] )->unmarshal($ptr), 3939, '3939 in and out' );
    }
    {
        my $ptr = ( Pointer [Short] )->marshal(-9);
        is( ( Pointer [Short] )->unmarshal($ptr), -9, '-9 in and out' );
    }
};
subtest 'Pointer[UShort]' => sub {
    {
        my $ptr = ( Pointer [UShort] )->marshal(55);
        is( ( Pointer [UShort] )->unmarshal($ptr), 55, '55 in and out' );
    }
    {
        my $ptr = ( Pointer [UShort] )->marshal(0);
        is( ( Pointer [UShort] )->unmarshal($ptr), 0, '0 in and out' );
    }
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
subtest 'Pointer[WStr]' => sub {
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
subtest 'Pointer[ArrayRef[...]]' => sub {
    my $type = Pointer [ ArrayRef [ Int, 10 ] ];
    subtest 'call something' => sub {
        isa_ok my $code = Affix::wrap( $lib, 'take_pointer_to_array_of_ints', [$type], Int ),
            'Affix', 'Int take_pointer_to_array_of_ints(Pointer [ ArrayRef [ Int, 10 ] ])';
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
        my $type = Pointer [ ArrayRef [ Struct [ alpha => Str, numeric => Int ], 3 ] ];
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
        my $type = Pointer [ ArrayRef [ CodeRef [ [Str] => Str ], 3 ] ];
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
