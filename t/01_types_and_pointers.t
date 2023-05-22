use strict;
use Test::More 0.98;
use lib '../lib', 'lib';
use Affix;
#
#~ use Data::Dump;
#~ ddx Struct [ i => Str, j => Long ];
#~ ddx Union [ u => Int, x => Double ];
#~ ddx Array [ Int, 10 ];
subtest 'types' => sub {
    isa_ok $_, 'Affix::Type::Base'
        for Void, Char, UChar, WChar, Short, UShort, Int, UInt, Long, ULong, LongLong, ULongLong,
        Size_t, SSize_t, Float, Double, Str, WStr, Pointer [Int], CodeRef [ [] => Str ],
        Struct [ i => Str, j => Long ], Union [ u => Int, x => Double ], ArrayRef [ Int, 10 ];
};
subtest 'coderef' => sub {
    my $ptr = ( CodeRef [ [] => Str ] )->marshal( sub { pass 'coderef called'; return 'Okay' }, );
    isa_ok $ptr, 'Affix::Pointer';
    my $cv = ( CodeRef [ [] => Str ] )->unmarshal($ptr);
    isa_ok $cv, 'CODE';
    is $cv->(), 'Okay', 'return value from coderef';
    subtest 'inside struct' => sub {
        my $type = Struct [ cb => CodeRef [ [Str] => Str ] ];
        my $ptr  = $type->marshal( { cb => sub { pass 'hi'; 'Yes' } } );
        isa_ok $ptr, 'Affix::Pointer';
        my $cv = $type->unmarshal($ptr);
        isa_ok $cv->{cb}, 'CODE';
        is $cv->{cb}->(), 'Yes', 'return value from coderef';
    };
};
subtest array => sub {
    subtest 'ArrayRef [ Int, 3 ]' => sub {
        my $type = ArrayRef [ Int, 3 ];
        my $data = [ 5, 10, 15 ];
        my $ptr  = $type->marshal($data);
        isa_ok $ptr, 'Affix::Pointer';
        is_deeply [ $type->unmarshal($ptr) ], [$data], 'round trip is correct';
    };
    subtest 'ArrayRef [ CodeRef [ [Str] => Str ], 3 ]' => sub {
        my $type = ArrayRef [ CodeRef [ [Str] => Str ], 3 ];
        my $ptr  = $type->marshal(
            [   sub { is shift, 'one',   'proper args passed to 1st'; 'One' },
                sub { is shift, 'two',   'proper args passed to 2nd'; 'Two' },
                sub { is shift, 'three', 'proper args passed to 3rd'; 'Three' }
            ]
        );
        isa_ok $ptr, 'Affix::Pointer';
        my $cv = $type->unmarshal($ptr);
        is scalar @$cv,         3,       '3 coderefs unpacked';
        is $cv->[0]->('one'),   'One',   'proper return value from 1st';
        is $cv->[1]->('two'),   'Two',   'proper return value from 2nd';
        is $cv->[2]->('three'), 'Three', 'proper return value from 3rd';
    };
    subtest 'ArrayRef [ Struct [ alpha => Str, numeric => Int ], 3 ]' => sub {
        my $type = ArrayRef [ Struct [ alpha => Str, numeric => Int ], 3 ];
        my $data = [
            { alpha => 'Smooth',   numeric => 4 },
            { alpha => 'Move',     numeric => 2 },
            { alpha => 'Ferguson', numeric => 0 }
        ];
        my $ptr = $type->marshal($data);
        isa_ok $ptr, 'Affix::Pointer';
        is_deeply [ $type->unmarshal($ptr) ], [$data], 'round trip is correct';
    };
};
subtest union => sub {
    subtest 'Union [ i => Int, f => Float ]' => sub {
        my $type = Union [ i => Int, f => Float ];
        my $data = { f => 3.14 };
        my $ptr  = $type->marshal($data);
        isa_ok $ptr, 'Affix::Pointer';
        is_deeply sprintf( '%1.2f', $type->unmarshal($ptr)->{f} ), 3.14, 'round trip is correct';
        #
        $data = { i => 6 };
        $ptr  = $type->marshal($data);
        isa_ok $ptr, 'Affix::Pointer';
        is_deeply $type->unmarshal($ptr)->{i}, 6, 'round trip is correct';

        #~ Affix::DumpHex( $ptr, 16 );
        #~ use Data::Dump;
        #~ ddx $type->unmarshal($ptr);
    };

    #~ subtest 'Array [ CodeRef [ [Str] => Str ], 3 ]' => sub {
    #~ my $type = Array [ CodeRef [ [Str] => Str ], 3 ];
    #~ my $ptr  = $type->marshal(
    #~ [   sub { is shift, 'one',   'proper args passed to 1st'; 'One' },
    #~ sub { is shift, 'two',   'proper args passed to 2nd'; 'Two' },
    #~ sub { is shift, 'three', 'proper args passed to 3rd'; 'Three' }
    #~ ]
    #~ );
    #~ isa_ok $ptr, 'Affix::Pointer';
    #~ my $cv = $type->unmarshal($ptr);
    #~ is scalar @$cv,         3,       '3 coderefs unpacked';
    #~ is $cv->[0]->('one'),   'One',   'proper return value from 1st';
    #~ is $cv->[1]->('two'),   'Two',   'proper return value from 2nd';
    #~ is $cv->[2]->('three'), 'Three', 'proper return value from 3rd';
    #~ };
    #~ subtest 'Array [ Struct [ alpha => Str, numeric => Int ], 3 ]' => sub {
    #~ my $type = Array [ Struct [ alpha => Str, numeric => Int ], 3 ];
    #~ my $data = [
    #~ { alpha => 'Smooth',   numeric => 4 },
    #~ { alpha => 'Move',     numeric => 2 },
    #~ { alpha => 'Ferguson', numeric => 0 }
    #~ ];
    #~ my $ptr = $type->marshal($data);
    #~ isa_ok $ptr, 'Affix::Pointer';
    #~ is_deeply [ $type->unmarshal($ptr) ], [$data], 'round trip is correct';
    #~ };
};
done_testing;
exit 0
