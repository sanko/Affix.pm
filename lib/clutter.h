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
#define DC_SIGCHAR_CODE '&'    // 'p' but allows us to wrap CV * for the user
#define DC_SIGCHAR_ARRAY '['   // 'A' but nicer
#define DC_SIGCHAR_STRUCT '{'  // 'A' but nicer
#define DC_SIGCHAR_UNION '<'   // 'A' but nicer
#define DC_SIGCHAR_BLESSED '$' // 'p' but an object or subclass of a given package
#define DC_SIGCHAR_ANY '*'     // 'p' but it's really an SV/HV/AV
#define DC_SIGCHAR_ENUM 'e'    // 'i' but with multiple options
#define DC_SIGCHAR_MAYBE '?'   // optional praam multiple options
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
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

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

static HV *ptr2perl(DCpointer ptr, AV *fields) {
    dTHX;

    if (av_count(fields) % 2)
        Perl_croak_nocontext("%s: %s is not an even sized list", "Dyn::Call::Pointer::perl",
                             "fields");
    /*DCpointer *  pointer;

  // Dyn::Call | DCCallVM * | DCCallVMPtr
  if (sv_derived_from(ST(0), "Dyn::Call::Pointer")){
    IV tmp = SvIV((SV*)SvRV(ST(0)));
    vm = INT2PTR(DCpointer *, tmp);
  }
  else
    croak("obj is not of type Dyn::Call::Pointer");*/

    // DumpHex(obj, sizeof(obj));

    // SV ** classinfo_ref = hv_fetch(MY_CXT.structs, pkg, strlen(pkg), 0);

    // HV * action;
    // if (classinfo_ref && SvOK(*classinfo_ref))
    //     action = MUTABLE_HV(SvRV(*classinfo_ref));
    // else
    //     croak("Attempt to cast a pointer to %s which is undefined", pkg);
    HV *RETVAL = newHV_mortal();
    size_t offset = 0;

    for (int i = 0; i < av_count(fields); i += 2) {
        DCpointer holder = NULL;
        SV **name_ref = av_fetch(fields, i, 0);
        SV **type_ref = av_fetch(fields, i + 1, 0);

        char *name = (char *)SvPV_nolen(*name_ref);
        char type = (char)*SvPV_nolen(*type_ref);
        SV *element;

        switch (type) {
        case DC_SIGCHAR_CHAR: {
            char *holder = (char *)safemalloc(1);
            Copy((DCpointer)(PTR2IV(ptr) + offset), holder, 1, char);
            offset += padding_needed_for(offset, 1);
            element = newSVpv(holder, 1);
            break;
        }
        case DC_SIGCHAR_UCHAR: {
            char *holder = (char *)safemalloc(1);
            Copy((DCpointer)(PTR2IV(ptr) + offset), holder, 1, unsigned char);
            offset += padding_needed_for(offset, 1);
            element = newSVpv(holder, 1);
            break;
        }
        default:
            Perl_croak_nocontext("%s: %c is not a known signature character",
                                 "Dyn::Call::Pointer::perl", type);
            break;
        }
        hv_store(RETVAL, name, strlen(name), element, 0);
        if (holder) safefree(holder);
    }
    return RETVAL;
}

static SV *ptr2sv(pTHX_ DCpointer ptr, SV *type) {
    SV *RETVAL = newSV(0);
    char *_type = SvPV_nolen(type);
    switch (_type[0]) {
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
        sv_setiv(RETVAL, *(long long *)ptr);
        break;
    case DC_SIGCHAR_ULONGLONG:
        sv_setuv(RETVAL, *(unsigned long long *)ptr);
        break;
    case DC_SIGCHAR_FLOAT:
        sv_setnv(RETVAL, *(float *)ptr);
        break;
    case DC_SIGCHAR_DOUBLE:
        sv_setnv(RETVAL, *(double *)ptr);
        break;
    default:
        croak("Oh, this is unexpected [%c]", _type[0]);
    }
    return RETVAL;
}

static SV *agg2sv(DCaggr *agg, SV *type, DCpointer data, size_t size) {
    dTHX;
    // sv_dump(aTHX_ sv);
    // sv_dump(aTHX_ SvRV(*hv_fetch(MUTABLE_HV(sv), "fields", 6, 0)));
    AV *fields = MUTABLE_AV(SvRV(*hv_fetch(MUTABLE_HV(type), "fields", 6, 0)));
    HV *RETVAL = newHV();
    //*(int*)ptr = 42;
    intptr_t offset;
    DCsize i = agg->n_fields;
    // warn("agg->n_fields == %d", i);
    DCpointer me = safemalloc(0);
    for (int i = 0; i < agg->n_fields; ++i) {
        // warn("i==%d type==%c", i, agg->fields[i].type);
        SV **field = av_fetch(fields, i, 0);
        SV **name_ptr = av_fetch(MUTABLE_AV(*field), 0, 0);
        // sv_dump(*name_ptr);
        offset = PTR2IV(data) + agg->fields[i].offset;
        //
        /*warn("field offset: %ld", agg->fields[i].offset);
        warn("field size: %ld", agg->fields[i].size);
        warn("field alignment: %ld", agg->fields[i].alignment);
        warn("field array_len: %ld", agg->fields[i].array_len);
        warn("field type: %c", agg->fields[i].type);*/

        // 	DCsize offset, size, alignment, array_len;
        me = saferealloc(me, agg->fields[i].size * agg->fields[i].array_len);

        // sv_dump(*field);
        switch (agg->fields[i].type) {
        case DC_SIGCHAR_BOOL:
            Copy(offset, me, agg->fields[i].array_len, bool);
            hv_store_ent(RETVAL, *name_ptr, boolSV(*(bool *)me), 0);
            break;
        case DC_SIGCHAR_CHAR:
            Copy(offset, me, agg->fields[i].array_len, char);
            if (agg->fields[i].array_len == 1)
                hv_store_ent(RETVAL, *name_ptr, newSViv(*(char *)me), 0);
            else
                hv_store_ent(RETVAL, *name_ptr, newSVpv((char *)me, agg->fields[i].array_len), 0);
            break;
        case DC_SIGCHAR_UCHAR:
            Copy(offset, me, agg->fields[i].array_len, unsigned char);
            if (agg->fields[i].array_len == 1)
                hv_store_ent(RETVAL, *name_ptr, newSVuv(*(unsigned char *)me), 0);
            else
                hv_store_ent(RETVAL, *name_ptr,
                             newSVpv((char *)(unsigned char *)me, agg->fields[i].array_len), 0);
            break;
        case DC_SIGCHAR_SHORT:
            Copy(offset, me, agg->fields[i].array_len, short);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(short *)me), 0);
            break;
        case DC_SIGCHAR_USHORT:
            Copy(offset, me, agg->fields[i].array_len, unsigned short);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(unsigned short *)me), 0);
            break;
        case DC_SIGCHAR_INT:
            Copy(offset, me, agg->fields[i].array_len, int);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(int *)me), 0);
            break;
        case DC_SIGCHAR_UINT:
            Copy(offset, me, agg->fields[i].array_len, int);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(int *)me), 0);
            break;
        case DC_SIGCHAR_LONG:
            Copy(offset, me, agg->fields[i].array_len, long);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(long *)me), 0);
            break;
        case DC_SIGCHAR_ULONG:
            Copy(offset, me, agg->fields[i].array_len, unsigned long);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(unsigned long *)me), 0);
            break;
        case DC_SIGCHAR_LONGLONG:
            Copy(offset, me, agg->fields[i].array_len, long long);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(long long *)me), 0);
            break;
        case DC_SIGCHAR_ULONGLONG:
            Copy(offset, me, agg->fields[i].array_len, unsigned long long);
            hv_store_ent(RETVAL, *name_ptr, newSViv(*(unsigned long long *)me), 0);
            break;
        case DC_SIGCHAR_FLOAT:
            Copy(offset, me, agg->fields[i].array_len, float);
            hv_store_ent(RETVAL, *name_ptr, newSVnv(*(float *)me), 0);
            break;
        case DC_SIGCHAR_DOUBLE:
            Copy(offset, me, agg->fields[i].array_len, double);
            hv_store_ent(RETVAL, *name_ptr, newSVnv(*(double *)me), 0);
            break;
        case DC_SIGCHAR_POINTER: {
            Copy(offset, me, agg->fields[i].array_len, void *);
            SV *RETVALSV = newSV(0); // sv_newmortal();
            sv_setref_pv(RETVALSV, "Dyn::Call::Pointer", me);
            hv_store_ent(RETVAL, *name_ptr, RETVALSV, 0);
        } break;
        case DC_SIGCHAR_STRING: {
            Copy(offset, me, agg->fields[i].array_len, char **);
            hv_store_ent(RETVAL, *name_ptr, newSVpv(*(char **)me, 0), 0);
            if (me != NULL) hv_store_ent(RETVAL, *name_ptr, newSVpv(*(char **)me, 0), 0);
        } break;
        case DC_SIGCHAR_AGGREGATE: {
            SV **type_ptr = av_fetch(MUTABLE_AV(*field), 1, 0);
            Copy(offset, me, agg->fields[i].size * agg->fields[i].array_len, char);
            SV *kid = agg2sv((DCaggr *)agg->fields[i].sub_aggr, SvRV(*type_ptr), me,
                             agg->fields[i].size * agg->fields[i].array_len);
            hv_store_ent(RETVAL, *name_ptr, kid, 0);
        } break;
        default:
            warn("TODO: %c", agg->fields[i].type);
            hv_store_ent(RETVAL, *name_ptr,
                         newSVpv(form("Unhandled type: %c", agg->fields[i].type), 0), 0);
            break;
        }
    }

    safefree(me);

    {
        SV *RETVALSV;
        RETVALSV = newRV((SV *)RETVAL);
        // RETVALSV = sv_2mortal(RETVALSV);
        return RETVALSV;
    }
}
