#include "../Affix.h"

SV *ptr2av(pTHX_ DCpointer ptr, SV *type_sv) {
#if DEBUG
    warn("ptr2av(%p, %s)) at %s line %d", ptr, AXT_STRINGIFY(type_sv), __FILE__, __LINE__);
#endif
    PING;

    SV *retval = NULL;
    if (ptr == NULL) { retval = newSV(0); }
    else {
        PING;

#if DEBUG > 1
        // size_t field_size = AXT_SIZEOF(type_sv);
        // DumpHex(ptr, field_size);
#endif
        PING;

        AV *RETVAL_ = newAV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));
        SV *subtype = *hv_fetchs(_type, "type", 0);
        SV **size_ptr = hv_fetchs(_type, "size", 0);
        if (size_ptr == NULL) size_ptr = hv_fetchs(_type, "dyn_size", 0);
        size_t size = SvIV(*size_ptr);
        size_t el_len = AXT_SIZEOF(subtype);
        size_t pos = 0; // override
        if (sv_derived_from(subtype, "Affix::Types::Array")) {
            void **_ptr = (void **)ptr;
            for (size_t i = 0; i < size; ++i) {
                av_push(RETVAL_, ptr2sv(aTHX_ _ptr[i], subtype));
            }
        }
        else {
            for (size_t i = 0; i < size; ++i) {
                av_push(RETVAL_, ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + pos), subtype));
                pos += el_len;
            }
        }
        retval = newRV(MUTABLE_SV(RETVAL_));
    }
#if DEBUG
    warn("/ptr2sv(%p, %s (%d)) at %s line %d", ptr, AXT_STRINGIFY(type_sv), __FILE__, __LINE__);
    DD(retval);
#endif
    return retval;
}
SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    PING;
#if DEBUG
    warn("ptr2sv(%p, %s (%d)) at %s line %d", ptr, AXT_STRINGIFY(type_sv), __FILE__, __LINE__);
#endif
    PING;
    SV *retval = NULL;
    if (ptr == NULL) {
        warn("NULL pointer");
        retval = &PL_sv_undef;
    }
    else {
        PING;
#if DEBUG > 1
        // size_t field_size = AXT_SIZEOF(type_sv);
        // if (ptr != NULL) DumpHex(ptr, field_size);
#endif
        switch (SvIV(type_sv)) {
        case BOOL_FLAG: {
            retval = newSV(0);
            sv_setbool_mg(retval, (bool)*(bool *)ptr);
        } break;
        case CHAR_FLAG:
        case SCHAR_FLAG: {
            retval = newSV(0);
            sv_setsv(retval, newSVpv((char *)ptr, 1));
            (void)SvUPGRADE(retval, SVt_PVIV);
            SvIV_set(retval, ((IV) * (char *)ptr));
            SvIOK_on(retval);
        } break;
        case UCHAR_FLAG: {
            retval = newSV(0);
            sv_setsv(retval, newSVpv((char *)(unsigned char *)ptr, 1));
            (void)SvUPGRADE(retval, SVt_PVIV);
            SvIV_set(retval, ((UV) * (unsigned char *)ptr));
            SvIOK_on(retval);
        } break;
        case WCHAR_FLAG: {
            if (wcslen((wchar_t *)ptr)) {
                retval = wchar2utf(aTHX_(wchar_t *) ptr, 1 /*wcslen((wchar_t *)ptr)*/);
            }
        } break;
        case SHORT_FLAG: {
            retval = newSViv(*(short *)ptr);
        } break;
        case USHORT_FLAG: {
            retval = newSVuv(*(unsigned short *)ptr);
        } break;
        case INT_FLAG: {
            retval = newSViv(*(int *)ptr);
        } break;
        case UINT_FLAG: {
            retval = newSVuv(*(unsigned int *)ptr);
        } break;
        case LONG_FLAG: {
            retval = newSViv(*(long *)ptr);
        } break;
        case ULONG_FLAG: {
            retval = newSVuv(*(unsigned long *)ptr);
        } break;
        case LONGLONG_FLAG: {
            retval = newSViv(*(long long *)ptr);
        } break;
        case ULONGLONG_FLAG: {
            retval = newSVuv(*(unsigned long long *)ptr);
        } break;
        case FLOAT_FLAG: {
            retval = newSVnv(*(float *)ptr);
        } break;
        case DOUBLE_FLAG: {
            retval = newSVnv(*(double *)ptr);
        } break;
        case STRING_FLAG: {
            char *str = (char *)*(void **)ptr;
            STRLEN len = strlen(str);
            retval = len ? newSVpv(str, len) : &PL_sv_undef;
        } break;
        case WSTRING_FLAG: {
            if (ptr && wcslen((wchar_t *)ptr)) {
                retval = wchar2utf(aTHX_ * (wchar_t **)ptr, wcslen(*(wchar_t **)ptr));
            }
            else { retval = &PL_sv_undef; }
        } break;
        case STRUCT_FLAG:
        case CPPSTRUCT_FLAG: {
            retval = newSV(0);
            HV *RETVAL_ = newHV_mortal();
            HV *_type = MUTABLE_HV(SvRV(type_sv));
#if TIE_MAGIC
            SV *p = newSV(0);
            warn("WHERE");

            sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
            SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
            hv_store(MUTABLE_HV(SvRV(tie)), "pointer", 7, p, 0);
            hv_store(MUTABLE_HV(SvRV(tie)), "type", 4, newRV_inc(type_sv), 0);
            sv_bless(tie, gv_stashpv("Affix::Struct", TRUE));
            hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
            SvSetSV(retval, newRV(MUTABLE_SV(RETVAL_)));
#else
            AV *fields = MUTABLE_AV(SvRV(*hv_fetchs(_type, "fields", 0)));
            size_t field_count = av_count(fields);
            for (size_t i = 0; i < field_count; ++i) {
                AV *field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
                SV *name = *av_fetch(field, 0, 0);
                SV *subtype = *av_fetch(field, 1, 0);
                (void)hv_store_ent(
                    RETVAL_, name,
                    ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + AXT_OFFSET(subtype)), subtype),
                    0);
            }
            SvSetSV(retval, newRV(MUTABLE_SV(RETVAL_)));
            //~ sv_dump(MUTABLE_SV(RETVAL_));
            //~ sv_dump(retval);
#endif
        } break;
        case ARRAY_FLAG: {
            retval = ptr2av(aTHX_ ptr, type_sv);
        } break;
        case CODEREF_FLAG: {
            Callback *cb = (Callback *)dcbGetUserData((DCCallback *)((CallbackWrapper *)ptr)->cb);
            SvSetSV(retval, cb->cv);
        } break;
        case POINTER_FLAG: {
            SV *subtype_sv = *hv_fetch(MUTABLE_HV(SvRV(type_sv)), "type", 4, 0);
            int subtype = SvIV(subtype_sv);
            switch (subtype) {
            case CHAR_FLAG:
            case SCHAR_FLAG:
            case UCHAR_FLAG: {
                char *str = (char *)*(void **)&ptr;
                STRLEN len = strlen(str);
                retval = len ? newSVpv(str, len) : &PL_sv_undef;
            } break;
            case WCHAR_FLAG: {
                if (wcslen((wchar_t *)ptr)) {
                    retval = wchar2utf(aTHX_(wchar_t *) ptr, wcslen((wchar_t *)ptr));
                }
            } break;
            case VOID_FLAG: {
                retval = newSV(0);
                if (ptr != NULL) {
                    HV *_type = MUTABLE_HV(SvRV(type_sv));
                    SV **typedef_ptr = hv_fetch(_type, "class", 5, 0);
                    if (typedef_ptr != NULL) {
                        sv_setref_pv(retval, SvPV_nolen(*typedef_ptr), ptr);
                    }
                    else { sv_setref_pv(retval, "Affix::Pointer::Unmanaged", ptr); }
                }
            } break;
            //~ case POINTER_FLAG:{
            //~ retval = ptr2sv(aTHX_ *(void**)ptr, subtype_sv);
            //~ }break;
            //~ case STRUCT_FLAG:
            //~ case CPPSTRUCT_FLAG:
            //~ case UNION_FLAG: {
            //~ retval = ptr2sv(aTHX_ *(void**)ptr, subtype_sv);
            //~ }break;
            default: {
                retval = ptr2sv(aTHX_ ptr, subtype_sv);
            } break;
            }
        } break;
        case UNION_FLAG: {
            HV *RETVAL_ = newHV_mortal();
            warn("HEfdsafdasfdasfdsaRE");

            SV *p = newSV(0);
            sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
            SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
            hv_store(MUTABLE_HV(SvRV(tie)), "pointer", 7, p, 0);
            hv_store(MUTABLE_HV(SvRV(tie)), "type", 4, newRV_inc(type_sv), 0);
            sv_bless(tie, gv_stashpv("Affix::Union", TRUE));
            hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
            retval = newRV(MUTABLE_SV(RETVAL_));
        } break;
            /*

        #define WCHAR_FLAG 44
*/
        case SV_FLAG: {
            if (ptr == NULL) { retval = newSV(0); }
            else if (*(void **)ptr != NULL && SvOK(MUTABLE_SV(*(void **)ptr))) {
                retval = MUTABLE_SV(*(void **)ptr);
            }
        } break;
            /*
                    #define AFFIX_TYPE_REF 48
                        #define AFFIX_TYPE_STD_STRING 50
                        #define AFFIX_TYPE_INSTANCE_OF 52
                        */
        default:
            croak("Unhandled type: %s", AXT_STRINGIFY(type_sv));
        }
    }
    PING;
    // retval = sv_2mortal(retval);
    {
        PING;
        HV *_type = MUTABLE_HV(SvRV(type_sv));
        PING;
        SV **typedef_ptr = hv_fetch(_type, "class", 5, 0);
        PING;
        if (typedef_ptr != NULL) {
            PING;
            //~ sv_dump(*typedef_ptr);
            //~ sv_dump(retval);
            PING;
            // sv_bless(retval, SvSTASH(*typedef_ptr));
            PING;
            // retval = newSVrv(retval, SvPV_nolen(*typedef_ptr));
            //~ sv_dump(retval);
            //~ DD(retval);
        }
        PING;
        //
    }
    PING;
#if DEBUG
    warn("/ptr2sv(%p, %s) at %s line %d", ptr, type_sv, __FILE__, __LINE__);
    // DD(retval);
#endif
    return retval;
}

void *av2ptr(pTHX_ SV *type, AV *av_data, bool packed) {
    PING;
    DD(type);

    // DCpointer ret = NULL;
    HV *hv_type = MUTABLE_HV(SvRV(type));
    SV *subtype = *hv_fetchs(hv_type, "type", 0);
    int16_t i_type = SvIV(type);
#if DEBUG
    warn("av2ptr(%s, ..., %s) at %s line %d", AXT_STRINGIFY(type), (packed ? "true" : "false"),
         __FILE__, __LINE__);
#if DEBUG > 1
    DD(type);
    DD(MUTABLE_SV(av_data));
//~ sv_dump(av_data);
#endif
#endif

    SV **size_ptr = hv_fetchs(hv_type, "size", 0);
    size_t size = size_ptr != NULL && SvOK(*size_ptr) ? SvIV(*size_ptr) : av_count(av_data);
    // XXX: THIS IS INCORRECT!!!!!!!!!!!!!!
    DCpointer ret = safemalloc(AXT_SIZEOF(type) * (size + 1));
    if (size_ptr != NULL && SvOK(*size_ptr)) {
        size_t av_len = av_count(av_data);
        if (av_len != size) croak("Expected an array of %zu elements; found %zu", size, av_len);
    }
    size_t el_len = AXT_SIZEOF(subtype);
    PING;
    size_t pos = 0; // override
    PING;
    if (sv_derived_from(subtype, "Affix::Types::Pointer") ||
        sv_derived_from(subtype, "Affix::Types::Array")) {
        for (size_t i = 0; i < size; ++i) {
            DCpointer block = sv2ptr(aTHX_ subtype, *(av_fetch(av_data, i, 0)), packed);
            Move(&block, INT2PTR(DCpointer, PTR2IV(ret) + pos), AXT_SIZEOF(subtype), char);
            pos += el_len;
        }
    }
    else {
        for (size_t i = 0; i < size; ++i) {
            DCpointer block = sv2ptr(aTHX_ subtype, *(av_fetch(av_data, i, 0)), packed);
            Move(block, INT2PTR(DCpointer, PTR2IV(ret) + pos), AXT_SIZEOF(subtype), char);
            //~ safefree(block);
            pos += el_len;
            PING;
        }
    }
#if DEBUG
    warn("/av2ptr(%s, ..., %s) => %p at %s line %d", AXT_STRINGIFY(type),
         (packed ? "true" : "false"), ret, __FILE__, __LINE__);
#endif
    return ret;
}

void *sv2ptr(pTHX_ SV *type_sv, SV *data, bool packed) {
    DCpointer ret = NULL;
    int16_t type = SvIV(type_sv);
#if DEBUG
    warn("sv2ptr(%s, ..., %s) at %s line %d", AXT_STRINGIFY(type_sv), (packed ? "true" : "false"),
         __FILE__, __LINE__);
#if DEBUG > 1
    DD(data);
    DD(type_sv);
#endif
#endif
    switch (type) {
    case VOID_FLAG: {
        if (!SvOK(data)) {
            ret = NULL;
            //~ ret = safemalloc(sizeof(intptr_t));
            //~ Zero(ret, 1, intptr_t);
        }
        else if (sv_derived_from(data, "Affix::Pointer")) {
            croak("UGH!");
            IV tmp = SvIV((SV *)SvRV(data));
            ret = INT2PTR(DCpointer, tmp);
        }
        else {
            croak("UGH!");
            size_t len;
            char *raw = SvPV(data, len);
            Renew(ret, len + 1, char);
            Copy((DCpointer)raw, ret, len + 1, char);
        }
    } break;
    case BOOL_FLAG: {
        ret = safemalloc(BOOL_SIZE);
        bool value = SvOK(data) ? SvTRUE(data) : false; // default to false
        Copy(&value, ret, 1, bool);
    } break;
    case CHAR_FLAG: {
        if (!SvOK(data)) { ret = safecalloc(CHAR_SIZE, 1); }
        else if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            if (len) {
                ret = safecalloc(CHAR_SIZE, len + 1);
                Copy(value, ret, len + 1, char);
            }
            else
                ret = safecalloc(CHAR_SIZE, 1);
        }
        else {
            char value = SvIOK(data) ? SvIV(data) : 0;
            ret = safemalloc(CHAR_SIZE);
            Copy(&value, ret, 1, char);
        }
    } break;
    case UCHAR_FLAG: {
        if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            ret = safemalloc(UCHAR_SIZE * (len + 1));
            Copy(value, ret, len + 1, unsigned char);
        }
        else {
            unsigned char value = SvIOK(data) ? SvIV(data) : 0;
            ret = safemalloc(UCHAR_SIZE);
            Copy(&value, ret, 1, unsigned char);
        }
    } break;
    case WCHAR_FLAG: {
        if (SvPOK(data)) {
            STRLEN len;
            (void)SvPVutf8(data, len);
            wchar_t *value = utf2wchar(aTHX_ data, len + 1);
            len = wcslen(value);
            Renew(ret, len + 1, wchar_t);
            Copy(value, ret, len + 1, wchar_t);
        }
        else {
            wchar_t value = SvIOK(data) ? SvIV(data) : 0;
            // Renew(ptr, 1, wchar_t);
            Copy(&value, ret, 1, wchar_t);
        }
    } break;
    case SHORT_FLAG: {
        short value = SvOK(data) ? (short)SvIV(data) : 0;
        ret = safemalloc(sizeof(short));
        Copy(&value, ret, 1, short);
    } break;
    case USHORT_FLAG: {
        unsigned short value = SvOK(data) ? (unsigned short)SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned short));
        Copy(&value, ret, 1, unsigned short);
    } break;
    case INT_FLAG: {
        int value = SvOK(data) ? SvIV(data) : 0;
        ret = safemalloc(INTSIZE);
        Copy(&value, ret, 1, int);
    } break;
    case UINT_FLAG: {
        unsigned int value = SvOK(data) ? SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned int));
        Copy(&value, ret, 1, unsigned int);
    } break;
    case LONG_FLAG: {
        long value = SvOK(data) ? SvIV(data) : 0;
        ret = safemalloc(sizeof(long));
        Copy(&value, ret, 1, long);
    } break;
    case ULONG_FLAG: {
        unsigned long value = SvOK(data) ? SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned long));
        Copy(&value, ret, 1, unsigned long);
    } break;
    case LONGLONG_FLAG: {
        I64 value = SvOK(data) ? SvIV(data) : 0;
        ret = safemalloc(sizeof(long long));
        Copy(&value, ret, 1, I64);
    } break;
    case ULONGLONG_FLAG: {
        U64 value = SvOK(data) ? SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned long long));
        Copy(&value, ret, 1, U64);
    } break;
    case FLOAT_FLAG: {
        float value = SvOK(data) ? SvNV(data) : 0;
        ret = safemalloc(sizeof(float));
        Copy(&value, ret, 1, float);
    } break;
    case DOUBLE_FLAG: {
        double value = SvOK(data) ? SvNV(data) : 0;
        ret = safemalloc(sizeof(double));
        Copy(&value, ret, 1, double);
    } break;
    case POINTER_FLAG: {
        HV *type_hv = MUTABLE_HV(SvRV(type_sv));
        SV *subtype = *hv_fetchs(type_hv, "type", 0);
        if (!SvOK(data)) { ret = safecalloc(AXT_SIZEOF(subtype), 1); }
        else if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
            ret = av2ptr(aTHX_ type_sv, MUTABLE_AV(SvRV(data)), packed);
        }
        else {
            DCpointer block = sv2ptr(aTHX_ subtype, data, packed);
            ret = safemalloc(INTPTR_T_SIZE);
            Copy(block, ret, 1, intptr_t);
            safefree(block);
        }
    } break;
    case STRING_FLAG: {
        if (SvPOK(data)) {
            STRLEN len;
            const char *str = SvPV(data, len);
            ret = safemalloc(INTPTR_T_SIZE);
            DCpointer value;
            Newxz(value, len + 1, char);
            Copy(str, value, len, char);
            Copy(&value, ret, 1, intptr_t);
        }
        else {
            const char *str = "";
            DCpointer value;
            ret = safemalloc(CHAR_SIZE);
            Newxz(value, 1, char);
            Copy(str, value, 1, char);
            Copy(&value, ret, 1, intptr_t);
        }
    } break;
    case WSTRING_FLAG: {
        ret = safemalloc(INTPTR_T_SIZE);
        if (SvPOK(data)) {
            STRLEN len;
            (void)SvPVutf8(data, len);
            wchar_t *str = utf2wchar(aTHX_ data, len + 1);
            // safemalloc(len+1 * WCHAR_T_SIZE);
            //~ DumpHex(str, strlen(str_));
            DCpointer value;
            Newxz(value, len, wchar_t);
            Copy(str, value, len, wchar_t);
            Copy(&value, ret, 1, intptr_t);
        }
        else {
            // ret = safemalloc(0);
            Zero(ret, 1, intptr_t);
        }
    } break;
    case STRUCT_FLAG: {
        ret = safemalloc(AXT_SIZEOF(type_sv));
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
                    DCpointer block = sv2ptr(aTHX_ * type_ptr, *_data, packed);
                    Move(block, INT2PTR(DCpointer, PTR2IV(ret) + AXT_OFFSET(*type_ptr)),
                         AXT_SIZEOF(type_ptr), char);
                    safefree(block);
                }
            }
        }
        else
            Zero(ret, 1, intptr_t);
    } break;
    case UNION_FLAG: {
        ret = safemalloc(INTPTR_T_SIZE);
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
                    DCpointer block =
                        sv2ptr(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 1)), packed);
                    Move(block, INT2PTR(DCpointer, PTR2IV(ret) + AXT_OFFSET(*type_ptr)),
                         AXT_SIZEOF(*type_ptr), char);
                    safefree(block);
                    break;
                }
            }
        }
        else
            Zero(ret, 1, intptr_t);
    } break;
    case ARRAY_FLAG: {
        ret = safemalloc(AXT_SIZEOF(type_sv));
        if (SvOK(data)) {
            AV *elements;
            PING;
            if (SvPOK(data)) { // char[]
                elements = newAV_mortal();
                STRLEN len;
                char *str = SvPV(data, len);
                for (size_t i = 0; i < len; ++i) {
                    av_push(elements, newSViv(str[i]));
                }
            }
            else
                elements = MUTABLE_AV(SvRV(data));
            HV *hv_ptr = MUTABLE_HV(SvRV(type_sv));
            hv_stores(hv_ptr, "dyn_size", newSVuv(av_count(elements)));
            ret = av2ptr(aTHX_ type_sv, elements, packed);
        }
        else
            Zero(ret, 1, intptr_t);
    } break;
    case CODEREF_FLAG: {
        if (SvOK(data)) {
            PING;
            HV *field = MUTABLE_HV(SvRV(type_sv)); // Make broad assumptions
            //~ SV **ret = hv_fetchs(field, "ret", 0);
            SV **args = hv_fetchs(field, "args", 0);
            SV **sig = hv_fetchs(field, "sig", 0);
            Callback *callback;
            Newxz(callback, 1, Callback);
            callback->arg_info = MUTABLE_AV(SvRV(*args));
            size_t arg_count = av_count(callback->arg_info);
            Newxz(callback->sig, arg_count, char);
            for (size_t i = 0; i < arg_count; ++i) {
                callback->sig[i] = type_as_dc(SvIV(*av_fetch(callback->arg_info, i, 0)));
            }
            callback->sig = SvPV_nolen(*sig);
            callback->sig_len = strchr(callback->sig, ')') - callback->sig;
            callback->ret = callback->sig[callback->sig_len + 1];
            callback->cv = SvREFCNT_inc(data);
            storeTHX(callback->perl);
            PING;
            Newxz(ret, 1, CallbackWrapper);
            ((CallbackWrapper *)ret)->cb = dcbNewCallback(callback->sig, cbHandler, callback);
        }
        else { Newxz(ret, 1, intptr_t); }
    } break;
    case SV_FLAG: {
        if (SvOK(data)) {
            ret = safemalloc(INTPTR_T_SIZE);
            SvREFCNT_inc(data); // TODO: This might leak; I'm just being lazy
            DCpointer value = (DCpointer)data;
            Renew(ret, 1, intptr_t);
            Copy(&value, ret, 1, intptr_t);
            //~ DD(data);
        }
    } break;
    default: {
        croak("%s is not a known type in sv2ptr(...)", AXT_STRINGIFY(type_sv));
    }
    }
#if DEBUG
    warn("/sv2ptr(%s, ..., %s) => %p at %s line %d", AXT_STRINGIFY(type_sv),
         (packed ? "true" : "false"), ret, __FILE__, __LINE__);
#endif
    return ret;
}
