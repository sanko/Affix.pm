#ifdef __cplusplus
extern "C" {
#endif

#define PERL_NO_GET_CONTEXT 1 /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef MULTIPLICITY
#define storeTHX(var) (var) = aTHX
#define dTHXfield(var) tTHX var;
#else
#define storeTHX(var) dNOOP
#define dTHXfield(var)
#endif

#define newAV_mortal() (AV *)sv_2mortal((SV *)newAV())
#define newHV_mortal() (HV *)sv_2mortal((SV *)newHV())

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto)                                             \
    (PL_Sv = (SV *)newXS(name, c_impl, file), sv_setpv(PL_Sv, proto), (CV *)PL_Sv)
#endif /* !defined(newXS_flags) */

#if PERL_VERSION_LE(5, 21, 5)
#define newXS_deffile(a, b) Perl_newXS(aTHX_ a, b, file)
#else
#define newXS_deffile(a, b) Perl_newXS_deffile(aTHX_ a, b)
#endif

#define dcAllocMem safemalloc
#define dcFreeMem Safefree

#ifndef av_count
#define av_count(av) (AvFILL(av) + 1)
#endif

#if defined(_WIN32) || defined(_WIN64)
#else
#include <dlfcn.h>
#include <iconv.h>
#endif

// older perls are missing wcslen
// PERL_VERSION is deprecated but PERL_VERSION_LE, etc. do not exist pre-5.34.x
#if /*(defined(PERL_VERSION_LE) && PERL_VERSION_LE(5, 30, '*')) ||*/ PERL_VERSION <= 30
#include <wchar.h>
#endif

#include <dyncall.h>
#include <dyncall_callback.h>
#include <dynload.h>

#include <dyncall_callf.h>
#include <dyncall_value.h>

#include <dyncall_signature.h>

#include <dyncall/dyncall/dyncall_aggregate.h>

const char *file = __FILE__;

/* globals */
#define MY_CXT_KEY "Affix::_guts" XS_VERSION

typedef struct {
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

/* Native argument types */
#define AFFIX_ARG_VOID 0
#define AFFIX_ARG_BOOL 1
#define AFFIX_ARG_CHAR 2
#define AFFIX_ARG_SHORT 4
#define AFFIX_ARG_INT 6
#define AFFIX_ARG_LONG 8
#define AFFIX_ARG_LONGLONG 10
#define AFFIX_ARG_FLOAT 12
#define AFFIX_ARG_DOUBLE 14
#define AFFIX_ARG_ASCIISTR 16
#define AFFIX_ARG_UTF8STR 18
#define AFFIX_ARG_UTF16STR 20
#define AFFIX_ARG_CSTRUCT 22
#define AFFIX_ARG_CARRAY 24
#define AFFIX_ARG_CALLBACK 26
#define AFFIX_ARG_CPOINTER 28
#define AFFIX_ARG_VMARRAY 30
#define AFFIX_ARG_UCHAR 32
#define AFFIX_ARG_USHORT 34
#define AFFIX_ARG_UINT 36
#define AFFIX_ARG_ULONG 38
#define AFFIX_ARG_ULONGLONG 40
#define AFFIX_ARG_CUNION 42
#define AFFIX_ARG_CPPSTRUCT 44
#define AFFIX_ARG_WCHAR 46
#define AFFIX_ARG_TYPE_MASK 62

/* Flag for whether we should free a string after passing it or not. */
#define AFFIX_ARG_NO_FREE_STR 0
#define AFFIX_ARG_FREE_STR 1
#define AFFIX_ARG_FREE_STR_MASK 1

/* Flag for whether we need to refresh a CArray after passing or not. */
#define AFFIX_ARG_NO_REFRESH 0
#define AFFIX_ARG_REFRESH 1
#define AFFIX_ARG_REFRESH_MASK 1
#define AFFIX_ARG_NO_RW 0
#define AFFIX_ARG_RW 256
#define AFFIX_ARG_RW_MASK 256

#define AFFIX_UNMARSHAL_KIND_GENERIC -1
#define AFFIX_UNMARSHAL_KIND_RETURN -2
#define AFFIX_UNMARSHAL_KIND_NATIVECAST -3

/* Useful but undefined in perlapi */
#define FLOAT_SIZE sizeof(float)
#define BOOL_SIZE sizeof(bool)         // ha!
#define DOUBLE_SIZE sizeof(double)     // ugh...
#define INTPTR_T_SIZE sizeof(intptr_t) // ugh...
#define WCHAR_T_SIZE sizeof(wchar_t)

void register_constant(const char *package, const char *name, SV *value) {
    dTHX;
    HV *_stash = gv_stashpv(package, TRUE);
    newCONSTSUB(_stash, (char *)name, value);
}

#define export_function(package, what, tag)                                                        \
    _export_function(aTHX_ get_hv(form("%s::EXPORT_TAGS", package), GV_ADD), what, tag)

void _export_function(pTHX_ HV *_export, const char *what, const char *_tag) {
    SV **tag = hv_fetch(_export, _tag, strlen(_tag), TRUE);
    if (tag && SvOK(*tag) && SvROK(*tag) && (SvTYPE(SvRV(*tag))) == SVt_PVAV)
        av_push((AV *)SvRV(*tag), newSVpv(what, 0));
    else {
        SV *av;
        av = (SV *)newAV();
        av_push((AV *)av, newSVpv(what, 0));
        tag = hv_store(_export, _tag, strlen(_tag), newRV_noinc(av), 0);
    }
}
void export_constant(const char *package, const char *name, const char *_tag, double val) {
    dTHX;
    register_constant(package, name, newSVnv(val));
    export_function(package, name, _tag);
}

void set_isa(const char *klass, const char *parent) {
    dTHX;
    HV *parent_stash = gv_stashpv(parent, GV_ADD | GV_ADDMULTI);
    av_push(get_av(form("%s::ISA", klass), TRUE), newSVpv(parent, 0));
    // TODO: make this spider up the list and make deeper connections?
}

struct CallbackWrapper {
    DCCallback *cb;
};

typedef struct {
    char *sig;
    size_t sig_len;
    char ret;
    char *perl_sig;
    SV *cv;
    AV *args;
    SV *retval;
    dTHXfield(perl)
} Callback;

static size_t _sizeof(pTHX_ SV *type) {
    char *_type = SvPVbytex_nolen(type);
    switch (_type[0]) {
    case AFFIX_ARG_VOID:
        return 0;
    case AFFIX_ARG_BOOL:
        return BOOL_SIZE;
    case AFFIX_ARG_CHAR:
    case AFFIX_ARG_UCHAR:
        return I8SIZE;
    case AFFIX_ARG_SHORT:
    case AFFIX_ARG_USHORT:
        return SHORTSIZE;
    case AFFIX_ARG_INT:
    case AFFIX_ARG_UINT:
        return INTSIZE;
    case AFFIX_ARG_LONG:
    case AFFIX_ARG_ULONG:
        return LONGSIZE;
    case AFFIX_ARG_LONGLONG:
    case AFFIX_ARG_ULONGLONG:
        return LONGLONGSIZE;
    case AFFIX_ARG_FLOAT:
        return FLOAT_SIZE;
    case AFFIX_ARG_DOUBLE:
        return DOUBLE_SIZE;
    case AFFIX_ARG_CSTRUCT:
    case AFFIX_ARG_CUNION:
    case AFFIX_ARG_CARRAY:
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
    case AFFIX_ARG_CALLBACK: // automatically wrapped in a DCCallback pointer
    case AFFIX_ARG_CPOINTER:
    case AFFIX_ARG_ASCIISTR:
    case AFFIX_ARG_UTF16STR:
    //~ case AFFIX_ARG_ANY:
    case AFFIX_ARG_CPPSTRUCT:
        return INTPTR_T_SIZE;
    case AFFIX_ARG_WCHAR:
        return WCHAR_T_SIZE;
    default:
        croak("Failed to gather sizeof info for unknown type: %s", _type);
        return -1;
    }
}

static size_t _offsetof(pTHX_ SV *type) {
    if (hv_exists(MUTABLE_HV(SvRV(type)), "offset", 6))
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "offset", 0));
    return 0;
}

static SV *call_encoding(pTHX_ const char *method, SV *obj, SV *src, SV *check) {
    dSP;
    I32 count;
    SV *dst = &PL_sv_undef;
    PUSHMARK(sp);
    if (check) check = sv_2mortal(newSVsv(check));
    if (!check || SvROK(check) || !SvTRUE_nomg(check)) src = sv_2mortal(newSVsv(src));
    XPUSHs(obj);
    XPUSHs(src);
    XPUSHs(check ? check : &PL_sv_no);
    PUTBACK;
    count = call_method(method, G_SCALAR);
    SPAGAIN;
    if (count > 0) {
        dst = POPs;
        SvREFCNT_inc(dst);
    }
    PUTBACK;
    return dst;
}

// https://www.gnu.org/software/libunistring/manual/html_node/The-wchar_005ft-mess.html
// TODO: store this SV* for the sake of speed
static SV *find_encoding(pTHX) {
    char encoding[9];
    my_snprintf(encoding, 9, "UTF-%d%cE", (WCHAR_T_SIZE == 2 ? 16 : 32),
                ((BYTEORDER == 0x1234 || BYTEORDER == 0x12345678) ? 'L' : 'B'));
    // warn("encoding: %s", encoding);
    dSP;
    int count;
    require_pv("Encode.pm");
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(sv_2mortal(newSVpv(encoding, 0)));
    PUTBACK;
    count = call_pv("Encode::find_encoding", G_SCALAR);
    SPAGAIN;
    if (SvTRUE(ERRSV)) {
        warn("Error: %s\n", SvPV_nolen(ERRSV));
        (void)POPs;
    }
    if (count != 1) croak("find_encoding fault: bad number of returned values: %d", count);
    SV *encode = POPs;
    SvREFCNT_inc(encode);
    PUTBACK;
    FREETMPS;
    LEAVE;
    return encode;
}

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    SV *RETVAL = newSV(0);
    int16_t type = SvIV(type_sv);
    //~ warn("ptr2sv(%p, %d) at %s line %d", ptr, type, __FILE__, __LINE__);
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
    //~ case AFFIX_ARG_CPOINTER: {
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
    //~ } break;
    //~ case AFFIX_ARG_ASCIISTR:
    //~ sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
    //~ break;
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
    //~ case AFFIX_ARG_CARRAY: {
    //~ AV *RETVAL_ = newAV_mortal();
    //~ HV *_type = MUTABLE_HV(SvRV(type));
    //~ SV *subtype = *hv_fetchs(_type, "type", 0);
    //~ SV **size = hv_fetchs(_type, "size", 0);
    //~ size_t pos = PTR2IV(ptr);
    //~ size_t sof = _sizeof(aTHX_ subtype);
    //~ size_t av_len;
    //~ if (SvOK(*size))
    //~ av_len = SvIV(*size);
    //~ else
    //~ av_len = SvIV(*hv_fetchs(_type, "size_", 0)) + 1;
    //~ for (size_t i = 0; i < av_len; ++i) {
    //~ av_push(RETVAL_, ptr2sv(aTHX_ INT2PTR(DCpointer, pos), subtype));
    //~ pos += sof;
    //~ }
    //~ SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    //~ } break;
    //~ case AFFIX_ARG_CSTRUCT:
    //~ case AFFIX_ARG_CUNION: {
    //~ HV *RETVAL_ = newHV_mortal();
    //~ HV *_type = MUTABLE_HV(SvRV(type));
    //~ AV *fields = MUTABLE_AV(SvRV(*hv_fetchs(_type, "fields", 0)));
    //~ size_t field_count = av_count(fields);
    //~ for (size_t i = 0; i < field_count; ++i) {
    //~ AV *field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
    //~ SV *name = *av_fetch(field, 0, 0);
    //~ SV *subtype = *av_fetch(field, 1, 0);
    //~ (void)hv_store_ent(
    //~ RETVAL_, name,
    //~ ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ subtype)), subtype),
    //~ 0);
    //~ }
    //~ SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    //~ } break;
    //~ case AFFIX_ARG_CALLBACK: {
    //~ CallbackWrapper *p = (CallbackWrapper *)ptr;
    //~ Callback *cb = (Callback *)dcbGetUserData((DCCallback *)p->cb);
    //~ SvSetSV(RETVAL, cb->cv);
    //~ } break;
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
        croak("Oh, this is unexpected: %c", type);
    }
    return RETVAL;
}

void sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed) {
    SV *RETVAL = newSV(0);
    int16_t type = SvIV(type_sv);
    //~ warn("sv2ptr(%d, ..., %p, %s) at %s line %d", type, ptr, (packed ? "true" : "false"),
    //__FILE__, ~ __LINE__);
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
    //~ case AFFIX_ARG_ENUM_CHAR:
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
    //~ case AFFIX_ARG_ENUM:
    case AFFIX_ARG_INT: {
        int value = SvIOK(data) ? SvIV(data) : 0;
        Copy(&value, ptr, 1, int);
    } break;
    //~ case AFFIX_ARG_ENUM_UINT:
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
        else
            Zero(ptr, 1, intptr_t);
    } break;
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
    //~ } break;
    case AFFIX_ARG_CSTRUCT:
        case AFFIX_ARG_CUNION:{
        size_t size = _sizeof(aTHX_ type);
        if (SvOK(data)) {
            if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
            HV *hv_type = MUTABLE_HV(SvRV(type));
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
                if (data != NULL)
                    sv2ptr(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 1)),
                           INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ * type_ptr)), packed);
            }
        }
    } break;
    case AFFIX_ARG_CARRAY: {
        int spot = 1;
        AV *elements = MUTABLE_AV(SvRV(data));
        SV *pointer;
        HV *hv_ptr = MUTABLE_HV(SvRV(type));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        size_t size = SvOK(*size_ptr) ? SvIV(*size_ptr) : av_len(elements);
        hv_stores(hv_ptr, "size_", newSViv(size));
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
            if (SvTYPE(SvRV(data)) != SVt_PVAV) croak("Expected an array");
            // //sv_dump(*type_ptr);
            // //sv_dump(*size_ptr);
            size_t av_len = av_count(elements);
            if (SvOK(*size_ptr)) {
                if (av_len != size)
                    croak("Expected and array of %zu elements; found %zu", size, av_len);
            }
            size_t el_len = _sizeof(aTHX_ * type_ptr);
            size_t pos = 0; // override
            for (int i = 0; i < av_len; ++i) {
                //~ warn("Putting index %d into pointer plus %d", i, pos);
                sv2ptr(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)),
                       INT2PTR(DCpointer, PTR2IV(ptr) + pos), packed);
                pos += (el_len);
            }
        }
            // return _sizeof(aTHX_ type);
        }
    } break;
    case AFFIX_ARG_CALLBACK: {
        DCCallback *cb = NULL;
        HV *field = MUTABLE_HV(SvRV(type)); // Make broad assumptions
        SV **sig = hv_fetchs(field, "signature", 0);
        SV **sig_len = hv_fetchs(field, "sig_len", 0);
        SV **ret = hv_fetchs(field, "return", 0);
        SV **args = hv_fetchs(field, "args", 0);
        Callback *callback;
        Newxz(callback, 1, Callback);
        callback->args = MUTABLE_AV(SvRV(*args));
        callback->sig = SvPV_nolen(*sig);
        callback->sig_len = (size_t)SvIV(*sig_len);
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
    } break;*/
    default: {
        croak("%d is not a known type in sv2ptr(...)", type);
    }
    }
    return;
}

char *locate_lib(aTHX_ char *lib) {
    // Use perl to get the actual path to the library
    dSP;
    int count;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    EXTEND(SP, 1);
    mPUSHp(lib, strlen(lib));
    PUTBACK;
    count = call_pv("Affix::locate_lib", G_SCALAR);
    SPAGAIN;
    if (count == 1) strcpy(lib, SvPVx_nolen(POPs));
    PUTBACK;
    FREETMPS;
    LEAVE;
    return lib;
}

#ifdef _WIN32
static const char *dlerror(void) {
    static char buf[32];
    DWORD dw = GetLastError();
    if (dw == 0) return NULL;
    snprintf(buf, 32, "error 0x%" PRIx32 "", (MVMuint32)dw);
    return buf;
}
#endif

/* Affix::pin( ... ) System
Bind an exported variable to a perl var */
typedef struct {
    void *ptr;
    SV *type_sv;
} var_ptr;

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
    DLLib *lib;
    if (!SvOK(ST(1)))
        lib = NULL;
    else if (SvROK(ST(1)) && sv_derived_from(ST(1), "Affix::Lib")) {
        IV tmp = SvIV(MUTABLE_SV(SvRV(ST(1))));
        lib = INT2PTR(DLLib *, tmp);
    }
    else {
        char *_libpath = locate_lib(SvPV_nolen(ST(1)));
        lib =
#if defined(_WIN32) || defined(_WIN64)
            dlLoadLibrary(_libpath);
#else
            (DLLib *)dlopen(_libpath, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (lib == NULL) {
            char *reason = dlerror();
            croak("Failed to load %s: %s", lib, reason);
        }
    }
    const char *symbol = (const char *)SvPV_nolen(ST(2));
    DCpointer ptr = dlFindSymbol(lib, symbol);
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

// Type system
XS_INTERNAL(Affix_Type_asint) {
    dXSARGS;
    XSRETURN_IV(XSANY.any_i32);
}

#define SIMPLE_TYPE(TYPE)                                                                          \
    XS_INTERNAL(Affix_Type_##TYPE) {                                                               \
        dXSARGS;                                                                                   \
        ST(0) = sv_2mortal(                                                                        \
            sv_bless(newRV_inc(MUTABLE_SV(newHV())), gv_stashpv("Affix::Type::" #TYPE, GV_ADD)));  \
        XSRETURN(1);                                                                               \
    }

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
SIMPLE_TYPE(Float);
SIMPLE_TYPE(Double);
SIMPLE_TYPE(Str);
SIMPLE_TYPE(WStr);

#define TYPE(NAME, AFFIX_CHAR, DC_CHAR)                                                            \
    {                                                                                              \
        set_isa("Affix::Type::" #NAME, "Affix::Type::Base");                                       \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::" #NAME, Affix_Type_##NAME, file, "");                \
            XSANY.any_i32 = (int)AFFIX_CHAR;                                                       \
        }                                                                                          \
        export_function("Affix", #NAME, "types");                                                  \
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(\"\"", Affix_Type_asint, file, ";$");   \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        /* Making a sub named "Affix::Type::Int::()" allows the package */                         \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Type::Int") to return true. */                             \
        (void)newXSproto_portable("Affix::Type::" #NAME "::()", Affix_Type_asint, file, ";$");     \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
    }

XS_INTERNAL(Affix_load_lib) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "lib_name");
    char *_libpath = SvPV_nolen(ST(0));
        DLLib *lib = (DLLib *)
#if defined(_WIN32) || defined(_WIN64)
		     dlLoadLibrary(locate_lib(_libpath);
#else
        dlopen(locate_lib(_libpath), RTLD_NOW);
#endif
				   if (lib == NULL)
				   croak("Failed to load %s: %s", _libpath, dlerror());
				   SV *RETVAL = sv_newmortal();
				   sv_setref_pv(RETVAL, "Affix::Lib", lib);
				   ST(0) = RETVAL;
				   XSRETURN(1);
}

XS_INTERNAL(Affix_END) { // cleanup
        dXSARGS;
        dMY_CXT;
        if (MY_CXT.cvm) dcFree(MY_CXT.cvm);
        XSRETURN_EMPTY;
}

#ifdef __cplusplus
extern "C"
#endif
    XS_EXTERNAL(boot_Affix) {
#if PERL_VERSION_LE(5, 21, 5)
        dVAR;
        dXSARGS;
        XS_VERSION_BOOTCHECK;
#ifdef XS_APIVERSION_BOOTCHECK
        XS_APIVERSION_BOOTCHECK;
#endif
#else
    dVAR;
    dXSBOOTARGSXSAPIVERCHK;
#endif
        /* register the overloading (type 'A') magic */
#if PERL_VERSION_LE(5, 8, 999) /* PERL_VERSION_LT is 5.33+ */
        PL_amagic_generation++;
#endif

        MY_CXT_INIT;

        // Allow user defined value in a BEGIN{ } block
        SV *vmsize = get_sv("Affix::VMSize", 0);
        MY_CXT.cvm = dcNewCallVM(vmsize == NULL ? 8192 : SvIV(vmsize));

        TYPE(Void, AFFIX_ARG_VOID, DC_SIGCHAR_VOID);
        TYPE(Bool, AFFIX_ARG_BOOL, DC_SIGCHAR_BOOL);
        TYPE(Char, AFFIX_ARG_CHAR, DC_SIGCHAR_CHAR);
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
            warn("Invalid wchar_t size (%d)! This is a bug. Report it", WCHAR_T_SIZE);
        }
        TYPE(Short, AFFIX_ARG_SHORT, DC_SIGCHAR_SHORT);
        TYPE(UShort, AFFIX_ARG_USHORT, DC_SIGCHAR_SHORT);
        TYPE(Int, AFFIX_ARG_INT, DC_SIGCHAR_INT);
        TYPE(UInt, AFFIX_ARG_UINT, DC_SIGCHAR_INT);
        TYPE(Long, AFFIX_ARG_LONG, DC_SIGCHAR_LONG);
        TYPE(ULong, AFFIX_ARG_ULONG, DC_SIGCHAR_LONG);
        TYPE(LongLong, AFFIX_ARG_LONGLONG, DC_SIGCHAR_LONGLONG);
        TYPE(ULongLong, AFFIX_ARG_ULONGLONG, DC_SIGCHAR_LONGLONG);
        TYPE(Float, AFFIX_ARG_FLOAT, DC_SIGCHAR_FLOAT);
        TYPE(Double, AFFIX_ARG_DOUBLE, DC_SIGCHAR_DOUBLE);
        TYPE(Str, AFFIX_ARG_ASCIISTR, DC_SIGCHAR_STRING);
        TYPE(WStr, AFFIX_ARG_UTF16STR, DC_SIGCHAR_POINTER);

        /*
        #define AFFIX_ARG_UTF8STR 18
        #define AFFIX_ARG_CSTRUCT 22
        #define AFFIX_ARG_CARRAY 24
        #define AFFIX_ARG_CALLBACK 26
        #define AFFIX_ARG_CPOINTER 28
        #define AFFIX_ARG_VMARRAY 30

        #define AFFIX_ARG_CUNION 42
        #define AFFIX_ARG_CPPSTRUCT 44
        #define AFFIX_ARG_WCHAR 46
        */

        //~ (void)newXSproto_portable("Affix::_list_symbols", XS_Affix__list_symbols, file, "$");

        (void)newXSproto_portable("Affix::load_lib", Affix_load_lib, file, "$;$");
        (void)newXSproto_portable("Affix::pin", Affix_pin, file, "$$$$");
        (void)newXSproto_portable("Affix::unpin", Affix_unpin, file, "$");

        //~ cv = newXSproto_portable("Affix::affix", XS_Affix_affix, file, "$$$;$");
        //~ XSANY.any_i32 = 0;
        //~ cv = newXSproto_portable("Affix::wrap", XS_Affix_affix, file, "$$$;$");
        //~ XSANY.any_i32 = 1;
        //~ (void)newXSproto_portable("Affix::typedef", XS_Affix_typedef, file, "$$");
        //~ (void)newXSproto_portable("Affix::CLONE", XS_Affix_CLONE, file, ";@");
        //~ (void)newXSproto_portable("Affix::sv2ptr", XS_Affix_sv2ptr, file, "$$");
        //~ (void)newXSproto_portable("Affix::ptr2sv", XS_Affix_ptr2sv, file, "$$");

        //~ (void)newXSproto_portable("Affix::DumpHex", XS_Affix_DumpHex, file, "$$");
        //~ (void)newXSproto_portable("Affix::sv_dump", XS_Affix_sv_dump, file, "$");

        //~ (void)newXSproto_portable("Affix::sizeof", XS_Affix_sizeof, file, "$");
        //~ (void)newXSproto_portable("Affix::offsetof", XS_Affix_offsetof, file, "$$");

        //~ (void)newXSproto_portable("Affix::malloc", XS_Affix_malloc, file, "$");
        //~ (void)newXSproto_portable("Affix::calloc", XS_Affix_calloc, file, "$$");
        //~ (void)newXSproto_portable("Affix::realloc", XS_Affix_realloc, file, "$$");
        //~ (void)newXSproto_portable("Affix::free", XS_Affix_free, file, "$");
        //~ (void)newXSproto_portable("Affix::memchr", XS_Affix_memchr, file, "$$$");
        //~ (void)newXSproto_portable("Affix::memcmp", XS_Affix_memcmp, file, "$$$");
        //~ (void)newXSproto_portable("Affix::memset", XS_Affix_memset, file, "$$$");
        //~ (void)newXSproto_portable("Affix::memcpy", XS_Affix_memcpy, file, "$$$");
        //~ (void)newXSproto_portable("Affix::memmove", XS_Affix_memmove, file, "$$$");
        //~ (void)newXSproto_portable("Affix::strdup", XS_Affix_strdup, file, "$");

        //~ (void)newXSproto_portable("Affix::_shutdown", XS_Affix__shutdown, file, "");

        /* The magic for overload gets a GV* via gv_fetchmeth as mentioned above,
        /* and looks in the SV* slot of it for the "fallback" status. */
        //~ sv_setsv(get_sv("Affix::Pointer::()", TRUE), &PL_sv_yes);
        /* Making a sub named "Affix::Pointer::()" allows the package */
        /* to be findable via fetchmethod(), and causes */
        /* overload::Overloaded("Affix::Pointer") to return true. */
        //~ (void)newXS_deffile("Affix::Pointer::()", Affix_Pointer_stringify);

        //~ (void)newXSproto_portable("Affix::Pointer::plus", XS_Affix__Pointer_plus, file, "$$$");
        //~ (void)newXSproto_portable("Affix::Pointer::(+", XS_Affix__Pointer_plus, file, "$$$");
        //~ (void)newXSproto_portable("Affix::Pointer::minus", XS_Affix__Pointer_minus, file,
        //"$$$"); ~ (void)newXSproto_portable("Affix::Pointer::(-", XS_Affix__Pointer_minus, file,
        //"$$$"); ~ (void)newXSproto_portable("Affix::Pointer::as_string",
        // XS_Affix__Pointer_as_string, file, ~ "$;@"); ~
        //(void)newXSproto_portable("Affix::Pointer::(\"\"", XS_Affix__Pointer_as_string, file,
        //"$;@"); ~ (void)newXSproto_portable("Affix::Pointer::raw", XS_Affix__Pointer_raw, file,
        //"$$;$"); ~ (void)newXSproto_portable("Affix::Pointer::dump", XS_Affix__Pointer_dump, file,
        //"$$");

#ifdef USE_ITHREADS
        my_perl = (PerlInterpreter *)PERL_GET_CONTEXT;
#endif

        //~ {
        //~ (void)newXSproto_portable("Affix::Type", Types_type, file, "$");
        //~ (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, file, "$");

        //~ CV *cv;

        //~ TYPE(Pointer, AFFIX_ARG_CPOINTER, AFFIX_ARG_CPOINTER);
        //~ TYPE(Str, AFFIX_ARG_ASCIISTR, AFFIX_ARG_ASCIISTR);
        //~ TYPE(Struct, AFFIX_ARG_CSTRUCT, AFFIX_ARG_AGGREGATE);
        //~ TYPE(ArrayRef, AFFIX_ARG_CARRAY, AFFIX_ARG_AGGREGATE);
        //~ TYPE(Union, AFFIX_ARG_CUNION, AFFIX_ARG_AGGREGATE);
        //~ TYPE(CodeRef, AFFIX_ARG_CALLBACK, AFFIX_ARG_AGGREGATE);
        //~ TYPE(InstanceOf, AFFIX_ARG_CPPSTRUCT, AFFIX_ARG_CPOINTER);
        //~ TYPE(Any, AFFIX_ARG_ANY, AFFIX_ARG_CPOINTER);
        //~ TYPE(SSize_t, AFFIX_ARG_SSIZE_T, AFFIX_ARG_SSIZE_T);
        //~ TYPE(Size_t, AFFIX_ARG_SIZE_T, AFFIX_ARG_SIZE_T);

        //~ TYPE(WStr, AFFIX_ARG_UTF16STR, AFFIX_ARG_CPOINTER);

        //~ TYPE(Enum, AFFIX_ARG_ENUM, AFFIX_ARG_INT);

        //~ TYPE(IntEnum, AFFIX_ARG_ENUM, AFFIX_ARG_INT);
        //~ set_isa("Affix::Type::IntEnum", "Affix::Type::Enum");

        //~ TYPE(UIntEnum, AFFIX_ARG_ENUM_UINT, AFFIX_ARG_UINT);
        //~ set_isa("Affix::Type::UIntEnum", "Affix::Type::Enum");

        //~ TYPE(CharEnum, AFFIX_ARG_ENUM_CHAR, AFFIX_ARG_CHAR);
        //~ set_isa("Affix::Type::CharEnum", "Affix::Type::Enum");

        //~ TYPE(CC_DEFAULT, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_DEFAULT);
        //~ TYPE(CC_THISCALL, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_THISCALL);
        //~ TYPE(CC_ELLIPSIS, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_ELLIPSIS);
        //~ TYPE(CC_ELLIPSIS_VARARGS, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_ELLIPSIS_VARARGS);
        //~ TYPE(CC_CDECL, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_CDECL);
        //~ TYPE(CC_STDCALL, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_STDCALL);
        //~ TYPE(CC_FASTCALL_MS, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_FASTCALL_MS);
        //~ TYPE(CC_FASTCALL_GNU, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_FASTCALL_GNU);
        //~ TYPE(CC_THISCALL_MS, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_THISCALL_MS);
        //~ TYPE(CC_THISCALL_GNU, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_THISCALL_GNU);
        //~ TYPE(CC_ARM_ARM, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_ARM_ARM);
        //~ TYPE(CC_ARM_THUMB, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_ARM_THUMB);
        //~ TYPE(CC_SYSCALL, AFFIX_ARG_CC_PREFIX, AFFIX_ARG_CC_SYSCALL);

        //~ //
        //~ TYPE(Class, AFFIX_ARG_CLASS, AFFIX_ARG_CSTRUCT);
        //~ TYPE(Method, AFFIX_ARG_CLASS_METHOD, AFFIX_ARG_CALLBACK);

        //~ // Enum[]?
        //~ export_function("Affix", "typedef", "types");
        //~ export_function("Affix", "wrap", "default");
        //~ export_function("Affix", "affix", "default");
        //~ export_function("Affix", "MODIFY_CODE_ATTRIBUTES", "default");
        //~ export_function("Affix", "AUTOLOAD", "default");
        //~ }

        //~ {
        //~ export_function("Affix", "sv2ptr", "utility");
        //~ export_function("Affix", "ptr2sv", "utility");
        //~ export_function("Affix", "DumpHex", "utility");
        //~ export_function("Affix", "pin", "default");
        //~ export_function("Affix", "cast", "default");

        //~ export_function("Affix", "DEFAULT_ALIGNMENT", "vars");
        //~ export_constant("Affix", "ALIGNBYTES", "all", AFFIX_ALIGNBYTES);
        //~ export_constant("Affix::Feature", "Syscall", "feature",
        //~ #ifdef DC__Feature_Syscall
        //~ 1
        //~ #else
        //~ 0
        //~ #endif
        //~ );
        //~ export_constant("Affix::Feature", "AggrByVal", "feature",
        //~ #ifdef DC__Feature_AggrByVal
        //~ 1
        //~ #else
        //~ 0
        //~ #endif
        //~ );

        //~ export_constant_char("Affix", "C", "abi", MANGLE_C);
        //~ export_constant_char("Affix", "ITANIUM", "abi", MANGLE_ITANIUM);
        //~ export_constant_char("Affix", "GCC", "abi", MANGLE_GCC);
        //~ export_constant_char("Affix", "MSVC", "abi", MANGLE_MSVC);
        //~ export_constant_char("Affix", "RUST", "abi", MANGLE_RUST);
        //~ export_constant_char("Affix", "SWIFT", "abi", MANGLE_SWIFT);
        //~ export_constant_char("Affix", "D", "abi", MANGLE_D);
        //~ }

        //~ {
        //~ export_function("Affix", "offsetof", "default");
        //~ export_function("Affix", "sizeof", "default");
        //~ export_function("Affix", "malloc", "memory");
        //~ export_function("Affix", "calloc", "memory");
        //~ export_function("Affix", "realloc", "memory");
        //~ export_function("Affix", "free", "memory");
        //~ export_function("Affix", "memchr", "memory");
        //~ export_function("Affix", "memcmp", "memory");
        //~ export_function("Affix", "memset", "memory");
        //~ export_function("Affix", "memcpy", "memory");
        //~ export_function("Affix", "memmove", "memory");
        //~ export_function("Affix", "strdup", "memory");
        //~ set_isa("Affix::Pointer", "Dyn::Call::Pointer");
        //~ }

        //~ cv = newXSproto_portable("Affix::hit_it", XS_Affix_hit_it, file, "$$$;$");
        //~ XSANY.any_i32 = 0;
        //~ cv = newXSproto_portable("Affix::wrap_it", XS_Affix_hit_it, file, "$$$;$");
        //~ XSANY.any_i32 = 1;
        //~ (void)newXSproto_portable("Affix::NoThanks::call", XS_Affix_hit_call, file, "$;@");
        //~ (void)newXSproto_portable("Affix::NoThanks::DESTROY", XS_Affix_hit_DESTROY, file, "$");

        (void)newXSproto_portable("Affix::END", Affix_END, file, "");

#if PERL_VERSION_LE(5, 21, 5)
#if PERL_VERSION_GE(5, 9, 0)
        if (PL_unitcheckav) call_list(PL_scopestack_ix, PL_unitcheckav);
#endif
        XSRETURN_YES;
#else
    Perl_xs_boot_epilog(aTHX_ ax);
#endif
}
