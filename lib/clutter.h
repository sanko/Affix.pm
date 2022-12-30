#ifdef __cplusplus
extern "C" {
#endif

#define PERL_NO_GET_CONTEXT 1 /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

#ifdef MULTIPLICITY
#define storeTHX(var) (var) = aTHX
#define dTHXfield(var) tTHX var;
#else
#define storeTHX(var) dNOOP
#define dTHXfield(var)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#define dcAllocMem safemalloc
#define dcFreeMem Safefree

//#include "ppport.h"

#ifndef av_count
#define av_count(av) (AvFILL(av) + 1)
#endif

#ifndef aTHX_
#define aTHX_ aTHX,
#endif

#if defined(_WIN32) || defined(_WIN64)
// Handle special Windows stuff
#else
#include <dlfcn.h>
#endif

#include <dyncall.h>
#include <dyncall_callback.h>
#include <dynload.h>

#include <dyncall_callf.h>
#include <dyncall_value.h>

#include <dyncall_signature.h>

#include <dyncall/dyncall/dyncall_aggregate.h>

//{ii[5]Z&<iZ>}
#define DC_SIGCHAR_CODE '&'      // 'p' but allows us to wrap CV * for the user
#define DC_SIGCHAR_ARRAY '['     // 'A' but nicer
#define DC_SIGCHAR_STRUCT '{'    // 'A' but nicer
#define DC_SIGCHAR_UNION '<'     // 'A' but nicer
#define DC_SIGCHAR_BLESSED '$'   // 'p' but an object or subclass of a given package
#define DC_SIGCHAR_ANY '*'       // 'p' but it's really an SV/HV/AV
#define DC_SIGCHAR_ENUM 'e'      // 'i' but with multiple options
#define DC_SIGCHAR_ENUM_UINT 'E' // 'I' but with multiple options
#define DC_SIGCHAR_ENUM_CHAR 'o' // 'c' but with multiple options

// MEM_ALIGNBYTES is messed up by quadmath and long doubles
#define AFFIX_ALIGNBYTES 8

#if Size_t_size == INTSIZE
#define DC_SIGCHAR_SSIZE_T DC_SIGCHAR_INT
#define DC_SIGCHAR_SIZE_T DC_SIGCHAR_UINT
#elif Size_t_size == LONGSIZE
#define DC_SIGCHAR_SSIZE_T DC_SIGCHAR_LONG
#define DC_SIGCHAR_SIZE_T DC_SIGCHAR_ULONG
#elif Size_t_size == LONGLONGSIZE
#define DC_SIGCHAR_SSIZE_T DC_SIGCHAR_LONGLONG
#define DC_SIGCHAR_SIZE_T DC_SIGCHAR_ULONGLONG
#else // quadmath is broken
#define DC_SIGCHAR_SSIZE_T DC_SIGCHAR_LONGLONG
#define DC_SIGCHAR_SIZE_T DC_SIGCHAR_ULONGLONG
#endif

// bring balance
#define DC_SIGCHAR_ARRAY_END ']'
#define DC_SIGCHAR_STRUCT_END '}'
#define DC_SIGCHAR_UNION_END '>'

/* portability stuff not supported by ppport.h yet */

#ifndef STATIC_INLINE /* from 5.13.4 */
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#define STATIC_INLINE static inline
#else
#define STATIC_INLINE static
#endif
#endif /* STATIC_INLINE */

#ifndef newSVpvs_share
#define newSVpvs_share(s) Perl_newSVpvn_share(aTHX_ STR_WITH_LEN(s), 0U)
#endif

#ifndef get_cvs
#define get_cvs(name, flags) get_cv(name, flags)
#endif

#ifndef GvNAME_get
#define GvNAME_get GvNAME
#endif
#ifndef GvNAMELEN_get
#define GvNAMELEN_get GvNAMELEN
#endif

#ifndef CvGV_set
#define CvGV_set(cv, gv) (CvGV(cv) = (gv))
#endif

/* general utility */

#if PERL_BCDVERSION >= 0x5008005
#define LooksLikeNumber(x) looks_like_number(x)
#else
#define LooksLikeNumber(x) (SvPOKp(x) ? looks_like_number(x) : (I32)SvNIOKp(x))
#endif

// added in perl 5.35.7?
#ifndef sv_setbool_mg
#define sv_setbool_mg(sv, b) sv_setsv_mg(sv, boolSV(b))
#endif

#define newAV_mortal() (AV *)sv_2mortal((SV *)newAV())
#define newHV_mortal() (HV *)sv_2mortal((SV *)newHV())
#define newRV_inc_mortal(sv) sv_2mortal(newRV_inc(sv))
#define newRV_noinc_mortal(sv) sv_2mortal(newRV_noinc(sv))
/*
#define DECL_BOOT(name) EXTERN_C XS(CAT2(boot_, name))
#define CALL_BOOT(name)                                                                            \
    STMT_START {                                                                                   \
        PUSHMARK(SP);                                                                              \
        CALL_FPTR(CAT2(boot_, name))(aTHX_ cv);                                                    \
    }                                                                                              \
    STMT_END
*/

/* Useful but undefined in perlapi */
#define FLOATSIZE sizeof(float)
#define BOOLSIZE sizeof(bool) // ha!

const char *file = __FILE__;

/* api wrapping utils */

#define MY_CXT_KEY "Affix::_guts" XS_VERSION

typedef struct {
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

typedef struct CoW {
    DCCallback *cb;
    struct CoW *next;
} CoW;

static CoW *cow;

typedef struct {
    char *sig;
    size_t sig_len;
    char ret;
    void *fptr;
    char *perl_sig;
    DLLib *lib;
    AV *args;
    SV *retval;
    bool reset;
} Call;

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

// http://www.catb.org/esr/structure-packing/#_structure_alignment_and_padding
/* Returns the amount of padding needed after `offset` to ensure that the
following address will be aligned to `alignment`. */
size_t padding_needed_for(size_t offset, size_t alignment) {
    size_t misalignment = offset % alignment;
    if (misalignment) // round to the next multiple of alignment
        return alignment - misalignment;
    return 0; // already a multiple of alignment*/
}

void set_isa(const char *klass, const char *parent) {
    dTHX;
    HV *parent_stash = gv_stashpv(parent, GV_ADD | GV_ADDMULTI);
    av_push(get_av(form("%s::ISA", klass), TRUE), newSVpv(parent, 0));
    // TODO: make this spider up the list and make deeper connections?
}

void register_constant(const char *package, const char *name, SV *value) {
    dTHX;
    HV *_stash = gv_stashpv(package, TRUE);
    newCONSTSUB(_stash, (char *)name, value);
}

void export_function__(HV *_export, const char *what, const char *_tag) {
    dTHX;
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
void export_function(const char *package, const char *what, const char *tag) {
    dTHX;
    export_function__(get_hv(form("%s::EXPORT_TAGS", package), GV_ADD), what, tag);
}

void export_constant(const char *package, const char *name, const char *_tag, double val) {
    dTHX;
    register_constant(package, name, newSVnv(val));
    export_function(package, name, _tag);
}

#define DumpHex(addr, len)                                                                         \
    ;                                                                                              \
    _DumpHex(addr, len, __FILE__, __LINE__)

void _DumpHex(const void *addr, size_t len, const char *file, int line) {
    fflush(stdout);
    int perLine = 16;
    // Silently ignore silly per-line values.
    if (perLine < 4 || perLine > 64) perLine = 16;

    int i;
    unsigned char buff[perLine + 1];
    const unsigned char *pc = (const unsigned char *)addr;

    printf("Dumping %lu bytes from %p at %s line %d\n", len, addr, file, line);

    // Length checks.
    if (len == 0) croak("ZERO LENGTH");

    if (len < 0) croak("NEGATIVE LENGTH: %lu", len);

    for (i = 0; i < len; i++) {
        if ((i % perLine) == 0) { // Only print previous-line ASCII buffer for
            // lines beyond first.
            if (i != 0) printf(" | %s\n", buff);
            printf("#  %04x ", i); // Output the offset of current line.
        }

        // Now the hex code for the specific character.

        printf(" %02x", pc[i]);

        // And buffer a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % perLine] = '.';
        else
            buff[i % perLine] = pc[i];
        buff[(i % perLine) + 1] = '\0';
    }

    // Pad out last line if not exactly perLine characters.

    while ((i % perLine) != 0) {
        printf("   ");
        i++;
    }

    printf(" | %s\n", buff);
    fflush(stdout);
}

SV *enum2sv(pTHX_ SV *type, int in) {
    // warn("enum2sv( type, %d ); at %s line %d", in, __FILE__, __LINE__);

    SV *val = newSViv(in);
    AV *values = MUTABLE_AV(SvRV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "values", 0)));
    for (int i = 0; i < av_count(values); ++i) {
        SV *el = *av_fetch(values, i, 0);
        // Future ref: https://groups.google.com/g/perl.perl5.porters/c/q1k1qfbeVk0
        // if(sv_numeq(val, el))
        if (in == SvIV(el)) return el;
    }
    return val;
}

char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata) {
    // warn("here at %s line %d", __FILE__, __LINE__);
    Callback *cbx = (Callback *)userdata;
    // warn("Triggering callback: %c (%s [%d] return: %c) at %s line %d", cbx->ret, cbx->sig,
    // cbx->sig_len, cbx->ret, __FILE__, __LINE__);
    dTHXa(cbx->perl);

    dSP;
    int count;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);

    EXTEND(SP, cbx->sig_len);
    char type;
    for (int i = 0; i < cbx->sig_len; ++i) {
        type = cbx->sig[i];
        // warn("type : %c", type);
        switch (type) {
        case DC_SIGCHAR_VOID:
            // TODO: push undef?
            break;
        case DC_SIGCHAR_BOOL:
            mPUSHs(boolSV(dcbArgBool(args)));
            break;
        case DC_SIGCHAR_CHAR:
            mPUSHi((IV)dcbArgChar(args));
            break;
        case DC_SIGCHAR_UCHAR:
            mPUSHu((UV)dcbArgChar(args));
            break;
        case DC_SIGCHAR_SHORT:
            mPUSHi((IV)dcbArgShort(args));
            break;
        case DC_SIGCHAR_USHORT:
            mPUSHu((UV)dcbArgShort(args));
            break;
        case DC_SIGCHAR_INT:
            mPUSHi((IV)dcbArgInt(args));
            break;
        case DC_SIGCHAR_UINT:
            mPUSHu((UV)dcbArgInt(args));
            break;
        case DC_SIGCHAR_LONG:
            mPUSHi((IV)dcbArgLong(args));
            break;
        case DC_SIGCHAR_ULONG:
            mPUSHu((UV)dcbArgLong(args));
            break;
        case DC_SIGCHAR_LONGLONG:
            mPUSHi((IV)dcbArgLongLong(args));
            break;
        case DC_SIGCHAR_ULONGLONG:
            mPUSHu((UV)dcbArgLongLong(args));
            break;
        case DC_SIGCHAR_FLOAT:
            mPUSHn((NV)dcbArgFloat(args));
            break;
        case DC_SIGCHAR_DOUBLE:
            mPUSHn((NV)dcbArgDouble(args));
            break;
        case DC_SIGCHAR_POINTER: {
            mPUSHs(sv_setref_pv(newSV(1), "Affix::Pointer", dcbArgPointer(args)));
        } break;
        case DC_SIGCHAR_STRING: {
            DCpointer ptr = dcbArgPointer(args);
            PUSHs(newSVpv((char *)ptr, 0));
        } break;
        case DC_SIGCHAR_BLESSED: {
            DCpointer ptr = dcbArgPointer(args);
            HV *blessed = MUTABLE_HV(SvRV(*av_fetch(cbx->args, i, 0)));
            SV **package = hv_fetchs(blessed, "package", 0);
            PUSHs(sv_setref_pv(newSV(1), SvPV_nolen(*package), ptr));
        } break;
        case DC_SIGCHAR_ENUM:
        case DC_SIGCHAR_ENUM_UINT: {
            PUSHs(enum2sv(aTHX_ * av_fetch(cbx->args, i, 0), dcbArgInt(args)));
        } break;
        case DC_SIGCHAR_ENUM_CHAR: {
            PUSHs(enum2sv(aTHX_ * av_fetch(cbx->args, i, 0), dcbArgChar(args)));
        } break;
        case DC_SIGCHAR_ANY: {
            DCpointer ptr = dcbArgPointer(args);
            SV *sv = newSV(0);
            if (ptr != NULL && SvOK(MUTABLE_SV(ptr))) { sv = MUTABLE_SV(ptr); }
            PUSHs(sv);
        } break;
        default:
            croak("Unhandled callback arg. Type: %c [%s]", cbx->sig[i], cbx->sig);
            break;
        }
    }
    PUTBACK;
    if (cbx->ret == DC_SIGCHAR_VOID) { call_sv(cbx->cv, G_VOID); }
    else {
        count = call_sv(cbx->cv, G_SCALAR);
        if (count != 1) croak("Big trouble: %d returned items", count);
        SPAGAIN;
        switch (cbx->ret) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_BOOL:
            result->B = SvTRUEx(POPs);
            break;
        case DC_SIGCHAR_CHAR:
            result->c = POPu;
            break;
        case DC_SIGCHAR_UCHAR:
            result->C = POPu;
            break;
        case DC_SIGCHAR_SHORT:
            result->s = POPu;
            break;
        case DC_SIGCHAR_USHORT:
            result->S = POPi;
            break;
        case DC_SIGCHAR_INT:
            result->i = POPi;
            break;
        case DC_SIGCHAR_UINT:
            result->I = POPu;
            break;
        case DC_SIGCHAR_LONG:
            result->j = POPl;
            break;
        case DC_SIGCHAR_ULONG:
            result->J = POPul;
            break;
        case DC_SIGCHAR_LONGLONG:
            result->l = POPi;
            break;
        case DC_SIGCHAR_ULONGLONG:
            result->L = POPu;
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
                if (sv_derived_from(sv_ptr, "Affix::Pointer")) {
                    IV tmp = SvIV((SV *)SvRV(sv_ptr));
                    result->p = INT2PTR(DCpointer, tmp);
                }
                else
                    croak("Returned value is not a Affix::Pointer or subclass");
            }
            else
                result->p = NULL; // ha.
        } break;
        case DC_SIGCHAR_STRING:
            result->Z = POPp;
            break;
        default:
            croak("Unhandled return from callback: %c", cbx->ret);
        }
        PUTBACK;
    }

    FREETMPS;
    LEAVE;

    return cbx->ret;
}

const char *ordinal(int n) {
    static const char suffixes[][3] = {"th", "st", "nd", "rd"};
    int ord = n % 100;
    if (ord / 10 == 1) { ord = 0; }
    ord = ord % 10;
    if (ord > 3) { ord = 0; }
    return suffixes[ord];
}

bool is_valid_class_name(SV *sv) { // Stolen from Type::Tiny::XS::Util
    dTHX;
    bool RETVAL;
    SvGETMAGIC(sv);
    if (SvPOKp(sv) && SvCUR(sv) > 0) {
        UV i;
        RETVAL = TRUE;
        for (i = 0; i < SvCUR(sv); i++) {
            char const c = SvPVX(sv)[i];
            if (!(isALNUM(c) || c == ':')) {
                RETVAL = FALSE;
                break;
            }
        }
    }
    else { RETVAL = SvNIOKp(sv) ? TRUE : FALSE; }
    return RETVAL;
}

static size_t _sizeof(pTHX_ SV *type) {
    char *_type =
        SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
    // warn("_sizeof( '%s' ); at %s line %d", _type, __FILE__, __LINE__);
    switch (_type[0]) {
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
    case DC_SIGCHAR_ENUM:
    case DC_SIGCHAR_ENUM_UINT:
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
    case DC_SIGCHAR_STRUCT:
    case DC_SIGCHAR_UNION:
    case DC_SIGCHAR_ARRAY:
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
    case DC_SIGCHAR_CODE: // automatically wrapped in a DCCallback pointer
    case DC_SIGCHAR_POINTER:
    case DC_SIGCHAR_STRING:
    case DC_SIGCHAR_BLESSED:
    case DC_SIGCHAR_ANY:
        return PTRSIZE;
    default:
        croak("Failed to gather sizeof info for unknown type: %s", _type);
        return -1;
    }
}

static size_t _offsetof(pTHX_ SV *type) {
    // warn("_offsetof( type ); at %s line %d", __FILE__, __LINE__);
    if (hv_exists(MUTABLE_HV(SvRV(type)), "offset", 6))
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "offset", 0));
    return 0;
}

static DCaggr *_aggregate(pTHX_ SV *type) {
    // warn("here at %s line %d", __FILE__, __LINE__);
    //  //sv_dump(type);

    char *str = SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
                                       // warn("here at %s line %d", __FILE__, __LINE__);
    size_t size = _sizeof(aTHX_ type);

    switch (str[0]) {
    case DC_SIGCHAR_STRUCT:
    case DC_SIGCHAR_UNION: {
        HV *hv_type = MUTABLE_HV(SvRV(type));
        SV **agg_ = hv_fetch(hv_type, "aggregate", 9, 0);
        if (agg_ != NULL) {
            SV *agg = *agg_;
            if (sv_derived_from(agg, "Dyn::Call::Aggregate")) {
                HV *hv_ptr = MUTABLE_HV(agg);
                IV tmp = SvIV((SV *)SvRV(agg));
                return INT2PTR(DCaggr *, tmp);
            }
            else
                croak("Oh, no...");
        }
        else {
            SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);
            bool packed = false;
            if (str[0] == DC_SIGCHAR_STRUCT) {
                SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
                packed = SvTRUE(*sv_packed);
            }
            AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
            int field_count = av_count(idk_arr);
            DCaggr *agg = dcNewAggr(field_count, size);

            for (int i = 0; i < field_count; ++i) {
                SV **field_ptr = av_fetch(idk_arr, i, 0);
                AV *field = MUTABLE_AV(SvRV(*field_ptr));
                SV **type_ptr = av_fetch(field, 1, 0);
                size_t __sizeof = _sizeof(aTHX_ * type_ptr);
                size_t offset = _offsetof(aTHX_ * type_ptr);
                char *str = SvPVbytex_nolen(*type_ptr);
                switch (str[0]) {
                case DC_SIGCHAR_AGGREGATE:
                case DC_SIGCHAR_STRUCT:
                case DC_SIGCHAR_UNION: {
                    DCaggr *child = _aggregate(aTHX_ * type_ptr);
                    dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                } break;
                case DC_SIGCHAR_ARRAY: {
                    // sv_dump(*type_ptr);
                    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "type", 0);
                    int array_len = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "size", 0));
                    char *str = SvPVbytex_nolen(type);
                    dcAggrField(agg, str[0], offset, array_len);
                    /*warn("dcAggrField(agg, %c, %zd, %d); at %s line %d", str[0], offset,
                       array_len,
                         __FILE__, __LINE__);*/
                } break;
                default: {
                    dcAggrField(agg, str[0], offset, 1);
                    // warn("dcAggrField(agg, %c, %d, 1); at %s line %d", str[0], offset, __FILE__,
                    //     __LINE__);
                } break;
                }
            }
            // warn("here at %s line %d", __FILE__, __LINE__);

            // warn("dcCloseAggr(agg); at %s line %d", __FILE__, __LINE__);
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
        croak("unsupported aggregate: %s at %s line %d", str, __FILE__, __LINE__);
        break;
    }
    }
    return NULL;
}

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type) {
    SV *RETVAL = newSV(0);
    char *_type = SvPV_nolen(type);
    // sv_dump(type);
    //  warn("ptr2sv( %p, '%s' ); at %s line %d", ptr, _type, __FILE__, __LINE__);

    switch (_type[0]) {
    case DC_SIGCHAR_VOID:
        sv_setref_pv(RETVAL, "Affix::Pointer", ptr);
        break;
    case DC_SIGCHAR_BOOL:
        sv_setbool_mg(RETVAL, (bool)*(bool *)ptr);
        break;
    case DC_SIGCHAR_CHAR:
        sv_setiv(RETVAL, (IV) * (char *)ptr);
        break;
    case DC_SIGCHAR_UCHAR:
        sv_setuv(RETVAL, (UV) * (unsigned char *)ptr);
        break;
    case DC_SIGCHAR_SHORT:
        sv_setiv(RETVAL, *(short *)ptr);
        break;
    case DC_SIGCHAR_USHORT:
        sv_setuv(RETVAL, *(unsigned short *)ptr);
        break;
    case DC_SIGCHAR_INT:
        sv_setiv(RETVAL, *(int *)ptr);
        break;
    case DC_SIGCHAR_UINT:
        sv_setuv(RETVAL, *(unsigned int *)ptr);
        break;
    case DC_SIGCHAR_LONG:
        sv_setiv(RETVAL, *(long *)ptr);
        break;
    case DC_SIGCHAR_ULONG:
        sv_setuv(RETVAL, *(unsigned long *)ptr);
        break;
    case DC_SIGCHAR_LONGLONG:
        sv_setiv(RETVAL, *(I64 *)ptr);
        break;
    case DC_SIGCHAR_ULONGLONG:
        sv_setuv(RETVAL, *(U64 *)ptr);
        break;
    case DC_SIGCHAR_FLOAT:
        sv_setnv(RETVAL, *(float *)ptr);
        break;
    case DC_SIGCHAR_DOUBLE:
        sv_setnv(RETVAL, *(double *)ptr);
        break;
    case DC_SIGCHAR_STRING:
        sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
        break;
    case DC_SIGCHAR_ARRAY: {
        AV *RETVAL_ = newAV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type));
        SV *subtype = *hv_fetchs(_type, "type", 0);
        size_t av_len = SvIV(*hv_fetchs(_type, "size", 0));
        size_t pos = PTR2IV(ptr);
        size_t sof = _sizeof(aTHX_ subtype);
        for (size_t i = 0; i < av_len; ++i) {
            av_push(RETVAL_, ptr2sv(aTHX_ INT2PTR(DCpointer, pos), subtype));
            pos += sof;
        }
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case DC_SIGCHAR_STRUCT:
    case DC_SIGCHAR_UNION: {
        // sv_dump(type);
        HV *RETVAL_ = newHV_mortal();
        HV *_type = MUTABLE_HV(SvRV(type));
        // sv_dump(MUTABLE_SV(_type));
        // sv_dump(SvRV(*hv_fetchs(_type, "fields", 0)));
        AV *fields = MUTABLE_AV(SvRV(*hv_fetchs(_type, "fields", 0)));
        size_t field_count = av_count(fields);
        // warn("field_count == %d", field_count);
        for (size_t i = 0; i < field_count; ++i) {
            AV *field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
            // sv_dump(MUTABLE_SV(field));
            SV *name = *av_fetch(field, 0, 0);
            SV *subtype = *av_fetch(field, 1, 0);
            (void)hv_store_ent(
                RETVAL_, name,
                ptr2sv(aTHX_ INT2PTR(DCpointer, PTR2IV(ptr) + _offsetof(aTHX_ subtype)), subtype),
                0);
        }
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));
    } break;
    case DC_SIGCHAR_POINTER: {
        SV *subtype;
        if (sv_derived_from(type, "Affix::Type::Pointer"))
            subtype = *hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0);
        else
            subtype = type;
        char *_subtype = SvPV_nolen(subtype);
        if (_subtype[0] == DC_SIGCHAR_VOID) {
            SV *RETVALSV = newSV(0); // sv_newmortal();
            SvSetSV(RETVAL, sv_setref_pv(RETVALSV, "Dyn::Call::Pointer", ptr));
        }
        else {
            char *_subtype = SvPV_nolen(subtype);
            SvSetSV(RETVAL, ptr2sv(aTHX_ ptr, subtype));
        }
    } break;
    case DC_SIGCHAR_CODE: {
        // warn("here at %s line %d", __FILE__, __LINE__);

        CoW *p = (CoW *)ptr;
        // warn("here at %s line %d", __FILE__, __LINE__);

        Callback *cb = (Callback *)dcbGetUserData((DCCallback *)p->cb);
        // warn("here at %s line %d", __FILE__, __LINE__);

        SvSetSV(RETVAL, cb->cv);
        // warn("here at %s line %d", __FILE__, __LINE__);

    } break;
    default:
        croak("Oh, this is unexpected: %c", _type[0]);
    }
    return RETVAL;
}

void sv2ptr(pTHX_ SV *type, SV *data, DCpointer ptr, bool packed) {
    char *str = SvPVbytex_nolen(type);
    // warn("sv2ptr( %s, data, %p, packed) at %s line %d", str, ptr, __FILE__, __LINE__);
    switch (str[0]) {
    case DC_SIGCHAR_VOID: {
        if (sv_derived_from(data, "Dyn::Call::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(data));
            DCpointer ptr = INT2PTR(DCpointer, tmp);
            Copy((DCpointer)(&data), ptr, 1, intptr_t);
        }
        else
            croak("Expected a subclass of Dyn::Call::Pointer");
    } break;
    case DC_SIGCHAR_BOOL: {
        bool value = SvTRUE(data);
        Copy((char *)(&value), ptr, 1, bool);
    } break;
    case DC_SIGCHAR_CHAR: {
        if (SvIOK(data)) {
            char value = (char)SvIV(data);
            Copy((char *)(&value), ptr, 1, char);
        }
        else {
            char *value = SvPV_nolen(data);
            Copy(value, ptr, 1, char);
            Copy((char *)(&value), ptr, 1, char);
        }
    } break;
    case DC_SIGCHAR_UCHAR: {
        if (SvUOK(data)) {
            unsigned char value = (unsigned char)SvUV(data);
            Copy((char *)(&value), ptr, 1, unsigned char);
        }
        else {
            unsigned char *value = (unsigned char *)SvPV_nolen(data);
            Copy((char *)(&value), ptr, 1, unsigned char);
        }
    } break;
    case DC_SIGCHAR_SHORT: {
        short value = (short)SvIV(data);
        Copy((char *)(&value), ptr, 1, short);
    } break;
    case DC_SIGCHAR_USHORT: {
        unsigned short value = (unsigned short)SvUV(data);
        Copy((char *)(&value), ptr, 1, unsigned short);
    } break;
    case DC_SIGCHAR_INT: {
        int value = SvIV(data);
        Copy((char *)(&value), ptr, 1, int);
    } break;
    case DC_SIGCHAR_UINT: {
        unsigned int value = SvUV(data);
        Copy((char *)(&value), ptr, 1, unsigned int);
    } break;
    case DC_SIGCHAR_LONG: {
        long value = SvIV(data);
        Copy((char *)(&value), ptr, 1, long);
    } break;
    case DC_SIGCHAR_ULONG: {
        unsigned long value = SvUV(data);
        Copy((char *)(&value), ptr, 1, unsigned long);
    } break;
    case DC_SIGCHAR_LONGLONG: {
        I64 value = SvUV(data);
        Copy((char *)(&value), ptr, 1, I64);
    } break;
    case DC_SIGCHAR_ULONGLONG: {
        U64 value = SvUV(data);
        Copy((char *)(&value), ptr, 1, U64);
    } break;
    case DC_SIGCHAR_FLOAT: {
        float value = SvNV(data);
        Copy((char *)(&value), ptr, 1, float);
    } break;
    case DC_SIGCHAR_DOUBLE: {
        double value = SvNV(data);
        Copy((char *)(&value), ptr, 1, double);
    } break;
    case DC_SIGCHAR_STRING: {
        const char *str = SvPV_nolen(data);
        DCpointer value;
        Newxz(value, strlen(str) + 1, char);
        Copy(str, value, strlen(str), char);
        Copy(&value, ptr, 1, intptr_t);
    } break;
    case DC_SIGCHAR_POINTER: {
        HV *hv_ptr = MUTABLE_HV(SvRV(type));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        DCpointer value = safemalloc(_sizeof(aTHX_ * type_ptr));
        sv2ptr(aTHX_ * type_ptr, data, value, packed);
        Copy(&value, ptr, 1, intptr_t);
    } break;
    case DC_SIGCHAR_STRUCT: {
        if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
        size_t size = _sizeof(aTHX_ type);
        HV *hv_type = MUTABLE_HV(SvRV(type));
        HV *hv_data = MUTABLE_HV(SvRV(data));
        SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
        SV **sv_packed = hv_fetchs(hv_type, "packed", 0);
        AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
        int field_count = av_count(av_fields);
        // warn("Building struct with %d fields at %s line %d", field_count, __FILE__,
        // __LINE__);
        for (int i = 0; i < field_count; ++i) {
            SV **field = av_fetch(av_fields, i, 0);
            AV *name_type = MUTABLE_AV(SvRV(*field));
            SV **name_ptr = av_fetch(name_type, 0, 0);
            SV **type_ptr = av_fetch(name_type, 1, 0);
            char *key = SvPVbytex_nolen(*name_ptr);
            // warn("[%d of %d] ...adding field of type %s to struct at %s line %d", i+1,
            // field_count, key, __FILE__, __LINE__);
            if (!hv_exists(hv_data, key, strlen(key)))
                continue; // croak("Expected key %s does not exist in given data", key);
            SV **_data = hv_fetch(hv_data, key, strlen(key), 0);
            char *type = SvPVbytex_nolen(*type_ptr);
            size_t el_len = _sizeof(aTHX_ * type_ptr);
            size_t el_off = _offsetof(aTHX_ * type_ptr);
            if (SvOK(data) || SvOK(SvRV(data)))
                sv2ptr(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 0)),
                       INT2PTR(DCpointer, PTR2IV(ptr) + el_off), packed);
            // pos += el_len;
        }
    } break;
    case DC_SIGCHAR_ARRAY: {
        // sv_dump(data);
        int spot = 1;
        AV *elements = MUTABLE_AV(SvRV(data));
        SV *pointer;
        HV *hv_ptr = MUTABLE_HV(SvRV(type));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        size_t size = SvIV(*size_ptr);
        char *type_char = SvPVbytex_nolen(*type_ptr);

        switch (type_char[0]) {
        case DC_SIGCHAR_CHAR:
        case DC_SIGCHAR_UCHAR: {
            if (SvPOK(data)) {
                if (type_char[0] == DC_SIGCHAR_CHAR) {
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
                // warn("Putting index %d into pointer plus %d", i, pos);
                sv2ptr(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)),
                       INT2PTR(DCpointer, PTR2IV(ptr) + pos), packed);
                pos += (el_len);
            }
        }
            // return _sizeof(aTHX_ type);
        }
    } break;
    case DC_SIGCHAR_CODE: {
        // warn("here at %s line %d", __FILE__, __LINE__);
        DCCallback *cb = NULL;
        {
            CoW *p = cow;
            while (p != NULL) {
                if (p->cb) {
                    Callback *_cb = (Callback *)dcbGetUserData(p->cb);
                    if (SvRV(_cb->cv) == SvRV(data)) {
                        cb = p->cb;
                        break;
                    }
                }
                p = p->next;
            }
        }
        // warn("here at %s line %d", __FILE__, __LINE__);
        if (!cb) {
            HV *field = MUTABLE_HV(SvRV(type)); // Make broad assumptions
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

            callback->cv = SvREFCNT_inc(data);
            storeTHX(callback->perl);
            // warn("here at %s line %d", __FILE__, __LINE__);
            cb = dcbNewCallback(callback->sig, cbHandler, callback);
            // warn("here at %s line %d", __FILE__, __LINE__);
            {
                CoW *hold;
                Newxz(hold, 1, CoW);
                // warn("here at %s line %d", __FILE__, __LINE__);
                hold->cb = cb;
                hold->next = cow;
                cow = hold;
                // warn("here at %s line %d", __FILE__, __LINE__);
                Copy(hold, ptr, 1, DCpointer);
                // warn("here at %s line %d", __FILE__, __LINE__);
            }
            // warn("here at %s line %d", __FILE__, __LINE__);
        }
        // warn("here at %s line %d", __FILE__, __LINE__);
    } break;
    default: {
        char *str = SvPVbytex_nolen(type);
        croak("%c is not a known type in sv2ptr(...)", str[0]);
    }
    }

    return;
}
