package Affix::Bind v1.0.0 {
    use v5.40;
    use feature 'class';
    no warnings 'experimental::class';
    no warnings 'experimental::builtin';
    use Path::Tiny;
    use Capture::Tiny qw[capture];
    use JSON::PP;
    use File::Basename qw[basename];
    use Affix          qw[];

    class Affix::Bind::Type {
        use Affix qw[Void];
        field $name : reader : param //= 'void';
        method to_string { $self->name }
        use overload '""' => 'to_string', fallback => 1;

        # Factory method to parse a C type string into objects
        sub parse ( $class, $t ) {
            return $class->new( name => 'void' ) unless defined $t;

            # Cleanup attributes and whitespace
            $t =~ s/__attribute__\s*\(\(.*\)\)//g;
            $t =~ s/^\s+|\s+$//g;

            # Recursively parse array syntax: "Type [N]"
            if ( $t =~ /^(.*)\s*\[(\d+)\]$/ ) {
                return Affix::Bind::Type::Array->new( of => $class->parse($1), count => $2 );
            }

            # Handle Pointers: "Type *"
            # Clean up trailing qualifiers on the pointer itself (e.g., "int * const" -> "int *")
            $t =~ s/(\*)\s*(?:const|restrict)\s*$/$1/;
            if ( $t =~ /^(.+)\s*\*$/ ) {
                return Affix::Bind::Type::Pointer->new( of => $class->parse($1) );
            }

            # Base case: Concrete type
            return $class->new( name => $t );
        }

        # Returns the Affix signature string (e.g. "Int") for code generation
        method affix_type {
            my $t = $self->name;

            # Clean up const/restrict for map lookup
            $t =~ s/consts?\s+//g;
            $t =~ s/(\s+\**)const$/$1/g;
            $t =~ s/(\s+\**)restrict$/$1/g;
            $t =~ s/\s+$//;
            state $type_map //= {
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
                int8                 => 'Int8',
                sint8                => 'SInt8',
                uint8                => 'UInt8',
                int16                => 'Int16',
                sint16               => 'SInt16',
                uint16               => 'UInt16',
                int32                => 'Int32',
                sint32               => 'SInt32',
                uint32               => 'UInt32',
                int64                => 'Int64',
                sint64               => 'SInt64',
                uint64               => 'UInt64',
                int128               => 'Int128',
                sint128              => 'SInt128',
                uint128              => 'UInt128',
                size_t               => 'Size_t',
                wchar_t              => 'WChar',
                '...'                => 'VarArgs',
                'va_list'            => 'VarArgs',
            };

            # Check map
            return $type_map->{ lc $t } if defined $type_map->{ lc $t };

            # Fallback for specific known structs (legacy support)
            return $2 . '()' if $t =~ /^(struct\s+)?(SDL_.*)$/;
            return 'Void';
        }

        # Calls the actual Affix function (e.g. Affix::Int())
        method affix {
            my $func_name = $self->affix_type;

            # If the name is something complex like "SDL_Rect()", we assume it's a custom function call
            # For standard types, it's just "Int", "Void", etc.
            if ( $func_name =~ /^(\w+)$/ ) {
                no strict 'refs';
                my $fn = "Affix::$func_name";
                return $fn->() if defined &{$fn};
            }

            # Fallback: if we can't execute it (e.g. unknown struct), return Void()
            Void;
        }
    }

    class Affix::Bind::Type::Pointer : isa(Affix::Bind::Type) {
        use Affix qw[Pointer];
        field $of : reader : param;
        method name       { $of->name . '*' }
        method affix_type { 'Pointer[' . $of->affix_type . ']' }
        method affix      { Pointer [ $of->affix ] }
    }

    class Affix::Bind::Type::Array : isa(Affix::Bind::Type) {
        use Affix qw[Array];
        field $of    : reader : param;    # Affix::Bind::Type
        field $count : reader : param;    # Integer
        method name       { $of->name . "[" . $count . "]" }
        method affix_type { sprintf( 'Array[%s, %d]', $of->affix_type, $count ) }
        method affix      { Array [ $of->affix, $count ] }
    }

    class Affix::Bind::Argument {
        field $type : reader : param;     # Affix::Bind::Type
        field $name : reader : param //= '';
        method to_string { length($name) ? $type->to_string . " " . $name : $type->to_string }
        use overload '""' => 'to_string', fallback => 1;
        method affix_type { $type->affix_type }
        method affix      { $type->affix }
    }

    class Affix::Bind::Entity {
        field $name         : reader : param //= '';
        field $doc          : reader : param //= ();
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
        field $type       : reader : param //= '';      # Affix::Bind::Type
        field $doc        : reader : param //= undef;
        field $definition : reader : param //= undef;
        method affix_type { $type->affix_type }
        method affix      { $type->affix }
    }

    class Affix::Bind::Macro : isa(Affix::Bind::Entity) {
        field $value : reader : param //= '';
    }

    class Affix::Bind::Variable : isa(Affix::Bind::Entity) {
        field $type : reader : param;                   # Affix::Bind::Type
        method affix_type { $type->affix_type }
        method affix      { $type->affix }
    }

    class Affix::Bind::Typedef : isa(Affix::Bind::Entity) {
        field $underlying : reader : param;             # Affix::Bind::Type
        method affix_type { $underlying->affix_type }
        method affix      { $underlying->affix }
    }

    class Affix::Bind::Struct : isa(Affix::Bind::Entity) {
        field $tag     : reader : param //= 'struct';
        field $members : reader : param //= [];
    }

    class Affix::Bind::Enum : isa(Affix::Bind::Entity) {
        field $constants : reader : param //= [];
    }

    class Affix::Bind::Function : isa(Affix::Bind::Entity) {
        field $ret          : reader : param;           # Affix::Bind::Type
        field $args         : reader : param //= [];    # List of Affix::Bind::Argument
        field $mangled_name : reader : param //= ();
        method affix_ret { $ret->affix_type }

        method affix_args {
            [ map { $_->affix_type } @$args ]
        }

        # Returns the actual Affix::Type object for return value
        method call_ret { $ret->affix }

        # Returns a list of actual Affix::Type objects for arguments
        method call_args {
            [ map { $_->affix } @$args ]
        }
    }

    # The good one
    class Affix::Bind::Driver::Clang {
        field $project_files : param;
        field $allowed_files  = {};
        field $project_dirs   = [];
        field $paths_seen     = {};
        field $file_cache     = {};
        field $last_seen_file = undef;
        field $clang //= 'clang';

        method _basename ($path) {
            return "" unless defined $path;
            $path =~ s{^.*[/\\]}{};
            return lc($path);
        }

        # Helper to absolute paths and normalize slashes
        method _normalize ($path) {
            return "" unless defined $path;
            my $abs = Path::Tiny::path($path)->absolute->stringify;
            $abs =~ s{\\}{/}g;
            return $abs;
        }
        ADJUST {
            my %seen_dirs;
            for my $f (@$project_files) {
                my $abs = $self->_normalize($f);
                $allowed_files->{$abs} = 1;

                # Track project directory roots
                my $dir = Path::Tiny::path($abs)->parent->stringify;
                $dir =~ s{\\}{/}g;
                unless ( $seen_dirs{$dir}++ ) {
                    push @$project_dirs, $dir;
                }
            }
        }

        method parse ( $entry_point, $include_dirs //= [] ) {
            my $ep_abs = $self->_normalize($entry_point);

            # Ensure entry point is allowed
            $allowed_files->{$ep_abs} = 1;

            # Initialize state
            $last_seen_file = $ep_abs;

            # Ensure entry point directory is in allowed dirs
            my $ep_dir = Path::Tiny::path($ep_abs)->parent->stringify;
            $ep_dir =~ s{\\}{/}g;
            my $found = 0;
            for my $pd (@$project_dirs) {
                if ( $ep_dir eq $pd ) { $found = 1; last; }
            }
            push @$project_dirs, $ep_dir unless $found;
            my @includes = map { "-I" . $self->_normalize($_) } @$include_dirs;
            for my $d (@$project_dirs) {
                push @includes, "-I$d";
            }
            my @cmd = (
                $clang,                           '-Xclang',       '-ast-dump=json',       '-Xclang',
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
            @objects;
        }

        method _walk( $node, $acc, $current_file ) {
            return unless ref $node eq 'HASH';
            my $kind      = $node->{kind} // 'Unknown';
            my $node_file = $self->_get_node_file($node);

            # Context Management:
            # Clang's JSON format omits the 'file' property if it's the same as the
            # immediately preceding node (serialization optimization).
            # We track $last_seen_file to handle this.
            if ($node_file) {
                $current_file   = $self->_normalize($node_file);
                $last_seen_file = $current_file;
            }
            elsif ( defined $last_seen_file ) {
                $current_file = $last_seen_file;
            }

            # Filter: Check if file is system or external
            if ( $self->_is_valid_file($current_file) && !$node->{isImplicit} ) {
                warn $current_file;
                if ( $kind eq 'MacroDefinitionRecord' ) {
                    if ( $node->{range} ) { $self->_macro( $node, $acc, $current_file ); }
                }
                elsif ( $kind eq 'TypedefDecl' ) {
                    $self->_typedef( $node, $acc, $current_file );
                }
                elsif ( $kind eq 'RecordDecl' || $kind eq 'CXXRecordDecl' ) {
                    $self->_record( $node, $acc, $current_file );
                    return;
                }
                elsif ( $kind eq 'EnumDecl' ) {
                    $self->_enum( $node, $acc, $current_file );
                    return;
                }
                elsif ( $kind eq 'VarDecl' ) {
                    if ( ( $node->{storageClass} // '' ) ne 'static' ) {
                        $self->_var( $node, $acc, $current_file );
                    }
                }
                elsif ( $kind eq 'FunctionDecl' ) {
                    $self->_func( $node, $acc, $current_file );
                    return;
                }
                elsif ( $kind eq 'BuiltinType' ) {
                    return;
                }
            }
            if ( $node->{inner} ) {
                for ( @{ $node->{inner} } ) { $self->_walk( $_, $acc, $current_file ); }
            }
        }

        # Filter out system headers and non-project files
        method _is_valid_file ($f) {
            return 0 unless defined $f && length $f;

            # System Header Guard (Prioritized)
            return 0 if $f =~ m{^/usr/(include|lib|share|local/include)};
            return 0 if $f =~ m{^/System/Library};
            return 0 if $f =~ m{(Program Files|Strawberry|MinGW|Windows|cygwin|msys)}i;

            # Explicit Whitelist check
            return 1 if $allowed_files->{$f};

            # Project Directory check
            for my $dir (@$project_dirs) {
                return 1 if index( $f, $dir ) == 0;
            }
            return 0;
        }

        method _get_node_file($node) {
            my $loc = $node->{loc};
            return undef unless $loc;
            my $f;
            if ( ref($loc) eq 'HASH' ) {
                $f = $loc->{presumedLoc}{file} || $loc->{expansionLoc}{file} || $loc->{spellingLoc}{file} || $loc->{file};
            }
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
                underlying   => Affix::Bind::Type->parse( $n->{type}{qualType} ),
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
                    my $name     = $child->{name} // '';
                    my $raw_type = $child->{type}{qualType};
                    my $type_obj = Affix::Bind::Type->parse($raw_type);
                    my $def      = undef;
                    if (@pending_anonymous_records) {
                        $def = pop @pending_anonymous_records;
                    }
                    my $f_offset = $child->{range}{begin}{offset};
                    my $f_doc    = $self->_extract_doc( $f, $f_offset );
                    push @members, Affix::Bind::Member->new( name => $name, type => $type_obj, doc => $f_doc, definition => $def );
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
                type         => Affix::Bind::Type->parse( $n->{type}{qualType} ),
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }

        method _func( $n, $acc, $f ) {
            return if ( $n->{storageClass} // '' ) eq 'static';
            my ( $s, $e, $l, $el ) = $self->_meta($n);
            my $ret_str = $n->{type}{qualType};
            $ret_str =~ s/\(.*\)//;
            my $ret_obj = Affix::Bind::Type->parse($ret_str);
            my @args;
            for ( @{ $n->{inner} } ) {
                if ( ( $_->{kind} // '' ) eq 'ParmVarDecl' ) {
                    my $pt = Affix::Bind::Type->parse( $_->{type}{qualType} );
                    my $pn = $_->{name} // '';
                    push @args, Affix::Bind::Argument->new( type => $pt, name => $pn );
                }
            }
            push @$acc,
                Affix::Bind::Function->new(
                name         => $n->{name},
                mangled_name => $n->{mangledName},
                file         => $f,
                line         => $l,
                end_line     => $el,
                ret          => $ret_obj,
                args         => \@args,
                doc          => $self->_extract_doc( $f, $s ),
                start_offset => $s,
                end_offset   => $e
                );
        }
        #
        method _get_content($f) {

            # $f should already be normalized from _walk, but redundancy is safe
            my $abs = $self->_normalize($f);
            return $file_cache->{$abs} if exists $file_cache->{$abs};
            if ( -e $abs ) {
                return $file_cache->{$abs} = Path::Tiny::path($abs)->slurp_utf8;
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
            '';
        }

        method _scan_macros_fallback($acc) {
            my %seen = map { $_->name => 1 } grep { ref($_) eq 'Affix::Bind::Macro' } @$acc;
            for my $f ( keys %$allowed_files ) {

                # Respect filtering even here (though allowed_files contains filtered files?)
                # Actually allowed_files contains explicit inputs. Check is_valid again if needed,
                # but explicit inputs are filtered inside is_valid only if they are system paths.
                next unless $self->_is_valid_file($f);
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
            for my $td (@tds) {
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

    # The fallback
    class Affix::Bind::Driver::Regex {
        field $project_files : param;
        field $file_cache = {};

        method _normalize ($path) {
            return "" unless defined $path;
            my $abs = Path::Tiny::path($path)->absolute->stringify;
            $abs =~ s{\\}{/}g;
            return $abs;
        }

        # Filter out system headers (mirrors Clang logic)
        method _is_valid_file ($f) {
            return 0 unless defined $f && length $f;

            # System Header Guard
            return 0 if $f =~ m{^/usr/(include|lib|share|local/include)};
            return 0 if $f =~ m{^/System/Library};
            return 0 if $f =~ m{(Program Files|Strawberry|MinGW|Windows|cygwin|msys)};

            # Regex driver currently iterates only provided files, so we permit all others
            return 1;
        }

        method parse( $entry_point, $ids = [] ) {
            my @objs;
            for my $f (@$project_files) {

                # Use normalization and filtering
                my $abs = $self->_normalize($f);
                next unless $self->_is_valid_file($abs);
                if ( $f =~ /\.h(pp|xx)?$/i ) { $self->_scan( $f, \@objs ); $self->_scan_funcs( $f, \@objs ); }
                else                         { $self->_scan_funcs( $f, \@objs ); }
            }
            @objs = sort { ( $a->file cmp $b->file ) || ( $a->start_offset <=> $b->start_offset ) } @objs;
            @objs;
        }

        # Use cache if available, though _scan calls _read repeatedly with filename
        # We can implement simple slurp or use caching wrapper
        method _read($f) {
            my $abs = $self->_normalize($f);
            return $file_cache->{$abs} if exists $file_cache->{$abs};
            return $file_cache->{$abs} = Path::Tiny::path($f)->slurp_utf8;
        }

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
                my $s        = $-[0];
                my $e        = $+[0];
                my $ret_str  = $1;
                my $ret_obj  = Affix::Bind::Type->parse($ret_str);
                my $args_str = substr( $3, 1, -1 );
                my @args_raw = grep {length} map { s/^\s+|\s+$//g; $_ } split /,/, $args_str;
                my @args;

                for my $raw (@args_raw) {
                    if ( $raw =~ /^(.+?)\s+(\w+(?:\[.*?\])?)$/ ) {
                        my ( $t, $n ) = ( $1, $2 );
                        if ( $n =~ s/(\[.*\])$// ) { $t .= $1 }
                        push @args, Affix::Bind::Argument->new( type => Affix::Bind::Type->parse($t), name => $n );
                    }
                    else {
                        push @args, Affix::Bind::Argument->new( type => Affix::Bind::Type->parse($raw) );
                    }
                }
                push @$acc,
                    Affix::Bind::Function->new(
                    name     => $2,
                    ret      => $ret_obj,
                    args     => \@args,
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

                # Skip Whitespace
                if ( $b =~ s/^(\s+)// ) { next; }

                # Capture Comments (C++ style)
                if ( $b =~ s|^(//(.*?)\n)|| ) {
                    $pending_doc .= "$2\n";
                    next;
                }

                # Capture Comments (C style)
                if ( $b =~ s|^(\s*/\*(.*?)\*/)||s ) {
                    $pending_doc .= $2;
                    next;
                }

                # Match Nested Union/Struct
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
                if ( $b =~ s/^\s*([\w\s\*]+?)\s+(\w+(?:\[.*?\])?)\s*;// ) {
                    my ( $t, $n ) = ( $1, $2 );
                    $t =~ s/^\s+|\s+$//g;
                    if ( $n =~ s/(\[.*\])$// ) { $t .= $1 }
                    my $type_obj = Affix::Bind::Type->parse($t);
                    push @m, Affix::Bind::Member->new( name => $n, type => $type_obj, doc => $clean->($pending_doc) );
                    $pending_doc = '';
                    next;
                }
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
            $entry_point //= $project_files->[0];
            return $driver->parse( $entry_point, $include_dirs );
        }
    }
};
1;
