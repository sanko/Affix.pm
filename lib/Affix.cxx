#include "Affix.h"

/* globals */
#define MY_CXT_KEY "Affix::_guts" XS_VERSION

typedef struct {
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

static DCaggr *_aggregate(pTHX_ SV *type) {
    int t = SvIV(type);
    size_t size = _sizeof(aTHX_ type);
    switch (t) {
    case AFFIX_ARG_CSTRUCT:
    case AFFIX_ARG_CARRAY:
    case AFFIX_ARG_CUNION: {
        PING;
        HV *hv_type = MUTABLE_HV(SvRV(type));
        SV **agg_ = hv_fetch(hv_type, "aggregate", 9, 0);
        if (agg_ != NULL) {
            PING;
            SV *agg = *agg_;
            if (sv_derived_from(agg, "Affix::Aggregate")) {
                IV tmp = SvIV((SV *)SvRV(agg));
                return INT2PTR(DCaggr *, tmp);
            }
            else
                croak("Oh, no...");
        }
        else {
            PING;
            SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);
            //~ if (t == AFFIX_ARG_CSTRUCT) {
            //~ SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
            //~ }
            AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
            size_t field_count = av_count(idk_arr);
            DCaggr *agg = dcNewAggr(field_count, size);
            PING;
            for (size_t i = 0; i < field_count; ++i) {
                SV **field_ptr = av_fetch(idk_arr, i, 0);
                AV *field = MUTABLE_AV(SvRV(*field_ptr));
                SV **type_ptr = av_fetch(field, 1, 0);
                size_t offset = _offsetof(aTHX_ * type_ptr);
                int _t = SvIV(*type_ptr);
                switch (_t) {
                case AFFIX_ARG_CSTRUCT:
                case AFFIX_ARG_CUNION: {
                    PING;
                    DCaggr *child = _aggregate(aTHX_ * type_ptr);
                    dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                } break;
                case AFFIX_ARG_CARRAY: {
                    int array_len = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "size", 0));
                    dcAggrField(agg, type_as_dc(_t), offset, array_len);
                } break;
                default: {
                    //~ warn("  dcAggrField(agg, %c, %d, 1);", type_as_dc(_t), offset);
                    dcAggrField(agg, type_as_dc(_t), offset, 1);
                } break;
                }
            }
            dcCloseAggr(agg);
            {
                SV *RETVALSV;
                RETVALSV = newSV(1);
                sv_setref_pv(RETVALSV, "Affix::Aggregate", (DCpointer)agg);
                hv_stores(MUTABLE_HV(SvRV(type)), "aggregate", newSVsv(RETVALSV));
            }
            return agg;
        }
    } break;
    default: {
        croak("unsupported aggregate: %s at %s line %d", type_as_str(t), __FILE__, __LINE__);
        break;
    }
    }
    PING;
    return NULL;
}

typedef struct { // Used in CUnion and pin()
    void *ptr;
    SV *type_sv;
} var_ptr;

char *locate_lib(pTHX_ SV *_lib, SV *_ver) {
    // Use perl to get the actual path to the library
    dSP;
    int count;
    char *retval = NULL;
    //~ if (!SvOK(_lib)) {
    //~ GV *tmpgv = gv_fetchpvs("\030", GV_ADD | GV_NOTQUAL, SVt_PV); /* $^X */
    //~ _lib = GvSV(tmpgv);
    //~ }
    if (SvOK(_lib) /*&& SvREADONLY(_lib)*/) {
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        XPUSHs(_lib);
        XPUSHs(_ver);
        PUTBACK;
        count = call_pv("Affix::locate_lib", G_SCALAR);
        SPAGAIN;
        if (count == 1) {
            SV *ret = POPs;
            if (SvOK(ret)) {
                STRLEN len;
                //~ sv_dump(ret);
                char *__lib = SvPVx(ret, len);
                if (len) {
                    Newxz(retval, len + 1, char);
                    Copy(__lib, retval, len, char);
                }
            }
        }
        PUTBACK;
        FREETMPS;
        LEAVE;
    }
    return retval;
}

char *_mangle(pTHX_ const char *abi, SV *lib, const char *symbol, SV *args) {
    char *retval;
    {
        dSP;
        SV *err_tmp;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        XPUSHs(lib);
        mXPUSHp(symbol, strlen(symbol));
        XPUSHs(args);
        PUTBACK;
        (void)call_pv(form("Affix::%s_mangle", abi), G_SCALAR | G_EVAL | G_KEEPERR);
        SPAGAIN;
        err_tmp = ERRSV;
        if (SvTRUE(err_tmp)) {
            croak("Malformed call to %s_mangle( ... ): %s\n", abi, SvPV_nolen(err_tmp));
            (void)POPs;
        }
        else {
            retval = POPp;
            // SvSetMagicSV(type, retval);
        }
        // FREETMPS;
        LEAVE;
    }
    return retval;
}

#ifdef DC__OS_Win64
#include <cinttypes>
static const char *dlerror(void) {
    static char buf[1024];
    DWORD dw = GetLastError();
    if (dw == 0) return NULL;
    snprintf(buf, 32, "error 0x%" PRIx32 "", dw);
    return buf;
}
#endif

/* Affix::pin( ... ) System
Bind an exported variable to a perl var */
int get_pin(pTHX_ SV *sv, MAGIC *mg) {
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    SV *val = ptr2sv(aTHX_ ptr->ptr, ptr->type_sv);
    sv_setsv((sv), val);
    return 0;
}

int set_pin(pTHX_ SV *sv, MAGIC *mg) {
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    if (SvOK(sv)) sv2ptr(aTHX_ ptr->type_sv, sv, ptr->ptr, false);
    return 0;
}

int free_pin(pTHX_ SV *sv, MAGIC *mg) {
    PERL_UNUSED_VAR(sv);
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    sv_2mortal(ptr->type_sv);
    safefree(ptr);
    return 0;
}

static MGVTBL pin_vtbl = {
    get_pin,  // get
    set_pin,  // set
    NULL,     // len
    NULL,     // clear
    free_pin, // free
    NULL,     // copy
    NULL,     // dup
    NULL      // local
};

XS_INTERNAL(Affix_pin) {
    dXSARGS;
    if (items != 4) croak_xs_usage(cv, "var, lib, symbol, type");
    DLLib *_lib;
    // pin( my $integer, 't/src/58_affix_import_vars', 'integer', Int );

    if (!SvOK(ST(1)))
        _lib = NULL;
    else if (SvROK(ST(1)) && sv_derived_from(ST(1), "Affix::Lib")) {
        IV tmp = SvIV(MUTABLE_SV(SvRV(ST(1))));
        _lib = INT2PTR(DLLib *, tmp);
    }
    else {
        SV *lib, *ver;
        if (UNLIKELY(SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV)) {
            AV *tmp = MUTABLE_AV(SvRV(ST(1)));
            size_t tmp_len = av_count(tmp);
            // Non-fatal
            if (UNLIKELY(!(tmp_len == 1 || tmp_len == 2))) { warn("Expected a lib and version"); }
            lib = *av_fetch(tmp, 0, false);
            ver = *av_fetch(tmp, 1, false);
        }
        else {
            lib = newSVsv(ST(1));
            ver = newSV(0);
        }
        const char *_libpath = locate_lib(aTHX_ lib, ver);
        _lib =
#if defined(DC__OS_Win64) || defined(DC__OS_MacOSX)
            dlLoadLibrary(_libpath);
#else
            (DLLib *)dlopen(_libpath, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (_lib == NULL) {
            croak("Failed to load %s: %s", _libpath, dlerror());
        }
    }
    const char *symbol = (const char *)SvPV_nolen(ST(2));
    DCpointer ptr = dlFindSymbol(_lib, symbol);
    if (ptr == NULL) { croak("Failed to locate '%s'", symbol); }
    SV *sv = ST(0);
    MAGIC *mg = sv_magicext(sv, NULL, PERL_MAGIC_ext, &pin_vtbl, NULL, 0);
    {
        var_ptr *_ptr;
        Newx(_ptr, 1, var_ptr);
        _ptr->ptr = ptr;
        _ptr->type_sv = newSVsv(ST(3));
        mg->mg_ptr = (char *)_ptr;
    }
    XSRETURN_YES;
}

XS_INTERNAL(Affix_unpin) {
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "var");
    if (mg_findext(ST(0), PERL_MAGIC_ext, &pin_vtbl) &&
        !sv_unmagicext(ST(0), PERL_MAGIC_ext, &pin_vtbl))
        XSRETURN_YES;
    XSRETURN_NO;
}

SIMPLE_TYPE(Any);
SIMPLE_TYPE(Void);
SIMPLE_TYPE(Bool);
SIMPLE_TYPE(Char);
SIMPLE_TYPE(UChar);
SIMPLE_TYPE(WChar);
SIMPLE_TYPE(Short);
SIMPLE_TYPE(UShort);
SIMPLE_TYPE(Int);
SIMPLE_TYPE(UInt);
SIMPLE_TYPE(Long);
SIMPLE_TYPE(ULong);
SIMPLE_TYPE(LongLong);
SIMPLE_TYPE(ULongLong);
SIMPLE_TYPE(Size_t);
SIMPLE_TYPE(SSize_t);
SIMPLE_TYPE(Float);
SIMPLE_TYPE(Double);
SIMPLE_TYPE(Str);
SIMPLE_TYPE(WStr);

CC(DEFAULT);
CC(THISCALL);
CC(THISCALL_MS);
CC(THISCALL_GNU);
CC(CDECL);
CC(ELLIPSIS);
CC(ELLIPSIS_VARARGS);
CC(SYSCALL);
CC(STDCALL);
CC(FASTCALL_MS);
CC(FASTCALL_GNU);
CC(ARM_ARM);
CC(ARM_THUMB);

XS_INTERNAL(Affix_Type_CodeRef) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    if (av_count(fields) == 2) {
        char *sig;
        size_t field_count;
        {
            SV *arg_ptr = *av_fetch(fields, 0, 0);
            AV *args = MUTABLE_AV(SvRV(arg_ptr));
            field_count = av_count(args);
            Newxz(sig, field_count + 2, char);
            for (size_t i = 0; i < field_count; ++i) {
                SV **type_ref = av_fetch(args, i, 0);
                SV *type = *type_ref;
                if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                    croak("%s is not a subclass of "
                          "Affix::Type::Base",
                          SvPV_nolen(type));
                sig[i] = type_as_dc(SvIV(type));
            }
            hv_stores(RETVAL_HV, "args", SvREFCNT_inc(arg_ptr));
        }

        {
            sig[field_count] = ')';
            SV **ret_ref = av_fetch(fields, 1, 0);
            SV *ret = *ret_ref;
            if (!(sv_isobject(ret) && sv_derived_from(ret, "Affix::Type::Base")))
                croak("CodeRef[...] expects a return type that is a subclass of "
                      "Affix::Type::Base");
            sig[field_count + 1] = type_as_dc(SvIV(ret));
            hv_stores(RETVAL_HV, "ret", SvREFCNT_inc(ret));
        }

        {
            SV *signature = newSVpv(sig, field_count + 2);
            SvREADONLY_on(signature);
            hv_stores(RETVAL_HV, "sig", signature);
        }
    }
    else
        croak("CodeRef[...] expects a list of argument types and a single return type. e.g. "
              "CodeRef[[Int, Char, Str] => Void]");
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::CodeRef", GV_ADD)));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_ArrayRef) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    size_t fields_count = av_count(fields);
    SV *type;
    size_t array_length, array_sizeof = 0;
    bool packed = false;
    {
        type = *av_fetch(fields, 0, 0);
        if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
            croak("ArrayRef[...] expects a type that is a subclass of Affix::Type::Base");
        hv_stores(RETVAL_HV, "type", SvREFCNT_inc(type));
    }
    size_t type_alignof = _alignof(aTHX_ type);
    if (UNLIKELY(fields_count == 1)) {
        // wait for dynamic _sizeof(...) calculation
    }
    else if (fields_count == 2) {
        array_length = SvUV(*av_fetch(fields, 1, 0));
        size_t type_sizeof = _sizeof(aTHX_ type);
        for (size_t i = 0; i < array_length; ++i) {
            array_sizeof += type_sizeof;
            array_sizeof += packed ? 0
                                   : padding_needed_for(array_sizeof, type_alignof > type_sizeof
                                                                          ? type_sizeof
                                                                          : type_alignof);
        }
        hv_stores(RETVAL_HV, "sizeof", newSVuv(array_sizeof));
        hv_stores(RETVAL_HV, "size", newSVuv(array_length));
    }
    else
        croak("ArrayRef[...] expects a type and size. e.g ArrayRef[Int, 50]");

    hv_stores(RETVAL_HV, "align", newSVuv(type_alignof));
    hv_stores(RETVAL_HV, "name", newSV(0));
    hv_stores(RETVAL_HV, "packed", boolSV(packed));

    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::ArrayRef", GV_ADD)));

    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Struct) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields_in = MUTABLE_AV(SvRV(ST(0)));
    size_t field_count = av_count(fields_in);

    if (!(field_count % 2)) {
        bool packed = false; // TODO: handle packed structs correctly
        hv_stores(RETVAL_HV, "packed", boolSV(packed));
        AV *fields = newAV();
        size_t field_count = av_count(fields_in), size = 0;
        for (size_t i = 0; i < field_count; i += 2) {
            AV *field = newAV();
            SV *key = *av_fetch(fields_in, i, 0);
            //~ DD(key);
            if (!SvPOK(key)) croak("Given name of '%s' is not a string", SvPV_nolen(key));
            SV *type = *av_fetch(fields_in, i + 1, 0);
            //~ DD(type);
            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base"))) {
                char *_k = SvPV_nolen(key);
                croak("Given type for '%s' is not a subclass of Affix::Type::Base", _k);
            }
            size_t __sizeof = _sizeof(aTHX_ type);
            size_t __align = _alignof(aTHX_ type);
            size += packed ? 0 : padding_needed_for(size, __align > __sizeof ? __sizeof : __align);
            size += __sizeof;
            //~ DD(type);
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "offset", newSVuv(size - __sizeof));
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "align", newSVuv(__align));
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSVuv(__sizeof));
            //~ DD(type);
            av_push(field, SvREFCNT_inc(key));
            SV **value_ptr = av_fetch(fields_in, i + 1, 0);
            SV *value = *value_ptr;
            av_push(field, SvREFCNT_inc(value));
            SV *sv_field = (MUTABLE_SV(field));
            av_push(fields, newRV(sv_field));
        }

        hv_stores(RETVAL_HV, "fields", newRV(MUTABLE_SV(fields)));
        if (!packed && size > AFFIX_ALIGNBYTES * 2)
            size += padding_needed_for(size, AFFIX_ALIGNBYTES);
        hv_stores(RETVAL_HV, "sizeof", newSVuv(size));
        hv_stores(RETVAL_HV, "align", newSVuv(padding_needed_for(size, AFFIX_ALIGNBYTES)));
        ST(0) = sv_2mortal(
            sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::Struct", GV_ADD)));
    }
    else
        croak("Struct[...] expects an even size list of and field names and types. e.g. "
              "Struct[ "
              "epoch => Int, name => Str, ... ]");
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Union) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields_in = MUTABLE_AV(SvRV(ST(0)));
    size_t field_count = av_count(fields_in);
    if (!(field_count % 2)) {
        bool packed = false; // TODO: handle packed structs correctly
        hv_stores(RETVAL_HV, "packed", boolSV(packed));
        AV *fields = newAV();
        size_t field_count = av_count(fields_in), size = 0, _align = 0;
        for (size_t i = 0; i < field_count; i += 2) {
            AV *field = newAV();
            SV *key = newSVsv(*av_fetch(fields_in, i, 0));
            if (!SvPOK(key)) croak("Given name of '%s' is not a string", SvPV_nolen(key));
            SV *type = *av_fetch(fields_in, i + 1, 0);
            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                croak("Given type for '%s' is not a subclass of Affix::Type::Base",
                      SvPV_nolen(key));
            size_t __sizeof = _sizeof(aTHX_ type), __align = _alignof(aTHX_ type);
            if (__align > _align) _align = __align;
            if (size < __sizeof) size = __sizeof;
            if (!packed && field_count > 1 && __sizeof > AFFIX_ALIGNBYTES)
                size += padding_needed_for(__sizeof, AFFIX_ALIGNBYTES);
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "offset", newSVuv(0));
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "align", newSVuv(__align));
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSVuv(__sizeof));
            av_push(field, SvREFCNT_inc(key));
            SV **value_ptr = av_fetch(fields_in, i + 1, 0);
            SV *value = *value_ptr;
            av_push(field, SvREFCNT_inc(value));
            av_push(fields, newRV(MUTABLE_SV(field)));
        }
        hv_stores(RETVAL_HV, "align", newSVuv(_align));
        hv_stores(RETVAL_HV, "sizeof", newSVuv(size));
        hv_stores(RETVAL_HV, "fields", newRV(MUTABLE_SV(fields)));
    }
    else
        croak("Union[...] expects an even size list of and field names and types. e.g. Union[ "
              "epoch => Int, name => Str, ... ]");
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::Union", GV_ADD)));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Enum) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();

    {
        AV *vals = MUTABLE_AV(SvRV(ST(0)));
        AV *values = newAV_mortal();
        SV *current_value = newSViv(0);
        for (size_t i = 0; i < av_count(vals); ++i) {
            SV *name = newSV(0);
            SV **item = av_fetch(vals, i, 0);
            if (SvROK(*item)) {
                if (SvTYPE(SvRV(*item)) == SVt_PVAV) {
                    AV *cast = MUTABLE_AV(SvRV(*item));
                    if (av_count(cast) == 2) {
                        name = *av_fetch(cast, 0, 0);
                        current_value = *av_fetch(cast, 1, 0);
                        if (!SvIOK(current_value)) { // C-like enum math like: enum { a,
                            // b, c = a+b}
                            char *eval = NULL;
                            size_t pos = 0;
                            size_t size = 1024;
                            Newxz(eval, size, char);
                            for (size_t j = 0; j < av_count(values); j++) {
                                SV *e = *av_fetch(values, j, 0);
                                char *str = SvPV_nolen(e);
                                char *line;
                                if (SvIOK(e)) {
                                    int num = SvIV(e);
                                    line = form("sub %s(){%d}", str, num);
                                }
                                else {
                                    char *chr = SvPV_nolen(e);
                                    line = form("sub %s(){'%s'}", str, chr);
                                }
                                // size_t size = pos + strlen(line);
                                size = (strlen(eval) > (size + strlen(line))) ? size + strlen(line)
                                                                              : size;
                                Renewc(eval, size, char, char);
                                Copy(line, INT2PTR(DCpointer, PTR2IV(eval) + pos), strlen(line) + 1,
                                     char);
                                pos += strlen(line);
                            }
                            current_value = eval_pv(form("package Affix::Enum::eval{no warnings "
                                                         "qw'redefine reserved';%s%s}",
                                                         eval, SvPV_nolen(current_value)),
                                                    1);
                            safefree(eval);
                        }
                    }
                }
                else { croak("Enum element must be a [key => value] pair"); }
            }
            else
                sv_setsv(name, *item);
            {
                SV *TARGET = newSV(1);
                {
                    // Let's make enum values dualvars just 'cause; snagged from
                    // Scalar::Util
                    SV *num = newSVsv(current_value);
                    (void)SvUPGRADE(TARGET, SVt_PVNV);
                    sv_copypv(TARGET, name);
                    if (SvNOK(num) || SvPOK(num) || SvMAGICAL(num)) {
                        SvNV_set(TARGET, SvNV(num));
                        SvNOK_on(TARGET);
                    }
#ifdef SVf_IVisUV
                    else if (SvUOK(num)) {
                        SvUV_set(TARGET, SvUV(num));
                        SvIOK_on(TARGET);
                        SvIsUV_on(TARGET);
                    }
#endif
                    else {
                        SvIV_set(TARGET, SvIV(num));
                        SvIOK_on(TARGET);
                    }
                    if (PL_tainting && (SvTAINTED(num) || SvTAINTED(name))) SvTAINTED_on(TARGET);
                }
                av_push(values, newSVsv(TARGET));
            }
            sv_inc(current_value);
        }
        hv_stores(RETVAL_HV, "values", newRV_inc(MUTABLE_SV(values)));
    }

    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::Enum", GV_ADD)));
    XSRETURN(1);
}

// I might need to cram more context into these in a future version so I'm wrapping them this way
XS_INTERNAL(Affix_Type_IntEnum) {
    Affix_Type_Enum(aTHX_ cv);
}
XS_INTERNAL(Affix_Type_UIntEnum) {
    Affix_Type_Enum(aTHX_ cv);
}
XS_INTERNAL(Affix_Type_CharEnum) {
    Affix_Type_Enum(aTHX_ cv);
}

XS_INTERNAL(Types_return_typedef) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    dXSI32;
    PERL_UNUSED_VAR(ix);
    dXSTARG;
    PERL_UNUSED_VAR(targ);
    ST(0) = sv_2mortal(newSVsv(XSANY.any_sv));
    XSRETURN(1);
}

XS_INTERNAL(Affix_typedef) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "name, type");
    const char *name = (const char *)SvPV_nolen(ST(0));
    SV *type = ST(1);
    {
        CV *cv = newXSproto_portable(name, Types_return_typedef, __FILE__, "");
        XSANY.any_sv = SvREFCNT_inc(newSVsv(type));
    }
    if (sv_derived_from(type, "Affix::Type::Base")) {
        if (sv_derived_from(type, "Affix::Type::Enum")) {
            HV *href = MUTABLE_HV(SvRV(type));
            SV **values_ref = hv_fetch(href, "values", 6, 0);
            AV *values = MUTABLE_AV(SvRV(*values_ref));
            //~ HV *_stash = gv_stashpv(name, TRUE);
            for (size_t i = 0; i < av_count(values); ++i) {
                SV **value = av_fetch(MUTABLE_AV(values), i, 0);
                register_constant(name, SvPV_nolen(*value), *value);
            }
        }
        else if (sv_derived_from(type, "Affix::Type::Struct")) {
            HV *href = MUTABLE_HV(SvRV(type));
            hv_stores(href, "typedef", newSVpv(name, 0));
        }
    }
    else { croak("Expected a subclass of Affix::Type::Base"); }
    sv_setsv_mg(ST(1), type);
    SvSETMAGIC(ST(1));
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_load_lib) {
    dVAR;
    dXSARGS;
    if (items < 1 || items > 2) croak_xs_usage(cv, "lib_name, version");
    char *_libpath = locate_lib(aTHX_ ST(0), SvIOK(ST(1)) ? ST(1) : newSV(0));
    DLLib *lib =
#if defined(DC__OS_Win64) || defined(DC__OS_MacOSX)
        dlLoadLibrary(_libpath);
#else
        (DLLib *)dlopen(_libpath, RTLD_NOW);
#endif
    if (!lib) croak("Failed to load %s", dlerror());
    SV *RETVAL = sv_newmortal();
    sv_setref_pv(RETVAL, "Affix::Lib", lib);
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Lib_list_symbols) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "lib");

    AV *RETVAL;
    DLLib *lib;

    if (sv_derived_from(ST(0), "Affix::Lib")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        croak("lib is not of type Affix::Lib");

    RETVAL = newAV();
    char *name;
    Newxz(name, 1024, char);
    int len = dlGetLibraryPath(lib, name, 1024);
    if (len == 0) croak("Failed to get library name");
    DLSyms *syms = dlSymsInit(name);
    int count = dlSymsCount(syms);
    for (int i = 0; i < count; ++i) {
        av_push(RETVAL, newSVpv(dlSymsName(syms, i), 0));
    }
    dlSymsCleanup(syms);
    safefree(name);

    {
        SV *RETVALSV;
        RETVALSV = newRV_noinc((SV *)RETVAL);
        RETVALSV = sv_2mortal(RETVALSV);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_Lib_free) {
    dVAR;
    dXSARGS;
    warn("FREE LIB");
    if (items != 1) croak_xs_usage(cv, "lib");
    DLLib *lib;
    if (sv_derived_from(ST(0), "Affix::Lib")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        croak("lib is not of type Affix::Lib");
    if (lib != NULL) dlFreeLibrary(lib);
    lib = NULL;
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Lib_path) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "lib");

    SV *RETVAL;
    DLLib *lib;

    if (sv_derived_from(ST(0), "Affix::Lib")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        croak("lib is not of type Affix::Lib");

    char *name;
    Newxz(name, 1024, char);
    int len = dlGetLibraryPath(lib, name, 1024);
    PING;
    if (len == 0) croak("Failed to get library name");
    RETVAL = newSVpv(name, len - 1);
    safefree(name);
    {
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }

    XSRETURN(1);
}

extern "C" void Affix_trigger(pTHX_ CV *cv) {
    dXSARGS;
    dMY_CXT;

    Affix *ptr = (Affix *)XSANY.any_ptr;
    char **free_strs = NULL;
    void **free_ptrs = NULL;

    int num_args = ptr->num_args;
    int num_strs = 0, num_ptrs = 0;
    int16_t *arg_types = ptr->arg_types;
    bool void_ret = false;
    DCaggr *agg_ = NULL;

    dcReset(MY_CXT.cvm);

    switch (ptr->ret_type) {
    case AFFIX_ARG_CUNION:
    case AFFIX_ARG_CSTRUCT: {
        PING;
        agg_ = _aggregate(aTHX_ ptr->ret_info);
        dcBeginCallAggr(MY_CXT.cvm, agg_);
    } break;
    }

    if (UNLIKELY(items != num_args)) {
        if (UNLIKELY(items > num_args))
            croak("Too many arguments; wanted %d, found %d", num_args, items);
        croak("Not enough arguments; wanted %d, found %d", num_args, items);
    }
    for (int arg_pos = 0, info_pos = 0; LIKELY(arg_pos < num_args); ++arg_pos, ++info_pos) {
#ifdef DEBUG
        warn("arg_pos: %d, num_args: %d, info_pos: %d", arg_pos, num_args, info_pos);
        warn("   type: %d, as_str: %s", arg_types[info_pos], type_as_str(arg_types[info_pos]));
        warn(" masked: %d", arg_types[info_pos] & AFFIX_ARG_TYPE_MASK);
        //~ sv_dump(ST(arg_pos));
#endif
        switch (arg_types[info_pos]) {
        case DC_SIGCHAR_CC_PREFIX: {
            // TODO: Not a fan of this entire section
            SV *cc = *av_fetch(ptr->arg_info, info_pos, 0);
            char mode = DC_SIGCHAR_CC_DEFAULT;
            if (sv_derived_from(cc, "Affix::Type::CC::DEFALT")) { mode = DC_SIGCHAR_CC_DEFAULT; }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::THISCALL"))) {
                mode = DC_SIGCHAR_CC_THISCALL;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ELLIPSIS"))) {
                mode = DC_SIGCHAR_CC_ELLIPSIS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ELLIPSIS_VARARGS"))) {
                mode = DC_SIGCHAR_CC_ELLIPSIS_VARARGS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::CDECL"))) {
                mode = DC_SIGCHAR_CC_CDECL;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::STDCALL"))) {
                mode = DC_SIGCHAR_CC_STDCALL;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::FASTCALL_MS"))) {
                mode = DC_SIGCHAR_CC_FASTCALL_MS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::FASTCALL_GNU"))) {
                mode = DC_SIGCHAR_CC_FASTCALL_GNU;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::THISCALL_MS"))) {
                mode = DC_SIGCHAR_CC_THISCALL_MS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::THISCALL_GNU"))) {
                mode = DC_SIGCHAR_CC_THISCALL_GNU;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ARM_ARM "))) {
                mode = DC_SIGCHAR_CC_ARM_ARM;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ARM_THUMB"))) {
                mode = DC_SIGCHAR_CC_ARM_THUMB;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::SYSCALL"))) {
                mode = DC_SIGCHAR_CC_SYSCALL;
            }
            else { croak("Unknown calling convention"); }
            dcMode(MY_CXT.cvm, dcGetModeFromCCSigChar(mode));
            if (mode != DC_SIGCHAR_CC_ELLIPSIS && mode != DC_SIGCHAR_CC_ELLIPSIS_VARARGS) {
                dcReset(MY_CXT.cvm);
            }
            arg_pos--;
        } break;
        case AFFIX_ARG_VOID: // skip?
            break;
        case AFFIX_ARG_BOOL:
            dcArgBool(MY_CXT.cvm, SvTRUE(ST(arg_pos))); // Anything can be a bool
            break;
        case AFFIX_ARG_CHAR:
            dcArgChar(MY_CXT.cvm,
                      (char)(SvIOK(ST(arg_pos)) ? SvIV(ST(arg_pos)) : *SvPV_nolen(ST(arg_pos))));
            break;
        case AFFIX_ARG_UCHAR:
            dcArgChar(MY_CXT.cvm,
                      (U8)(SvIOK(ST(arg_pos)) ? SvUV(ST(arg_pos)) : *SvPV_nolen(ST(arg_pos))));
            break;
        case AFFIX_ARG_WCHAR: {
            if (SvOK(ST(arg_pos))) {
                char *eh = SvPV_nolen(ST(arg_pos));
                PUTBACK;
                const char *pat = "W";
                SSize_t s = unpackstring(pat, pat + 1, eh, eh + WCHAR_T_SIZE + 1, SVt_PVAV);
                SPAGAIN;
                if (UNLIKELY(s != 1)) croak("Failed to unpack wchar_t");
                switch (WCHAR_T_SIZE) {
                case I8SIZE:
                    dcArgChar(MY_CXT.cvm, (char)POPi);
                    break;
                case SHORTSIZE:
                    dcArgShort(MY_CXT.cvm, (short)POPi);
                    break;
                case INTSIZE:
                    dcArgInt(MY_CXT.cvm, (int)POPi);
                    break;
                default:
                    croak("Invalid wchar_t size for argument!");
                }
            }
            else
                dcArgInt(MY_CXT.cvm, 0);
        } break;
        case AFFIX_ARG_SHORT:
            dcArgShort(MY_CXT.cvm, (short)(SvIV(ST(arg_pos))));
            break;
        case AFFIX_ARG_USHORT:
            dcArgShort(MY_CXT.cvm, (unsigned short)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_ARG_INT:
            dcArgInt(MY_CXT.cvm, (int)(SvIV(ST(arg_pos))));
            break;
        case AFFIX_ARG_UINT:
            dcArgInt(MY_CXT.cvm, (unsigned int)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_ARG_LONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_ARG_ULONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_ARG_LONGLONG:
            dcArgLongLong(MY_CXT.cvm, (I64)(SvIV(ST(arg_pos))));
            break;
        case AFFIX_ARG_ULONGLONG:
            dcArgLongLong(MY_CXT.cvm, (U64)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_ARG_FLOAT:
            dcArgFloat(MY_CXT.cvm, (float)SvNV(ST(arg_pos)));
            break;
        case AFFIX_ARG_DOUBLE:
            dcArgDouble(MY_CXT.cvm, (double)SvNV(ST(arg_pos)));
            break;
        case AFFIX_ARG_ASCIISTR:
            dcArgPointer(MY_CXT.cvm, SvOK(ST(arg_pos)) ? SvPV_nolen(ST(arg_pos)) : NULL);
            break;
        case AFFIX_ARG_UTF8STR:
        case AFFIX_ARG_UTF16STR: {
            if (SvOK(ST(arg_pos))) {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                Newxz(free_ptrs[num_ptrs], _sizeof(aTHX_ newSViv(AFFIX_ARG_UTF16STR)), char);
                sv2ptr(aTHX_ newSViv(AFFIX_ARG_UTF16STR), ST(arg_pos), free_ptrs[num_ptrs], false);
                dcArgPointer(MY_CXT.cvm, *(DCpointer *)(free_ptrs[num_ptrs++]));
            }
            else { dcArgPointer(MY_CXT.cvm, NULL); }
        } break;
        case AFFIX_ARG_CSTRUCT: {
            if (!SvOK(ST(arg_pos)) && SvREADONLY(ST(arg_pos)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                if (!SvROK(ST(arg_pos)) || SvTYPE(SvRV(ST(arg_pos))) != SVt_PVHV)
                    croak("Type of arg %d must be an hash ref", arg_pos + 1);
                //~ AV *elements = MUTABLE_AV(SvRV(ST(i)));
                SV **type = av_fetch(ptr->arg_info, info_pos, 0);
                size_t size = _sizeof(aTHX_ * type);
                Newxz(free_ptrs[num_ptrs], size, char);
                DCaggr *agg = _aggregate(aTHX_ * type);
                sv2ptr(aTHX_ * type, ST(arg_pos), free_ptrs[num_ptrs], false);
                dcArgAggr(MY_CXT.cvm, agg, free_ptrs[num_ptrs++]);
            }
        }
        //~ croak("Unhandled arg type at %s line %d", __FILE__, __LINE__);
        break;
        case AFFIX_ARG_CARRAY: {
            if (!SvOK(ST(arg_pos)) && SvREADONLY(ST(arg_pos)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);

                // free_ptrs = (void **)safemalloc(num_args * sizeof(DCpointer));
                if (!SvROK(ST(arg_pos)) || SvTYPE(SvRV(ST(arg_pos))) != SVt_PVAV)
                    croak("Type of arg %d must be an array ref", arg_pos + 1);

                AV *elements = MUTABLE_AV(SvRV(ST(arg_pos)));
                SV **type = av_fetch(ptr->arg_info, info_pos, 0);
                HV *hv_ptr = MUTABLE_HV(SvRV(*type));
                size_t av_len;
                if (hv_exists(hv_ptr, "size", 4)) {
                    av_len = SvIV(*hv_fetchs(hv_ptr, "size", 0));
                    if (av_count(elements) != av_len)
                        croak("Expected an array of %lu elements; found %ld", av_len,
                              av_count(elements));
                }
                else {
                    av_len = av_count(elements);
                    hv_stores(hv_ptr, "dyn_size", newSVuv(av_len));
                }
                //~ hv_stores(hv_ptr, "sizeof", newSViv(av_len));
                size_t size = _sizeof(aTHX_ * type);
                Newxz(free_ptrs[num_ptrs], size, char);
                PING;
                sv2ptr(aTHX_ * type, ST(arg_pos), free_ptrs[num_ptrs], false);
                PING;
                dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs++]);
                PING;
            }
        } break;
        case AFFIX_ARG_CALLBACK: {
            if (SvOK(ST(arg_pos))) {
                //~ DCCallback *hold;
                //~ //Newxz(hold, 1, DCCallback);
                //~ sv2ptr(aTHX_ SvRV(*av_fetch(ptr->arg_info, info_pos, 0)), ST(arg_pos), hold,
                // false); ~ dcArgPointer(MY_CXT.cvm, hold);
                CallbackWrapper *hold;
                Newx(hold, 1, CallbackWrapper);
                sv2ptr(aTHX_ * av_fetch(ptr->arg_info, info_pos, 0), ST(arg_pos), hold, false);
                dcArgPointer(MY_CXT.cvm, hold->cb);
            }
            else
                dcArgPointer(MY_CXT.cvm, NULL);
        } break;
        case AFFIX_ARG_SV: {
            SV *type = *av_fetch(ptr->arg_info, info_pos, 0);
            DCpointer blah;
            Newxz(blah, 1, DCpointer);
            sv2ptr(aTHX_ type, ST(arg_pos), blah, false);
            dcArgPointer(MY_CXT.cvm, blah);
        } break;
        case AFFIX_ARG_CPOINTER: {
#ifdef DEBUG
            warn("AFFIX_ARG_CPOINTER [%d, %ld/%s]", arg_pos,
                 SvIV(*av_fetch(ptr->arg_info, info_pos, 0)),
                 type_as_str(SvIV(*av_fetch(ptr->arg_info, info_pos, 0))));
            DD(MUTABLE_SV(ptr->arg_info));
            DD(*av_fetch(ptr->arg_info, info_pos, 0));
#endif
            if (UNLIKELY(!SvOK(ST(arg_pos)) && SvREADONLY(ST(arg_pos)))) { // explicit undef
                PING;
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                SV *type_class = *av_fetch(ptr->arg_info, info_pos, 0);
                SV *type = *hv_fetch(MUTABLE_HV(SvRV(type_class)), "type", 4, 0);
                if (
                    UNLIKELY(sv_derived_from(type_class, "Affix::Type::InstanceOf"))) {
                    SV *cls = *hv_fetch(MUTABLE_HV(SvRV(type_class)), "class", 5, 0);
                    if (LIKELY(sv_isobject(ST(arg_pos))) && !(sv_derived_from(ST(arg_pos), SvPV_nolen(cls)))) {
                        croak("Expected variable of type %s", SvPV_nolen(cls));
                    }
                }
                if (LIKELY(sv_isobject(ST(arg_pos)) &&
                                sv_derived_from(ST(arg_pos), "Affix::Pointer"))) {
                    IV tmp = SvIV(SvRV(ST(arg_pos)));
                    dcArgPointer(MY_CXT.cvm, INT2PTR(DCpointer, tmp));
                }
                else {
                    if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                    if (type == NULL) croak("No idea");
                    switch (SvIV(type)) {
                    case AFFIX_ARG_VOID: {
                        if (sv_isobject(ST(arg_pos))) {
                            croak("Unexpected pointer to blessed object");
                        }
                        else {
                            free_ptrs[num_ptrs] = safemalloc(INTPTR_T_SIZE);
                            sv2ptr(aTHX_ type, ST(arg_pos), free_ptrs[num_ptrs], false);
                            dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                            num_ptrs++;
                        }
                    } break;
                    case AFFIX_ARG_CUNION: {
                        DCpointer p;
                        const MAGIC *mg = SvTIED_mg((SV *)SvRV(ST(arg_pos)), PERL_MAGIC_tied);
                        if (!UNLIKELY(
                                //~ SvRMAGICAL(sv) &&
                                LIKELY(
                                    SvOK(ST(arg_pos)) && SvTYPE(SvRV(ST(arg_pos))) == SVt_PVHV && mg
                                    //~ &&  sv_derived_from(SvRV(ST(arg_pos)), "Affix::Union")
                                    ))) { // Already a known union pointer
                            p = safemalloc(_sizeof(aTHX_ type));
                            sv_setsv_mg(ST(arg_pos),
                                        ptr2sv(aTHX_ p, *av_fetch(ptr->arg_info, info_pos, 0)));
                        }
                        else {
                            mg = SvTIED_mg((SV *)SvRV(ST(arg_pos)), PERL_MAGIC_tied);
                            SV *ref = SvTIED_obj(ST(arg_pos), mg);
                            {
                                HV *h = MUTABLE_HV(SvRV(ref));
                                SV **ptr_ptr = hv_fetchs(h, "pointer", 0);
                                {
                                    IV tmp = SvIV(SvRV(*ptr_ptr));
                                    p = INT2PTR(DCpointer, tmp);
                                }
                            }
                        }
                        dcArgPointer(MY_CXT.cvm, p);
                    } break;
                    default: {
                        Newxz(free_ptrs[num_ptrs], _sizeof(aTHX_ type), char);
                        if (SvOK(ST(arg_pos))) {
                            sv2ptr(aTHX_ type, ST(arg_pos), free_ptrs[num_ptrs], false);
                        }
                        dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                        num_ptrs++;
                    }
                    }
                }
#if 0
					if (UNLIKELY(SvREADONLY(ST(arg_pos)))) { // explicit undef
						if (LIKELY(SvOK(ST(arg_pos)))) {

						} else
							free_ptrs[num_ptrs] = NULL;
						dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
					} else if (SvOK(ST(arg_pos))) {
						warn("Alpha");
						if (subtype_ptr == NULL
								/*sv_derived_from(ST(arg_pos), "Affix::Type::Void")*/
						   ) {
							warn("Send void pointer...");
							IV tmp = SvIV((SV *)SvRV(ST(arg_pos)));
							free_ptrs[num_ptrs] = INT2PTR(DCpointer, tmp);
						} else {
							warn("Sending... something else?");
							if (sv_isobject(ST(arg_pos))) croak("Unexpected pointer to blessed object");
							free_ptrs[num_ptrs] = safemalloc(_sizeof(aTHX_ * subtype_ptr));
							sv2ptr(aTHX_ * subtype_ptr, ST(arg_pos), free_ptrs[num_ptrs], false);
						}
					} else { // treat as if it's an lvalue
						warn("...idk?");
						if (subtype_ptr == NULL
								/*sv_derived_from(ST(arg_pos), "Affix::Type::Void")*/
						   ) {
							size_t size = _sizeof(aTHX_ * subtype_ptr);
							warn("...dsdsa?");
							Newxz(free_ptrs[num_ptrs], size, char);
							warn("...dsdsa?");
						}
						dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
						num_ptrs++;
					}
#endif
            }
        } break;
        case AFFIX_ARG_CUNION:
        default:
            croak("Unhandled arg type %s (%d) at %s line %d", type_as_str(arg_types[info_pos]),
                  (arg_types[info_pos]), __FILE__, __LINE__);
            break;
        }
    }
#ifdef DEBUG
    warn("Return type %d (%s) / %p at %s line %d", ptr->ret_type, type_as_str(ptr->ret_type),
         ptr->entry_point, __FILE__, __LINE__);
#endif
    SV *RETVAL;
    switch (ptr->ret_type) {
    case AFFIX_ARG_VOID:
        void_ret = true;
        dcCallVoid(MY_CXT.cvm, ptr->entry_point);
        break;
    case AFFIX_ARG_BOOL:
        RETVAL = boolSV(dcCallBool(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_CHAR:
        // TODO: Make dualvar
        RETVAL = newSViv((char)dcCallChar(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_UCHAR:
        // TODO: Make dualvar
        RETVAL = newSVuv((U8)dcCallChar(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_WCHAR: {
        SV *container;
        RETVAL = newSVpvs("");
        const char *pat = "W";
        switch (WCHAR_T_SIZE) {
        case I8SIZE:
            container = newSViv((char)dcCallChar(MY_CXT.cvm, ptr->entry_point));
            break;
        case SHORTSIZE:
            container = newSViv((short)dcCallShort(MY_CXT.cvm, ptr->entry_point));
            break;
        case INTSIZE:
            container = newSViv((int)dcCallInt(MY_CXT.cvm, ptr->entry_point));
            break;
        default:
            croak("Invalid wchar_t size for argument!");
        }
        sv_2mortal(container);
        packlist(RETVAL, pat, pat + 1, &container, &container + 1);
    } break;
    case AFFIX_ARG_SHORT:
        RETVAL = newSViv((short)dcCallShort(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_USHORT:
        RETVAL = newSVuv((unsigned short)dcCallShort(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_INT:
        RETVAL = newSViv((signed int)dcCallInt(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_UINT:
        RETVAL = newSVuv((unsigned int)dcCallInt(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_LONG:
        RETVAL = newSViv((long)dcCallLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_ULONG:
        RETVAL = newSVuv((unsigned long)dcCallLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_LONGLONG:
        RETVAL = newSViv((I64)dcCallLongLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_ULONGLONG:
        RETVAL = newSVuv((U64)dcCallLongLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_FLOAT:
        RETVAL = newSVnv(dcCallFloat(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_DOUBLE:
        RETVAL = newSVnv(dcCallDouble(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_ASCIISTR:
        RETVAL = newSVpv((char *)dcCallPointer(MY_CXT.cvm, ptr->entry_point), 0);
        break;
    case AFFIX_ARG_UTF16STR: {
        wchar_t *str = (wchar_t *)dcCallPointer(MY_CXT.cvm, ptr->entry_point);
        RETVAL = wchar2utf(aTHX_ str, wcslen(str));
    } break;
    case AFFIX_ARG_CPOINTER: {
        SV *subtype = *hv_fetchs(MUTABLE_HV(SvRV(ptr->ret_info)), "type", 0);
        DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
        if (sv_derived_from(subtype, "Affix::Type::Void")) {
            RETVAL = ptr2sv(aTHX_ p, ptr->ret_info);
        }
        //~
        //~ DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
        //~ if(UNLIKELY(sv_derived_from(ptr->ret_info, "Affix::Type::InstanceOf"))){
        //~ SV *cls = *hv_fetchs(MUTABLE_HV(SvRV(ptr->ret_info)), "class", 0);
        //~ sv_setref_pv(RETVAL, SvPV_nolen(cls), p);
        //~ }
        else {
            RETVAL = newSV(1);
            sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", p);
        }
        //~ }
        //~ else {

        //~ }
        PING;

    } break;
    case AFFIX_ARG_CUNION:
    case AFFIX_ARG_CSTRUCT: {
        //~ warn ("        _sizeof(aTHX_ ptr->ret_info): %d",_sizeof(aTHX_ ptr->ret_info));
        DCpointer p = safemalloc(_sizeof(aTHX_ ptr->ret_info));
        dcCallAggr(MY_CXT.cvm, ptr->entry_point, agg_, p);
        RETVAL = ptr2sv(aTHX_ p, ptr->ret_info);
    } break;
    default:
        //~ sv_dump(ptr->ret_info);
        DD(ptr->ret_info);
        croak("Unknown return type: %s (%d)", type_as_str(ptr->ret_type), ptr->ret_type);
        break;
    }
    /*
    #define AFFIX_ARG_UTF8STR 18
    #define AFFIX_ARG_UTF16STR 20
    #define AFFIX_ARG_CSTRUCT 22
    #define AFFIX_ARG_CARRAY 24
    #define AFFIX_ARG_CALLBACK 26
    #define AFFIX_ARG_CPOINTER 28
    #define AFFIX_ARG_VMARRAY 30
    #define AFFIX_ARG_CUNION 42
    #define AFFIX_ARG_CPPSTRUCT 44
    #define AFFIX_ARG_WCHAR 46
    */
    //
    PING;
    //
    /* Free any memory that we need to. */
    if (free_strs != NULL) {
        for (int i = 0; UNLIKELY(i < num_strs); ++i)
            safefree(free_strs[i]);
        safefree(free_strs);
    }
    {
        for (int i = 0, p = 0; LIKELY(i < items); ++i) {
            PING;
            if (LIKELY(!SvREADONLY(ST(i)))) { // explicit undef
                switch (arg_types[i]) {
                case AFFIX_ARG_CARRAY: {
                    SV *sv = ptr2sv(aTHX_ free_ptrs[p++], *av_fetch(ptr->arg_info, i, 0));
                    if (SvFLAGS(ST(i)) & SVs_TEMP) { // likely a temp ref
                        AV *av = MUTABLE_AV(SvRV(sv));
                        av_clear(MUTABLE_AV(SvRV(ST(i))));
                        size_t av_len = av_count(av);
                        for (size_t q = 0; q < av_len; ++q) {
                            sv_setsv(*av_fetch(MUTABLE_AV(SvRV(ST(i))), q, 1), *av_fetch(av, q, 0));
                        }
                        SvSETMAGIC(SvRV(ST(i)));
                    }
                    else { // scalar ref is faster :D
                        SvSetMagicSV(ST(i), sv);
                    }

                } break;
                case AFFIX_ARG_CPOINTER: {
                    if (sv_derived_from((ST(i)), "Affix::Pointer")) {
                        ;
                        //~ warn("raw pointer");
                    }
                    else if (!SvREADONLY(ST(i))) {
                        //~ sv_dump((ST(i)));
                        //~ sv_dump(SvRV(ST(i)));
                        if (SvOK(ST(i))) {
                            const MAGIC *mg = SvTIED_mg((SV *)SvRV(ST(i)), PERL_MAGIC_tied);
                            if (LIKELY(SvOK(ST(i)) && SvTYPE(SvRV(ST(i))) == SVt_PVHV && mg
                                       //~ &&  sv_derived_from(SvRV(ST(i)), "Affix::Union")
                                       )) { // Already a known union pointer
                            }
                            else {
                                sv_setsv_mg(ST(i), ptr2sv(aTHX_ free_ptrs[p++],
                                                          *av_fetch(ptr->arg_info, i, 0)));
                            }
                        }
                        else {
                            sv_setsv_mg(ST(i), ptr2sv(aTHX_ free_ptrs[p++],
                                                      *av_fetch(ptr->arg_info, i, 0)));
                        }
                    }
                } break;
                }
            }
        }
    }
    PING;

    if (UNLIKELY(free_ptrs != NULL)) {
        for (int i = 0; LIKELY(i < num_ptrs); ++i) {
            safefree(free_ptrs[i]);
        }
        safefree(free_ptrs);
    }
    //~ if(agg_) dcFreeAggr(agg_);

    if (UNLIKELY(void_ret)) XSRETURN_EMPTY;
    PING;
    ST(0) = sv_2mortal(RETVAL);

    XSRETURN(1);
}

XS_INTERNAL(Affix_affix) {
    dXSARGS;
    dXSI32;
    if (items != 4) croak_xs_usage(cv, "$lib, $symbol, @arg_types, $ret_type");
    SV *RETVAL;
    Affix *ret;
    Newx(ret, 1, Affix);

    // Dumb defaults
    ret->num_args = 0;
    ret->arg_info = newAV();

    char *prototype = NULL;
    char *perl_name = NULL;
    {
        SV *lib, *ver;
        SV *LIBSV;

        // affix($lib, ..., ..., ...)
        // affix([$lib, 1.2.0], ..., ..., ...)
        // wrap($lib, ..., ...., ...
        // wrap([$lib, 'v3'], ..., ...., ...
        {
            if (UNLIKELY(SvROK(ST(0)) && SvTYPE(SvRV(ST(0))) == SVt_PVAV)) {
                AV *tmp = MUTABLE_AV(SvRV(ST(0)));
                size_t tmp_len = av_count(tmp);
                // Non-fatal
                if (UNLIKELY(!(tmp_len == 1 || tmp_len == 2))) {
                    warn("Expected a lib and version");
                }
                lib = *av_fetch(tmp, 0, false);
                ver = *av_fetch(tmp, 1, false);
            }
            else {
                lib = newSVsv(ST(0));
                ver = newSV(0);
            }
            if (sv_isobject(lib) && sv_derived_from(lib, "Affix::Lib")) {
                LIBSV = lib;
                {
                    IV tmp = SvIV(SvRV(lib));
                    ret->lib_handle = INT2PTR(DLLib *, tmp);
                }
                {
                    char *name;
                    Newxz(name, 1024, char);
                    int len = dlGetLibraryPath(ret->lib_handle, name, 1024);
                    Newxz(ret->lib_name, len + 1, char);
                    Copy(name, ret->lib_name, len, char);
                }
            }
            else {
                ret->lib_name = locate_lib(aTHX_ lib, ver);
                ret->lib_handle =
#if defined(DC__OS_Win64) || defined(DC__OS_MacOSX)
                    dlLoadLibrary(ret->lib_name);
#else
                    (DLLib *)dlopen(ret->lib_name, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
                if (!ret->lib_handle) {
                    croak("Failed to load lib %s", dlerror());
                }
                LIBSV = sv_newmortal();
                sv_setref_pv(LIBSV, "Affix::Lib", (DCpointer)ret->lib_handle);
            }
        }

        //~ sv_dump(lib);
        //~ sv_dump(ver);

        // affix(..., ..., [Int], ...)
        // wrap(..., ..., [], ...)
        {
            STMT_START {
                SV *const xsub_tmp_sv = ST(2);
                SvGETMAGIC(xsub_tmp_sv);
                PING;
                if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                    PING;
                    AV *tmp_args = (AV *)SvRV(xsub_tmp_sv);
                    size_t args_len = av_count(tmp_args);
                    SV **tmp_arg;
                    Newxz(ret->arg_types, args_len, int16_t); // TODO: safefree
                    Newxz(prototype, args_len, char);
                    for (size_t i = 0; i < args_len; ++i) {
                        tmp_arg = av_fetch(tmp_args, i, false);
                        if (LIKELY(SvROK(*tmp_arg) &&
                                   sv_derived_from(*tmp_arg, "Affix::Type::Base"))) {
                            ret->arg_types[i] = (int16_t)SvIV(*tmp_arg);
                            if (UNLIKELY(sv_derived_from(*tmp_arg, "Affix::Type::CC"))) {
                                av_store(ret->arg_info, i, newSVsv(*tmp_arg));
                                //~ warn("av_store(ret->arg_info, %d, *tmp_arg);", i);
                                if (UNLIKELY(
                                        sv_derived_from(*tmp_arg, "Affix::Type::CC::ELLIPSIS")) ||
                                    UNLIKELY(sv_derived_from(
                                        *tmp_arg, "Affix::Type::CC::ELLIPSIS_VARARGS"))) {
                                    prototype[i] = ';';
                                }
                            }
                            else {
                                ++ret->num_args;
                                prototype[i] = '$';
                                //~ switch (ret->arg_types[i]) {
                                //~ case AFFIX_ARG_CPOINTER:
                                //~ {
                                //~ SV *sv = *hv_fetchs(MUTABLE_HV(SvRV(*tmp_arg)), "type", 0);
                                //~ av_store(ret->arg_info, i, sv);
                                //~ break;
                                //~ }
                                //~ case AFFIX_ARG_CARRAY:
                                //~ case AFFIX_ARG_VMARRAY:
                                //~ case AFFIX_ARG_CSTRUCT:
                                //~ case AFFIX_ARG_CALLBACK:
                                //~ case AFFIX_ARG_CUNION:
                                //~ case AFFIX_ARG_CPPSTRUCT: {
                                //~ //av_store(ret->arg_info, i, *tmp_arg);
                                //~ } break;
                                //~ }
                            }
                            //~ warn("av_store(ret->arg_info, %d, *tmp_arg);", i);
                            av_store(ret->arg_info, i, newSVsv(*tmp_arg));
                        }
                        else { croak("Unexpected arg type in slot %ld", i + 1); }
                    }
                }
                else { croak("Expected a list of argument types as an array ref"); }
            }
            STMT_END;
        }

        // affix(..., $symbol, ..., ...)
        // affix(..., [$symbol, $name], ..., ...)
        // wrap(..., $symbol, ..., ...)
        {
            SV *symbol;
            if (UNLIKELY(SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV)) {
                AV *tmp = MUTABLE_AV(SvRV(ST(1)));
                size_t tmp_len = av_count(tmp);
                if (tmp_len != 2) { croak("Expected a symbol and name"); }
                if (ix == 1 && tmp_len > 1) {
                    croak("wrap( ... ) isn't expecting a name and has ignored it");
                }
                symbol = *av_fetch(tmp, 0, false);
                if (!SvPOK(symbol)) { croak("Undefined symbol name"); }
                perl_name = SvPV_nolen(*av_fetch(tmp, 1, false));
            }
            else if (UNLIKELY(!SvPOK(ST(1)))) { croak("Undefined symbol name"); }
            else {
                symbol = ST(1);
                perl_name = SvPV_nolen(symbol);
            }

            {
                char *sym_name = SvPV_nolen(symbol);
                PING;
                ret->entry_point = dlFindSymbol(ret->lib_handle, sym_name);
                if (ret->entry_point == NULL) {
                    PING;
                    ret->entry_point = dlFindSymbol(
                        ret->lib_handle, _mangle(aTHX_ "Itanium", LIBSV, sym_name, ST(2)));
                }
                if (ret->entry_point == NULL) {
                    ret->entry_point = dlFindSymbol(
                        ret->lib_handle, _mangle(aTHX_ "Rust_legacy", LIBSV, sym_name, ST(2)));
                }
                // TODO: D and Swift
                if (ret->entry_point == NULL) { croak("Failed to find symbol named %s", sym_name); }
            }
        }

        // affix(..., ..., ..., $ret)
        // wrap(..., ..., ..., $ret)
        if (LIKELY(SvROK(ST(3)) && sv_derived_from(ST(3), "Affix::Type::Base"))) {
            ret->ret_info = newSVsv(ST(3));
            ret->ret_type = SvIV(ST(3));
        }
        else { croak("Unknown return type"); }
    }

#ifdef DEBUG
    warn("lib: %p, entry_point: %p, as: %s, prototype: %s, ix: %d ", (DCpointer)ret->lib_handle,
         ret->entry_point, perl_name, prototype, ix);
    DD(MUTABLE_SV(ret->arg_info));
    DD(ret->ret_info);
#endif
    /*
    struct Affix {
    int16_t call_conv;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    void *entry_point;
    AV *arg_info;
    SV *ret_info;
    SV *resolve_lib_name;
    };
    */
    STMT_START {
        cv = newXSproto_portable(ix == 0 ? perl_name : NULL, Affix_trigger, __FILE__, prototype);
        if (UNLIKELY(cv == NULL))
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (DCpointer)ret;
    }
    STMT_END;
    RETVAL = sv_bless((UNLIKELY(ix == 1) ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                      gv_stashpv("Affix", GV_ADD));

    ST(0) = sv_2mortal(RETVAL);
    if (prototype) safefree(prototype);
    PING;
    XSRETURN(1);
}

XS_INTERNAL(Affix_DESTROY) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    Affix *ptr;
    CV *THIS;
    STMT_START {
        HV *st;
        GV *gvp;
        SV *const xsub_tmp_sv = ST(0);
        SvGETMAGIC(xsub_tmp_sv);
        THIS = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
        {
            CV *cv = THIS;
            ptr = (Affix *)XSANY.any_ptr;
        }
    }
    STMT_END;
    /*
    struct Affix {
    int16_t call_conv;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    void *entry_point;
    AV *arg_info;
    SV *ret_info;
    SV *resolve_lib_name;
    };
            */
    //~ if (ptr->arg_types != NULL) {
    //~ safefree(ptr->arg_types);
    //~ ptr->arg_types = NULL;
    //~ }
    if (ptr->lib_handle != NULL) {
        dlFreeLibrary(ptr->lib_handle);
        ptr->lib_handle = NULL;
    }
    //~ if(ptr->lib_name!=NULL){
    //~ Safefree(ptr->lib_name);
    //~ ptr->lib_name = NULL;
    //~ }

    //~ if(ptr->entry_point)
    //~ Safefree(ptr->entry_point);
    if (ptr) { Safefree(ptr); }

    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_END) { // cleanup
    dXSARGS;
    PERL_UNUSED_VAR(items);
    dMY_CXT;
    if (MY_CXT.cvm) dcFree(MY_CXT.cvm);
    XSRETURN_EMPTY;
}

// Utilities
XS_INTERNAL(Affix_sv_dump) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "sv");
    SV *sv = ST(0);
    sv_dump(sv);
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Aggregate_FETCH) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    SV *RETVAL = newSV(0);
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*hv_fetchs(h, "type", 0)))), "fields", 0);
    SV **ptr_ptr = hv_fetchs(h, "pointer", 0);
    char *key = SvPV_nolen(ST(1));
    AV *types = MUTABLE_AV(SvRV(*type_ptr));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(key, SvPV(*name, PL_na)) == 0) {
            SV *_type = *av_fetch(MUTABLE_AV(SvRV(*elm)), 1, 0);
            size_t offset = _offsetof(aTHX_ _type); // meaningless for union
            DCpointer ptr;
            {
                IV tmp = SvIV(SvRV(*ptr_ptr));
                ptr = INT2PTR(DCpointer, tmp + offset);
            }
            sv_setsv(RETVAL, sv_2mortal(ptr2sv(aTHX_ ptr, _type)));
            break;
        }
    }
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Aggregate_EXISTS) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    char *u = SvPV_nolen(ST(1));
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(u, SvPV(*name, PL_na)) == 0) { XSRETURN_YES; }
    }
    XSRETURN_NO;
}

XS_INTERNAL(Affix_Aggregate_FIRSTKEY) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "union");
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        ST(0) = sv_mortalcopy(*name);
        XSRETURN(1);
    }
    XSRETURN_UNDEF;
}

XS_INTERNAL(Affix_Aggregate_NEXTKEY) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    char *u = SvPV_nolen(ST(1));
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(u, SvPV(*name, PL_na)) == 0 && i + 1 < size) {
            elm = av_fetch(types, i + 1, 0);
            name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
            ST(0) = sv_mortalcopy(*name);
            XSRETURN(1);
        }
    }
    XSRETURN_UNDEF;
}

#define TYPE(NAME, AFFIX_CHAR, DC_CHAR)                                                            \
    {                                                                                              \
        set_isa("Affix::Type::" #NAME, "Affix::Type::Base");                                       \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::" #NAME, Affix_Type_##NAME, __FILE__, "");            \
            XSANY.any_i32 = (int)AFFIX_CHAR;                                                       \
        }                                                                                          \
        export_function("Affix", #NAME, "types");                                                  \
        /* Overload magic: */                                                                      \
        sv_setsv(get_sv("Affix::Type::" #NAME "::()", TRUE), &PL_sv_yes);                          \
        /* overload as sigchars with fallbacks */                                                  \
        cv = newXSproto_portable("Affix::Type::" #NAME "::()", Affix_Type_asint, __FILE__, "$");   \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::({", Affix_Type_asint, __FILE__, "$");   \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(function", Affix_Type_asint, __FILE__,  \
                                 "$");                                                             \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv =                                                                                       \
            newXSproto_portable("Affix::Type::" #NAME "::(\"\"", Affix_Type_asint, __FILE__, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(*/}", Affix_Type_asint, __FILE__, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(defined", Affix_Type_asint, __FILE__,   \
                                 "$");                                                             \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv =                                                                                       \
            newXSproto_portable("Affix::Type::" #NAME "::(here", Affix_Type_asint, __FILE__, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(/*", Affix_Type_asint, __FILE__, "$");  \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
    }

#define CC_TYPE(NAME, DC_CHAR)                                                                     \
    {                                                                                              \
        set_isa("Affix::Type::CC::" #NAME, "Affix::Type::CC");                                     \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::CC_" #NAME, Affix_CC_##NAME, __FILE__, "");           \
            XSANY.any_i32 = (int)DC_CHAR;                                                          \
        }                                                                                          \
        export_function("Affix", "CC_" #NAME, "types");                                            \
        export_function("Affix", "CC_" #NAME, "cc");                                               \
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable("Affix::Type::CC::" #NAME "::(\"\"", Affix_Type_asint, __FILE__,  \
                                 ";$");                                                            \
        XSANY.any_i32 = (int)DC_SIGCHAR_CC_PREFIX;                                                 \
        /* Making a sub named "Affix::Type::Int::()" allows the package */                         \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Type::Int") to return true. */                             \
        (void)newXSproto_portable("Affix::Type::CC::" #NAME "::()", Affix_Type_asint, __FILE__,    \
                                  ";$");                                                           \
        XSANY.any_i32 = (int)DC_SIGCHAR_CC_PREFIX;                                                 \
    }

XS_EXTERNAL(boot_Affix) {
    dVAR;
    dXSBOOTARGSXSAPIVERCHK;
    PERL_UNUSED_VAR(items);

#ifdef USE_ITHREADS
    my_perl = (PerlInterpreter *)PERL_GET_CONTEXT;
#endif

    MY_CXT_INIT;

    // Allow user defined value in a BEGIN{ } block
    SV *vmsize = get_sv("Affix::VMSize", 0);
    MY_CXT.cvm = dcNewCallVM(vmsize == NULL ? 8192 : SvIV(vmsize));

    TYPE(Any, AFFIX_ARG_SV, DC_SIGCHAR_SV);
    TYPE(Void, AFFIX_ARG_VOID, DC_SIGCHAR_VOID);
    TYPE(Bool, AFFIX_ARG_BOOL, DC_SIGCHAR_BOOL);
    TYPE(Char, AFFIX_ARG_CHAR, DC_SIGCHAR_CHAR);
    EXT_TYPE(CharEnum, AFFIX_ARG_CHAR, DC_SIGCHAR_CHAR);
    TYPE(UChar, AFFIX_ARG_UCHAR, DC_SIGCHAR_CHAR);
    switch (WCHAR_T_SIZE) {
    case I8SIZE:
        TYPE(WChar, AFFIX_ARG_WCHAR, DC_SIGCHAR_CHAR);
        break;
    case SHORTSIZE:
        TYPE(WChar, AFFIX_ARG_WCHAR, DC_SIGCHAR_SHORT);
        break;
    case INTSIZE:
        TYPE(WChar, AFFIX_ARG_WCHAR, DC_SIGCHAR_INT);
        break;
    default:
        warn("Invalid wchar_t size (%ld)! This is a bug. Report it.", WCHAR_T_SIZE);
    }
    TYPE(Short, AFFIX_ARG_SHORT, DC_SIGCHAR_SHORT);
    TYPE(UShort, AFFIX_ARG_USHORT, DC_SIGCHAR_SHORT);
    TYPE(Int, AFFIX_ARG_INT, DC_SIGCHAR_INT);
    EXT_TYPE(Enum, AFFIX_ARG_INT, DC_SIGCHAR_INT);
    EXT_TYPE(IntEnum, AFFIX_ARG_INT, DC_SIGCHAR_INT);
    TYPE(UInt, AFFIX_ARG_UINT, DC_SIGCHAR_INT);
    EXT_TYPE(UIntEnum, AFFIX_ARG_UINT, DC_SIGCHAR_UINT);
    TYPE(Long, AFFIX_ARG_LONG, DC_SIGCHAR_LONG);
    TYPE(ULong, AFFIX_ARG_ULONG, DC_SIGCHAR_LONG);
    TYPE(LongLong, AFFIX_ARG_LONGLONG, DC_SIGCHAR_LONGLONG);
    TYPE(ULongLong, AFFIX_ARG_ULONGLONG, DC_SIGCHAR_LONGLONG);
#if Size_t_size == INTSIZE
    TYPE(Size_t, AFFIX_ARG_SIZE_T, DC_SIGCHAR_UINT);
    TYPE(SSize_t, AFFIX_ARG_SSIZE_T, DC_SIGCHAR_INT);
#elif Size_t_size == LONGSIZE
    TYPE(Size_t, AFFIX_ARG_SIZE_T, DC_SIGCHAR_ULONG);
    TYPE(SSize_t, AFFIX_ARG_SSIZE_T, DC_SIGCHAR_LONG);
#elif Size_t_size == LONGLONGSIZE
    TYPE(Size_t, AFFIX_ARG_SIZE_T, DC_SIGCHAR_ULONGLONG);
    TYPE(SSize_t, AFFIX_ARG_SSIZE_T, DC_SIGCHAR_LONGLONG);
#else // quadmath is broken
    TYPE(Size_t, AFFIX_ARG_SIZE_T, DC_SIGCHAR_ULONGLONG);
    TYPE(SSize_t, AFFIX_ARG_SSIZE_T, DC_SIGCHAR_LONGLONG);
#endif
    TYPE(Float, AFFIX_ARG_FLOAT, DC_SIGCHAR_FLOAT);
    TYPE(Double, AFFIX_ARG_DOUBLE, DC_SIGCHAR_DOUBLE);
    TYPE(Str, AFFIX_ARG_ASCIISTR, DC_SIGCHAR_STRING);
    TYPE(WStr, AFFIX_ARG_UTF16STR, DC_SIGCHAR_POINTER);

    /*
    #define AFFIX_ARG_UTF8STR 18
    */
    EXT_TYPE(Struct, AFFIX_ARG_CSTRUCT, AFFIX_ARG_CSTRUCT);
    EXT_TYPE(ArrayRef, AFFIX_ARG_CARRAY, AFFIX_ARG_CARRAY);
    EXT_TYPE(CodeRef, AFFIX_ARG_CALLBACK, AFFIX_ARG_CALLBACK);

    /*
    #define AFFIX_ARG_VMARRAY 30
    */
    EXT_TYPE(Union, AFFIX_ARG_CUNION, AFFIX_ARG_CUNION);

    /*
    #define AFFIX_ARG_CPPSTRUCT 44
    */
    set_isa("Affix::Type::CC", "Affix::Type::Base");
    CC_TYPE(DEFAULT, DC_SIGCHAR_CC_DEFAULT);
    CC_TYPE(THISCALL, DC_SIGCHAR_CC_THISCALL);
    CC_TYPE(ELLIPSIS, DC_SIGCHAR_CC_ELLIPSIS);
    CC_TYPE(ELLIPSIS_VARARGS, DC_SIGCHAR_CC_ELLIPSIS_VARARGS);
    CC_TYPE(CDECL, DC_SIGCHAR_CC_CDECL);
    CC_TYPE(STDCALL, DC_SIGCHAR_CC_STDCALL);
    CC_TYPE(FASTCALL_MS, DC_SIGCHAR_CC_FASTCALL_MS);
    CC_TYPE(FASTCALL_GNU, DC_SIGCHAR_CC_FASTCALL_GNU);
    CC_TYPE(THISCALL_MS, DC_SIGCHAR_CC_THISCALL_MS);
    CC_TYPE(THISCALL_GNU, DC_SIGCHAR_CC_THISCALL_GNU);
    CC_TYPE(ARM_ARM, DC_SIGCHAR_CC_ARM_ARM);
    CC_TYPE(ARM_THUMB, DC_SIGCHAR_CC_ARM_THUMB);
    CC_TYPE(SYSCALL, DC_SIGCHAR_CC_SYSCALL);

    (void)newXSproto_portable("Affix::load_lib", Affix_load_lib, __FILE__, "$;$");
    export_function("Affix", "load_lib", "lib");
    (void)newXSproto_portable("Affix::Lib::list_symbols", Affix_Lib_list_symbols, __FILE__, "$");
    (void)newXSproto_portable("Affix::Lib::path", Affix_Lib_path, __FILE__, "$");
    (void)newXSproto_portable("Affix::Lib::free", Affix_Lib_free, __FILE__, "$;$");

    (void)newXSproto_portable("Affix::pin", Affix_pin, __FILE__, "$$$$");
    export_function("Affix", "pin", "default");
    (void)newXSproto_portable("Affix::unpin", Affix_unpin, __FILE__, "$");
    export_function("Affix", "unpin", "default");
    //
    cv = newXSproto_portable("Affix::affix", Affix_affix, __FILE__, "$$@$");
    XSANY.any_i32 = 0;
    export_function("Affix", "affix", "default");
    cv = newXSproto_portable("Affix::wrap", Affix_affix, __FILE__, "$$@$");
    XSANY.any_i32 = 1;
    export_function("Affix", "wrap", "all");
    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, __FILE__, "$");

    //~ (void)newXSproto_portable("Affix::CLONE", XS_Affix_CLONE, __FILE__, ";@");

    // Utilities
    (void)newXSproto_portable("Affix::sv_dump", Affix_sv_dump, __FILE__, "$");

    (void)newXSproto_portable("Affix::typedef", Affix_typedef, __FILE__, "$$");
    export_function("Affix", "typedef", "all");

    //~ export_function("Affix", "DEFAULT_ALIGNMENT", "vars");
    //~ export_constant("Affix", "ALIGNBYTES", "all", AFFIX_ALIGNBYTES);
    export_constant("Affix::Feature", "Syscall", "feature",
#ifdef DC__Feature_Syscall
                    1
#else
                    0
#endif
    );
    export_constant("Affix::Feature", "AggrByVal", "feature",
#ifdef DC__Feature_AggrByVal
                    1
#else
                    0
#endif
    );

    (void)newXSproto_portable("Affix::AggregateBase::FETCH", Affix_Aggregate_FETCH, __FILE__, "$$");
    (void)newXSproto_portable("Affix::AggregateBase::EXISTS", Affix_Aggregate_EXISTS, __FILE__,
                              "$$");
    (void)newXSproto_portable("Affix::AggregateBase::FIRSTKEY", Affix_Aggregate_FIRSTKEY, __FILE__,
                              "$");
    (void)newXSproto_portable("Affix::AggregateBase::NEXTKEY", Affix_Aggregate_NEXTKEY, __FILE__,
                              "$$");
    set_isa("Affix::Struct", "Affix::AggregateBase");
    set_isa("Affix::Union", "Affix::AggregateBase");

    (void)newXSproto_portable("Affix::END", Affix_END, __FILE__, "");

    boot_Affix_Pointer(aTHX_ cv);

    boot_Affix_InstanceOf(aTHX_ cv);

    Perl_xs_boot_epilog(aTHX_ ax);
}
