use v5.40;
use utf8;
use blib;
use Affix qw[:types sizeof offsetof];
use Test2::V1 -ipP;
$|++;
#
typedef char16_t => UInt16;
typedef char32_t => UInt32;
typedef Pos      => Struct [ x => Double, y => Double ];
typedef SmallCB  => Callback [ [ Int, Double ] => Int ];
typedef LargeCB  => Callback [ [Int128]        => UInt128 ];
typedef Primitives => Struct [
    s8   => Int8,
    u8   => UInt8,
    s16  => Int16,
    u16  => UInt16,
    s32  => Int32,
    u32  => UInt32,
    s64  => Int64,
    u64  => UInt64,
    f16  => Float16,
    f32  => Float,
    f64  => Double,
    b    => Bool,
    i128 => Int128,
    u128 => UInt128
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
    cb      => SmallCB(),
    big_cb  => LargeCB(),
    simd    => M256d
];
typedef BigData     => Struct [ val  => UInt128, id => Int ];
typedef GiantStruct => Struct [ id   => Int,     big_val => UInt128, big_arr => Array [ Int128, 3 ] ];
typedef UTF16Node   => Struct [ text => Array [ char16_t(), 10 ] ];
typedef UTF32Node   => Struct [ text => Array [ char32_t(), 10 ] ];
subtest 'Core Types: Primitives' => sub {
    my $p = cast( alloc_raw( sizeof( Primitives() ) ), "Primitives" );
    $p->{s8} = -128;
    is( $p->{s8}, -128, 'sint8 min' );
    $p->{s8} = 127;
    is( $p->{s8}, 127, 'sint8 max' );
    $p->{u8} = 255;
    is( $p->{u8}, 255, 'uint8 max' );
    $p->{s16} = -32768;
    is( $p->{s16}, -32768, 'sint16 min' );
    $p->{s16} = 32767;
    is( $p->{s16}, 32767, 'sint16 max' );
    $p->{u16} = 65535;
    is( $p->{u16}, 65535, 'uint16 max' );
    $p->{s32} = -2147483648;
    is( $p->{s32}, -2147483648, 'sint32 min' );
    $p->{s32} = 2147483647;
    is( $p->{s32}, 2147483647, 'sint32 max' );
    $p->{u32} = 4294967295;
    is( $p->{u32}, 4294967295, 'uint32 max' );
    $p->{s64} = -9223372036854775808;
    is( $p->{s64}, -9223372036854775808, 'sint64 min' );
    $p->{s64} = 9223372036854775807;
    is( $p->{s64}, 9223372036854775807, 'sint64 max' );
    $p->{u64} = 18446744073709551615;
    is( $p->{u64}, 18446744073709551615, 'uint64 max' );
    $p->{f16} = 1.5;
    is( $p->{f16}, 1.5, 'float16' );
    $p->{f16} = 0.5;
    is( $p->{f16}, 0.5, 'float16 (0.5)' );
    $p->{f16} = -2.0;
    is( $p->{f16}, -2.0, 'float16 (-2.0)' );
    $p->{f32} = 1.25;
    is( $p->{f32}, 1.25, 'float32' );
    $p->{f64} = 1.23456789;
    is( $p->{f64}, float(1.23456789), 'float64' );
    $p->{b} = 1;
    is( $p->{b}, 1, 'bool true' );
    $p->{b} = 0;
    is( $p->{b}, 0, 'bool false' );
    $p->{i128} = '170141183460469231731687303715884105727';
    is( $p->{i128}, '170141183460469231731687303715884105727', 'int128 max' );
    $p->{i128} = '-170141183460469231731687303715884105728';
    is( $p->{i128}, '-170141183460469231731687303715884105728', 'int128 min' );
    $p->{u128} = '340282366920938463463374607431768211455';
    is( $p->{u128}, '340282366920938463463374607431768211455', 'unt128 max' );
};
subtest '128-bit Integers (Formatting & Edge Cases)' => sub {
    my $raw = alloc_raw(16);

    # Unsigned uses Decimal
    my $u128 = cast( $raw, "uint128" );
    $u128 = "340282366920938463463374607431768211455";
    is( $u128, "340282366920938463463374607431768211455", 'uint128 reads back as decimal max string' );

    # Signed uses Decimal Strings now
    set_mem_u128( $raw, 0, 1 );
    my $s128 = cast( $raw, "sint128" );
    is( "$s128", "18446744073709551616", "sint128 formats natively as decimal string" );

    # Negative string parsing edge case
    $s128 = "-1";
    is( "$s128", "-1", "Negative strings are parsed and stored correctly into 128-bit memory" );
};
subtest '128-bit Integers in Structs and Arrays' => sub {
    my $raw  = alloc_raw( sizeof( GiantStruct() ) );
    my $data = cast( $raw, "GiantStruct" );
    $data->{big_val} = "340282366920938463463374607431768211455";
    is( $data->{big_val}, "340282366920938463463374607431768211455", "128-bit decimal string works flawlessly inside a struct field" );
    my $offset     = offsetof_member( 'GiantStruct', 'big_arr' );
    my $array_base = $raw + $offset;
    set_mem_u128( $array_base,      1,  0 );
    set_mem_u128( $array_base + 16, ~0, ~0 );    # -1
    is( $data->{big_arr}[0], "1",  "128-bit decimal string read perfectly from array index 0" );
    is( $data->{big_arr}[1], "-1", "128-bit decimal string read perfectly from array index 1" );
};
subtest 'Automatic 128-bit Boundary Native Marshalling' => sub {
    my $struct = { val => '170141183460469231731687303715884105726', id => 100 };
    verify_marshalling_128($struct);
    is( $struct->{id},  777,                                       "Struct ID updated natively by C" );
    is( $struct->{val}, "170141183460469231731687303715884105727", "128-bit value successfully incremented and marshalled back" );
};
subtest 'Aggregates: Arrays, Vectors & Nesting' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );
    $data->{simd}[3] = 4.4;
    is( sprintf( "%.1f", $data->{simd}[3] ), "4.4", "Vector element access" );
    $data->{matrix}[9][8]{x} = 12.34;
    is( $data->{matrix}[9][8]{x},       float(12.34), "Nested array access" );
    is( scalar( @{ $data->{matrix} } ), 10,           "Array size check" );
};
subtest 'Assignment Interceptor (The Overwrite Test)' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );
    $data->{u}{f} = 1.0;
    is( $data->{u}{i}, 1065353216, "Union punning (float 1.0 bits as int)" );
    $data->{u} = { f => 999.0 };
    is( $data->{u}{f}, float(999.0), "Overwritten union property synced accurately" );
    isnt( $data->{u}{i}, 1065353216, "Punning reflects the new 999.0 memory state" );
};
subtest 'Bitfields & Overflow Masking Edge Case' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );
    $data->{bits}{flag} = 1;
    $data->{bits}{val}  = 64;
    is( $data->{bits}{flag}, 1,  "Bitfield flag" );
    is( $data->{bits}{val},  64, "Bitfield value" );

    # EDGE CASE: Overflow a bitfield (val is 7 bits max)
    $data->{bits}{val} = 255;
    is( $data->{bits}{val}, 127, "Bitfield safely masks overflow bits (255 becomes 127 in a 7-bit field)" );
};
subtest 'Strings & Wide Strings (Truncation Edge Cases)' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );
    $data->{name} = "Infix Engine";
    is( $data->{name}, "Infix Engine", "char[] read/write" );
    $data->{wide16} = "中文 and 🚀";
    is( $data->{wide16}, "中文 and 🚀", "Wide string coercion worked" );

    # EDGE CASE: String Truncation
    my $long_string = "A" x 50;
    $data->{name} = $long_string;
    is( length( $data->{name} ), 31, "String safely truncated to fit 32-char buffer limit (31 + null byte)" );
    my $deref_str = cast( get_string_ptr(), "[32:char]" );
    is( $deref_str, "Hello from C Pointer", "char* explicit deref via mapped pointer" );
};
subtest 'Pointers & Recursive Structs' => sub {
    my $n1_ptr = alloc_raw( sizeof( Node() ) );
    my $n2_ptr = alloc_raw( sizeof( Node() ) );
    my $n1     = cast( $n1_ptr, "Node" );
    my $n2     = cast( $n2_ptr, "Node" );
    $n1->{id}   = 42;
    $n2->{id}   = 84;
    $n1->{next} = $n2_ptr;
    is( $n1->{id},                         42, "Node 1 ID" );
    is( cast( $n1->{next}, "Node" )->{id}, 84, "Node 2 ID via next pointer" );
};
subtest 'Casting & Pointer Workouts' => sub {
    my $raw = alloc_raw(28);
    set_mem_u128( $raw, 0x12345678, 0 );
    is( cast( $raw, "uint32" ), 0x12345678, "Cast raw memory directly to uint32" );
    is( cast( $raw, "sint16" ), 0x5678,     "Cast same raw memory to sint16 (aliasing)" );
    my $ptr_to_ptr = alloc_raw(8);
    my $pp         = cast( $ptr_to_ptr, "*uint32" );
    $pp = $raw;
    is( cast( $pp, "uint32" ), 0x12345678, "Deref via cast of magic pointer" );
};
subtest 'Function Callbacks & File Handles' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );
    $data->{cb} = sub { my ( $a, $b ) = @_; return $a + int($b) };
    is( test_invoke_callback( $data->{cb}, 10, 5.5 ), 15, "Perl callback invoked natively from C" );
    open my $fh, '>', 'infix_test.log' or die $!;
    my $file_ptr = get_file_ptr($fh);
    ok( $file_ptr > 0, "Extracted FILE* from Perl handle" );
    $data->{fp} = $file_ptr;
    is( $data->{fp}, $file_ptr, "FILE* stored correctly" );
    close $fh;
    unlink 'infix_test.log';
};
subtest 'Native Auto-Marshalling' => sub {
    my $struct = { x => 50, y => 100 };
    verify_and_mutate_struct_arg($struct);
    is( $struct->{x}, float(51),  "Struct property x incremented by native C function" );
    is( $struct->{y}, float(200), "Struct property y doubled by native C function" );
};
subtest 'Memory Lifetime Tracking (Owned SVs & Child Anchors)' => sub {
    my $child_ref;
    {
        my $owned = alloc_owned( sizeof( Everything() ) );
        my $data  = cast( $owned, "Everything" );
        $data->{matrix}[5][5]{x} = 42.42;
        $child_ref = \$data->{matrix}[5][5]{x};
    }

    # Once $data goes out of scope, Infix::Memory should NOT free the C memory yet!
    is( $$child_ref, float(42.42), "Child magic explicitly keeps parent C memory safely alive" );
};
subtest 'Memory Lifetime & Custom C++ Destructors' => sub {
    my $child;
    {
        # Wrap our mock C++ pointer with a custom destructor callback
        my $cxx_ptr   = mock_cxx_new(42);
        my $dtor_addr = get_mock_cxx_dtor();
        my $owned_cxx = wrap_owned( $cxx_ptr, $dtor_addr );
        my $data      = cast( $owned_cxx, '{ val: int }' );
        is( $data->{val}, 42, "C++ Object bound successfully" );
        $child = \$data->{val};    # Keep the memory alive by referencing an internal field
    }
    is( get_mock_cxx_dtor_calls(), 0, "Custom destructor delayed by child lifeline" );
    undef $child;                  # Drop the final reference, invoking DESTROY
    is( get_mock_cxx_dtor_calls(), 1, "Custom C++ destructor invoked successfully by Perl GC" );
};
subtest 'Explicit NULL Pointer Assignment' => sub {
    my $node_ptr = alloc_raw( sizeof( Node() ) );
    my $node     = cast( $node_ptr, "Node" );

    # Assign a valid pointer
    $node->{next} = alloc_raw( sizeof( Node() ) );
    ok( $node->{next} > 0, "Pointer assigned successfully" );

    # Nullify it via Perl undef
    $node->{next} = undef;
    is( $node->{next}, undef, "Assigning undef correctly sets native C pointer to NULL" );

    # Casting NULL directly
    my $null_ptr_cast = cast( 0, "*int" );
    is( $null_ptr_cast, undef, "Dereferencing a generic null pointer via cast safely yields undef" );
};
subtest 'Partial Struct Updates & Extraneous Keys' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );

    # Initialize a nested struct
    $data->{matrix}[0][0]{x} = 10.0;
    $data->{matrix}[0][0]{y} = 20.0;

    # Update ONLY x, passing an extraneous key 'z_fake'
    # This assigns to a magic property, triggering lazy_agg_set in C!
    $data->{matrix}[0][0] = { x => 99.0, z_fake => 100.0 };
    is( $data->{matrix}[0][0]{x}, float(99.0), "Present keys are natively updated" );
    is( $data->{matrix}[0][0]{y}, float(20.0), "Omitted keys are left completely untouched in C memory" );

    # If it didn't crash or create a new key, the extraneous 'z_fake' was safely ignored!
    my $dumped_keys = join( ",", sort keys %{ $data->{matrix}[0][0] } );
    is( $dumped_keys, "x,y", "Extraneous keys are rejected by the C struct mapper" );
};
subtest 'Short Array Assignments' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );

    # EDGE CASE: Array Bounds on Assignment (Should safely truncate)
    $data->{matrix}[0] = [ ( { x => 1, y => 1 } ) x 20 ];    # Passing 20 to a 10-length array
    is( $data->{matrix}[0][9]{x}, float(1.0), "Safely truncated over-sized array assignment at max index" );
    is( $data->{matrix}[0][10],   undef,      "Index 10 remains unassigned/out-of-bounds" );
    $data->{matrix}[1][0]{x} = 5.0;
    $data->{matrix}[1][1]{x} = 10.0;

    # Overwrite row 1 with a shorter array
    $data->{matrix}[1] = [ { x => 99.0 } ];
    is( $data->{matrix}[1][0]{x}, float(99.0), "Index 0 successfully updated" );
    is( $data->{matrix}[1][1]{x}, float(10.0), "Index 1 untouched by shorter array assignment" );
};
subtest 'Empty Strings (char and wide)' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );
    $data->{name}   = "Hello";
    $data->{wide16} = "World";

    # Overwrite with empty strings
    $data->{name}   = "";
    $data->{wide16} = "";
    is( $data->{name},   "", "ASCII string safely emptied (null terminated at index 0)" );
    is( $data->{wide16}, "", "Wide string safely emptied (null terminated at index 0)" );
};
subtest 'Callbacks with 128-bit Strings' => sub {
    my $data = cast( alloc_raw( sizeof( Everything() ) ), "Everything" );

    # Define a Perl callback that takes a signed 128-bit string, and returns an unsigned 128-bit string
    $data->{big_cb} = sub {
        my ($val) = @_;
        is( $val, "-500", "Perl received negative 128-bit signed argument correctly natively from C" );
        return "1000000000000000000000";    # Massive unsigned int
    };
    my $res = test_invoke_callback_128( $data->{big_cb}, "-500" );
    is( $res, "1000000000000000000000", "Returned 128-bit unsigned integer cleanly traversed the C-to-Perl-to-C trampoline" );
};
subtest 'Expanded FFI: 128-bit Hex & Robustness' => sub {
    my $data = cast( alloc_raw( sizeof( BigData() ) ), "BigData" );

    # 0xABCDEF0123456789ABCDEF0123456789 is 228367255721259569362527394270995113865
    $data->{val} = "0xABCDEF0123456789ABCDEF0123456789";
    is( $data->{val}, "228367255721259569362527394270995113865", "128-bit hex string correctly parsed into memory and read back as decimal" );
};
subtest 'Wide String Robustness (Japanese, Emoji, Truncation)' => sub {
    my $u16 = cast( alloc_raw(20), "\@UTF16Node" );
    my $u32 = cast( alloc_raw(40), "\@UTF32Node" );

    # Japanese
    my $jp = "日本語";
    $u16->{text} = $jp;
    $u32->{text} = $jp;
    is( $u16->{text}, $jp, "UTF-16 Japanese" );
    is( $u32->{text}, $jp, "UTF-32 Japanese" );

    # Emoji (Surrogate Pairs)
    my $emoji = "🚀🔥✨";
    $u16->{text} = $emoji;
    $u32->{text} = $emoji;
    is( $u16->{text}, $emoji, "UTF-16 Emoji (Surrogate Pairs)" );
    is( $u32->{text}, $emoji, "UTF-32 Emoji" );

    # Truncation Edge Case (UTF-16)
    #[10:@char16_t] -> 10 units. 1 for null = 9 usable.
    # "🚀" is 2 units. 4 rockets = 8 units. 5th rocket would take it to 10.
    $u16->{text} = "🚀" x 6;
    is( $u16->{text}, "🚀" x 4, "UTF-16 Truncation (Safety against splitting surrogates)" );

    # Embedded Nulls
    $u16->{text} = "A\0B";
    is( $u16->{text}, "A", "UTF-16 stops at embedded null" );
};
subtest '128-bit Robustness (Whitespace & Malformed)' => sub {
    my $data = cast( alloc_raw( sizeof( BigData() ) ), "BigData" );
    $data->{val} = "  12345  ";
    is $data->{val}, "12345", "128-bit decimal with whitespace";
    $data->{val} = "  0xABC  ";
    is $data->{val}, "2748", "128-bit hex with whitespace";
    $data->{val} = "123XYZ";
    is $data->{val}, "123", "128-bit stops at malformed character";
};
subtest 'Memory Mutation via Magic VTables' => sub {
    subtest 'Mutate C memory via Array Cast (int[1])' => sub {
        my $size = sizeof(Int);
        my $mem  = alloc_owned($size);

        # Cast to array and verify it initializes to 0 (since safecalloc is used)
        my $int_array = cast( $mem, Array [ Int, 1 ] );
        is $int_array->[0], 0, 'Memory is safely zero-initialized';

        # Mutate the memory
        $int_array->[0] = 42;
        is $int_array->[0], 42, 'Value updated in the mapped Perl array';

        # Cast the same raw memory again to ensure the C memory actually changed
        my $verify_array = cast( $mem, Array [ Int, 1 ] );
        is $verify_array->[0], 42, 'Underlying C memory was successfully mutated';
    };
    subtest 'Mutate C memory via Scalar Aliasing (for-loop)' => sub {
        my $size = sizeof(Int);
        my $mem  = alloc_owned($size);

        # Use the $_ aliasing trick to write directly to the magical scalar
        for ( cast( $mem, Int ) ) {
            is $_, 0, 'Memory is safely zero-initialized';
            $_ = 99;    # Mutate
            is $_, 99, 'Value updated via scalar alias';
        }

        # Cast again and alias to read the underlying memory
        for ( cast( $mem, Int ) ) {
            is $_, 99, 'Underlying C memory was successfully mutated via scalar alias';
        }
    };
    subtest 'Standard assignment strips magic (Expected Failure)' => sub {
        my $size = sizeof(Int);
        my $mem  = alloc_owned($size);

        # This copies the value (0) and strips the magic
        my $copied_val = cast( $mem, Int );

        # This only changes the Perl scalar, NOT the C memory
        $copied_val = 777;
        is $copied_val, 777, 'Perl scalar updated...';

        # Verify the C memory remains untouched (should still be 0)
        for ( cast( $mem, Int ) ) {
            is $_, 0, '...but underlying C memory remains unchanged because magic was stripped during copy';
        }
    };
};
subtest 'Linux Regression: Int[1] vs WChar[x]' => sub {

    # On Linux, sizeof(Int) == sizeof(WChar) == 4.
    # This test ensures they are distinguished by NAME, not SIZE.
    ok my $mem = alloc_owned( sizeof_type("int") ), 'malloc';

    # This should return an ARRAY REF (AV*), not a string ("")
    ok my $int_arr = cast( $mem, Array [ Int, 1 ] ), 'cast(Array[Int, 1])';
    is ref($int_arr), 'ARRAY', 'int[1] is treated as an array reference (AV*)';

    # This should return a STRING (PV)
    my $wchar_arr = cast( alloc_owned( sizeof_type(WChar) * 4 ), Array [ WChar, 4 ] );
    is ref($wchar_arr), '', 'wchar_t[4] is treated as a string (PV)';
};
done_testing;
