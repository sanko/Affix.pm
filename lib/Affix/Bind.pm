package Affix::Bind v1.0.0 {
    use v5.40;
    use feature 'class';
    no warnings 'experimental::class';
    no warnings 'experimental::builtin';
    use Path::Tiny;
    use Capture::Tiny qw[capture];
    use JSON::PP;
    use File::Basename qw[basename];

    class Affix::Bind::Entity {
        field $name         : reader : param //= '';
        field $doc          : reader : param //= undef;
        field $file         : reader : param //= '';
        field $line         : reader : param //= 0;
        field $end_line     : reader : param //= 0;
        field $start_offset : reader : param //= 0;
        field $end_offset   : reader : param //= 0;
        field $is_merged    : reader = 0;
        method mark_merged { $is_merged = 1 }
        method _base($p)   { return '' unless defined $p; $p =~ s{^.*[/\\]}{}; return $p }

        method describe {
            my $t = ref($self);
            $t =~ s/^Affix::Bind:://;
            return "[$t] $name (" . $self->_base($file) . ":$line)";
        }
    }

    class Affix::Bind::Member {
        field $name       : reader : param //= '';
        field $type       : reader : param //= '';
        field $doc        : reader : param //= undef;
        field $definition : reader : param //= undef;
    }

    class Affix::Bind::Macro : isa(Affix::Bind::Entity) {
        field $value : reader : param //= '';
    }

    class Affix::Bind::Variable : isa(Affix::Bind::Entity) {
        field $type : reader : param //= '';
    }

    class Affix::Bind::Typedef : isa(Affix::Bind::Entity) {
        field $underlying : reader : param //= '';
    }

    class Affix::Bind::Struct : isa(Affix::Bind::Entity) {
        field $tag     : reader : param //= 'struct';
        field $members : reader : param //= [];
    }

    class Affix::Bind::Enum : isa(Affix::Bind::Entity) {
        field $constants : reader : param //= [];
    }

    class Affix::Bind::Function : isa(Affix::Bind::Entity) {
        field $ret          : reader : param //= 'void';
        field $args         : reader : param //= [];
        field $mangled_name : reader : param //= undef;
    }

    # =============================================================================
    # DRIVER: CLANG
    # =============================================================================
    class Affix::Bind::Driver::Clang {
        field $project_files : param;
        field $allowed_files = {};
        field $paths_seen    = {};
        field $file_cache    = {};

        method _basename ($path) {
            return "" unless defined $path;
            $path =~ s{^.*[/\\]}{};
            return lc($path);
        }
        ADJUST {
            foreach my $f (@$project_files) {
                my $base = $self->_basename($f);
                $allowed_files->{$base} = $f;
            }
        }

        method parse ( $entry_point, $include_dirs = [] ) {
            my $ep_abs = Path::Tiny::path($entry_point)->absolute->stringify;
            $ep_abs =~ s{\\}{/}g;

            # Add entry point to allowed files to capture typedefs that fall back to TU scope
            my $ep_base = $self->_basename($ep_abs);
            $allowed_files->{$ep_base} = $ep_abs;
            my @includes = map {
                my $p = Path::Tiny::path($_)->absolute->stringify;
                $p =~ s{\\}{/}g;
                "-I$p"
            } @$include_dirs;

            # Ensure project file directories are included
            my %seen_dirs;
            foreach my $pf ( @$project_files, $ep_abs ) {
                my $dir = Path::Tiny::path($pf)->parent->absolute->stringify;
                $dir =~ s{\\}{/}g;
                unless ( $seen_dirs{$dir}++ ) {
                    push @includes, "-I$dir";
                }
            }
            my @cmd = (
                'clang',                          '-Xclang',       '-ast-dump=json',       '-Xclang',
                '-detailed-preprocessing-record', '-fsyntax-only', '-fparse-all-comments', '-Wno-everything',
                @includes,                        $ep_abs
            );
            my ( $stdout, $stderr, $exit ) = Capture::Tiny::capture { system(@cmd); };
            if ( $exit != 0 )               { die "Clang Error:\n$stderr"; }
            if ( $stdout =~ /^.*?(\{.*)/s ) { $stdout = $1; }
            my $ast = JSON::PP::decode_json($stdout);
            my @objects;
            $self->_walk( $ast, \@objects, $ep_abs );
            $self->_scan_macros_fallback( \@objects );
            $self->_merge_typedefs( \@objects );
            @objects = sort { ( $a->file cmp $b->file ) || ( $a->start_offset <=> $b->start_offset ) } @objects;
            return \@objects;
        }

        method _walk( $node, $acc, $current_file ) {
            return unless ref $node eq 'HASH';
            my $kind = $node->{kind} // 'Unknown';

            # 1. Update Context
            my $node_file = $self->_get_node_file($node);
            if ($node_file) { $current_file = $node_file; }

            # 2. Filter
            my $base    = $self->_basename($current_file);
            my $is_ours = exists $allowed_files->{$base};

            # 3. Capture
            if ( $is_ours && !$node->{isImplicit} ) {
                if ( $kind eq 'MacroDefinitionRecord' ) {
                    if ( $node->{range} ) { $self->_macro( $node, $acc, $current_file ); }
                }
                elsif ( $kind eq 'TypedefDecl' ) {
                    $self->_typedef( $node, $acc, $current_file );
                }
                elsif ( $kind eq 'RecordDecl' || $kind eq 'CXXRecordDecl' ) {
                    $self->_record( $node, $acc, $current_file );
                    return;    # Stop recursion: we handle inner members inside _record
                }
                elsif ( $kind eq 'EnumDecl' ) {
                    $self->_enum( $node, $acc, $current_file );
                    return;    # Stop recursion
                }
                elsif ( $kind eq 'VarDecl' ) {
                    if ( ( $node->{storageClass} // '' ) ne 'static' ) {
                        $self->_var( $node, $acc, $current_file );
                    }
                }
                elsif ( $kind eq 'FunctionDecl' ) {
                    $self->_func( $node, $acc, $current_file );
                    return;    # Stop recursion
                }
            }

            # Recurse
            if ( $node->{inner} ) {
                foreach ( @{ $node->{inner} } ) { $self->_walk( $_, $acc, $current_file ); }
            }
        }

        method _get_node_file($node) {
            my $loc = $node->{loc};
            return undef unless $loc;
            my $f;
            if ( ref($loc) eq 'HASH' ) {
                $f = $loc->{presumedLoc}{file} || $loc->{expansionLoc}{file} || $loc->{spellingLoc}{file} || $loc->{file};
            }

            # Fallback to range begin if loc is missing file
            if ( !$f && $node->{range} && $node->{range}{begin} ) {
                my $b = $node->{range}{begin};
                $f = $b->{presumedLoc}{file} || $b->{expansionLoc}{file} || $b->{spellingLoc}{file} || $b->{file};
            }
            return undef unless $f;
            $f =~ s{\\}{/}g;
            $paths_seen->{$f}++;
            return $f;
        }

        method _meta($n) {
            my $s        = $n->{range}{begin}{offset} // 0;
            my $e        = $n->{range}{end}{offset}   // 0;
            my $line     = 0;
            my $end_line = 0;
            if ( ref( $n->{loc} ) eq 'HASH' ) {
                $line = $n->{loc}{presumedLoc}{line} || $n->{loc}{line};
            }
            $line ||= $n->{range}{begin}{line} // 0;
            if ( $n->{range}{end} ) {
                $end_line = $n->{range}{end}{presumedLoc}{line} || $n->{range}{end}{line} || $n->{range}{end}{expansionLoc}{line} || $line;
            }
            else {
                $end_line = $line;
            }
            return ( $s, $e, $line, $end_line );
        }

        method _macro( $n, $acc, $f ) {
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            my $val = $self->_extract_macro_val( $n, $f );
            return unless length($val);
            push @$acc,
                Affix::Bind::Macro->new(
                name         => $n->{name},
                file         => $f,
                line         => $l,
                end_line     => $el,
                value        => $val,
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        method _typedef( $n, $acc, $f ) {
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            push @$acc,
                Affix::Bind::Typedef->new(
                name         => $n->{name},
                file         => $f,
                line         => $l,
                end_line     => $el,
                underlying   => $n->{type}{qualType},
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        method _record( $n, $acc, $f ) {
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            my $m_list = $self->_extract_members( $n, $f );
            return unless ( $n->{name} || @$m_list );
            my $tag = $n->{tagUsed} // 'struct';
            push @$acc,
                Affix::Bind::Struct->new(
                name         => $n->{name} // '(anonymous)',
                tag          => $tag,
                file         => $f,
                line         => $l,
                end_line     => $el,
                members      => $m_list,
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        method _extract_members( $n, $f ) {
            my @members;
            return \@members unless $n->{inner};
            my @pending_anonymous_records;
            for my $child ( @{ $n->{inner} } ) {
                my $kind = $child->{kind} // '';
                if ( $kind eq 'RecordDecl' || $kind eq 'CXXRecordDecl' ) {
                    my $sub_members = $self->_extract_members( $child, $f );
                    my $rec         = Affix::Bind::Struct->new(
                        name     => $child->{name}    // '',
                        tag      => $child->{tagUsed} // 'struct',
                        file     => $f,
                        line     => $child->{loc}{line} // 0,
                        end_line => $child->{loc}{line} // 0,
                        members  => $sub_members,
                        doc      => undef
                    );
                    my $name = $child->{name} // '';
                    if ( $name eq '' ) {
                        push @pending_anonymous_records, $rec;
                    }
                }
                elsif ( $kind eq 'FieldDecl' ) {
                    my $name = $child->{name} // '';
                    my $type = $child->{type}{qualType};
                    my $def  = undef;
                    if (@pending_anonymous_records) {
                        $def = pop @pending_anonymous_records;
                    }
                    my $f_offset = $child->{range}{begin}{offset};
                    my $f_doc    = $self->_extract_doc( $f, $f_offset );
                    push @members, Affix::Bind::Member->new( name => $name, type => $type, doc => $f_doc, definition => $def );
                }
            }
            return \@members;
        }

        method _enum( $n, $acc, $f ) {
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            my @c;
            my $cnt = 0;
            for my $ch ( @{ $n->{inner} } ) {
                if ( ( $ch->{kind} // '' ) eq 'EnumConstantDecl' ) {
                    my $v = undef;
                    if ( $ch->{inner} ) {
                        for ( @{ $ch->{inner} } ) {
                            if ( $_->{kind} =~ /ConstantExpr|IntegerLiteral/ ) { $v = $_->{value}; }
                        }
                    }
                    $v //= $cnt;
                    $cnt = $v + 1;
                    push @c, { name => $ch->{name}, value => $v };
                }
            }
            push @$acc,
                Affix::Bind::Enum->new(
                name         => $n->{name} // '(anonymous)',
                file         => $f,
                line         => $l,
                end_line     => $el,
                constants    => \@c,
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        method _var( $n, $acc, $f ) {
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            push @$acc,
                Affix::Bind::Variable->new(
                name         => $n->{name},
                file         => $f,
                line         => $l,
                end_line     => $el,
                type         => $n->{type}{qualType},
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        method _func( $n, $acc, $f ) {
            return if ( $n->{storageClass} // '' ) eq 'static';
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            my $ret = $n->{type}{qualType};
            $ret =~ s/\(.*\)//;
            my @a;
            for ( @{ $n->{inner} } ) {
                if ( ( $_->{kind} // '' ) eq 'ParmVarDecl' ) {
                    push @a, ( $_->{type}{qualType} . " " . ( $_->{name} // '' ) );
                }
            }
            push @$acc,
                Affix::Bind::Function->new(
                name         => $n->{name},
                mangled_name => $n->{mangledName},
                file         => $f,
                line         => $l,
                end_line     => $el,
                ret          => $ret,
                args         => \@a,
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        # --- Utilities ---
        method _get_content($f) {
            my $base = $self->_basename($f);
            return $file_cache->{$base} if exists $file_cache->{$base};
            if ( exists $allowed_files->{$base} ) {
                return $file_cache->{$base} = Path::Tiny::path( $allowed_files->{$base} )->slurp_utf8;
            }
            return "";
        }

        method _extract_doc( $f, $off ) {
            return undef unless defined $off;
            my $content = $self->_get_content($f);
            return undef unless length($content);
            my $pre   = substr( $content, 0, $off );
            my @lines = split /\n/, $pre;
            my @d;
            my $cap = 0;
            while ( my $line = pop @lines ) {
                next if !$cap && $line =~ /^\s*$/;
                if    ( $line =~ /\*\/\s*$/ ) { $cap = 1; }
                elsif ( $line =~ /^\s*\/\// ) { $cap = 1; }
                if    ($cap) {
                    unshift @d, $line;
                    last if $line =~ /^\s*\/\*/;
                    if ( $line =~ /^\s*\/\// && ( !@lines || $lines[-1] !~ /^\s*\/\// ) ) { last; }
                }
                else { last; }
            }
            return undef unless @d;
            my $t = join( "\n", @d );
            $t =~ s/^\s*\/\*\*?//mg;
            $t =~ s/\s*\*\/$//mg;
            $t =~ s/^\s*\*\s?//mg;
            $t =~ s/^\s*\/\/\s?//mg;
            $t =~ s/^\s+|\s+$//g;
            return $t;
        }

        method _extract_macro_val( $n, $f ) {
            my $off = $n->{range}{begin}{offset};
            return "" unless defined $off;
            my $content = $self->_get_content($f);
            return "" unless length($content);
            my $r = substr( $content, $off );
            if ( $r =~ /^(.*?)$/m ) {
                my $line = $1;
                my $name = $n->{name};
                if ( $line =~ /#\s*define\s+\Q$name\E\s+(.*)/ ) {
                    my $v = $1;
                    $v =~ s/^\s+|\s+$//g;
                    return $v;
                }
            }
            return "";
        }

        method _scan_macros_fallback($acc) {
            my %seen = map { $_->name => 1 } grep { ref($_) eq 'Affix::Bind::Macro' } @$acc;
            foreach my $base ( keys %$allowed_files ) {
                my $f = $allowed_files->{$base};
                my $c = $self->_get_content($f);
                while ( $c =~ /^\s*#\s*define\s+(\w+)(?:[ \t]+(.*?))?\s*$/mg ) {
                    my $name = $1;
                    next if $seen{$name};
                    my $val  = $2 // "";
                    my $off  = $-[0];
                    my $end  = $+[0];
                    my $pre  = substr( $c, 0, $off );
                    my $line = ( $pre =~ tr/\n// ) + 1;
                    push @$acc,
                        Affix::Bind::Macro->new(
                        name         => $name,
                        file         => $f,
                        line         => $line,
                        end_line     => $line,
                        value        => $val,
                        doc          => $self->_extract_doc( $f, $off ),
                        start_offset => $off,
                        end_offset   => $end
                        );
                }
            }
        }

        method _merge_typedefs($objs) {
            my @tds = grep { ref($_) eq 'Affix::Bind::Typedef' } @$objs;
            foreach my $td (@tds) {
                next if $td->is_merged;
                my ($child) = grep {
                    !$_->is_merged                                                               &&
                        ( $_->name eq '(anonymous)' || $_->name eq '' || $_->name eq $td->name ) &&
                        ( ref($_) eq 'Affix::Bind::Enum' || ref($_) eq 'Affix::Bind::Struct' )   &&

                        # Use a wider window for multi-line structs
                        ( ( abs( $_->line - $td->line ) < 50 ) || ( abs( $_->end_line - $td->line ) < 50 ) )
                } @$objs;
                if ($child) {
                    my $new;
                    my $doc       = $td->doc     // $child->doc;
                    my $real_file = $child->file // $td->file;
                    if ( !defined $doc ) {
                        $doc = $self->_extract_doc( $real_file, $td->start_offset );
                    }
                    if ( ref($child) eq 'Affix::Bind::Enum' ) {
                        $new = Affix::Bind::Enum->new(
                            name         => $td->name,
                            file         => $real_file,
                            line         => $td->line,
                            end_line     => $child->end_line,
                            doc          => $doc,
                            start_offset => $td->start_offset,
                            end_offset   => $td->end_offset,
                            constants    => $child->constants
                        );
                    }
                    elsif ( ref($child) eq 'Affix::Bind::Struct' ) {
                        $new = Affix::Bind::Struct->new(
                            name         => $td->name,
                            tag          => $child->tag,
                            file         => $real_file,
                            line         => $td->line,
                            end_line     => $child->end_line,
                            doc          => $doc,
                            start_offset => $td->start_offset,
                            end_offset   => $td->end_offset,
                            members      => $child->members
                        );
                    }
                    for ( my $i = 0; $i < @$objs; $i++ ) {
                        if ( $objs->[$i] == $td ) { $objs->[$i] = $new; last; }
                    }
                    $child->mark_merged();
                }
            }
            @$objs = grep { !$_->is_merged } @$objs;
        }
    }

    # =============================================================================
    # DRIVER: REGEX (FALLBACK)
    # =============================================================================
    class Affix::Bind::Driver::Regex {
        field $project_files : param;

        method parse( $entry_point, $ids = [] ) {
            my @objs;
            for my $f (@$project_files) {
                if ( $f =~ /\.h(pp|xx)?$/i ) { $self->_scan( $f, \@objs ); $self->_scan_funcs( $f, \@objs ); }
                else                         { $self->_scan_funcs( $f, \@objs ); }
            }
            @objs = sort { ( $a->file cmp $b->file ) || ( $a->start_offset <=> $b->start_offset ) } @objs;
            return \@objs;
        }
        method _read($f) { Path::Tiny::path($f)->slurp_utf8 }

        method _scan( $f, $acc ) {
            my $c = $self->_read($f);
            while ( $c =~ /^\s*#\s*define\s+(\w+)\s+(.+?)\s*$/gm ) {
                my $s = $-[0];
                my $e = $+[0];
                push @$acc,
                    Affix::Bind::Macro->new(
                    name     => $1,
                    value    => $2,
                    file     => $f,
                    line     => $self->_ln( $c, $s ),
                    end_line => $self->_ln( $c, $e ),
                    doc      => $self->_doc( $c, $s )
                    );
            }
            while ( $c =~ /typedef\s+struct\s*(\{(?:[^{}]++|(?1))*\})\s*(\w+)\s*;/gs ) {
                my $s   = $-[0];
                my $e   = $+[0];
                my $mem = $self->_mem( substr( $1, 1, -1 ) );
                push @$acc,
                    Affix::Bind::Struct->new(
                    name     => $2,
                    tag      => 'struct',
                    members  => $mem,
                    file     => $f,
                    line     => $self->_ln( $c, $s ),
                    end_line => $self->_ln( $c, $e ),
                    doc      => $self->_doc( $c, $s )
                    );
            }
            while ( $c =~ /enum\s+(\w+)\s*(\{(?:[^{}]++|(?2))*\})\s*;/gs ) {
                my $s = $-[0];
                my $e = $+[0];
                my $b = substr( $2, 1, -1 );
                my @cs;
                my $v = 0;
                for ( split /,/, $b ) {
                    s/\/\/.*$//;
                    s/\/\*.*?\*\///s;
                    if (/\s*(\w+)\s*(?:=\s*(\d+))?/) { my $val = $2 // $v; $v = $val + 1; push @cs, { name => $1, value => $val }; }
                }
                push @$acc,
                    Affix::Bind::Enum->new(
                    name      => $1,
                    constants => \@cs,
                    file      => $f,
                    line      => $self->_ln( $c, $s ),
                    end_line  => $self->_ln( $c, $e ),
                    doc       => $self->_doc( $c, $s )
                    );
            }
        }

        method _scan_funcs( $f, $acc ) {
            my $c = $self->_read($f);
            while ( $c
                =~ /^\s*((?:const\s+|unsigned\s+|struct\s+|[\w:<>]+(?:\s*::\s*[\w:<>]+)*\s*\*?\s*)+?)\s*(\w+)\s*(\((?:[^()]++|(?3))*\))(?:\s*;|\s*\{)/gm
            ) {
                next if $2 =~ /^(if|while|for|return|switch)$/ || $1 =~ /static/;
                my $s    = $-[0];
                my $e    = $+[0];
                my $args = substr( $3, 1, -1 );
                my @a    = grep {length} map { s/^\s+|\s+$//g; $_ } split /,/, $args;
                push @$acc,
                    Affix::Bind::Function->new(
                    name     => $2,
                    ret      => $1,
                    args     => \@a,
                    file     => $f,
                    line     => $self->_ln( $c, $s ),
                    end_line => $self->_ln( $c, $e ),
                    doc      => $self->_doc( $c, $s )
                    );
            }
        }

        method _mem($b) {
            my @m;
            my $pending_doc = '';

            # Helper to clean docs
            my $clean = sub ($t) {
                $t =~ s/^\s*\/\*\*?//mg;
                $t =~ s/\s*\*\/$//mg;
                $t =~ s/^\s*\*\s?//mg;
                $t =~ s/^\s*\/\/\s?//mg;
                $t =~ s/^\s+|\s+$//g;
                return length($t) ? $t : undef;
            };
            while ( length($b) > 0 ) {

                # 1. Skip Whitespace
                if ( $b =~ s/^(\s+)// ) { next; }

                # 2. Capture Comments (C++ style)
                if ( $b =~ s|^(//(.*?)\n)|| ) {
                    $pending_doc .= "$2\n";
                    next;
                }

                # 3. Capture Comments (C style)
                if ( $b =~ s|^(\s*/\*(.*?)\*/)||s ) {
                    $pending_doc .= $2;
                    next;
                }

                # 4. Match Nested Union/Struct
                if ( $b =~ s/^\s*(union|struct)\s*(\{(?:[^{}]++|(?2))*\})\s*(\w+)\s*;// ) {
                    my $d = Affix::Bind::Struct->new(
                        name     => '',
                        tag      => $1,
                        members  => $self->_mem( substr( $2, 1, -1 ) ),
                        file     => '',
                        line     => 0,
                        end_line => 0,
                        doc      => undef
                    );
                    push @m, Affix::Bind::Member->new( name => $3, definition => $d, doc => $clean->($pending_doc) );
                    $pending_doc = '';
                    next;
                }

                # 5. Match Standard Field
                if ( $b =~ s/^\s*([\w\s\*]+?)\s+(\w+(?:\[.*?\])?)\s*;// ) {
                    my ( $t, $n ) = ( $1, $2 );
                    $t =~ s/^\s+|\s+$//g;
                    if ( $n =~ s/(\[.*\])$// ) { $t .= $1 }
                    push @m, Affix::Bind::Member->new( name => $n, type => $t, doc => $clean->($pending_doc) );
                    $pending_doc = '';
                    next;
                }

                # 6. Safety advance if nothing matched (e.g. junk or preprocessor in struct)
                substr( $b, 0, 1 ) = "";
                $pending_doc = '';
            }
            return \@m;
        }
        method _ln( $c, $o ) { ( substr( $c, 0, $o ) =~ tr/\n// ) + 1 }

        method _doc( $c, $o ) {
            return undef if $o == 0;
            my @l = split /\n/, substr( $c, 0, $o );
            my @d;
            my $cap = 0;
            while ( my $l = pop @l ) {
                next if !$cap && $l =~ /^\s*$/;
                if    ( $l =~ /\*\// )    { $cap = 1 }
                elsif ( $l =~ m{^\s*//} ) { $cap = 1 }
                if    ($cap)              { unshift @d, $l; last if $l =~ /\/\*/; last if $l =~ m{^\s*//} && ( !@l || $l[-1] !~ m{^\s*//} ); }
                else                      {last}
            }
            return undef unless @d;
            my $t = join "\n", @d;
            $t =~ s/^\s*(\/\*+|\*+\/|\*|\/\/)\s?//mg;
            $t =~ s/^\s+|\s+$//g;
            return $t;
        }
    }

    # =============================================================================
    # MAIN CLASS
    # =============================================================================
    class Affix::Bind {
        field $driver;
        field $project_files : param;
        field $include_dirs : param //= [];
        ADJUST {
            my $has_clang = 0;
            my ( $out, $err, $exit ) = Capture::Tiny::capture { system( 'clang', '--version' ); };
            $has_clang = 1 if $exit == 0;
            if ($has_clang) {
                $driver = Affix::Bind::Driver::Clang->new( project_files => $project_files );
            }
            else {
                $driver = Affix::Bind::Driver::Regex->new( project_files => $project_files );
            }
        }

        method parse( $entry_point = undef ) {

            # Default to the first project file if no entry point provided
            $entry_point //= $project_files->[0];
            return $driver->parse( $entry_point, $include_dirs );
        }

        sub _affix_type ($t) {
            $t =~ s/__attribute__//;
            return 'Void' unless defined $t;
            return '' if $t eq '*';    # Remnant from function typedef
            state $re //= qr[
    (?(DEFINE)
        (?<PARENS> \( (?: [^()]+ | (?&PARENS) )* \) )
    )
    ^ \s* (?: [^()]+ | (?&PARENS) )+ \s* $
]x;
            state $type_map //= {

                # Primitives (mostly)
                void                 => 'Void',
                bool                 => 'Bool',
                short                => 'Short',
                'unsigned short'     => 'UShort',
                char                 => 'Char',
                'signed char'        => 'SChar',
                'unsigned char'      => 'UChar',
                int                  => 'Int',
                'unsigned int'       => 'UInt',
                long                 => 'Long',
                'unsigned long'      => 'ULong',
                'long long'          => 'LongLong',
                'unsigned long long' => 'ULongLong',
                float                => 'Float',
                double               => 'Double',
                'long double'        => 'LongDouble',
                #
                int8        => 'Int8',
                sint8       => 'SInt8',
                uint8       => 'UInt8',
                int16       => 'Int16',
                sint16      => 'SInt16',
                uint16      => 'UInt16',
                int32       => 'Int32',
                sint32      => 'SInt32',
                uint32      => 'UInt32',
                int64       => 'Int64',
                sint64      => 'SInt64',
                uint64      => 'UInt64',
                int128      => 'Int128',
                sint128     => 'SInt128',
                uint128     => 'UInt128',
                __int128_t  => 'Int128',
                __uint128_t => 'UInt128',
                #
                size_t                 => 'Size_t',
                wchar_t                => 'WChar',
                '...'                  => 'VarArgs',    # unhandled for now
                'va_list'              => 'VarArgs',    # unhandled for now
                'struct __va_list_tag' => 'VarArgs',    # unhandled for now

                # Platform dependant
                #'void *restrict'       => 'Pointer[Void]',
                #'char *restrict'       => 'Pointer[Char]',
                'struct __locale_data' => 'Struct[__locale_data => Pointer[Void]]',
                locale_t               => 'Struct[__locale_t => Pointer[Void]]',
                wint_t                 => 'WChar'
            };

            # Cleanup
            $t =~ s/consts?\s+//g;
            $t =~ s/(\s+\**)const$/$1/g;
            $t =~ s/(\s+\**)restrict$/$1/g;
            $t =~ s/\s+$//;
            #
            return $type_map->{ lc $t }               if defined $type_map->{ lc $t };
            return 'Pointer[' . _affix_type($1) . ']' if $t =~ /^(.+)\s*\*$/;
            state $grammar //= qr{
    (?(DEFINE)
        (?<PARENS>
            \(
            (?:
                [^()]+        # Match normal characters
                |
                (?&PARENS)    # Match nested parentheses recursively
            )*
            \)
        )
    )
}x;

            # Splits return value from args
            state $splitter //= qr{
    $grammar
    ^
    (?<RETURN_TYPE> .* )        # Capture everything...
    \s*
    (?<ARG_BLOCK> (?&PARENS) )  # ...until the final balanced group
    \s*
    $
}x;

            # Splits comma seperated list of args while being aware of nested groups for things like function pointers as arg types
            state $tokenizer //= qr{
    $grammar
    \G\s*                       # Continue from previous match, skip whitespace
    (?<ARG>                     # Capture the argument
        (?:
            [^(),]+             # Text that isn't commas or parens
            |
            (?&PARENS)          # Balanced parens (commas allowed inside here!)
        )+
    )
    \s*
    (?: , | $ )                 # Stop at comma or end of string
}x;

            # Apply above grammar
            if ( $t =~ $splitter ) {
                my $ret       = '';
                my $ret_type  = $+{RETURN_TYPE};
                my $arg_block = $+{ARG_BLOCK};

                # Trim return type
                $ret_type =~ s/^\s+|\s+$//g;

                # Remove the outer parentheses (trust fall that clang never changes...)
                my $inner_args = substr( $arg_block, 1, -1 );

                # Tokenize the list
                my $count = 0;
                my @args;
                while ( $inner_args =~ /$tokenizer/g ) {
                    my $arg = $+{ARG};
                    $arg =~ s/^\s+|\s+$//g;    # Trim
                    $count++;
                    push @args, $arg;
                }
                return sprintf 'Callback[ [%s] => %s]', join( ', ', map { _affix_type($_) } @args ), _affix_type($ret_type);
            }

            # Structs by value or typedefs
            return $2 . '()'                                                                  if $t =~ /^(struct\s+)?(SDL_.*)$/;
            return sprintf( 'Array[%s%s]', _affix_type($1), ( defined $2 ? ', ' . $2 : '' ) ) if $t =~ /^(.+)\[(\d+)\]$/;

            # If we still see unnamed types here, the parser logic failed to link the RecordDecl in parse_record_fields
            die 'FATAL: map_type received an raw anonymous type: ' . $t if $t =~ /unnamed/;

            #warn '** Unknown type: ' . $t;    # fallback
            return '';
        }
    }
};
1
