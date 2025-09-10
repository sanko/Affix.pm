#include "../Affix.h"

XS_INTERNAL(Affix_Type_Pointer_store) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "type, data");
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Type::Pointer")))
        croak("type is not of type Affix::Type");
    Affix_Type * type = new_Affix_Type(aTHX_ ST(0));
    //~ if (UNLIKELY(type->type != POINTER_FLAG))
    //~ croak("type must be a Pointer");

    DCpointer ptr = NULL;
    type->data.pointer_type->store(aTHX_ type->data.pointer_type, ST(1), &ptr);
    if (ptr == NULL)
        XSRETURN_EMPTY;

    Affix_Pointer * pointer = NULL;
    Newxz(pointer, 1, Affix_Pointer);
    pointer->address = ptr;
    //~ pointer->count = 0;
    ST(0) = sv_setref_pv(newSV(0), "Affix::Pointer::Unmanaged", (DCpointer)pointer);

    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Pointer_fetch) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "type, data");
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Type::Pointer")))
        croak("type is not of type Affix::Type");

    DCpointer ptr = NULL;
    if (SvROK(ST(1)) && sv_derived_from(ST(1), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(1))));
        ptr = pointer->address;
    }
    else if (SvIOK(ST(1))) {
        IV tmp = SvIV((SV *)(ST(1)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;

    Affix_Type * type = new_Affix_Type(aTHX_ ST(0));
    //~ if (UNLIKELY(type->type != POINTER_FLAG))
    //~ croak("type must be a Pointer");

    ST(0) = type->data.pointer_type->fetch(aTHX_ type->data.pointer_type, ptr, NULL);
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_malloc) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    SSize_t size = (SSize_t)SvIV(ST(0));
    DCpointer ptr = safemalloc(size);
    if (ptr == NULL) {
        warn("Failed to allocate new pointer");
        XSRETURN_EMPTY;
    }
    Affix_Pointer * pointer = NULL;
    Newxz(pointer, 1, Affix_Pointer);
    pointer->address = ptr;
    //~ pointer->count = 0;
    ST(0) = sv_setref_pv(newSV(0), "Affix::Pointer", (DCpointer)pointer);
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_calloc) {
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$num, $size");
    SSize_t num = (SSize_t)SvIV(ST(0));
    SSize_t size = (SSize_t)SvIV(ST(1));
    DCpointer ptr = safecalloc(num, size);
    if (ptr == NULL) {
        warn("Failed to allocate new pointer");
        XSRETURN_EMPTY;
    }
    Affix_Pointer * pointer = NULL;
    Newxz(pointer, 1, Affix_Pointer);
    pointer->address = ptr;
    //~ pointer->count = 0;
    ST(0) = sv_setref_pv(newSV(0), "Affix::Pointer", (DCpointer)pointer);
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_realloc) {
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$ptr, $size");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;
    SSize_t size = (SSize_t)SvIV(ST(1));
    ptr = saferealloc(ptr, size);
    if (ptr == NULL) {
        warn("Failed to allocate new pointer");
        XSRETURN_EMPTY;
    }
    Affix_Pointer * pointer = NULL;
    Newxz(pointer, 1, Affix_Pointer);
    pointer->address = ptr;
    //~ pointer->count = 0;
    ST(0) = sv_setref_pv(newSV(0), "Affix::Pointer", (DCpointer)pointer);
    XSRETURN(1);
}
XS_INTERNAL(Affix_Pointer_dump) {
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$src, $count");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;
    // Correct context
    _DumpHex(aTHX_ ptr, SvIV(ST(1)), OutCopFILE(PL_curcop), CopLINE(PL_curcop));
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Pointer_raw) {
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$src, $count");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;
    ST(0) = newSVpv((const char *)ptr, SvIV(ST(1)));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_memcpy) {
    dXSARGS;
    if (items != 3)
        croak_xs_usage(cv, "$dest, $src, $count");

    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src, RETVAL;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        Affix_Pointer * dest_ptr = INT2PTR(Affix_Pointer *, tmp);
        if (dest_ptr == NULL)
            XSRETURN_EMPTY;
        dest = dest_ptr->address;
        if (dest == NULL)
            XSRETURN_EMPTY;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Affix::Pointer");

    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        Affix_Pointer * src_ptr = INT2PTR(Affix_Pointer *, tmp);
        src = src_ptr->address;
    }
    else if (SvIOK(ST(1))) {
        IV tmp = SvIV((SV *)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvPOK(ST(1))) {
        src = (DCpointer)(U8 *)SvPV_nolen(ST(1));
    }
    else
        croak("dest is not of type Affix::Pointer");

    RETVAL = CopyD(src, dest, nitems, char);

    {
        SV * RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_free) {
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "$ptr");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;
    safefree(ptr);
    ptr = NULL;
    sv_set_undef(ST(0));
    SvSETMAGIC(ST(0));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_memchr) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 3)
        croak_xs_usage(cv, "ptr, ch, count");
    {
        DCpointer RETVAL = NULL;
        DCpointer ptr = NULL;
        if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
            Affix_Pointer * pointer;
            pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
            ptr = pointer->address;
        }
        else if (SvIOK(ST(0))) {
            IV tmp = SvIV((SV *)(ST(0)));
            ptr = INT2PTR(DCpointer, tmp);
        }
        if (ptr == NULL)
            XSRETURN_EMPTY;

        char ch = (char)*SvPV_nolen(ST(1));
        size_t count = (size_t)SvUV(ST(2));

        RETVAL = memchr(ptr, ch, count);
        {
            SV * RETVALSV;
            RETVALSV = sv_newmortal();
            sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
            ST(0) = RETVALSV;
        }
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_DESTROY) {
    dXSARGS;
    PERL_UNUSED_VAR(items);

    if (items != 1)
        croak_xs_usage(cv, "$ptr");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
        safefree(pointer);
        pointer = NULL;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;
    // safefree(ptr);
    ptr = NULL;

    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Pointer_Unmanaged_DESTROY) {
    dXSARGS;
    PERL_UNUSED_VAR(items);

    if (items != 1)
        croak_xs_usage(cv, "$ptr");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
        safefree(pointer);
        pointer = NULL;
    }
    // DO NOTHING HERE! Require users to manually call ->free() instead

    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Pointer_as_string) {
    dXSARGS;
    if (items < 1)
        croak_xs_usage(cv, "$pointer");
    char * RETVAL;
    dXSTARG;
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;

    RETVAL = (char *)ptr;
    sv_setpv(TARG, RETVAL);
    XSprePUSH;
    PUSHTARG;

    XSRETURN(1);
}
/*
XS_INTERNAL(Affix_Pointer_as_string) {
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$src, $count");
    DCpointer ptr = NULL;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Pointer")) {
        _Affix_Pointer * pointer;
        pointer = INT2PTR(_Affix_Pointer *, SvIV(SvRV(ST(0))));
        ptr = pointer->address;
    } else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    if (ptr == NULL)
        XSRETURN_EMPTY;
    ST(0) = newSVpv((const char *)ptr, strlen(ptr));
    XSRETURN(1);
}*/

XS_INTERNAL(Affix_Pointer_deref_hash) {
    dVAR;
    dXSARGS;
    PERL_UNUSED_VAR(items);
    /* if (items < 1)
         croak_xs_usage(cv, "$pointer");
     warn("DEREF HASH!!!!!!!!!!!!!!!!!!!!!!");

     {
         char * RETVAL;
         dXSTARG;
         Affix_Pointer * ptr;

         if (sv_derived_from(ST(0), "Affix::Pointer")) {
             IV tmp = SvIV((SV *)SvRV(ST(0)));
             ptr = INT2PTR(Affix_Pointer *, tmp);
         } else
             croak("ptr is not of type Affix::Pointer");
         //     RETVAL = (char *)ptr->address;
         //     sv_setpv(TARG, RETVAL);
         //     XSprePUSH;
         //     PUSHTARG;
         if (ptr->type->numeric != STRUCT_FLAG)
             XSRETURN(1);  // Just toss back garbage
         ST(0) = newRV(MUTABLE_SV(newHV_mortal()));
     }*/
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_deref_list) {
    dVAR;
    dXSARGS;
    PERL_UNUSED_VAR(items);

    /*
    if (items < 1)
        croak_xs_usage(cv, "$pointer");
    warn("DEREF LIST!!!!!!!!!!!!!!!!!!!!!!");

    {
        char * RETVAL;
        dXSTARG;
        Affix_Pointer * ptr;

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            ptr = INT2PTR(Affix_Pointer *, tmp);
        } else
            croak("ptr is not of type Affix::Pointer");
        //     RETVAL = (char *)ptr->address;
        //     sv_setpv(TARG, RETVAL);
        //     XSprePUSH;
        //     PUSHTARG;
        // if (ptr->type->numeric != STRUCT_FLAG)
        // XSRETURN(1);  // Just toss back garbage
        ST(0) = newRV(MUTABLE_SV(newAV_mortal()));
    }*/
    XSRETURN(1);
}


XS_INTERNAL(Affix_Pointer_FETCH) {
    dVAR;
    dXSARGS;
    PERL_UNUSED_VAR(items);
    /*
         if (items != 2)
             croak_xs_usage(cv, "$pointer, $index");
         warn("FETCH!!!!!!!!!!!!!!!! %d", SvIV(ST(1)));*/
    XSRETURN_EMPTY;
}

void boot_Affix_Pointer(pTHX_ CV * cv) {
    PERL_UNUSED_VAR(cv);

    {
        (void)newXSproto_portable("Affix::malloc", Affix_Pointer_malloc, __FILE__, "$");
        export_function("Affix", "malloc", "memory");
        (void)newXSproto_portable("Affix::calloc", Affix_Pointer_calloc, __FILE__, "$$");
        export_function("Affix", "calloc", "memory");
        (void)newXSproto_portable("Affix::realloc", Affix_Pointer_realloc, __FILE__, "$$");
        export_function("Affix", "realloc", "memory");
        (void)newXSproto_portable("Affix::free", Affix_Pointer_free, __FILE__, "$");
        export_function("Affix", "free", "memory");
        (void)newXSproto_portable("Affix::memchr", Affix_Pointer_memchr, __FILE__, "$$$");
        export_function("Affix", "memchr", "memory");
        //~ (void)newXSproto_portable("Affix::memcmp", Affix_memcmp, __FILE__, "$$$");
        //~ export_function("Affix", "memcmp", "memory");
        //~ (void)newXSproto_portable("Affix::memset", Affix_memset, __FILE__, "$$$");
        //~ export_function("Affix", "memset", "memory");
        (void)newXSproto_portable("Affix::memcpy", Affix_Pointer_memcpy, __FILE__, "$$$");
        export_function("Affix", "memcpy", "memory");
        //~ (void)newXSproto_portable("Affix::memmove", Affix_memmove, __FILE__, "$$$");
        //~ export_function("Affix", "memmove", "memory");
        //~ (void)newXSproto_portable("Affix::strdup", Affix_strdup, __FILE__, "$");
        //~ export_function("Affix", "strdup", "memory");
    }

    (void)newXSproto_portable("Affix::Type::Pointer::store", Affix_Type_Pointer_store, __FILE__, "$$");
    (void)newXSproto_portable("Affix::Type::Pointer::fetch", Affix_Type_Pointer_fetch, __FILE__, "$$");

    //(void)newXSproto_portable("Affix::Type::Pointer::(|", Affix_Type_Pointer, __FILE__, "");
    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(get_sv("Affix::Pointer::()", TRUE), &PL_sv_yes);
    /* Making a sub named "Affix::Pointer::()" allows the package */
    /* to be findable via fetchmethod(), and causes */
    /* overload::Overloaded("Affix::Pointer") to return true. */
    // (void)newXS_deffile("Affix::Pointer::()", Affix_Pointer_as_string);
    (void)newXSproto_portable("Affix::Pointer::()", Affix_Pointer_as_string, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::(\"\"", Affix_Pointer_as_string, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::as_string", Affix_Pointer_as_string, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::(%{}", Affix_Pointer_deref_hash, __FILE__, "$;@");
    (void)newXSproto_portable("Affix::Pointer::(@{}", Affix_Pointer_deref_list, __FILE__, "$;@");
    //  ${}  @{}  %{}  &{}  *{}


    (void)newXSproto_portable("Affix::Pointer::dump", Affix_Pointer_dump, __FILE__, "$$");

    (void)newXSproto_portable("Affix::Pointer::raw", Affix_Pointer_raw, __FILE__, "$$");


    (void)newXSproto_portable("Affix::Pointer::free", Affix_Pointer_free, __FILE__, "$;$");
    (void)newXSproto_portable("Affix::Pointer::DESTROY", Affix_Pointer_DESTROY, __FILE__, "$");
    (void)newXSproto_portable("Affix::Pointer::Unmanaged::DESTROY", Affix_Pointer_Unmanaged_DESTROY, __FILE__, "$");


    (void)newXSproto_portable("Affix::Pointer::FETCH", Affix_Pointer_FETCH, __FILE__, "$$");


    set_isa("Affix::Pointer::Unmanaged", "Affix::Pointer");
}
