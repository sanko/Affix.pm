#ifdef __cplusplus
extern "C" {
#endif

#define PERL_NO_GET_CONTEXT 1 /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

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
#define PING warn("Ping at %s line %d", __FILE__, __LINE__);

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

#include "marshal.h"

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
        return DC_SIGCHAR_POINTER;
    default:
        return -1;
    }
}

/* Useful but undefined in perlapi */
#define FLOAT_SIZE sizeof(float)
#define BOOL_SIZE sizeof(bool)         // ha!
#define DOUBLE_SIZE sizeof(double)     // ugh...
#define INTPTR_T_SIZE sizeof(intptr_t) // ugh...
#define WCHAR_T_SIZE sizeof(wchar_t)

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
void export_constant(const char *package, const char *name, const char *_tag, double val) {
    dTHX;
    register_constant(package, name, newSVnv(val));
    export_function(package, name, _tag);
}

void set_isa(const char *klass, const char *parent) {
    dTHX;
    HV *parent_stash = gv_stashpv(parent, GV_ADD | GV_ADDMULTI);
    av_push(get_av(form("%s::ISA", klass), TRUE), newSVpv(parent, 0));
}

#define DumpHex(addr, len) _DumpHex(aTHX_ addr, len, __FILE__, __LINE__)

void _DumpHex(pTHX_ const void *addr, size_t len, const char *file, int line) {
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
    case AFFIX_ARG_CARRAY:
        // sv_dump(type);
        // DD(type);
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
        HV *hv_type = MUTABLE_HV(SvRV(type));
        SV **agg_ = hv_fetch(hv_type, "aggregate", 9, 0);
        if (agg_ != NULL) {
            SV *agg = *agg_;
            if (sv_derived_from(agg, "Affix::Aggregate")) {
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
            if (t == AFFIX_ARG_CSTRUCT) {
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
                int _t = SvIV(*type_ptr);
                switch (_t) {
                case AFFIX_ARG_CPPSTRUCT:
                case AFFIX_ARG_CSTRUCT:
                case AFFIX_ARG_CUNION: {
                    DCaggr *child = _aggregate(aTHX_ * type_ptr);
                    dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                } break;
                case AFFIX_ARG_CARRAY: {
                    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "type", 0);
                    int array_len = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "size", 0));
                    char *str = SvPVbytex_nolen(type);
                    dcAggrField(agg, type_as_dc(_t), offset, array_len);
                } break;
                default: {
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
    return NULL;
}

// Callback system
char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata) {
    Callback *cbx = (Callback *)userdata;
    dTHXa(cbx->perl);
    dSP;
    int count;
    char ret_c = cbx->ret;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    EXTEND(SP, cbx->sig_len);
    char type;
    for (size_t i = 0; i < cbx->sig_len; ++i) {
        type = cbx->sig[i];
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
            DCpointer ptr = dcbArgPointer(args);
            SV *__type = *av_fetch(cbx->args, i, 0);
            char *_type = SvPV_nolen(__type);
            //~ switch (_type[0]) { // true type
            //~ case DC_SIGCHAR_ANY: {
            //~ SV *s = ptr2sv(aTHX_ ptr, __type);
            //~ mPUSHs(s);
            //~ } break;
            //~ case DC_SIGCHAR_CODE: {
            //~ Callback *cb = (Callback *)dcbGetUserData((DCCallback *)ptr);
            //~ mPUSHs(cb->cv);
            //~ } break;
            //~ default:
            //~ mPUSHs(sv_setref_pv(newSV(1), "Affix::Pointer", ptr));
            //~ break;
            //~ }
        } break;
        case AFFIX_ARG_ASCIISTR: {
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
        //~ case DC_SIGCHAR_STRING:
        //~ result->Z = SvPOK(ret) ? SvPVx_nolen_const(ret) : NULL;
        //~ break;
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

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv);
void sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed);

typedef struct { // Used in CUnion and pin()
    void *ptr;
    SV *type_sv;
} var_ptr;

int get_union(pTHX_ SV *sv, MAGIC *mg) {
    warn("get union");
    //~ sv_dump(sv);
    //~ PING;
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    //~ PING;

    //~ sv_dump(ptr->type_sv);
    //~ DumpHex(ptr->ptr, 16);
    //~ PING;

    SV *val = ptr2sv(aTHX_ ptr->ptr, ptr->type_sv);
    //~ PING;

    sv_setsv((sv), val);
    //~ PING;

    return 0;
}

int set_union(pTHX_ SV *sv, MAGIC *mg) {
    warn("set union");
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    if (SvOK(sv)) sv2ptr(aTHX_ ptr->type_sv, sv, ptr->ptr, false);
    return 0;
}

int free_union(pTHX_ SV *sv, MAGIC *mg) {
    croak("free union");
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    sv_2mortal(ptr->type_sv);
    safefree(ptr);
    return 0;
}

static MGVTBL union_vtbl = {
    get_union,  // get
    set_union,  // set
    NULL,       // len
    NULL,       // clear
    free_union, // free
    NULL,       // copy
    NULL,       // dup
    NULL        // local
};

char *locate_lib(aTHX_ char *lib, int ver) {
    // Use perl to get the actual path to the library
    dSP;
    int count;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    mXPUSHp(lib, strlen(lib));
    if (ver) mXPUSHn(ver);
    PUTBACK;
    count = call_pv("Affix::locate_lib", G_SCALAR);
    SPAGAIN;
    if (count == 1) {
        SV *ret = POPs;
        if (SvOK(ret)) strcpy(lib, SvPVx_nolen(ret));
    }
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
int get_pin(pTHX_ SV *sv, MAGIC *mg) {
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    SV *val = ptr2sv(aTHX_ ptr->ptr, ptr->type_sv);
    sv_setsv((sv), val);
    return 0;
}

int set_pin(pTHX_ SV *sv, MAGIC *mg) {
    warn("set");
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    if (SvOK(sv)) sv2ptr(aTHX_ ptr->type_sv, sv, ptr->ptr, false);
    return 0;
}

int free_pin(pTHX_ SV *sv, MAGIC *mg) {
    warn("free");
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
        char *_libpath = locate_lib(SvPV_nolen(ST(1)), 0);
        lib =
#if defined(_WIN32) || defined(_WIN64)
            dlLoadLibrary(_libpath);
#else
            (DLLib *)dlopen(_libpath, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (lib == NULL) {
            char *reason = dlerror();
            croak("Failed to load %s: %s", _libpath, reason);
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
SIMPLE_TYPE(Size_t);
SIMPLE_TYPE(SSize_t);
SIMPLE_TYPE(Float);
SIMPLE_TYPE(Double);
SIMPLE_TYPE(Str);
SIMPLE_TYPE(WStr);

XS_INTERNAL(Affix_Type_Pointer) {
    dXSARGS;
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    bool rw = false;
    switch (av_count(fields)) {
    case 2: {
        SV *inside;
        SV **rw_ref = av_fetch(fields, 1, 0);
        rw = SvTRUE(*rw_ref);
    } // fall through
    case 1: {
        SV *inside;
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

XS_INTERNAL(Affix_Type_RWPointer) {
    croak("No.");
}
XS_INTERNAL(Affix_Type_Pointer_RW) {
    dXSARGS;
    if (!(sv_isobject(ST(0)) && sv_derived_from(ST(0), "Affix::Type::Pointer")))
        croak("... | RW expects a subclass of Affix::Type::Pointer to the left");
    ST(0) = sv_bless(ST(0), gv_stashpv("Affix::Type::RWPointer", GV_ADD));
    SvSETMAGIC(ST(0));

    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_CodeRef) {
    dXSARGS;
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    if (av_count(fields) == 2) {
        char *sig;
        size_t field_count;
        {
            SV *arg_ptr = *av_fetch(fields, 0, 0);
            AV *args = MUTABLE_AV(SvRV(arg_ptr));
            size_t field_count = av_count(args);
            Newxz(sig, field_count + 2, char);
            for (int i = 0; i < field_count; ++i) {
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
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    size_t fields_count = av_count(fields);
    if (fields_count == 2) {
        AV *type_size = MUTABLE_AV(SvRV(ST(1)));
        SV *type, *size;
        size_t array_length, array_sizeof = 0;
        bool packed = false;
        {
            type = *av_fetch(fields, 0, 0);
            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                croak("ArrayRef[...] expects a type that is a subclass of Affix::Type::Base");
            hv_stores(RETVAL_HV, "type", SvREFCNT_inc(type));
        }
        size_t type_alignof = _alignof(aTHX_ type);

        if (fields_count == 2) {
            array_length = SvUV(*av_fetch(fields, 1, 0));
            size_t offset = 0;
            size_t type_sizeof = _sizeof(aTHX_ type);
            for (int i = 0; i < array_length; ++i) {
                array_sizeof += type_sizeof;
                array_sizeof += packed ? 0
                                       : padding_needed_for(array_sizeof, type_alignof > type_sizeof
                                                                              ? type_sizeof
                                                                              : type_alignof);
                offset = array_sizeof;
            }
            size = newSVuv(array_length);
        }
        else { size = newSV(0); }
        hv_stores(RETVAL_HV, "sizeof", newSVuv(array_sizeof));
        hv_stores(RETVAL_HV, "align", newSVuv(type_alignof));
        hv_stores(RETVAL_HV, "size", size);
        hv_stores(RETVAL_HV, "name", newSV(0));
        hv_stores(RETVAL_HV, "packed", boolSV(packed));
        ST(0) = sv_2mortal(sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)),
                                    gv_stashpv("Affix::Type::ArrayRef", GV_ADD)));
    }
    else
        croak("ArrayRef[...] expects a type and size. e.g ArrayRef[Int, 50]");
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Struct) {
    dXSARGS;
    HV *RETVAL_HV = newHV();
    AV *fields_in = MUTABLE_AV(SvRV(ST(0)));
    size_t field_count = av_count(fields_in);

    if (!(field_count % 2)) {
        bool packed = false; // TODO: handle packed structs correctly
        hv_stores(RETVAL_HV, "packed", boolSV(packed));
        AV *fields = newAV();
        size_t field_count = av_count(fields_in), size = 0;
        for (int i = 0; i < field_count; i += 2) {
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
    HV *RETVAL_HV = newHV();
    AV *fields_in = MUTABLE_AV(SvRV(ST(0)));
    size_t field_count = av_count(fields_in);
    if (!(field_count % 2)) {
        bool packed = false; // TODO: handle packed structs correctly
        hv_stores(RETVAL_HV, "packed", boolSV(packed));
        AV *fields = newAV();
        size_t field_count = av_count(fields_in), size = 0, _align = 0;
        for (int i = 0; i < field_count; i += 2) {
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

XS_INTERNAL(Affix_load_lib) {
    dVAR;
    dXSARGS;
    if (items < 1 || items > 2) croak_xs_usage(cv, "lib_name, version");
    char *_libpath = locate_lib(SvPV_nolen(ST(0)), SvIOK(ST(1)) ? SvIV(ST(1)) : 0);
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

/* Affix::affix(...) and Affix::wrap(...) System */
struct CallBody {
    int16_t call_conv;
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

    CallBody *ptr = (CallBody *)XSANY.any_ptr;
    char **free_strs = NULL;
    void **free_ptrs = NULL;

    size_t num_args = ptr->num_args, num_strs = 0, num_ptrs = 0, i;
    int16_t *arg_types = ptr->arg_types;
    bool void_ret = false;

    //~ dcMode(MY_CXT.cvm, ptr->call_conv);
    dcReset(MY_CXT.cvm);

    if (UNLIKELY(items != num_args)) {
        if (UNLIKELY(items > num_args)) croak("Too many arguments");
        croak("Not enough arguments");
    }
    for (i = 0; LIKELY(i < num_args); ++i) {
        //~ warn("%d of %d == %s (%d)", i + 1, num_args, type_as_str(ptr->arg_types[i]),
        //~ (arg_types[i] & AFFIX_ARG_TYPE_MASK));
        //~ sv_dump(ST(i));
        switch (arg_types[i]) {
        case AFFIX_ARG_VOID: // skip?
            break;
        case AFFIX_ARG_BOOL:
            dcArgBool(MY_CXT.cvm, SvTRUE(ST(i))); // Anything can be a bool
            break;
        case AFFIX_ARG_CHAR:
            dcArgChar(MY_CXT.cvm, (char)(SvIOK(ST(i)) ? SvIV(ST(i)) : *SvPV_nolen(ST(i))));
            break;
        case AFFIX_ARG_UCHAR:
            dcArgChar(MY_CXT.cvm, (unsigned char)(SvIOK(ST(i)) ? SvUV(ST(i)) : *SvPV_nolen(ST(i))));
            break;
        case AFFIX_ARG_WCHAR: {
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
            PING;
            if (!free_ptrs) free_ptrs = (void **)safemalloc(num_args * sizeof(void *));
            size_t size = _sizeof(aTHX_ newSViv(AFFIX_ARG_UTF16STR));
            PING;
            warn("size: %d", size);
            // Newxz(free_ptrs[num_ptrs], size, char);

            PING;
            Newxz(free_ptrs[num_ptrs], size, char);

            sv2ptr(aTHX_ newSViv(AFFIX_ARG_UTF16STR), ST(i), free_ptrs[num_ptrs], false);
            PING;
            dcArgPointer(MY_CXT.cvm, *(void **)(free_ptrs[num_ptrs]));
            PING;
            num_ptrs++;
        } break;
        case AFFIX_ARG_CSTRUCT: {
            if (!SvOK(ST(i)) && SvREADONLY(ST(i)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) free_ptrs = (void **)safemalloc(num_args * sizeof(void *));
                if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVHV)
                    croak("Type of arg %lu must be an hash ref", i + 1);
                AV *elements = MUTABLE_AV(SvRV(ST(i)));
                SV **type = av_fetch(ptr->arg_info, i, 0);
                size_t size = _sizeof(aTHX_ SvRV(*type));
                Newxz(free_ptrs[num_ptrs], size, char);
                DCaggr *agg = _aggregate(aTHX_ SvRV(*type));
                sv2ptr(aTHX_ SvRV(*type), ST(i), free_ptrs[num_ptrs], false);
                dcArgAggr(MY_CXT.cvm, agg, free_ptrs[num_ptrs]);
                num_ptrs++;
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
                    croak("Type of arg %lu must be an array ref", i + 1);
                AV *elements = MUTABLE_AV(SvRV(ST(i)));
                SV **type = av_fetch(ptr->arg_info, i, 0);
                HV *hv_ptr = MUTABLE_HV(SvRV(SvRV(*type)));
                SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
                SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);

                //~ SV **ptr_ptr =
                //~ hv_exists(hv_ptr, "pointer", 7) ?
                //~ hv_fetchs(hv_ptr, "pointer", 0)
                //~ newSvRV(newSV(0));
                size_t av_len;
                if (SvOK(*size_ptr)) {
                    av_len = SvIV(*size_ptr);
                    if (av_count(elements) != av_len)
                        croak("Expected an array of %lu elements; found %zd", av_len,
                              av_count(elements));
                }
                else
                    av_len = av_count(elements);
                //~ hv_stores(hv_ptr, "sizeof", newSViv(av_len));
                size_t size = _sizeof(aTHX_(SvRV(*type)));
                Newxz(free_ptrs[num_ptrs], av_len * size, char);
                sv2ptr(aTHX_ SvRV(*type), ST(num_ptrs), free_ptrs[num_ptrs], false);
                dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                num_ptrs++;
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
                PING;

                dcArgPointer(MY_CXT.cvm, hold->cb);
                PING;
            }
            else
                dcArgPointer(MY_CXT.cvm, NULL);

            dcArgPointer(MY_CXT.cvm, NULL);
        } break;
        case AFFIX_ARG_CPOINTER: {
            if (!free_ptrs) free_ptrs = (void **)safemalloc(num_args * INTPTR_T_SIZE);
            //~ warn("AFFIX_ARG_CPOINTER [%d,%d]", i,
            //~ arg_types[i] & AFFIX_ARG_TYPE_MASK & AFFIX_ARG_RW_MASK);
            SV **type = av_fetch(ptr->arg_info, i, 0);
            if (type == NULL) croak("No idea");

            switch (SvIV(*type)) {
            case AFFIX_ARG_VOID: {
                if (UNLIKELY(!SvOK(ST(i)) && SvREADONLY(ST(i)))) { // explicit undef
                    dcArgPointer(MY_CXT.cvm, NULL);
                }
                else if (LIKELY(sv_isobject(ST(i)) && sv_derived_from(ST(i), "Affix::Pointer"))) {
                    IV tmp = SvIV(SvRV(ST(i)));
                    dcArgPointer(MY_CXT.cvm, INT2PTR(DCpointer, tmp));
                }
                else {
                    if (sv_isobject(ST(i))) croak("Unexpected pointer to blessed object");
                    free_ptrs[num_ptrs] = safemalloc(INTPTR_T_SIZE);
                    sv2ptr(aTHX_ * type, ST(i), free_ptrs[num_ptrs], false);
                    dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                    num_ptrs++;
                }
            } break;
            case AFFIX_ARG_CUNION: {
                DCpointer ptr;
                const MAGIC *mg = SvTIED_mg((SV *)SvRV(ST(i)), PERL_MAGIC_tied);
                if (LIKELY(SvOK(ST(i)) && SvTYPE(SvRV(ST(i))) == SVt_PVHV && mg
                           //~ &&  sv_derived_from(SvRV(ST(i)), "Affix::Union")
                           )) { // Already a known union pointer
                    SV *union_ = SvTIED_obj((SV *)SvRV(ST(i)), mg);
                    SV **ptr_ptr = hv_fetchs(MUTABLE_HV(SvRV(union_)), "pointer", 0);
                    {
                        IV tmp = SvIV(SvRV(*ptr_ptr));
                        ptr = INT2PTR(DCpointer, tmp);
                    }
                }
                else {
                    ptr = safemalloc(_sizeof(aTHX_ * type));
                    sv2ptr(aTHX_ * type, ST(i), ptr, false);
                    {
                        SV *RETVAL_ = newSV(0);
                        SV *p = newSV(0);
                        sv_setref_pv(p, "Affix::Pointer", ptr);
                        HV *hash;
                        HV *stash;
                        SV *tie;
                        SV *r = ST(i);
                        hash = newHV();
                        tie = sv_2mortal(newRV_noinc((SV *)newHV()));
                        hv_store((HV *)SvRV(tie), "pointer", 7, p, 0);
                        hv_store((HV *)SvRV(tie), "type", 4, newRV_inc(*type), 0);
                        sv_bless(tie, gv_stashpv("Affix::Union", GV_ADD));
                        hv_magic(hash, (GV *)tie, PERL_MAGIC_tied);
                        r = sv_2mortal(newRV_noinc((SV *)hash));
                        sv_setsv_mg(ST(i), r);
                        SvSETMAGIC(ST(i));
                    }
                }
                dcArgPointer(MY_CXT.cvm, ptr);
                break;
            }
            default: {
                free_ptrs[num_ptrs] = safemalloc(_sizeof(aTHX_ * type));
                sv2ptr(aTHX_ * type, ST(i), free_ptrs[num_ptrs], false);
                dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                num_ptrs++;
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
        RETVAL = newSVuv((unsigned char)dcCallChar(MY_CXT.cvm, ptr->entry_point));
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
        DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
        RETVAL = ptr2sv(aTHX_ p, ptr->ret_info);
    } break;
    case AFFIX_ARG_CPOINTER: {
        PING;
        SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ptr->ret_info)), "type", 0);
        if (sv_derived_from(type, "Affix::Type::Void")) {
            RETVAL = newSV(1);
            DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
            sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", p);
            warn("if ptr: %p", p);
        }
        else {
            DCpointer p = dcCallPointer(MY_CXT.cvm, ptr->entry_point);
            warn("else ptr: %p", p);
            RETVAL = newRV_noinc(ptr2sv(aTHX_ p, type));
        }
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
    if (UNLIKELY(free_ptrs != NULL)) {
        {
            size_t p = 0;
            for (i = 0; LIKELY(i < items); ++i) {
                switch (arg_types[i]) {
                case AFFIX_ARG_CPOINTER: {
                    //~ warn("[i: %d, p: %d]", i, p);
                    //~ sv_setsv_mg(ST(i),
                    //~ ptr2sv(aTHX_ free_ptrs[p], (*av_fetch(ptr->arg_info, i, 0))));
                    //~ SvSETMAGIC(ST(i));
                    ++p;
                } break;
                }
            }
        }

        /*
        if (UNLIKELY(

                                                UNLIKELY((arg_types[i] & AFFIX_ARG_TYPE_MASK) ==
        AFFIX_ARG_CPOINTER)
                        //~ ||

                                                //~ UNLIKELY((arg_types[i] & AFFIX_ARG_TYPE_MASK) ==
        AFFIX_ARG_CSTRUCT)
                                                //~ /*&&
                                                         //~ UNLIKELY((arg_types[i] &
        AFFIX_ARG_RW_MASK) == AFFIX_ARG_RW)* /

                                                //~ && !LIKELY(sv_derived_from(ST(i),
        "Affix::Pointer"))

                                        )) {
                                //~ break;
        warn("*** set a pointer from %d to ST(%d)");
                                SV **type = av_fetch(ptr->arg_info, i, 0);
                                SV *idk = ptr2sv(aTHX_ free_ptrs[i], SvRV(*type));
                                sv_setsv_mg(ST(i), idk);
                                SvSETMAGIC(ST(i));
                        }
            */
        for (i = 0; LIKELY(i < num_ptrs); ++i) {
            safefree(free_ptrs[i]);
        }
        safefree(free_ptrs);
    }

    if (UNLIKELY(void_ret)) XSRETURN_EMPTY;

    ST(0) = sv_2mortal(RETVAL);

    XSRETURN(1);
}

XS_INTERNAL(Affix_affix) {
    dXSARGS;
    dXSI32;
    if (items < 1 || items > 6)
        croak_xs_usage(cv,
                       "lib, symbol, arg_types, ret_type, resolve_lib_name, calling_convention");
    SV *RETVAL;
    CallBody *ret;
    Newx(ret, 1, CallBody);
    DLLib *lib;
    if (!SvOK(ST(0)))
        lib = NULL;
    else if (SvROK(ST(0)) && sv_derived_from(ST(0), "Affix::Lib")) {
        IV tmp = SvIV(MUTABLE_SV(SvRV(ST(0))));
        lib = INT2PTR(DLLib *, tmp);
    }
    else {
        ret->lib_name = locate_lib(SvPV_nolen(ST(0)), 0);
        lib =
#if defined(_WIN32) || defined(_WIN64)
            dlLoadLibrary(ret->lib_name);
#else
            (DLLib *)dlopen(ret->lib_name, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (!lib) {
            char *reason = dlerror();
            croak("Failed to load %s: %s", lib, reason);
        }
    }

    char *name = NULL;
    if (LIKELY(ix != 1)) {
        if (UNLIKELY(SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV)) {
            // [ symbol, rename ]
            AV *av;
            av = (AV *)SvRV(ST(1));
            SV **_sym = av_fetch(av, 0, 0);
            SV **_name = av_fetch(av, 1, 0);
            ret->sym_name = (char *)(SvPOK(*_sym) ? SvPV_nolen(*_sym) : NULL);
            name = (char *)(SvPOK(*_name) ? SvPV_nolen(*_name) : NULL);
        }
        else { name = ret->sym_name = (char *)(SvPOK(ST(1)) ? SvPV_nolen(ST(1)) : NULL); }
    }
    else
        ret->sym_name = (char *)(SvPOK(ST(1)) ? SvPV_nolen(ST(1)) : NULL);

    char *prototype;
    {
        AV *av;
        SV **ssv;
        int x;
        if (LIKELY(SvROK(ST(2)) && (SvTYPE(SvRV(ST(2))) == SVt_PVAV)))
            av = (AV *)SvRV(ST(2));
        else { croak("rv was not an AV ref"); }

        /* is it empty? */
        ret->num_args = av_len(av) + 1;
        Newxz(prototype, ret->num_args, char);
        if (ret->num_args) ret->arg_info = newAV();
        if (ret->num_args < 0) { ret->num_args = 0; }
        else {
            Newxz(ret->arg_types, ret->num_args, int16_t);
            if (ret->arg_types == NULL) { croak("unable to malloc int array"); }
            for (x = 0; x < ret->num_args; ++x) {
                ssv = av_fetch(av, x, 0);
                if (LIKELY(ssv != NULL) && SvROK(*ssv) &&
                    sv_derived_from(*ssv, "Affix::Type::Base")) {
                    switch (SvIV(*ssv)) {
                    case AFFIX_ARG_CPOINTER: {
                        SV *sv = *hv_fetchs(MUTABLE_HV(SvRV(*ssv)), "type", 0);
                        av_store(ret->arg_info, x, sv);
                        break;
                    }
                    case AFFIX_ARG_CARRAY:
                    case AFFIX_ARG_VMARRAY:
                    case AFFIX_ARG_CSTRUCT:
                    case AFFIX_ARG_CALLBACK:
                    case AFFIX_ARG_CUNION:
                    case AFFIX_ARG_CPPSTRUCT: {
                        av_store(ret->arg_info, x, newRV_inc(*ssv));
                    } break;
                    }
                    ret->arg_types[x] = (int16_t)SvIV(*ssv);
                    prototype[x] = '$';
                }
                else {
                    PING;
                    //~ croak("Arg # %d is an invalid or unknown type", x);
                    ret->arg_types[x] = (int16_t)AFFIX_ARG_VOID;
                }
            }
        }
    }
    ret->ret_type = (int16_t)(SvOK(ST(3)) ? SvIV(ST(3)) : AFFIX_ARG_VOID);
    ret->ret_info = newSVsv(ST(3));
    ret->resolve_lib_name = SvOK(ST(4)) ? newSVsv(ST(4)) : newSV(0);
    ret->call_conv = SvOK(ST(5)) ? SvIV(ST(5)) : DC_CALL_C_DEFAULT;
    ret->lib_handle = lib;
    ret->entry_point = dlFindSymbol(ret->lib_handle, ret->sym_name);
    //~ warn("entry_point: %p, sym_name: %s, as: %s, prototype: %s, ix: %d", ret->entry_point,
    //~ ret->sym_name, name, prototype, ix);
    //~ DD(MUTABLE_SV(ret->arg_info));
    STMT_START {
        cv = newXSproto_portable(name, Affix_trigger, file, prototype);
        if (UNLIKELY(cv == NULL))
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (DCpointer)ret;
    }
    STMT_END;
    RETVAL = sv_bless((UNLIKELY(ix == 1) ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                      gv_stashpv("Affix", GV_ADD));

    ST(0) = sv_2mortal(RETVAL);
    safefree(prototype);

    XSRETURN(1);
}

XS_INTERNAL(Affix_DESTROY) {
    dXSARGS;
    CallBody *ptr;
    CV *THIS;
    STMT_START {
        HV *st;
        GV *gvp;
        SV *const xsub_tmp_sv = ST(0);
        SvGETMAGIC(xsub_tmp_sv);
        THIS = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
        {
            CV *cv = THIS;
            ptr = (CallBody *)XSANY.any_ptr;
        }
    }
    STMT_END;

    //~ if (ptr->arg_types != NULL) {
    //~ Safefree(ptr->arg_types);
    //~ ptr->arg_types = NULL;
    //~ }
    //~ if (ptr->arg_types) { sv_2mortal(MUTABLE_SV(ptr->arg_types)); }
    /*
        if (ptr->lib_name != NULL) {
            Safefree(ptr->lib_name);
            ptr->lib_name = NULL;
        }

        if (ptr->sym_name != NULL) {
            Safefree(ptr->sym_name);
            ptr->sym_name = NULL;
        }

        // Safefree(ptr->entry_point);



        sv_2mortal(ptr->resolve_lib_name);

        /*
            char *lib_name;
        DLLib *lib_handle;
        char *sym_name;
        void *entry_point;
        int16_t call_conv;
        size_t num_args;
        int16_t *arg_types;
        SV **arg_info;
        CV *resolve_lib_name;
        */
    Safefree(ptr);
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_END) { // cleanup
    dXSARGS;
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
    if (items != 2) croak_xs_usage(cv, "data, type");
    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ST(0))), "type", 0);
    SV *data = ST(1);
    DCpointer RETVAL = safemalloc(_sizeof(aTHX_ type));
    sv2ptr(aTHX_ type, data, RETVAL, false);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Pointer_unmarshal) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "ptr, type");
    SV *RETVAL;
    DCpointer ptr;
    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ST(0))), "type", 0);
    if (sv_derived_from(ST(1), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(1)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    RETVAL = ptr2sv(aTHX_ ptr, type);
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
    IV swap = (IV)SvIV(ST(2));
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(0)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp) + other);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer", ptr);
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
    IV swap = (IV)SvIV(ST(2));
    if (UNLIKELY(!sv_derived_from(ST(0), "Affix::Pointer")))
        croak("ptr is not of type Affix::Pointer");
    IV tmp = SvIV((SV *)SvRV(ST(0)));
    ptr = INT2PTR(DCpointer, PTR2IV(tmp) - other);
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer", ptr);
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
    if (items < 2 || items > 3) croak_xs_usage(cv, "ptr, size, utf8= false");
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
        DumpHex(ptr, size);
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
        //~ warn("DESTROY ::Pointer %p", ptr);
        //~ sv_dump(ST(0));
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
    SV *RETVAL;
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    char *key = SvPV_nolen(ST(1));
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    SV *actual;
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(key, SvPV(*name, PL_na)) == 0) {
            SV *_type = *av_fetch(MUTABLE_AV(SvRV(*elm)), 1, 0);
            size_t offset = _offsetof(aTHX_ _type); // meaningless for union
            SV **ptr_ptr = hv_fetchs(h, "pointer", 0);
            DCpointer ptr;
            {
                IV tmp = SvIV(SvRV(*ptr_ptr));
                ptr = INT2PTR(DCpointer, tmp + offset);
            }
            SV *val = ptr2sv(ptr, _type);
            RETVAL = sv_2mortal(val);
            break;
        }
    }

    if (actual != NULL) RETVAL = newSV(0);
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Aggregate_EXISTS) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    SV *RETVAL;
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
    SV *RETVAL;
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    char *u = SvPV_nolen(ST(1));
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
    SV *RETVAL;
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

    size_t RETVAL;
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
        sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
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
        sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
        ST(0) = RETVALSV;
    }

    XSRETURN(1);
}

XS_INTERNAL(Affix_realloc) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "ptr, size");

    DCpointer RETVAL;
    DCpointer ptr;
    size_t size = (size_t)SvUV(ST(1));

    if (sv_derived_from(ST(0), "Affix::Pointer")) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        ptr = INT2PTR(DCpointer, tmp);
    }
    else
        croak("ptr is not of type Affix::Pointer");
    ptr = saferealloc(ptr, size);
    sv_setref_pv(ST(0), "Affix::Pointer", ptr);
    SvSETMAGIC(ST(0));
    {
        SV *RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
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
            sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
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
            else if (SvPOK(ST(1))) { rhs = (DCpointer)(unsigned char *)SvPV_nolen(ST(1)); }
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
            sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
            ST(0) = RETVALSV;
        }
    }
    XSRETURN(1);
}

XS_INTERNAL(Affix_memcpy) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "dest, src, nitems");
    SP -= items;
    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src;
    {
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
        else if (SvPOK(ST(1))) { src = (DCpointer)(unsigned char *)SvPV_nolen(ST(1)); }
        else
            croak("dest is not of type Affix::Pointer");
        CopyD(src, dest, nitems, char);
    }
    PUTBACK;
    return;
}

XS_INTERNAL(Affix_memmove) {
    dVAR;
    dXSARGS;
    if (items != 3) croak_xs_usage(cv, "dest, src, nitems");
    SP -= items;

    size_t nitems = (size_t)SvUV(ST(2));
    DCpointer dest, src;
    {
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
        else if (SvPOK(ST(1))) { src = (DCpointer)(unsigned char *)SvPV_nolen(ST(1)); }
        else
            croak("dest is not of type Affix::Pointer");
        Move(src, dest, nitems, char);
    }
    PUTBACK;
    return;
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
        sv_setref_pv(RETVALSV, "Affix::Pointer", RETVAL);
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
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(\"\"", Affix_Type_asint, file, ";$");   \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        /* Making a sub named "Affix::Type::Int::()" allows the package */                         \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Type::Int") to return true. */                             \
        (void)newXSproto_portable("Affix::Type::" #NAME "::()", Affix_Type_asint, file, ";$");     \
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
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable("Affix::Type::" #NAME "::(\"\"", Affix_Type_asint, file, ";$");   \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
        /* Making a sub named "Affix::Type::Int::()" allows the package */                         \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Type::Int") to return true. */                             \
        (void)newXSproto_portable("Affix::Type::" #NAME "::()", Affix_Type_asint, file, ";$");     \
        XSANY.any_i32 = (int)AFFIX_CHAR;                                                           \
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
    TYPE(UInt, AFFIX_ARG_UINT, DC_SIGCHAR_INT);
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

    //~ (void)newXSproto_portable("Affix::_list_symbols", XS_Affix__list_symbols, file, "$");

    (void)newXSproto_portable("Affix::load_lib", Affix_load_lib, file, "$;$");
    export_function("Affix", "load_lib", "default");
    (void)newXSproto_portable("Affix::pin", Affix_pin, file, "$$$$");
    export_function("Affix", "pin", "default");
    (void)newXSproto_portable("Affix::unpin", Affix_unpin, file, "$");
    export_function("Affix", "unpin", "default");
    //
    cv = newXSproto_portable("Affix::affix", Affix_affix, file, "$$$;$");
    XSANY.any_i32 = 0;
    export_function("Affix", "affix", "default");
    cv = newXSproto_portable("Affix::wrap", Affix_affix, file, "$$$;$");
    XSANY.any_i32 = 1;
    export_function("Affix", "wrap", "all");
    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, file, "$");

    //~ cv = newXSproto_portable("Affix::affix", XS_Affix_affix, file, "$$$;$");
    //~ XSANY.any_i32 = 0;
    //~ cv = newXSproto_portable("Affix::wrap", XS_Affix_affix, file, "$$$;$");
    //~ XSANY.any_i32 = 1;
    //~ (void)newXSproto_portable("Affix::typedef", XS_Affix_typedef, file, "$$");
    //~ (void)newXSproto_portable("Affix::CLONE", XS_Affix_CLONE, file, ";@");

    // Utilities
    (void)newXSproto_portable("Affix::sv_dump", Affix_sv_dump, file, "$");

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

    //~ export_constant_char("Affix", "C", "abi", MANGLE_C);
    //~ export_constant_char("Affix", "ITANIUM", "abi", MANGLE_ITANIUM);
    //~ export_constant_char("Affix", "GCC", "abi", MANGLE_GCC);
    //~ export_constant_char("Affix", "MSVC", "abi", MANGLE_MSVC);
    //~ export_constant_char("Affix", "RUST", "abi", MANGLE_RUST);
    //~ export_constant_char("Affix", "SWIFT", "abi", MANGLE_SWIFT);
    //~ export_constant_char("Affix", "D", "abi", MANGLE_D);
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

#if PERL_VERSION_LE(5, 21, 5)
#if PERL_VERSION_GE(5, 9, 0)
    if (PL_unitcheckav) call_list(PL_scopestack_ix, PL_unitcheckav);
#endif
    XSRETURN_YES;
#else
    Perl_xs_boot_epilog(aTHX_ ax);
#endif
}

char *wchar2utf(const wchar_t *str, int len) {
    if (len < 0) len = wcslen(str);
    char *r = (char *)safemalloc(len * WCHAR_T_SIZE + 1);
    char *p = r;
    while (len--) {
        unsigned int w = *str++;
        if (w <= 0x7f) { *p++ = w; }
        else if (w <= 0x7ff) {
            *p++ = 0xc0 | (w >> 6);
            *p++ = 0x80 | (w & 0x3f);
        }
        else if (w <= 0xffff) {
            *p++ = 0xe0 | (w >> 12);
            *p++ = 0x80 | ((w >> 6) & 0x3f);
            *p++ = 0x80 | (w & 0x3f);
        }
        else {
            *p++ = 0xf0 | (w >> 18);
            *p++ = 0x80 | ((w >> 12) & 0x3f);
            *p++ = 0x80 | ((w >> 6) & 0x3f);
            *p++ = 0x80 | (w & 0x3f);
        }
    }
    *p++ = 0;
    return r;
}

wchar_t *utf2wchar(const char *str, int len) {
    wchar_t *r = (wchar_t *)safemalloc((len + 1) * WCHAR_T_SIZE), *p = r;
    for (unsigned char *s = (unsigned char *)str, *e = s + len; s < (unsigned char *)str + len;
         s++) {
        unsigned char c = *s;

        if (c < 0x80) { *p++ = c; }
        else if (c >= 0xc2 && c <= 0xdf && s[1] & 0xc0) {
            *p++ = ((c & 0x1f) << 6) | (s[1] & 0x3f);
            s++;
        }
        else if (c == 0xe0 && s[1] >= 0xa0 && s[1] <= 0xbf ||
                 c >= 0xe1 && c <= 0xec && s[1] >= 0x80 && s[1] <= 0xbf ||
                 c == 0xed && s[1] >= 0x80 && s[1] <= 0x9f ||
                 c >= 0xee && c <= 0xef && s[1] >= 0x80 && s[1] <= 0xbf) {
            *p++ = ((c & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
            s += 2;
        }
        else if (c == 0xf0 && s[1] >= 0x90 && s[1] <= 0xbf ||
                 c >= 0xf1 && c <= 0xf3 && s[1] >= 0x80 && s[1] <= 0xbf ||
                 c == 0xf4 && s[1] >= 0x80 && s[1] <= 0x8f) {
            *p++ =
                ((c & 0x07) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
            s += 3;
        }
        else { *p++ = 0xfffd; }
    }
    *p = 0;
    return r;
}

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv) {
    if (ptr == NULL) croak("Fuck");
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
        size_t len = wcslen((const wchar_t *)ptr);
        if (LIKELY(len)) {
            char *str = wchar2utf(*(const wchar_t **)ptr, (len + 2) * WCHAR_T_SIZE);
            RETVAL = newSVpvn_utf8(str, strlen(str), true);
        }
        else
            sv_set_undef(RETVAL);
    } break;
    case AFFIX_ARG_WCHAR: {
        SV *container = newSV(0);
        RETVAL = newSVpvs("");
        const char *pat = "W";
        switch (WCHAR_T_SIZE) {
        case I8SIZE:
            sv_setiv(container, (IV) * (char *)ptr);
            break;
        case SHORTSIZE:
            sv_setiv(container, (IV) * (short *)ptr);
            break;
        case INTSIZE:
            sv_setiv(container, *(int *)ptr);
            break;
        default:
            croak("Invalid wchar_t size for argument!");
        }
        sv_2mortal(container);
        packlist(RETVAL, pat, pat + 1, &container, &container + 1);
    } break;
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
        SV *p = newSV(0);
        sv_setref_pv(p, "Affix::Pointer::Unmanaged", ptr);
        SV *tie = newRV_noinc(MUTABLE_SV(newHV()));
        hv_store((HV *)SvRV(tie), "pointer", 7, p, 0);
        hv_store((HV *)SvRV(tie), "type", 4, newRV_inc(type_sv), 0);
        sv_bless(tie, gv_stashpv("Affix::Union", TRUE));
        hv_magic(RETVAL_, tie, PERL_MAGIC_tied);
        SvSetSV(RETVAL, newRV(MUTABLE_SV(RETVAL_)));

        //~ {
        //~ HV *RETVAL_ = newHV_mortal();
        //~ HV *_type = MUTABLE_HV(SvRV(type_sv));
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
        croak("Unhandled type in ptr2sv: %s (%d)", type, type_as_str(type));
    }

    return RETVAL;
}

void sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed) {
    SV *RETVAL = newSV(0);
    int16_t type = SvIV(type_sv);
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
    case AFFIX_ARG_CHAR:
    case AFFIX_ARG_UCHAR: {
        if (SvPOK(data)) {
            size_t len;
            char *value = SvPV(data, len);
            Copy(value, ptr, len + 1, char);
        }
        else {
            char value = SvIOK(data) ? SvIV(data) : 0;
            Copy(&value, ptr, 1, char);
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
            const char *str_ = SvPVutf8(data, len);
            wchar_t *str = utf2wchar(str_, len + 1);
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
        DCCallback *cb = NULL;
        HV *field = MUTABLE_HV(SvRV(type_sv)); // Make broad assumptions
        SV **ret = hv_fetchs(field, "ret", 0);
        SV **args = hv_fetchs(field, "args", 0);
        SV **sig = hv_fetchs(field, "sig", 0);
        Callback *callback;
        Newxz(callback, 1, Callback);
        callback->args = MUTABLE_AV(SvRV(*args));
        size_t arg_count = av_count(callback->args);
        Newxz(callback->sig, arg_count, char);
        for (size_t i = 0; i < arg_count; ++i) {
            SV **type = av_fetch(callback->args, i, 0);
            callback->sig[i] = type_as_dc(aTHX_ SvIV(*type));
        }
        callback->sig = SvPV_nolen(*sig);
        callback->ret = callback->sig[arg_count];
        callback->cv = SvREFCNT_inc(data);
        storeTHX(callback->perl);
        cb = dcbNewCallback(callback->sig, cbHandler, callback);
        {
            CallbackWrapper *hold;
            Newxz(hold, 1, CallbackWrapper);
            hold->cb = cb;
            DumpHex(hold, 16);
            DumpHex(ptr, 16);
            Copy(hold, ptr, 1, DCpointer);
            PING;
        }
    } break;
    default: {
        croak("%s (%d) is not a known type in sv2ptr(...)", type_as_str(type), type);
    }
    }
    return;
}
