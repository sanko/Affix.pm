#ifdef __cplusplus
extern "C" {
#endif

#define PERL_NO_GET_CONTEXT 1 /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

//~ #include "ppport.h"

#ifndef sv_setbool_mg
#define sv_setbool_mg(sv, b) sv_setsv_mg(sv, boolSV(b)) /* new in perl 5.36 */
#endif

#include <wchar.h>

#if __WIN32
#include <windows.h>
#endif

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

#define newAV_mortal() MUTABLE_AV(sv_2mortal((SV *)newAV()))
#define newHV_mortal() MUTABLE_HV(sv_2mortal((SV *)newHV()))

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto)                                             \
    (PL_Sv = (SV *)newXS(name, c_impl, file), sv_setpv(PL_Sv, proto), (CV *)PL_Sv)
#endif /* !defined(newXS_flags) */

#define newXS_deffile(a, b) Perl_newXS_deffile(aTHX_ a, b)

#define dcAllocMem safemalloc
#define dcFreeMem safefree

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
#if DEGUG
#define PING warn_nocontext("Ping at %s line %d", __FILE__, __LINE__);
#else
#define PING ;
#endif

/* globals */
#define MY_CXT_KEY "Affix::_guts" XS_VERSION

typedef struct {
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

// Settings
#define TIE_MAGIC 0 // If true, aggregate values are tied and magical

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
#if Size_t_size == INTSIZE
#define AFFIX_ARG_SSIZE_T AFFIX_ARG_INT
#define AFFIX_ARG_SIZE_T AFFIX_ARG_UINT
#elif Size_t_size == LONGSIZE
#define AFFIX_ARG_SSIZE_T AFFIX_ARG_LONG
#define AFFIX_ARG_SIZE_T AFFIX_ARG_ULONG
#elif Size_t_size == LONGLONGSIZE
#define AFFIX_ARG_SSIZE_T AFFIX_ARG_LONGLONG
#define AFFIX_ARG_SIZE_T AFFIX_ARG_ULONGLONG
#else // quadmath is broken
#define AFFIX_ARG_SSIZE_T AFFIX_ARG_LONGLONG
#define AFFIX_ARG_SIZE_T AFFIX_ARG_ULONGLONG
#endif
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

#include "Affix.h"

const char *type_as_str(int type) {
    switch (type) {
    case AFFIX_ARG_VOID:
        return "Void";
    case AFFIX_ARG_BOOL:
        return "Bool";
    case AFFIX_ARG_CHAR:
        return "Char";
    case AFFIX_ARG_SHORT:
        return "Short";
    case AFFIX_ARG_INT:
        return "Int";
    case AFFIX_ARG_LONG:
        return "Long";
    case AFFIX_ARG_LONGLONG:
        return "LongLong";
    case AFFIX_ARG_FLOAT:
        return "Float";
    case AFFIX_ARG_DOUBLE:
        return "Double";
    case AFFIX_ARG_ASCIISTR:
    case AFFIX_ARG_UTF8STR:
        return "Str";
    case AFFIX_ARG_UTF16STR:
        return "WStr";
    case AFFIX_ARG_CSTRUCT:
        return "Struct";
    case AFFIX_ARG_CARRAY:
        return "ArrayRef";
    case AFFIX_ARG_CALLBACK:
        return "CodeRef";
    case AFFIX_ARG_CPOINTER:
        return "Pointer";
    /*case  AFFIX_ARG_VMARRAY 30*/
    case AFFIX_ARG_UCHAR:
        return "UChar";
    case AFFIX_ARG_USHORT:
        return "UShort";
    case AFFIX_ARG_UINT:
        return "UInt";
    case AFFIX_ARG_ULONG:
        return "ULong";
    case AFFIX_ARG_ULONGLONG:
        return "ULongLong";
    case AFFIX_ARG_CUNION:
        return "Union";
    /*case  AFFIX_ARG_CPPSTRUCT 44*/
    case AFFIX_ARG_WCHAR:
        return "WChar";
    default:
        return "Unknown";
    }
}

int type_as_dc(int type) {
    switch (type) {
    case AFFIX_ARG_VOID:
        return DC_SIGCHAR_VOID;
    case AFFIX_ARG_BOOL:
        return DC_SIGCHAR_BOOL;
    case AFFIX_ARG_CHAR:
        return DC_SIGCHAR_CHAR;
    case AFFIX_ARG_SHORT:
        return DC_SIGCHAR_SHORT;
    case AFFIX_ARG_INT:
        return DC_SIGCHAR_INT;
    case AFFIX_ARG_LONG:
        return DC_SIGCHAR_LONG;
    case AFFIX_ARG_LONGLONG:
        return DC_SIGCHAR_LONGLONG;
    case AFFIX_ARG_FLOAT:
        return DC_SIGCHAR_FLOAT;
    case AFFIX_ARG_DOUBLE:
        return DC_SIGCHAR_DOUBLE;
    case AFFIX_ARG_ASCIISTR:
    case AFFIX_ARG_UTF8STR:
        return DC_SIGCHAR_STRING;
    case AFFIX_ARG_UTF16STR:
    case AFFIX_ARG_CARRAY:
    case AFFIX_ARG_CALLBACK:
    case AFFIX_ARG_CPOINTER:
        return DC_SIGCHAR_POINTER;
    /*case  AFFIX_ARG_VMARRAY 30*/
    case AFFIX_ARG_UCHAR:
        return DC_SIGCHAR_UCHAR;
    case AFFIX_ARG_USHORT:
        return DC_SIGCHAR_USHORT;
    case AFFIX_ARG_UINT:
        return DC_SIGCHAR_UINT;
    case AFFIX_ARG_ULONG:
        return DC_SIGCHAR_ULONG;
    case AFFIX_ARG_ULONGLONG:
        return DC_SIGCHAR_ULONGLONG;
    case AFFIX_ARG_CSTRUCT:
    case AFFIX_ARG_CUNION:
        return DC_SIGCHAR_AGGREGATE;
    /*case  AFFIX_ARG_CPPSTRUCT 44*/
    case AFFIX_ARG_WCHAR:
        return (int)AFFIX_ARG_WCHAR;
    //~ DC_SIGCHAR_POINTER;
    default:
        return -1;
    }
}

/* Alignment. */
#if 1
// HAVE_ALIGNOF
/* A GCC extension. */
#define ALIGNOF(t) __alignof__(t)
#elif defined _MSC_VER
/* MSVC extension. */
#define ALIGNOF(t) __alignof(t)
#else
/* Alignment by measuring structure padding. */
#define ALIGNOF(t)                                                                                 \
    ((char *)(&((struct {                                                                          \
                   char c;                                                                         \
                   t _h;                                                                           \
               } *)0)                                                                              \
                   ->_h) -                                                                         \
     (char *)0)
#endif

// MEM_ALIGNBYTES is messed up by quadmath and long doubles
#define AFFIX_ALIGNBYTES 8

#define BOOL_ALIGN ALIGNOF(bool);
#define INT_ALIGN ALIGNOF(int);
#define I8ALIGN ALIGNOF(char);
#define SHORTALIGN ALIGNOF(short);
#define INTALIGN ALIGNOF(int);
#define LONGALIGN ALIGNOF(long);
#define LONGLONGALIGN ALIGNOF(long long);
#define FLOAT_ALIGN ALIGNOF(float);
#define DOUBLE_ALIGN ALIGNOF(double);
#define INTPTR_T_ALIGN ALIGNOF(intptr_t);
#define WCHAR_T_ALIGN ALIGNOF(wchar_t);

// http://www.catb.org/esr/structure-packing/#_structure_alignment_and_padding
/* Returns the amount of padding needed after `offset` to ensure that the
following address will be aligned to `alignment`. */
size_t padding_needed_for(size_t offset, size_t alignment) {
    if (alignment == 0) return 0;
    size_t misalignment = offset % alignment;
    if (misalignment) // round to the next multiple of alignment
        return alignment - misalignment;
    return 0; // already a multiple of alignment*/
}

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

void export_constant_char(const char *package, const char *name, const char *_tag, char val) {
    dTHX;
    register_constant(package, name, newSVpv(&val, 1));
    export_function(package, name, _tag);
}

void export_constant(const char *package, const char *name, const char *_tag, double val) {
    dTHX;
    register_constant(package, name, newSVnv(val));
    export_function(package, name, _tag);
}

void set_isa(const char *klass, const char *parent) {
    dTHX;
    gv_stashpv(parent, GV_ADD | GV_ADDMULTI);
    av_push(get_av(form("%s::ISA", klass), TRUE), newSVpv(parent, 0));
}

#define DumpHex(addr, len) _DumpHex(aTHX_ addr, len, __FILE__, __LINE__)

void _DumpHex(pTHX_ const void *addr, size_t len, const char *file, int line) {
    fflush(stdout);
    int perLine = 16;
    // Silently ignore silly per-line values.
    if (perLine < 4 || perLine > 64) perLine = 16;
    size_t i;
    U8 buff[perLine + 1];
    const U8 *pc = (const U8 *)addr;
    printf("Dumping %lu bytes from %p at %s line %d\n", len, addr, file, line);
    // Length checks.
    if (len == 0) croak("ZERO LENGTH");
    for (i = 0; i < len; i++) {
        if ((i % perLine) == 0) { // Only print previous-line ASCII buffer for
            // lines beyond first.
            if (i != 0) printf(" | %s\n", buff);
            printf("#  %03zu ", i); // Output the offset of current line.
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
#define DD(scalar) _DD(aTHX_ scalar, __FILE__, __LINE__)
void _DD(pTHX_ SV *scalar, const char *file, int line) {
    Perl_load_module(aTHX_ PERL_LOADMOD_NOIMPORT, newSVpvs("Data::Dump"), NULL, NULL, NULL);
    if (!get_cvs("Data::Dump::dump", GV_NOADD_NOINIT | GV_NO_SVGMAGIC)) return;

    fflush(stdout);
    dSP;
    int count;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    EXTEND(SP, 1);
    PUSHs(scalar);
    PUTBACK;

    count = call_pv("Data::Dump::dump", G_SCALAR);

    SPAGAIN;

    if (count != 1) croak("Big trouble\n");

    STRLEN len;
    const char *s = SvPVx(POPs, len);

    printf("%s at %s line %d\n", s, file, line);
    fflush(stdout);

    PUTBACK;
    FREETMPS;
    LEAVE;
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
    int _type = SvIV(type);
    switch (_type) {
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
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
    case AFFIX_ARG_CARRAY:
        if (LIKELY(hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6)))
            return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        {
            size_t type_alignof = _alignof(aTHX_ * hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0));
            size_t array_length = SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "dyn_size", 0));
            bool packed = SvTRUE(*hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0));
            size_t type_sizeof = _sizeof(aTHX_ * hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0));
            size_t array_sizeof = 0;
            for (size_t i = 0; i < array_length; ++i) {
                array_sizeof += type_sizeof;
                array_sizeof += packed ? 0
                                       : padding_needed_for(array_sizeof, type_alignof > type_sizeof
                                                                              ? type_sizeof
                                                                              : type_alignof);
            }
            return array_sizeof;
        }
        croak("Do some math!");
        return 0;
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
        croak("Failed to gather sizeof info for unknown type: %d", _type);
        return -1;
    }
}

static size_t _alignof(pTHX_ SV *type) {
    int _type = SvIV(type);
    switch (_type) {
    case AFFIX_ARG_VOID:
        return 0;
    case AFFIX_ARG_BOOL:
        return BOOL_ALIGN;
    case AFFIX_ARG_CHAR:
    case AFFIX_ARG_UCHAR:
        return I8ALIGN;
    case AFFIX_ARG_SHORT:
    case AFFIX_ARG_USHORT:
        return SHORTALIGN;
    case AFFIX_ARG_INT:
    case AFFIX_ARG_UINT:
        return INTALIGN;
    case AFFIX_ARG_LONG:
    case AFFIX_ARG_ULONG:
        return LONGALIGN;
    case AFFIX_ARG_LONGLONG:
    case AFFIX_ARG_ULONGLONG:
        return LONGLONGALIGN;
    case AFFIX_ARG_FLOAT:
        return FLOAT_ALIGN;
    case AFFIX_ARG_DOUBLE:
        return DOUBLE_ALIGN;
    case AFFIX_ARG_CSTRUCT:
    case AFFIX_ARG_CUNION:
    case AFFIX_ARG_CARRAY:
        //~ sv_dump(type);
        //~ DD(type);
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "align", 0));
    case AFFIX_ARG_CALLBACK: // automatically wrapped in a DCCallback pointer
    case AFFIX_ARG_CPOINTER:
    case AFFIX_ARG_ASCIISTR:
    case AFFIX_ARG_UTF16STR:
    //~ case AFFIX_ARG_ANY:
    case AFFIX_ARG_CPPSTRUCT:
        return INTPTR_T_ALIGN;
    case AFFIX_ARG_WCHAR:
        return WCHAR_T_ALIGN;
    default:
        croak("Failed to gather alignment info for unknown type: %d", _type);
        return -1;
    }
}

static size_t _offsetof(pTHX_ SV *type) {
    if (hv_exists(MUTABLE_HV(SvRV(type)), "offset", 6))
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "offset", 0));
    return 0;
}

static DCaggr *_aggregate(pTHX_ SV *type) {
    int t = SvIV(type);
    size_t size = _sizeof(aTHX_ type);
    switch (t) {
    case AFFIX_ARG_CSTRUCT:
    case AFFIX_ARG_CPPSTRUCT:
    case AFFIX_ARG_CARRAY:
    case AFFIX_ARG_VMARRAY:
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
                case AFFIX_ARG_CPPSTRUCT:
                case AFFIX_ARG_CSTRUCT:
                case AFFIX_ARG_CUNION: {
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
                sv_setref_pv(RETVALSV, "Affix::Aggregate", (void *)agg);
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

// Callback system
char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata) {
    PERL_UNUSED_VAR(cb);
    Callback *cbx = (Callback *)userdata;
    dTHXa(cbx->perl);
    dSP;
    int count;
    char ret_c = cbx->ret;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    EXTEND(SP, (int)cbx->sig_len);
    char type;
    //~ warn("callback! %d args; sig: %s", cbx->sig_len, cbx->sig);
    /*
    char *sig;
    size_t sig_len;
    char ret;
    char *perl_sig;
    SV *cv;
    AV *args;
    SV *retval;
    dTHXfield(perl)
    */
    for (size_t i = 0; i < cbx->sig_len; ++i) {
        type = cbx->sig[i];
        //~ warn("arg %d of %d: %c", i, cbx->sig_len, type);
        switch (type) {
        case DC_SIGCHAR_VOID:
            // TODO: push undef?
            break;
        case DC_SIGCHAR_BOOL:
            mPUSHs(boolSV(dcbArgBool(args)));
            break;
        case DC_SIGCHAR_CHAR:
        case DC_SIGCHAR_UCHAR: {
            char *c = (char *)safemalloc(sizeof(char) * 2);
            c[0] = dcbArgChar(args);
            c[1] = 0;
            SV *w = newSVpv(c, 1);
            SvUPGRADE(w, SVt_PVNV);
            SvIVX(w) = SvIV(newSViv(c[0]));
            SvIOK_on(w);
            mPUSHs(w);
        } break;
        case AFFIX_ARG_WCHAR: {
            wchar_t *c;
            Newx(c, 2, wchar_t);
            c[0] = (wchar_t)dcbArgLong(args);
            c[1] = 0;
            SV *w = wchar2utf(aTHX_ c, 1);
            SvUPGRADE(w, SVt_PVNV);
            SvIVX(w) = SvIV(newSViv(c[0]));
            SvIOK_on(w);
            mPUSHs(w);
        } break;
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
            DCpointer ptr = dcbArgPointer(args);
            SV *__type = *av_fetch(cbx->args, i, 0);
            int _type = SvIV(__type);
            //~ warn("Pointer to (%d/%s)...", _type, type_as_str(_type));
            //~ sv_dump(__type);
            switch (_type) { // true type
            case AFFIX_ARG_WCHAR: {
                //~ SV *wchar2utf(pTHX_ wchar_t *str, int len);
                mPUSHs(ptr2sv(aTHX_ ptr, newSViv(_type)));
            } break;
            case AFFIX_ARG_VOID: {
                SV *s = ptr2sv(aTHX_ ptr, __type);
                mPUSHs(s);
            } break;
            case AFFIX_ARG_CALLBACK: {
                Callback *cb = (Callback *)dcbGetUserData((DCCallback *)ptr);
                mPUSHs(cb->cv);
            } break;
            default:
                mPUSHs(sv_setref_pv(newSV(1), "Affix::Pointer::Unmanaged", ptr));
                break;
            }
        } break;
        case DC_SIGCHAR_STRING: {
            DCpointer ptr = dcbArgPointer(args);
            PUSHs(newSVpv((char *)ptr, 0));
        } break;
        case AFFIX_ARG_UTF16STR: {
            DCpointer ptr = dcbArgPointer(args);
            SV **type_sv = av_fetch(cbx->args, i, 0);
            PUSHs(ptr2sv(aTHX_ ptr, *type_sv));
            /*
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


            */
        } break;
        //~ case DC_SIGCHAR_INSTANCEOF: {
        //~ DCpointer ptr = dcbArgPointer(args);
        //~ HV *blessed = MUTABLE_HV(SvRV(*av_fetch(cbx->args, i, 0)));
        //~ SV **package = hv_fetchs(blessed, "package", 0);
        //~ PUSHs(sv_setref_pv(newSV(1), SvPV_nolen(*package), ptr));
        //~ } break;
        //~ case DC_SIGCHAR_ENUM:
        //~ case DC_SIGCHAR_ENUM_UINT: {
        //~ PUSHs(enum2sv(aTHX_ * av_fetch(cbx->args, i, 0), dcbArgInt(args)));
        //~ } break;
        //~ case DC_SIGCHAR_ENUM_CHAR: {
        //~ PUSHs(enum2sv(aTHX_ * av_fetch(cbx->args, i, 0), dcbArgChar(args)));
        //~ } break;
        //~ case DC_SIGCHAR_ANY: {
        //~ DCpointer ptr = dcbArgPointer(args);
        //~ SV *sv = newSV(0);
        //~ if (ptr != NULL && SvOK(MUTABLE_SV(ptr))) { sv = MUTABLE_SV(ptr); }
        //~ PUSHs(sv);
        //~ } break;
        default:
            croak("Unhandled callback arg. Type: %c [%s]", cbx->sig[i], cbx->sig);
            break;
        }
    }

    PUTBACK;
    if (cbx->ret == DC_SIGCHAR_VOID) {
        count = call_sv(cbx->cv, G_VOID);
        SPAGAIN;
    }
    else {
        count = call_sv(cbx->cv, G_SCALAR);
        SPAGAIN;
        if (count != 1) croak("Big trouble: %d returned items", count);
        SV *ret = POPs;
        switch (ret_c) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_BOOL:
            result->B = SvTRUEx(ret);
            break;
        case DC_SIGCHAR_CHAR:
            result->c = SvIOK(ret) ? SvIV(ret) : 0;
            break;
        case DC_SIGCHAR_UCHAR:
            result->C = SvIOK(ret) ? ((UV)SvUVx(ret)) : 0;
            break;
        case AFFIX_ARG_WCHAR: {
            ret_c = DC_SIGCHAR_LONG; // Fake it
            if (SvPOK(ret)) {
                STRLEN len;
                (void)SvPVx(ret, len);
                result->L = utf2wchar(aTHX_ ret, len)[0];
            }
            else { result->L = 0; }
        } break;
        case DC_SIGCHAR_SHORT:
            result->s = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_USHORT:
            result->S = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_INT:
            result->i = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_UINT:
            result->I = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_LONG:
            result->j = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_ULONG:
            result->J = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_LONGLONG:
            result->l = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_ULONGLONG:
            result->L = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_FLOAT:
            result->f = SvNOK(ret) ? SvNVx(ret) : 0.0;
            break;
        case DC_SIGCHAR_DOUBLE:
            result->d = SvNOK(ret) ? SvNVx(ret) : 0.0;
            break;
        case DC_SIGCHAR_POINTER: {
            if (SvOK(ret)) {
                if (sv_derived_from(ret, "Affix::Pointer")) {
                    IV tmp = SvIV((SV *)SvRV(ret));
                    result->p = INT2PTR(DCpointer, tmp);
                }
                else
                    croak("Returned value is not a Affix::Pointer or subclass");
            }
            else
                result->p = NULL; // ha.
        } break;
        case DC_SIGCHAR_STRING:
            result->Z = SvPOK(ret) ? SvPVx_nolen_const(ret) : NULL;
            break;
        //~ case DC_SIGCHAR_WIDE_STRING:
        //~ result->p = SvPOK(ret) ? (DCpointer)SvPVx_nolen_const(ret) : NULL;
        //~ ret_c = DC_SIGCHAR_POINTER;
        //~ break;
        //~ case DC_SIGCHAR_STRUCT:
        //~ case DC_SIGCHAR_UNION:
        //~ case DC_SIGCHAR_INSTANCEOF:
        //~ case DC_SIGCHAR_ANY:
        //~ result->p = SvPOK(ret) ?  sv2ptr(aTHX_ ret, _instanceof(aTHX_ cbx->retval), false):
        // NULL; ~ ret_c = DC_SIGCHAR_POINTER; ~ break;
        default:
            croak("Unhandled return from callback: %c", ret_c);
        }
    }
    PUTBACK;

    FREETMPS;
    LEAVE;

    return ret_c;
}
typedef struct { // Used in CUnion and pin()
    void *ptr;
    SV *type_sv;
} var_ptr;

char *locate_lib(pTHX_ char *_lib, int ver) {
    // Use perl to get the actual path to the library
    dSP;
    int count;
    char *retval = NULL;
    if (_lib != NULL) {
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        mXPUSHp(_lib, strlen(_lib));
        if (ver) mXPUSHn(ver);
        PUTBACK;
        count = call_pv("Affix::locate_lib", G_SCALAR);
        SPAGAIN;
        if (count == 1) {
            SV *ret = POPs;
            if (SvOK(ret)) {
                STRLEN len;
                char *__lib = SvPVx(ret, len);
                Newxz(retval, len + 1, char);
                Copy(__lib, retval, len, char);
            }
        }
        PUTBACK;
        FREETMPS;
        LEAVE;
    }
    return retval;
}

#ifdef _WIN32
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
    DLLib *lib;
    if (!SvOK(ST(1)))
        lib = NULL;
    else if (SvROK(ST(1)) && sv_derived_from(ST(1), "Affix::Lib")) {
        IV tmp = SvIV(MUTABLE_SV(SvRV(ST(1))));
        lib = INT2PTR(DLLib *, tmp);
    }
    else {
        const char *_libpath = SvPOK(ST(1)) ? locate_lib(aTHX_ SvPV_nolen(ST(1)), 0) : NULL;
        lib =
#if defined(_WIN32) || defined(_WIN64)
            dlLoadLibrary(_libpath);
#else
            (DLLib *)dlopen(_libpath, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (lib == NULL) {
            croak("Failed to load %s: %s", _libpath, dlerror());
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
    PERL_UNUSED_VAR(items);
    XSRETURN_IV(XSANY.any_i32);
}

#define SIMPLE_TYPE(TYPE)                                                                          \
    XS_INTERNAL(Affix_Type_##TYPE) {                                                               \
        dXSARGS;                                                                                   \
        PERL_UNUSED_VAR(items);                                                                    \
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
SIMPLE_TYPE(Size_t);
SIMPLE_TYPE(SSize_t);
SIMPLE_TYPE(Float);
SIMPLE_TYPE(Double);
SIMPLE_TYPE(Str);
SIMPLE_TYPE(WStr);

#define CC(TYPE)                                                                                   \
    XS_INTERNAL(Affix_CC_##TYPE) {                                                                 \
        dXSARGS;                                                                                   \
        PERL_UNUSED_VAR(items);                                                                    \
        ST(0) = sv_2mortal(sv_bless(newRV_inc(MUTABLE_SV(newHV())),                                \
                                    gv_stashpv("Affix::Type::CC::" #TYPE, GV_ADD)));               \
        XSRETURN(1);                                                                               \
    }

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

XS_INTERNAL(Affix_Type_Pointer) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    bool rw = false;
    switch (av_count(fields)) {
    case 2: {
        SV **rw_ref = av_fetch(fields, 1, 0);
        rw = SvTRUE(*rw_ref);
    } // fall through
    case 1: {
        SV **type_ref = av_fetch(fields, 0, 0);
        SV *type = *type_ref;
        if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
            croak("Pointer[...] expects a subclass of Affix::Type::Base");
        //~ sv_dump(type);
        hv_stores(RETVAL_HV, "type", SvREFCNT_inc(type));
    } break;
    default:
        croak("Pointer[...] expects a single type. e.g. Pointer[Int]");
    };
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)),
                 gv_stashpv(rw ? "Affix::Type::RWPointer" : "Affix::Type::Pointer", GV_ADD)));
    XSRETURN(1);
}

//~ XS_INTERNAL(Affix_Type_RWPointer) {
//~ croak("No.");
//~ }

XS_INTERNAL(Affix_Type_Pointer_RW) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    if (!(sv_isobject(ST(0)) && sv_derived_from(ST(0), "Affix::Type::Pointer")))
        croak("... | RW expects a subclass of Affix::Type::Pointer to the left");
    ST(0) = sv_bless(ST(0), gv_stashpv("Affix::Type::RWPointer", GV_ADD));
    SvSETMAGIC(ST(0));

    XSRETURN(1);
}

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
    char *_libpath =
        SvPOK(ST(0)) ? locate_lib(aTHX_ SvPV_nolen(ST(0)), SvIOK(ST(1)) ? SvIV(ST(1)) : 0) : NULL;
    DLLib *lib =
#if defined(_WIN32) || defined(_WIN64)
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

/* Affix::affix(...) and Affix::wrap(...) System */
struct Affix {
    int16_t call_conv;
    U8 abi;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    char *sym_name;
    void *entry_point;
    AV *arg_info;
    SV *ret_info;
    SV *resolve_lib_name;
};

extern "C" void Affix_trigger(pTHX_ CV *cv) {
    dXSARGS;
    dMY_CXT;

    Affix *ptr = (Affix *)XSANY.any_ptr;
    char **free_strs = NULL;
    void **free_ptrs = NULL;

    int num_args = ptr->num_args, i, p;
    int num_strs = 0, num_ptrs = 0;
    int16_t *arg_types = ptr->arg_types;
    bool void_ret = false;
    DCaggr *agg_ = NULL;

    dcReset(MY_CXT.cvm);

    switch (ptr->ret_type) {
    case AFFIX_ARG_CUNION:
    case AFFIX_ARG_CSTRUCT: {
        agg_ = _aggregate(aTHX_ ptr->ret_info);
        dcBeginCallAggr(MY_CXT.cvm, agg_);
    } break;
    }

    if (UNLIKELY(items != num_args)) {
        if (UNLIKELY(items > num_args))
            croak("Too many arguments; wanted %d, found %d", num_args, items);
        croak("Not enough arguments; wanted %d, found %d", num_args, items);
    }
    for (i = 0, p = 0; LIKELY(i < num_args); ++i, ++p) {
        //~ warn("%d of %d == %s (%d/%d)", i + 1, num_args, type_as_str(arg_types[p]),
        //~ (arg_types[p] & AFFIX_ARG_TYPE_MASK), arg_types[p]);
        //~ sv_dump(ST(i));
        switch (arg_types[p]) {
        case DC_SIGCHAR_CC_PREFIX: {
            // TODO: Not a fan of this entire section
            SV *cc = *av_fetch(ptr->arg_info, i, 0);
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
            i--;
        } break;
        case AFFIX_ARG_VOID: // skip?
            break;
        case AFFIX_ARG_BOOL:
            dcArgBool(MY_CXT.cvm, SvTRUE(ST(i))); // Anything can be a bool
            break;
        case AFFIX_ARG_CHAR:
            dcArgChar(MY_CXT.cvm, (char)(SvIOK(ST(i)) ? SvIV(ST(i)) : *SvPV_nolen(ST(i))));
            break;
        case AFFIX_ARG_UCHAR:
            dcArgChar(MY_CXT.cvm, (U8)(SvIOK(ST(i)) ? SvUV(ST(i)) : *SvPV_nolen(ST(i))));
            break;
        case AFFIX_ARG_WCHAR: {
            if (SvOK(ST(i))) {
                char *eh = SvPV_nolen(ST(i));
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
            dcArgShort(MY_CXT.cvm, (short)(SvIV(ST(i))));
            break;
        case AFFIX_ARG_USHORT:
            dcArgShort(MY_CXT.cvm, (unsigned short)(SvUV(ST(i))));
            break;
        case AFFIX_ARG_INT:
            dcArgInt(MY_CXT.cvm, (int)(SvIV(ST(i))));
            break;
        case AFFIX_ARG_UINT:
            dcArgInt(MY_CXT.cvm, (unsigned int)(SvUV(ST(i))));
            break;
        case AFFIX_ARG_LONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)(SvUV(ST(i))));
            break;
        case AFFIX_ARG_ULONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)(SvUV(ST(i))));
            break;
        case AFFIX_ARG_LONGLONG:
            dcArgLongLong(MY_CXT.cvm, (I64)(SvIV(ST(i))));
            break;
        case AFFIX_ARG_ULONGLONG:
            dcArgLongLong(MY_CXT.cvm, (U64)(SvUV(ST(i))));
            break;
        case AFFIX_ARG_FLOAT:
            dcArgFloat(MY_CXT.cvm, (float)SvNV(ST(i)));
            break;
        case AFFIX_ARG_DOUBLE:
            dcArgDouble(MY_CXT.cvm, (double)SvNV(ST(i)));
            break;
        case AFFIX_ARG_ASCIISTR:
            dcArgPointer(MY_CXT.cvm, SvOK(ST(i)) ? SvPV_nolen(ST(i)) : NULL);
            break;
        case AFFIX_ARG_UTF8STR:
        case AFFIX_ARG_UTF16STR: {
            if (SvOK(ST(i))) {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                Newxz(free_ptrs[num_ptrs], _sizeof(aTHX_ newSViv(AFFIX_ARG_UTF16STR)), char);
                sv2ptr(aTHX_ newSViv(AFFIX_ARG_UTF16STR), ST(i), free_ptrs[num_ptrs], false);
                dcArgPointer(MY_CXT.cvm, *(DCpointer *)(free_ptrs[num_ptrs++]));
            }
            else { dcArgPointer(MY_CXT.cvm, NULL); }
        } break;
        case AFFIX_ARG_CSTRUCT: {
            if (!SvOK(ST(i)) && SvREADONLY(ST(i)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVHV)
                    croak("Type of arg %d must be an hash ref", i + 1);
                //~ AV *elements = MUTABLE_AV(SvRV(ST(i)));
                SV **type = av_fetch(ptr->arg_info, i, 0);
                size_t size = _sizeof(aTHX_ SvRV(*type));
                Newxz(free_ptrs[num_ptrs], size, char);
                DCaggr *agg = _aggregate(aTHX_ SvRV(*type));
                sv2ptr(aTHX_ SvRV(*type), ST(i), free_ptrs[num_ptrs], false);
                dcArgAggr(MY_CXT.cvm, agg, free_ptrs[num_ptrs++]);
            }
        }
        //~ croak("Unhandled arg type at %s line %d", __FILE__, __LINE__);
        break;
        case AFFIX_ARG_CARRAY: {
            if (!SvOK(ST(i)) && SvREADONLY(ST(i)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);

                // free_ptrs = (void **)safemalloc(num_args * sizeof(void *));
                if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVAV)
                    croak("Type of arg %d must be an array ref", i + 1);

                AV *elements = MUTABLE_AV(SvRV(ST(i)));
                SV **type = av_fetch(ptr->arg_info, i, 0);
                HV *hv_ptr = MUTABLE_HV(SvRV(SvRV(*type)));
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
                size_t size = _sizeof(aTHX_(SvRV(*type)));
                Newxz(free_ptrs[num_ptrs], size, char);
                sv2ptr(aTHX_ SvRV(*type), ST(i), free_ptrs[num_ptrs], false);
                dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs++]);
            }
        } break;
        case AFFIX_ARG_CALLBACK: {
            if (SvOK(ST(i))) {
                //~ DCCallback *hold;
                //~ //Newxz(hold, 1, DCCallback);
                //~ sv2ptr(aTHX_ SvRV(*av_fetch(ptr->arg_info, i, 0)), ST(i), hold, false);
                //~ dcArgPointer(MY_CXT.cvm, hold);
                CallbackWrapper *hold;
                Newx(hold, 1, CallbackWrapper);
                sv2ptr(aTHX_ SvRV(*av_fetch(ptr->arg_info, i, 0)), ST(i), hold, false);
                dcArgPointer(MY_CXT.cvm, hold->cb);
            }
            else
                dcArgPointer(MY_CXT.cvm, NULL);
        } break;
        case AFFIX_ARG_CPOINTER: {
            //~ warn("AFFIX_ARG_CPOINTER [%d, %d/%s]", i, SvIV(*av_fetch(ptr->arg_info, i, 0)),
            //~ type_as_str(SvIV(*av_fetch(ptr->arg_info, i, 0))));
            if (UNLIKELY(!SvOK(ST(i)) && SvREADONLY(ST(i)))) { // explicit undef
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else if (LIKELY(sv_isobject(ST(i)) && sv_derived_from(ST(i), "Affix::Pointer"))) {
                IV tmp = SvIV(SvRV(ST(i)));
                dcArgPointer(MY_CXT.cvm, INT2PTR(DCpointer, tmp));
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                SV **type = av_fetch(ptr->arg_info, i, 0);
                if (type == NULL) croak("No idea");
                switch (SvIV(*type)) {
                case AFFIX_ARG_VOID: {
                    if (sv_isobject(ST(i))) croak("Unexpected pointer to blessed object");
                    free_ptrs[num_ptrs] = safemalloc(INTPTR_T_SIZE);
                    sv2ptr(aTHX_ * type, ST(i), free_ptrs[num_ptrs], false);
                    dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                    num_ptrs++;
                } break;
                case AFFIX_ARG_CUNION: {
                    DCpointer p;
                    const MAGIC *mg = SvTIED_mg((SV *)SvRV(ST(i)), PERL_MAGIC_tied);
                    if (!UNLIKELY(
                            //~ SvRMAGICAL(sv) &&
                            LIKELY(SvOK(ST(i)) && SvTYPE(SvRV(ST(i))) == SVt_PVHV && mg
                                   //~ &&  sv_derived_from(SvRV(ST(i)), "Affix::Union")
                                   ))) { // Already a known union pointer
                        p = safemalloc(_sizeof(aTHX_ * type));
                        sv_setsv_mg(ST(i), ptr2sv(aTHX_ p, *av_fetch(ptr->arg_info, i, 0)));
                    }
                    else {
                        mg = SvTIED_mg((SV *)SvRV(ST(i)), PERL_MAGIC_tied);
                        SV *ref = SvTIED_obj(ST(i), mg);
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
                    Newxz(free_ptrs[num_ptrs], _sizeof(aTHX_ * type), char);
                    if (SvOK(ST(i))) { sv2ptr(aTHX_ * type, ST(i), free_ptrs[num_ptrs], false); }
                    dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                    num_ptrs++;
                }
                }
#if 0
					if (UNLIKELY(SvREADONLY(ST(i)))) { // explicit undef
						if (LIKELY(SvOK(ST(i)))) {

						} else
							free_ptrs[num_ptrs] = NULL;
						dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
					} else if (SvOK(ST(i))) {
						warn("Alpha");
						if (subtype_ptr == NULL
								/*sv_derived_from(ST(i), "Affix::Type::Void")*/
						   ) {
							warn("Send void pointer...");
							IV tmp = SvIV((SV *)SvRV(ST(i)));
							free_ptrs[num_ptrs] = INT2PTR(DCpointer, tmp);
						} else {
							warn("Sending... something else?");
							if (sv_isobject(ST(i))) croak("Unexpected pointer to blessed object");
							free_ptrs[num_ptrs] = safemalloc(_sizeof(aTHX_ * subtype_ptr));
							sv2ptr(aTHX_ * subtype_ptr, ST(i), free_ptrs[num_ptrs], false);
						}
					} else { // treat as if it's an lvalue
						warn("...idk?");
						if (subtype_ptr == NULL
								/*sv_derived_from(ST(i), "Affix::Type::Void")*/
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
        case AFFIX_ARG_VMARRAY:
        case AFFIX_ARG_CUNION:
        case AFFIX_ARG_CPPSTRUCT:
            croak("Unhandled arg type at %s line %d", __FILE__, __LINE__);
            break;
        }
    }
    //
    /*
    #define AFFIX_ARG_CSTRUCT 22
    #define AFFIX_ARG_CARRAY 24
    #define AFFIX_ARG_VMARRAY 30
    #define AFFIX_ARG_CUNION 42
    #define AFFIX_ARG_CPPSTRUCT 44
       */
    //~ warn("oy %d (%s) / %p at %s line %d", ptr->ret_type, type_as_str(ptr->ret_type),
    //~ ptr->entry_point, __FILE__, __LINE__);
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
        SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ptr->ret_info)), "type", 0);
        if (sv_derived_from(type, "Affix::Type::Void")) {
            RETVAL = newSV(1);
            DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
            sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", p);
        }
        else {
            DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
            RETVAL = newRV_noinc(ptr2sv(aTHX_ p, type));
        }
    } break;
    case AFFIX_ARG_CUNION:
    case AFFIX_ARG_CSTRUCT: {
        //~ warn ("        _sizeof(aTHX_ ptr->ret_info): %d",_sizeof(aTHX_ ptr->ret_info));
        DCpointer p = safemalloc(_sizeof(aTHX_ ptr->ret_info));
        dcCallAggr(MY_CXT.cvm, ptr->entry_point, agg_, p);
        RETVAL = ptr2sv(aTHX_ p, ptr->ret_info);
    } break;
    default:
        sv_dump(ptr->ret_info);
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

    //
    /* Free any memory that we need to. */
    if (free_strs != NULL) {
        for (i = 0; UNLIKELY(i < num_strs); ++i)
            safefree(free_strs[i]);
        safefree(free_strs);
    }
    {
        size_t p = 0;
        for (i = 0; LIKELY(i < items); ++i) {
            if (LIKELY(!SvREADONLY(ST(i)))) { // explicit undef
                switch (arg_types[i]) {
                case AFFIX_ARG_CARRAY: {
                    SV *sv = ptr2sv(aTHX_ free_ptrs[p++], SvRV(*av_fetch(ptr->arg_info, i, 0)));
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

    if (UNLIKELY(free_ptrs != NULL)) {
        for (i = 0; LIKELY(i < num_ptrs); ++i) {
            safefree(free_ptrs[i]);
        }
        safefree(free_ptrs);
    }
    //~ if(agg_) dcFreeAggr(agg_);

    if (UNLIKELY(void_ret)) XSRETURN_EMPTY;

    ST(0) = sv_2mortal(RETVAL);

    XSRETURN(1);
}

char *_mangle(pTHX_ const char *abi, SV *lib, const char *symbol, SV *args) {
    char *retval;
    {
        dSP;
        int count;
        SV *err_tmp;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        XPUSHs(lib);
        mXPUSHp(symbol, strlen(symbol));
        XPUSHs(args);
        PUTBACK;
        count = call_pv(form("Affix::%s_mangle", abi), G_SCALAR | G_EVAL | G_KEEPERR);
        SPAGAIN;
        err_tmp = ERRSV;
        if (SvTRUE(err_tmp)) {
            croak("Malformed call to %s_mangle( ... )\n", abi, SvPV_nolen(err_tmp));
            POPs;
        }
        else if (count != 1) { croak("Failed to mangle %s symbol named %s", abi, abi); }
        else {
            retval = POPp;
            // SvSetMagicSV(type, retval);
        }
        // FREETMPS;
        LEAVE;
    }
    return retval;
}

XS_INTERNAL(Affix_affix) {
    dXSARGS;
    dXSI32;
    PING;
    if (items != 4) croak_xs_usage(cv, "$lib, $symbol, @arg_types, $ret_type");
    SV *RETVAL;
    Affix *ret;
    Newx(ret, 1, Affix);

    // Dumb defaults
    ret->abi = (U8)AFFIX_ABI_C;
    ret->num_args = 0;
    ret->arg_info = newAV();

    char *prototype = NULL;
    char *name = NULL;
    {
        SV *lib;

        // affix($lib, ..., ..., ...)
        // affix([$lib, ABI_C], ..., ..., ...)
        // wrap($lib, ..., ...., ...
        // wrap([$lib, ABI_C], ..., ...., ...
        {
            if (UNLIKELY(SvROK(ST(0)) && SvTYPE(SvRV(ST(0))) == SVt_PVAV)) {
                AV *tmp = MUTABLE_AV(SvRV(ST(0)));
                size_t tmp_len = av_count(tmp);
                // Non-fatal
                if (UNLIKELY(!(tmp_len == 1 || tmp_len == 2))) { warn("Expected a lib and ABI"); }
                lib = *av_fetch(tmp, 0, false);
                //
                SV **ptr_abi = av_fetch(tmp, 1, false);
                if (ptr_abi == NULL || !SvOK(*ptr_abi)) { croak("Expected a lib and ABI"); }
                else {
                    U8 abi = (U8)*SvPV_nolen(*ptr_abi);
                    switch (abi) {
                    case AFFIX_ABI_C:
                    case AFFIX_ABI_ITANIUM:
                    case AFFIX_ABI_RUST:
                        ret->abi = abi;
                        break;
                    case AFFIX_ABI_SWIFT:
                    case AFFIX_ABI_D:
                    default:
                        croak("Unknown or unsupported ABI");
                    }
                }
            }
            else { lib = newSVsv(ST(0)); }
            //
            ret->lib_name = SvPOK(lib) ? locate_lib(aTHX_ SvPV_nolen(lib), 0) : NULL;
            ret->lib_handle =
#if defined(_WIN32) || defined(_WIN64)
                dlLoadLibrary(ret->lib_name);
#else
                (DLLib *)dlopen(ret->lib_name, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
            if (!ret->lib_handle) {
                croak("Failed to load %s: %s", ret->lib_name, dlerror());
            }
        }

        // affix(..., ..., [Int], ...)
        // wrap(..., ..., [], ...)
        {
            STMT_START {
                SV *const xsub_tmp_sv = ST(2);
                SvGETMAGIC(xsub_tmp_sv);
                if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                    AV *tmp_args = (AV *)SvRV(xsub_tmp_sv);
                    size_t args_len = av_count(tmp_args);
                    SV **tmp_arg;
                    Newxz(ret->arg_types, args_len, int16_t); // TODO: safefree
                    Newxz(prototype, args_len, char);
                    for (size_t i = 0; i < args_len; ++i) {
                        SV **tmp_arg = av_fetch(tmp_args, i, false);
                        if (LIKELY(SvROK(*tmp_arg) &&
                                   sv_derived_from(*tmp_arg, "Affix::Type::Base"))) {
                            ret->arg_types[i] = (int16_t)SvIV(*tmp_arg);
                            if (UNLIKELY(sv_derived_from(*tmp_arg, "Affix::Type::CC"))) {
                                av_store(ret->arg_info, i, *tmp_arg);
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
                                switch (ret->arg_types[i]) {
                                case AFFIX_ARG_CPOINTER: {
                                    SV *sv = *hv_fetchs(MUTABLE_HV(SvRV(*tmp_arg)), "type", 0);
                                    av_store(ret->arg_info, i, sv);
                                    break;
                                }
                                case AFFIX_ARG_CARRAY:
                                case AFFIX_ARG_VMARRAY:
                                case AFFIX_ARG_CSTRUCT:
                                case AFFIX_ARG_CALLBACK:
                                case AFFIX_ARG_CUNION:
                                case AFFIX_ARG_CPPSTRUCT: {
                                    av_store(ret->arg_info, i, newRV_inc(*tmp_arg));
                                } break;
                                }
                            }
                            av_push(ret->arg_info, newSVsv(*tmp_arg));
                        }
                        else { croak("Unexpected arg type in slot %d", i + 1); }
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
            if (UNLIKELY(SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV)) {
                AV *tmp = MUTABLE_AV(SvRV(ST(1)));
                size_t tmp_len = av_count(tmp);
                if (tmp_len != 2) { croak("Expected a lib and ABI"); }
                if (ix == 1 && tmp_len > 1) {
                    warn("wrap( ... ) isn't expecting a name and has ignored it");
                }
                else if (tmp_len != 2) { croak("Expected a symbol and name"); }
                SV **name_ptr = av_fetch(tmp, 0, false);
                if (UNLIKELY(name_ptr == NULL || !SvPOK(*name_ptr))) {
                    croak("Undefined symbol name");
                }
                ret->sym_name = SvPV_nolen(*name_ptr);
                name = SvPV_nolen(*av_fetch(tmp, 1, false));
            }
            else if (UNLIKELY(!SvPOK(ST(1)))) { croak("Undefined symbol name"); }
            else { name = ret->sym_name = SvPV_nolen(ST(1)); }

            {
                SV *LIBSV;
                LIBSV = sv_newmortal();
                sv_setref_pv(LIBSV, "Affix::Lib", (void *)ret->lib_handle);

                switch (ret->abi) {
                case AFFIX_ABI_C:
                    break;
                case AFFIX_ABI_ITANIUM:
                    ret->sym_name = _mangle(aTHX_ "Itanium", LIBSV, ret->sym_name, ST(2));
                    break;
                case AFFIX_ABI_RUST:
                    ret->sym_name = _mangle(aTHX_ "Rust_legacy", LIBSV, ret->sym_name, ST(2));
                    break;
                case AFFIX_ABI_SWIFT:
                case AFFIX_ABI_D:
                    croak("Unhandled ABI. Patches welcome");
                    break;
                default:
                    croak("Unknown ABI. Patches welcome");
                    break;
                }
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

    ret->entry_point = dlFindSymbol(ret->lib_handle, ret->sym_name);

    //~ warn("lib: %p, entry_point: %p, sym_name: %s, as: %s, prototype: %s, ix: %d, abi: %c: ",
    //~ ret->lib_handle, ret->entry_point, ret->sym_name, name, prototype, ix, ret->abi);
    //~ DD(MUTABLE_SV(ret->arg_info));
    //~ DD(ret->ret_info);

    /*
    struct Affix {
    int16_t call_conv;
    U8 abi;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    char *sym_name;
    void *entry_point;
    AV *arg_info;
    SV *ret_info;
    SV *resolve_lib_name;
    };
    */
    if (!ret->entry_point)
        croak("Failed to locate symbol named '%s' in %s", ret->sym_name, ret->lib_name);
    STMT_START {
        cv = newXSproto_portable(ix == 0 ? name : NULL, Affix_trigger, file, prototype);
        if (UNLIKELY(cv == NULL))
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (DCpointer)ret;
    }
    STMT_END;
    RETVAL = sv_bless((UNLIKELY(ix == 1) ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                      gv_stashpv("Affix", GV_ADD));

    ST(0) = sv_2mortal(RETVAL);
    if (prototype) safefree(prototype);

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
    U8 abi;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    char *sym_name;
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

XS_INTERNAL(Affix_Type_Pointer_marshal) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "type, data");
    SV *data = ST(1);
    DCpointer RETVAL = NULL; // = safemalloc(1);
    //~ warn("RETVAL should be %d bytes", _sizeof(aTHX_ type));
    RETVAL = sv2ptr(aTHX_ ST(0), data, RETVAL, false);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Pointer_unmarshal) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "pointer, type");
    SV *RETVAL;
    DCpointer ptr;
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("pointer is not of type Affix::Pointer");
    RETVAL = ptr2sv(aTHX_ ptr, ST(0));
    RETVAL = sv_2mortal(RETVAL);
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_plus) {
    dVAR;
    dXSARGS;
    if (UNLIKELY(items != 3)) croak_xs_usage(cv, "ptr, other, swap");
    DCpointer ptr;
    IV other = (IV)SvIV(ST(1));
    // IV swap = (IV)SvIV(ST(2));
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(0)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp) + other);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", ptr);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_minus) {
    dVAR;
    dXSARGS;
    if (UNLIKELY(items != 3)) croak_xs_usage(cv, "ptr, other, swap");
    DCpointer ptr;
    IV other = (IV)SvIV(ST(1));
    //~ IV swap = (IV)SvIV(ST(2));
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(0)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp) - other);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", ptr);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_as_string) {
    dVAR;
    dXSARGS;
    if (items < 1) croak_xs_usage(cv, "ptr, ...");
    {
        char *RETVAL;
        dXSTARG;
        DCpointer ptr;

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            ptr = INT2PTR(DCpointer, tmp);
        }
        else
            croak("ptr is not of type Affix::Pointer");
        RETVAL = (char *)ptr;
        sv_setpv(TARG, RETVAL);
        XSprePUSH;
        PUSHTARG;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_raw) {
    dVAR;
    dXSARGS;
    if (items < 2 || items > 3) croak_xs_usage(cv, "ptr, size[, utf8]");
    {
        SV *RETVAL;
        size_t size = (size_t)SvUV(ST(1));
        bool utf8;

        if (items < 3)
            utf8 = false;
        else { utf8 = (bool)SvTRUE(ST(2)); }
        {
            DCpointer ptr;
            if (sv_derived_from(ST(0), "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(ST(0)));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else if (SvIOK(ST(0))) {
                IV tmp = SvIV((SV *)(ST(0)));
                ptr = INT2PTR(DCpointer, tmp);
            }
            else
                croak("dest is not of type Affix::Pointer");
            RETVAL = newSVpvn_utf8((const char *)ptr, size, utf8 ? 1 : 0);
        }
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Pointer_DumpHex) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "ptr, size");
    size_t size = (size_t)SvUV(ST(1));
    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        DCpointer ptr;
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
        // Gathers perl caller() info
#ifdef USE_ITHREADS
        _DumpHex(aTHX_ ptr, size, PL_curcop->cop_file, PL_curcop->cop_line);
#else
        if (PL_curcop) { // Gathers perl caller() info
            const COP *cop = Perl_closest_cop(aTHX_ PL_curcop, OpSIBLING(PL_curcop), PL_op, FALSE);
            if (!cop) cop = PL_curcop;
            if (CopLINE(cop)) {
                _DumpHex(aTHX_ ptr, size, OutCopFILE(cop), CopLINE(cop));
                XSRETURN_EMPTY;
            }
        }
        DumpHex(ptr, size);
#endif
        XSRETURN_EMPTY;
    }
    else
        croak("ptr is not of type Affix::Pointer");
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_Pointer_DESTROY) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "ptr");
    DCpointer ptr;
    if (UNLIKELY(sv_derived_from(ST(0), "Affix::Pointer::Unmanaged")))
        ;
    else if (LIKELY(sv_derived_from(ST(0), "Affix::Pointer"))) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
        if (ptr != NULL) {
            safefree(ptr);
            ptr = NULL;
        }
    }
    else
        croak("ptr is not of type Affix::Pointer");
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

XS_INTERNAL(Affix_sizeof) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "type");
    size_t RETVAL;
    dXSTARG;
    SV *type = ST(0);
    RETVAL = _sizeof(aTHX_ type);
    XSprePUSH;
    PUSHu((UV)RETVAL);
    XSRETURN(1);
}

XS_INTERNAL(Affix_offsetof) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "type, field");
    size_t RETVAL = 0;
    dXSTARG;
    SV *type = ST(0);
    char *field = (char *)SvPV_nolen(ST(1));
    {
        if (sv_isobject(type) && (sv_derived_from(type, "Affix::Type::Struct"))) {
            HV *href = MUTABLE_HV(SvRV(type));
            SV **fields_ref = hv_fetch(href, "fields", 6, 0);
            AV *fields = MUTABLE_AV(SvRV(*fields_ref));
            size_t field_count = av_count(fields);
            for (size_t i = 0; i < field_count; ++i) {
                AV *av_field = MUTABLE_AV(SvRV(*av_fetch(fields, i, 0)));
                SV *sv_field = *av_fetch(av_field, 0, 0);
                char *this_field = SvPV_nolen(sv_field);
                if (!strcmp(this_field, field)) {
                    RETVAL = _offsetof(aTHX_ * av_fetch(av_field, 1, 0));
                    break;
                }
                if (i == field_count)
                    croak("Given structure does not contain field named '%s'", field);
            }
        }
        else
            croak("Given type is not a structure");
    }
    XSprePUSH;
    PUSHu((UV)RETVAL);
    XSRETURN(1);
}

XS_INTERNAL(Affix_malloc) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "size");

    DCpointer RETVAL;
    size_t size = (size_t)SvUV(ST(0));
    {
        RETVAL = safemalloc(size);
        if (RETVAL == NULL) XSRETURN_EMPTY;
    }
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_calloc) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "num, size");

    DCpointer RETVAL;
    size_t num = (size_t)SvUV(ST(0));
    size_t size = (size_t)SvUV(ST(1));
    {
        RETVAL = safecalloc(num, size);
        if (RETVAL == NULL) XSRETURN_EMPTY;
    }
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_realloc) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "ptr, size");
    DCpointer ptr;
    size_t size = (size_t)SvUV(ST(1));
    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    ptr = saferealloc(ptr, size);
    sv_setref_pv(ST(0), "Affix::Pointer:Unmanaged", ptr);
    SvSETMAGIC(ST(0));
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", ptr);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_free) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "ptr");
    SP -= items;

    DCpointer ptr;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    {
        if (ptr) {
            sv_set_undef(ST(0));
            SvSETMAGIC(ST(0));
        }
        sv_set_undef(ST(0));
        SvSETMAGIC(ST(0));
    } // Let Affix::Pointer::DESTROY take care of the rest
    PUTBACK;
    XSRETURN(1);
}

XS_INTERNAL(Affix_memchr) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "ptr, ch, count");
    {
        DCpointer RETVAL;
        DCpointer ptr;
        char ch = (char)*SvPV_nolen(ST(1));
        size_t count = (size_t)SvUV(ST(2));

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            ptr = INT2PTR(DCpointer, tmp);
        }
        else
            croak("ptr is not of type Affix::Pointer");

        RETVAL = memchr(ptr, ch, count);
        {
            SV *RETVALSV;
            RETVALSV = sv_newmortal();
            sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
            ST(0) = RETVALSV;
        }
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memcmp) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "lhs, rhs, count");
    {
        int RETVAL;
        dXSTARG;
        size_t count = (size_t)SvUV(ST(2));
        DCpointer lhs, rhs;
        {
            if (sv_derived_from(ST(0), "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(ST(0)));
                lhs = INT2PTR(DCpointer, tmp);
            }
            else if (SvIOK(ST(0))) {
                IV tmp = SvIV((SV *)(ST(0)));
                lhs = INT2PTR(DCpointer, tmp);
            }
            else
                croak("ptr is not of type Affix::Pointer");
            if (sv_derived_from(ST(1), "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(ST(1)));
                rhs = INT2PTR(DCpointer, tmp);
            }
            else if (SvIOK(ST(1))) {
                IV tmp = SvIV((SV *)(ST(1)));
                rhs = INT2PTR(DCpointer, tmp);
            }
            else if (SvPOK(ST(1))) { rhs = (DCpointer)(U8 *)SvPV_nolen(ST(1)); }
            else
                croak("dest is not of type Affix::Pointer");
            RETVAL = memcmp(lhs, rhs, count);
        }
        XSprePUSH;
        PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memset) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "dest, ch, count");
    {
        DCpointer RETVAL;
        DCpointer dest;
        char ch = (char)*SvPV_nolen(ST(1));
        size_t count = (size_t)SvUV(ST(2));

        if (sv_derived_from(ST(0), "Affix::Pointer")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            dest = INT2PTR(DCpointer, tmp);
        }
        else
            croak("dest is not of type Affix::Pointer");

        RETVAL = memset(dest, ch, count);
        {
            SV *RETVALSV;
            RETVALSV = sv_newmortal();
            sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
            ST(0) = RETVALSV;
        }
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memcpy) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "dest, src, nitems");
    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src, RETVAL;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Affix::Pointer");
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(1))) {
        IV tmp = SvIV((SV *)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvPOK(ST(1))) { src = (DCpointer)(U8 *)SvPV_nolen(ST(1)); }
    else
        croak("dest is not of type Affix::Pointer");
    RETVAL = CopyD(src, dest, nitems, char);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memmove) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "dest, src, nitems");

    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src, RETVAL;

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(0))) {
        IV tmp = SvIV((SV *)(ST(0)));
        dest = INT2PTR(DCpointer, tmp);
    }
    else
        croak("dest is not of type Affix::Pointer");
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvIOK(ST(1))) {
        IV tmp = SvIV((SV *)(ST(1)));
        src = INT2PTR(DCpointer, tmp);
    }
    else if (SvPOK(ST(1))) { src = (DCpointer)(U8 *)SvPV_nolen(ST(1)); }
    else
        croak("dest is not of type Affix::Pointer");

    RETVAL = MoveD(src, dest, nitems, char);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_strdup) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "str1");

    DCpointer RETVAL;
    char *str1 = (char *)SvPV_nolen(ST(0));

    RETVAL = strdup(str1);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer::Unmanaged", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

//~ SV *
//~ FETCH(HV *spy, SV *key)
//~ PREINIT:
//~ HE *he;
//~ CODE:
//~ he = hv_fetch_ent(spy, key, 0, 0);
//~ RETVAL = (he ? newSVsv(hv_iterval(spy, he)) : &PL_sv_undef);
//~ OUTPUT:
//~ RETVAL

// Bootstap

#define EXT_TYPE(NAME, AFFIX_CHAR, DC_CHAR)                                                        \
    {                                                                                              \
        set_isa("Affix::Type::" #NAME, "Affix::Type::Base");                                       \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::" #NAME, Affix_Type_##NAME, file, "$");               \
            XSANY.any_i32 = (int)AFFIX_CHAR;                                                       \
        }                                                                                          \
        export_function("Affix", #NAME, "types");                                                  \
        /* Overload magic: */                                                                      \
        sv_setsv(get_sv("Affix::Type::" #NAME "::()", TRUE), &PL_sv_yes);                          \
        /* overload as sigchars with fallbacks */                                                  \
        cv = newXSproto_portable("Affix::Type::" #NAME "::()", Affix_Type_asint, file, "$");       \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::({", Affix_Type_asint, file, "$");       \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv =                                                                                       \
            newXSproto_portable("Affix::Type::" #NAME "::(function", Affix_Type_asint, file, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(\"\"", Affix_Type_asint, file, "$");    \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(*/}", Affix_Type_asint, file, "$");     \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(defined", Affix_Type_asint, file, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(here", Affix_Type_asint, file, "$");    \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(/*", Affix_Type_asint, file, "$");      \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
    }

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
        /* Overload magic: */                                                                      \
        sv_setsv(get_sv("Affix::Type::" #NAME "::()", TRUE), &PL_sv_yes);                          \
        /* overload as sigchars with fallbacks */                                                  \
        cv = newXSproto_portable("Affix::Type::" #NAME "::()", Affix_Type_asint, file, "$");       \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::({", Affix_Type_asint, file, "$");       \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv =                                                                                       \
            newXSproto_portable("Affix::Type::" #NAME "::(function", Affix_Type_asint, file, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(\"\"", Affix_Type_asint, file, "$");    \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(*/}", Affix_Type_asint, file, "$");     \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(defined", Affix_Type_asint, file, "$"); \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(here", Affix_Type_asint, file, "$");    \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(/*", Affix_Type_asint, file, "$");      \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
    }

#define CC_TYPE(NAME, DC_CHAR)                                                                     \
    {                                                                                              \
        set_isa("Affix::Type::CC::" #NAME, "Affix::Type::CC");                                     \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::CC_" #NAME, Affix_CC_##NAME, file, "");               \
            XSANY.any_i32 = (int)DC_CHAR;                                                          \
        }                                                                                          \
        export_function("Affix", "CC_" #NAME, "types");                                            \
        export_function("Affix", "CC_" #NAME, "cc");                                               \
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable("Affix::Type::CC::" #NAME "::(\"\"", Affix_Type_asint, file,      \
                                 ";$");                                                            \
        XSANY.any_i32 = (int)DC_SIGCHAR_CC_PREFIX;                                                 \
        /* Making a sub named "Affix::Type::Int::()" allows the package */                         \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Type::Int") to return true. */                             \
        (void)newXSproto_portable("Affix::Type::CC::" #NAME "::()", Affix_Type_asint, file, ";$"); \
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
    EXT_TYPE(Pointer, AFFIX_ARG_CPOINTER, AFFIX_ARG_CPOINTER);

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

    (void)newXSproto_portable("Affix::load_lib", Affix_load_lib, file, "$;$");
    export_function("Affix", "load_lib", "lib");
    (void)newXSproto_portable("Affix::Lib::list_symbols", Affix_Lib_list_symbols, file, "$");
    export_function("Affix", "load_lib", "lib");

    (void)newXSproto_portable("Affix::pin", Affix_pin, file, "$$$$");
    export_function("Affix", "pin", "default");
    (void)newXSproto_portable("Affix::unpin", Affix_unpin, file, "$");
    export_function("Affix", "unpin", "default");
    //
    cv = newXSproto_portable("Affix::affix", Affix_affix, file, "$$@$");
    XSANY.any_i32 = 0;
    export_function("Affix", "affix", "default");
    cv = newXSproto_portable("Affix::wrap", Affix_affix, file, "$$@$");
    XSANY.any_i32 = 1;
    export_function("Affix", "wrap", "all");
    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, file, "$");

    //~ (void)newXSproto_portable("Affix::typedef", XS_Affix_typedef, file, "$$");
    //~ (void)newXSproto_portable("Affix::CLONE", XS_Affix_CLONE, file, ";@");

    // Utilities
    (void)newXSproto_portable("Affix::sv_dump", Affix_sv_dump, file, "$");

    (void)newXSproto_portable("Affix::typedef", Affix_typedef, file, "$$");
    export_function("Affix", "typedef", "all");

    (void)newXSproto_portable("Affix::Type::Pointer::marshal", Affix_Type_Pointer_marshal, file,
                              "$$");
    (void)newXSproto_portable("Affix::Type::Pointer::unmarshal", Affix_Type_Pointer_unmarshal, file,
                              "$$");
    (void)newXSproto_portable("Affix::Type::Pointer::(|", Affix_Type_Pointer_RW, file, "");

    //~ (void)newXSproto_portable("Affix::_shutdown", XS_Affix__shutdown, file, "");

    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(get_sv("Affix::Pointer::()", TRUE), &PL_sv_yes);
    /* Making a sub named "Affix::Pointer::()" allows the package */
    /* to be findable via fetchmethod(), and causes */
    /* overload::Overloaded("Affix::Pointer") to return true. */
    (void)newXS_deffile("Affix::Pointer::()", Affix_Pointer_as_string);
    (void)newXSproto_portable("Affix::Pointer::plus", Affix_Pointer_plus, file, "$$$");
    (void)newXSproto_portable("Affix::Pointer::(+", Affix_Pointer_plus, file, "$$$");
    (void)newXSproto_portable("Affix::Pointer::minus", Affix_Pointer_minus, file, "$$$");
    (void)newXSproto_portable("Affix::Pointer::(-", Affix_Pointer_minus, file, "$$$");
    (void)newXSproto_portable("Affix::Pointer::as_string", Affix_Pointer_as_string, file, "$;@");
    (void)newXSproto_portable("Affix::Pointer::(\"\"", Affix_Pointer_as_string, file, "$;@");
    (void)newXSproto_portable("Affix::Pointer::raw", Affix_Pointer_raw, file, "$$;$");
    (void)newXSproto_portable("Affix::Pointer::dump", Affix_Pointer_DumpHex, file, "$$");
    (void)newXSproto_portable("Affix::DumpHex", Affix_Pointer_DumpHex, file, "$$");
    (void)newXSproto_portable("Affix::Pointer::DESTROY", Affix_Pointer_DESTROY, file, "$");
    set_isa("Affix::Pointer::Unmanaged", "Affix::Pointer");
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

    //~ //
    //~ TYPE(Class, AFFIX_ARG_CLASS, AFFIX_ARG_CSTRUCT);
    //~ TYPE(Method, AFFIX_ARG_CLASS_METHOD, AFFIX_ARG_CALLBACK);

    //~ // Enum[]?
    //~ export_function("Affix", "typedef", "types");

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

    export_constant_char("Affix", "ABI_C", "abi", AFFIX_ABI_C);
    export_constant_char("Affix", "ABI_ITANIUM", "abi", AFFIX_ABI_ITANIUM);
    export_constant_char("Affix", "ABI_GCC", "abi", AFFIX_ABI_GCC);
    export_constant_char("Affix", "ABI_MSVC", "abi", AFFIX_ABI_MSVC);
    export_constant_char("Affix", "ABI_RUST", "abi", AFFIX_ABI_RUST);
    export_constant_char("Affix", "ABI_SWIFT", "abi", AFFIX_ABI_SWIFT);
    export_constant_char("Affix", "ABI_D", "abi", AFFIX_ABI_D);
    //~ }

    //~ {

    (void)newXSproto_portable("Affix::sizeof", Affix_sizeof, file, "$");
    export_function("Affix", "sizeof", "default");
    (void)newXSproto_portable("Affix::offsetof", Affix_offsetof, file, "$$");
    export_function("Affix", "offsetof", "default");
    (void)newXSproto_portable("Affix::malloc", Affix_malloc, file, "$");
    export_function("Affix", "malloc", "memory");
    (void)newXSproto_portable("Affix::calloc", Affix_calloc, file, "$$");
    export_function("Affix", "calloc", "memory");
    (void)newXSproto_portable("Affix::realloc", Affix_realloc, file, "$$");
    export_function("Affix", "realloc", "memory");
    (void)newXSproto_portable("Affix::free", Affix_free, file, "$");
    export_function("Affix", "free", "memory");
    (void)newXSproto_portable("Affix::memchr", Affix_memchr, file, "$$$");
    export_function("Affix", "memchr", "memory");
    (void)newXSproto_portable("Affix::memcmp", Affix_memcmp, file, "$$$");
    export_function("Affix", "memcmp", "memory");
    (void)newXSproto_portable("Affix::memset", Affix_memset, file, "$$$");
    export_function("Affix", "memset", "memory");
    (void)newXSproto_portable("Affix::memcpy", Affix_memcpy, file, "$$$");
    export_function("Affix", "memcpy", "memory");
    (void)newXSproto_portable("Affix::memmove", Affix_memmove, file, "$$$");
    export_function("Affix", "memmove", "memory");
    (void)newXSproto_portable("Affix::strdup", Affix_strdup, file, "$");
    export_function("Affix", "strdup", "memory");
    //~ set_isa("Affix::Pointer", "Dyn::Call::Pointer");
    //~ }

    //~ cv = newXSproto_portable("Affix::hit_it", XS_Affix_hit_it, file, "$$$;$");
    //~ XSANY.any_i32 = 0;
    //~ cv = newXSproto_portable("Affix::wrap_it", XS_Affix_hit_it, file, "$$$;$");
    //~ XSANY.any_i32 = 1;
    //~ (void)newXSproto_portable("Affix::NoThanks::call", XS_Affix_hit_call, file, "$;@");
    //~ (void)newXSproto_portable("Affix::NoThanks::DESTROY", XS_Affix_hit_DESTROY, file, "$");

    (void)newXSproto_portable("Affix::AggregateBase::FETCH", Affix_Aggregate_FETCH, file, "$$");
    (void)newXSproto_portable("Affix::AggregateBase::EXISTS", Affix_Aggregate_EXISTS, file, "$$");
    (void)newXSproto_portable("Affix::AggregateBase::FIRSTKEY", Affix_Aggregate_FIRSTKEY, file,
                              "$");
    (void)newXSproto_portable("Affix::AggregateBase::NEXTKEY", Affix_Aggregate_NEXTKEY, file, "$$");
    set_isa("Affix::Struct", "Affix::AggregateBase");
    set_isa("Affix::Union", "Affix::AggregateBase");

    (void)newXSproto_portable("Affix::END", Affix_END, file, "");

    Perl_xs_boot_epilog(aTHX_ ax);
}

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
    case AFFIX_ARG_VOID: {
        sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", ptr);
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
