#ifndef AFFIX_H_SEEN
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

//~ #include "ppport.h"

#ifndef sv_setbool_mg
#define sv_setbool_mg(sv, b) sv_setsv_mg(sv, boolSV(b)) /* new in perl 5.36 */
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

#ifdef DEBUG
#define PING warn("Ping at %s line %d", __FILE__, __LINE__);
#else
#define PING ;
#endif

/* Native argument types */
#define AFFIX_ARG_SV 100
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

// marshal.cxx
size_t padding_needed_for(size_t offset, size_t alignment);
SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv);
void *sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed);
size_t _alignof(pTHX_ SV *type);
size_t _sizeof(pTHX_ SV *type);
size_t _offsetof(pTHX_ SV *type);

// wchar_t.cxx
SV *wchar2utf(pTHX_ wchar_t *src, int len);
wchar_t *utf2wchar(pTHX_ SV *src, int len);

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
    AV *args;
    SV *retval;
    dTHXfield(perl)
} Callback;

char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata);

// XS Boot
void boot_Affix_Pointer(pTHX_ CV *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
