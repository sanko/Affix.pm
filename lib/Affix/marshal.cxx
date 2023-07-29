#include "../Affix.h"

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    if (ptr == NULL) return newSV(0);
    SV *RETVAL = newSV(1); // sv_newmortal();
    int16_t type = SvIV(type_sv);
//~ {
#if DEBUG
    warn("ptr2sv(%p, %s (%d)) at %s line %d", ptr, type_as_str(type), type, __FILE__, __LINE__);
#endif
    //~ if (type != AFFIX_TYPE_VOID) {
    //~ size_t l = _sizeof(type_sv);
    //~ DumpHex(ptr, l);
    //~ }
    //~ }
    switch (type) {
    case AFFIX_TYPE_SV: {
        if (ptr == NULL) { RETVAL = newSV(0); }
        else if (*(void **)ptr != NULL && SvOK(MUTABLE_SV(*(void **)ptr))) {
            RETVAL = MUTABLE_SV(*(void **)ptr);
        }
    } break;
    case AFFIX_TYPE_VOID: {
        if (ptr == NULL) { RETVAL = newSV(0); }
        else { sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", ptr); }
    } break;
    case AFFIX_TYPE_BOOL:
        sv_setbool_mg(RETVAL, (bool)*(bool *)ptr);
        break;
    case AFFIX_TYPE_CHAR:
    case AFFIX_TYPE_UCHAR:
        sv_setsv(RETVAL, newSVpv((char *)ptr, 0));
        (void)SvUPGRADE(RETVAL, SVt_PVIV);
        SvIV_set(RETVAL, ((IV) * (char *)ptr));
        SvIOK_on(RETVAL);
        break;
    case AFFIX_TYPE_WCHAR: {
        if (wcslen((wchar_t *)ptr)) {
            RETVAL = wchar2utf(aTHX_(wchar_t *) ptr, wcslen((wchar_t *)ptr));
        }
    } break;
    case AFFIX_TYPE_SHORT:
        sv_setiv(RETVAL, *(short *)ptr);
        break;
    case AFFIX_TYPE_USHORT:
        sv_setuv(RETVAL, *(unsigned short *)ptr);
        break;
    case AFFIX_TYPE_INT:
        sv_setiv(RETVAL, *(int *)ptr);
        break;
    case AFFIX_TYPE_UINT:
        sv_setuv(RETVAL, *(unsigned int *)ptr);
        break;
    case AFFIX_TYPE_LONG:
        sv_setiv(RETVAL, *(long *)ptr);
        break;
    case AFFIX_TYPE_ULONG:
        sv_setuv(RETVAL, *(unsigned long *)ptr);
        break;
    case AFFIX_TYPE_LONGLONG:
        sv_setiv(RETVAL, *(I64 *)ptr);
        break;
    case AFFIX_TYPE_ULONGLONG:
        sv_setuv(RETVAL, *(U64 *)ptr);
        break;
    case AFFIX_TYPE_FLOAT:
        sv_setnv(RETVAL, *(float *)ptr);
        break;
    case AFFIX_TYPE_DOUBLE:
        sv_setnv(RETVAL, *(double *)ptr);
        break;
    case AFFIX_TYPE_CPOINTER:
    case AFFIX_TYPE_REF: {
        SV *subtype = *hv_fetchs(MUTABLE_HV(SvRV(type_sv)), "type", 0);
        if (sv_derived_from(type_sv, "Affix::Type::InstanceOf")) {
            if (ptr == NULL) { RETVAL = newSV(0); }
            else {
                SV *cls = *hv_fetchs(MUTABLE_HV(SvRV(type_sv)), "class", 0);
                sv_setref_pv(RETVAL, SvPV_nolen(cls), ptr);
            }
        }
        else if (sv_derived_from(subtype, "Affix::Type::Pointer") ||
                 sv_derived_from(subtype, "Affix::Type::Array")) {
            if (ptr != NULL) { SvSetSV(RETVAL, ptr2sv(aTHX_ * (void **)ptr, subtype)); }
        }
        else {
            SV *subtype = *hv_fetchs(MUTABLE_HV(SvRV(type_sv)), "type", 0);
            SvSetSV(RETVAL, ptr2sv(aTHX_ ptr, subtype));
        }
    } break;
    case AFFIX_TYPE_ASCIISTR:
        if (*(char **)ptr) sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
        break;
    case AFFIX_TYPE_UTF16STR: {
        if (ptr && wcslen((wchar_t *)ptr)) {
            RETVAL = wchar2utf(aTHX_ * (wchar_t **)ptr, wcslen(*(wchar_t **)ptr));
        }
        else { sv_set_undef(RETVAL); }
    } break;
    case AFFIX_TYPE_CARRAY: {
        PING;
        AV *RETVAL_ = newAV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));
        SV *subtype = *hv_fetchs(_type, "type", 0);
        PING;
        if (sv_derived_from(subtype, "Affix::Type::Char")) { PING; }
        else {
            PING;
            SV **size_ptr = hv_fetchs(_type, "size", 0);
            if (size_ptr == NULL) size_ptr = hv_fetchs(_type, "dyn_size", 0);
            size_t size = SvIV(*size_ptr);
            PING;
            size_t el_len = _sizeof(aTHX_ subtype);
            size_t pos = 0; // override
            for (size_t i = 0; i < size; ++i) {
                //~ warn("int[%d] of %d", i, size);
                //~ warn("Putting index %d into pointer plus %d", i, pos);
                SV *hold = ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + pos), subtype);
                // sv_dump(hold);
                av_push(RETVAL_, hold);
                pos += el_len;
                //~ warn("i: %d, pos == %d, %p", i, pos, INT2PTR(DCpointer, PTR2IV(ptr) + pos));
            }
        }
        PING;

        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
        PING;

    } break;
    case AFFIX_TYPE_CSTRUCT: {
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
    case AFFIX_TYPE_CUNION: {
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
    case AFFIX_TYPE_CALLBACK: {
        CallbackWrapper *p = (CallbackWrapper *)ptr;
        Callback *cb = (Callback *)dcbGetUserData((DCCallback *)p->cb);
        SvSetSV(RETVAL, cb->cv);
    } break;
    default:
        croak("Unhandled type in ptr2sv: %s (%d)", type_as_str(type), type);
    }
#if DEBUG
    warn("/ptr2sv(%p, %s (%d)) at %s line %d", ptr, type_as_str(type), type, __FILE__, __LINE__);
#endif
    return RETVAL;
}

void *sv2ptr(pTHX_ SV *type_sv, SV *data, bool packed) {
    DCpointer ret = NULL;
    int16_t type = SvIV(type_sv);
#if DEBUG
    warn("sv2ptr(%s (%d), ..., %s) at %s line %d", type_as_str(type), type,
         (packed ? "true" : "false"), __FILE__, __LINE__);
#if DEBUG > 1
    sv_dump(type_sv);
#endif
#endif
    switch (type) {
    case AFFIX_TYPE_VOID: {
        if (!SvOK(data)) {
            ret = safemalloc(sizeof(intptr_t));
            Zero(ret, 1, intptr_t);
        }
        else if (sv_derived_from(data, "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(data));
            ret = INT2PTR(DCpointer, tmp);
        }
        else {
            size_t len;
            char *raw = SvPV(data, len);
            Renew(ret, len + 1, char);
            Copy((DCpointer)raw, ret, len + 1, char);
        }
    } break;
    case AFFIX_TYPE_BOOL: {
        ret = safemalloc(BOOL_SIZE);
        bool value = SvOK(data) ? SvTRUE(data) : false; // default to false
        Copy(&value, ret, 1, bool);
    } break;
    case AFFIX_TYPE_CHAR: {
        if (SvPOK(data)) {
            PING;
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            ret = safemalloc(sizeof(char) * (len + 1));
            Copy(value, ret, len + 1, char);
            DumpHex(ret, len);
        }
        else {
            PING;
            char value = SvIOK(data) ? SvIV(data) : 0;
            ret = safemalloc(sizeof(char));
            Copy(&value, ret, 1, char);
        }
    } break;
    case AFFIX_TYPE_UCHAR: {
        if (SvPOK(data)) {
            STRLEN len;
            DCpointer value = (DCpointer)SvPV(data, len);
            ret = safemalloc(sizeof(unsigned char) * (len + 1));
            Copy(value, ret, len + 1, unsigned char);
        }
        else {
            unsigned char value = SvIOK(data) ? SvIV(data) : 0;
            ret = safemalloc(sizeof(unsigned char));
            Copy(&value, ret, 1, unsigned char);
        }
    } break;
    case AFFIX_TYPE_WCHAR: {
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
    case AFFIX_TYPE_SHORT: {
        short value = SvOK(data) ? (short)SvIV(data) : 0;
        ret = safemalloc(sizeof(short));
        Copy(&value, ret, 1, short);
    } break;
    case AFFIX_TYPE_USHORT: {
        unsigned short value = SvOK(data) ? (unsigned short)SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned short));
        Copy(&value, ret, 1, unsigned short);
    } break;
    case AFFIX_TYPE_INT: {
        int value = SvOK(data) ? SvIV(data) : 0;
        ret = safemalloc(INTSIZE);
        Copy(&value, ret, 1, int);
    } break;
    case AFFIX_TYPE_UINT: {
        unsigned int value = SvOK(data) ? SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned int));
        Copy(&value, ret, 1, unsigned int);
    } break;
    case AFFIX_TYPE_LONG: {
        long value = SvOK(data) ? SvIV(data) : 0;
        ret = safemalloc(sizeof(long));
        Copy(&value, ret, 1, long);
    } break;
    case AFFIX_TYPE_ULONG: {
        unsigned long value = SvOK(data) ? SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned long));

        Copy(&value, ret, 1, unsigned long);
    } break;
    case AFFIX_TYPE_LONGLONG: {
        I64 value = SvOK(data) ? SvIV(data) : 0;
        ret = safemalloc(sizeof(long long));
        Copy(&value, ret, 1, I64);
    } break;
    case AFFIX_TYPE_ULONGLONG: {
        U64 value = SvOK(data) ? SvUV(data) : 0;
        ret = safemalloc(sizeof(unsigned long long));
        Copy(&value, ret, 1, U64);
    } break;
    case AFFIX_TYPE_FLOAT: {
        float value = SvOK(data) ? SvNV(data) : 0;
        ret = safemalloc(sizeof(float));
        Copy(&value, ret, 1, float);
    } break;
    case AFFIX_TYPE_DOUBLE: {
        double value = SvOK(data) ? SvNV(data) : 0;
        ret = safemalloc(sizeof(double));
        Copy(&value, ret, 1, double);
    } break;
    case AFFIX_TYPE_CPOINTER:
    case AFFIX_TYPE_REF: {
        HV *type_hv = MUTABLE_HV(SvRV(type_sv));
        SV *subtype = *hv_fetchs(type_hv, "type", 0);
        //~ if (sv_derived_from(subtype, "Affix::Type::Char")) {
        //~ if (SvPOK(data)) {
        //~ STRLEN len;
        //~ void *hold = SvPV(data, len);
        //~ Renew(ret, len + 1, char);
        //~ Copy(hold, ret, len + 1, char);
        //~ }
        //~ else {
        //~ char value = SvIOK(data) ? SvIV(data) : 0;
        //~ Copy(&value, ret, 1, char);
        //~ }
        //~ }
        //~ else
        if (!(sv_derived_from(subtype, "Affix::Type::Pointer") ||
              sv_derived_from(subtype, "Affix::Type::Array"))) {
            ret = sv2ptr(aTHX_ subtype, data, packed);
        }
        else {
            DCpointer block = sv2ptr(aTHX_ subtype, data, packed);
            ret = safemalloc(INTPTR_T_SIZE);
            Copy(&block, ret, 1, intptr_t);
            // safefree(block);
        }
    } break;
    case AFFIX_TYPE_ASCIISTR: {
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
            ret = safemalloc(sizeof(char));
            Newxz(value, 1, char);
            Copy(str, value, 1, char);
            Copy(&value, ret, 1, intptr_t);
        }
    } break;
    case AFFIX_TYPE_UTF16STR: {
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
    case AFFIX_TYPE_CSTRUCT: {
        ret = safemalloc(_sizeof(aTHX_ type_sv));
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
                    Move(block, INT2PTR(DCpointer, PTR2IV(ret) + _offsetof(aTHX_ * type_ptr)),
                         _sizeof(aTHX_ * type_ptr), char);
                    safefree(block);
                }
            }
        }
        else
            Zero(ret, 1, intptr_t);
    } break;
    case AFFIX_TYPE_CUNION: {
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
                    Move(block, INT2PTR(DCpointer, PTR2IV(ret) + _offsetof(aTHX_ * type_ptr)),
                         _sizeof(aTHX_ * type_ptr), char);
                    safefree(block);
                    break;
                }
            }
        }
        else
            Zero(ret, 1, intptr_t);
    } break;
    case AFFIX_TYPE_CARRAY: {
        PING;
        ret = safemalloc(_sizeof(aTHX_ type_sv));
        if (SvOK(data)) {
            AV *elements;
            PING;
            if (SvPOK(data)) {
                PING;

                elements = newAV_mortal();
                STRLEN len;
                char *str = SvPV(data, len);
                for (size_t i = 0; i < len; ++i) {
                    av_push(elements, newSViv(str[i]));
                }
            }
            else
                elements = MUTABLE_AV(SvRV(data));
            PING;
            HV *hv_ptr = MUTABLE_HV(SvRV(type_sv));
            PING;
            SV *subtype = *hv_fetchs(hv_ptr, "type", 0);
            SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
            PING;
            hv_stores(hv_ptr, "dyn_size", newSVuv(av_count(elements)));
            PING;
            size_t size =
                size_ptr != NULL && SvOK(*size_ptr) ? SvIV(*size_ptr) : av_count(elements);
            PING;
            // hv_stores(type_hv, "size", newSViv(size));
            PING;
            switch (SvIV(subtype)) {
            case AFFIX_TYPE_CHAR:
            case AFFIX_TYPE_UCHAR: {
                PING;
                if (SvPOK(data)) {
                    if (SvIV(subtype) == AFFIX_TYPE_CHAR) {
                        char *value = SvPV(data, size);
                        ret = safemalloc(sizeof(char) * size);
                        Copy(value, ret, size, char);
                    }
                    else {
                        unsigned char *value = (unsigned char *)SvPV(data, size);
                        ret = safemalloc(sizeof(unsigned char) * size);
                        Copy(value, ret, size, unsigned char);
                    }
                    break;
                }
            }
            // fall through
            default: {
                PING;
                if (SvOK(SvRV(data)) && SvTYPE(SvRV(data)) != SVt_PVAV) croak("Expected an array");
                if (size_ptr != NULL && SvOK(*size_ptr)) {
                    size_t av_len = av_count(elements);
                    if (av_len != size)
                        croak("Expected and array of %zu elements; found %zu", size, av_len);
                }
                size_t el_len = _sizeof(aTHX_ subtype);
                size_t pos = 0; // override
                for (size_t i = 0; i < size; ++i) {
                    //~ warn("int[%d] of %d", i, size);
                    //~ warn("Putting index %d into pointer plus %d", i, pos);
                    DCpointer block = sv2ptr(aTHX_ subtype, *(av_fetch(elements, i, 0)), packed);

                    Move(block, INT2PTR(DCpointer, PTR2IV(ret) + pos), _sizeof(aTHX_ subtype),
                         char);
                    safefree(block);

                    pos += el_len;
                    //~ warn("i: %d, pos == %d, %p", i, pos, INT2PTR(DCpointer, PTR2IV(ptr) + pos));
                }
            }
                // return _sizeof(aTHX_ type);
            }
        }
        else
            Zero(ret, 1, intptr_t);

        PING;
    } break;
    case AFFIX_TYPE_CALLBACK: {
        ret = safemalloc(INTPTR_T_SIZE);
        if (SvOK(data)) {
            DCCallback *cb = NULL;
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
                SV **type = av_fetch(callback->arg_info, i, 0);
                callback->sig[i] = type_as_dc(SvIV(*type));
            }
            callback->sig = SvPV_nolen(*sig);
            callback->sig_len = strchr(callback->sig, ')') - callback->sig;
            callback->ret = callback->sig[callback->sig_len + 1];
            callback->cv = SvREFCNT_inc(data);
            storeTHX(callback->perl);
            PING;
            cb = dcbNewCallback(callback->sig, cbHandler, callback);
            {
                CallbackWrapper *hold;
                Newxz(hold, 1, CallbackWrapper);
                hold->cb = cb;
                Copy(hold, ret, 1, DCpointer);
            }
        }
        else
            Zero(ret, 1, intptr_t);
    } break;
    case AFFIX_TYPE_SV: {
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
        croak("%s (%d) is not a known type in sv2ptr(...)", type_as_str(type), type);
    }
    }
#if DEBUG
    warn("/sv2ptr(%s (%d), ..., %s) at %s line %d", type_as_str(type), type,
         (packed ? "true" : "false"), __FILE__, __LINE__);
#endif
    return ret;
}
