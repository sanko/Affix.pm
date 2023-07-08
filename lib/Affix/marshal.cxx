#include "../Affix.h"

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    if (ptr == NULL) return newSV(0);
    SV *RETVAL = newSV(1); // sv_newmortal();
    int16_t type = SvIV(type_sv);
    //~ {
    //~ warn("ptr2sv(%p, %s) at %s line %d", ptr, type_as_str(type), __FILE__, __LINE__);
    //~ if (type != AFFIX_ARG_VOID) {
    //~ size_t l = _sizeof(type_sv);
    //~ DumpHex(ptr, l);
    //~ }
    //~ }
    switch (type) {
    case AFFIX_ARG_SV: {
        if (ptr == NULL) { RETVAL = newSV(0); }
        else if (*(void **)ptr != NULL && SvOK(MUTABLE_SV(*(void **)ptr))) {
            RETVAL = MUTABLE_SV(*(void **)ptr);
        }
    } break;
    case AFFIX_ARG_VOID: {
        if (ptr == NULL) { RETVAL = newSV(0); }
        else { sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", ptr); }
    } break;
    case AFFIX_ARG_BOOL:
        sv_setbool_mg(RETVAL, (bool)*(bool *)ptr);
        break;
    case AFFIX_ARG_CHAR:
    case AFFIX_ARG_UCHAR:
        sv_setsv(RETVAL, newSVpv((char *)ptr, 0));
        (void)SvUPGRADE(RETVAL, SVt_PVIV);
        SvIV_set(RETVAL, ((IV) * (char *)ptr));
        SvIOK_on(RETVAL);
        break;
    case AFFIX_ARG_WCHAR: {
        if (wcslen((wchar_t *)ptr)) {
            RETVAL = wchar2utf(aTHX_(wchar_t *) ptr, wcslen((wchar_t *)ptr));
        }
    } break;
    case AFFIX_ARG_SHORT:
        sv_setiv(RETVAL, *(short *)ptr);
        break;
    case AFFIX_ARG_USHORT:
        sv_setuv(RETVAL, *(unsigned short *)ptr);
        break;
    case AFFIX_ARG_INT:
        sv_setiv(RETVAL, *(int *)ptr);
        break;
    case AFFIX_ARG_UINT:
        sv_setuv(RETVAL, *(unsigned int *)ptr);
        break;
    case AFFIX_ARG_LONG:
        sv_setiv(RETVAL, *(long *)ptr);
        break;
    case AFFIX_ARG_ULONG:
        sv_setuv(RETVAL, *(unsigned long *)ptr);
        break;
    case AFFIX_ARG_LONGLONG:
        sv_setiv(RETVAL, *(I64 *)ptr);
        break;
    case AFFIX_ARG_ULONGLONG:
        sv_setuv(RETVAL, *(U64 *)ptr);
        break;
    case AFFIX_ARG_FLOAT:
        sv_setnv(RETVAL, *(float *)ptr);
        break;
    case AFFIX_ARG_DOUBLE:
        sv_setnv(RETVAL, *(double *)ptr);
        break;
    case AFFIX_ARG_CPOINTER: {
        //~ croak("POINTER!!!!!!!!");
        SV *subtype;
        if (sv_derived_from(type_sv, "Affix::Type::Pointer"))
            subtype = *hv_fetchs(MUTABLE_HV(SvRV(type_sv)), "type", 0);
        else { subtype = type_sv; }
        //~ //char *_subtype = SvPV_nolen(subtype);
        //~ if (_subtype[0] == AFFIX_ARG_VOID) {
        //~ SV *RETVALSV = newSV(1); // sv_newmortal();
        //~ SvSetSV(RETVAL, sv_setref_pv(RETVALSV, "Affix::Pointer", *(DCpointer *)ptr));
        //~ }
        //~ else {
        SvSetSV(RETVAL, ptr2sv(aTHX_ ptr, subtype));
        //~ }
    } break;
    case AFFIX_ARG_ASCIISTR:
        if (*(char **)ptr) sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
        break;
    case AFFIX_ARG_UTF16STR: {
        if (wcslen((wchar_t *)ptr)) {
            RETVAL = wchar2utf(aTHX_ * (wchar_t **)ptr, wcslen(*(wchar_t **)ptr));
        }
        else
            sv_set_undef(RETVAL);
    } break;
    case AFFIX_ARG_CARRAY: {
        AV *RETVAL_ = newAV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));

        SV *subtype = *hv_fetchs(_type, "type", 0);
        SV **size = hv_fetchs(_type, "size", 0);
        if (size == NULL) size = hv_fetchs(_type, "dyn_size", 0);

        size_t pos = PTR2IV(ptr);
        size_t sof = _sizeof(aTHX_ subtype);

        size_t av_len = SvIV(*size);
        for (size_t i = 0; i < av_len; ++i) {
            av_push(RETVAL_, ptr2sv(aTHX_ INT2PTR(DCpointer, pos), subtype));
            pos += sof;
        }

        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case AFFIX_ARG_CSTRUCT: {
#if TIE_MAGIC
        HV *RETVAL_ = newHV_mortal();
        SV *p = newSV(0);
        sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
        SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
        hv_store(MUTABLE_HV(SvRV(tie)), "pointer", 7, p, 0);
        hv_store(MUTABLE_HV(SvRV(tie)), "type", 4, newRV_inc(type_sv), 0);
        sv_bless(tie, gv_stashpv("Affix::Struct", TRUE));
        hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
#else
        HV *RETVAL_ = newHV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));
        AV *fields = MUTABLE_AV(SvRV(*hv_fetchs(_type, "fields", 0)));

        size_t field_count = av_count(fields);
        for (size_t i = 0; i < field_count; ++i) {
            AV *field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
            SV *name = *av_fetch(field, 0, 0);
            SV *subtype = *av_fetch(field, 1, 0);
            (void)hv_store_ent(
                RETVAL_, name,
                ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ subtype)), subtype),
                0);
        }
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
#endif
    } break;
    case AFFIX_ARG_CUNION: {
        HV *RETVAL_ = newHV_mortal();
        SV *p = newSV(0);
        sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
        SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
        hv_store(MUTABLE_HV(SvRV(tie)), "pointer", 7, p, 0);
        hv_store(MUTABLE_HV(SvRV(tie)), "type", 4, newRV_inc(type_sv), 0);
        sv_bless(tie, gv_stashpv("Affix::Union", TRUE));
        hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case AFFIX_ARG_CALLBACK: {
        CallbackWrapper *p = (CallbackWrapper *)ptr;
        Callback *cb = (Callback *)dcbGetUserData((DCCallback *)p->cb);
        SvSetSV(RETVAL, cb->cv);
    } break;
    //~ case AFFIX_ARG_CPPSTRUCT: {
    //~ RETVAL = ptr2sv(aTHX_ ptr, _instanceof(aTHX_ type));
    //~ } break;
    //~ case AFFIX_ARG_ENUM: {
    //~ SvSetSV(RETVAL, enum2sv(aTHX_ type, *(int *)ptr));
    //~ }; break;
    //~ case AFFIX_ARG_ENUM_UINT: {
    //~ SvSetSV(RETVAL, enum2sv(aTHX_ type, *(unsigned int *)ptr));
    //~ }; break;
    //~ case AFFIX_ARG_ENUM_CHAR: {
    //~ SvSetSV(RETVAL, enum2sv(aTHX_ type, (IV) * (char *)ptr));
    //~ }; break;
    default:
        croak("Unhandled type in ptr2sv: %s (%d)", type_as_str(type), type);
    }

    return RETVAL;
}

void *sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed) {
    int16_t type = SvIV(type_sv);
    if (ptr == NULL) ptr = safemalloc(_sizeof(aTHX_ type_sv));
    //~ warn("sv2ptr(%s (%d), ..., %p, %s) at %s line %d", type_as_str(type), type, ptr,
    //~ (packed ? "true" : "false"), __FILE__, __LINE__);
    switch (type) {
    case AFFIX_ARG_SV: {
        if (!SvOK(data))
            Zero(ptr, 1, intptr_t);
        else {
            SvREFCNT_inc(data); // TODO: This might leak; I'm just being lazy
            DCpointer value = (DCpointer)data;
            Renew(ptr, 1, intptr_t);
            Copy(&value, ptr, 1, intptr_t);
        }
    } break;
    case AFFIX_ARG_VOID: {
        if (!SvOK(data))
            Zero(ptr, 1, intptr_t);
        else if (sv_derived_from(data, "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(data));
            ptr = INT2PTR(DCpointer, tmp);
            Copy((DCpointer)(&data), ptr, 1, intptr_t);
        }
        else if (SvPOK(data)) {
            size_t len;
            char *raw = SvPV(data, len);
            Renew(ptr, len + 1, char);
            Copy((DCpointer)raw, ptr, len + 1, char);
        }
        else
            croak("Expected a subclass of Affix::Pointer");
    } break;
    case AFFIX_ARG_BOOL: {
        bool value = SvOK(data) ? SvTRUE(data) : (bool)0; // default to false
        Copy(&value, ptr, 1, bool);
    } break;
    case AFFIX_ARG_CHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            Renew(ptr, len + 1, char);
            Copy(value, ptr, len + 1, char);
        }
        else {
            char value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, char);
        }
    } break;
    case AFFIX_ARG_UCHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            Renew(ptr, len + 1, unsigned char);
            Copy(value, ptr, len + 1, unsigned char);
        }
        else {
            unsigned char value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, unsigned char);
        }
    } break;
    case AFFIX_ARG_WCHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            (void)SvPVutf8(data, len);
            wchar_t *value = utf2wchar(aTHX_ data, len + 1);
            len = wcslen(value);
            Renew(ptr, len + 1, wchar_t);
            Copy(value, ptr, len + 1, wchar_t);
        }
        else {
            wchar_t value = SvIOK(data) ? SvIV(data) : 0;
            // Renew(ptr, 1, wchar_t);
            Copy(&value, ptr, 1, wchar_t);
        }
    } break;
    case AFFIX_ARG_SHORT: {
        short value = SvOK(data) ? (short)SvIV(data) : 0;
        Copy(&value, ptr, 1, short);
    } break;
    case AFFIX_ARG_USHORT: {
        unsigned short value = SvOK(data) ? (unsigned short)SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned short);
    } break;
    case AFFIX_ARG_INT: {
        int value = SvOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, int);
    } break;
    case AFFIX_ARG_UINT: {
        unsigned int value = SvOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned int);
    } break;
    case AFFIX_ARG_LONG: {
        long value = SvOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, long);
    } break;
    case AFFIX_ARG_ULONG: {
        unsigned long value = SvOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned long);
    } break;
    case AFFIX_ARG_LONGLONG: {
        I64 value = SvOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, I64);
    } break;
    case AFFIX_ARG_ULONGLONG: {
        U64 value = SvOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, U64);
    } break;
    case AFFIX_ARG_FLOAT: {
        float value = SvOK(data) ? SvNV(data) : 0;
        Copy(&value, ptr, 1, float);
    } break;
    case AFFIX_ARG_DOUBLE: {
        double value = SvOK(data) ? SvNV(data) : 0;
        Copy(&value, ptr, 1, double);
    } break;
        /*
        case AFFIX_ARG_CPOINTER: {
            HV *hv_ptr = MUTABLE_HV(SvRV(type));
            SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
            DCpointer value = safemalloc(_sizeof(aTHX_ * type_ptr));
            if (SvOK(data)) sv2ptr(aTHX_ * type_ptr, data, value, packed);
            Copy(&value, ptr, 1, intptr_t);
        } break;*/

    case AFFIX_ARG_ASCIISTR: {
        if (SvPOK(data)) {
            STRLEN len;
            const char *str = SvPV(data, len);
            DCpointer value;
            Newxz(value, len + 1, char);
            Copy(str, value, len, char);
            Copy(&value, ptr, 1, intptr_t);
        }
        else {
            const char *str = "";
            DCpointer value;
            Newxz(value, 1, char);
            Copy(str, value, 1, char);
            Copy(&value, ptr, 1, intptr_t);
        }
    } break;
    case AFFIX_ARG_UTF16STR: {
        if (SvPOK(data)) {
            STRLEN len;
            (void)SvPVutf8(data, len);
            wchar_t *str = utf2wchar(aTHX_ data, len + 1);

            //~ DumpHex(str, strlen(str_));
            DCpointer value;
            Newxz(value, len, wchar_t);
            Copy(str, value, len, wchar_t);
            Copy(&value, ptr, 1, intptr_t);
        }
        else
            Zero(ptr, 1, intptr_t);
    } break;
    //~ case AFFIX_ARG_CPPSTRUCT: {
    //~ HV *hv_ptr = MUTABLE_HV(SvRV(type));
    //~ SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
    //~ DCpointer value = safemalloc(_sizeof(aTHX_ * type_ptr));
    //~ if (SvOK(data)) sv2ptr(aTHX_ _instanceof(aTHX_ * type_ptr), data, value, packed);
    //~ Copy(&value, ptr, 1, intptr_t);
    //~ } break;
    case AFFIX_ARG_CSTRUCT: {
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type_sv));
            HV *hv_data = MUTABLE_HV(SvRV(data));
            SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
            //~ SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
            AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
            size_t field_count = av_count(av_fields);
            for (size_t i = 0; i < field_count; ++i) {
                SV **field = av_fetch(av_fields, i, 0);
                AV *name_type = MUTABLE_AV(SvRV(*field));
                SV **name_ptr = av_fetch(name_type, 0, 0);
                SV **type_ptr = av_fetch(name_type, 1, 0);
                char *key = SvPVbytex_nolen(*name_ptr);
                SV **_data = hv_fetch(hv_data, key, strlen(key), 1);
                if (_data != NULL) {
                    sv2ptr(aTHX_ * type_ptr, *_data,
                           INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr)), packed);
                }
            }
        }
    } break;
    case AFFIX_ARG_CUNION: {
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type_sv));
            HV *hv_data = MUTABLE_HV(SvRV(data));
            SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
            //~ SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
            AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
            size_t field_count = av_count(av_fields);
            for (size_t i = 0; i < field_count; ++i) {
                SV **field = av_fetch(av_fields, i, 0);
                AV *name_type = MUTABLE_AV(SvRV(*field));
                SV **name_ptr = av_fetch(name_type, 0, 0);
                SV **type_ptr = av_fetch(name_type, 1, 0);
                char *key = SvPVbytex_nolen(*name_ptr);
                SV **_data = hv_fetch(hv_data, key, strlen(key), 1);
                if (data != NULL && SvOK(*_data)) {
                    sv2ptr(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 1)),
                           INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr)), packed);
                    break;
                }
            }
        }
    } break;
    case AFFIX_ARG_CARRAY: {
        AV *elements = MUTABLE_AV(SvRV(data));

        HV *hv_ptr = MUTABLE_HV(SvRV(type_sv));

        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);

        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        hv_stores(hv_ptr, "dyn_size", newSVuv(av_count(elements)));

        size_t size = size_ptr != NULL && SvOK(*size_ptr) ? SvIV(*size_ptr) : av_count(elements);

        // hv_stores(hv_ptr, "size", newSViv(size));
        char *type_char = SvPVbytex_nolen(*type_ptr);

        switch (type_char[0]) {
        case AFFIX_ARG_CHAR:
        case AFFIX_ARG_UCHAR: {
            if (SvPOK(data)) {
                if (type_char[0] == AFFIX_ARG_CHAR) {
                    char *value = SvPV(data, size);
                    Copy(value, ptr, size, char);
                }
                else {
                    unsigned char *value = (unsigned char *)SvPV(data, size);
                    Copy(value, ptr, size, unsigned char);
                }
                break;
            }
        }
        // fall through
        default: {

            if (SvOK(SvRV(data)) && SvTYPE(SvRV(data)) != SVt_PVAV) croak("Expected an array");
            if (size_ptr != NULL && SvOK(*size_ptr)) {
                size_t av_len = av_count(elements);
                if (av_len != size)
                    croak("Expected and array of %zu elements; found %zu", size, av_len);
            }

            size_t el_len = _sizeof(aTHX_ * type_ptr);
            size_t pos = 0; // override
            for (size_t i = 0; i < size; ++i) {
                //~ warn("int[%d] of %d", i, size);
                //~ warn("Putting index %d into pointer plus %d", i, pos);
                sv2ptr(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)),
                       INT2PTR(DCpointer, PTR2IV(ptr) + pos), packed);
                pos += el_len;
            }
        }
            // return _sizeof(aTHX_ type);
        }
    } break;
    case AFFIX_ARG_CALLBACK: {
        DCCallback *cb = NULL;
        HV *field = MUTABLE_HV(SvRV(type_sv)); // Make broad assumptions
        //~ SV **ret = hv_fetchs(field, "ret", 0);
        SV **args = hv_fetchs(field, "args", 0);
        SV **sig = hv_fetchs(field, "sig", 0);
        Callback *callback;
        Newxz(callback, 1, Callback);
        callback->args = MUTABLE_AV(SvRV(*args));
        size_t arg_count = av_count(callback->args);
        Newxz(callback->sig, arg_count, char);
        for (size_t i = 0; i < arg_count; ++i) {
            SV **type = av_fetch(callback->args, i, 0);
            callback->sig[i] = type_as_dc(SvIV(*type));
        }
        callback->sig = SvPV_nolen(*sig);
        callback->sig_len = strchr(callback->sig, ')') - callback->sig;
        callback->ret = callback->sig[callback->sig_len + 1];
        callback->cv = SvREFCNT_inc(data);
        storeTHX(callback->perl);
        cb = dcbNewCallback(callback->sig, cbHandler, callback);
        {
            CallbackWrapper *hold;
            Newxz(hold, 1, CallbackWrapper);
            hold->cb = cb;
            Copy(hold, ptr, 1, DCpointer);
        }
    } break;
    default: {
        croak("%s (%d) is not a known type in sv2ptr(...)", type_as_str(type), type);
    }
    }
    return ptr;
}

SV *Xptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    if (ptr == NULL) return newSV(0);
    SV *RETVAL = newSV(1); // sv_newmortal();
    int16_t type = SvIV(type_sv);
    //~ {
    warn("ptr2sv(%p, %s) at %s line %d", ptr, type_as_str(type), __FILE__, __LINE__);
    DD(type_sv);
    //~ if (type != AFFIX_ARG_VOID) {
    //~ size_t l = _sizeof(type_sv);
    //~ DumpHex(ptr, l);
    //~ }
    //~ }
    switch (type) {
    case AFFIX_ARG_VOID: {
        PING;
        sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", ptr);
        PING;
    } break;
    case AFFIX_ARG_BOOL:
        sv_setbool_mg(RETVAL, (bool)*(bool *)ptr);
        break;
    case AFFIX_ARG_CHAR:
    case AFFIX_ARG_UCHAR:
        sv_setsv(RETVAL, newSVpv((char *)ptr, 0));
        (void)SvUPGRADE(RETVAL, SVt_PVIV);
        SvIV_set(RETVAL, ((IV) * (char *)ptr));
        SvIOK_on(RETVAL);
        break;
    case AFFIX_ARG_WCHAR: {
        if (wcslen((wchar_t *)ptr)) {
            RETVAL = wchar2utf(aTHX_(wchar_t *) ptr, wcslen((wchar_t *)ptr));
        }
    } break;
    case AFFIX_ARG_SHORT:
        sv_setiv(RETVAL, *(short *)ptr);
        break;
    case AFFIX_ARG_USHORT:
        sv_setuv(RETVAL, *(unsigned short *)ptr);
        break;
    case AFFIX_ARG_INT:
        sv_setiv(RETVAL, *(int *)ptr);
        break;
    case AFFIX_ARG_UINT:
        sv_setuv(RETVAL, *(unsigned int *)ptr);
        break;
    case AFFIX_ARG_LONG:
        sv_setiv(RETVAL, *(long *)ptr);
        break;
    case AFFIX_ARG_ULONG:
        sv_setuv(RETVAL, *(unsigned long *)ptr);
        break;
    case AFFIX_ARG_LONGLONG:
        sv_setiv(RETVAL, *(I64 *)ptr);
        break;
    case AFFIX_ARG_ULONGLONG:
        sv_setuv(RETVAL, *(U64 *)ptr);
        break;
    case AFFIX_ARG_FLOAT:
        sv_setnv(RETVAL, *(float *)ptr);
        break;
    case AFFIX_ARG_DOUBLE:
        sv_setnv(RETVAL, *(double *)ptr);
        break;
    case AFFIX_ARG_CPOINTER: {
        PING;
        SV *subtype = *hv_fetch(MUTABLE_HV(SvRV(type_sv)), "type", 4, 0);
        SV *RETVALSV;
        switch (SvIV(subtype)) {
        case AFFIX_ARG_VOID: {
            PING;
            RETVALSV = newSV(1); // sv_newmortal();
            SvSetSV(RETVAL, sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", *(void **)ptr));
        } break;
        default: {
            PING;
            RETVALSV = ptr2sv(aTHX_ * (void **)ptr, subtype);
            SvSetSV(RETVAL, RETVALSV);
        }
        }
        PING;

    } break;
    case AFFIX_ARG_ASCIISTR:
        if (*(char **)ptr) sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
        break;
    case AFFIX_ARG_UTF16STR: {
        if (wcslen((wchar_t *)ptr)) {
            RETVAL = wchar2utf(aTHX_ * (wchar_t **)ptr, wcslen(*(wchar_t **)ptr));
        }
        else
            sv_set_undef(RETVAL);
    } break;
    case AFFIX_ARG_CARRAY: {
        AV *RETVAL_ = newAV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));

        SV *subtype = *hv_fetchs(_type, "type", 0);
        SV **size = hv_fetchs(_type, "size", 0);
        if (size == NULL) size = hv_fetchs(_type, "dyn_size", 0);

        size_t pos = PTR2IV(ptr);
        size_t sof = _sizeof(aTHX_ subtype);

        size_t av_len = SvIV(*size);
        for (size_t i = 0; i < av_len; ++i) {
            av_push(RETVAL_, ptr2sv(aTHX_ INT2PTR(DCpointer, pos), subtype));
            pos += sof;
        }
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case AFFIX_ARG_CSTRUCT: {
#if TIE_MAGIC
        HV *RETVAL_ = newHV_mortal();
        SV *p = newSV(0);
        sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
        SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
        hv_store(MUTABLE_HV(SvRV(tie)), "pointer", 7, p, 0);
        hv_store(MUTABLE_HV(SvRV(tie)), "type", 4, newRV_inc(type_sv), 0);
        sv_bless(tie, gv_stashpv("Affix::Struct", TRUE));
        hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
#else
        HV *RETVAL_ = newHV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));
        AV *fields = MUTABLE_AV(SvRV(*hv_fetchs(_type, "fields", 0)));

        size_t field_count = av_count(fields);
        for (size_t i = 0; i < field_count; ++i) {
            AV *field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
            SV *name = *av_fetch(field, 0, 0);
            SV *subtype = *av_fetch(field, 1, 0);
            (void)hv_store_ent(
                RETVAL_, name,
                ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ subtype)), subtype),
                0);
        }
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
#endif
    } break;
    case AFFIX_ARG_CUNION: {
        HV *RETVAL_ = newHV_mortal();
        SV *p = newSV(0);
        sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
        SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
        hv_store(MUTABLE_HV(SvRV(tie)), "pointer", 7, p, 0);
        hv_store(MUTABLE_HV(SvRV(tie)), "type", 4, newRV_inc(type_sv), 0);
        sv_bless(tie, gv_stashpv("Affix::Union", TRUE));
        hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case AFFIX_ARG_CALLBACK: {
        CallbackWrapper *p = (CallbackWrapper *)ptr;
        Callback *cb = (Callback *)dcbGetUserData((DCCallback *)p->cb);
        SvSetSV(RETVAL, cb->cv);
    } break;
    //~ case AFFIX_ARG_CPPSTRUCT: {
    //~ RETVAL = ptr2sv(aTHX_ ptr, _instanceof(aTHX_ type));
    //~ } break;
    //~ case AFFIX_ARG_ENUM: {
    //~ SvSetSV(RETVAL, enum2sv(aTHX_ type, *(int *)ptr));
    //~ }; break;
    //~ case AFFIX_ARG_ENUM_UINT: {
    //~ SvSetSV(RETVAL, enum2sv(aTHX_ type, *(unsigned int *)ptr));
    //~ }; break;
    //~ case AFFIX_ARG_ENUM_CHAR: {
    //~ SvSetSV(RETVAL, enum2sv(aTHX_ type, (IV) * (char *)ptr));
    //~ }; break;
    default:
        croak("Unhandled type in ptr2sv: %s (%d)", type_as_str(type), type);
    }
    return RETVAL;
}

void *Xsv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed) {
    int16_t type = SvIV(type_sv);
    if (ptr == NULL) {
        //~ warn("safemalloc(%d)", _sizeof(aTHX_ type_sv));
        ptr = safemalloc(_sizeof(aTHX_ type_sv));
    }
    warn("sv2ptr(%s (%d), ..., %p, %s) at %s line %d", type_as_str(type), type, ptr,
         (packed ? "true" : "false"), __FILE__, __LINE__);
    //~ sv_dump(type_sv);
    sv_dump(data);
    switch (type) {
    case AFFIX_ARG_VOID: {
        if (!SvOK(data))
            Zero(ptr, 1, intptr_t);
        else if (sv_derived_from(data, "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(data));
            ptr = INT2PTR(DCpointer, tmp);
            Copy((DCpointer)(&data), ptr, 1, intptr_t);
        }
        else {
            size_t len;
            char *raw = SvPV(data, len);
            Renew(ptr, len + 1, char);
            Copy((DCpointer)raw, ptr, len + 1, char);
        }
        // else
        //     croak("Expected a subclass of Affix::Pointer");
    } break;
    case AFFIX_ARG_BOOL: {
        bool value = SvOK(data) ? SvTRUE(data) : (bool)0; // default to false
        Copy(&value, ptr, 1, bool);
    } break;
    case AFFIX_ARG_CHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            Renew(ptr, len + 1, char);
            Copy(value, ptr, len + 1, char);
        }
        else {
            char value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, char);
        }
    } break;
    case AFFIX_ARG_UCHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            Renew(ptr, len + 1, unsigned char);
            Copy(value, ptr, len + 1, unsigned char);
        }
        else {
            unsigned char value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, unsigned char);
        }
    } break;
    case AFFIX_ARG_WCHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            (void)SvPVutf8(data, len);
            wchar_t *value = utf2wchar(aTHX_ data, len + 1);
            len = wcslen(value);
            Renew(ptr, len + 1, wchar_t);
            Copy(value, ptr, len + 1, wchar_t);
        }
        else {
            wchar_t value = SvIOK(data) ? SvIV(data) : 0;
            // Renew(ptr, 1, wchar_t);
            Copy(&value, ptr, 1, wchar_t);
        }
    } break;
    case AFFIX_ARG_SHORT: {
        short value = SvOK(data) ? (short)SvIV(data) : 0;
        Copy(&value, ptr, 1, short);
    } break;
    case AFFIX_ARG_USHORT: {
        unsigned short value = SvOK(data) ? (unsigned short)SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned short);
    } break;
    case AFFIX_ARG_INT: {
        int value = SvOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, int);
    } break;
    case AFFIX_ARG_UINT: {
        unsigned int value = SvOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned int);
    } break;
    case AFFIX_ARG_LONG: {
        long value = SvOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, long);
    } break;
    case AFFIX_ARG_ULONG: {
        unsigned long value = SvOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned long);
    } break;
    case AFFIX_ARG_LONGLONG: {
        I64 value = SvOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, I64);
    } break;
    case AFFIX_ARG_ULONGLONG: {
        U64 value = SvOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, U64);
    } break;
    case AFFIX_ARG_FLOAT: {
        float value = SvOK(data) ? SvNV(data) : 0;
        Copy(&value, ptr, 1, float);
    } break;
    case AFFIX_ARG_DOUBLE: {
        double value = SvOK(data) ? SvNV(data) : 0;
        Copy(&value, ptr, 1, double);
        warn("Putting double with value == %f at %p", value, ptr);
        DumpHex(ptr, sizeof(double));
    } break;
    case AFFIX_ARG_CPOINTER: {
        if (0) {
            DCpointer value;
            DCpointer top = NULL;
            size_t depth = 0;
            do {
                depth++;
                type_sv = *hv_fetch(MUTABLE_HV(SvRV(type_sv)), "type", 4, false);
            } while (sv_derived_from((type_sv), "Affix::Type::Pointer"));
            sv2ptr(aTHX_ type_sv, data, value, packed);
            warn("value: %p", value);
            // Iterate through the levels of the pointer.
            for (size_t i = 0; i < depth; i++) {
                // value = *value;
                if (top == NULL) top = value;
                warn("deeper: %zu %p", i, top);
            }
            Copy(top, ptr, 1, intptr_t);
        }
        else {
            SV *subtype = *hv_fetch(MUTABLE_HV(SvRV(type_sv)), "type", 4, false);
            DCpointer target = safemalloc(_sizeof(aTHX_ subtype));
            warn("_sizeof(aTHX_ subtype) == %zu", _sizeof(aTHX_ subtype));
            sv2ptr(aTHX_ subtype, data, target, packed);
            PING;
            DumpHex(target, 16);
            Copy(&target, ptr, 1, intptr_t);
            DumpHex(ptr, 16);
            //~ croak("Fuck");
            PING;
            //~ DumpHex(target, 16);
        }
    } break;
    case AFFIX_ARG_ASCIISTR: {
        if (SvPOK(data)) {
            STRLEN len;
            const char *str = SvPV(data, len);
            DCpointer value;
            Newxz(value, len + 1, char);
            Copy(str, value, len, char);
            Copy(&value, ptr, 1, intptr_t);
        }
        else {
            const char *str = "";
            DCpointer value;
            Newxz(value, 1, char);
            Copy(str, value, 1, char);
            Copy(&value, ptr, 1, intptr_t);
        }
    } break;
    case AFFIX_ARG_UTF16STR: {
        if (SvPOK(data)) {
            STRLEN len;
            (void)SvPVutf8(data, len);
            wchar_t *str = utf2wchar(aTHX_ data, len + 1);

            //~ DumpHex(str, strlen(str_));
            DCpointer value;
            Newxz(value, len, wchar_t);
            Copy(str, value, len, wchar_t);
            Copy(&value, ptr, 1, intptr_t);
        }
        else
            Zero(ptr, 1, intptr_t);
    } break;
    //~ case AFFIX_ARG_CPPSTRUCT: {
    //~ HV *hv_ptr = MUTABLE_HV(SvRV(type));
    //~ SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
    //~ DCpointer value = safemalloc(_sizeof(aTHX_ * type_ptr));
    //~ if (SvOK(data)) sv2ptr(aTHX_ _instanceof(aTHX_ * type_ptr), data, value, packed);
    //~ Copy(&value, ptr, 1, intptr_t);
    //~ } break;
    case AFFIX_ARG_CSTRUCT: {
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type_sv));
            HV *hv_data = MUTABLE_HV(SvRV(data));
            SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
            //~ SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
            AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
            size_t field_count = av_count(av_fields);
            for (size_t i = 0; i < field_count; ++i) {
                SV **field = av_fetch(av_fields, i, 0);
                AV *name_type = MUTABLE_AV(SvRV(*field));
                SV **name_ptr = av_fetch(name_type, 0, 0);
                SV **type_ptr = av_fetch(name_type, 1, 0);
                char *key = SvPVbytex_nolen(*name_ptr);
                SV **_data = hv_fetch(hv_data, key, strlen(key), 1);
                if (_data != NULL) {
                    sv2ptr(aTHX_ * type_ptr, *_data,
                           INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr)), packed);
                }
            }
        }
    } break;
    case AFFIX_ARG_CUNION: {
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type_sv));
            HV *hv_data = MUTABLE_HV(SvRV(data));
            SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
            //~ SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
            AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
            size_t field_count = av_count(av_fields);
            for (size_t i = 0; i < field_count; ++i) {
                SV **field = av_fetch(av_fields, i, 0);
                AV *name_type = MUTABLE_AV(SvRV(*field));
                SV **name_ptr = av_fetch(name_type, 0, 0);
                SV **type_ptr = av_fetch(name_type, 1, 0);
                char *key = SvPVbytex_nolen(*name_ptr);
                SV **_data = hv_fetch(hv_data, key, strlen(key), 1);
                if (data != NULL && SvOK(*_data)) {
                    sv2ptr(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 1)),
                           INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr)), packed);
                    break;
                }
            }
        }
    } break;
    case AFFIX_ARG_CARRAY: {
        AV *elements = MUTABLE_AV(SvRV(data));

        HV *hv_ptr = MUTABLE_HV(SvRV(type_sv));

        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);

        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        hv_stores(hv_ptr, "dyn_size", newSVuv(av_count(elements)));

        size_t size = size_ptr != NULL && SvOK(*size_ptr) ? SvIV(*size_ptr) : av_count(elements);

        // hv_stores(hv_ptr, "size", newSViv(size));
        char *type_char = SvPVbytex_nolen(*type_ptr);

        switch (type_char[0]) {
        case AFFIX_ARG_CHAR:
        case AFFIX_ARG_UCHAR: {
            if (SvPOK(data)) {
                if (type_char[0] == AFFIX_ARG_CHAR) {
                    char *value = SvPV(data, size);
                    Copy(value, ptr, size, char);
                }
                else {
                    unsigned char *value = (unsigned char *)SvPV(data, size);
                    Copy(value, ptr, size, unsigned char);
                }
                break;
            }
        }
        // fall through
        default: {

            if (SvOK(SvRV(data)) && SvTYPE(SvRV(data)) != SVt_PVAV) croak("Expected an array");
            if (size_ptr != NULL && SvOK(*size_ptr)) {
                size_t av_len = av_count(elements);
                if (av_len != size)
                    croak("Expected and array of %zu elements; found %zu", size, av_len);
            }

            size_t el_len = _sizeof(aTHX_ * type_ptr);
            size_t pos = 0; // override
            for (size_t i = 0; i < size; ++i) {
                //~ warn("int[%d] of %d", i, size);
                //~ warn("Putting index %d into pointer plus %d", i, pos);
                sv2ptr(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)),
                       INT2PTR(DCpointer, PTR2IV(ptr) + pos), packed);
                pos += el_len;
            }
        }
            // return _sizeof(aTHX_ type);
        }
    } break;
    case AFFIX_ARG_CALLBACK: {
        PING;
        DCCallback *cb = NULL;
        HV *field = MUTABLE_HV(SvRV(type_sv)); // Make broad assumptions
        //~ SV **ret = hv_fetchs(field, "ret", 0);
        SV **args = hv_fetchs(field, "args", 0);
        SV **sig = hv_fetchs(field, "sig", 0);
        Callback *callback;
        PING;
        Newxz(callback, 1, Callback);
        callback->args = MUTABLE_AV(SvRV(*args));
        PING;
        size_t arg_count = av_count(callback->args);
        Newxz(callback->sig, arg_count, char);
        PING;
        for (size_t i = 0; i < arg_count; ++i) {
            PING;
            SV **type = av_fetch(callback->args, i, 0);
            callback->sig[i] = type_as_dc(SvIV(*type));
        }
        PING;
        callback->sig = SvPV_nolen(*sig);
        callback->sig_len = strchr(callback->sig, ')') - callback->sig;
        callback->ret = callback->sig[callback->sig_len + 1];
        callback->cv = SvREFCNT_inc(data);
        storeTHX(callback->perl);
        PING;
        cb = dcbNewCallback(callback->sig, cbHandler, callback);
        {
            PING;
            CallbackWrapper *hold;
            Newxz(hold, 1, CallbackWrapper);
            hold->cb = cb;
            Copy(hold, ptr, 1, DCpointer);
            PING;
        }
        PING;
    } break;
    default: {
        croak("%s (%d) is not a known type in sv2ptr(...)", type_as_str(type), type);
    }
    }
    PING;
    return ptr;
}
