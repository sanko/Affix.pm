MODULE = Dyn::Call   PACKAGE = Dyn::Call::Value

=pod

=encoding utf-8

=head1 NAME

Dyn::Call::Value - dyncall value variant

=head1 DESCRIPTION

Value variant union-type that carries all supported dyncall types.

=head1 Methods

This package is object oriented and brings along the folloiwng methods...

=head2 C<new( )>

    my $value = Dyn::Call::Value->new();

Generates a new Dyn::Call::Value object.

=head2 C<B( [...] )>

    if ( $value->B ) { ... }
    $value->B( !0 );

Gets and potentially sets the boolean value of the underlying union.

=head2 C<c( [...] )>

    if ( $value->c ) { ... }
    $value->c( ord 'a' );

Gets and potentially sets the char value of the underlying union.

=head2 C<C( [...] )>

    if ( $value->C ) { ... }
    $value->C( ord 'a' );

Gets and potentially sets the unsigned char value of the underlying union.

=head2 C<s( [...] )>

    if ( $value->s == -5 ) { ... }
    $value->s( -16 );

Gets and potentially sets the short value of the underlying union.

=head2 C<S( [...] )>

    if ( $value->S > 3 ) { ... }
    $value->S( 44 );

Gets and potentially sets the unsigned short value of the underlying union.


=head1 LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under
the terms found in the Artistic License 2. Other copyrights, terms, and
conditions may apply to data transmitted through this module.

=head1 AUTHOR

Sanko Robinson E<lt>sanko@cpan.orgE<gt>

=begin stopwords

dyncall

=end stopwords

=cut

SV *
new(const char * package)
CODE:
    {
        RETVAL = newSV(0);
        DCValue * dcv;
        Newxz(dcv, 1, DCValue);
        sv_setref_pv(RETVAL, package, (DCpointer)dcv);
    }
OUTPUT:
    RETVAL

SV *
_fetch(SV * me, SV * new_value = NULL)
ALIAS:
    B= DC_SIGCHAR_BOOL
    c= DC_SIGCHAR_CHAR
    C= DC_SIGCHAR_UCHAR
    s= DC_SIGCHAR_SHORT
    S= DC_SIGCHAR_USHORT
    i= DC_SIGCHAR_INT
    I= DC_SIGCHAR_UINT
    j= DC_SIGCHAR_LONG
    J= DC_SIGCHAR_ULONG
    l= DC_SIGCHAR_LONGLONG
    L= DC_SIGCHAR_ULONGLONG
    f= DC_SIGCHAR_FLOAT
    d= DC_SIGCHAR_DOUBLE
    p= DC_SIGCHAR_POINTER
    Z= DC_SIGCHAR_STRING
CODE:
    DCValue * value;
    if (sv_derived_from(ST(0), "Dyn::Call::Value")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        value = INT2PTR(DCValue *, tmp);
    }
    else
        croak("value is not of type Dyn::Call::Value");
    switch(ix) {
        case DC_SIGCHAR_BOOL:
            if(new_value) value->B = SvTRUE(new_value);
            RETVAL = boolSV(value->B);





        case DC_SIGCHAR_INT:
            if(new_value) value->i = SvIV(new_value);
            RETVAL = newSViv(value->i);
            break;
      default:
          croak("Dyn::Call::Value has no field %c", ix);
            break;
    }
OUTPUT:
    RETVAL

void
DESTROY(SV * me)
CODE:
    DCValue * value;
    if (sv_derived_from(ST(0), "Dyn::Call::Value")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        value = INT2PTR(DCValue *, tmp);
    }
    else
        croak("value is not of type Dyn::Call::Value");
    safefree(value);