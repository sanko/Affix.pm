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
            hv_store(MUTABLE_HV(SvRV(*type_ptr)), "offset", 6, newSViv(size), 0);
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
        hv_store(MUTABLE_HV(SvRV(type)), "sizeof", 6, newSViv(size), 0);
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
        hv_store(MUTABLE_HV(SvRV(type)), "sizeof", 6, newSViv(size), 0);
        return size;
    }
    case DC_SIGCHAR_CODE: // automatically wrapped in a DCCallback pointer
        return PTRSIZE;
    case DC_SIGCHAR_POINTER:
    case DC_SIGCHAR_STRING:
        return PTRSIZE;
    default:
        warn("&str == %s", str);
        croak("OH, NO!");
        return -1;
    }
}

static DCaggr *_aggregate(pTHX_ SV *type) {
    char *str = SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
    size_t size = _sizeof(aTHX_ type);
    switch (str[0]) {
    case DC_SIGCHAR_STRUCT: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "aggregate", 9)) {
            // return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "aggregate", 0));
            if (sv_derived_from(type, "Dyn::Call::Aggregate")) {
                HV *hv_ptr = MUTABLE_HV(type);
                SV **ptr_ptr = hv_fetchs(hv_ptr, "aggregate", 1);
                IV tmp = SvIV((SV *)SvRV(*ptr_ptr));
                return INT2PTR(DCaggr *, tmp);
            }
            else
                croak("Oh, no...");
        }
        else {
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
    // warn("*str == %s", str);
    switch (str[0]) {
    case DC_SIGCHAR_CHAR: {
        char *value = SvPV_nolen(data);
        Copy(value, ptr, 1, char);
        // return I8SIZE;
    } break;
    case DC_SIGCHAR_STRUCT: {
        if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
        size_t size = _sizeof(aTHX_ type);
        warn("STRUCT! size: %d", size);

        HV *hv_type = MUTABLE_HV(SvRV(type));
        HV *hv_data = MUTABLE_HV(SvRV(data));
        // sv_dump(MUTABLE_SV(hv_type));

        SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
        SV **sv_packed = hv_fetchs(hv_type, "packed", 0);

        AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
        int field_count = av_count(av_fields);

        // warn("field_count [%d]", field_count);

        warn("size [%d]", size);

        DumpHex(ptr, size);
        DCaggr *retval = _aggregate(aTHX_ SvRV(type));

        for (int i = 0; i < field_count; ++i) {
            SV **field = av_fetch(av_fields, i, 0);
            AV *key_value = MUTABLE_AV((*field));
            // //sv_dump( MUTABLE_SV((*field)));

            SV **name_ptr = av_fetch(key_value, 0, 0);
            SV **type_ptr = av_fetch(key_value, 1, 0);
            char *key = SvPVbytex_nolen(*name_ptr);
            // SV * type = *type_ptr;
            // warn("key[%d] %s", i, key);
            // warn("val[%d] %s", i, SvPVbytex_nolen(val));
            if (!hv_exists(hv_data, key, strlen(key)))
                croak("Expected key %s does not exist in given data", key);
            SV **_data = hv_fetch(hv_data, key, strlen(key), 0);
            char *type = SvPVbytex_nolen(*type_ptr);

            if (!packed) pos += padding_needed_for(pos, _sizeof(aTHX_ * type_ptr));
            warn("Added %c:'%s' at slot %d", type[0], key, pos);
            coerce(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 0)),
                   ((DCpointer)(PTR2IV(ptr) + pos)), packed, 0);

            // warn("dcAggrField(*agg, DC_SIGCHAR_INT, %d, 1);", pos);
            // dcAggrField(retval, DC_SIGCHAR_INT, 0, 1);

            // //sv_dump(*_data);
        }

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

        size_t av_len = SvIV(*size_ptr);
        size_t el_len = _sizeof(aTHX_ * type_ptr);
        size_t tmp = av_count(elements);
        if (av_len != tmp) croak("Expected and array of %d elements; found %d", av_len, tmp);

        for (int i = 0; i < av_len; ++i)

            coerce(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)), ((DCpointer)(PTR2IV(ptr) + pos)),
                   packed, pos);

        // return _sizeof(aTHX_ type);
    }
    // croak("ARRAY!");

    break;
    case DC_SIGCHAR_CODE:
        croak("CODE!");
        break;
    case DC_SIGCHAR_INT: {
        int value = SvIV(data);

        // memcpy(ptr, &value, sizeof(int));

        warn("int needs to go to    %p", ptr);
        //*((double *)ptr) = (double)SvNV(data);
        //((double*)(ptr))[0] = (double)SvNV(data);

        char *ptr_ = (char *)(&value);
        warn("int is %d", *ptr_);

        Copy(ptr_, ptr, 1, int);
        // &ptr=value;

        // return INTSIZE;
    }

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

XS_EUPXS(Dyn_coerce); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Dyn_coerce) {
    dVAR;
    dXSARGS;
    dXSI32;
    size_t size = _sizeof(aTHX_ ST(0));
    // //sv_dump(ST(0));
    // warn("_sizeof(ST(0)) == %d", size);
    DCpointer raw;
    Newxz(raw, size, char);
    DumpHex(raw, size);

    coerce(aTHX_ ST(0), ST(1), raw, false, 0);

    DumpHex(raw, size);
    safefree(raw);
    // raw = NULL;
    XSRETURN(1);
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

    HV *RETVAL_SV = newHV_mortal();
    // warn("here at %s line %d.", __FILE__, __LINE__);
    //  warn("ix == %c", ix);
    switch (ix) {
    case DC_SIGCHAR_ARRAY: {
        // SV *packed = SvTRUE(false);
        AV *type_size;
        warn("items == %d", items);
        if (items == 2)
            type_size = MUTABLE_AV(SvRV(ST(1)));
        else
            croak("Expected a single type and array length");

        if (av_count(type_size) != 2) croak("Expected a single type and array length");

        SV *type;
        type = *av_fetch(type_size, 0, 0);

        if (!(sv_isobject(type) && sv_derived_from(type, "Dyn::Type::Base")))
            croak("Given type for '%s' is not a subclass of Dyn::Type::Base", SvPV_nolen(type));

        SV *size = *av_fetch(type_size, 1, 0);
        if (!SvIOK(size)) croak("Given size %d is not an integer", SvUV(size));

        hv_stores(RETVAL_SV, "size", newSVsv(size));
        hv_stores(RETVAL_SV, "name", newSV(0));
        // hv_stores(RETVAL_SV, "packed", packed);
        hv_stores(RETVAL_SV, "type", newSVsv(type));
    } break;
    case DC_SIGCHAR_CODE: {

        warn("CODE[...] %d ", items);
        /*
                AV *oh, *args;
                SV *retval;
                size_t field_count;

                if (items == 2) {
                    oh = MUTABLE_AV(SvRV(ST(1)));
                    field_count = av_count(oh);
                    if (av_count(oh) != 2) croak("Expected a list of arguments and a return value");
                    args = MUTABLE_AV(SvRV(*av_fetch(oh, 0, 0)));
                    field_count = av_count(args);
                    retval = (*av_fetch(oh, 1, 0));
                }
                else
                    croak("CodeRef[ [args], return]");
                hv_stores(MUTABLE_HV(RETVAL_SV), "args", newRV_noinc(MUTABLE_SV(args)));

                // sv_dump(SvRV(retval));
                // sv_dump(retval);
                hv_stores(MUTABLE_HV(RETVAL_SV), "return", newSVsv(retval));

                if (!(sv_isobject(retval))) croak("NOT AN
           OBJECT_!_!_!_!_@_@__~~_~__!_>_?_?_?_?__?!?!?!?");

                if (!(sv_isobject(retval) && sv_derived_from(retval, "Dyn::Type::Base")))
                    croak("Given type for return value is not a subclass of Dyn::Type::Base");

                char signature[field_count + 2];

                for (int i = 0; i < field_count; i++) {
                    warn("i %d", i);
                    SV **type_ref = av_fetch(args, i, 0);
                    if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Dyn::Type::Base")))
                        croak("Given type for arg %d is not a subclass of Dyn::Type::Base", i);
                    char *str = SvPVbytex_nolen(*type_ref);
                    warn("Type: %s", str);
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
                signature[field_count] = ')';
                {
                    char *str = SvPVbytex_nolen(retval);
                    switch (str[0]) {
                    case DC_SIGCHAR_CODE:
                    case DC_SIGCHAR_ARRAY:
                        signature[field_count + 1] = DC_SIGCHAR_POINTER;
                        break;
                    case DC_SIGCHAR_AGGREGATE:
                    case DC_SIGCHAR_STRUCT:
                        signature[field_count + 1] = DC_SIGCHAR_AGGREGATE;
                        break;
                    default:
                        signature[field_count + 1] = str[0];
                        break;
                    }
                }
                signature[field_count + 2] = (char)0;

                hv_stores(MUTABLE_HV(RETVAL_SV), "signature", newSVpv(signature, field_count + 2));
                {
                    DCCallback *RETVAL;
                    SV *funcptr = NULL;
                    _callback *container = (_callback *)safemalloc(sizeof(_callback));
                    if (!container) // OOM
                        XSRETURN_UNDEF;
                    dcMode(MY_CXT.cvm, container->mode);
                    dcReset(MY_CXT.cvm);
                    Newxz(container->signature, field_count + 2, char);
                    Copy(signature, container->signature, field_count + 2, char);
                    container->cb = &PL_sv_undef;       // Filled in later
                    container->userdata = &PL_sv_undef; // items > 2 ? newRV_inc(ST(2)) :
           &PL_sv_undef; int i; for (i = 0; container->signature[i + 1] != '\0'; ++i) {
                        // warn("here at %s line %d.", __FILE__, __LINE__);
                        if (container->signature[i] == ')') {
                            container->ret_type = container->signature[i + 1];
                            break;
                        }
                    }
                    // warn("signature: %s at %s line %d.", signature, __FILE__, __LINE__);
                    RETVAL = dcbNewCallback(signature, callback_handler, (void *)container);
                    {
                        SV *RETVALSV;
                        RETVALSV = sv_newmortal();
                        // Dyn::Callback | DCCallback * | DCCallbackPtr
                        sv_setref_pv(RETVALSV, "Dyn::Callback", (void *)RETVAL);
                        hv_stores(MUTABLE_HV(RETVAL_SV), "callback", newSVsv(RETVALSV));
                    }
                }
        */
        hv_stores(MUTABLE_HV(RETVAL_SV), "packed", sv_2mortal(boolSV(false)));

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
                if (!(sv_isobject(value) && sv_derived_from(value, "Dyn::Type::Base")))
                    croak("Given type for '%s' is not a subclass of Dyn::Type::Base",
                          SvPV_nolen(key));
                av_push(field, SvREFCNT_inc(value));
                av_push(fields, (MUTABLE_SV(field)));
            }
            hv_stores(MUTABLE_HV(RETVAL_SV), "fields", newRV_inc(MUTABLE_SV(fields)));
        }
        else
            croak("Struct[...]");
        hv_stores(MUTABLE_HV(RETVAL_SV), "packed", sv_2mortal(boolSV(false)));
    } break;
    case DC_SIGCHAR_POINTER: {
        // warn("Pointer[...] w/ %d args at %s line %d.", items, __FILE__, __LINE__);
        // sv_dump(ST(0));
        // if (field_count % 2) croak("Expected an even sized list");
        if (items > 2) croak("Pointer[...] expected 1 type; found %d", items);
        // warn("here at %s line %d.", __FILE__, __LINE__);
        if (items == 1) {}
        else {
            AV *fields = newAV_mortal();

            AV *types_in = MUTABLE_AV(SvRV(ST(1)));
            // warn("here at %s line %d.", __FILE__, __LINE__);
            size_t field_count = av_count(types_in);
            // warn("%d at %s line %d.", field_count, __FILE__, __LINE__);
            switch (field_count) {
            case 0: {
                // TODO: should we attempt to dereference this one properly?
                //warn("One. ...known type pointer?");
            } break;
            case 1: { // warn("here at %s line %d.", __FILE__, __LINE__);
                SV **type_ptr = av_fetch(types_in, 0, 0);
                SV *type = sv_mortalcopy(*type_ptr);
                if (!(sv_isobject(type) && sv_derived_from(type, "Dyn::Type::Base")))
                    croak("Given type for pointer is not a subclass of Dyn::Type::Base");
                hv_stores(MUTABLE_HV(RETVAL_SV), "type", SvREFCNT_inc(type));

            } break;
            default: { // warn("here at %s line %d.", __FILE__, __LINE__);
                croak("Pointer[] or Pointer[...]");
            } break;
            }
        }

        /*
                SV *type = *av_fetch(MUTABLE_AV(SvRV(ST(1))), 0, 0);// deref bug
                warn("here at %s line %d.", __FILE__, __LINE__);
                if (!(sv_isobject(type) && sv_derived_from(type, "Dyn::Type::Base")))
                    croak("Given type for pointer is not a subclass of Dyn::Type::Base");
                 warn("here at %s line %d.", __FILE__, __LINE__);
                /*{
                    SV *pointer_sv;
                    DCpointer pointer;
                    Newxz(pointer, 1, double); // TODO: correct type from type...
                    pointer_sv = newSV(1);
                    sv_setref_pv(pointer_sv, "Dyn::Call::Pointer", (DCpointer)pointer);
                    hv_stores(MUTABLE_HV(RETVAL_SV), "pointer", pointer_sv);
                }*/
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
                if (!(sv_isobject(type) && sv_derived_from(type, "Dyn::Type::Base")))
                    croak("%d%s is not a subclass of Dyn::Type::Base", i, ordinal(i));
                av_push(fields, newSVsv(type));
            }
            hv_stores(MUTABLE_HV(RETVAL_SV), "types", newRV_noinc(MUTABLE_SV(fields)));
        }
        else
            hv_stores(MUTABLE_HV(RETVAL_SV), "types", newRV_noinc(MUTABLE_SV(newAV_mortal())));
    } break;
    default:
        // warn("Unhandled...");
        break;
    }

    SV *self = newRV_inc_mortal(MUTABLE_SV(RETVAL_SV));
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
        Newxz(idk, 1024 + 2, char); // Just a guess
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

    HV *type_registry = get_hv("Dyn::Type::_reg", GV_ADD);
    if (hv_exists_ent(type_registry, ST(0), 0))
        croak("Type named '%s' is already defined", SvPV_nolen(ST(0)));
    const char *name = SvPV_nolen(ST(0));
    if (!(sv_isobject(ST(1)) && sv_derived_from(ST(1), "Dyn::Type::Base")))
        croak("Given type for '%s' is not a subclass of Dyn::Type::Base", name);
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
        HV *type_registry = get_hv("Dyn::Type::_reg", GV_ADD);
        const char *type_str = SvPV_nolen(type);
        if (!hv_exists_ent(type_registry, type, 0)) croak("Type named '%s' is undefined", type_str);
        type = MUTABLE_SV(SvRV(*hv_fetch(type_registry, type_str, strlen(type_str), 0)));
    }
    ST(0) = newSVsv(type);
    XSRETURN(1);
}

static void *ref_pointer(pTHX_ SV *type, SV *value) {
    if (SvROK(value)) {
        void *pointer = ref_pointer(aTHX_ type, SvRV(value));
        return pointer;
    }
    if (sv_derived_from(type, "Dyn::Type::Pointer")) {
        HV *hv_ptr = MUTABLE_HV(type);
        SV **ptr_ptr = hv_fetchs(hv_ptr, "pointer", 1);

        IV tmp = SvIV((SV *)SvRV(*ptr_ptr));
        return INT2PTR(double *, tmp);
    }
    croak("Attempt to dereference non-pointer type");

    return NULL;
}

static void *deref_pointer(pTHX_ SV *type, SV *value, bool set) {
    void *RETVAL;
    {
        sv_dump(type);
        sv_dump(value);

        SV *pointer;
        HV *hv_ptr = MUTABLE_HV(SvRV(SvRV(type)));

        char type;
        if (hv_exists(hv_ptr, "type", 0)) {
            SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
            type = (SvPVbytex_nolen(*type_ptr))[0];
            // char * str = SvPVbytex_nolen(*type_ptr);
            // type = str[0];
        }
        else
            type = 'v'; // Default is opaque
        warn("here: %c at %s line %d.", type, __FILE__, __LINE__);
        SV *ptr;
        SV **ptr_ptr = hv_fetchs(hv_ptr, "pointer", 0);

        if (ptr_ptr == NULL) {
            // croak("Fudge.");
            {
                DCpointer pointer;
                // Newxz(pointer, 1, void); // TODO: correct type from type...
                ptr = newSV(1);
                sv_setref_pv(ptr, "Dyn::Call::Pointer", (DCpointer)pointer);
                // hv_stores(MUTABLE_HV(RETVAL_SV), "pointer", pointer_sv);
            }
        }
        else { ptr = *ptr_ptr; }
        switch (type) {
        case DC_SIGCHAR_VOID: {
            warn("Void pointer");

            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(void *, tmp); // huh?
            if (set)
                *((int *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv(SvRV(value), (IV) * (int *)RETVAL);
        } break;
        case DC_SIGCHAR_BOOL: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(bool *, tmp);
            if (set)
                *((bool *)RETVAL) = SvTRUE(SvRV(value));
            else
                sv_setbool_mg(SvRV(value), (bool)*(bool *)RETVAL);
        } break;
        case DC_SIGCHAR_CHAR: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(char *, tmp);
            if (set)
                *((char *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv(SvRV(value), (IV) * (char *)RETVAL);
        } break;
        case DC_SIGCHAR_UCHAR: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(unsigned char *, tmp);
            if (set)
                *((unsigned char *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setiv(SvRV(value), (UV) * (unsigned char *)RETVAL);
        } break;
        case DC_SIGCHAR_SHORT: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(short *, tmp);
            if (set)
                *((short *)RETVAL) = SvNV(SvRV(value));
            else
                sv_setiv(SvRV(value), (IV) * (short *)RETVAL);
        } break;
        case DC_SIGCHAR_USHORT: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(unsigned short *, tmp);
            if (set)
                *((unsigned short *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setuv(SvRV(value), (UV) * (unsigned short *)RETVAL);
        } break;
        case DC_SIGCHAR_INT: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(int *, tmp);
            if (set)
                *((int *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv(SvRV(value), (IV) * (int *)RETVAL);
        } break;
        case DC_SIGCHAR_UINT: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(unsigned int *, tmp);
            if (set)
                *((unsigned int *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setuv(SvRV(value), (UV) * (unsigned int *)RETVAL);
        } break;
        case DC_SIGCHAR_LONG: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(long *, tmp);
            if (set)
                *((long *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv(SvRV(value), (IV) * (long *)RETVAL);
        } break;
        case DC_SIGCHAR_ULONG: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(unsigned long *, tmp);
            if (set)
                *((unsigned long *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setuv(SvRV(value), (UV) * (unsigned long *)RETVAL);
        } break;
        case DC_SIGCHAR_LONGLONG: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(long long *, tmp);
            if (set)
                *((long long *)RETVAL) = SvIV(SvRV(value));
            else
                sv_setiv(SvRV(value), (IV) * (long long *)RETVAL);

        } break;
        case DC_SIGCHAR_ULONGLONG: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(unsigned long long *, tmp);
            if (set)
                *((unsigned long long *)RETVAL) = SvUV(SvRV(value));
            else
                sv_setuv(SvRV(value), (UV) * (unsigned long long *)RETVAL);
        } break;
        case DC_SIGCHAR_FLOAT: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(float *, tmp);
            if (set)
                *((float *)RETVAL) = SvNV(SvRV(value));
            else
                sv_setnv(SvRV(value), (NV) * (float *)RETVAL);
        } break;
        case DC_SIGCHAR_DOUBLE: {
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(double *, tmp);
            if (set)
                *((double *)RETVAL) = SvNV(SvRV(value));
            else
                sv_setnv(SvRV(value), (NV) * (double *)RETVAL);
        } break;
        case DC_SIGCHAR_POINTER: {
            warn("here at %s line %d.", __FILE__, __LINE__);
            IV tmp = SvIV((SV *)SvRV(ptr));
            RETVAL = (void *)INT2PTR(void **, tmp); // wut?
            // if (set) *((void *)RETVAL) = SvIV(SvRV(value));
        } break;
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

    warn("field_count == %d", field_count);

    return data;
}

XS_EUPXS(Types_type_call); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_type_call) {
    dXSARGS;
    dXSI32;
    dMY_CXT;
    // warn("Calling at %s line %d", __FILE__, __LINE__);

    Call *call = (Call *)XSANY.any_ptr;
    AV * args = NULL; // just in case...
    //AV *args = MUTABLE_AV(SvRV(MUTABLE_SV(call->args)));
    //SV *ret_type = (SvRV(call->retval));

    if (call->sig_len != items) {
        if (call->sig_len < items) croak("Too many arguments");
        if (call->sig_len > items) croak("Not enough arguments");
    }

    dcReset(MY_CXT.cvm);
    bool pointers = false;
    // warn("Calling at %s line %d", __FILE__, __LINE__);

    for (int i = 0; i < call->sig_len; ++i) {
        // warn("Working on element %d of %d at %s line %d", i, arg_count, __FILE__, __LINE__);

        switch (call->sig[i]) {
        case DC_SIGCHAR_BOOL:
            dcArgBool(MY_CXT.cvm, SvTRUE(ST(i)));
            break; // Anything can bee a bool
        case DC_SIGCHAR_CHAR:
            dcArgChar(MY_CXT.cvm, (char)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_UCHAR:
            dcArgChar(MY_CXT.cvm, (unsigned char)SvUV(ST(i)));
            break;
        case DC_SIGCHAR_SHORT:
            dcArgShort(MY_CXT.cvm, (short)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_USHORT:
            dcArgShort(MY_CXT.cvm, (unsigned short)SvUV(ST(i)));
            break;
        case DC_SIGCHAR_INT:
            dcArgInt(MY_CXT.cvm, (int)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_UINT:
            dcArgInt(MY_CXT.cvm, (unsigned int)SvUV(ST(i)));
            break;
        case DC_SIGCHAR_LONG:
            dcArgLong(MY_CXT.cvm, (long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_ULONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_LONGLONG:
            dcArgLongLong(MY_CXT.cvm, (long long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_ULONGLONG:
            dcArgLongLong(MY_CXT.cvm, (unsigned long long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_FLOAT:
            dcArgFloat(MY_CXT.cvm, (float)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_DOUBLE:
            dcArgDouble(MY_CXT.cvm, (double)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_POINTER: {
            DCpointer ptr;
            if (SvROK(ST(i))) {
                IV tmp = SvIV((SV *)SvRV(ST(i)));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else if (!SvOK(ST(i))) // Passed us an undef
                ;                  // ptr = NULL;
            else
                croak("Type of arg %d must be scalar ref", i + 1);
            // DCpointer ptr = deref_pointer(aTHX_ field, MUTABLE_SV(ST(i)), false);
            dcArgPointer(MY_CXT.cvm, ptr);
            pointers = true;
        } break;
        case DC_SIGCHAR_STRING: {
            dcArgPointer(MY_CXT.cvm, SvPOK(ST(i)) ? SvPV_nolen(ST(i)) : NULL);
        } break;
        case DC_SIGCHAR_CODE: {
            if(args == NULL)
                args = MUTABLE_AV(SvRV(MUTABLE_SV(call->args)));
            SV *field = *av_fetch(args, i, 0); // Make broad assumptions
            SV **cb = hv_fetchs(MUTABLE_HV(SvRV(field)), "callback", 0);
            DCCallback *callback;

            if (SvROK(*cb) && sv_derived_from(*cb, "Dyn::Callback")) {
                IV tmp = SvIV(SvRV(*cb));
                callback = INT2PTR(DCCallback *, tmp);
            }
            else
                croak("Malformed CodeRef type");

            _callback *container = (_callback *)dcbGetUserData(callback);
            CV *coderef;

            STMT_START {
                HV *st;
                GV *gvp;
                SV *const xsub_tmp_sv = ST(i);
                SvGETMAGIC(xsub_tmp_sv);
                coderef = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
                if (!coderef) croak("Type of arg %d must be code ref", i + 1);
            }
            STMT_END;
            container->cb = SvREFCNT_inc(MUTABLE_SV((coderef)));
            container->userdata = &PL_sv_undef; // items > 2 ? newRV_inc(ST(2)) : &PL_sv_undef;
            dcArgPointer(MY_CXT.cvm, callback);
        } break;
        case DC_SIGCHAR_ARRAY: {
            if(args == NULL)
                args = MUTABLE_AV(SvRV(MUTABLE_SV(call->args)));
            SV *field = *av_fetch(args, i, 0); // Make broad assumptions

            if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVAV)
                croak("Type of arg %d must be an array ref", i + 1);
            AV *elements = MUTABLE_AV(SvRV(ST(i)));

            SV *pointer;
            HV *hv_ptr = MUTABLE_HV(SvRV(field));
            SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
            SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
            // sv_dump(*type_ptr);
            // sv_dump(*size_ptr);
            size_t av_len = SvIV(*size_ptr);

            if (av_count(elements) != av_len)
                croak("Expected an array of %d elements; found %d", av_len, av_count(elements));

            intptr_t pos;
            DCpointer ptr;
            size_t size = _sizeof(aTHX_ field);
            Newxz(ptr, size, char);

            // double ptr[5] = {1, 2, 3, 17, 50};
            // pos = (intptr_t)(*((void **)ptr));
            warn("size == %d; sizeof(ptr) == %d", size, sizeof(ptr));

            coerce(aTHX_ field, ST(i), ptr, false, pos);
            // static intptr_t coerce(pTHX_ SV *type, SV *data, intptr_t pos, bool packed) {

            warn("sizeof(ptr) == %d", sizeof(ptr));

            DumpHex(ptr, size);
            // croak("I need to get this to load the elements into an array");
            DCaggr *ag = dcNewAggr(1, sizeof(ptr));

            // TODO: I need to set the correct type here
            dcAggrField(ag, DC_SIGCHAR_INT, 0, av_len);

            dcCloseAggr(ag);
            DumpHex(ptr, sizeof(ptr));

            dcArgPointer(MY_CXT.cvm, ptr);
        } break;
        case DC_SIGCHAR_STRUCT: {
            if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVHV)
                croak("Type of arg %d must be a hash ref", i + 1);
            if(args == NULL)
                args = MUTABLE_AV(SvRV(MUTABLE_SV(call->args)));
            SV *field = *av_fetch(args, i, 0); // Make broad assumptions
            DCaggr *agg = _aggregate(aTHX_ field);
            DCpointer ptr = sloppy_coerce(field, SvRV(ST(i)));

            dcArgAggr(MY_CXT.cvm, agg, ptr);
        } break;

        default:
            croak("--> Unfinished: [%c/%d]", call->sig[i], i);
                    if(args == NULL)
                args = MUTABLE_AV(SvRV(MUTABLE_SV(call->args)));
            SV *field = *av_fetch(args, i, 0); // Make broad assumptions
            if (sv_derived_from(field, "Dyn::Type::Struct")) {}
        }
    }

    // warn("Return type: %c at %s line %d", call->ret, __FILE__, __LINE__);

    SV *retval;
    {
        switch (call->ret) {
        case DC_SIGCHAR_VOID:
            dcCallVoid(MY_CXT.cvm, call->fptr);
            retval = newSV(0);
            break;
        case DC_SIGCHAR_BOOL:
            sv_setbool_mg(retval, (bool)dcCallBool(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_CHAR:
            retval = newSViv((char)dcCallChar(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_UCHAR:
            retval = newSVuv((unsigned char)dcCallChar(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_SHORT:
            retval = newSViv((short)dcCallShort(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_USHORT:
            retval = newSVuv((unsigned short)dcCallShort(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_INT:
            retval = newSViv((int)dcCallInt(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_UINT:
            retval = newSVuv((unsigned int)dcCallInt(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_LONG:
            retval = newSViv((long)dcCallLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_ULONG:
            retval = newSVuv((unsigned long)dcCallLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_LONGLONG:
            retval = newSViv((long long)dcCallLongLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_ULONGLONG:
            retval = newSVuv((unsigned long long)dcCallLongLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_FLOAT:
            retval = newSVnv((float)dcCallFloat(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_DOUBLE:
            retval = newSVnv((double)dcCallDouble(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_POINTER: {
            DCpointer ptr = dcCallPointer(MY_CXT.cvm, call->fptr);
            retval = newSV(1); // TODO: check if pointer can coerce when type is provided
            sv_setref_pv(retval, "Dyn::Call::Pointer",
                         ptr); // TODO: The Pointer[] might have a defined type
        } break;
        case DC_SIGCHAR_STRING:
            retval = newSVpv((char *)dcCallPointer(MY_CXT.cvm, call->fptr), 0);
            break;
        default:
            croak("Unhandled return type: %c", call->ret);
        }

        if (pointers) {
            for (int i = 0; i < call->sig_len; ++i) {
                switch (call->sig[i]) {
                case DC_SIGCHAR_POINTER: {
                    // DCpointer ptr = deref_pointer(aTHX_ field, MUTABLE_SV(ST(i)), false);
                    SvSETMAGIC(SvRV(ST(i)));
                    break;
                }
                default:
                    break;
                }
            }
        }
        if(call->ret == DC_SIGCHAR_VOID)
            XSRETURN_EMPTY;
        ST(0) = sv_2mortal(retval);
        XSRETURN(1);
    }
}

XS_EUPXS(Types_type_callx); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Types_type_callx) {
    dXSARGS;
    dXSI32;
    dMY_CXT;

    Call *call = (Call *)XSANY.any_ptr;
    warn("Here at %s line %d", __FILE__, __LINE__);
    DCpointer ptr;
    SV *ret_type = SvRV(call->retval);
    warn("Here at %s line %d", __FILE__, __LINE__);

    AV *args = MUTABLE_AV(SvRV(MUTABLE_SV(call->args)));

    size_t arg_count = av_count(args);

    warn("Here at %s line %d [%d]", __FILE__, __LINE__, arg_count);

    if (arg_count != items) {
        if (arg_count < items) croak("Too many arguments");
        if (arg_count > items) croak("Not enough arguments");
    }
    warn("Here at %s line %d", __FILE__, __LINE__);
    dcMode(MY_CXT.cvm, call->mode);
    dcReset(MY_CXT.cvm);
    bool pointers = false;
    DCaggr *ag;
    warn("Here at %s line %d", __FILE__, __LINE__);
    for (int i = 0; i < arg_count; ++i) {
        SV *field = *av_fetch(args, i, 0); // Make broad assumptions
        switch (call->sig[0]) {
        case DC_SIGCHAR_BOOL:
            dcArgDouble(MY_CXT.cvm, SvTRUE(ST(i)));
            break; // Anything can bee a bool
        case DC_SIGCHAR_CHAR:
            dcArgChar(MY_CXT.cvm, (char)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_UCHAR:
            dcArgChar(MY_CXT.cvm, (unsigned char)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_SHORT:
            dcArgShort(MY_CXT.cvm, (short)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_USHORT:
            dcArgShort(MY_CXT.cvm, (unsigned short)SvUV(ST(i)));
            break;
        case DC_SIGCHAR_INT:
            dcArgInt(MY_CXT.cvm, (int)SvIV(ST(i)));
            break;
        case DC_SIGCHAR_UINT:
            dcArgInt(MY_CXT.cvm, (unsigned int)SvUV(ST(i)));
            break;
        case DC_SIGCHAR_LONG:
            dcArgLong(MY_CXT.cvm, (long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_ULONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_LONGLONG:
            dcArgLongLong(MY_CXT.cvm, (long long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_ULONGLONG:
            dcArgLongLong(MY_CXT.cvm, (unsigned long long)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_FLOAT:
            dcArgFloat(MY_CXT.cvm, (float)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_DOUBLE:
            // if (!SvNIOK(ST(i)))
            //     croak("Expected a double in the %d%s parameter", i + 1, ordinal(i + 1));
            dcArgDouble(MY_CXT.cvm, (double)SvNV(ST(i)));
            break;
        case DC_SIGCHAR_POINTER: {
            if (!SvROK(ST(i))) croak("Type of arg %d must be scalar ref", i + 1);
            DCpointer ptr = deref_pointer(aTHX_ field, MUTABLE_SV(ST(i)), false);
            dcArgPointer(MY_CXT.cvm, ptr);
            pointers = true;
        } break;
        case DC_SIGCHAR_STRING: {
            if (!SvPOK(ST(i))) croak("Type of arg %d must be a string", i + 1);
            char *string = SvPV_nolen(ST(i));
            // DCpointer ptr = deref_pointer(aTHX_ field, MUTABLE_SV(ST(i)), false);
            dcArgPointer(MY_CXT.cvm, string);
            // pointers = true;
        } break;
        case DC_SIGCHAR_CODE: {
            SV **cb = hv_fetchs(MUTABLE_HV(SvRV(field)), "callback", 0);
            DCCallback *callback;

            if (SvROK(*cb) && sv_derived_from(*cb, "Dyn::Callback")) {
                IV tmp = SvIV(SvRV(*cb));
                callback = INT2PTR(DCCallback *, tmp);
            }
            else
                croak("Malformed CodeRef type");

            _callback *container = (_callback *)dcbGetUserData(callback);
            CV *coderef;

            STMT_START {
                HV *st;
                GV *gvp;
                SV *const xsub_tmp_sv = ST(i);
                SvGETMAGIC(xsub_tmp_sv);
                coderef = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
                if (!coderef) croak("Type of arg %d must be code ref", i + 1);
            }
            STMT_END;
            container->cb = SvREFCNT_inc(MUTABLE_SV((coderef)));
            container->userdata = &PL_sv_undef; // items > 2 ? newRV_inc(ST(2)) : &PL_sv_undef;
            dcArgPointer(MY_CXT.cvm, callback);
        } break;
        case DC_SIGCHAR_ARRAY: {
            if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVAV)
                croak("Type of arg %d must be an array ref", i + 1);
            AV *elements = MUTABLE_AV(SvRV(ST(i)));

            SV *pointer;
            HV *hv_ptr = MUTABLE_HV(SvRV(field));
            SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
            SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
            // sv_dump(*type_ptr);
            // sv_dump(*size_ptr);
            size_t av_len = SvIV(*size_ptr);

            if (av_count(elements) != av_len)
                croak("Expected an array of %d elements; found %d", av_len, av_count(elements));

            intptr_t pos;
            DCpointer ptr;
            size_t size = _sizeof(aTHX_ field);
            Newxz(ptr, size, char);

            // double ptr[5] = {1, 2, 3, 17, 50};
            // pos = (intptr_t)(*((void **)ptr));
            warn("size == %d; sizeof(ptr) == %d", size, sizeof(ptr));

            coerce(aTHX_ field, ST(i), ptr, false, pos);
            // static intptr_t coerce(pTHX_ SV *type, SV *data, intptr_t pos, bool packed) {

            warn("sizeof(ptr) == %d", sizeof(ptr));

            DumpHex(ptr, size);
            // croak("I need to get this to load the elements into an array");

            ag = dcNewAggr(1, sizeof(ptr));

            // TODO: I need to set the correct type here
            dcAggrField(ag, DC_SIGCHAR_INT, 0, av_len);

            dcCloseAggr(ag);
            DumpHex(ptr, sizeof(ptr));

            dcArgPointer(MY_CXT.cvm, ptr);
        } break;
        case DC_SIGCHAR_STRUCT: {
            if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVHV)
                croak("Type of arg %d must be a hash ref", i + 1);

            DCaggr *agg = _aggregate(aTHX_ field);
            DCpointer ptr = sloppy_coerce(field, SvRV(ST(i)));

            dcArgAggr(MY_CXT.cvm, agg, ptr);
        } break;

        default:
            croak("--> Unfinished: [%c/%d]", call->sig[i], i);
            if (sv_derived_from(field, "Dyn::Type::Struct")) {}
        }
    }
    warn("Here at %s line %d", __FILE__, __LINE__);

    // dXSTARG;

    warn("Here at %s line %d", __FILE__, __LINE__);

    SV *retval;
    {
        switch (call->ret) {
        case DC_SIGCHAR_VOID:
            dcCallVoid(MY_CXT.cvm, call->fptr);
            break;
        case DC_SIGCHAR_BOOL:
            sv_setbool_mg(retval, (bool)dcCallBool(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_CHAR:
            retval = newSViv((char)dcCallChar(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_UCHAR:
            retval = newSVuv((unsigned char)dcCallChar(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_SHORT:
            retval = newSViv((short)dcCallShort(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_USHORT:
            retval = newSVuv((unsigned short)dcCallShort(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_INT:
            retval = newSViv((int)dcCallInt(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_UINT:
            retval = newSVuv((unsigned int)dcCallInt(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_LONG:
            retval = newSViv((long)dcCallLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_ULONG:
            retval = newSVuv((unsigned long)dcCallLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_LONGLONG:
            retval = newSViv((long long)dcCallLongLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_ULONGLONG:
            retval = newSVuv((unsigned long long)dcCallLongLong(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_FLOAT:
            retval = newSVnv((float)dcCallFloat(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_DOUBLE:
            retval = newSVnv((double)dcCallDouble(MY_CXT.cvm, call->fptr));
            break;
        case DC_SIGCHAR_POINTER: {
            DCpointer ptr = dcCallPointer(MY_CXT.cvm, call->fptr);
            retval = newSV(0); // TODO: check if pointer can coerce when type is provided
            sv_setref_pv(retval, "Dyn::Call::Pointer", ptr);
        } break;
        case DC_SIGCHAR_STRING:
            retval = newSVpv((char *)dcCallPointer(MY_CXT.cvm, call->fptr), 0);
            break;
        default:
            croak("Unhandled return type: %c", call->ret);
        }
        if (pointers) {
            for (int i = 0; i < arg_count; ++i) {
                /*//sv_dump(ST(i));
                //sv_dump(field);*/
                switch (call->sig[i]) {
                case DC_SIGCHAR_POINTER: {
                    SV *field = *av_fetch(MUTABLE_AV(call->args), i, 0); // Make broad assumptions
                    DCpointer ptr = deref_pointer(aTHX_ field, MUTABLE_SV(ST(i)), false);
                    SvSETMAGIC(SvRV(ST(i)));
                    break;
                }
                default:
                    break;
                }
            }
        }

        switch (call->ret) {
        case DC_SIGCHAR_VOID:
            XSRETURN_EMPTY;
            break;

        default:
            ST(0) = sv_2mortal(retval);
            XSRETURN(1);
        }
    }
}

static Call *_load(pTHX_ DLLib *lib, const char *symbol, AV *args, SV *retval, DCint mode) {
    if (lib == NULL) return NULL;
    // warn("_load(..., %s, '%c')", symbol, mode);
    Call *RETVAL;
    Newx(RETVAL, 1, Call);


    RETVAL->lib = lib;
    RETVAL->fptr = dlFindSymbol(RETVAL->lib, symbol);
    int len = av_count(args);

    if (RETVAL->fptr == NULL) { // TODO: throw warning
        safefree(RETVAL);
        return NULL;
    }

    warn("Here at %s line %d", __FILE__, __LINE__);

    sv_dump(MUTABLE_SV(args));

    RETVAL->args = MUTABLE_AV(newRV_inc(MUTABLE_SV(args)));
    RETVAL->retval = newRV_inc(retval);

    char perl_sig[len + 2];
    char c_sig[len + 2];
    for (int i = 0; i < len; ++i) {
        SV **type_ref = av_fetch(args, i, 0);
        if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Dyn::Type::Base")))
            croak("Given type for arg %d is not a subclass of Dyn::Type::Base", i);
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
    perl_sig[len] = (char)0;
    c_sig[len] = (char)0;

    Newxz(RETVAL->perl_sig, len, char);
    Newxz(RETVAL->sig, len, char);

    Copy(perl_sig, RETVAL->perl_sig, strlen(perl_sig), char);
    Copy(c_sig, RETVAL->sig, strlen(c_sig), char);
    {
        char *str = SvPVbytex_nolen(retval);
        RETVAL->ret = str[0];
    }
    return RETVAL;
}

static Call *_loadX(pTHX_ DLLib *lib, const char *symbol, AV *args, SV *retval, DCint mode) {
    if (lib == NULL) return NULL;
    // warn("_load(..., %s, '%c')", symbol, mode);
    Call *RETVAL;
    Newx(RETVAL, 1, Call);

    RETVAL->mode = mode;
    RETVAL->lib = lib;
    RETVAL->fptr = dlFindSymbol(RETVAL->lib, symbol);
    int len = av_count(args);

    if (RETVAL->fptr == NULL) { // TODO: throw warning
        safefree(RETVAL);
        return NULL;
    }

    RETVAL->args = MUTABLE_AV(newRV_inc(MUTABLE_SV(args)));
    RETVAL->retval = newRV_inc(retval);

    // RETVAL->args = MUTABLE_AV(sv_mortalcopy(MUTABLE_SV(args))); //MUTABLE_AV(newRV_inc());
    // RETVAL->retval = newRV_inc(newSVsv(retval));

    char perl_sig[len + 2];
    char c_sig[len + 2];
    for (int i = 0; i < len; ++i) {
        SV **type_ref = av_fetch(args, i, 0);
        if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Dyn::Type::Base")))
            croak("Given type for arg %d is not a subclass of Dyn::Type::Base", i);
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
    perl_sig[len] = (char)0;
    c_sig[len] = (char)0;

    Newxz(RETVAL->perl_sig, len, char);
    Newxz(RETVAL->sig, len, char);

    Copy(perl_sig, RETVAL->perl_sig, strlen(perl_sig), char);
    Copy(c_sig, RETVAL->sig, strlen(c_sig), char);
    {
        char *str = SvPVbytex_nolen(retval);
        RETVAL->ret = str[0];
    }
    return RETVAL;
}

XS_EUPXS(XS_Dyn_attacht);
XS_EUPXS(XS_Dyn_attacht) {
    dVAR;
    dXSARGS;

    if (items < 4 || items > 6)
        croak_xs_usage(cv, "lib, symbol_name, args, return, mode = DC_SIGCHAR_CC_DEFAULT, "
                           "func_name = symbol_name");
    {
        Call *call;
        DLLib *lib;
        SV *RETVAL;
        const char *symbol_name = (const char *)SvPV_nolen(ST(1));
        AV *args;
        STMT_START {
            SV *const xsub_tmp_sv = ST(2);
            SvGETMAGIC(xsub_tmp_sv);
            if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                args = (AV *)SvRV(xsub_tmp_sv);
            }
            else {
                Perl_croak_nocontext("%s: %s is not an ARRAY reference", "Dyn::attach", "types");
            }
        }
        STMT_END;
        SV *retval = newSVsv(ST(3));
        const char *func_name;
        DCint mode;
        if (items == 4)
            mode = DC_SIGCHAR_CC_DEFAULT;
        else if (SvIOK(ST(4)))
            mode = SvIV(ST(4));
        else {
            char *junk = SvPV_nolen(ST(4));
            mode = (int)junk[0];
        }
        if (items > 5)
            func_name = (const char *)SvPV_nolen(ST(5));
        else
            func_name = NULL;
        if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::Load::Lib")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            lib = INT2PTR(DLLib *, tmp);
        }
        else {
            const char *lib_name = (const char *)SvPV_nolen(ST(0));
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
                croak("Failed to load %s", reason);
#endif
            }
        }

        call = _load(aTHX_ lib, symbol_name, args, retval, mode);

        if (call == NULL) croak("Failed to attach %s", symbol_name);
        /* Create a new XSUB instance at runtime and set it's XSANY.any_ptr to contain the
         * necessary user data. name can be NULL => fully anonymous sub!
         **/
        CV *cv;
        STMT_START {

            // cv = newXSproto_portable(func_name, XS_Dyn__call_Dyn, (char*)__FILE__,
            // call->perl_sig);
            // cv = get_cvs("Dyn::_call_Dyn", 0)

            cv = newXSproto_portable(func_name, Types_type_call, (char *)__FILE__, call->perl_sig);
            ////warn("N");

            if (cv == NULL) croak("ARG! Something went really wrong while installing a new XSUB!");
            ////warn("Q");
            XSANY.any_ptr = (void *)call;
        }
        STMT_END;

        ST(0) =
            // func_name == NULL ?
            sv_2mortal(sv_bless((newRV_inc((SV *)cv)), gv_stashpv("Dyn", GV_ADD))) //:
            // sv_2mortal(newRV_inc((SV *)cv))
            ;
    }
    XSRETURN(1);
}

XS_EUPXS(XS_Dyn_attachx);
XS_EUPXS(XS_Dyn_attachx) {
    dVAR;
    dXSARGS;

    if (items < 4 || items > 6)
        croak_xs_usage(cv, "lib, symbol_name, args, return, mode = DC_SIGCHAR_CC_DEFAULT, "
                           "func_name = symbol_name");
    {
        Call *call;
        DLLib *lib;
        const char *symbol_name = (const char *)SvPV_nolen(ST(1));
        AV *args;
        STMT_START {
            SV *const xsub_tmp_sv = ST(2);
            SvGETMAGIC(xsub_tmp_sv);
            if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                args = (AV *)SvRV(xsub_tmp_sv);
            }
            else {
                Perl_croak_nocontext("%s: %s is not an ARRAY reference", "Dyn::attach", "types");
            }
        }
        STMT_END;
        SV *retval = newSVsv(ST(3));
        const char *func_name;
        DCint mode;
        if (items == 4)
            mode = DC_SIGCHAR_CC_DEFAULT;
        else if (SvIOK(ST(4)))
            mode = SvIV(ST(4));
        else {
            char *junk = SvPV_nolen(ST(4));
            mode = (int)junk[0];
        }
        if (items > 5)
            func_name = (const char *)SvPV_nolen(ST(5));
        else
            func_name = NULL;

        if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::Load::Lib")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            lib = INT2PTR(DLLib *, tmp);
        }
        else {
            const char *lib_name = (const char *)SvPV_nolen(ST(0));
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
                croak("Failed to load %s", reason);
#endif
            }
        }

        call = _load(aTHX_ lib, symbol_name, args, retval, mode);
        if (call == NULL) croak("Failed to attach %s", symbol_name);
        /* Create a new XSUB instance at runtime and set it's XSANY.any_ptr to contain the
         * necessary user data. name can be NULL => fully anonymous sub!
         **/
        CV *cv;
        STMT_START {
            // cv = newXSproto_portable(func_name, XS_Dyn__call_Dyn, (char*)__FILE__,
            // call->perl_sig);
            // cv = get_cvs("Dyn::_call_Dyn", 0)
            cv = newXSproto_portable(func_name, Types_type_call, (char *)__FILE__, call->perl_sig);
            ////warn("N");

            if (cv == NULL) croak("ARG! Something went really wrong while installing a new XSUB!");
            ////warn("Q");

            {
                /*
                        cv = newXSproto_portable(package, Types, file, ";$"); \
                        Newx(XSANY.any_ptr, strlen(package) + 1, char); \
                        Copy(package, XSANY.any_ptr, strlen(package) + 1, char); \
                        cv = newXSproto_portable(form("%s::new", package), Types, file, "$"); \
                        safefree(XSANY.any_ptr); \


                */

                Newx(XSANY.any_ptr, 1, Call);
                Copy(call, XSANY.any_ptr, 1, Call);
                // XSANY.any_ptr = (DCpointer)call;
                warn("signature was %s", ((Call *)XSANY.any_ptr)->sig);
            }

            SV *RETVAL;
            RETVAL = (sv_bless(newRV_noinc(MUTABLE_SV(cv)), gv_stashpv("Dyn", GV_ADD)));
            RETVAL = sv_2mortal(RETVAL);
            ST(0) = RETVAL;
            // sv_2mortal(MUTABLE_SV(cv));
        }
        STMT_END;

        XSRETURN(1);
        return;

        ST(0) = ((sv_bless(newRV_inc(MUTABLE_SV(cv)), gv_stashpv("Dyn", GV_ADD))));
    }
    XSRETURN(1);
}

XS_EUPXS(Dyn_DESTROY); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(Dyn_DESTROY) {
    dVAR;
    dXSARGS;
    // warn("Dyn::DESTROY");
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
        const char *package = form("Dyn::%s", NAME);                                               \
        cv = newXSproto_portable(package, Types_wrapper, file, ";$");                              \
        Newx(XSANY.any_ptr, strlen(package) + 1, char);                                            \
        Copy(package, XSANY.any_ptr, strlen(package) + 1, char);                                   \
        cv = newXSproto_portable(form("%s::new", package), Types, file, "$");                      \
        safefree(XSANY.any_ptr);                                                                   \
        XSANY.any_i32 = (int)SIGCHAR;                                                              \
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
        /* Making a sub named "Dyn::Call::Aggregate::()" allows the package */                     \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Dyn::Call::Aggregate") to return true. */                         \
        (void)newXSproto_portable(form("%s::()", package), Types_sig, file, ";$");                 \
        set_isa(package, "Dyn::Type::Base");                                                       \
    }

MODULE = Dyn PACKAGE = Dyn

BOOT :
#ifdef USE_ITHREADS
    my_perl = (PerlInterpreter *)PERL_GET_CONTEXT;
#endif
    {
        MY_CXT_INIT;
        MY_CXT.count = 0;
        MY_CXT.cvm = dcNewCallVM(4096);
    }
    {
        //(void)newXSproto_portable("Dyn::attach", XS_Dyn_attach, file, "$$@$");
        (void)newXSproto_portable("Dyn::coerce", Dyn_coerce, file, "$$");
        (void)newXSproto_portable("Dyn::typedef", Types_typedef, file, "$$");
        (void)newXSproto_portable("Dyn::Type", Types_type, file, "$");
        (void)newXSproto_portable("Dyn::Type::Base::sizeof", Types_type_sizeof, file, "$");
        (void)newXSproto_portable("Dyn::Type::Base::aggregate", Types_type_aggregate, file, "$");
        (void)newXSproto_portable("Dyn::DESTROY", Dyn_DESTROY, file, "$");

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
        // Enum[]?
    }

SV *
attach(lib, symbol, args, ret, mode = DC_SIGCHAR_CC_DEFAULT, func_name = NULL)
    const char * symbol
    AV * args
    SV * ret
    char mode
    const char * func_name
PREINIT :
    dMY_CXT;
INIT:
CODE:
    Call *call;
    DLLib *lib;

    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::Load::Lib")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else {
        const char *lib_name = (const char *)SvPV_nolen(ST(0));
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
            croak("Failed to load %s", reason);
#endif
        }
    }
    if (lib == NULL) XSRETURN_EMPTY;
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

    call->args = MUTABLE_AV(newRV_inc(MUTABLE_SV(args)));
    call->retval = SvREFCNT_inc(ret);

    char perl_sig[call->sig_len];
    char c_sig[call->sig_len];
    for (int i = 0; i < call->sig_len; ++i) {
        SV **type_ref = av_fetch(args, i, 0);
        if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Dyn::Type::Base")))
            croak("Given type for arg %d is not a subclass of Dyn::Type::Base", i);
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

        // cv = newXSproto_portable(func_name, XS_Dyn__call_Dyn, (char*)__FILE__,
        // call->perl_sig);
        // cv = get_cvs("Dyn::_call_Dyn", 0)

        cv = newXSproto_portable(func_name, Types_type_call, (char *)__FILE__, call->perl_sig);

        if (cv == NULL) croak("ARG! Something went really wrong while installing a new XSUB!");
        ////warn("Q");
        XSANY.any_ptr = (DCpointer)call;
    }
    STMT_END;
    RETVAL = sv_bless((func_name == NULL ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                      gv_stashpv("Dyn", GV_ADD));
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

