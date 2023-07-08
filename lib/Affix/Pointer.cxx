#include "../Affix.h"

//~ XS_INTERNAL(Affix_Type_RWPointer) {
//~ croak("No.");
//~ }

XS_INTERNAL(Affix_Type_Pointer_RW) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    if (!(sv_isobject(ST(0)) && sv_derived_from(ST(0), "Affix::Type::Pointer")))
        croak("... | RW expects a subclass of Affix::Type::Pointer to the left");
    ST(0) = sv_bless(ST(0), gv_stashpv("Affix::Type::RWPointer", GV_ADD));
    SvSETMAGIC(ST(0));

    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Pointer_marshal) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "type, data");
    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ST(0))), "type", 0);
    SV *data = ST(1);
    DCpointer RETVAL = NULL; // = safemalloc(1);
    //~ warn("RETVAL should be %d bytes", _sizeof(aTHX_ type));
    RETVAL = sv2ptr(aTHX_ type, data, RETVAL, false);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Pointer_unmarshal) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "pointer, type");
    SV *RETVAL;
    DCpointer ptr;
    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ST(0))), "type", 0);
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("pointer is not of type Affix::Pointer");
    RETVAL = ptr2sv(aTHX_ ptr, type);
    RETVAL = sv_2mortal(RETVAL);
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_plus) {
    dVAR;
    dXSARGS;
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

XS_INTERNAL(Affix_Pointer_as_string) {
    dVAR;
    dXSARGS;
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

XS_INTERNAL(Affix_Pointer_raw) {
    dVAR;
    dXSARGS;
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
    if (items != 2) croak_xs_usage(cv, "ptr, size");
    size_t size = (size_t)SvUV(ST(1));
    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        DCpointer ptr;
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
        // Gathers perl caller() info
#ifdef USE_ITHREADS
        _DumpHex(aTHX_ ptr, size, PL_curcop->cop_file, PL_curcop->cop_line);
#else
        if (PL_curcop) { // Gathers perl caller() info
            const COP *cop = Perl_closest_cop(aTHX_ PL_curcop, OpSIBLING(PL_curcop), PL_op, FALSE);
            if (!cop) cop = PL_curcop;
            if (CopLINE(cop)) {
                _DumpHex(aTHX_ ptr, size, OutCopFILE(cop), CopLINE(cop));
                XSRETURN_EMPTY;
            }
        }
        DumpHex(ptr, size);
#endif
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
    if (UNLIKELY(sv_derived_from(ST(0), "Affix::Pointer::Unmanaged")))
        ;
    else if (LIKELY(sv_derived_from(ST(0), "Affix::Pointer"))) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
        if (ptr != NULL) {
            safefree(ptr);
            ptr = NULL;
        }
    }
    else
        croak("ptr is not of type Affix::Pointer");
    XSRETURN_EMPTY;
}

void boot_Affix_Pointer(pTHX_ CV *cv) {
    (void)newXSproto_portable("Affix::Type::Pointer::marshal", Affix_Type_Pointer_marshal, __FILE__,
                              "$$");
    (void)newXSproto_portable("Affix::Type::Pointer::unmarshal", Affix_Type_Pointer_unmarshal,
                              __FILE__, "$$");
    (void)newXSproto_portable("Affix::Type::Pointer::(|", Affix_Type_Pointer_RW, __FILE__, "");
    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(get_sv("Affix::Pointer::()", TRUE), &PL_sv_yes);
    /* Making a sub named "Affix::Pointer::()" allows the package */
    /* to be findable via fetchmethod(), and causes */
    /* overload::Overloaded("Affix::Pointer") to return true. */
    (void)newXS_deffile("Affix::Pointer::()", Affix_Pointer_as_string);
    (void)newXSproto_portable("Affix::Pointer::plus", Affix_Pointer_plus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::(+", Affix_Pointer_plus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::minus", Affix_Pointer_minus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::(-", Affix_Pointer_minus, __FILE__, "$$$");
    (void)newXSproto_portable("Affix::Pointer::as_string", Affix_Pointer_as_string, __FILE__,
                              "$;@");
    (void)newXSproto_portable("Affix::Pointer::(\"\"", Affix_Pointer_as_string, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::as_double", Affix_Pointer_as_double, __FILE__,
                              "$;@");
    (void)newXSproto_portable("Affix::Pointer::(0+", Affix_Pointer_as_double, __FILE__, "$;@");
    //~ (void)newXSproto_portable("Affix::Pointer::(${}", Affix_Pointer_deref_scalar, __FILE__,
    //"$;@"); ~ (void)newXSproto_portable("Affix::Pointer::deref_scalar",
    // Affix_Pointer_deref_scalar, __FILE__, "$;@");

    (void)newXSproto_portable("Affix::Pointer::raw", Affix_Pointer_raw, __FILE__, "$$;$");
    (void)newXSproto_portable("Affix::Pointer::dump", Affix_Pointer_DumpHex, __FILE__, "$$");
    (void)newXSproto_portable("Affix::DumpHex", Affix_Pointer_DumpHex, __FILE__, "$$");
    (void)newXSproto_portable("Affix::Pointer::DESTROY", Affix_Pointer_DESTROY, __FILE__, "$");
    set_isa("Affix::Pointer::Unmanaged", "Affix::Pointer");
}
