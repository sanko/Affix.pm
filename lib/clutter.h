#ifdef __cplusplus
extern "C" {
#endif

#define PERL_NO_GET_CONTEXT /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

#ifdef USE_ITHREADS
static PerlInterpreter *my_perl; /***    The Perl interpreter    ***/
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#define dcAllocMem Newxz
#define dcFreeMem Safefree

#include "ppport.h"

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

#include <dyncall_value.h>
#include <dyncall_callf.h>

#include <dyncall_signature.h>

#include <dyncall/dyncall/dyncall_aggregate.h>

//{ii[5]Z&<iZ>}
#define DC_SIGCHAR_CODE '&'   // 'p' but allows us to wrap CV * for the user
#define DC_SIGCHAR_ARRAY '['  // 'A' but nicer
#define DC_SIGCHAR_STRUCT '{' // 'A' but nicer
#define DC_SIGCHAR_UNION '<'  // 'A' but nicer
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

#define DECL_BOOT(name) EXTERN_C XS(CAT2(boot_, name))
#define CALL_BOOT(name)                                                                            \
    STMT_START {                                                                                   \
        PUSHMARK(SP);                                                                              \
        CALL_FPTR(CAT2(boot_, name))(aTHX_ cv);                                                    \
    }                                                                                              \
    STMT_END

/* Useful but undefined in perlapi */
#define FLOATSIZE sizeof(float)
#define BOOLSIZE sizeof(bool) // ha!

#if PERL_VERSION_LE(5, 8, 999) /* PERL_VERSION_LT is 5.33+ */
char *file = __FILE__;
#else
const char *file = __FILE__;
#endif

/* api wrapping utils */

#define MY_CXT_KEY "Dyn::_guts" XS_VERSION

typedef struct
{
    int count;
    HV *structs;
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

typedef struct
{
    DLLib *lib;
    DLSyms *syms;
    DCCallVM *cvm;
} Dyncall;

typedef struct
{
    const char *name;
    const char *sig;
    const char *ret;
    DCpointer *fptr;
    Dyncall *lib;
} DynXSub;

typedef struct
{
    SV *cb;
    const char *signature;
    char ret_type;
    char mode;
    SV *userdata;
    DCCallVM *cvm;
} _callback;

/* Returns the amount of padding needed after `offset` to ensure that the
following address will be aligned to `alignment`. */
size_t padding_needed_for(size_t offset, size_t alignment) {
    // dTHX;
    // warn("padding_needed_for( %d, %d );", offset, alignment);
    size_t misalignment = offset % alignment;
    if (misalignment > 0) // round to the next multiple of alignment
        return alignment - misalignment;
    return 0; // already a multiple of alignment
}

HV *Fl_stash,   // For inserting stuff directly into Fl's namespace
    *Fl_export, // For inserting stuff directly into Fl's exports//
    *Fl_cache;  // For caching weak refs to widgets and other objects

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

void export_function(const char *what, const char *_tag) {
    dTHX;
    warn("Exporting %s to %s", what, _tag);
    SV **tag = hv_fetch(Fl_export, _tag, strlen(_tag), TRUE);
    if (tag && SvOK(*tag) && SvROK(*tag) && (SvTYPE(SvRV(*tag))) == SVt_PVAV)
        av_push((AV *)SvRV(*tag), newSVpv(what, 0));
    else {
        SV *av;
        av = (SV *)newAV();
        av_push((AV *)av, newSVpv(what, 0));
        tag = hv_store(Fl_export, _tag, strlen(_tag), newRV_noinc(av), 0);
    }
}

void export_constant(const char *name, const char *_tag, double val) {
    dTHX;
    SV *value;
    value = newSVnv(val);
    register_constant("Dyn", name, value);
    export_function(name, _tag);
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

    printf("Dumping %d bytes from %p at %s line %d\n", len, addr, file, line);

    // Length checks.
    if (len == 0) croak("ZERO LENGTH");

    if (len < 0) croak("NEGATIVE LENGTH: %d", len);

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

static char callback_handler(DCCallback *cb, DCArgs *args, DCValue *result, void *userdata) {
    dTHX;
#ifdef USE_ITHREADS
    PERL_SET_CONTEXT(my_perl);
#endif
    char ret_type;
    {
        dSP;
        int count;

        _callback *container = ((_callback *)userdata);
        // int * ud = (int*) container->userdata;

        SV *cb_sv = container->cb;

        ENTER;
        SAVETMPS;
        // warn("here at %s line %d.", __FILE__, __LINE__);
        PUSHMARK(SP);
        {
            const char *signature = container->signature;
            // warn("signature == %s at %s line %d.", container->signature, __FILE__, __LINE__);
            int done, okay;
            int i;
            // warn("signature: %s at %s line %d.", signature, __FILE__, __LINE__);
            for (i = 0; signature[i + 1] != '\0'; ++i) {
                done = okay = 0;
                // warn("here at %s line %d.", __FILE__, __LINE__);
                // warn("signature[%d] == %c at %s line %d.", i, signature[i], __FILE__, __LINE__);
                switch (signature[i]) {
                case DC_SIGCHAR_VOID:
                    // warn("Unhandled callback argument '%c' at %s line %d.", signature[i],
                    // __FILE__, __LINE__);
                    break;
                case DC_SIGCHAR_BOOL:
                    XPUSHs(newSViv(dcbArgBool(args)));
                    break;
                case DC_SIGCHAR_CHAR:
                case DC_SIGCHAR_UCHAR:
                    XPUSHs(newSViv(dcbArgChar(args)));
                    break;
                case DC_SIGCHAR_SHORT:
                case DC_SIGCHAR_USHORT:
                    XPUSHs(newSViv(dcbArgShort(args)));
                    break;
                case DC_SIGCHAR_INT:
                    XPUSHs(newSViv(dcbArgInt(args)));
                    break;
                case DC_SIGCHAR_UINT:
                    XPUSHs(newSVuv(dcbArgInt(args)));
                    break;
                case DC_SIGCHAR_LONG:
                    XPUSHs(newSVnv(dcbArgLong(args)));
                    break;
                case DC_SIGCHAR_ULONG:
                    XPUSHs(newSVuv(dcbArgLong(args)));
                    break;
                case DC_SIGCHAR_LONGLONG:
                    XPUSHs(newSVnv(dcbArgLongLong(args)));
                    break;
                case DC_SIGCHAR_ULONGLONG:
                    XPUSHs(newSVuv(dcbArgLongLong(args)));
                    break;
                case DC_SIGCHAR_FLOAT:
                    XPUSHs(newSVnv(dcbArgFloat(args)));
                    break;
                case DC_SIGCHAR_DOUBLE:
                    XPUSHs(newSVnv(dcbArgDouble(args)));
                    break;
                case DC_SIGCHAR_POINTER:
                    XPUSHs(sv_setref_pv(newSV(0), "Dyn::Call::Pointer", dcbArgPointer(args)));
                    break;
                case DC_SIGCHAR_STRING:
                    XPUSHs(newSVpv((const char *)dcbArgPointer(args), 0));
                    break;
                case DC_SIGCHAR_AGGREGATE:
                    warn("Unhandled callback argument '%c' at %s line %d.", signature[i], __FILE__,
                         __LINE__);
                    break;
                case DC_SIGCHAR_ENDARG:
                    ret_type = signature[i + 1];
                    done++;
                    break;
                default:
                    warn("Unhandled callback argument '%c' at %s line %d.", signature[i], __FILE__,
                         __LINE__);
                    break;
                };
                if (done) break;
                /*
                                int       arg1 = dcbArgInt     (args);
                float     arg2 = dcbArgFloat   (args);
                short     arg3 = dcbArgShort   (args);
                double    arg4 = dcbArgDouble  (args);
                long long arg5 = dcbArgLongLong(args);
                  */
            }
            // warn("here at %s line %d.", __FILE__, __LINE__);
        }
        // warn("here at %s line %d.", __FILE__, __LINE__);

        // XXX: Does anyone expect this?
        // XPUSHs(container->userdata);

        PUTBACK;

        // warn("here at %s line %d.", __FILE__, __LINE__);
        // SV ** signature = hv_fetch(container, "f_signature", 11, 0);
        // warn("here at %s line %d.", __FILE__, __LINE__);
        // warn("signature was %s", signature);

        count = call_sv(cb_sv, ret_type == DC_SIGCHAR_VOID ? G_VOID : G_SCALAR);

        SPAGAIN;

        // warn("return type: %c at %s line %d.", ret_type, __FILE__, __LINE__);

        switch (ret_type) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_BOOL:
            if (count != 1) croak("Unexpected return values");
            result->B = (bool)POPi;
            break;
        case DC_SIGCHAR_CHAR:
            if (count != 1) croak("Unexpected return values");
            result->c = (char)POPi;
            break;
        case DC_SIGCHAR_UCHAR:
            if (count != 1) croak("Unexpected return values");
            result->C = (u_char)POPi;
            break;
        case DC_SIGCHAR_SHORT:
            if (count != 1) croak("Unexpected return values");
            result->s = (short)POPi;
            break;
        case DC_SIGCHAR_USHORT:
            if (count != 1) croak("Unexpected return values");
            result->S = (u_short)POPi;
            break;
        case DC_SIGCHAR_INT:
            if (count != 1) croak("Unexpected return values");
            result->i = (int)POPi;
            break;
        case DC_SIGCHAR_UINT:
            if (count != 1) croak("Unexpected return values");
            result->I = (u_int)POPi;
            break;
        case DC_SIGCHAR_LONG:
            if (count != 1) croak("Unexpected return values");
            result->j = POPl;
            break;
        case DC_SIGCHAR_ULONG:
            if (count != 1) croak("Unexpected return values");
            result->J = POPul;
            break;
        case DC_SIGCHAR_LONGLONG:
            if (count != 1) croak("Unexpected return values");
            result->l = (long long)POPl;
            break;
        case DC_SIGCHAR_ULONGLONG:
            if (count != 1) croak("Unexpected return values");
            result->L = POPul;
            break;
        case DC_SIGCHAR_FLOAT: // double
            if (count != 1) croak("Unexpected return values");
            result->f = (float)POPn;
            break;
        case DC_SIGCHAR_DOUBLE: // double
            if (count != 1) croak("Unexpected return values");
            result->d = (double)POPn;
            break;
        case DC_SIGCHAR_POINTER: // string
            if (count != 1) croak("Unexpected return values");
            result->p = (DCpointer)((intptr_t)POPl);
            break;
        case DC_SIGCHAR_STRING: // string
            if (count != 1) croak("Unexpected return values");
            result->Z = POPpx;
            break;
        case DC_SIGCHAR_AGGREGATE: // string
            if (count != 1) croak("Unexpected return values");
            warn("Unhandled return type at %s line %d.", __FILE__, __LINE__);
            // result->l = POPl;            break;
            break;
        }

        PUTBACK;
        FREETMPS;
        LEAVE;
    }

    return ret_type;
}

const char *ordinal(int n) {
    static const char suffixes[][3] = {"th", "st", "nd", "rd"};
    auto ord = n % 100;
    if (ord / 10 == 1) { ord = 0; }
    ord = ord % 10;
    if (ord > 3) { ord = 0; }
    return suffixes[ord];
}
