#include "../../Affix.h"

XS_INTERNAL(Affix_Type_marshal) {
    dVAR;
    dXSARGS;
    PING;
    if (items != 2) croak_xs_usage(cv, "type, data");
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Type"))) croak("type is not of type Affix::Type");
    sv_dump(ST(0));
    DCpointer RETVAL = sv2ptr(aTHX_ ST(0), ST(1));
    // DumpHex(RETVAL, _sizeof(aTHX_ ST(0)));
    PING;
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_unmarshal) {
    dVAR;
    dXSARGS;
    PING;

    if (UNLIKELY(items != 2)) croak_xs_usage(cv, "type, ptr");
    DCpointer ptr;
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Type"))) croak("type is not of type Affix::Type");
    if (UNLIKELY(!sv_derived_from(ST(1), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(1)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp));
    ST(0) = // sv_2mortal(
        newRV(ptr2sv(aTHX_ ptr, ST(0)))
        //)
        ;
    // ST(0) = ptr2sv(aTHX_ ptr, ST(0));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_plus) {
    dVAR;
    dXSARGS;
    PING;

    if (UNLIKELY(items != 3)) croak_xs_usage(cv, "ptr, other, swap");
    DCpointer ptr;
    IV other = (IV)SvIV(ST(1));
    // IV swap = (IV)SvIV(ST(2));
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(0)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp) + other);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", ptr);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_minus) {
    dVAR;
    dXSARGS;
    PING;

    if (UNLIKELY(items != 3)) croak_xs_usage(cv, "ptr, other, swap");
    DCpointer ptr;
    IV other = (IV)SvIV(ST(1));
    //~ IV swap = (IV)SvIV(ST(2));
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(0)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp) - other);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", ptr);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_malloc) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 1) croak_xs_usage(cv, "size");

    DCpointer RETVAL;
    size_t size = (size_t)SvUV(ST(0));
    {
        RETVAL = safemalloc(size);
        if (RETVAL == NULL) XSRETURN_EMPTY;
    }
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_calloc) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 2) croak_xs_usage(cv, "num, size");

    DCpointer RETVAL;
    size_t num = (size_t)SvUV(ST(0));
    size_t size = (size_t)SvUV(ST(1));
    {
        RETVAL = safecalloc(num, size);
        if (RETVAL == NULL) XSRETURN_EMPTY;
    }
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_realloc) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 2) croak_xs_usage(cv, "ptr, size");
    DCpointer ptr;
    size_t size = (size_t)SvUV(ST(1));
    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    ptr = saferealloc(ptr, size);
    sv_setref_pv(ST(0), "Affix::Pointer:Unmanaged", ptr);
    SvSETMAGIC(ST(0));
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", ptr);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_free) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 1) croak_xs_usage(cv, "ptr");
    SP -= items;

    DCpointer ptr;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    {
        if (ptr) {
            sv_set_undef(ST(0));
            SvSETMAGIC(ST(0));
        }
        sv_set_undef(ST(0));
        SvSETMAGIC(ST(0));
    } // Let Affix::Pointer::DESTROY take care of the rest
    PUTBACK;
    XSRETURN(1);
}

XS_INTERNAL(Affix_memchr) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 3) croak_xs_usage(cv, "ptr, ch, count");
    {
        DCpointer RETVAL;
        DCpointer ptr;
        char ch = (char)*SvPV_nolen(ST(1));
        size_t count = (size_t)SvUV(ST(2));

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            ptr = INT2PTR(DCpointer, tmp);
        }
        else
            croak("ptr is not of type Affix::Pointer");

        RETVAL = memchr(ptr, ch, count);
        {
            SV *RETVALSV;
            RETVALSV = sv_newmortal();
            sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
            ST(0) = RETVALSV;
        }
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memcmp) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 3) croak_xs_usage(cv, "lhs, rhs, count");
    {
        int RETVAL;
        dXSTARG;
        size_t count = (size_t)SvUV(ST(2));
        DCpointer lhs, rhs;
        {
            if (sv_derived_from(ST(0), "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(ST(0)));
                lhs = INT2PTR(DCpointer, tmp);
            }
            else if (SvIOK(ST(0))) {
                IV tmp = SvIV((SV *)(ST(0)));
                lhs = INT2PTR(DCpointer, tmp);
            }
            else
                croak("ptr is not of type Affix::Pointer");
            if (sv_derived_from(ST(1), "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(ST(1)));
                rhs = INT2PTR(DCpointer, tmp);
            }
            else if (SvIOK(ST(1))) {
                IV tmp = SvIV((SV *)(ST(1)));
                rhs = INT2PTR(DCpointer, tmp);
            }
            else if (SvPOK(ST(1))) { rhs = (DCpointer)(U8 *)SvPV_nolen(ST(1)); }
            else
                croak("dest is not of type Affix::Pointer");
            RETVAL = memcmp(lhs, rhs, count);
        }
        XSprePUSH;
        PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memset) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 3) croak_xs_usage(cv, "dest, ch, count");
    {
        DCpointer RETVAL;
        DCpointer dest;
        char ch = (char)*SvPV_nolen(ST(1));
        size_t count = (size_t)SvUV(ST(2));

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            dest = INT2PTR(DCpointer, tmp);
        }
        else
            croak("dest is not of type Affix::Pointer");

        RETVAL = memset(dest, ch, count);
        {
            SV *RETVALSV;
            RETVALSV = sv_newmortal();
            sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
            ST(0) = RETVALSV;
        }
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memcpy) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 3) croak_xs_usage(cv, "dest, src, nitems");
    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src, RETVAL;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Affix::Pointer");
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(1))) {
        IV tmp = SvIV((SV *)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvPOK(ST(1))) { src = (DCpointer)(U8 *)SvPV_nolen(ST(1)); }
    else
        croak("dest is not of type Affix::Pointer");
    RETVAL = CopyD(src, dest, nitems, char);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memmove) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 3) croak_xs_usage(cv, "dest, src, nitems");

    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src, RETVAL;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Affix::Pointer");
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(1))) {
        IV tmp = SvIV((SV *)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvPOK(ST(1))) { src = (DCpointer)(U8 *)SvPV_nolen(ST(1)); }
    else
        croak("dest is not of type Affix::Pointer");

    RETVAL = MoveD(src, dest, nitems, char);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_strdup) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 1) croak_xs_usage(cv, "str1");

    DCpointer RETVAL;
    char *str1 = (char *)SvPV_nolen(ST(0));

    RETVAL = strdup(str1);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_as_string) {
    dVAR;
    dXSARGS;
    PING;

    if (items < 1) croak_xs_usage(cv, "ptr, ...");
    {
        char *RETVAL;
        dXSTARG;
        DCpointer ptr;

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            ptr = INT2PTR(DCpointer, tmp);
        }
        else
            croak("ptr is not of type Affix::Pointer");
        RETVAL = (char *)ptr;
        sv_setpv(TARG, RETVAL);
        XSprePUSH;
        PUSHTARG;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_as_double) {
    dVAR;
    dXSARGS;
    PING;

    if (items < 1) croak_xs_usage(cv, "ptr, ...");

    double RETVAL;
    dXSTARG;
    DCpointer ptr;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    RETVAL = *(double *)ptr;
    sv_setnv_mg(TARG, RETVAL);
    XSprePUSH;
    PUSHTARG;

    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_as_int) {
    dVAR;
    dXSARGS;
    PING;

    if (items < 1) croak_xs_usage(cv, "ptr, ...");

    int RETVAL;
    dXSTARG;
    DCpointer ptr;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    RETVAL = *(int *)ptr;
    sv_setiv_mg(TARG, RETVAL);
    XSprePUSH;
    PUSHTARG;

    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_deref_scalar) {
    dVAR;
    dXSARGS;
    PING;

    if (items < 1) croak_xs_usage(cv, "ptr, ...");

    int RETVAL;
    DCpointer ptr;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    RETVAL = *(int *)ptr;
    ST(0) = newRV(newSViv(RETVAL));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_raw) {
    dVAR;
    dXSARGS;
    PING;

    if (items < 2 || items > 3) croak_xs_usage(cv, "ptr, size[, utf8]");
    {
        SV *RETVAL;
        size_t size = (size_t)SvUV(ST(1));
        bool utf8;

        if (items < 3)
            utf8 = false;
        else { utf8 = (bool)SvTRUE(ST(2)); }
        {
            DCpointer ptr;
            if (sv_derived_from(ST(0), "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(ST(0)));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else if (SvIOK(ST(0))) {
                IV tmp = SvIV((SV *)(ST(0)));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else
                croak("dest is not of type Affix::Pointer");
            RETVAL = newSVpvn_utf8((const char *)ptr, size, utf8 ? 1 : 0);
        }
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_DumpHex) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 2) croak_xs_usage(cv, "ptr, size");
    size_t size = (size_t)SvUV(ST(1));
    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        DCpointer ptr;
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
        _DumpHex(aTHX_ ptr, size, OutCopFILE(PL_curcop), CopLINE(PL_curcop));
        XSRETURN_EMPTY;
    }
    else
        croak("ptr is not of type Affix::Pointer");
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Pointer_DESTROY) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "ptr");
    DCpointer ptr;
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer"))) {
        croak("ptr is not of type Affix::Pointer");
    }
    if (UNLIKELY(sv_derived_from(ST(0), "Affix::Pointer::Unmanaged"))) { ; }
    else {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
        warn("DESTROY %p", ptr);
        if (ptr != NULL) {
            safefree(ptr);
            ptr = NULL;
        }
    }

    XSRETURN_EMPTY;
}

void boot_Affix_Pointer(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);
    {
        (void)newXSproto_portable("Affix::malloc", Affix_malloc, __FILE__, "$");
        export_function("Affix", "malloc", "memory");
        (void)newXSproto_portable("Affix::calloc", Affix_calloc, __FILE__, "$$");
        export_function("Affix", "calloc", "memory");
        (void)newXSproto_portable("Affix::realloc", Affix_realloc, __FILE__, "$$");
        export_function("Affix", "realloc", "memory");
        (void)newXSproto_portable("Affix::free", Affix_free, __FILE__, "$");
        export_function("Affix", "free", "memory");
        (void)newXSproto_portable("Affix::memchr", Affix_memchr, __FILE__, "$$$");
        export_function("Affix", "memchr", "memory");
        (void)newXSproto_portable("Affix::memcmp", Affix_memcmp, __FILE__, "$$$");
        export_function("Affix", "memcmp", "memory");
        (void)newXSproto_portable("Affix::memset", Affix_memset, __FILE__, "$$$");
        export_function("Affix", "memset", "memory");
        (void)newXSproto_portable("Affix::memcpy", Affix_memcpy, __FILE__, "$$$");
        export_function("Affix", "memcpy", "memory");
        (void)newXSproto_portable("Affix::memmove", Affix_memmove, __FILE__, "$$$");
        export_function("Affix", "memmove", "memory");
        (void)newXSproto_portable("Affix::strdup", Affix_strdup, __FILE__, "$");
        export_function("Affix", "strdup", "memory");
    }

    (void)newXSproto_portable("Affix::Type::marshal", Affix_Type_marshal, __FILE__, "$$");
    (void)newXSproto_portable("Affix::Type::unmarshal", Affix_Type_unmarshal, __FILE__, "$$");

    //(void)newXSproto_portable("Affix::Type::Pointer::(|", Affix_Type_Pointer, __FILE__, "");
    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(get_sv("Affix::Pointer::()", TRUE), &PL_sv_yes);
    /* Making a sub named "Affix::Pointer::()" allows the package */
    /* to be findable via fetchmethod(), and causes */
    /* overload::Overloaded("Affix::Pointer") to return true. */
    //~ (void)newXS_deffile("Affix::Pointer::()", Affix_Pointer_as_string);
    (void)newXSproto_portable("Affix::Pointer::plus", Affix_Pointer_plus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::(+", Affix_Pointer_plus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::minus", Affix_Pointer_minus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::(-", Affix_Pointer_minus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::as_string", Affix_Pointer_as_string, __FILE__,
                              "$;@");
    (void)newXSproto_portable("Affix::Pointer::(\"\"", Affix_Pointer_as_string, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::as_double", Affix_Pointer_as_double, __FILE__,
                              "$;@");
    (void)newXSproto_portable("Affix::Pointer::as_int", Affix_Pointer_as_int, __FILE__, "$;@");

    (void)newXSproto_portable("Affix::Pointer::(0+", Affix_Pointer_as_int, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::(${}", Affix_Pointer_deref_scalar, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::deref_scalar", Affix_Pointer_deref_scalar, __FILE__,
                              "$;@");
    (void)newXSproto_portable("Affix::Pointer::raw", Affix_Pointer_raw, __FILE__, "$$;$");
    (void)newXSproto_portable("Affix::Pointer::dump", Affix_Pointer_DumpHex, __FILE__, "$$");
    (void)newXSproto_portable("Affix::DumpHex", Affix_Pointer_DumpHex, __FILE__, "$$");
    (void)newXSproto_portable("Affix::Pointer::DESTROY", Affix_Pointer_DESTROY, __FILE__, "$");
    // $ptr->free or Affix::free($ptr)
    (void)newXSproto_portable("Affix::Pointer::free", Affix_free, __FILE__, "$");

    set_isa("Affix::Pointer::Unmanaged", "Affix::Pointer");

    set_isa("Affix::Type::Ref", "Affix::Type::Pointer");
    set_isa("Affix::Ref", "Affix::Pointer");
}
