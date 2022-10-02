MODULE = Dyn::Call  PACKAGE = Dyn::Call

DCpointer
malloc(size_t size)
CODE:
    RETVAL = safemalloc( size );
    if ( RETVAL == NULL ) XSRETURN_EMPTY;
OUTPUT:
    RETVAL

DCpointer
calloc(size_t num, size_t size)
CODE:
    RETVAL = safecalloc( num, size );
    if ( RETVAL == NULL ) XSRETURN_EMPTY;
OUTPUT:
    RETVAL

DCpointer
realloc(IN_OUT DCpointer ptr, size_t size)
CODE:
    ptr = saferealloc(ptr, size);
OUTPUT:
    RETVAL

void
free(DCpointer ptr)
PPCODE:
    if (ptr != NULL) dcFreeMem(ptr);
    ptr = NULL;
    sv_set_undef(ST(0)); // Let Dyn::Call::Pointer::DESTROY take care of the rest

DCpointer
memchr( DCpointer ptr, char ch, size_t count )

int
memcmp( lhs, rhs, size_t count )
INIT:
    DCpointer lhs, rhs;
CODE:
    if (sv_derived_from(ST(0), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        lhs = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(0))) {
        IV tmp = SvIV((SV*)(ST(0)));
        lhs = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Dyn::Call::Pointer");

    if (sv_derived_from(ST(1), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(1)));
        rhs = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(1))) {
        IV tmp = SvIV((SV*)(ST(1)));
        rhs = INT2PTR(DCpointer, tmp);
    }
    else if(SvPOK(ST(1))) {
        rhs = (DCpointer)(unsigned char *)SvPV_nolen(ST(1));
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");
    RETVAL = memcmp(lhs, rhs, count);
OUTPUT:
    RETVAL

DCpointer
memset( DCpointer dest, char ch, size_t count )

void
memcpy(dest, src, size_t nitems)
INIT:
    DCpointer dest, src;
PPCODE:
    if (sv_derived_from(ST(0), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(0))) {
        IV tmp = SvIV((SV*)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");

    if (sv_derived_from(ST(1), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(1))) {
        IV tmp = SvIV((SV*)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if(SvPOK(ST(1))) {
        src = (DCpointer)(unsigned char *)SvPV_nolen(ST(1));
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");
    Copy(src, dest, nitems, char);

void
memmove(dest, src, size_t nitems)
INIT:
    DCpointer dest, src;
PPCODE:
    if (sv_derived_from(ST(0), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(0))) {
        IV tmp = SvIV((SV*)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");

    if (sv_derived_from(ST(1), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(1))) {
        IV tmp = SvIV((SV*)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if(SvPOK(ST(1))) {
        src = (DCpointer)(unsigned char *)SvPV_nolen(ST(1));
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");
    Move(src, dest, nitems, char);

BOOT:
    export_function("Dyn::Call", "malloc",  "memory");
    export_function("Dyn::Call", "calloc",  "memory");
    export_function("Dyn::Call", "realloc", "memory");
    export_function("Dyn::Call", "free",    "memory");
    export_function("Dyn::Call", "memchr",  "memory");
    export_function("Dyn::Call", "memcmp",  "memory");
    export_function("Dyn::Call", "memset",  "memory");
    export_function("Dyn::Call", "memcpy",  "memory");
    export_function("Dyn::Call", "memmove", "memory");

MODULE = Dyn::Call  PACKAGE = Dyn::Call::Pointer

FALLBACK: TRUE

IV
plus(DCpointer ptr, IV other, IV swap)
OVERLOAD: +
CODE:
    RETVAL = PTR2IV(ptr) + other;
OUTPUT:
    RETVAL

IV
minus(DCpointer ptr, IV other, IV swap)
OVERLOAD: -
CODE:
    RETVAL = PTR2IV(ptr) - other;
OUTPUT:
    RETVAL

char *
as_string(DCpointer ptr, ...)
OVERLOAD: \"\"
CODE:
    RETVAL = (char *)ptr;
OUTPUT:
    RETVAL

SV *
raw(ptr, size_t size)
CODE:
    DCpointer ptr;
    if (sv_derived_from(ST(0), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(0))) {
        IV tmp = SvIV((SV*)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");
    RETVAL = newSVpvn_utf8((const char *) ptr, size, 1);
OUTPUT:
    RETVAL

void
dump(ptr, size_t size)
CODE:
    DCpointer ptr;
    if (sv_derived_from(ST(0), "Dyn::Call::Pointer")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else if(SvIOK(ST(0))) {
        IV tmp = SvIV((SV*)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Dyn::Call::Pointer");
    DumpHex(ptr, size);
