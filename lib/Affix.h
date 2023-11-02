#ifndef AFFIX_H_SEEN
#define AFFIX_H_SEEN

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

/* Native argument types (core types match Itanium mangling) */
#define VOID_FLAG 'v'
#define BOOL_FLAG 'b'
#define SCHAR_FLAG 'a'
#define CHAR_FLAG 'c'
#define UCHAR_FLAG 'h'
#define WCHAR_FLAG 'w'
#define SHORT_FLAG 's'
#define USHORT_FLAG 't'
#define INT_FLAG 'i'
#define UINT_FLAG 'j'
#define LONG_FLAG 'l'
#define ULONG_FLAG 'm'
#define LONGLONG_FLAG 'x'
#define ULONGLONG_FLAG 'y'
#if Size_t_size == INTSIZE
#define SSIZE_T_FLAG AFFIX_TYPE_INT
#define SIZE_T_FLAG AFFIX_TYPE_UINT
#elif Size_t_size == LONGSIZE
#define SSIZE_T_FLAG LONG_FLAG
#define SIZE_T_FLAG ULONG_FLAG
#elif Size_t_size == LONGLONGSIZE
#define SSIZE_T_FLAG LONGLONG_FLAG
#define SIZE_T_FLAG ULONGLONG_FLAG
#else // quadmath is broken
#define SSIZE_T_FLAG LONGLONG_FLAG
#define SIZE_T_FLAG ULONGLONG_FLAG
#endif
#define FLOAT_FLAG 'f'
#define DOUBLE_FLAG 'd'
#define STRING_FLAG 'z'
#define WSTRING_FLAG '<'
#define STDSTRING_FLAG 'Y'
#define STRUCT_FLAG 'A'
#define CPPSTRUCT_FLAG 'B'
#define UNION_FLAG 'u'
#define ARRAY_FLAG '@'
#define CODEREF_FLAG '&'
#define POINTER_FLAG '\\'
#define SV_FLAG '?'

// Calling conventions
#define RESET_FLAG '>' // DC_SIGCHAR_CC_DEFAULT
#define THIS_FLAG '*'
#define ELLIPSIS_FLAG 'e'
#define VARARGS_FLAG '.'
#define DCECL_FLAG 'D'
#define STDCALL_FLAG 'T'
#define MSFASTCALL_FLAG '='
#define GNUFASTCALL_FLAG '3'
#define MSTHIS_FLAG '+'
#define GNUTHIS_FLAG '#'
#define ARM_FLAG 'r'
#define THUMB_FLAG 'g'
#define SYSCALL_FLAG 'H'

#define CONST_FLAG 'K'

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

// MEM_ALIGNBYTES is messed up by quadmath and long doubles
#define AFFIX_ALIGNBYTES 8

/* Some are undefined in perlapi */
#define BOOL_SIZE sizeof(bool) // ha!
#define CHAR_SIZE sizeof(char)
#define UCHAR_SIZE sizeof(unsigned char)
#define WCHAR_SIZE sizeof(wchar_t)
#define SHORT_SIZE sizeof(short)
#define USHORT_SIZE sizeof(unsigned short)
#define INT_SIZE INTSIZE
#define UINT_SIZE sizeof(unsigned int)
#define LONG_SIZE sizeof(long)
#define ULONG_SIZE sizeof(unsigned long)
#define LONGLONG_SIZE sizeof(long long)
#define ULONGLONG_SIZE sizeof(unsigned long long)
#define FLOAT_SIZE sizeof(float)
#define DOUBLE_SIZE sizeof(double) // ugh...
#if Size_t_size == INTSIZE
#define SIZE_T_SIZE INT_SIZE
#define SSIZE_T_SIZE UINT_SIZE
#elif Size_t_size == LONGSIZE
#define SIZE_T_SIZE LONGLONG_SIZE
#define SSIZE_T_SIZE ULONG_SIZE
#elif Size_t_size == LONGLONGSIZE
#define SIZE_T_SIZE ULONGLONG_SIZE
#define SSIZE_T_SIZE LONGLONG_SIZE
#else // quadmath is broken
#define SIZE_T_SIZE ULONGLONG_SIZE
#define SSIZE_T_SIZE LONGLONG_SIZE
#endif
#define INTPTR_T_SIZE sizeof(intptr_t) // ugh...

#define BOOL_ALIGN ALIGNOF(bool)
#define CHAR_ALIGN ALIGNOF(char)
#define UCHAR_ALIGN ALIGNOF(unsigned char)
#define WCHAR_ALIGN ALIGNOF(wchar_t)
#define SHORT_ALIGN ALIGNOF(short)
#define USHORT_ALIGN ALIGNOF(unsigned short)
#define INT_ALIGN ALIGNOF(int)
#define UINT_ALIGN ALIGNOF(unsigned int)
#define LONG_ALIGN ALIGNOF(long)
#define ULONG_ALIGN ALIGNOF(unsigned long)
#define LONGLONG_ALIGN ALIGNOF(long long)
#define ULONGLONG_ALIGN ALIGNOF(unsigned long long)
#define FLOAT_ALIGN ALIGNOF(float)
#define DOUBLE_ALIGN ALIGNOF(double)
#define INTPTR_T_ALIGN ALIGNOF(intptr_t)
#define SIZE_T_ALIGN ALIGNOF(size_t)
#define SSIZE_T_ALIGN ALIGNOF(ssize_t)

// [ text, id, size, align, offset, subtype, length, aggregate, typedef ]
#define SLOT_STRINGIFY 0
#define SLOT_NUMERIC 1
#define SLOT_SIZEOF 2
#define SLOT_ALIGNMENT 3
#define SLOT_OFFSET 4
#define SLOT_SUBTYPE 5
#define SLOT_ARRAYLEN 6
#define SLOT_AGGREGATE 7
#define SLOT_TYPEDEF 8
#define SLOT_CAST 9

#define AXT_STRINGIFY(t) SvPV_nolen(*av_fetch(MUTABLE_AV(SvRV(t)), SLOT_STRINGIFY, 0))
#define AXT_NUMERIC(t) (char)SvIV(*av_fetch(MUTABLE_AV(SvRV(t)), SLOT_NUMERIC, 0))
#define AXT_SIZEOF(t) SvIV(*av_fetch(MUTABLE_AV(SvRV(t)), SLOT_SIZEOF, 0))
#define AXT_ALIGN(t) SvIV(*av_fetch(MUTABLE_AV(SvRV(t)), SLOT_ALIGNMENT, 0))
#define AXT_OFFSET(t) SvIV(*av_fetch(MUTABLE_AV(SvRV(t)), SLOT_OFFSET, 0))
#define AXT_SUBTYPE(t) *av_fetch(MUTABLE_AV(SvRV(t)), SLOT_SUBTYPE, 0)
#define AXT_ARRAYLEN(t) SvIV(*av_fetch(MUTABLE_AV(SvRV(t)), SLOT_ARRAYLEN, 0))
#define AXT_AGGREGATE(t) av_fetch(MUTABLE_AV(SvRV(t)), SLOT_AGGREGATE, 0)
#define AXT_TYPEDEF(t) av_fetch(MUTABLE_AV(SvRV(t)), SLOT_TYPEDEF, 0)
#define AXT_CAST(t) av_fetch(MUTABLE_AV(SvRV(t)), SLOT_CAST, 0)
// marshal.cxx
size_t padding_needed_for(size_t offset, size_t alignment);
SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv);
DCpointer sv2ptr(pTHX_ SV *type_sv, SV *data);

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

int type_as_dc(int type); // TODO: Find a better place for this

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
    void **arg_info;
    DCaggr **aggregates;
    SV *ret_info;
    DCaggr *ret_aggregate;
    bool ellipsis; // varargs or ellipsis
    bool _cpp_constructor;
    bool _cpp_const;    // const member function in CPPStruct[]
    bool _cpp_struct;   // CPPStruct[] as this
    SV *_cpp_this_info; // typedef
    void **temp_ptrs;
    void *ret_ptr; // in case we need it
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

// XS Boot
void boot_Affix_pin(pTHX_ CV *);
void boot_Affix_Pointer(pTHX_ CV *);
void boot_Affix_Lib(pTHX_ CV *);
void boot_Affix_Aggregate(pTHX_ CV *);
void boot_Affix_Platform(pTHX_ CV *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include <string>

#endif // AFFIX_H_SEEN
