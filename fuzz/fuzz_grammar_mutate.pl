#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix qw[:all];
use POSIX qw[EXIT_SUCCESS EXIT_FAILURE];
use Test2::V0 defined $ENV{FUZZ_SRAND} ? $ENV{FUZZ_SRAND} == 0 ? ( -no_srand => 1 ) : ( -srand => $ENV{FUZZ_SRAND} ) : ();
$|++;
my $max_iter = $ENV{FUZZ_MAX_ITER} // 1000;
my $timeout  = $ENV{FUZZ_TIMEOUT}  // 5;
my $verbose  = $ENV{FUZZ_VERBOSE}  // 0;

# Grammar for C type signatures (infix notation as used by Affix)
# Grammar:
#   type     = primitive | '*' type | '{' fields '}' | '<' fields '>' | '[' count ':' type ']'
#   primitive = 'void' | 'bool' | 'char' | 'short' | 'int' | 'long' | 'float' | 'double'
#              | 'uchar' | 'ushort' | 'uint' | 'ulong' | 'longlong' | 'ulonglong'
#              | 'sint8' | 'uint8' | ... | 'size_t' | 'ssize_t'
#   fields   = field (',' field)*
#   field    = name ':' type
my @PRIM_SIGS = qw[
    void bool char uchar short ushort int uint long ulong
    longlong ulonglong float double longdouble
    sint8 uint8 sint16 uint16 sint32 uint32 sint64 uint64
    size_t ssize_t
];
sub pick (@list) { $list[ int( rand(@list) ) ] }

sub gen_sig_type {
    my $depth = shift // 0;
    return 'void' if $depth > 3;
    my $kind = pick(qw[primitive pointer struct union array]);
    if ( $kind eq 'primitive' || $depth > 2 ) {
        return pick(@PRIM_SIGS);
    }
    elsif ( $kind eq 'pointer' ) {
        return '*' . gen_sig_type( $depth + 1 );
    }
    elsif ( $kind eq 'struct' ) {
        my $n = 1 + int( rand(3) );
        my @fields;
        for my $i ( 0 .. $n - 1 ) {
            push @fields, sprintf( "f%d:%s", $i, gen_sig_type( $depth + 1 ) );
        }
        return '{' . join( ',', @fields ) . '}';
    }
    elsif ( $kind eq 'union' ) {
        my $n = 1 + int( rand(3) );
        my @fields;
        for my $i ( 0 .. $n - 1 ) {
            push @fields, sprintf( "f%d:%s", $i, gen_sig_type( $depth + 1 ) );
        }
        return '<' . join( ',', @fields ) . '>';
    }
    elsif ( $kind eq 'array' ) {
        return '[' . ( 1 + int( rand(8) ) ) . ':' . gen_sig_type( $depth + 1 ) . ']';
    }
    return 'int';
}

# Mutation operators
sub mutate_sig {
    my ($sig) = @_;
    my $kind = pick(qw[insert delete replace swap duplicate corrupt]);
    if ( $kind eq 'insert' ) {
        my $pos = int( rand( length($sig) + 1 ) );
        my $ch  = pick( '*', '{', '}', '<', '>', '[', ']', ',', ':', 'i', 'v', 'd' );
        substr( $sig, $pos, 0 ) = $ch;
    }
    elsif ( $kind eq 'delete' ) {
        my $pos = int( rand( length($sig) ) );
        substr( $sig, $pos, 1 ) = '';
    }
    elsif ( $kind eq 'replace' ) {
        my $pos = int( rand( length($sig) ) );
        my $ch  = pick( '*', '{', '}', '<', '>', '[', ']', 'x', 'q', '0', '9' );
        substr( $sig, $pos, 1 ) = $ch;
    }
    elsif ( $kind eq 'swap' ) {
        return $sig if length($sig) < 2;
        my $a     = int( rand( length($sig) - 1 ) );
        my $b     = $a + 1 + int( rand( length($sig) - $a - 1 ) );
        my @chars = split //, $sig;
        ( $chars[$a], $chars[$b] ) = ( $chars[$b], $chars[$a] );
        $sig = join '', @chars;
    }
    elsif ( $kind eq 'duplicate' ) {
        my $pos = int( rand( length($sig) ) );
        my $ch  = substr( $sig, $pos, 1 );
        substr( $sig, $pos, 0 ) = $ch x ( 1 + int( rand(3) ) );
    }
    elsif ( $kind eq 'corrupt' ) {

        # Insert non-ASCII bytes
        my $pos = int( rand( length($sig) + 1 ) );
        substr( $sig, $pos, 0 ) = chr( 0x80 + int( rand(128) ) );
    }
    return $sig;
}

# Main
printf STDERR "Fuzzing: Grammar-aware C signature mutations\n";
printf STDERR "  Iterations: %d, Timeout: %ds\n", $max_iter, $timeout;
printf STDERR "%s\n", "=" x 60;
my @history;
for my $i ( 1 .. $max_iter ) {
    note("iter $i/$max_iter") if $i % 20 == 0 || $verbose;
    my $sig;
    if ( !@history || rand(1) < 0.3 ) {

        # Generate fresh
        $sig = gen_sig_type();
    }
    else {
        # Mutate from history
        $sig = mutate_sig( pick(@history) );
    }
    push @history, $sig;
    @history = @history[ -50 .. -1 ] if @history > 50;    # keep last 50
    if ($verbose) {
        note("Sig: [$sig]");
    }

    # Test: sizeof() should not crash
    my $result;
    my $crashed = 0;
    eval {
        local $SIG{ALRM} = sub { die "sizeof timeout\n" };
        alarm $timeout;
        $result = Affix::sizeof($sig);
        alarm 0;
    };
    $crashed = 1 if $@;
    if ($crashed) {
        if ( $sig =~ /^[\*\{\[\<]/ || grep { $sig eq $_ } @PRIM_SIGS ) {
            fail("sizeof crashed on valid: $sig");
            note("$@") if $verbose;
        }
        else {
            pass("expected crash on garbage");
        }
    }
    else {
        pass("sizeof survived: $sig");
    }
}
done_testing;
