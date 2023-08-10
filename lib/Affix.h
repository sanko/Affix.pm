﻿#ifndef AFFIX_H_SEEN
#define AFFIX_H_SEEN 1

// Settings
#define TIE_MAGIC 0 // If true, aggregate values are tied and magical

#ifdef __cplusplus
extern "C" {
#endif

#define PERL_NO_GET_CONTEXT 1 /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
#define NO_XSLOCKS /* for exceptions */
#include <XSUB.h>

#ifndef sv_setbool_mg
#define sv_setbool_mg(sv, b) sv_setsv_mg(sv, boolSV(b)) /* new in perl 5.36 */
#endif
#ifndef newSVbool
#define newSVbool(b) boolSV(b) /* new in perl 5.36 */
#endif

#include <wchar.h>

#if __WIN32
#include <cstdint>
#include <windows.h>
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

// Only in 5.38.0+
#ifndef PERL_ARGS_ASSERT_NEWSV_FALSE
#define newSV_false() newSVsv(&PL_sv_no)
#endif

#ifndef PERL_ARGS_ASSERT_NEWSV_TRUE
#define newSV_true() newSVsv(&PL_sv_yes)
#endif

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

#include <dyncall_version.h>

#if defined(DC__C_GNU) || defined(DC__C_CLANG)
#include <cxxabi.h>
#endif

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

#if DEBUG > 1
#define PING warn("Ping at %s line %d", __FILE__, __LINE__);
#else
#define PING ;
#endif

/* Native argument types */
#define AFFIX_TYPE_VOID 'v'
#define AFFIX_TYPE_BOOL 'b'
#define AFFIX_TYPE_SCHAR 'a'
#define AFFIX_TYPE_CHAR 'c'
#define AFFIX_TYPE_UCHAR 'h'
#define AFFIX_TYPE_SHORT 's'
#define AFFIX_TYPE_USHORT 't'
#define AFFIX_TYPE_INT 'i'
#define AFFIX_TYPE_UINT 'j'
#define AFFIX_TYPE_LONG 'l'
#define AFFIX_TYPE_ULONG 'm'
#define AFFIX_TYPE_LONGLONG 'x'
#define AFFIX_TYPE_ULONGLONG 'y'
#define AFFIX_TYPE_FLOAT 'f'
#define AFFIX_TYPE_DOUBLE 'd'
#define AFFIX_TYPE_ASCIISTR 28
#define AFFIX_TYPE_UTF8STR 30
#define AFFIX_TYPE_UTF16STR 32
#define AFFIX_TYPE_CSTRUCT 34
#define AFFIX_TYPE_CPPSTRUCT 35
#define AFFIX_TYPE_CARRAY 36
#define AFFIX_TYPE_CALLBACK 38
#define AFFIX_TYPE_CPOINTER 'P'
#define AFFIX_TYPE_CUNION 42
#if Size_t_size == INTSIZE
#define AFFIX_TYPE_SSIZE_T AFFIX_TYPE_INT
#define AFFIX_TYPE_SIZE_T AFFIX_TYPE_UINT
#elif Size_t_size == LONGSIZE
#define AFFIX_TYPE_SSIZE_T AFFIX_TYPE_LONG
#define AFFIX_TYPE_SIZE_T AFFIX_TYPE_ULONG
#elif Size_t_size == LONGLONGSIZE
#define AFFIX_TYPE_SSIZE_T AFFIX_TYPE_LONGLONG
#define AFFIX_TYPE_SIZE_T AFFIX_TYPE_ULONGLONG
#else // quadmath is broken
#define AFFIX_TYPE_SSIZE_T AFFIX_TYPE_LONGLONG
#define AFFIX_TYPE_SIZE_T AFFIX_TYPE_ULONGLONG
#endif
#define AFFIX_TYPE_WCHAR 'w'
#define AFFIX_TYPE_SV 46
#define AFFIX_TYPE_REF 48
#define AFFIX_TYPE_STD_STRING 50
#define AFFIX_TYPE_INSTANCE_OF 52

/* Flag for whether we should free a string after passing it or not. */
#define AFFIX_TYPE_NO_FREE_STR 0
#define AFFIX_TYPE_FREE_STR 1
#define AFFIX_TYPE_FREE_STR_MASK 1

/* Flag for whether we need to refresh a CArray after passing or not. */
#define AFFIX_TYPE_NO_REFRESH 0
#define AFFIX_TYPE_REFRESH 1
#define AFFIX_TYPE_REFRESH_MASK 1
#define AFFIX_TYPE_NO_RW 0
#define AFFIX_TYPE_RW 256
#define AFFIX_TYPE_RW_MASK 256

#define AFFIX_UNMARSHAL_KIND_GENERIC -1
#define AFFIX_UNMARSHAL_KIND_RETURN -2
#define AFFIX_UNMARSHAL_KIND_NATIVECAST -3

// MEM_ALIGNBYTES is messed up by quadmath and long doubles
#define AFFIX_ALIGNBYTES 8

/* Useful but undefined in perlapi */
#define FLOAT_SIZE sizeof(float)
#define BOOL_SIZE sizeof(bool)         // ha!
#define DOUBLE_SIZE sizeof(double)     // ugh...
#define INTPTR_T_SIZE sizeof(intptr_t) // ugh...
#define WCHAR_T_SIZE sizeof(wchar_t)

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

#define EXT_TYPE(NAME, AFFIX_CHAR, DC_CHAR)                                                        \
    {                                                                                              \
        set_isa("Affix::Type::" #NAME, "Affix::Type::Base");                                       \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::" #NAME, Affix_Type_##NAME, __FILE__, "$");           \
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

// marshal.cxx
size_t padding_needed_for(size_t offset, size_t alignment);
SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv);
DCpointer sv2ptr(pTHX_ SV *type_sv, SV *data, bool packed);
size_t _alignof(pTHX_ SV *type);
size_t _sizeof(pTHX_ SV *type);
size_t _offsetof(pTHX_ SV *type);

// wchar_t.cxx
SV *wchar2utf(pTHX_ wchar_t *src, int len);
wchar_t *utf2wchar(pTHX_ SV *src, int len);

// Affix/Aggregate.cxx
DCaggr *_aggregate(pTHX_ SV *type);

// Affix/Utils.cxx
#define export_function(package, what, tag)                                                        \
    _export_function(aTHX_ get_hv(form("%s::EXPORT_TAGS", package), GV_ADD), what, tag)
void register_constant(const char *package, const char *name, SV *value);
void _export_function(pTHX_ HV *_export, const char *what, const char *_tag);
void export_constant_char(const char *package, const char *name, const char *_tag, char val);
void export_constant(const char *package, const char *name, const char *_tag, double val);
void set_isa(const char *klass, const char *parent);

#define DumpHex(addr, len) _DumpHex(aTHX_ addr, len, __FILE__, __LINE__)
void _DumpHex(pTHX_ const void *addr, size_t len, const char *file, int line);
#define DD(scalar) _DD(aTHX_ scalar, __FILE__, __LINE__)
void _DD(pTHX_ SV *scalar, const char *file, int line);

const char *type_as_str(int type);
int type_as_dc(int type);

// Affix/Lib.cxx
char *locate_lib(pTHX_ SV *_lib, SV *_ver);
char *mangle(pTHX_ const char *abi, SV *affix, const char *symbol, SV *args);

// Affix::affix(...) and Affix::wrap(...) System
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
    bool _cpp_constructor;
    bool _cpp_const;    // const member function in CPPStruct[]
    bool _cpp_struct;   // CPPStruct[] as this
    SV *_cpp_this_info; // typedef
};

// Callback system
struct CallbackWrapper {
    DCCallback *cb;
};

typedef struct {
    char *sig;
    size_t sig_len;
    char ret;
    char *perl_sig;
    SV *cv;
    AV *arg_info;
    SV *retval;
    dTHXfield(perl)
} Callback;

char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata);

// Type system
__attribute__unused__ XS_INTERNAL(Affix_Type_asint) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    XSRETURN_IV(XSANY.any_i32);
}

// XS Boot
void boot_Affix_pin(pTHX_ CV *);
void boot_Affix_Pointer(pTHX_ CV *);
void boot_Affix_Type_InstanceOf(pTHX_ CV *);
void boot_Affix_Lib(pTHX_ CV *);
void boot_Affix_Type(pTHX_ CV *);
void boot_Affix_Type_InstanceOf(pTHX_ CV *);
void boot_Affix_Aggregate(pTHX_ CV *);
void boot_Affix_Platform(pTHX_ CV *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include <string>

#endif
