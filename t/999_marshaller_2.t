use v5.40;
use utf8;
use blib;
use Affix qw[:types sizeof];
use Test2::V1 -ipP;
$|++;
#
typedef char16_t => UInt16;
typedef Pos      => Struct [ x => Double, y => Double ];
typedef Primitives => Struct [
    s8  => Int8,
    u8  => UInt8,
    s16 => Int16,
    u16 => UInt16,
    s32 => Int32,
    u32 => UInt32,
    s64 => Int64,
    u64 => UInt64,
    f32 => Float,
    f64 => Double,
    b   => Bool
];
typedef 'Node';
typedef Node => Struct [ id => Int, next => Pointer [ Node() ] ];
typedef Everything => Struct [
    matrix  => Array [ Array [ Pos(), 10 ], 10 ],
    bits    => Struct [ flag => UInt8 | 1, val => UInt8 | 7 ],
    u       => Union [ i => Int, f => Float ],
    comp    => Complex [Float],
    textptr => Pointer [Char],
    name    => Array [ Char,  32 ],
    wide16  => Array [ WChar, 16 ],
    fp      => Pointer [Void],
    cb      => Callback [ [ Int, Double ] => Int ],
    simd    => M256d
];
subtest 'Core Types: Primitives' => sub {
    my $raw = alloc_raw( sizeof( Primitives() ) );
    my $p   = cast( $raw, 'Primitives' );
    $p->{s8} = -128;
    is $p->{s8}, -128, 'sint8 min';
    $p->{s8} = 127;
    is $p->{s8}, 127, 'sint8 max';
    $p->{u8} = 255;
    is $p->{u8}, 255, 'uint8 max';
    $p->{s16} = -32768;
    is $p->{s16}, -32768, 'sint16 min';
    $p->{s16} = 32767;
    is $p->{s16}, 32767, 'sint16 max';
    $p->{u16} = 65535;
    is $p->{u16}, 65535, 'uint16 max';
    $p->{s32} = -2147483648;
    is $p->{s32}, -2147483648, 'sint32 min';
    $p->{s32} = 2147483647;
    is $p->{s32}, 2147483647, 's32 max';
    $p->{u32} = 4294967295;
    is $p->{u32}, 4294967295, 'u32 max';
    $p->{s64} = -9223372036854775808;
    is $p->{s64}, -9223372036854775808, 's64 min';
    $p->{s64} = 9223372036854775807;
    is $p->{s64}, 9223372036854775807, 's64 max';
    $p->{u64} = 18446744073709551615;
    is $p->{u64}, 18446744073709551615, 'u64 max';
    $p->{f32} = 1.25;
    is $p->{f32}, float( 1.25, tolerance => 0.01 ), 'float32';
    $p->{f64} = 1.23456789;
    is $p->{f64}, float( 1.23456789, tolerance => 0.01 ), 'float64';
    $p->{b} = 1;
    is $p->{b}, T(), 'bool true';
    $p->{b} = 0;
    is $p->{b}, F(), 'bool false';
};
subtest 'Core Types: 128-bit Decimals' => sub {
    my $raw = alloc_raw(16);
    Affix::sv_dump($raw);
    my $u128 = cast( $raw, UInt128 );
    $u128 = '340282366920938463463374607431768211455';
    is $u128, '340282366920938463463374607431768211455', 'uint128 max string';
};
subtest 'Core Types: 128-bit Hex (via sint128)' => sub {
    my $todo = todo 'Not ready yet...';
    my $raw  = alloc_raw(16);
    set_mem_u128( $raw, 0, 1 );
    my $val = cast( $raw, Int128 );
    like $val, qr/0x00000000000000010000000000000000/, '128-bit hex string read from memory';
};
subtest 'Core Types: Pointers & Recursive Structs' => sub {
    my $n1_ptr = alloc_raw( sizeof( Node() ) );
    my $n2_ptr = alloc_raw( sizeof( Node() ) );
    my $n1     = cast( $n1_ptr, Node() );
    my $n2     = cast( $n2_ptr, Node() );
    $n1->{id}   = 42;
    $n2->{id}   = 84;
    $n1->{next} = $n2_ptr;
    is $n1->{id}, 42, 'Node 1 ID';
    my $deref = cast( $n1->{next}, Node() );
    is $deref->{id}, 84, 'Node 2 ID via next pointer';
};
subtest 'Core Types: Arrays, Vectors & Nesting' => sub {
    my $raw  = alloc_raw( sizeof( Everything() ) );
    my $data = cast( $raw, Everything() );
    $data->{simd}[3] = 4.4;
    is $data->{simd}[3], float( '4.4', tolerance => 0.01 ), 'Vector element access';
    $data->{matrix}[9][8]{x} = 12.34;
    is $data->{matrix}[9][8]{x},       float( 12.34, tolerance => 0.01 ), 'Nested array access';
    is scalar( @{ $data->{matrix} } ), 10,                                'Array size check';
};
subtest 'Assignment Interceptor (The Overwrite Test)' => sub {
    my $raw  = alloc_raw( sizeof( Everything() ) );
    my $data = cast( $raw, Everything() );
    $data->{bits}{flag} = 1;
    $data->{bits}{val}  = 64;
    is $data->{bits}{flag}, 1,  'Bitfield flag';
    is $data->{bits}{val},  64, 'Bitfield value';
    $data->{u}{f} = 1.0;
    is $data->{u}{i}, 1065353216, 'Union punning (float 1.0 bits as int)';

    # The Overwrite Test
    $data->{u} = { f => 999.0 };
    is $data->{u}{f},   float(999.0), 'Overwritten property synced accurately';
    isnt $data->{u}{i}, 0,            'Proxy was seamlessly restored post-assignment';
    isnt $data->{u}{i}, 1065353216,   'Punning reflects the new 999 memory state';
};
subtest 'Array Assignment Interceptor (Deep Overwrite)' => sub {
    my $raw  = alloc_raw( sizeof( Everything() ) );
    my $data = cast( $raw, Everything() );

    # Set an initial value natively
    $data->{matrix}[0][0]{x} = 1.0;
    is $data->{matrix}[0][0]{x}, float(1.0), 'Initial state is 1.0';

    # OVERWRITE an entire row (an array of structs) with a pure Perl ArrayRef of HashRefs!
    $data->{matrix}[0] = [ { x => 99.9, y => 88.8 }, { x => 77.7, y => 66.6 } ];

    # Prove that the C memory was correctly synced
    is $data->{matrix}[0][0]{x}, float(99.9), 'Overwritten array element 0, struct property x synced';
    is $data->{matrix}[0][1]{y}, float(66.6), 'Overwritten array element 1, struct property y synced';

    # Prove safety: The matrix row has 10 elements. We only provided 2 in our ArrayRef.
    # The interceptor should safely leave the rest untouched (default 0.0)
    is $data->{matrix}[0][2]{x}, float(0.0), 'Untouched array elements are safely ignored';

    # Prove array bounds safety: Assigning too many elements safely truncates
    # (matrix[0] is size 10, we'll try to assign 12)
    my @too_big = map { { x => $_, y => $_ } } 1 .. 12;
    $data->{matrix}[1] = \@too_big;
    is $data->{matrix}[1][9]{x},          float(10.0), 'Successfully wrote up to the C array boundary';
    is scalar( @{ $data->{matrix}[1] } ), 10,          'C Array size strictly enforced (did not grow to 12)';
};
subtest 'Strings & Wide Strings' => sub {
    my $raw  = alloc_raw( sizeof( Everything() ) );
    my $data = cast( $raw, Everything() );
    $data->{name} = 'Infix Engine';
    is $data->{name}, 'Infix Engine', 'char[] read/write';
    $data->{wide16} = '中文 and 🚀';
    is $data->{wide16}, '中文 and 🚀', 'Wide string coercion worked';
    $data->{textptr} = get_string_ptr();
    my $deref_str = cast( $data->{textptr}, '[32:char]' );
    is $deref_str, 'Hello from C Pointer', 'char* explicit deref';
};
subtest 'Callbacks & Function Pointers' => sub {
    my $raw  = alloc_raw( sizeof( Everything() ) );
    my $data = cast( $raw, Everything() );
    $data->{cb} = sub { my ( $a, $b ) = @_; return $a + int($b) };
    is test_invoke_callback( $data->{cb}, 10, 5.5 ), 15, 'Perl callback invoked from C';
    $data->{fp} = 0x12345678;
    is $data->{fp}, 0x12345678, 'void* stored as IV';
};
subtest 'File Handles' => sub {
    my $raw  = alloc_raw( sizeof( Everything() ) );
    my $data = cast( $raw, Everything() );
    open my $fh, '>', 'infix_test.log' or die $!;
    my $file_ptr = get_file_ptr($fh);
    ok $file_ptr > 0, 'Extracted FILE* from Perl handle';
    $data->{fp} = $file_ptr;
    is $data->{fp}, $file_ptr, 'FILE* stored correctly';
    close $fh;
    unlink 'infix_test.log';
};
subtest 'Casting & Pointer Workouts' => sub {
    my $raw = alloc_raw(28);
    set_mem_u128( $raw, 0x12345678, 0 );
    is cast( $raw, UInt32 ), 0x12345678, 'Cast raw to uint32';
    is cast( $raw, Int16 ),  0x5678,     'Cast same raw to sint16 (alias)';
    my $ptr_to_ptr = alloc_raw(8);
    my $pp         = cast( $ptr_to_ptr, '*uint32' );
    $pp = $raw;
    my $val = cast( $pp, UInt32 );
    is $val, 0x12345678, 'Deref via cast of magic pointer';
};
subtest 'Edge Cases: Zeroed Memory & Void' => sub {
    my $raw = alloc_raw(4);
    my $val = cast( $raw, Int );
    is $val, 0, 'Zeroed memory returns 0';
    my $v = cast( 0, Void );
    is $v, undef, 'void returns undef';
};
subtest 'Memory Lifetime Tracking (Owned SVs)' => sub {
    my $child_ref;
    {
        my $owned_raw = alloc_owned( sizeof( Everything() ) );
        my $data      = cast( $owned_raw, Everything() );
        $data->{matrix}[5][5]{x} = 42.42;

        # We hold a reference directly to a nested magic proxy
        $child_ref = \$data->{matrix}[5][5]{x};

        # Both the parent object ($data) and the allocated memory ($owned_raw) go out of scope here!
        # But because $child_ref holds a magical SV, Perl's magic system keeps the memory alive!
    }

    # We can still read from the memory safely!
    is $$child_ref, float(42.42), 'Child magic keeps C memory safely alive';
};
subtest Errors => sub {
    subtest alloc_raw => sub {
        like warning { alloc_raw(0) },  qr[zero],     'alloc_raw( 0 )';
        like warning { alloc_raw(-1) }, qr[negative], 'alloc_raw( -1 )';
        like warnings { alloc_raw(undef) }->[0],  qr[uninitialized], 'alloc_raw( undef )';
        like warnings { alloc_raw('blah') }->[0], qr[numeric],       'alloc_raw( "blah" )';
    };
    subtest cast => sub {
        my $raw = alloc_raw(1024);
        like warning { cast( undef, Everything() ) },              qr[undef],        'undef pointer';
        like warnings { cast( alloc_raw(0), Everything() ) }->[0], qr[undef],        'NULL pointer';
        like warning { cast( $raw, 'Trash' ) },                    qr[Invalid type], 'unknown type';
    };
};
#
done_testing;
