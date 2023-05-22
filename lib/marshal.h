#define PERL_NO_GET_CONTEXT 1 /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

#include <wchar.h>

#if __WIN32
#include <windows.h>
#endif

static int wstring2ptr(pTHX_ SV *sv, void *p) {
    wchar_t *str = (wchar_t *)SvPVutf8x(sv, PL_na);
    int len = wcslen(str);
#if __WIN32
    len = MultiByteToWideChar(CP_UTF8, 0, SvPV(sv, PL_na), -1, str, len);
    if (len == 0) {
        DWORD err = GetLastError();
        croak("MultiByteToWideChar failed with error %d", err);
    }
#endif
    memcpy(p, str, len * sizeof(wchar_t));
    return len * sizeof(wchar_t);
}

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    if (ptr == NULL) croak("Fuck");
    SV *RETVAL = newSV(0);
    int16_t type = SvIV(type_sv);
    //~ {
    //~ warn("ptr2sv(%p, %s) at %s line %d", ptr, type_as_str(type), __FILE__, __LINE__);
    //~ if (type != AFFIX_ARG_VOID) {
    //~ size_t l = _sizeof(type_sv);
    //~ DumpHex(ptr, l);
    //~ }
    //~ }
    switch (type & AFFIX_ARG_TYPE_MASK) {
    case AFFIX_ARG_VOID:
        sv_setref_pv(RETVAL, "Affix::Pointer", ptr);
        break;
    case AFFIX_ARG_BOOL:
        sv_setbool_mg(RETVAL, (bool)*(bool *)ptr);
        break;
    case AFFIX_ARG_CHAR:
        sv_setiv(RETVAL, (IV) * (char *)ptr);
        break;
    case AFFIX_ARG_UCHAR:
        sv_setuv(RETVAL, (UV) * (unsigned char *)ptr);
        break;
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
        croak("POINTER!!!!!!!!");
        //~ SV *subtype;
        //~ if (sv_derived_from(type, "Affix::Type::Pointer"))
        //~ subtype = *hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0);
        //~ else
        //~ subtype = type;
        //~ char *_subtype = SvPV_nolen(subtype);
        //~ if (_subtype[0] == AFFIX_ARG_VOID) {
        //~ SV *RETVALSV = newSV(1); // sv_newmortal();
        //~ SvSetSV(RETVAL, sv_setref_pv(RETVALSV, "Affix::Pointer", *(DCpointer *)ptr));
        //~ }
        //~ else { SvSetSV(RETVAL, ptr2sv(aTHX_ ptr, subtype)); }
    } break;
    case AFFIX_ARG_ASCIISTR:
        sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
        // PING;
        break;
    //~ case AFFIX_ARG_UTF16STR: {
    //~ size_t len = wcslen((const wchar_t *)ptr) * WCHAR_T_SIZE;
    //~ RETVAL =
    //~ call_encoding(aTHX_ "decode", find_encoding(aTHX), newSVpv((char *)ptr, len), NULL);
    //~ } break;
    //~ case AFFIX_ARG_WCHAR: {
    //~ SV *container = newSV(0);
    //~ RETVAL = newSVpvs("");
    //~ const char *pat = "W";
    //~ switch (WCHAR_T_SIZE) {
    //~ case I8SIZE:
    //~ sv_setiv(container, (IV) * (char *)ptr);
    //~ break;
    //~ case SHORTSIZE:
    //~ sv_setiv(container, (IV) * (short *)ptr);
    //~ break;
    //~ case INTSIZE:
    //~ sv_setiv(container, *(int *)ptr);
    //~ break;
    //~ default:
    //~ croak("Invalid wchar_t size for argument!");
    //~ }
    //~ sv_2mortal(container);
    //~ packlist(RETVAL, pat, pat + 1, &container, &container + 1);
    //~ } break;
    case AFFIX_ARG_CARRAY: {
        AV *RETVAL_ = newAV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type_sv));
        SV *subtype = *hv_fetchs(_type, "type", 0);
        SV **size = hv_fetchs(_type, "size", 0);
        size_t pos = PTR2IV(ptr);
        size_t sof = _sizeof(aTHX_ subtype);
        size_t av_len;
        if (SvOK(*size))
            av_len = SvIV(*size);
        else
            av_len = SvIV(*hv_fetchs(_type, "size_", 0)) + 1;
        for (size_t i = 0; i < av_len; ++i) {
            av_push(RETVAL_, ptr2sv(aTHX_ INT2PTR(DCpointer, pos), subtype));
            pos += sof;
        }
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case AFFIX_ARG_CSTRUCT: {
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
    } break;
    case AFFIX_ARG_CUNION: {
        HV *RETVAL_ = newHV_mortal();

        if (0) {

            HV *_type = MUTABLE_HV(SvRV(type_sv));
            AV *fields = MUTABLE_AV(SvRV(*hv_fetchs(_type, "fields", 0)));
            size_t field_count = av_count(fields);

            PING;
            (void)hv_store(RETVAL_, "_*_", 3, newSViv(PTR2IV(ptr)), 0);

            for (size_t i = 0; i < field_count; ++i) {
                warn("%d of %d fields .fdsafdsafdsafdasfdsafdsa", i, field_count);
                PING;

                AV *field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
                SV *name = *av_fetch(field, 0, 0);
                PING;

                SV *subtype = *av_fetch(field, 1, 0);
                SV *_field = newSV(1);
                PING;

                MAGIC *mg = sv_magicext(_field, NULL, PERL_MAGIC_ext, &union_vtbl, NULL, 0);
                PING;

                {
                    var_ptr *_ptr;
                    Newx(_ptr, 1, var_ptr);
                    _ptr->ptr = ptr;
                    _ptr->type_sv = newSVsv(subtype);
                    mg->mg_ptr = (char *)_ptr;
                }
                PING;

                (void)hv_store_ent(RETVAL_, name, _field, 0);
                PING;
            }
            PING;

            // If the SV already has union magic, grab it and take the pointer

            SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
            //~ sv_dump(RETVAL);
            //~ SvNV_set(RETVAL, PTR2IV(ptr));
            //~ SvNOK_on(RETVAL);
            PING;

            //~ sv_dump(RETVAL);
            PING;

            // DumpHex(ptr, sizeof(unsigned int));
            //  sv_setref_pv(RETVAL, "Affix::Union", ptr);
            //~ sv_dump(RETVAL);
            //  croak("blah");
        }
        else {
            SV *p = newSV(0);
            sv_setref_pv(p, "Affix::Pointer", ptr);
            SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
            hv_store((HV *)SvRV(tie), "pointer", 7, p, 0);
            hv_store((HV *)SvRV(tie), "type", 4, newRV_inc(type_sv), 0);
            sv_bless(tie, gv_stashpv("Affix::Union", TRUE));
            hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
            SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
        }
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
        croak("Oh, this is unexpected: %d", type);
    }
    return RETVAL;
}

void sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed) {
    SV *RETVAL = newSV(0);
    int16_t type = SvIV(type_sv);
    //~ warn("sv2ptr(%s (%d), ..., %p, %s) at %s line %d", type_as_str(type), type, ptr,
    //~ (packed ? "true" : "false"), __FILE__, __LINE__);
    switch (type & AFFIX_ARG_TYPE_MASK) {
    case AFFIX_ARG_VOID: {
        if (!SvOK(data))
            Zero(ptr, 1, intptr_t);
        else if (sv_derived_from(data, "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(data));
            ptr = INT2PTR(DCpointer, tmp);
            Copy((DCpointer)(&data), ptr, 1, intptr_t);
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
            char *value = SvPV_nolen(data);
            Copy(&value, ptr, 1, char);
        }
        else {
            char value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, char);
        }
    } break;
    case AFFIX_ARG_UCHAR: {
        if (SvPOK(data)) {
            unsigned char *value = (unsigned char *)SvPV_nolen(data);
            Copy(&value, ptr, 1, unsigned char);
        }
        else {
            unsigned char value = SvUOK(data) ? SvUV(data) : 0;
            Copy(&value, ptr, 1, unsigned char);
        }
    } break;
    case AFFIX_ARG_SHORT: {
        short value = SvIOK(data) ? (short)SvIV(data) : 0;
        Copy(&value, ptr, 1, short);
    } break;
    case AFFIX_ARG_USHORT: {
        unsigned short value = SvUOK(data) ? (unsigned short)SvIV(data) : 0;
        Copy(&value, ptr, 1, unsigned short);
    } break;
    case AFFIX_ARG_INT: {
        int value = SvIOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, int);
    } break;
    case AFFIX_ARG_UINT: {
        unsigned int value = SvUOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned int);
    } break;
    case AFFIX_ARG_LONG: {
        long value = SvIOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, long);
    } break;
    case AFFIX_ARG_ULONG: {
        unsigned long value = SvUOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, unsigned long);
    } break;
    case AFFIX_ARG_LONGLONG: {
        I64 value = SvIOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, I64);
    } break;
    case AFFIX_ARG_ULONGLONG: {
        U64 value = SvIOK(data) ? SvUV(data) : 0;
        Copy(&value, ptr, 1, U64);
    } break;
    case AFFIX_ARG_FLOAT: {
        float value = SvOK(data) ? SvNV(data) : 0.0f;
        Copy(&value, ptr, 1, float);
    } break;
    case AFFIX_ARG_DOUBLE: {
        double value = SvOK(data) ? SvNV(data) : 0.0f;
        Copy(&value, ptr, 1, double);
    } break;
    /*
    case AFFIX_ARG_CPOINTER: {
        HV *hv_ptr = MUTABLE_HV(SvRV(type));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        DCpointer value = safemalloc(_sizeof(aTHX_ * type_ptr));
        if (SvOK(data)) sv2ptr(aTHX_ * type_ptr, data, value, packed);
        Copy(&value, ptr, 1, intptr_t);
    } break;
    case AFFIX_ARG_WCHAR: {
        char *eh = SvPV_nolen(data);
        dXSARGS;
        PUTBACK;
        const char *pat = "W";
        SSize_t s = unpackstring(pat, pat + 1, eh, eh + WCHAR_T_SIZE + 1, SVt_PVAV);
        SPAGAIN;
        if (s != 1) croak("Failed to unpack wchar_t");
        SV *data = POPs;
        switch (WCHAR_T_SIZE) {
        case I8SIZE:
            if (SvPOK(data)) {
                char *value = SvPV_nolen(data);
                Copy(&value, ptr, 1, char);
            }
            else {
                char value = SvIOK(data) ? SvIV(data) : 0;
                Copy(&value, ptr, 1, char);
            }
            break;
        case SHORTSIZE: {
            short value = SvIOK(data) ? (short)SvIV(data) : 0;
            Copy(&value, ptr, 1, short);
        } break;
        case INTSIZE: {
            int value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, int);
        } break;
        default:
            croak("Invalid wchar_t size for argument!");
        }
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
        else
            Zero(ptr, 1, intptr_t);
    } break; /*
case AFFIX_ARG_UTF16STR: {
if (SvPOK(data)) {
SV *idk = call_encoding(aTHX_ "encode", find_encoding(aTHX), data, NULL);
STRLEN len;
char *str = SvPV(idk, len);
DCpointer value;
Newxz(value, len + WCHAR_T_SIZE, char);
Copy(str, value, len, char);
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
//~ } break;*/
    case AFFIX_ARG_CSTRUCT: {
        size_t size = _sizeof(aTHX_ type_sv);
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type_sv));
            HV *hv_data = MUTABLE_HV(SvRV(data));
            SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
            SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
            AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
            int field_count = av_count(av_fields);
            for (int i = 0; i < field_count; ++i) {
                SV **field = av_fetch(av_fields, i, 0);
                AV *name_type = MUTABLE_AV(SvRV(*field));
                SV **name_ptr = av_fetch(name_type, 0, 0);
                SV **type_ptr = av_fetch(name_type, 1, 0);
                char *key = SvPVbytex_nolen(*name_ptr);
                SV **_data = hv_fetch(hv_data, key, strlen(key), 1);
                if (_data != NULL) {
                    size_t fdsa = _offsetof(aTHX_ * type_ptr);
                    int fdsafe = PTR2IV(ptr);
                    DCpointer blah = INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr));
                    sv2ptr(aTHX_ * type_ptr, *_data,
                           INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr)), packed);
                }
            }
        }
    } break;
    case AFFIX_ARG_CUNION: {
        size_t size = _sizeof(aTHX_ type_sv);
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type_sv));
            HV *hv_data = MUTABLE_HV(SvRV(data));
            SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
            SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
            AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
            int field_count = av_count(av_fields);
            for (int i = 0; i < field_count; ++i) {
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
        int spot = 1;
        AV *elements = MUTABLE_AV(SvRV(data));
        SV *pointer;
        HV *hv_ptr = MUTABLE_HV(SvRV(type_sv));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        size_t size = SvOK(*size_ptr) ? SvIV(*size_ptr) : av_len(elements) + 1;
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
            // //sv_dump(*type_ptr);
            // //sv_dump(*size_ptr);
            if (SvOK(*size_ptr)) {
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
        //~ DD(type_sv);
        DCCallback *cb = NULL;
        HV *field = MUTABLE_HV(SvRV(type_sv)); // Make broad assumptions
        // SV **sig = hv_fetchs(field, "signature", 0);
        // SV **sig_len = hv_fetchs(field, "sig_len", 0);
        SV **ret = hv_fetchs(field, "ret", 0);
        SV **args = hv_fetchs(field, "args", 0);
        Callback *callback;
        Newxz(callback, 1, Callback);
        callback->args = MUTABLE_AV(SvRV(*args));

        // callback->sig = SvPV_nolen(*sig);
        // callback->sig_len = (size_t)SvIV(*sig_len);
        callback->sig = ")Z";
        callback->sig_len = 2;

        callback->ret = (char)*SvPV_nolen(*ret);
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
        croak("%d is not a known type in sv2ptr(...)", type);
    }
    }
    return;
}
