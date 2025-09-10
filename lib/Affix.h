#pragma once

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
#ifndef sv_setbool
#define sv_setbool sv_setsv /* new in perl 5.38 */
#endif

#if __WIN32
#include <windows.h>
#endif

#ifdef MULTIPLICITY
#define storeTHX(var) (var) = aTHX
#define dTHXfield(var) tTHX var;
#else
#define storeTHX(var) dNOOP
#define dTHXfield(var)
#endif

// in CORE as of perl 5.40 but I might try to support older perls without ppp
// #if PERL_VERSION_LT(5, 40, 0)
#if PERL_VERSION_MINOR < 40
#define newAV_mortal() MUTABLE_AV(sv_2mortal((SV *)newAV()))
#endif
#define newHV_mortal() MUTABLE_HV(sv_2mortal((SV *)newHV()))

#define hv_existsor(hv, key, _or) hv_exists(hv, key, strlen(key)) ? *hv_fetch(hv, key, strlen(key), 0) : _or

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto) \
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

#define MY_CXT_KEY "Affix::_cxt" XS_VERSION

#include <dyncall/dyncall.h>
#include <dyncall/dyncall_aggregate.h>
#include <dyncall/dyncall_callf.h>
#include <dyncall/dyncall_signature.h>
#include <dyncall/dyncall_value.h>
#include <dyncall/dyncall_version.h>
#include <dyncallback/dyncall_callback.h>
#include <dynload/dynload.h>

typedef struct {
    DCCallVM * cvm;
} my_cxt_t;

#if defined(DC__OS_Win32) || defined(DC__OS_Win64)
#elif defined(DC__OS_MacOS)
#else
#include <dlfcn.h>
//~ #include <iconv.h>
#endif

#include <inttypes.h>
#include <wchar.h>

#ifdef DC__OS_Win64
static const char * dlerror(void) {
    static char buf[1024];
    DWORD dw = GetLastError();
    if (dw == 0)
        return NULL;  // No error
    DWORD result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL,
                                  dw,
                                  0,  // Default language
                                  (LPSTR)buf,
                                  sizeof(buf),
                                  NULL);

    if (result == 0)
        snprintf(buf, sizeof(buf), "Unknown Windows error 0x%u", (unsigned int)dw);
    else {
        // FormatMessage often appends a LF and/or CR
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';
    }
    return buf;
}
#endif  // DC__OS_Win64

#if DEBUG > 1  // Simple location ping. No-op if DEBUG is not high enough.
#define PING warn("Ping at %s line %d", __FILE__, __LINE__);
#else
#define PING ;
#endif

/*
Native argument types used internally by Affix. These match Itanium mangling
which differs from dyncall's sigchars.

https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-builtin
*/
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
#define SIZE_T_FLAG 'o'
#define SSIZE_T_FLAG 'O'
#define FLOAT_FLAG 'f'
#define DOUBLE_FLAG 'd'
#define STDSTRING_FLAG 'Y'  // Placeholder/future for std::string if I go with C++
#define STRUCT_FLAG 'A'
#define CPPSTRUCT_FLAG 'B'
#define UNION_FLAG 'u'
#define POINTER_FLAG 'p'
#define ARRAY_FLAG '@'
#define STRING_FLAG 'Z'
#define WSTRING_FLAG 'z'
#define CV_FLAG '&'
#define SV_FLAG '$'
#define ENUM_FLAG 'E'

// Calling conventions
#define MODE_FLAG '_'         // General mode/modifier flag
#define RESET_FLAG '>'        // Reset calling convention/state
#define DEFAULT_FLAG ':'      // Default calling convention
#define THIS_FLAG '*'         // C++ 'this' pointer for methods
#define ELLIPSIS_FLAG 'e'     // Varargs ellipsis (...)
#define VARARGS_FLAG '.'      // Dyncall varargs (distinct from ellipsis)
#define CDECL_FLAG 'D'        // C Declaration calling convention
#define STDCALL_FLAG 'T'      // Standard Call calling convention (Windows)
#define MSFASTCALL_FLAG '='   // Microsoft FastCall (Windows)
#define GNUFASTCALL_FLAG '3'  // GNU FastCall (Linux/GCC)
#define MSTHIS_FLAG '+'       // Microsoft 'thiscall'
#define GNUTHIS_FLAG '#'      // GNU 'thiscall'
#define ARM_FLAG 'r'          // ARM calling convention
#define THUMB_FLAG 'g'        // ARM Thumb calling convention
#define SYSCALL_FLAG 'H'      // System call convention

/*
Returns the amount of padding needed after `offset` to ensure that the
following address will be aligned to `alignment`.

http://www.catb.org/esr/structure-packing/#_structure_alignment_and_padding
*/
#if 1  // Assumes HAVE_ALIGNOF (GCC/Clang extension)
/* A GCC extension. */
#define alignof(t) __alignof__(t)
#elif defined _MSC_VER
#define alignof(t) __alignof(t)
#else
/* Fallback. Calculate by measuring structure padding. */
#define alignof(t)         \
    ((char *)(&((struct {  \
                   char c; \
                   t _h;   \
               } *)0)      \
                   ->_h) - \
     (char *)0)
#endif

// MEM_ALIGNBYTES is messed up by quadmath and long doubles
#define AFFIX_ALIGNBYTES __alignof__(intptr_t)

/* Some are undefined in perlapi */
#define SIZEOF_BOOL sizeof(bool)
#define SIZEOF_CHAR sizeof(char)
#define SIZEOF_SCHAR sizeof(signed char)
#define SIZEOF_UCHAR sizeof(unsigned char)
#define SIZEOF_WCHAR sizeof(wchar_t)
#define SIZEOF_SHORT sizeof(short)
#define SIZEOF_USHORT sizeof(unsigned short)
#define SIZEOF_INT INTSIZE
#define SIZEOF_UINT sizeof(unsigned int)
#define SIZEOF_LONG sizeof(long)
#define SIZEOF_ULONG sizeof(unsigned long)
#define SIZEOF_LONGLONG sizeof(long long)
#define SIZEOF_ULONGLONG sizeof(unsigned long long)
#define SIZEOF_FLOAT sizeof(float)
#define SIZEOF_DOUBLE sizeof(double)
#define SIZEOF_SIZE_T (sizeof(size_t))
#define SIZEOF_SSIZE_T (sizeof(ssize_t))
#define SIZEOF_INTPTR_T sizeof(intptr_t)
#define SIZEOF_SV sizeof(SV)

#define ALIGNOF_BOOL __alignof__(bool)
#define ALIGNOF_CHAR __alignof__(char)
#define ALIGNOF_SCHAR __alignof__(signed char)
#define ALIGNOF_UCHAR __alignof__(unsigned char)
#define ALIGNOF_WCHAR __alignof__(wchar_t)
#define ALIGNOF_SHORT __alignof__(short)
#define ALIGNOF_USHORT __alignof__(unsigned short)
#define ALIGNOF_INT __alignof__(int)
#define ALIGNOF_UINT __alignof__(unsigned int)
#define ALIGNOF_LONG __alignof__(long)
#define ALIGNOF_ULONG __alignof__(unsigned long)
#define ALIGNOF_LONGLONG __alignof__(long long)
#define ALIGNOF_ULONGLONG __alignof__(unsigned long long)
#define ALIGNOF_FLOAT __alignof__(float)
#define ALIGNOF_DOUBLE __alignof__(double)
#define ALIGNOF_INTPTR_T __alignof__(intptr_t)
#define ALIGNOF_SIZE_T __alignof__(size_t)
#define ALIGNOF_SSIZE_T __alignof__(ssize_t)
#define ALIGNOF_SV __alignof__(SV)

// Forward!
typedef struct _Affix Affix;
typedef struct _Affix_Type Affix_Type;

// Represents a field within an aggregate type (struct/union).
typedef struct {
    char * name;
    Affix_Type * type;
} Affix_Type_Aggregate_Fields;

// Represents a Perl subroutine (CV) acting as a callback from C.
typedef struct {
    SV * cv;
    size_t arg_count;
    I32 cb_context;
    Affix_Type ** args;
    Affix_Type * ret;
    SV * retval;
    dTHXfield(perl)
} Affix_Type_Callback;

// Represents an aggregate C type (struct or union)
typedef struct {
    size_t field_count;
    DCaggr * ag;
    Affix_Type_Aggregate_Fields * fields;
} Affix_Type_Aggregate;

// Represents a C array type.
typedef struct {
    Affix_Type * type;
    size_t length;
} Affix_Type_Array;

// Function pointer typedefs for type-specific operations (weak polymorphism)
typedef void (*handle_store)(pTHX_ Affix_Type *, SV *, DCpointer *);
typedef SV * (*handle_fetch)(pTHX_ Affix_Type *, DCpointer, SV *);
typedef SV * (*handle_call)(pTHX_ Affix *, Affix_Type *, DCCallVM *, DCpointer);
typedef size_t (*handle_pass)(pTHX_ Affix *, Affix_Type *, DCCallVM *, Stack_off_t);
typedef DCsigchar (*handle_cb_call)(pTHX_ Affix_Type *, DCValue *, SV *);  // XXX: INCOMPLETE
typedef SV * (*handle_cb_pass)(pTHX_ Affix_Type *, DCArgs *);

// The goods.
typedef struct _Affix_Type {
    char type;
    DCsigchar dc_type;
    size_t size, align, offset;
    union {
        Affix_Type * pointer_type;
        Affix_Type_Array * array_type;
        Affix_Type_Aggregate * aggregate_type;  // For hashes and structs
        Affix_Type_Callback * callback_type;
    } data;
    //
    handle_store store;
    handle_fetch fetch;
    handle_call call;
    handle_pass pass;
    handle_cb_call cb_call;
    handle_cb_pass cb_pass;
} Affix_Type;

// No, wait, this is the goods.
typedef struct _Affix {
    size_t lvalue_count, args;
    DCpointer symbol;
    Affix_Type ** push;
    Affix_Type * pop;
    DCpointer * lvalues;
    Stack_off_t instructions;
} Affix;

// Represents a managed pointer, typically for a C data structure or actual pointer
typedef struct {
    size_t count;
    size_t position;
    DCpointer address;
    Affix_Type * type;
} Affix_Pointer;

// Affix.c
void destroy_affix(pTHX_ Affix * affix);

// Type.c
#define AFFIX_PUSH(Type) \
    STATIC size_t _pass_##Type(pTHX_ Affix * affix, Affix_Type * type, DCCallVM * cvm, Stack_off_t st)
#define AFFIX_CALL(Type) \
    STATIC SV * _call_##Type(pTHX_ Affix * affix, Affix_Type * type, DCCallVM * cvm, DCpointer entrypoint)
#define CALLBACK_PUSH(Type) STATIC SV * _cb_pass_##Type(pTHX_ Affix_Type * type, DCArgs * args)
#define CALLBACK_CALL(Type) STATIC DCsigchar _cb_call_##Type(pTHX_ Affix_Type * type, DCValue * result, SV * sv)
#define POINTER_STORE(Type) STATIC void _store_##Type(pTHX_ Affix_Type * type, SV * data, DCpointer * target)
#define POINTER_FETCH(Type) STATIC SV * _fetch_##Type(pTHX_ Affix_Type * type, DCpointer data, SV * sv)
Affix_Type * new_Affix_Type(pTHX_ SV *);
void destroy_Affix_Type(pTHX_ Affix_Type *);
Affix_Type * _reset();
Affix_Type * _mode();
Affix_Type * _aggr();
DCsigchar _handle_CV(DCCallback *, DCArgs *, DCValue *, DCpointer);

// pin.c
typedef struct {
    DCpointer pointer;
    Affix_Type * type;
    bool managed;
} Affix_Pin;
bool is_pin(pTHX_ SV *);
DCpointer get_pin_pointer(pTHX_ SV * sv);
Affix_Pin * get_pin(pTHX_ SV * sv);
void pin(pTHX_ Affix_Type * type, SV * sv, DCpointer ptr);

// wchar_t.c
wchar_t * utf2wchar(pTHX_ SV *, size_t);
SV * wchar2utf(pTHX_ wchar_t *, size_t);

// utils.c
SV * gen_dualvar(pTHX_ IV, const char *);
#define export_function(package, what, tag) \
    _export_function(aTHX_ get_hv(form("%s::EXPORT_TAGS", package), GV_ADD), what, tag)
void register_constant(const char * package, const char * name, SV * value);
void _export_function(pTHX_ HV * _export, const char * what, const char * _tag);
void export_constant_char(const char * package, const char * name, const char * _tag, char val);
void export_constant(const char * package, const char * name, const char * _tag, double val);
void set_isa(const char * klass, const char * parent);
DCsigchar atype_to_dtype(char);
const char * dump_Affix_Type(pTHX_ Affix_Type * type);

#define DumpHex(addr, len) _DumpHex(aTHX_ addr, len, __FILE__, __LINE__)
void _DumpHex(pTHX_ const void * addr, size_t len, const char * file, int line);
#define DD(scalar) _DD(aTHX_ scalar, __FILE__, __LINE__)
void _DD(pTHX_ SV * scalar, const char * file, int line);

// XS Boot
void boot_Affix_Platform(pTHX_ CV *);
void boot_Affix_Pointer(pTHX_ CV *);
void boot_Affix_pin(pTHX_ CV *);
void boot_Affix_Type(pTHX_ CV *);
//~ void boot_Affix_Lib(pTHX_ CV *);
