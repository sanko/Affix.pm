use v5.40;
use lib '../lib';
use Affix::Bind;
use Path::Tiny;
use Data::Printer;
$|++;
#
my $driver_class = 'Affix::Bind::Driver::Clang';
#
sub spew_files ( $dir, %files ) {
    $dir->child($_)->spew_utf8( $files{$_} ) for keys %files;
    $dir;
}
#
my $dir = Path::Tiny->tempdir;
spew_files(
    $dir,
    'edge.h' => <<'EOF',
typedef struct {
    int data[16];
    char* name;
    float matrix[4][4];
} Buffer;
EOF
    'main.c' => '#include "edge.h"'
);
my $parser = $driver_class->new( project_files => [ $dir->child('edge.h')->stringify ] );
for my $obj ( $parser->parse( $dir->child('main.c')->stringify, [ $dir->stringify ] ) ) {
    p $obj;
    warn $obj->name;
    warn $obj->doc // '';
    warn $obj->describe;
    p $obj->members;
    if ( $obj isa 'Affix::Bind::Record' ) {
        for my $member ( @{ $obj->members } ) {
            p $member;
            say '     ' . $member->name . ' ' . $parser->_affix_type( $member->type );
        }
        ...;
    }
}
__END__
package Template::Liquid v1.0.0 {
    use v5.42;
    no warnings 'experimental';
    no warnings 'uninitialized';
    use MIME::Base64;
    use Time::Piece;
    use Scalar::Util;
    use List::Util;
    use feature 'class';
    use HTTP::Tiny;
    use JSON::PP;
    #
    package    #
        Template::Liquid::Utils {
        sub to_a { ref( $_[0] ) eq 'ARRAY' ? $_[0] : defined( $_[0] ) ? [ $_[0] ] : [] }
        sub to_n { Scalar::Util::looks_like_number( $_[0] ) ? 0 + $_[0] : 0 }

        sub to_s {
            my $v = $_[0];
            return '' unless defined $v;
            return join( '', map { to_s($_) } @$v )  if ref $v eq 'ARRAY';
            return $v                                if !ref $v || JSON::PP::is_bool($v);
            return $v->strftime('%Y-%m-%d %H:%M:%S') if ref $v eq 'Time::Piece';
            return '';
        }

        sub is_true {
            my $v = $_[0];
            return 0 if !defined $v;
            return 0 if JSON::PP::is_bool($v) && !$v;
            return 1;
        }

        sub strict_eq {
            my ( $a, $b ) = @_;
            my $ba = JSON::PP::is_bool($a);
            my $bb = JSON::PP::is_bool($b);
            if ( $ba || $bb )                                                                 { return $ba && $bb && "$a" eq "$b" }
            if ( !defined $a || !defined $b )                                                 { return !defined $a && !defined $b }
            if ( Scalar::Util::looks_like_number($a) && Scalar::Util::looks_like_number($b) ) { return $a == $b }
            return "$a" eq "$b";
        }
    }

    # AST Nodes
    class Template::Liquid::Node {
        method run( $c, $e ) { }
    }

    class Template::Liquid::Node::Blk : isa(Template::Liquid::Node) {
        field @n;
        method add($x) { push @n, $x }

        method run( $c, $e ) {
            my $o = '';
            for (@n) { $o .= $_->run( $c, $e ); return $o if $c->{_s} }
            $o;
        }
    }

    class Template::Liquid::Node::Txt : isa(Template::Liquid::Node) {
        field $t : param;
        method run( $c, $e ) {$t}
    }

    class Template::Liquid::Node::Out : isa(Template::Liquid::Node) {
        field $x : param;
        method run( $c, $e ) { Template::Liquid::Utils::to_s( $e->ev( $c, $x ) ) }
    }

    class Template::Liquid::Node::Set : isa(Template::Liquid::Node) {
        field $k : param;
        field $v : param;
        method run( $c, $e ) { $c->{$k} = $e->ev( $c, $v ); '' }
    }

    class Template::Liquid::Node::Cap : isa(Template::Liquid::Node) {
        field $k : param;
        field $b : param;
        method run( $c, $e ) { $c->{$k} = $b->run( $c, $e ); '' }
    }

    class Template::Liquid::Node::Flow : isa(Template::Liquid::Node) {
        field $t : param;
        method run( $c, $e ) { $c->{_s} = $t; '' }
    }

    class Template::Liquid::Node::Inc : isa(Template::Liquid::Node) {
        field $k : param;
        field $v : param;

        method run( $c, $e ) {
            my $n = $c->{_cnt}{$k} //= 0;
            $c->{_cnt}{$k} += $v;
            Template::Liquid::Utils::to_s($n);
        }
    }

    class Template::Liquid::Node::Dec : isa(Template::Liquid::Node) {
        field $k : param;
        field $v : param;

        method run( $c, $e ) {
            my $n = $c->{_cnt}{$k} //= 0;
            $c->{_cnt}{$k} += $v;
            Template::Liquid::Utils::to_s( $c->{_cnt}{$k} );
        }
    }

    class Template::Liquid::Node::Cyc : isa(Template::Liquid::Node) {
        field $g : param;
        field $v : param;

        method run( $c, $e ) {
            my $k = $g // join( $", @$v );
            Template::Liquid::Utils::to_s( $v->[ ( $c->{_cyc}{$k}++ ) % @$v ] );
        }
    }

    class Template::Liquid::Node::If : isa(Template::Liquid::Node) {
        field @b;
        method add( $c, $b, $i ) { push @b, [ $c, $b, $i ] }

        method run( $c, $e ) {
            for (@b) {
                if (
                    $_->[2] ? !Template::Liquid::Utils::is_true( $e->ev( $c, $_->[0] ) ) : Template::Liquid::Utils::is_true( $e->ev( $c, $_->[0] ) ) )
                {
                    return $_->[1]->run( $c, $e );
                }
            }
            '';
        }
    }

    class Template::Liquid::Node::Case : isa(Template::Liquid::Node) {
        field $v : param;
        field @b;
        method add( $k, $b ) { push @b, [ $k, $b ] }

        method run( $c, $e ) {
            my $w = $e->ev( $c, $v );
            for (@b) {
                if (
                    !defined $_->[0] ||
                    ( grep { my $c = $e->ev( $c, $_ ); Template::Liquid::Utils::strict_eq( $w, $c ) } @{ Template::Liquid::Utils::to_a( $_->[0] ) } )
                ) {
                    return $_->[1]->run( $c, $e );
                }
            }
            '';
        }
    }

    class Template::Liquid::Node::IfCh : isa(Template::Liquid::Node) {
        field $b : param;

        method run( $c, $e ) {
            my $out = $b->run( $c, $e );
            my $k   = refaddr($self);
            if ( $out ne ( $c->{_ifch}{$k} // '' ) ) { $c->{_ifch}{$k} = $out; return $out; }
            return '';
        }
    }

    class Template::Liquid::Node::For : isa(Template::Liquid::Node) {
        field $v   : param;
        field $s   : param;
        field $a   : param;
        field $blk : param;
        field $els;
        method else_b { $els = Template::Liquid::Node::Blk->new }

        method run( $c, $e ) {
            my $src = $e->ev( $c, $s );
            my @L;
            if    ( ref $src eq 'ARRAY' ) { @L = @$src }
            elsif ( ref $src eq 'HASH' ) {
                @L   = sort keys %$src;
                $src = [ map { [ $_, $src->{$_} ] } @L ];
            }
            my $p = $a // '';
            my ( $lm, $of, $rv ) = ( 0, 0, 0 );
            $lm = $1         if $p =~ /limit:\s*(\d+)/;
            $of = $1         if $p =~ /offset:\s*(\d+)/;
            $rv = 1          if $p =~ /reversed/;
            @L  = @$src      if ref $src eq 'ARRAY';
            @L  = reverse @L if $rv;
            splice( @L, 0, $of )       if $of;
            splice( @L, $lm )          if $lm;
            return $els->run( $c, $e ) if !@L && $els;
            my ( $o, $len ) = ( '', scalar @L );
            my $parent = $c->{forloop};

            for my $i ( 0 .. $#L ) {
                local $c->{$v} = $L[$i];
                local $c->{forloop} = {
                    name       => "$v-$s",
                    length     => $len,
                    index      => $i + 1,
                    index0     => $i,
                    first      => $i == 0,
                    last       => $i == $#L,
                    rindex     => $len - $i,
                    rindex0    => $len - $i - 1,
                    parentloop => $parent
                };
                $o .= $blk->run( $c, $e );
                if ( $c->{_s} ) { my $s = delete $c->{_s}; last if $s eq 'brk' }
            }
            $o;
        }
    }

    class Template::Liquid::Node::TRow : isa(Template::Liquid::Node) {
        field $v   : param;
        field $s   : param;
        field $a   : param;
        field $blk : param;

        method run( $c, $e ) {
            my $src = $e->ev( $c, $s );
            my @L   = @{ Template::Liquid::Utils::to_a($src) };
            my $p   = $a // '';
            my ( $cols, $lm, $of ) = ( 0, 0, 0 );
            $cols = $1 if $p =~ /cols:\s*(\d+)/;
            $lm   = $1 if $p =~ /limit:\s*(\d+)/;
            $of   = $1 if $p =~ /offset:\s*(\d+)/;
            splice( @L, 0, $of ) if $of;
            splice( @L, $lm ) if $lm;
            return '' unless @L;
            $cols ||= scalar @L;
            $cols = 1 if $cols < 1;
            my $o = qq{<tr class="row1">\n};
            my ( $i, $broken ) = ( 0, 0 );

            for ( ; $i <= $#L; $i++ ) {
                my $col = $i % $cols;
                my $row = int( $i / $cols ) + 1;
                $o .= qq{</tr>\n<tr class="row$row">\n} if $col == 0 && $i > 0;
                local $c->{$v} = $L[$i];
                local $c->{tablerow} = {
                    length    => scalar(@L),
                    index     => $i + 1,
                    index0    => $i,
                    col       => $col + 1,
                    col0      => $col,
                    first     => $i == 0,
                    last      => $i == $#L,
                    col_first => $col == 0,
                    col_last  => $col == $cols - 1,
                    row       => $row
                };
                $o .= qq{<td class="col} . ( $col + 1 ) . qq{">} . $blk->run( $c, $e ) . qq{</td>};
                if ( $c->{_s} ) { delete $c->{_s}; $broken = 1; $i++; last; }
            }
            if ( !$broken ) {
                my $rem = $cols - ( $i % $cols );
                if ( $rem < $cols && $rem > 0 ) {
                    for ( 1 .. $rem ) { $o .= qq{<td class="col} . ( $cols - $rem + $_ ) . qq{"></td>} }
                }
            }
            $o .= qq{</tr>\n};
            $o;
        }
    }

    class TinyLiquid {
        field $root;
        field %F = (
            abs           => sub { abs( Template::Liquid::Utils::to_n( $_[0] ) ) },
            at_least      => sub { List::Util::max( Template::Liquid::Utils::to_n( $_[0] ), Template::Liquid::Utils::to_n( $_[1] ) ) },
            at_most       => sub { List::Util::min( Template::Liquid::Utils::to_n( $_[0] ), Template::Liquid::Utils::to_n( $_[1] ) ) },
            ceil          => sub { int( Template::Liquid::Utils::to_n( $_[0] ) + 0.999 ) },
            divided_by    => sub { my $d = Template::Liquid::Utils::to_n( $_[1] ); $d ? Template::Liquid::Utils::to_n( $_[0] ) / $d : 0 },
            floor         => sub { int( Template::Liquid::Utils::to_n( $_[0] ) ) },
            minus         => sub { Template::Liquid::Utils::to_n( $_[0] ) - Template::Liquid::Utils::to_n( $_[1] ) },
            modulo        => sub { my $d = Template::Liquid::Utils::to_n( $_[1] ); $d ? Template::Liquid::Utils::to_n( $_[0] ) % $d : 0 },
            plus          => sub { Template::Liquid::Utils::to_n( $_[0] ) + Template::Liquid::Utils::to_n( $_[1] ) },
            round         => sub { sprintf( "%.0f", Template::Liquid::Utils::to_n( $_[0] ) ) },
            times         => sub { Template::Liquid::Utils::to_n( $_[0] ) * Template::Liquid::Utils::to_n( $_[1] ) },
            append        => sub { Template::Liquid::Utils::to_s( $_[0] ) . $_[1] },
            capitalize    => sub { ucfirst( Template::Liquid::Utils::to_s( $_[0] ) ) },
            downcase      => sub { lc( Template::Liquid::Utils::to_s( $_[0] ) ) },
            escape        => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/([&<>"'])/sprintf("&#%d;",ord$1)/ger },
            lstrip        => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/^\s+//r },
            newline_to_br => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\r?\n/<br \/>\n/gr },
            prepend       => sub { $_[1] . Template::Liquid::Utils::to_s( $_[0] ) },
            remove        => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\Q$_[1]\E//gr },
            remove_first  => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\Q$_[1]\E//r },
            replace       => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\Q$_[1]\E/$_[2]/gr },
            replace_first => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\Q$_[1]\E/$_[2]/r },
            rstrip        => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\s+$//r },
            size          => sub { ref $_[0] eq 'ARRAY' ? scalar @{ $_[0] } : length( Template::Liquid::Utils::to_s( $_[0] ) ) },
            slice         => sub {
                my ( $s, $i, $l ) = (
                    Template::Liquid::Utils::to_s( $_[0] ),
                    Template::Liquid::Utils::to_n( $_[1] ),
                    defined $_[2] ? Template::Liquid::Utils::to_n( $_[2] ) : 1
                );
                return '' if abs($i) > length($s);
                substr( $s, $i, $l ) // '';
            },
            split          => sub { return [] unless defined $_[0]; $_[1] //= ''; $_[1] eq '' ? [ split //, $_[0] ] : [ split /\Q$_[1]\E/, $_[0] ] },
            strip          => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/^\s+|\s+$//gr },
            strip_html     => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/<(?:script|style).*?>.*?<\/(?:script|style)>//gsr =~ s/<[^>]*>//gsr },
            strip_newlines => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\r?\n//gr },
            truncate       => sub {
                length( Template::Liquid::Utils::to_s( $_[0] ) ) > ( $_[1] // 50 ) ?
                    substr( $_[0], 0, ( $_[1] // 50 ) - length( $_[2] // '...' ) ) . ( $_[2] // '...' ) :
                    $_[0];
            },
            truncatewords => sub {
                my @w = split /\s+/, Template::Liquid::Utils::to_s( $_[0] );
                @w > $_[1] ? join( ' ', @w[ 0 .. ( $_[1] || 15 ) - 1 ] ) . ( $_[2] // '...' ) : $_[0];
            },
            upcase                 => sub { uc( Template::Liquid::Utils::to_s( $_[0] ) ) },
            url_decode             => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/\+/ /gr =~ s/%([a-fA-F0-9]{2})/chr(hex($1))/ger },
            url_encode             => sub { Template::Liquid::Utils::to_s( $_[0] ) =~ s/([^a-zA-Z0-9_.-])/sprintf("%%%02X",ord($1))/ger },
            base64_encode          => sub { MIME::Base64::encode_base64( Template::Liquid::Utils::to_s( $_[0] ) ) =~ s/\s+//gr },
            base64_decode          => sub { MIME::Base64::decode_base64( Template::Liquid::Utils::to_s( $_[0] ) ) },
            base64_url_safe_encode => sub { MIME::Base64::encode_base64url( Template::Liquid::Utils::to_s( $_[0] ) ) },
            base64_url_safe_decode => sub { MIME::Base64::decode_base64url( Template::Liquid::Utils::to_s( $_[0] ) ) },
            compact                => sub {
                [ grep {defined} @{ Template::Liquid::Utils::to_a( $_[0] ) } ]
            },
            concat => sub { [ ( @{ Template::Liquid::Utils::to_a( $_[0] ) }, @{ Template::Liquid::Utils::to_a( $_[1] ) } ) ] },
            first  => sub { Template::Liquid::Utils::to_a( $_[0] )->[0] },
            join   => sub { join( $_[1] // ' ', @{ Template::Liquid::Utils::to_a( $_[0] ) } ) },
            last   => sub { Template::Liquid::Utils::to_a( $_[0] )->[-1] },
            map    => sub ( $v, $k ){
                [ map { ref $_ eq 'HASH' ? $_->{$k} : ( ref $_ eq 'ARRAY' && $k eq 'size' ? scalar @$_ : undef ) }
                        @{ Template::Liquid::Utils::to_a($v) } ];
            },
            reverse => sub { ref $_[0] eq 'ARRAY' ? [ reverse @{ $_[0] } ] : [ reverse split //, Template::Liquid::Utils::to_s( $_[0] ) ] },
            sort    => sub {
                [ sort { Template::Liquid::Utils::to_s($a) cmp Template::Liquid::Utils::to_s($b) } @{ Template::Liquid::Utils::to_a( $_[0] ) } ]
            },
            sort_natural => sub  ( $r, $k ){
                [   sort {
                        my ( $A, $B ) = defined $k ? ( $a->{$k}, $b->{$k} ) : ( $a, $b );
                        fc( Template::Liquid::Utils::to_s($A) ) cmp fc( Template::Liquid::Utils::to_s($B) )
                    } @{ Template::Liquid::Utils::to_a($r) }
                ];
            },
            sum => sub ( $r, $k ){
                             my $s = 0;
                $s += ( ref $_ eq 'HASH' ? $_->{$k} : $_ )
                    for grep { Scalar::Util::looks_like_number( ref $_ eq 'HASH' ? $_->{$k} : $_ ) } @{ Template::Liquid::Utils::to_a($r) };
                $s;
            },
            uniq => sub {
                my %s;
                [ grep { !$s{ ref $_ ? Template::Liquid::Utils::to_s($_) : $_ }++ } @{ Template::Liquid::Utils::to_a( $_[0] ) } ];
            },
            where => sub ( $r, $k, $v ) {

                [   grep {
                        ref $_ eq 'HASH' &&
                            (
                            defined $v ? Template::Liquid::Utils::strict_eq( $_->{$k}, $v ) :
                            ( exists $_->{$k} ? ( defined $_->{$k} && $_->{$k} ne JSON::PP::false ) : 0 ) )
                    } @{ Template::Liquid::Utils::to_a($r) }
                ];
            },
            date => sub( $i, $f ) {

                $f //= '%b %d, %Y';
                my $s = Template::Liquid::Utils::to_s($i);
                $s =~ s/^\s+|\s+$//g;
                return '' if $s eq '';
                return $s if $s eq 'now' || $s eq 'today';
                my $t;
                if ( $s =~ /^\d+$/ ) { $t = gmtime($s) }
                else {
                    try { $t = Time::Piece->strptime( $s, "%Y-%m-%d %H:%M:%S" ) } catch($e) {  }
                }
                return $s unless $t;
                try { return $t->strftime($f) } catch($e) { return $s };
            },
            default => sub { my ( $v, $d, %a ) = @_; ( $a{allow_false} ? defined $v : Template::Liquid::Utils::is_true($v) ) ? $v : $d },
        );

        method val( $c, $k ) {
            return undef           if $k eq 'nil' || $k eq 'null';
            return JSON::PP::true  if $k eq 'true';
            return JSON::PP::false if $k eq 'false';
            return $2 =~ s/\\n/\n/gr =~ s/\\t/\t/gr =~ s/\\r/\r/gr if $k =~ /^(['"])(.*)\1$/;
            return 0 + $k if $k =~ /^-?\d+(\.\d+)?$/;
            my $v = $c;

            # Enforce valid start identifier [a-zA-Z_]
            if ( $k =~ s/^([a-zA-Z_][\w\-\?]*)(?=[.\[]|$)// ) {
                $v = ref $v eq 'HASH' ? $v->{$1} : undef;
            }
            else {
                return undef;
            }
            while ( $k ne '' ) {
                if ( $k =~ s/^\.([\w\-\?]+)// ) {
                    my $key = $1;
                    if    ( $key eq 'size' ) { $v = ref $v eq 'ARRAY' ? scalar @$v : length( Template::Liquid::Utils::to_s($v) ) }
                    elsif ( $key eq 'first' && ref $v eq 'ARRAY' ) { $v = $v->[0] }
                    elsif ( $key eq 'last' && ref $v eq 'ARRAY' )  { $v = $v->[-1] }
                    else                                           { $v = ref $v eq 'HASH' ? $v->{$key} : undef }
                }
                elsif ( $k =~ s/^\[(["'])(.*?)\1\]// ) { $v = ref $v eq 'HASH'  ? $v->{$2} : undef }
                elsif ( $k =~ s/^\[(-?\d+)\]// )       { $v = ref $v eq 'ARRAY' ? $v->[$1] : undef }
                elsif ( $k =~ s/^\[([^0-9"'].*?)\]// ) {
                    my $kv = $self->val( $c, $1 );
                    $v = ref $v eq 'HASH' ? $v->{$kv} : ( ref $v eq 'ARRAY' ? $v->[$kv] : undef );
                }
                else { return undef }
            }
            $v;
        }

        method ev( $c, $e ) {
            my $trim = sub { $_[0] =~ s/^\s+|\s+$//gr };
            if ( $e =~ /^\s*\((.+?)\.\.(.+?)\)\s*$/ ) {
                my ( $start, $end ) = ( Template::Liquid::Utils::to_n( $self->ev( $c, $1 ) ), Template::Liquid::Utils::to_n( $self->ev( $c, $2 ) ) );
                return [] if abs( $end - $start ) > 10000;
                my $r = [ $start .. $end ];
                return $self->apply_filters( $c, $r, split( /(?:\|)(?=(?:[^'"]|'[^']*'|"[^"]*")*$)/, $e ) ); # Hack to split filters only on top level
            }
            my ( $raw, @fs );

            # Split on pipes not in quotes
            while ( $e =~ /((?:[^|'"]|'[^']*'|"[^"]*")+)/g ) { push @fs, $1 if defined $1 && length $trim->($1) }
            $raw = shift @fs;
            if ( $raw =~ /(.+?)\s+(contains|==|!=|<>|<=|>=|<|>)\s+(.+)/ ) {
                my ( $l, $op, $r ) = ( $self->val( $c, $trim->($1) ), $2, $self->val( $c, $trim->($3) ) );
                if ( $op eq 'contains' ) {
                    return 0 unless defined $l;
                    return ( grep { defined $_ && Template::Liquid::Utils::strict_eq( $_, $r ) } @$l ) > 0 if ref $l eq 'ARRAY';
                    return exists $l->{$r}                                                                 if ref $l eq 'HASH';
                    return index( Template::Liquid::Utils::to_s($l), Template::Liquid::Utils::to_s($r) ) >= 0;
                }
                if ( $op eq '==' ) { return Template::Liquid::Utils::strict_eq( $l,  $r ) }
                if ( $op eq '!=' ) { return !Template::Liquid::Utils::strict_eq( $l, $r ) }
                if ( !defined $l || !defined $r ) { return 0 }
                if ( Scalar::Util::looks_like_number($l) && Scalar::Util::looks_like_number($r) ) {
                    return $op eq '<' ? $l < $r : $op eq '>' ? $l > $r : $op eq '<=' ? $l <= $r : $l >= $r;
                }
                return 0;
            }
            my $v = $self->val( $c, $trim->($raw) );
            $self->apply_filters( $c, $v, @fs );
        }

        method apply_filters( $c, $v, @fs ) {
            my $trim = sub { $_[0] =~ s/^\s+|\s+$//gr };
            for (@fs) {
                my ( $n, $args ) = split /:/, $_, 2;
                my @a;
                if ($args) {
                    while ( $args =~ /((?:[^,'"]|'[^']*'|"[^"]*")+)/g ) {
                        my $kv = $trim->($1);
                        if ( $kv =~ /^(\w+)\s*:\s*(.*)$/ ) { push @a, $1, $self->val( $c, $2 ) }
                        else                               { push @a, $self->val( $c, $kv ) }
                    }
                }
                $v = ( $F{ $trim->($n) } // sub { $_[0] } )->( $v, @a );
            }
            $v;
        }

        method handle_tag( $k, $a, $s, $c ) {
            $a //= '';
            if ( $k =~ /^(if|unless|case)$/ ) {
                my $n = $k eq 'case' ? Template::Liquid::Node::Case->new( v => $a ) : Template::Liquid::Node::If->new;
                $s->[-1]->add($n);
                push @$c, $n;
                push @$s, ( my $b = Template::Liquid::Node::Blk->new );
                if ( $k ne 'case' ) { $n->add( $a, $b, $k eq 'unless' ) }
            }
            elsif ( $k =~ /^(elsif|else|when)$/ ) {
                pop @$s;
                push @$s, ( my $b = Template::Liquid::Node::Blk->new );
                my $top = $c->[-1];
                if    ( $top isa Template::Liquid::Node::For ) { $top->else_b->add($b) }
                elsif ( $top isa Template::Liquid::Node::Case ) {
                    my @vals;
                    while ( $a =~ /((?:[^,'"]|'[^']*'|"[^"]*")+)/g ) { push @vals, $1 =~ s/^['"]|['"]$//gr }
                    $top->add( \@vals, $b );
                }
                else { $top->add( $k eq 'else' ? 'true' : $a, $b, 0 ) }
            }
            elsif ( $k =~ /^end(if|unless|case|for|capture|ifchanged|tablerow)/ ) { pop @$s; pop @$c }
            elsif ( $k =~ /^(for|tablerow)/ ) {
                $a =~ /(\w+)\s+in\s+(.+?)(?:\s+(.+))?$/;
                if ( $k eq 'for' ) {
                    $s->[-1]->add( my $n
                            = Template::Liquid::Node::For->new( v => $1, s => $2, a => $3, blk => ( my $b = Template::Liquid::Node::Blk->new ) ) );
                    push @$c, $n;
                    push @$s, $b;
                }
                else {
                    $s->[-1]->add( my $n
                            = Template::Liquid::Node::TRow->new( v => $1, s => $2, a => $3, blk => ( my $b = Template::Liquid::Node::Blk->new ) ) );
                    push @$c, $n;
                    push @$s, $b;
                }
            }
            elsif ( $k eq 'ifchanged' ) {
                $s->[-1]->add( my $n = Template::Liquid::Node::IfCh->new( b => ( my $b = Template::Liquid::Node::Blk->new ) ) );
                push @$c, $n;
                push @$s, $b;
            }
            elsif ( $k eq 'capture' ) {
                if ( $a =~ /^([a-zA-Z_][\w\-]*)$/ ) {
                    $s->[-1]->add( my $n = Template::Liquid::Node::Cap->new( k => $1, b => ( my $b = Template::Liquid::Node::Blk->new ) ) );
                    push @$c, $n;
                    push @$s, $b;
                }
                else {
                    $s->[-1]
                        ->add( my $n = Template::Liquid::Node::Cap->new( k => '__invalid__', b => ( my $b = Template::Liquid::Node::Blk->new ) ) );
                    push @$c, $n;
                    push @$s, $b;
                }
            }
            elsif ( $k eq 'cycle' ) {
                my ( $g, $v ) = ( $1, $2 ) if $a =~ /(?:(['"].+?['"])\s*:\s*)?(.+)/;
                $s->[-1]->add( Template::Liquid::Node::Cyc->new( g => $g, v => [ split /\s*,\s*/, $v =~ s/['"]//gr ] ) );
            }
            elsif ( $k =~ /^(break|continue)$/ ) { $s->[-1]->add( Template::Liquid::Node::Flow->new( t => $k eq 'break' ? 'brk' : 'cnt' ) ) }
            elsif ( $k eq 'assign' ) {
                if ( $a =~ /^([a-zA-Z_][\w\-]*)\s*=\s*(.+)/ ) { $s->[-1]->add( Template::Liquid::Node::Set->new( k => $1, v => $2 ) ) }
            }
            elsif ( $k eq 'echo' )      { $s->[-1]->add( Template::Liquid::Node::Out->new( x => $a ) ) }
            elsif ( $k eq 'increment' ) { $s->[-1]->add( Template::Liquid::Node::Inc->new( k => $a, v => 1 ) ) }
            elsif ( $k eq 'decrement' ) { $s->[-1]->add( Template::Liquid::Node::Dec->new( k => $a, v => -1 ) ) }
        }

        method parse($t) {
            my @s = ( $root = Template::Liquid::Node::Blk->new );
            my @c;
            pos($t) = 0;
            while ( pos($t) < length($t) ) {
                if ( $t =~ /\G(.*?)(?:(\{\{-?\s*(.*?)\s*-?\}\})|(\{%\-?\s*(.*?)\s*-?%\}))/gsc ) {
                    my ( $txt, $out, $tag ) = ( $1, $3, $5 );
                    if    ( $2 && $2 =~ /^\{\{-/ ) { $s[-1]->add( Template::Liquid::Node::Txt->new( t => $txt =~ s/\s+$//r ) ); $txt = '' }
                    elsif ( $4 && $4 =~ /^\{%-/ )  { $s[-1]->add( Template::Liquid::Node::Txt->new( t => $txt =~ s/\s+$//r ) ); $txt = '' }
                    else                           { $s[-1]->add( Template::Liquid::Node::Txt->new( t => $txt ) ) }
                    if    ($out) { $s[-1]->add( Template::Liquid::Node::Out->new( x => $out ) ) }
                    elsif ($tag) {
                        if ( $tag =~ /^raw/ ) {
                            if ( $t =~ /\G(.*?)(\{%\-?\s*endraw\s*-?%\})/gsc ) { $s[-1]->add( Template::Liquid::Node::Txt->new( t => $1 ) ) }
                            next;
                        }
                        if ( $tag =~ /^comment/ ) { $t =~ /\G.*?\{%\-?\s*endcomment\s*-?%\}/gsc; next; }
                        if ( $tag =~ /^doc/ )     {next}
                        if ( $tag eq 'liquid' ) {
                            if ( $t =~ /\G(.*?)(?:(?=\{%)|$)/gsc ) {
                                for ( split /\n/, $1 ) {
                                    next if /^\s*#|^\s*$/;
                                    s/^\s+//;
                                    my ( $k, $a ) = split /\s+/, $_, 2;
                                    $self->handle_tag( $k, $a, \@s, \@c );
                                }
                            }
                            next;
                        }
                        my ( $k, $a ) = split /\s+/, $tag, 2;
                        $self->handle_tag( $k, $a, \@s, \@c );
                    }
                }
                else {
                    $s[-1]->add( Template::Liquid::Node::Txt->new( t => substr( $t, pos($t) ) ) );
                    last;
                }
            }
        }
        method render($c) { $root->run( $c, $self ) }
    }
};
1;
use HTTP::Tiny;
use JSON::PP;
my $url  = 'https://raw.githubusercontent.com/jg-rp/golden-liquid/main/golden_liquid.json';
my $resp = HTTP::Tiny->new->get($url);
if ( !$resp->{success} ) { die "Failed: $resp->{status}\n" }
my $tests = decode_json( $resp->{content} )->{tests};
my ( $pass, $fail, $skip ) = ( 0, 0, 0 );

for my $t (@$tests) {

    #~ if ($t->{template} =~ /\{% (include)/) { $skip++; next }
    eval {
        my $app = TinyLiquid->new;
        $app->parse( $t->{template} );
        my $out = $app->render( $t->{data} // {} );
        if ( $out eq $t->{result} ) { $pass++ }
        else {
            $fail++;
            my ( $got, $exp ) = ( $out, $t->{result} );
            s/\n/\\n/g for ( $got, $exp );
            print STDERR "FAIL: $t->{name}\n  EXP: [$exp]\n  GOT: [$got]\n";
        }
    };
    if ($@) { $fail++; print STDERR "CRASH: $t->{name}\n  ERR: $@\n"; }
}
print "PASS: $pass, FAIL: $fail, SKIP: $skip\n";
