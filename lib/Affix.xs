#if defined(_WIN32) || defined(__WIN32__)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT extern "C"
#endif
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/clutter.h"

typedef struct CoW
{
    DCCallback *cb;
    struct CoW *next;
} CoW;

static CoW *cow;

typedef struct
{
    char *sig;
    size_t sig_len;
    char ret;
    char mode;
    void *fptr;
    char *perl_sig;
    DLLib *lib;
    AV *args;
    SV *retval;
} Call;

typedef struct
{
    char *sig;
    size_t sig_len;
    char ret;
    char *perl_sig;
    SV *cv;
    AV *args;
    SV *retval;
    dTHXfield(perl)
} Callback;

char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata) {
    Callback *cbx = (Callback *)userdata;
    //  result->i = 1244;

    // warn("Bruh %c (%s [%d] return: %c) at %s line %d", cbx->ret, cbx->sig, cbx->sig_len,
    // cbx->ret,
    //     __FILE__, __LINE__);
    dTHXa(cbx->perl);

    {
        dSP;
        int count;

        ENTER;
        SAVETMPS;

        PUSHMARK(SP);

        EXTEND(SP, cbx->sig_len);
        for (int i = 0; i < cbx->sig_len; ++i) {
            switch (cbx->sig[i]) {
            case DC_SIGCHAR_VOID:
                // mPUSHi((IV)dcbArgInt(args));
                break;
            case DC_SIGCHAR_INT:
                mPUSHi((IV)dcbArgInt(args));
                break;
            case DC_SIGCHAR_POINTER: {
                DCpointer ptr = dcbArgPointer(args);
                PUSHs(sv_setref_pv(newSV(1), "Dyn::Call::Pointer", dcbArgPointer(args)));
            } break;
            case DC_SIGCHAR_BLESSED: {
                DCpointer ptr = dcbArgPointer(args);
                HV *blessed = MUTABLE_HV(SvRV(*av_fetch(cbx->args, i, 0)));
                SV **package = hv_fetchs(blessed, "package", 0);
                PUSHs(sv_setref_pv(newSV(1), SvPV_nolen(*package), ptr));
            } break;
            case DC_SIGCHAR_ANY: {
                DCpointer ptr = dcbArgPointer(args);
                if (ptr && SvOK((SV *)ptr))
                    PUSHs((SV *)ptr);
                else
                    PUSHs(&PL_sv_undef);
            } break;
            default:
                croak("Unhandled callback arg. Type: %c [%s]", cbx->sig[i], cbx->sig);
                break;
            }
        }

        PUTBACK;

        if (cbx->ret == DC_SIGCHAR_VOID)
            call_sv(cbx->cv, G_DISCARD);
        else {
            count = call_sv(cbx->cv, G_EVAL | G_SCALAR);
            if (count != 1) croak("Big trouble: %d returned items", count);
            SPAGAIN;
            switch (cbx->ret) {
            case DC_SIGCHAR_VOID:
                break;
            case DC_SIGCHAR_FLOAT:
                result->f = POPn;
                break;
            case DC_SIGCHAR_DOUBLE:
                result->d = POPn;
                break;
            case DC_SIGCHAR_POINTER: {
                SV *sv_ptr = POPs;
                if (SvOK(sv_ptr)) {
                    if (sv_derived_from(sv_ptr, "Dyn::Call::Pointer")) {
                        IV tmp = SvIV((SV *)SvRV(sv_ptr));
                        result->p = INT2PTR(DCpointer, tmp);
                    }
                    croak("Returned value is not a Dyn::Call::Pointer or subclass");
                }
                else
                    result->p = NULL; // ha.
            } break;
            default:
                croak("Unhandled return from callback: %c", cbx->ret);
            }
            PUTBACK;
        }

        FREETMPS;
        LEAVE;
    }

    return cbx->ret;
}

/* TODO: This might be compatible further back than 5.10.0. */
#if PERL_VERSION_GE(5, 10, 0) && PERL_VERSION_LE(5, 15, 1)
#undef XS_EXTERNAL
#undef XS_INTERNAL
#if defined(__CYGWIN__) && defined(USE_DYNAMIC_LOADING)
#define XS_EXTERNAL(name) __declspec(dllexport) XSPROTO(name)
#define XS_INTERNAL(name) STATIC XSPROTO(name)
#endif
#if defined(__SYMBIAN32__)
#define XS_EXTERNAL(name) EXPORT_C XSPROTO(name)
#define XS_INTERNAL(name) EXPORT_C STATIC XSPROTO(name)
#endif
#ifndef XS_EXTERNAL
#if defined(HASATTRIBUTE_UNUSED) && !defined(__cplusplus)
#define XS_EXTERNAL(name) void name(pTHX_ CV *cv __attribute__unused__)
#define XS_INTERNAL(name) STATIC void name(pTHX_ CV *cv __attribute__unused__)
#else
#ifdef __cplusplus
#define XS_EXTERNAL(name) extern "C" XSPROTO(name)
#define XS_INTERNAL(name) static XSPROTO(name)
#else
#define XS_EXTERNAL(name) XSPROTO(name)
#define XS_INTERNAL(name) STATIC XSPROTO(name)
#endif
#endif
#endif
#endif

/* perl >= 5.10.0 && perl <= 5.15.1 */

/* The XS_EXTERNAL macro is used for functions that must not be static
 * like the boot XSUB of a module. If perl didn't have an XS_EXTERNAL
 * macro defined, the best we can do is assume XS is the same.
 * Dito for XS_INTERNAL.
 */
#ifndef XS_EXTERNAL
#define XS_EXTERNAL(name) XS(name)
#endif
#ifndef XS_INTERNAL
#define XS_INTERNAL(name) XS(name)
#endif

/* Now, finally, after all this mess, we want an ExtUtils::ParseXS
 * internal macro that we're free to redefine for varying linkage due
 * to the EXPORT_XSUB_SYMBOLS XS keyword. This is internal, use
 * XS_EXTERNAL(name) or XS_INTERNAL(name) in your code if you need to!
 */

#undef XS_EUPXS
#if defined(PERL_EUPXS_ALWAYS_EXPORT)
#define XS_EUPXS(name) XS_EXTERNAL(name)
#else
/* default to internal */
#define XS_EUPXS(name) XS_INTERNAL(name)
#endif

#ifndef PERL_ARGS_ASSERT_CROAK_XS_USAGE
#define PERL_ARGS_ASSERT_CROAK_XS_USAGE                                                            \
    assert(cv);                                                                                    \
    assert(params)

/* prototype to pass -Wmissing-prototypes */
STATIC void S_croak_xs_usage(const CV *const cv, const char *const params);

STATIC void S_croak_xs_usage(const CV *const cv, const char *const params) {
    const GV *const gv = CvGV(cv);

    PERL_ARGS_ASSERT_CROAK_XS_USAGE;

    if (gv) {
        const char *const gvname = GvNAME(gv);
        const HV *const stash = GvSTASH(gv);
        const char *const hvname = stash ? HvNAME(stash) : NULL;

        if (hvname)
            Perl_croak_nocontext("Usage: %s::%s(%s)", hvname, gvname, params);
        else
            Perl_croak_nocontext("Usage: %s(%s)", gvname, params);
    }
    else {
        /* Pants. I don't think that it should be possible to get here. */
        Perl_croak_nocontext("Usage: CODE(0x%" UVxf ")(%s)", PTR2UV(cv), params);
    }
}
#undef PERL_ARGS_ASSERT_CROAK_XS_USAGE

#define croak_xs_usage S_croak_xs_usage

#endif

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto)                                             \
    (PL_Sv = (SV *)newXS(name, c_impl, file), sv_setpv(PL_Sv, proto), (CV *)PL_Sv)
#endif /* !defined(newXS_flags) */

static size_t _sizeof(pTHX_ SV *type) {
    // sv_dump(type);
    char *str = SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
    // warn("str == %s", str);
    switch (str[0]) {
    case DC_SIGCHAR_VOID:
        return 0;
    case DC_SIGCHAR_BOOL:
        return BOOLSIZE;
    case DC_SIGCHAR_CHAR:
    case DC_SIGCHAR_UCHAR:
        return I8SIZE;
    case DC_SIGCHAR_SHORT:
    case DC_SIGCHAR_USHORT:
        return SHORTSIZE;
    case DC_SIGCHAR_INT:
    case DC_SIGCHAR_UINT:
        return INTSIZE;
    case DC_SIGCHAR_LONG:
    case DC_SIGCHAR_ULONG:
        return LONGSIZE;
    case DC_SIGCHAR_LONGLONG:
    case DC_SIGCHAR_ULONGLONG:
        return LONGLONGSIZE;
    case DC_SIGCHAR_FLOAT:
        return FLOATSIZE;
    case DC_SIGCHAR_DOUBLE:
        return DOUBLESIZE;
    case DC_SIGCHAR_STRUCT: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6))
            return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        size_t size = 0;
        SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);
        SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
        bool packed = SvTRUE(*sv_packed);
        AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
        int field_count = av_count(idk_arr);
        for (int i = 0; i < field_count; ++i) {
            SV **type_ptr = av_fetch(MUTABLE_AV(*av_fetch(idk_arr, i, 0)), 1, 0);
            size_t __sizeof = _sizeof(aTHX_ * type_ptr);
            if (!packed) size += padding_needed_for(size, __sizeof);
            hv_stores(MUTABLE_HV(SvRV(*type_ptr)), "offset", newSViv(size));
            size += __sizeof;
        }
        hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(size));
        return size;
    }
    case DC_SIGCHAR_ARRAY: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6))
            return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        SV **type_ptr = hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0);
        SV **size_ptr = hv_fetchs(MUTABLE_HV(SvRV(type)), "size", 0);
        size_t size = _sizeof(aTHX_ * type_ptr) * SvIV(*size_ptr);
        hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(size));
        return size;
    }
    case DC_SIGCHAR_UNION: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6))
            return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        size_t size = 0, this_size;
        SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "types", 0);
        AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
        for (int i = 0; i < av_count(idk_arr); ++i) {
            SV **type_ptr = av_fetch(idk_arr, i, 0);
            this_size = _sizeof(aTHX_ * type_ptr);
            if (size < this_size) size = this_size;
        }
        hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(size));
        return size;
    }
    case DC_SIGCHAR_CODE: // automatically wrapped in a DCCallback pointer
        return PTRSIZE;
    case DC_SIGCHAR_POINTER:
    case DC_SIGCHAR_STRING:
    case DC_SIGCHAR_BLESSED:
        return PTRSIZE;
    case DC_SIGCHAR_ANY:
        return sizeof(SV);
    default:
        warn("&str == %s", str);
        croak("OH, NO!");
        return -1;
    }
}

static DCaggr *_aggregate(pTHX_ SV *type) {
    // warn("here at %s line %d", __FILE__, __LINE__);
    // sv_dump(type);

    char *str = SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
                                       // warn("here at %s line %d", __FILE__, __LINE__);

    size_t size = _sizeof(aTHX_ type);
    // warn("here at %s line %d", __FILE__, __LINE__);

    switch (str[0]) {
    case DC_SIGCHAR_STRUCT: {
        // warn("here at %s line %d", __FILE__, __LINE__);

        if (hv_exists(MUTABLE_HV(SvRV(type)), "aggregate", 9)) {
            SV *__type = *hv_fetchs(MUTABLE_HV(SvRV(type)), "aggregate", 0);
            // warn("here at %s line %d", __FILE__, __LINE__);
            // sv_dump(__type);

            // return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "aggregate", 0));
            if (sv_derived_from(__type, "Dyn::Call::Aggregate")) {
                // warn("here at %s line %d", __FILE__, __LINE__);

                HV *hv_ptr = MUTABLE_HV(__type);
                // warn("here at %s line %d", __FILE__, __LINE__);

                IV tmp = SvIV((SV *)SvRV(__type));
                // warn("here at %s line %d", __FILE__, __LINE__);

                return INT2PTR(DCaggr *, tmp);
            }
            else
                croak("Oh, no...");
        }
        else {
            // warn("here at %s line %d", __FILE__, __LINE__);

            SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);
            SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
            bool packed = SvTRUE(*sv_packed);
            AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
            int field_count = av_count(idk_arr);
            // warn("DCaggr *main = dcNewAggr(%d, %d);", field_count, size);
            DCaggr *agg = dcNewAggr(field_count, size);
            for (int i = 0; i < field_count; ++i) {
                SV **type_ptr = av_fetch(MUTABLE_AV(*av_fetch(idk_arr, i, 0)), 1, 0);
                size_t __sizeof = _sizeof(aTHX_ * type_ptr);
                size_t offset = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "offset", 0));
                char *str = SvPVbytex_nolen(*type_ptr);
                switch (str[0]) {
                case DC_SIGCHAR_STRUCT: {
                    DCaggr *child = _aggregate(aTHX_ * type_ptr);
                    dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                    // warn("dcAggrField(agg, DC_SIGCHAR_AGGREGATE, %d, 1, child);", offset);
                    //  warn("kid!");
                } break;
                default: {
                    dcAggrField(agg, str[0], offset, 1);
                    // warn("dcAggrField(agg, %c, %d, 1);", str[0], offset);
                } break;
                }
            }
            // warn("here at %s line %d", __FILE__, __LINE__);

            // warn("dcCloseAggr(agg);");
            dcCloseAggr(agg);
            {
                SV *RETVALSV;
                RETVALSV = newSV(1);
                sv_setref_pv(RETVALSV, "Dyn::Call::Aggregate", (void *)agg);
                hv_stores(MUTABLE_HV(SvRV(type)), "aggregate", newSVsv(RETVALSV));
            }
            return agg;
        }
    } break;
    default: {
        warn("fallback");
        break;
    }
    }
    return NULL;
}

XS_EUPXS(Types_type_sizeof); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_type_sizeof) {
    dXSARGS;
    dXSI32;
    dXSTARG;

    XSRETURN_IV(_sizeof(aTHX_ MUTABLE_SV(ST(0))));
}

XS_EUPXS(Types_type_aggregate); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_type_aggregate) {
    dVAR;
    dXSARGS;
    dXSI32;
    DCaggr *agg = _aggregate(aTHX_ MUTABLE_SV(ST(0)));
    if (agg != NULL) {
        SV *RETVALSV;
        RETVALSV = newSV(1);
        sv_setref_pv(RETVALSV, "Dyn::Call::Aggregate", (DCpointer)agg);
        ST(0) = sv_2mortal((RETVALSV));
        XSRETURN(1);
    }
    else
        XSRETURN_UNDEF;
}

static DCaggr *coerce(pTHX_ SV *type, SV *data, DCpointer ptr, bool packed, size_t pos) {
    // void *RETVAL;
    // Newxz(RETVAL, 1024, char);

    // warn("pos == %p", pos);

    // if(SvROK(type))
    //     type = SvRV(type);
    // sv_dump(type);

    char *str = SvPVbytex_nolen(type);
    // warn("type: %s, offset: %d", str, pos);
    switch (str[0]) {
    case DC_SIGCHAR_CHAR: {
        char *value = SvPV_nolen(data);
        Copy(value, ptr, 1, char);
        // return I8SIZE;
    } break;
    case DC_SIGCHAR_STRUCT: {
        if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
        size_t size = _sizeof(aTHX_ type);
        // warn("STRUCT! size: %d", size);

        HV *hv_type = MUTABLE_HV(SvRV(type));
        HV *hv_data = MUTABLE_HV(SvRV(data));
        // sv_dump(MUTABLE_SV(hv_type));

        SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
        SV **sv_packed = hv_fetchs(hv_type, "packed", 0);

        AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
        int field_count = av_count(av_fields);

        // warn("field_count [%d]", field_count);

        // warn("size [%d]", size);

        // DumpHex(ptr, size);
        // warn("here at %s line %d", __FILE__, __LINE__);

        DCaggr *retval = _aggregate(aTHX_ type);
        // warn("here at %s line %d", __FILE__, __LINE__);
        for (int i = 0; i < field_count; ++i) {
            // warn("here at %s line %d", __FILE__, __LINE__);

            SV **field = av_fetch(av_fields, i, 0);

            AV *key_value = MUTABLE_AV((*field));
            // //sv_dump( MUTABLE_SV((*field)));
            // warn("here at %s line %d", __FILE__, __LINE__);

            SV **name_ptr = av_fetch(key_value, 0, 0);
            SV **type_ptr = av_fetch(key_value, 1, 0);
            char *key = SvPVbytex_nolen(*name_ptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

            // SV * type = *type_ptr;
            // warn("key[%d] %s", i, key);
            // warn("val[%d] %s", i, SvPVbytex_nolen(val));
            if (!hv_exists(hv_data, key, strlen(key)))
                croak("Expected key %s does not exist in given data", key);
            SV **_data = hv_fetch(hv_data, key, strlen(key), 0);
            char *type = SvPVbytex_nolen(*type_ptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

            warn("Added %c:'%s' in slot %d at %s line %d", type[0], key, pos, __FILE__, __LINE__);

            size_t el_len = _sizeof(aTHX_ * type_ptr);

            coerce(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 0)),
                   ((DCpointer)(PTR2IV(ptr) + pos)), packed, pos);

            warn("padding needed: %d for size of %d at %s line %d",
                 padding_needed_for(PTR2IV(ptr), _sizeof(aTHX_ * type_ptr)),
                 _sizeof(aTHX_ * type_ptr), __FILE__, __LINE__);
            pos += el_len;

            /*
            if (!packed)
                pos += padding_needed_for(pos, _sizeof(aTHX_ * type_ptr));
            else
                pos += _sizeof(aTHX_ * type_ptr);*/

            warn("value of pos is %d at %s line %d", pos, __FILE__, __LINE__);

            // warn("dcAggrField(*agg, DC_SIGCHAR_INT, %d, 1);", pos);
            // dcAggrField(retval, DC_SIGCHAR_INT, 0, 1);

            // //sv_dump(*_data);
        }
        // DumpHex(ptr, size);

        // DumpHex(ptr, pos);
        dcCloseAggr(retval);
        // warn("     dcCloseAggr(agg);");

        // dcAggrField(retval, DC_SIGCHAR_AGGREGATE, pos, 1, retval);
        // warn ("     dcAggrField(*agg, DC_SIGCHAR_AGGREGATE, %d, %d, me);", pos, 1);

        /*
                size_t av_count = av_count(av);
                HV *hash = MUTABLE_HV(data);
                int pos = 0;
                for (int i = 0; i < av_count; i++) {
                    warn("i == %d", i);
                    SV **field_ptr = av_fetch(av, i, 0);
                    AV *field = MUTABLE_AV(*field_ptr);
                    SV **name_ptr = av_fetch(field, 0, 0);
                    SV **type_ptr = av_fetch(field, 1, 0);
                    //sv_dump(*type_ptr);
                    // HeVAL(hv_fetch_ent(MUTABLE_HV(data), *name_ptr, 0, 0)) =
                    // MUTABLE_SV(newAV_mortal());//coerce(*type_ptr, data);
                    // //sv_dump(data);
                    warn("HERE");
                    //sv_dump(*name_ptr);
                    const char *name = SvPV_nolen(*name_ptr);
                    warn("name == %s", name);
                    HV *data_hv = MUTABLE_HV(SvRV(data));
                    warn("fdsafdasfdasfdsa");
                    // return data;

                    SV **idk_wtf = hv_fetch(data_hv, name, strlen(name), 0);

                    //sv_dump(*idk_wtf);

                    // SV * value = coerce(*type_ptr,

                    // const char * _type = SvPVbytex_nolen(type);
                    // SV ** target = hv_fetch(hash, _type, 0, 0);
                    // //sv_dump(*target);

                    pos = coerce(*type_ptr, *idk_wtf, ptr, packed);

                    // //sv_dump(SvRV(field));
                    //  SV * in = coerce((field), data);
                }
                // IV tmp = SvIV((SV*)SvRV(ST(0)));
                // ptr = INT2PTR(DCpointer *, tmp);
                */
        return retval;
    } break;
    case DC_SIGCHAR_ARRAY: {

        // sv_dump(data);
        if (SvTYPE(SvRV(data)) != SVt_PVAV) croak("Expected an array");
        int spot = 1;
        AV *elements = MUTABLE_AV(SvRV(data));
        SV *pointer;
        HV *hv_ptr = MUTABLE_HV(SvRV(type));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        // //sv_dump(*type_ptr);
        // //sv_dump(*size_ptr);
        size_t av_len = av_count(elements);
        if (SvOK(*size_ptr)) {
            size_t tmp = SvIV(*size_ptr);
            if (av_len != tmp) croak("Expected and array of %d elements; found %d", tmp, av_len);
        }
        size_t el_len = _sizeof(aTHX_ * type_ptr);

        for (int i = 0; i < av_len; ++i) {
            coerce(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)), ((DCpointer)(PTR2IV(ptr) + pos)),
                   packed, pos);
            pos += (el_len);
        }

        // return _sizeof(aTHX_ type);
    }
    // croak("ARRAY!");

    break;
    case DC_SIGCHAR_CODE:
        croak("CODE!");
        break;
    case DC_SIGCHAR_INT: {
        int value = SvIV(data);
        Copy((char *)(&value), ptr, 1, int);
    } break;
    case DC_SIGCHAR_UINT: {
        int value = SvUV(data);
        Copy((char *)(&value), ptr, 1, unsigned int);
    } break;
    case DC_SIGCHAR_FLOAT: {
        float value = SvNV(data);
        Copy((char *)(&value), ptr, 1, float);
        // return FLOATSIZE;
    } break;
    case DC_SIGCHAR_DOUBLE: {
        double value = SvNV(data);
        Copy((char *)(&value), ptr, 1, double);
        // return DOUBLESIZE;
    } break;
    case DC_SIGCHAR_STRING: {
        char *value = SvPV_nolen(data);
        Copy(&value, ptr, 1, intptr_t);
        // return PTRSIZE;
    } break;
    default: {
        croak("OTHER");
        char *str = SvPVbytex_nolen(type);
        warn("char *str = SvPVbytex_nolen(type) == %s", str);
        size_t size;
        size_t array_len = 1; // TODO...
        void *value;

        switch (str[0]) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_CHAR:
            size = I8SIZE;
            break;

        case DC_SIGCHAR_FLOAT:
            size = FLOATSIZE;
            break;
        case DC_SIGCHAR_DOUBLE: {
            double value = (double)SvNV(data);
            // Copy(value, pos, 1, double);
            size = DOUBLESIZE;
        } break;

        // DOUBLESIZE
        default:
            croak("%c is not a known type", str[0]);
        }
        // warn("aaaa %p - %d - %d", pos, size, array_len);

        // if (!packed) pos += padding_needed_for(pos, size * array_len);

        warn("bbb");

        // dcAggrField(ag, type, current_offset, array_len);
        // warn("Adding slot! type: %c, offset: 0[%p], array_len: %d", str[0], pos, array_len);
        // croak("%d | %d", current_offset, array_len);

        // value = newSVnv(*((float *)&val));

        // pos += size * array_len;

        // return newSVpv(value, 1);
    }
    }
    // DumpHex(RETVAL, 1024);
    return NULL; // pos;
                 /*return newSV(0);
                 return MUTABLE_SV(newAV_mortal());
                 return (newSVpv((const char *)RETVAL, 1024)); // XXX: Use mock sizeof from elements
                 */
}

XS_EUPXS(Types_wrapper); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_wrapper) {
    dVAR;
    dXSARGS;
    dXSI32;

    char *package = (char *)XSANY.any_ptr; // SvPV_nolen(ST(0));

    PERL_UNUSED_VAR(ax); /* -Wall */
    package = form("%s", package);

    // warn("Calling %s->new(...) [%d]", package, items);
    SV *RETVAL;

    {
        dSP;
        int count;

        ENTER;
        SAVETMPS;

        PUSHMARK(SP);

        mXPUSHs(newSVpv(package, 0));
        for (int i = 0; i < items; i++)
            mXPUSHs(newSVsv(ST(i)));
        PUTBACK;

        count = call_method("new", G_SCALAR);

        SPAGAIN;

        if (count != 1) croak("Big trouble\n");

        RETVAL = newSVsv(POPs);

        PUTBACK;
        FREETMPS;
        LEAVE;
    }

    RETVAL = sv_2mortal(RETVAL);
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_EUPXS(Types); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types) {
    dVAR;
    dXSARGS;
    dXSI32;
    dMY_CXT;

    char *package = (char *)SvPV_nolen(ST(0));

    // warn("ix == %i %c", ix, ix);

    // PERL_UNUSED_VAR(ax); /* -Wall */

    // warn("Creating a new %s [ix == %c]", package, ix);

    // dXSTARG;

    HV *RETVAL_HV = newHV_mortal();
    //
    //  warn("ix == %c", ix);
    switch (ix) {
    case DC_SIGCHAR_ARRAY: { // ArrayRef[Int] or ArrayRef[Int, 5]
        // SV *packed = SvTRUE(false);
        AV *type_size = MUTABLE_AV(SvRV(ST(1)));
        SV *type, *size = &PL_sv_undef;

        switch (av_count(type_size)) {
        case 2:
            size = *av_fetch(type_size, 1, 0);
            if (!SvIOK(size)) croak("Given size %d is not an integer", SvUV(size));

        // no break; fallthrough
        case 1:
            type = *av_fetch(type_size, 0, 0);

            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                croak("Given type for '%s' is not a subclass of Affix::Type::Base",
                      SvPV_nolen(type));

            break;
        default:
            croak("Expected a single type and optional array length: ArrayRef[Int] or "
                  "ArrayRef[Int, 5]");
        }
        hv_stores(RETVAL_HV, "size", newSVsv(size));
        hv_stores(RETVAL_HV, "name", newSV(0));
        // hv_stores(RETVAL_HV, "packed", packed);
        hv_stores(RETVAL_HV, "type", newSVsv(type));
    } break;
    case DC_SIGCHAR_CODE: {
        AV *fields = newAV_mortal();
        SV *retval = sv_newmortal();

        size_t field_count;
        {
            if (items != 2) croak("CodeRef[ [args] => return]");

            AV *args = MUTABLE_AV(SvRV(ST(1)));
            if (av_count(args) != 2) croak("Expected a list of arguments and a return value");
            fields = MUTABLE_AV(SvRV(*av_fetch(args, 0, 0)));
            field_count = av_count(fields);

            for (int i = i; i < field_count; ++i) {
                SV **type_ref = av_fetch(fields, i, 0);
                if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Affix::Type::Base")))
                    croak("Given type for CodeRef %d is not a subclass of Affix::Type::Base", i);
                av_push(fields, SvREFCNT_inc(((*type_ref))));
            }

            sv_setsv(retval, *av_fetch(args, 1, 0));
            if (!(sv_isobject(retval) && sv_derived_from(retval, "Affix::Type::Base")))
                croak("Given type for return value is not a subclass of Affix::Type::Base");

            char signature[field_count];
            for (int i = 0; i < field_count; i++) {
                SV **type_ref = av_fetch(fields, i, 0);
                char *str = SvPVbytex_nolen(*type_ref);
                // av_push(cb->args, SvREFCNT_inc(*type_ref));
                switch (str[0]) {
                case DC_SIGCHAR_CODE:
                case DC_SIGCHAR_ARRAY:
                    signature[i] = DC_SIGCHAR_POINTER;
                    break;
                case DC_SIGCHAR_AGGREGATE:
                case DC_SIGCHAR_STRUCT:
                    signature[i] = DC_SIGCHAR_AGGREGATE;
                    break;
                default:
                    signature[i] = str[0];
                    break;
                }
            }

            hv_stores(RETVAL_HV, "args", SvREFCNT_inc(*av_fetch(args, 0, 0)));
            hv_stores(RETVAL_HV, "return", SvREFCNT_inc(retval));
            hv_stores(RETVAL_HV, "sig_len", newSViv(field_count));
            hv_stores(RETVAL_HV, "signature", newSVpv(signature, field_count));
        }
    } break;
    case DC_SIGCHAR_STRUCT: {
        if (items == 2) {
            AV *fields = newAV_mortal();
            AV *fields_in = MUTABLE_AV(SvRV(ST(1)));
            size_t field_count = av_count(fields_in);
            if (field_count % 2) croak("Expected an even sized list");
            for (int i = 0; i < field_count; i += 2) {
                AV *field = newAV();
                SV **key_ptr = av_fetch(fields_in, i, 0);
                SV *key = sv_mortalcopy(*key_ptr);
                if (!SvPOK(key)) croak("Given name of '%s' is not a string", SvPV_nolen(key));
                av_push(field, SvREFCNT_inc(key));
                SV **value_ptr = av_fetch(fields_in, i + 1, 0);
                SV *value = sv_mortalcopy(*value_ptr);
                if (!(sv_isobject(value) && sv_derived_from(value, "Affix::Type::Base")))
                    croak("Given type for '%s' is not a subclass of Affix::Type::Base",
                          SvPV_nolen(key));
                av_push(field, SvREFCNT_inc(value));
                av_push(fields, (MUTABLE_SV(field)));
            }
            hv_stores(RETVAL_HV, "fields", newRV_inc(MUTABLE_SV(fields)));
        }
        else
            croak("Struct[...]");
        hv_stores(RETVAL_HV, "packed", sv_2mortal(boolSV(false)));
    } break;
    case DC_SIGCHAR_POINTER: {
        AV *fields = MUTABLE_AV(SvRV(ST(1)));
        if (av_count(fields) == 1) {
            SV *inside;
            SV **type_ref = av_fetch(fields, 0, 0);
            SV *type = *type_ref;
            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                croak("Pointer[...] expects a subclass of Affix::Type::Base");
            hv_stores(RETVAL_HV, "type", SvREFCNT_inc(type));
        }
        else
            croak("Pointer[...] expects a single type. e.g. Pointer[Int]");
    } break;
    case DC_SIGCHAR_UNION: {
        AV *fields_;
        SV *aggregate = newSV(0), *packed = newSV(0);

        sv_set_undef(aggregate);
        if (items == 2) {
            fields_ = MUTABLE_AV(SvRV(ST(1)));
            sv_set_undef(packed);
        }
        else
            croak("Union[...]");
        size_t field_count = av_count(fields_);
        AV *args;

        if (field_count) {
            DCaggr *aggr = dcNewAggr(field_count - 1, 1);
            AV *fields = newAV_mortal();
            for (int i = 0; i < field_count; i++) {
                AV *eh = newAV();
                SV **type_ref = av_fetch(fields_, i, 0);
                SV *type = *type_ref;
                if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                    croak("%d%s is not a subclass of Affix::Type::Base", i, ordinal(i));
                av_push(fields, newSVsv(type));
            }
            hv_stores(RETVAL_HV, "types", newRV_noinc(MUTABLE_SV(fields)));
        }
        else
            hv_stores(RETVAL_HV, "types", newRV_noinc(MUTABLE_SV(newAV_mortal())));
    } break;
    case DC_SIGCHAR_BLESSED: {
        AV *packages_in = MUTABLE_AV(SvRV(ST(1)));
        if (av_count(packages_in) != 1) croak("InstanceOf[...] expects a single package name");
        SV **package_ptr = av_fetch(packages_in, 0, 0);
        if (is_valid_class_name(*package_ptr))
            hv_stores(RETVAL_HV, "package", newSVsv(*package_ptr));
        else
            croak("%s is not a known type", SvPVbytex_nolen(*package_ptr));
    } break;
    case DC_SIGCHAR_ANY: {
        break;
    }
    default:
        if (items > 1)
            croak("Too many arguments for subroutine '%s' (got %d; expected 0)", package, items);
        // warn("Unhandled...");
        break;
    }

    SV *self = newRV_inc_mortal(MUTABLE_SV(RETVAL_HV));
    ST(0) = sv_bless(self, gv_stashpv(package, GV_ADD));
    // SvREADONLY_on(self);

    XSRETURN(1);
    // PUTBACK;
    // return;
}

XS_EUPXS(Types_csig); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_csig) {
    dVAR;
    dXSARGS;
    dXSI32;

    warn("Types_csig - ix == %i %c", ix, ix);
    PERL_UNUSED_VAR(ax); /* -Wall */

    // //sv_dump(MUTABLE_SV(ST(0)));

    char *RETVAL;
    switch (ix) {
    case DC_SIGCHAR_ARRAY: {

        AV *s;

        STMT_START {
            SV *const xsub_tmp_sv = ST(0);
            SvGETMAGIC(xsub_tmp_sv);
            if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                s = (AV *)SvRV(xsub_tmp_sv);
            }
            else {
                Perl_croak_nocontext("%s: %s is not an ARRAY reference", GvNAME(CvGV(cv)), "s");
            }
        }
        STMT_END;

        /*
        char *idk;
        Newxz(idk, 1024, char); // Just a guess
        SV **type_ptr = av_fetch(s, 0, 0);
        SV **size_ptr = av_fetch(s, 1, 0);
        SV *type = *type_ptr;
        SV *size_sv = *size_ptr;
        char *str = SvPVbytex_nolen(type);
        int size = SvIV(size_sv);
        my_strlcat(idk, str, sizeof(str));
        RETVAL = form("%s[%d]", idk, size);
        Safefree(idk);*/
        size_t alen = av_count(s);
        char *idk;
        Newxz(idk, 1024, char); // Just a guess
        int size;
        for (int i = 0; i < alen; ++i) {
            SV **field_ptr = av_fetch(s, i, 0);
            SV *field = *field_ptr;
            SV **type_ptr = av_fetch(MUTABLE_AV(field), 0, 0);
            SV *type = *type_ptr;
            SV **size_ptr = av_fetch(MUTABLE_AV(field), 1, 0);
            char *str = SvPVbytex_nolen(type);
            size = SvIV(*size_ptr);
            my_strlcat(idk, str, 0);
        }
        RETVAL = form("%s[%d]", idk, size);
        Safefree(idk);
    } break;
    case DC_SIGCHAR_STRUCT: {

        AV *s;

        STMT_START {
            SV *const xsub_tmp_sv = ST(0);
            SvGETMAGIC(xsub_tmp_sv);
            if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                s = (AV *)SvRV(xsub_tmp_sv);
            }
            else {
                Perl_croak_nocontext("%s: %s is not an ARRAY reference", GvNAME(CvGV(cv)), "s");
            }
        }
        STMT_END;

        SV *holder = newSV(0);
        size_t alen = av_count(s);
        char *idk;
        Newxz(idk, 0, char); // Just a guess
        for (int i = 0; i < alen; ++i) {
            SV **field_ptr = av_fetch(s, i, 0);
            SV *field = *field_ptr;
            SV **type_ptr = av_fetch(MUTABLE_AV(field), 1, 0);
            SV *type = *type_ptr;
            char *str = SvPVbytex_nolen(type);
            sv_catpv(holder, str);
            warn(str);
            my_strlcat(idk, str, 0);
            warn(idk);
        }
        RETVAL = form("{%s}", idk);
        Safefree(idk);
    } break;
    case DC_SIGCHAR_POINTER:
        croak("Incomplete");
        break;

    default:

        RETVAL = (char *)&ix;
        break;
    }
    // warn(RETVAL);

    ST(0) = sv_2mortal(newSVpv(RETVAL, 0));
    XSRETURN(1);
}

XS_EUPXS(Types_sig); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_sig) {
    dXSARGS;
    dXSI32;
    dXSTARG;

    if (PL_phase == PERL_PHASE_DESTRUCT) XSRETURN_IV(0);

    // warn("Types_sig %c/%d", ix, ix);
    ST(0) = sv_2mortal(newSVpv((char *)&ix, 1));
    // ST(0) = newSViv((char)ix);

    XSRETURN(1);
}

XS_EUPXS(Types_typedef); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_typedef) {
    dXSARGS;
    dXSI32;
    dXSTARG;
    XSprePUSH;

    HV *type_registry = get_hv("Affix::Type::_reg", GV_ADD);
    if (hv_exists_ent(type_registry, ST(0), 0))
        croak("Type named '%s' is already defined", SvPV_nolen(ST(0)));
    const char *name = SvPV_nolen(ST(0));
    if (!(sv_isobject(ST(1)) && sv_derived_from(ST(1), "Affix::Type::Base")))
        croak("Given type for '%s' is not a subclass of Affix::Type::Base", name);
    ST(0) = *hv_store(type_registry, name, strlen(name), newRV_inc(ST(1)), 0);
    XSRETURN(1);
}

XS_EUPXS(Types_type); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_type) {
    dXSARGS;
    dXSI32;
    dXSTARG;

    // XSprePUSH;
    if (items != 1) croak("Expected 1 parameter; found %d", items);
    AV *args = MUTABLE_AV(SvRV(newSVsv(ST(0))));
    if (av_count(args) > 1) croak("Expected 1 parameter; found %d", av_count(args));
    SV *type = av_shift(args);
    if (SvPOK(type)) {
        HV *type_registry = get_hv("Affix::Type::_reg", GV_ADD);
        const char *type_str = SvPV_nolen(type);
        if (!hv_exists_ent(type_registry, type, 0)) croak("Type named '%s' is undefined", type_str);
        type = MUTABLE_SV(SvRV(*hv_fetch(type_registry, type_str, strlen(type_str), 0)));
    }
    ST(0) = newSVsv(type);
    XSRETURN(1);
}

static DCpointer deref_pointer(pTHX_ SV *type, SV *value, bool set) {
    DCpointer RETVAL;
    if (set)
        RETVAL = safemalloc(_sizeof(aTHX_ type));
    else {
        HV *type_hv = MUTABLE_HV(SvRV(type));
        SV **type_ref = hv_fetch(type_hv, "type", 4, 0);
        type = *type_ref;
        SV **ptr_ref = hv_fetch(type_hv, "pointer", 7, 0);
        {
            IV tmp = SvIV((SV *)SvRV(*ptr_ref));
            RETVAL = (void *)INT2PTR(char *, tmp);
        }
    }

    {
        char *type_ = SvPV_nolen(type);
        switch (type_[0]) {
        case DC_SIGCHAR_VOID: {
            if (set)
                *((int *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv((value), (IV) * (int *)RETVAL);
        } break;
        case DC_SIGCHAR_BOOL: {
            if (set)
                *((bool *)RETVAL) = SvTRUE(SvRV(value));
            else
                sv_setbool_mg((value), (bool)*(bool *)RETVAL);
        } break;
        case DC_SIGCHAR_CHAR: {
            if (set)
                *((char *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv((value), (IV) * (char *)RETVAL);
        } break;
        case DC_SIGCHAR_UCHAR: {
            if (set)
                *((unsigned char *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setiv((value), (UV) * (unsigned char *)RETVAL);
        } break;
        case DC_SIGCHAR_SHORT: {
            if (set)
                *((short *)RETVAL) = SvNV(SvRV(value));
            else
                sv_setiv((value), (IV) * (short *)RETVAL);
        } break;
        case DC_SIGCHAR_USHORT: {
            if (set)
                *((unsigned short *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setuv((value), (UV) * (unsigned short *)RETVAL);
        } break;
        case DC_SIGCHAR_INT: {
            if (set)
                *((int *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv((value), (IV) * (int *)RETVAL);
        } break;
        case DC_SIGCHAR_UINT: {
            if (set)
                *((unsigned int *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setuv((value), (UV) * (unsigned int *)RETVAL);
        } break;
        case DC_SIGCHAR_LONG: {
            if (set)
                *((long *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv((value), (IV) * (long *)RETVAL);
        } break;
        case DC_SIGCHAR_ULONG: {
            if (set)
                *((unsigned long *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setuv((value), (UV) * (unsigned long *)RETVAL);
        } break;
        case DC_SIGCHAR_LONGLONG: {
            if (set)
                *((long long *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv((value), (IV) * (long long *)RETVAL);

        } break;
        case DC_SIGCHAR_ULONGLONG: {
            if (set)
                *((unsigned long long *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setuv((value), (UV) * (unsigned long long *)RETVAL);
        } break;
        case DC_SIGCHAR_FLOAT: {
            if (set)
                *((float *)RETVAL) = SvNV(SvRV(value));
            else
                sv_setnv(value, *(float *)RETVAL);
        } break;
        case DC_SIGCHAR_DOUBLE: {
            if (set)
                *((double *)RETVAL) = SvNV(SvRV(value));
            else
                sv_setnv(value, *(double *)RETVAL);
        } break;
        case DC_SIGCHAR_POINTER:
        default:
            croak("Unknown or unsupported pointer type: %c", type);
        };
    }
    return RETVAL;
}

static DCaggr *coerce_aggregate(pTHX_ void *in, SV *type, void *out) {
    DCaggr *RETVAL;
    /*
                size_t size, current_offset = 0;
                for (int i = 0; i < av_len; ++i) {
                    //if (!packed)
                        current_offset += padding_needed_for(current_offset, size * av_len);
                    //dcAggrField(ag, *type_ptr, current_offset, array_len);
                    current_offset += size * array_len; // * ( $field->{list} // 1 );
                }*/
    return RETVAL;
}

#define sloppy_coerce(type, in) _sloppy_coerce(aTHX_ type, in, NULL)

static DCpointer _sloppy_coerce(pTHX_ SV *type, SV *in, DCpointer data) {
    size_t size = _sizeof(aTHX_ type);
    if (data == NULL) data = safecalloc(size, 1);
    // DumpHex(data, size);

    HV *in_hv = MUTABLE_HV(in);
    AV *fields = MUTABLE_AV(SvRV(*hv_fetch(MUTABLE_HV(SvRV(type)), "fields", 6, 0)));
    size_t field_count = av_count(fields);
    for (int i = 0; i < field_count; ++i) {
        SV **field_ptr = av_fetch(fields, i, 0);
        SV **field_name_ptr = av_fetch(MUTABLE_AV(*field_ptr), 0, 0);
        SV **field_type_ptr = av_fetch(MUTABLE_AV(*field_ptr), 1, 0);
        SV **field_offset_ptr = hv_fetch(MUTABLE_HV(SvRV(*field_type_ptr)), "offset", 6, 0);
        size_t offset = PTR2IV(data) + SvIV(*field_offset_ptr);

        char *name = SvPV_nolen(*field_name_ptr);
        char *type = SvPVbytex_nolen(*field_type_ptr);

        // warn("name == %s | type == %s | offset == %d", name, type, offset);

        if (!hv_exists_ent(in_hv, *field_name_ptr, 0)) croak("Missing field %s", name);

        SV **value_ptr = hv_fetch(in_hv, name, strlen(name), 0);
        // SV * value = *value_ptr;
        switch (type[0]) {
        case DC_SIGCHAR_VOID: {
            // int value = SvIV(*value_ptr);
            // Copy((char *)(&value), data, 1, int);
        } break;
        case DC_SIGCHAR_BOOL: {
            bool value = SvTRUE(*value_ptr);
            Copy((char *)(&value), offset, 1, bool);
        } break;
        case DC_SIGCHAR_CHAR: {
            unsigned char value = SvIV(*value_ptr);
            Copy((char *)(&value), offset, 1, unsigned char);
        } break;
        case DC_SIGCHAR_UCHAR: {
            unsigned char value = SvUV(*value_ptr);
            Copy((char *)(&value), offset, 1, unsigned char);
        } break;
            break;
        case DC_SIGCHAR_SHORT: {
            short value = SvIV(*value_ptr);
            Copy((char *)(&value), offset, 1, short);
        } break;
        case DC_SIGCHAR_USHORT: {
            unsigned short value = SvUV(*value_ptr);
            Copy((char *)(&value), offset, 1, unsigned short);
        } break;
        case DC_SIGCHAR_INT: {
            int value = SvIV(*value_ptr);
            Copy((char *)(&value), offset, 1, int);
        } break;
        case DC_SIGCHAR_UINT: {
            unsigned int value = SvUV(*value_ptr);
            Copy((char *)(&value), offset, 1, unsigned int);
        } break;
        case DC_SIGCHAR_LONG: {
            long value = SvIV(*value_ptr);
            Copy((char *)(&value), offset, 1, long);
        } break;
        case DC_SIGCHAR_ULONG: {
            unsigned long value = SvUV(*value_ptr);
            Copy((char *)(&value), offset, 1, unsigned long);
        } break;
        case DC_SIGCHAR_LONGLONG: {
            long long value = SvIV(*value_ptr);
            Copy((char *)(&value), offset, 1, long long);
        } break;
        case DC_SIGCHAR_ULONGLONG: {
            unsigned long long value = SvUV(*value_ptr);
            Copy((char *)(&value), offset, 1, unsigned long long);
        } break;
        case DC_SIGCHAR_FLOAT: {
            float value = SvNV(*value_ptr);
            Copy((char *)(&value), offset, 1, float);
        } break;
            break;
        case DC_SIGCHAR_DOUBLE: {
            double value = SvNV(*value_ptr);
            Copy((char *)(&value), offset, 1, double);
        } break;
        case DC_SIGCHAR_POINTER:
            croak("TODO: push pointer");
            break;
        case DC_SIGCHAR_STRING:
            croak("TODO: push string (pointer)");
            break;
        case DC_SIGCHAR_CODE:
            croak("TODO: push code (DCcallback");
            break;
        case DC_SIGCHAR_AGGREGATE:
        case DC_SIGCHAR_STRUCT:
        case DC_SIGCHAR_UNION:
        case DC_SIGCHAR_ARRAY: {
            _sloppy_coerce(aTHX_ * field_type_ptr, SvRV(*value_ptr), ((DCpointer)(offset)));
        } break;
        default:
            sv_dump((*value_ptr));
        }
        // SV ** field_data = hv_fetch(SvPV_nolen());
        //_sloppy_coerce(aTHX_ SvRV(*field_type_ptr), MUTABLE_SV(newHV_mortal()), ptr);

        //  sv_dump(*field_name_ptr);
    }

    // warn("field_count == %d", field_count);
    // DumpHex(data, _sizeof(aTHX_ type));

    return data;
}

XS_EUPXS(Types_type_call); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_type_call) {
    dVAR;
    dXSARGS;
    dXSI32;
    dMY_CXT;
    // warn("Calling at %s line %d", __FILE__, __LINE__);

    Call *call = (Call *)XSANY.any_ptr;

    dcReset(MY_CXT.cvm);
    bool pointers = false;

    // warn("Calling at %s line %d", __FILE__, __LINE__);
    /*
        if (2== items) {
            DCpointer ptr;
            DCCallback* cb;


            {
            if (SvROK(ST(0))) {
                IV tmp = SvIV((SV *)SvRV(ST(0)));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else if (SvOK(ST(0)))
                ptr = (DCpointer) ST(0);
                //ptr = NULL;
            else
                croak("Type of arg %d must be scalar ref", 0 + 1);
                }




            {
             short result = 0;
             int userdata = 1337;
             cb = dcbNewCallback("pp)v", &cbHandler, &userdata);
             //result = ((short(*)(int, float, short, double, long long))cb)
             //  (123, 23.f, 3, 1.82, 9909ll);
            }
            dcbFreeCallback(cb);

            XSRETURN_EMPTY;return;

            dcArgPointer(MY_CXT.cvm, ptr);
            dcArgPointer(MY_CXT.cvm, cb);
            dcArgInt(MY_CXT.cvm, 1);
            XSRETURN_EMPTY;return;

            //Fl_Button_set_callback(but, cb, count);

            dcCallVoid(MY_CXT.cvm, call->fptr);

            XSRETURN_EMPTY;return;
        }
*/

    if (call->sig_len != items) {
        if (call->sig_len < items) croak("Too many arguments");
        if (call->sig_len > items) croak("Not enough arguments");
    }
    // warn("ping at %s line %d", __FILE__, __LINE__);

    SV *value;
    for (int i = 0; i < call->sig_len; ++i) {
        // warn("Working on element %d of %d (type: %c) at %s line %d", i, call->sig_len - 1,
        //     call->sig[i], __FILE__, __LINE__);
        value = ST(i);
        switch (call->sig[i]) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_BOOL:
            dcArgBool(MY_CXT.cvm, SvTRUE(value));
            break; // Anything can bee a bool
        case DC_SIGCHAR_CHAR:
            dcArgChar(MY_CXT.cvm, SvIOK(value) ? (char)SvIV(value) : (char)*SvPV_nolen(value));
            break;
        case DC_SIGCHAR_UCHAR:
            dcArgChar(MY_CXT.cvm, SvIOK(value) ? (unsigned char)SvUV(value)
                                               : (unsigned char)*SvPV_nolen(value));
            break;
        case DC_SIGCHAR_SHORT:
            dcArgShort(MY_CXT.cvm, (short)SvIV(value));
            break;
        case DC_SIGCHAR_USHORT:
            dcArgShort(MY_CXT.cvm, (unsigned short)SvUV(value));
            break;
        case DC_SIGCHAR_INT:
            dcArgInt(MY_CXT.cvm, (int)SvIV(value));
            break;
        case DC_SIGCHAR_UINT:
            dcArgInt(MY_CXT.cvm, (unsigned int)SvUV(value));
            break;
        case DC_SIGCHAR_LONG:
            dcArgLong(MY_CXT.cvm, (long)SvIV(value));
            break;
        case DC_SIGCHAR_ULONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)SvUV(value));
            break;
        case DC_SIGCHAR_LONGLONG:
            dcArgLongLong(MY_CXT.cvm, (long long)SvIV(value));
            break;
        case DC_SIGCHAR_ULONGLONG:
            dcArgLongLong(MY_CXT.cvm, (unsigned long long)SvUV(value));
            break;
        case DC_SIGCHAR_FLOAT:
            dcArgFloat(MY_CXT.cvm, (float)SvNV(value));
            break;
        case DC_SIGCHAR_DOUBLE:
            dcArgDouble(MY_CXT.cvm, (double)SvNV(value));
            break;
        case DC_SIGCHAR_POINTER: {
            DCpointer ptr;
            SV *package = *av_fetch(call->args, i, 0); // Make broad assumptions
            HV *type_hv = MUTABLE_HV(SvRV(package));
            if (SvROK(value)) {
                SV **ptr_ptr = hv_fetchs(type_hv, "pointer", 0);
                if (ptr_ptr) {
                    IV tmp = SvIV((SV *)SvRV(*ptr_ptr));
                    ptr = INT2PTR(DCpointer, tmp);
                }
                if (sv_derived_from(ST(i), "Dyn::Call::Pointer")) {
                    IV tmp = SvIV((SV *)SvRV(ST(i)));
                    ptr = INT2PTR(DCpointer, tmp);
                }
                else if (SvOK(ST(i))) {
                    SV **type_ref = hv_fetch(type_hv, "type", 4, 0);
                    ptr = deref_pointer(aTHX_ * type_ref, value, true);
                }
                else
                    ptr = NULL;
            }
            else if (!SvOK(value))
                ptr = NULL;
            else
                croak("%d%s parameter must be a scalar ref or Dyn::Call::Pointer object", i + 1,
                      ordinal(i + 1));

            pointers = true;

            dcArgPointer(MY_CXT.cvm, ptr);
            if (ptr != NULL) {
                SV *RETVALSV = newSV(0); // sv_newmortal();
                sv_setref_pv(RETVALSV, "Dyn::Call::Pointer", ptr);
                hv_stores(type_hv, "pointer", RETVALSV);
            }
        } break;
        case DC_SIGCHAR_BLESSED: {                     // Essentially the same as DC_SIGCHAR_POINTER
            SV *package = *av_fetch(call->args, i, 0); // Make broad assumptions
            SV **package_ptr = hv_fetchs(MUTABLE_HV(SvRV(package)), "package", 0);
            DCpointer ptr;
            if (SvROK(value) &&
                sv_derived_from((value), (const char *)SvPVbytex_nolen(*package_ptr))) {
                IV tmp = SvIV((SV *)SvRV(value));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else if (!SvOK(value)) // Passed us an undef
                ;                  // ptr = NULL;
            else
                croak("Type of arg %d must be an instance or subclass of %s", i + 1,
                      SvPVbytex_nolen(*package_ptr));
            // DCpointer ptr = deref_pointer(aTHX_ field, MUTABLE_SV(value), false);
            dcArgPointer(MY_CXT.cvm, ptr);

            // pointers = true;
        } break;
        case DC_SIGCHAR_ANY: {
            dcArgPointer(MY_CXT.cvm, SvOK(value) ? SvREFCNT_inc(value) : NULL);
        } break;
        case DC_SIGCHAR_STRING: {
            dcArgPointer(MY_CXT.cvm, SvOK(value) ? SvPV_nolen(value) : NULL);
        } break;
        case DC_SIGCHAR_CODE: {
            if (SvOK(value)) {
                DCCallback *cb = NULL;
                {
                    CoW *p = cow;
                    while (p != NULL) {
                        if (p->cb) {
                            Callback *_cb = (Callback *)dcbGetUserData(p->cb);
                            if (SvRV(_cb->cv) == SvRV(value)) {
                                cb = p->cb;
                                break;
                            }
                        }
                        p = p->next;
                    }
                }

                if (!cb) {
                    HV *field =
                        MUTABLE_HV(SvRV(*av_fetch(call->args, i, 0))); // Make broad assumptions
                    SV **sig = hv_fetchs(field, "signature", 0);
                    SV **sig_len = hv_fetchs(field, "sig_len", 0);
                    SV **ret = hv_fetchs(field, "return", 0);
                    SV **args = hv_fetchs(field, "args", 0);

                    Callback *callback;
                    Newxz(callback, 1, Callback);

                    callback->args = MUTABLE_AV(SvRV(*args));
                    callback->sig = SvPV_nolen(*sig);
                    callback->sig_len = strlen(callback->sig);
                    callback->ret = (char)*SvPV_nolen(*ret);

                    /*CV *coderef;
                    STMT_START {
                        HV *st;
                        GV *gvp;
                        SV *const xsub_tmp_sv = ST(i);
                        SvGETMAGIC(xsub_tmp_sv);
                        coderef = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
                        if (!coderef) croak("Type of arg %d must be code ref", i + 1);
                    }
                    STMT_END;
                    if (callback->cv) SvREFCNT_dec(callback->cv);
                    callback->cv = SvREFCNT_inc(MUTABLE_SV(coderef));*/

                    callback->cv = SvREFCNT_inc(value);
                    storeTHX(callback->perl);

                    cb = dcbNewCallback(callback->sig, cbHandler, callback);
                    {
                        CoW *hold;
                        Newxz(hold, 1, CoW);
                        hold->cb = cb;
                        hold->next = cow;
                        cow = hold;
                    }
                }
                dcArgPointer(MY_CXT.cvm, cb);
            }
            else
                dcArgPointer(MY_CXT.cvm, NULL);
        } break;
        case DC_SIGCHAR_ARRAY: {
            SV *field = *av_fetch(call->args, i, 0); // Make broad assumptions
            if (!SvROK(value) || SvTYPE(SvRV(value)) != SVt_PVAV)
                croak("Type of arg %d must be an array ref", i + 1);
            AV *elements = MUTABLE_AV(SvRV(value));
            HV *hv_ptr = MUTABLE_HV(SvRV(field));
            SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
            SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
            SV **ptr_ptr = hv_fetchs(hv_ptr, "pointer", 0);
            size_t av_len;
            if (SvOK(*size_ptr)) {
                av_len = SvIV(*size_ptr);
                if (av_count(elements) != av_len)
                    croak("Expected an array of %d elements; found %d", av_len, av_count(elements));
            }
            else
                av_len = av_count(elements);
            size_t size = _sizeof(aTHX_ * type_ptr);
            // warn("av_len * size = %d * %d = %d", av_len, size, av_len * size);
            DCpointer ptr = NULL;
            if (0) {
                if (ptr_ptr) {
                    // warn("Reuse!");
                    IV tmp = SvIV((SV *)SvRV(*ptr_ptr));
                    ptr = saferealloc(INT2PTR(DCpointer, tmp), av_len * size);
                }
            }
            if (ptr == NULL) {
                // warn("Pointer was NULL!");
                ptr = safemalloc(av_len * size);
            }
            if (0) {
                SV *RETVALSV = newSV(0); // sv_newmortal();
                sv_setref_pv(RETVALSV, "Dyn::Call::Pointer", ptr);
                hv_stores(hv_ptr, "pointer", RETVALSV);
            }
            DCaggr *ag = coerce(aTHX_ field, value, ptr, false, 0);
            // DumpHex(ptr, size * av_len);
            dcArgAggr(MY_CXT.cvm, ag, ptr);
        } break;
        case DC_SIGCHAR_STRUCT: {
            if (!SvROK(value) || SvTYPE(SvRV(value)) != SVt_PVHV)
                croak("Type of arg %d must be a hash ref", i + 1);

            SV *field = *av_fetch(call->args, i, 0); // Make broad assumptions
            // DCaggr *agg = _aggregate(aTHX_ field);
            DCpointer ptr = safemalloc(_sizeof(aTHX_ field));

            // static DCaggr *coerce(pTHX_ SV *type, SV *data, DCpointer ptr, bool packed, size_t
            // pos) {
            DCaggr *agg = coerce(aTHX_ field, value, ptr, false, 0);

            // sv_dump(field);
            // sv_dump(value);

            // DumpHex(ptr, 12);
            // DumpHex(ptr, _sizeof(aTHX_ field));

            dcArgAggr(MY_CXT.cvm, agg, ptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

        } break;

        default:
            croak("--> Unfinished: [%c/%d]", call->sig[i], i);
        }
    }
    // warn("Return type: %c at %s line %d", call->ret, __FILE__, __LINE__);
    SV *RETVAL;
    {
        switch (call->ret) {
        case DC_SIGCHAR_VOID:
            dcCallVoid(MY_CXT.cvm, call->fptr);
            break;
        case DC_SIGCHAR_BOOL:
            RETVAL = newSV(0);
            sv_setbool_mg(RETVAL, (bool)dcCallBool(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_CHAR:
            RETVAL = newSViv((char)dcCallChar(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_UCHAR:
            RETVAL = newSVuv((unsigned char)dcCallChar(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_SHORT:
            RETVAL = newSViv((short)dcCallShort(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_USHORT:
            RETVAL = newSVuv((unsigned short)dcCallShort(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_INT:
            RETVAL = newSViv((int)dcCallInt(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_UINT:
            RETVAL = newSVuv((unsigned int)dcCallInt(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_LONG:
            RETVAL = newSViv((long)dcCallLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_ULONG:
            RETVAL = newSVuv((unsigned long)dcCallLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_LONGLONG:
            RETVAL = newSViv((long long)dcCallLongLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_ULONGLONG:
            RETVAL = newSVuv((unsigned long long)dcCallLongLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_FLOAT:
            RETVAL = newSVnv((float)dcCallFloat(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_DOUBLE:
            RETVAL = newSVnv((double)dcCallDouble(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_POINTER: {
            SV *RETVALSV;
            RETVALSV = newSV(1);
            sv_setref_pv(RETVALSV, "Dyn::Call::Pointer", dcCallPointer(MY_CXT.cvm, call->fptr));
            RETVAL = RETVALSV;
        } break;
        case DC_SIGCHAR_STRING:
            RETVAL = newSVpv((char *)dcCallPointer(MY_CXT.cvm, call->fptr), 0);
            break;
        case DC_SIGCHAR_BLESSED: {
            // warn("here at %s line %d", __FILE__, __LINE__);

            DCpointer ptr = dcCallPointer(MY_CXT.cvm, call->fptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

            SV **package = hv_fetchs(MUTABLE_HV(SvRV(call->retval)), "package", 0);
            // warn("here at %s line %d", __FILE__, __LINE__);

            RETVAL = newSV(1);
            // warn("here at %s line %d", __FILE__, __LINE__);

            sv_setref_pv(RETVAL, SvPVbytex_nolen(*package), ptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

        } break;
        case DC_SIGCHAR_ANY: {
            DCpointer ptr = dcCallPointer(MY_CXT.cvm, call->fptr);
            if (ptr && SvOK((SV *)ptr))
                RETVAL = (SV *)ptr;
            else
                sv_set_undef(RETVAL);
        } break;
        default:
            croak("Unhandled return type: %c", call->ret);
        }

        if (pointers) {
            // warn("pointers! at %s line %d", __FILE__, __LINE__);
            for (int i = 0; i < call->sig_len; ++i) {
                switch (call->sig[i]) {
                case DC_SIGCHAR_POINTER: {
                    if (sv_derived_from(ST(i), "Dyn::Call::Pointer")) {
                        // IV tmp = SvIV((SV *)SvRV(ST(i)));
                        // ptr = INT2PTR(DCpointer, tmp);
                    }
                    else if (SvROK(ST(i))) {
                        SV *field = *av_fetch(call->args, i, 0);
                        deref_pointer(aTHX_ field, SvRV(ST(i)), false);
                        SvSETMAGIC(SvRV(ST(i)));
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
        if (call->ret == DC_SIGCHAR_VOID) XSRETURN_EMPTY;
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;

        XSRETURN(1);
    }
}

XS_EUPXS(Affix_DESTROY); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Affix_DESTROY) {
    dVAR;
    dXSARGS;
    // warn("Affix::DESTROY");
    Call *call;
    CV *THIS;
    STMT_START {
        HV *st;
        GV *gvp;
        SV *const xsub_tmp_sv = ST(0);
        SvGETMAGIC(xsub_tmp_sv);
        THIS = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
        {
            CV *cv = THIS;
            call = (Call *)XSANY.any_ptr;
        }
    }
    STMT_END;
    if (call == NULL) XSRETURN_EMPTY;
    if (call->lib != NULL) dlFreeLibrary(call->lib);
    if (call->fptr != NULL) call->fptr = NULL;
    // if (call->args != NULL)
    // sv_free(MUTABLE_SV(call->args));
    // if (call->retval != NULL)
    // sv_free(call->retval);
    // SvREFCNT_dec(call->args);

    // call->args = MUTABLE_AV(SvREFCNT_inc(args));

    // SvREFCNT_dec(call->args);
    SvREFCNT_dec(call->args);
    SvREFCNT_dec(call->retval);
    if (call->sig != NULL) safefree(call->sig);
    if (call->perl_sig != NULL) safefree(call->perl_sig);
    safefree(call);
    call = NULL;
    safefree(XSANY.any_ptr);
    XSANY.any_ptr = NULL;
    XSRETURN_EMPTY;
}

#define PTYPE(NAME, SIGCHAR) CTYPE(NAME, SIGCHAR, SIGCHAR)
#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define TYPE(...) GET_4TH_ARG(__VA_ARGS__, CTYPE, PTYPE)(__VA_ARGS__)
#define CTYPE(NAME, SIGCHAR, SIGCHAR_C)                                                            \
    {                                                                                              \
        const char *package = form("Affix::Type::%s", NAME);                                       \
        cv = newXSproto_portable(form("Affix::%s", NAME), Types_wrapper, file, ";$");              \
        Newx(XSANY.any_ptr, strlen(package) + 1, char);                                            \
        Copy(package, XSANY.any_ptr, strlen(package) + 1, char);                                   \
        cv = newXSproto_portable(form("%s::new", package), Types, file, "$");                      \
        safefree(XSANY.any_ptr);                                                                   \
        XSANY.any_i32 = (int)SIGCHAR;                                                              \
        export_function("Affix", NAME, "types");                                                   \
        /* Int->sig == 'i'; Struct[Int, Float]->sig == '{if}' */                                   \
        cv = newXSproto_portable(form("%s::sig", package), Types_sig, file, "$");                  \
        XSANY.any_i32 = (int)SIGCHAR;                                                              \
        cv = newXSproto_portable(form("%s::csig", package), Types_csig, file, "$");                \
        XSANY.any_i32 = (int)SIGCHAR;                                                              \
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable(form("%s::(\"\"", package), Types_sig, file, ";$");               \
        XSANY.any_i32 = (int)SIGCHAR;                                                              \
        /* The magic for overload gets a GV* via gv_fetchmeth as */                                \
        /* mentioned above, and looks in the SV* slot of it for */                                 \
        /* the "fallback" status. */                                                               \
        sv_setsv(get_sv(form("%s::()", package), TRUE), &PL_sv_yes);                               \
        /* Making a sub named "Affix::Call::Aggregate::()" allows the package */                   \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Call::Aggregate") to return true. */                       \
        (void)newXSproto_portable(form("%s::()", package), Types_sig, file, ";$");                 \
        set_isa(package, "Affix::Type::Base");                                                     \
    }
// clang-format off

MODULE = Affix PACKAGE = Affix

BOOT:
// clang-format on
#ifdef USE_ITHREADS
    my_perl = (PerlInterpreter *)PERL_GET_CONTEXT;
#endif
{
    MY_CXT_INIT;
    MY_CXT.cvm = dcNewCallVM(4096);
}
{

    (void)newXSproto_portable("Affix::typedef", Types_typedef, file, "$$");
    (void)newXSproto_portable("Affix::Type", Types_type, file, "$");
    (void)newXSproto_portable("Affix::Type::Base::sizeof", Types_type_sizeof, file, "$");
    (void)newXSproto_portable("Affix::Type::Base::aggregate", Types_type_aggregate, file, "$");
    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, file, "$");

    CV *cv;
    TYPE("Void", DC_SIGCHAR_VOID);
    TYPE("Bool", DC_SIGCHAR_BOOL);
    TYPE("Char", DC_SIGCHAR_CHAR);
    TYPE("UChar", DC_SIGCHAR_UCHAR);
    TYPE("Short", DC_SIGCHAR_SHORT);
    TYPE("UShort", DC_SIGCHAR_USHORT);
    TYPE("Int", DC_SIGCHAR_INT);
    TYPE("UInt", DC_SIGCHAR_UINT);
    TYPE("Long", DC_SIGCHAR_LONG);
    TYPE("ULong", DC_SIGCHAR_ULONG);
    TYPE("LongLong", DC_SIGCHAR_LONGLONG);
    TYPE("ULongLong", DC_SIGCHAR_ULONGLONG);
    TYPE("Float", DC_SIGCHAR_FLOAT);
    TYPE("Double", DC_SIGCHAR_DOUBLE);
    TYPE("Pointer", DC_SIGCHAR_POINTER);
    TYPE("Str", DC_SIGCHAR_STRING);
    TYPE("Aggregate", DC_SIGCHAR_AGGREGATE);
    TYPE("Struct", DC_SIGCHAR_STRUCT, DC_SIGCHAR_AGGREGATE);
    TYPE("ArrayRef", DC_SIGCHAR_ARRAY, DC_SIGCHAR_AGGREGATE);
    TYPE("Union", DC_SIGCHAR_UNION, DC_SIGCHAR_AGGREGATE);
    TYPE("CodeRef", DC_SIGCHAR_CODE, DC_SIGCHAR_AGGREGATE);
    TYPE("InstanceOf", DC_SIGCHAR_BLESSED, DC_SIGCHAR_POINTER);
    TYPE("Any", DC_SIGCHAR_ANY, DC_SIGCHAR_POINTER);

    // Enum[]?
    export_function("Affix", "typedef", "types");
    export_function("Affix", "wrap", "default");
    export_function("Affix", "attach", "default");
    export_function("Affix", "MODIFY_CODE_ATTRIBUTES", "sugar");
    export_function("Affix", "AUTOLOAD", "sugar");
    export_function("Affix", "MODIFY_CODE_ATTRIBUTES", "default");
    export_function("Affix", "AUTOLOAD", "default");
}
// clang-format off

DLLib *
load_lib(const char * lib_name)
CODE:
{
    // clang-format on
    // Use perl to get the actual path to the library
    {
        dSP;
        int count;

        ENTER;
        SAVETMPS;

        PUSHMARK(SP);
        EXTEND(SP, 1);
        PUSHs(ST(0));
        PUTBACK;

        count = call_pv("Affix::guess_library_name", G_SCALAR);

        SPAGAIN;

        if (count == 1) lib_name = SvPVx_nolen(POPs);

        PUTBACK;
        FREETMPS;
        LEAVE;
    }
    RETVAL =
#if defined(_WIN32) || defined(_WIN64)
        dlLoadLibrary(lib_name);
#else
        (DLLib *)dlopen(lib_name, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
    if (RETVAL == NULL) {
#if defined(_WIN32) || defined(__WIN32__)
        unsigned int err = GetLastError();
        croak("Failed to load %s: %d", lib_name, err);
#else
        char *reason = dlerror();
        croak("Failed to load %s: %s", lib_name, reason);
#endif
        XSRETURN_EMPTY;
    }
}
// clang-format off
OUTPUT:
    RETVAL

SV *
attach(lib, symbol, args, ret, mode = DC_SIGCHAR_CC_DEFAULT, func_name = (ix == 1) ? NULL : symbol)
    const char * symbol
    AV * args
    SV * ret
    char mode
    const char * func_name
ALIAS:
    attach = 0
    wrap   = 1
PREINIT:
    dMY_CXT;
CODE:
// clang-format on
{
    Call *call;
    DLLib *lib;

    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::Load::Lib")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else {
        char *lib_name = (char *)SvPV_nolen(ST(0));

        // Use perl to get the actual path to the library
        {
            dSP;
            int count;

            ENTER;
            SAVETMPS;

            PUSHMARK(SP);
            EXTEND(SP, 1);
            PUSHs(ST(0));
            PUTBACK;

            count = call_pv("Affix::guess_library_name", G_SCALAR);

            SPAGAIN;

            if (count == 1) lib_name = SvPVx_nolen(POPs);

            PUTBACK;
            FREETMPS;
            LEAVE;
        }

        warn("lib_name == %s", lib_name);

        lib =
#if defined(_WIN32) || defined(_WIN64)
            dlLoadLibrary(lib_name);
#else
            (DLLib *)dlopen(lib_name, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (lib == NULL) {
#if defined(_WIN32) || defined(__WIN32__)
            unsigned int err = GetLastError();
            croak("Failed to load %s: %d", lib_name, err);
#else
            char *reason = dlerror();
            croak("Failed to load %s: %s", lib_name, reason);
#endif
            XSRETURN_EMPTY;
        }
    }
    Newx(call, 1, Call);

    // warn("_load(..., %s, '%c')", symbol, mode);
    call->mode = mode;
    call->lib = lib;
    call->fptr = dlFindSymbol(call->lib, symbol);
    call->sig_len = av_count(args);

    if (call->fptr == NULL) { // TODO: throw a warning
        safefree(call);
        XSRETURN_EMPTY;
    }

    call->retval = SvREFCNT_inc(ret);

    char perl_sig[call->sig_len];
    char c_sig[call->sig_len];
    call->args = newAV();

    for (int i = 0; i < call->sig_len; ++i) {
        SV **type_ref = av_fetch(args, i, 0);
        if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Affix::Type::Base")))
            croak("Given type for arg %d is not a subclass of Affix::Type::Base", i);
        av_push(call->args, SvREFCNT_inc(*type_ref));
        char *str = SvPVbytex_nolen(*type_ref);
        c_sig[i] = str[0];
        switch (str[0]) {
        case DC_SIGCHAR_CODE:
            perl_sig[i] = '&';
            break;
        case DC_SIGCHAR_ARRAY:
            perl_sig[i] = '@';
            break;
        case DC_SIGCHAR_STRUCT:
            perl_sig[i] = '%';
            break;
        default:
            perl_sig[i] = '$';
            break;
        }
    }

    Newxz(call->perl_sig, call->sig_len, char);
    Newxz(call->sig, call->sig_len, char);

    Copy(perl_sig, call->perl_sig, strlen(perl_sig), char);
    Copy(c_sig, call->sig, strlen(c_sig), char);
    {
        char *str = SvPVbytex_nolen(ret);
        call->ret = str[0];
    }

    // if (call == NULL) croak("Failed to attach %s", symbol);
    /* Create a new XSUB instance at runtime and set it's XSANY.any_ptr to contain the
     * necessary user data. name can be NULL => fully anonymous sub!
     **/

    CV *cv;
    STMT_START {

        // cv = newXSproto_portable(func_name, XS_Affix__call_Affix, (char*)__FILE__,
        // call->perl_sig);
        // cv = get_cvs("Affix::_call_Affix", 0)

        cv = newXSproto_portable(func_name, Types_type_call, (char *)__FILE__, call->perl_sig);

        if (cv == NULL) croak("ARG! Something went really wrong while installing a new XSUB!");
        ////warn("Q");
        XSANY.any_ptr = (DCpointer)call;
    }
    STMT_END;
    RETVAL = sv_bless((func_name == NULL ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                      gv_stashpv("Affix", GV_ADD));
}
// clang-format off
OUTPUT:
    RETVAL

void
CLONE(...)
CODE :
    MY_CXT_CLONE;

void
DUMP_IT(SV * sv)
CODE :
    sv_dump(sv);

DCpointer
coerce(SV * type, SV * data)
CODE:
    size_t size = _sizeof(aTHX_ type);
    Newxz(RETVAL, size, char);
    coerce(aTHX_ type, data, RETVAL, false, 0);
OUTPUT:
    RETVAL

MODULE = Affix PACKAGE = Affix::ArrayRef

void
DESTROY(HV * me)
CODE:
// clang-format on
{
    SV **ptr_ptr = hv_fetchs(me, "pointer", 0);
    if (!ptr_ptr) return;
    DCpointer ptr;
    IV tmp = SvIV((SV *)SvRV(*ptr_ptr));
    ptr = INT2PTR(DCpointer, tmp);
    if (ptr) safefree(ptr);
    ptr = NULL;
}
// clang-format off
