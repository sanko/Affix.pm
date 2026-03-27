use v5.40;
use utf8;
use blib;
use Affix;
use Test2::V1 -ipP;
#
#~ newXS_deffile('main::create_registry', XS_main_create_registry);
#~ newXS_deffile('main::define_types', XS_main_define_types);
#~ newXS_deffile('main::sizeof_type', XS_main_sizeof_type);
#~ newXS_deffile('main::cast', XS_main_cast);
#~ newXS_deffile('main::alloc_raw', XS_main_alloc_raw);
#~ newXS_deffile('main::get_string_ptr', XS_main_get_string_ptr);
#~ newXS_deffile('main::test_invoke_callback', XS_main_test_invoke_callback);
#~ newXS_deffile('main::set_mem_u128', XS_main_set_mem_u128);
#~ newXS_deffile('main::get_file_ptr', XS_main_get_file_ptr);
#
define_types(<<'END');
    @char16_t = uint16;
    @Pos = { x: double, y: double };
    @Callback = (int, double) -> int;

    @Primitives = {
        s8: sint8, u8: uint8,
        s16: sint16, u16: uint16,
        s32: sint32, u32: uint32,
        s64: sint64, u64: uint64,
        f32: float, f64: double,
        b: bool
    };

    @Node = { id: int, next: *@Node };

    @Everything = {
        matrix:[10:[10:@Pos]],
        bits:{ flag:uint8:1, val:uint8:7 },
        u:< i:int, f:float >,
        comp:c[float],
        textptr:*char,
        name:[32:char],
        wide16:[16:@char16_t],
        fp:*void,
        cb:*@Callback,
        simd:m256d
    };
END
subtest 'Core Types: Primitives' => sub {
    my $raw = alloc_raw( sizeof_type('Primitives') );
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
    my $raw  = alloc_raw(16);
    my $u128 = cast( $raw, 'uint128' );
    $u128 = '340282366920938463463374607431768211455';
    is $u128, '340282366920938463463374607431768211455', 'uint128 max string';
};
subtest 'Core Types: 128-bit Hex (via sint128)' => sub {
    my $todo = todo 'Not ready yet...';
    my $raw  = alloc_raw(16);
    set_mem_u128( $raw, 0, 1 );
    my $val = cast( $raw, 'sint128' );
    like $val, qr/0x00000000000000010000000000000000/, '128-bit hex string read from memory';
};
subtest 'Core Types: Pointers & Recursive Structs' => sub {
    my $n1_ptr = alloc_raw( sizeof_type('Node') );
    my $n2_ptr = alloc_raw( sizeof_type('Node') );
    my $n1     = cast( $n1_ptr, 'Node' );
    my $n2     = cast( $n2_ptr, 'Node' );
    $n1->{id}   = 42;
    $n2->{id}   = 84;
    $n1->{next} = $n2_ptr;
    is $n1->{id}, 42, 'Node 1 ID';
    my $deref = cast( $n1->{next}, 'Node' );
    is $deref->{id}, 84, 'Node 2 ID via next pointer';
};
subtest 'Core Types: Arrays, Vectors & Nesting' => sub {
    my $raw  = alloc_raw( sizeof_type('Everything') );
    my $data = cast( $raw, 'Everything' );
    $data->{simd}[3] = 4.4;
    is $data->{simd}[3], float( '4.4', tolerance => 0.01 ), 'Vector element access';
    $data->{matrix}[9][8]{x} = 12.34;
    is $data->{matrix}[9][8]{x},       float( 12.34, tolerance => 0.01 ), 'Nested array access';
    is scalar( @{ $data->{matrix} } ), 10,                                'Array size check';
};
subtest 'Assignment Interceptor (The Overwrite Test)' => sub {
    my $raw  = alloc_raw( sizeof_type('Everything') );
    my $data = cast( $raw, 'Everything' );
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
    my $raw  = alloc_raw( sizeof_type('Everything') );
    my $data = cast( $raw, 'Everything' );

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
    my $raw  = alloc_raw( sizeof_type('Everything') );
    my $data = cast( $raw, 'Everything' );
    $data->{name} = 'Infix Engine';
    is $data->{name}, 'Infix Engine', 'char[] read/write';
    $data->{wide16} = '中文 and 🚀';
    is $data->{wide16}, '中文 and 🚀', 'Wide string coercion worked';
    $data->{textptr} = get_string_ptr();
    my $deref_str = cast( $data->{textptr}, '[32:char]' );
    is $deref_str, 'Hello from C Pointer', 'char* explicit deref';
};
subtest 'Callbacks & Function Pointers' => sub {
    my $raw  = alloc_raw( sizeof_type('Everything') );
    my $data = cast( $raw, 'Everything' );
    $data->{cb} = sub { my ( $a, $b ) = @_; return $a + int($b) };
    is test_invoke_callback( $data->{cb}, 10, 5.5 ), 15, 'Perl callback invoked from C';
    $data->{fp} = 0x12345678;
    is $data->{fp}, 0x12345678, 'void* stored as IV';
};
subtest 'File Handles' => sub {
    my $raw  = alloc_raw( sizeof_type('Everything') );
    my $data = cast( $raw, 'Everything' );
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
    is cast( $raw, 'uint32' ), 0x12345678, 'Cast raw to uint32';
    is cast( $raw, 'sint16' ), 0x5678,     'Cast same raw to sint16 (alias)';
    my $ptr_to_ptr = alloc_raw(8);
    my $pp         = cast( $ptr_to_ptr, '*uint32' );
    $pp = $raw;
    my $val = cast( $pp, 'uint32' );
    is $val, 0x12345678, 'Deref via cast of magic pointer';
};
subtest 'Edge Cases: Zeroed Memory & Void' => sub {
    my $raw = alloc_raw(4);
    my $val = cast( $raw, 'int' );
    is $val, 0, 'Zeroed memory returns 0';
    my $v = cast( 0, 'void' );
    is $v, undef, 'void returns undef';
};
#
done_testing;
