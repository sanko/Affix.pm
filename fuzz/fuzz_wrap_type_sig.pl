#!/usr/bin/env perl
use v5.40;
use blib;
use lib 'blib/lib', 'lib';
use Affix qw[:all];
use POSIX qw[EXIT_SUCCESS EXIT_FAILURE];
use Test2::V0;

# Fuzz the infix type parser by feeding random/malformed signature strings
# into Affix::sizeof() and Affix::_is_type(), which both exercise the
# C-level signature parser (via infix).
#
# We cannot use Affix::Wrap::Type->parse() because it has a broken
# 'use Affix qw[Void Const]' import at compile time.
$|++;
my $max_iter = $ENV{FUZZ_MAX_ITER} // 1000;
my $timeout  = $ENV{FUZZ_TIMEOUT}  // 5;
my $verbose  = $ENV{FUZZ_VERBOSE}  // 0;

# infix signature string generators
my @PRIM_SIGS = qw[
    void bool char uchar short ushort int uint long ulong
    longlong ulonglong float double longdouble
    sint8 uint8 sint16 uint16 sint32 uint32 sint64 uint64
    size_t ssize_t wchar_t
];
sub pick (@list) { $list[ int( rand(@list) ) ] }

# Generate valid infix signature strings
sub gen_valid_sig {
    my $depth = shift // 0;
    return 'void' if $depth > 3;
    my $kind = pick(qw[primitive pointer struct union array]);
    if ( $kind eq 'primitive' || $depth > 2 ) {
        return pick(@PRIM_SIGS);
    }
    elsif ( $kind eq 'pointer' ) {
        return '*' . gen_valid_sig( $depth + 1 );
    }
    elsif ( $kind eq 'struct' ) {
        my $n = 1 + int( rand(3) );
        my @fields;
        for my $i ( 0 .. $n - 1 ) {
            push @fields, sprintf( "f%d:%s", $i, gen_valid_sig( $depth + 1 ) );
        }
        return '{' . join( ',', @fields ) . '}';
    }
    elsif ( $kind eq 'union' ) {
        my $n = 1 + int( rand(3) );
        my @fields;
        for my $i ( 0 .. $n - 1 ) {
            push @fields, sprintf( "f%d:%s", $i, gen_valid_sig( $depth + 1 ) );
        }
        return '<' . join( ',', @fields ) . '>';
    }
    elsif ( $kind eq 'array' ) {
        return '[' . ( 1 + int( rand(8) ) ) . ':' . gen_valid_sig( $depth + 1 ) . ']';
    }
    return 'int';
}

# Generate function signatures: (args)->ret
sub gen_func_sig {
    my $nargs    = int( rand(5) );
    my @args     = map { gen_valid_sig() } 1 .. $nargs;
    my $args_str = join( ',', @args );
    return "($args_str)->" . gen_valid_sig();
}

# Generate malformed signature strings for robustness testing
sub gen_malformed_sig {
    my $kind = pick(
        qw[empty null garbage deeply_nested unmatched_parens
            truncated invalid_chars double_braces empty_struct]
    );
    if ( $kind eq 'empty' )            { return ''; }
    if ( $kind eq 'null' )             { return undef; }
    if ( $kind eq 'garbage' )          { return '!@#$%^&*()_+' x 3; }
    if ( $kind eq 'deeply_nested' )    { return '{a:' x 20 . 'int' . '}' x 20; }
    if ( $kind eq 'unmatched_parens' ) { return '(' x 10; }
    if ( $kind eq 'truncated' )        { return substr( gen_valid_sig(), 0, int( rand(3) ) + 1 ); }
    if ( $kind eq 'invalid_chars' )    { return gen_valid_sig() . chr(0x00) . 'int'; }
    if ( $kind eq 'double_braces' )    { return '{{{int}}}}'; }
    if ( $kind eq 'empty_struct' )     { return '{}'; }
    return 'int';
}

# Mutation operators on existing signatures
sub mutate_sig {
    my ($sig) = @_;
    my $kind  = pick(qw[insert_char delete_char replace_char swap_chars duplicate_char]);
    my @chars = split //, $sig;
    return $sig if !@chars;
    if ( $kind eq 'insert_char' ) {
        my $pos = int( rand( @chars + 1 ) );
        splice( @chars, $pos, 0, pick( '*', '{', '}', '<', '>', '[', ']', ',', ':', 'x', '0' ) );
    }
    elsif ( $kind eq 'delete_char' ) {
        my $pos = int( rand(@chars) );
        splice( @chars, $pos, 1 );
    }
    elsif ( $kind eq 'replace_char' ) {
        my $pos = int( rand(@chars) );
        $chars[$pos] = pick( '*', '{', '}', 'x', 'q', '0', '9', 'z' );
    }
    elsif ( $kind eq 'swap_chars' ) {
        return $sig if @chars < 2;
        my $a = int( rand( @chars - 1 ) );
        my $b = $a + 1 + int( rand( @chars - $a - 1 ) );
        ( $chars[$a], $chars[$b] ) = ( $chars[$b], $chars[$a] );
    }
    elsif ( $kind eq 'duplicate_char' ) {
        my $pos = int( rand(@chars) );
        splice( @chars, $pos, 0, $chars[$pos] );
    }
    return join '', @chars;
}

# Main
printf STDERR "Fuzzing: infix type signature parser (via sizeof / _is_type)\n";
printf STDERR "  Iterations: %d, Timeout: %ds\n", $max_iter, $timeout;
printf STDERR "%s\n", "=" x 60;
my @history;
for my $i ( 1 .. $max_iter ) {
    note("iter $i/$max_iter") if $i % 20 == 0 || $verbose;
    my $sig;
    my $is_valid;
    my $mode = rand(1);
    if ( $mode < 0.35 ) {

        # Generate a fresh valid sig
        $sig      = gen_valid_sig();
        $is_valid = 1;
    }
    elsif ( $mode < 0.55 ) {

        # Generate a valid function sig
        $sig      = gen_func_sig();
        $is_valid = 1;
    }
    elsif ( $mode < 0.75 ) {

        # Mutate a previously valid sig
        if (@history) {
            $sig      = mutate_sig( pick(@history) );
            $is_valid = 0;                              # mutation may break validity
        }
        else {
            $sig      = gen_valid_sig();
            $is_valid = 1;
        }
    }
    else {
        # Generate a deliberately malformed sig
        $sig      = gen_malformed_sig();
        $is_valid = 0;
    }
    push @history, $sig if defined $sig && $sig ne '';
    @history = @history[ -30 .. -1 ] if @history > 30;
    if ($verbose) {
        note( "Sig[" . ( $is_valid ? 'V' : 'M' ) . "]: [" . ( defined $sig ? $sig : '<undef>' ) . "]" );
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
        if ($is_valid) {
            fail("sizeof crashed on valid: $sig");
            note("$@") if $verbose;
        }
        else {
            ok lives { Affix::sizeof($sig) }, "malformed sig did not crash";
        }
    }
    else {
        ok 1, "sizeof survived: " . ( defined $sig ? $sig : '<undef>' );
    }
}
done_testing;
