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

/* Useful but undefined in perlapi */
#define FLOAT_SIZE sizeof(float)
#define BOOL_SIZE sizeof(bool)         // ha!
#define DOUBLE_SIZE sizeof(double)     // ugh...
#define INTPTR_T_SIZE sizeof(intptr_t) // ugh...
#define WCHAR_T_SIZE sizeof(wchar_t)

// MEM_ALIGNBYTES is messed up by quadmath and long doubles
#define AFFIX_ALIGNBYTES 8

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
    case AFFIX_ARG_ASCIISTR:
        sv_setsv(RETVAL, newSVpv(*(char **)ptr, 0));
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
            croak("Failed to load %s", lib, reason);
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
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::Pointer", GV_ADD)));
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_CodeRef) {
    dXSARGS;
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    if (av_count(fields) == 2) {
        {
            SV *arg_ptr = *av_fetch(fields, 0, 0);
            AV *args = MUTABLE_AV(SvRV(arg_ptr));
            size_t field_count = av_count(args);
            for (int i = 0; i < field_count; ++i) {
                SV **type_ref = av_fetch(args, i, 0);
                if (!(sv_isobject(*type_ref) && sv_derived_from(*type_ref, "Affix::Type::Base")))
                    croak("%s is not a subclass of "
                          "Affix::Type::Base",
                          SvPV_nolen(*type_ref));
            }
            hv_stores(RETVAL_HV, "args", SvREFCNT_inc(arg_ptr));
        }

        {
            SV **ret_ref = av_fetch(fields, 1, 0);
            SV *ret = *ret_ref;
            if (!(sv_isobject(ret) && sv_derived_from(ret, "Affix::Type::Base")))
                croak("CodeRef[...] expects a return type that is a subclass of Affix::Type::Base");
            hv_stores(RETVAL_HV, "ret", SvREFCNT_inc(ret));
        }
    }
    else
        croak("CodeRef[...] expects a list of argument types and a single return type. e.g. "
              "CodeRef[[Int, Char, Str] => Void]");
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::CodeRef", GV_ADD)));
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
        size_t field_count = av_count(fields_in);
        size_t size = 0;

        for (int i = 0; i < field_count; i += 2) {
            AV *field = newAV();
            SV *key = newSVsv(*av_fetch(fields_in, i, 0));
            if (!SvPOK(key)) croak("Given name of '%s' is not a string", SvPV_nolen(key));
            SV *type = *av_fetch(fields_in, i + 1, 0);
            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                croak("Given type for '%s' is not a subclass of Affix::Type::Base",
                      SvPV_nolen(key));
            size_t __sizeof = _sizeof(aTHX_ type);
            size += packed ? 0
                           : padding_needed_for(
                                 size, AFFIX_ALIGNBYTES > __sizeof ? __sizeof : AFFIX_ALIGNBYTES);
            size += __sizeof;
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "offset", newSVuv(size - __sizeof));

            (void)hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSVuv(__sizeof));
            av_push(field, SvREFCNT_inc(key));
            SV **value_ptr = av_fetch(fields_in, i + 1, 0);
            SV *value = *value_ptr;
            av_push(field, SvREFCNT_inc(value));
            SV *sv_field = (MUTABLE_SV(field));
            av_push(fields, newRV(sv_field));
        }

        if (!packed && size > AFFIX_ALIGNBYTES * 2)
            size += padding_needed_for(size, AFFIX_ALIGNBYTES);

        hv_stores(RETVAL_HV, "sizeof", newSVuv(size));
        hv_stores(RETVAL_HV, "fields", newRV(MUTABLE_SV(fields)));
    }
    else
        croak("Struct[...] expects an even size list of and field names and types. e.g. Struct[ "
              "epoch => Int, name => Str, ... ]");
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::Struct", GV_ADD)));
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
        size_t field_count = av_count(fields_in);
        size_t size = 0;

        for (int i = 0; i < field_count; i += 2) {
            AV *field = newAV();
            SV *key = newSVsv(*av_fetch(fields_in, i, 0));
            if (!SvPOK(key)) croak("Given name of '%s' is not a string", SvPV_nolen(key));
            SV *type = *av_fetch(fields_in, i + 1, 0);
            if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
                croak("Given type for '%s' is not a subclass of Affix::Type::Base",
                      SvPV_nolen(key));
            size_t __sizeof = _sizeof(aTHX_ type);

            if (size < __sizeof) size = __sizeof;
            if (!packed && field_count > 1 && __sizeof > AFFIX_ALIGNBYTES)
                size += padding_needed_for(__sizeof, AFFIX_ALIGNBYTES);
            (void)hv_stores(MUTABLE_HV(SvRV(type)), "offset", newSVuv(0));

            (void)hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSVuv(__sizeof));
            av_push(field, SvREFCNT_inc(key));
            SV **value_ptr = av_fetch(fields_in, i + 1, 0);
            SV *value = *value_ptr;
            av_push(field, SvREFCNT_inc(value));
            SV *sv_field = (MUTABLE_SV(field));
            av_push(fields, newRV(sv_field));
        }

        hv_stores(RETVAL_HV, "sizeof", newSVuv(size));
        hv_stores(RETVAL_HV, "fields", newRV(MUTABLE_SV(fields)));
    }
    else
        croak("Union[...] expects an even size list of and field names and types. e.g. Struct[ "
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
    char *lib_name;
    DLLib *lib_handle;
    char *sym_name;
    void *entry_point;
    int16_t call_conv;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    SV **arg_info;
    SV *ret_info;
    SV *resolve_lib_name;
};

extern "C" void Affix_trigger(pTHX_ CV *cv) {
    dXSARGS;
    dMY_CXT;

    CallBody *ptr = (CallBody *)XSANY.any_ptr;
    char **free_strs = NULL;
    void **free_ptrs = NULL;
    int num_args = ptr->num_args, num_strs = 0, num_ptrs = 0, i;
    int16_t *arg_types = ptr->arg_types;
    bool void_ret = false;

    //~ dcMode(MY_CXT.cvm, ptr->call_conv);
    dcReset(MY_CXT.cvm);

    if (UNLIKELY(items != num_args)) {
        if (UNLIKELY(items > num_args)) croak("Too many arguments");
        croak("Not enough arguments");
    }

    for (i = 0; LIKELY(i < num_args); ++i) {
        //~ warn("%d of %d == %d (%d)", i, num_args, ptr->arg_types[i], (arg_types[i] &
        // AFFIX_ARG_TYPE_MASK)); ~ sv_dump(ST(i));
        switch (arg_types[i] & AFFIX_ARG_TYPE_MASK) {
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
            if (!free_strs) free_strs = (char **)safemalloc(num_args * sizeof(char *));
            SV *idk = call_encoding(aTHX_ "encode", find_encoding(aTHX), ST(i), NULL);
            STRLEN len;
            char *holder = SvPV(idk, len);
            free_strs[num_strs] = (char *)safemalloc(len + WCHAR_T_SIZE);
            Copy(holder, free_strs[num_strs], len, char);
            dcArgPointer(MY_CXT.cvm, free_strs[num_strs]);
            num_strs++;
        } break;
        case AFFIX_ARG_CSTRUCT:
        case AFFIX_ARG_CARRAY: {
            //~ if (!SvOK(ST(i)) && SvREADONLY(ST(i)) // explicit undef
            //~ ) {
            //~ dcArgPointer(MY_CXT.cvm, NULL);
            //~ }
            //~ else {
            //~ if (!SvROK(ST(i)) || SvTYPE(SvRV(ST(i))) != SVt_PVAV)
            //~ croak("Type of arg %lu must be an array ref", i + 1);
            //~ AV *elements = MUTABLE_AV(SvRV(ST(i)));
            //~ HV *hv_ptr = MUTABLE_HV(SvRV(type));
            //~ SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
            //~ SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
            //~ SV **ptr_ptr = hv_fetchs(hv_ptr, "pointer", 0);
            //~ size_t av_len;
            //~ if (SvOK(*size_ptr)) {
            //~ av_len = SvIV(*size_ptr);
            //~ if (av_count(elements) != av_len)
            //~ croak("Expected an array of %lu elements; found %zd", av_len,
            //~ av_count(elements));
            //~ }
            //~ else
            //~ av_len = av_count(elements);
            //~ hv_stores(hv_ptr, "size_", newSViv(av_len));
            //~ size_t size = _sizeof(aTHX_ * type_ptr);
            //~ warn("av_len * size = %d * %d = %d", av_len, size, av_len * size);
            //~ Newxz(pointer[pos_arg], av_len * size, char);
            //~ l_pointer[pos_arg] = true;
            //~ pointers = true;
            //~ sv2ptr(aTHX_ type, ST(pos_arg), pointer[pos_arg], false);
            //~ dcArgPointer(MY_CXT.cvm, pointer[pos_arg]);
            //~ }
        }
            croak("Unhandled arg type");

        case AFFIX_ARG_CALLBACK: {
            if (SvOK(ST(i))) {
                DCCallback *hold;
                // Newx(hold, 1, DCCallback);
                sv2ptr(aTHX_ ptr->arg_info[i], ST(i), hold, false);
                dcArgPointer(MY_CXT.cvm, hold);
            }
            else
                dcArgPointer(MY_CXT.cvm, NULL);
        } break;
        case AFFIX_ARG_CPOINTER: {
            if (!free_ptrs) free_ptrs = (void **)safemalloc(num_args * sizeof(intptr_t));
            //~ warn("DC_SIGCHAR_POINTER [%d,%d,%d]", pos_arg, pos_csig, pos_psig);
            //~ sv_dump(type);
            SV **subtype_ptr = hv_fetchs(MUTABLE_HV(SvRV(ptr->arg_info[i])), "type", 0);
            if (SvOK(ST(i))) {
                if (sv_derived_from(ST(i), "Affix::Pointer")) {
                    IV tmp = SvIV((SV *)SvRV(ST(i)));
                    free_ptrs[num_ptrs] = INT2PTR(DCpointer, tmp);
                }
                else {
                    if (sv_isobject(ST(i))) croak("Unexpected pointer to blessed object");
                    free_ptrs[num_ptrs] = safemalloc(_sizeof(aTHX_ * subtype_ptr));
                    sv2ptr(aTHX_ * subtype_ptr, ST(i), free_ptrs[num_ptrs], false);
                }
            }
            else if (SvREADONLY(ST(i))) { // explicit undef
                free_ptrs[num_ptrs] = NULL;
            }
            else { // treat as if it's an lST(pos_arg)
                SV **subtype_ptr = hv_fetchs(MUTABLE_HV(SvRV(ptr->arg_info[i])), "type", 0);
                SV *type = *subtype_ptr;
                size_t size = _sizeof(aTHX_ type);
                Newxz(free_ptrs[num_ptrs], size, char);
            }
            dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
            num_ptrs++;
        } break;
        case AFFIX_ARG_VMARRAY:
        case AFFIX_ARG_CUNION:
        case AFFIX_ARG_CPPSTRUCT:
            croak("Unhandled arg type");
            break;
        }
    }
    /*
    #define AFFIX_ARG_CSTRUCT 22
    #define AFFIX_ARG_CARRAY 24
    #define AFFIX_ARG_VMARRAY 30
    #define AFFIX_ARG_CUNION 42
    #define AFFIX_ARG_CPPSTRUCT 44
       */

    //~ warn("oy line %d / %d / %p", __LINE__, ptr->ret_type, ptr->entry_point);

    switch (ptr->ret_type) {
    case AFFIX_ARG_VOID:
        void_ret = true;
        dcCallVoid(MY_CXT.cvm, ptr->entry_point);
        break;
    case AFFIX_ARG_BOOL:
        ST(0) = boolSV(dcCallBool(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_CHAR:
        ST(0) = newSViv((char)dcCallChar(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_UCHAR:
        ST(0) = newSVuv((unsigned char)dcCallChar(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_SHORT:
        ST(0) = newSViv((short)dcCallShort(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_USHORT:
        ST(0) = newSVuv((unsigned short)dcCallShort(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_INT:
        ST(0) = newSViv(dcCallInt(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_UINT:
        ST(0) = newSVuv((unsigned int)dcCallInt(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_LONG:
        ST(0) = newSViv((long)dcCallLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_ULONG:
        ST(0) = newSVuv((unsigned long)dcCallLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_LONGLONG:
        ST(0) = newSViv((I64)dcCallLongLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_ULONGLONG:
        ST(0) = newSVuv((U64)dcCallLongLong(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_FLOAT:
        ST(0) = newSVnv(dcCallFloat(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_DOUBLE:
        ST(0) = newSVnv(dcCallDouble(MY_CXT.cvm, ptr->entry_point));
        break;
    case AFFIX_ARG_ASCIISTR:
        ST(0) = newSVpv((char *)dcCallPointer(MY_CXT.cvm, ptr->entry_point), 0);
        break;
    case AFFIX_ARG_CPOINTER: {
        SV *type = *hv_fetchs(MUTABLE_HV(SvRV(ptr->ret_info)), "type", 0);
        if (sv_derived_from(type, "Affix::Type::Void")) {
            sv_setref_pv(ST(0), "Affix::Pointer", dcCallPointer(MY_CXT.cvm, ptr->entry_point));
        }
        else {
            ST(0) = newRV_noinc(ptr2sv(aTHX_ dcCallPointer(MY_CXT.cvm, ptr->entry_point), type));
        }
    } break;
    default:
        croak("Unknown return type: %d", ptr->ret_type);
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

    for (i = 0; LIKELY(i < num_args); ++i) {
        if (UNLIKELY((arg_types[i] & AFFIX_ARG_RW_MASK) == AFFIX_ARG_RW)) {
            switch (arg_types[i] & AFFIX_ARG_TYPE_MASK) {
            case AFFIX_ARG_VOID:
            case AFFIX_ARG_DOUBLE:
            // break;
            default:
                croak("Unhandled pointer type");
            };
        }
    }

    /* Free any memory that we need to. */
    if (UNLIKELY(num_strs > 0)) {
        for (i = 0; i < num_strs; ++i)
            safefree(free_strs[i]);
        safefree(free_strs);
    }
    if (UNLIKELY(num_ptrs > 0)) {
        for (i = 0; LIKELY(i < num_ptrs); ++i)
            safefree(free_ptrs[i]);
        safefree(free_ptrs);
    }
    if (UNLIKELY(void_ret)) XSRETURN_EMPTY;

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
            croak("Failed to load %s", lib, reason);
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
        if (ret->num_args < 0) { ret->num_args = 0; }
        else {
            Newxz(ret->arg_types, ret->num_args, int16_t);
            if (ret->arg_types == NULL) { croak("unable to malloc int array"); }
            for (x = 0; x < ret->num_args; ++x) {
                ssv = av_fetch(av, x, 0);
                if (LIKELY(ssv != NULL) && SvROK(*ssv) &&
                    sv_derived_from(*ssv, "Affix::Type::Base")) {
                    ret->arg_types[x] = (int16_t)SvIV(*ssv);
                    prototype[x] = '$';
                    //~ warn( "=== ret->arg_types[%d] == %d", x, ret->arg_types[x]);
                }
                else {
                    croak("Arg # %d is an invalid or unknown type", x);
                    ret->arg_types[x] = (int16_t)0;
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

    STMT_START {
        cv = newXSproto_portable(name, (/*ix == 2 ? Affix_trigger2 :*/ Affix_trigger), file,
                                 prototype);
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

    if (ptr->arg_types != NULL) {
        Safefree(ptr->arg_types);
        ptr->arg_types = NULL;
    }

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

    /*
    #define AFFIX_ARG_CARRAY 24
    */
    EXT_TYPE(CodeRef, AFFIX_ARG_CALLBACK, AFFIX_ARG_CALLBACK);
    EXT_TYPE(Pointer, AFFIX_ARG_CPOINTER, AFFIX_ARG_CPOINTER);

    /*
    #define AFFIX_ARG_VMARRAY 30
    */
    EXT_TYPE(Union, AFFIX_ARG_CUNION, AFFIX_ARG_CUNION);

    /*
    #define AFFIX_ARG_CPPSTRUCT 44
    #define AFFIX_ARG_WCHAR 46
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
    export_function("Affix", "wrap", "default");

    cv = newXSproto_portable("Affix::affix_2", Affix_affix, file, "$$$;$");
    XSANY.any_i32 = 2;

    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, file, "$");

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
    //~ (void)newXSproto_portable("Affix::calloc", XS_Affix_triggeroc, file, "$$");
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
