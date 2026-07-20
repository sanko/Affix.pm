#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix qw[:all];
use POSIX qw[EXIT_SUCCESS EXIT_FAILURE];
use Test2::V0;
$|++;
my $max_iter = $ENV{FUZZ_MAX_ITER} // 1000;
my $timeout  = $ENV{FUZZ_TIMEOUT}  // 5;
my $verbose  = $ENV{FUZZ_VERBOSE}  // 0;
my @TYPES    = qw[
    Void Bool Char UChar SChar
    Short UShort Int UInt Long ULong LongLong ULongLong
    Float Double LongDouble
    SInt8 UInt8 SInt16 UInt16 SInt32 UInt32 SInt64 UInt64
    Size_t SSize_t
    String WString
];
my @POINTER_TYPES   = map {"Pointer [ $_ ]"} @TYPES;
my @COMPOSITE_TYPES = (
    'Struct [ x => Int ]',
    'Struct [ x => Int, y => Double ]',
    'Struct [ a => Int, b => Float, c => Char ]',
    'Union [ a => Int, b => Float ]',
    'Array [ Int, 4 ]',
    'Array [ Double, 8 ]',
    'Enum [ [A => 0], [B => 1], [C => 2] ]'
);
sub pick (@list) { $list[ int( rand(@list) ) ] }

sub gen_type_def_string {
    my $kind = pick(qw[primitive pointer struct union array enum typedef_ref]);
    if ( $kind eq 'primitive' ) {
        return pick(@TYPES);
    }
    elsif ( $kind eq 'pointer' ) {
        return pick(@POINTER_TYPES);
    }
    elsif ( $kind eq 'struct' ) {
        return pick(@COMPOSITE_TYPES);
    }
    elsif ( $kind eq 'union' ) {
        return "Union [ a => Int, b => " . pick(@TYPES) . " ]";
    }
    elsif ( $kind eq 'array' ) {
        return "Array [ " . pick(@TYPES) . ", " . ( 1 + int( rand(16) ) ) . " ]";
    }
    elsif ( $kind eq 'enum' ) {
        my $n     = 1 + int( rand(5) );
        my @elems = map { sprintf( "[ E%d => %d ]", $_, $_ * 10 ) } 0 .. $n - 1;
        return "Enum [ " . join( ', ', @elems ) . " ]";
    }
    elsif ( $kind eq 'typedef_ref' ) {
        return pick(@TYPES) . "()";
    }
    return 'Int';
}

# Main
printf STDERR "Fuzzing: Affix::_typedef() -- C parser direct\n";
printf STDERR "  Iterations: %d, Timeout: %ds\n", $max_iter, $timeout;
printf STDERR "%s\n", "=" x 60;
my $type_counter = 0;
for my $i ( 1 .. $max_iter ) {
    note("iter $i/$max_iter") if $i % 20 == 0 || $verbose;

    # Generate a typedef string
    my $type_name = "FuzzType_${type_counter}";
    $type_counter++;
    my $type_body   = gen_type_def_string();
    my $typedef_str = "$type_name = $type_body";
    if ($verbose) {
        note("typedef: [$typedef_str]");
    }

    # Test: typedef should not crash
    my $ok = eval {
        local $SIG{ALRM} = sub { die "typedef timeout\n" };
        alarm $timeout;
        my $result = Affix::typedef( $type_name, $type_body );
        alarm 0;
        1;
    };
    ok $ok, "typedef $type_name";
    note("typedef crashed: $typedef_str: $@") if !$ok && $verbose;
}
done_testing;
