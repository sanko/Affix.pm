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

#include <dyncall_value.h>
#include <dyncall_callf.h>

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

const char *file = __FILE__;

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

SV *enum2sv(SV *type, int in) {
    dTHX;
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
    // sv_dump(type);
    char *str = SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
    // warn("str == %s", str);
    switch (str[0]) {
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
    case DC_SIGCHAR_STRUCT: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6))
            return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        size_t size = 0;
        SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);
        SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
        bool packed = SvTRUE(*sv_packed);
        AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
        int field_count = av_count(idk_arr);
        for (int i = 0; i < field_count; ++i) {
            SV **type_ptr = av_fetch(MUTABLE_AV(*av_fetch(idk_arr, i, 0)), 1, 0);
            size_t __sizeof = _sizeof(aTHX_ * type_ptr);
            hv_stores(MUTABLE_HV(SvRV(*type_ptr)), "offset", newSViv(size));
            if (!packed) size += padding_needed_for(size, __sizeof);
            size += __sizeof;
        }
        hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(size));
        return size;
    }
    case DC_SIGCHAR_ARRAY: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6))
            return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        SV **type_ptr = hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0);
        SV **size_ptr = hv_fetchs(MUTABLE_HV(SvRV(type)), "size", 0);
        size_t size = _sizeof(aTHX_ * type_ptr) * SvIV(*size_ptr);
        hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(size));
        return size;
    }
    case DC_SIGCHAR_UNION: {
        if (hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6))
            return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        size_t size = 0, this_size;
        SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);
        AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
        for (int i = 0; i < av_count(idk_arr); ++i) {
            SV **type_ptr = av_fetch(MUTABLE_AV(*av_fetch(idk_arr, i, 0)), 1, 0);
            hv_stores(MUTABLE_HV(SvRV(*type_ptr)), "offset", newSViv(0));
            this_size = _sizeof(aTHX_ * type_ptr);
            hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(this_size));
            if (size < this_size) size = this_size;
        }
        hv_stores(MUTABLE_HV(SvRV(type)), "sizeof", newSViv(size));
        return size;
    }
    case DC_SIGCHAR_CODE: // automatically wrapped in a DCCallback pointer
    case DC_SIGCHAR_POINTER:
    case DC_SIGCHAR_STRING:
    case DC_SIGCHAR_BLESSED:
        return PTRSIZE;
    case DC_SIGCHAR_ANY:
        return sizeof(SV);
    default:
        croak("Failed to gather sizeof info for unknown type: %s", str);
        return -1;
    }
}

static DCaggr *_aggregate(pTHX_ SV *type) {
    // warn("here at %s line %d", __FILE__, __LINE__);
    // sv_dump(type);

    char *str = SvPVbytex_nolen(type); // stringify to sigchar; speed cheat vs sv_derived_from(...)
                                       // warn("here at %s line %d", __FILE__, __LINE__);

    size_t size = _sizeof(aTHX_ type);
    // warn("here at %s line %d", __FILE__, __LINE__);

    switch (str[0]) {
    case DC_SIGCHAR_STRUCT:
    case DC_SIGCHAR_UNION: {
        // warn("here at %s line %d", __FILE__, __LINE__);

        if (hv_exists(MUTABLE_HV(SvRV(type)), "aggregate", 9)) {
            SV *__type = *hv_fetchs(MUTABLE_HV(SvRV(type)), "aggregate", 0);
            // warn("here at %s line %d", __FILE__, __LINE__);
            // sv_dump(__type);

            // return SvIV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "aggregate", 0));
            if (sv_derived_from(__type, "Dyn::Call::Aggregate")) {
                // warn("here at %s line %d", __FILE__, __LINE__);

                HV *hv_ptr = MUTABLE_HV(__type);
                // warn("here at %s line %d", __FILE__, __LINE__);

                IV tmp = SvIV((SV *)SvRV(__type));
                // warn("here at %s line %d", __FILE__, __LINE__);

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
            // warn("DCaggr *agg = dcNewAggr(%d, %d); at %s line %d", field_count, size, __FILE__,
            //     __LINE__);
            DCaggr *agg = dcNewAggr(field_count, size);
            for (int i = 0; i < field_count; ++i) {
                SV **type_ptr = av_fetch(MUTABLE_AV(*av_fetch(idk_arr, i, 0)), 1, 0);
                size_t __sizeof = _sizeof(aTHX_ * type_ptr);
                size_t offset = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "offset", 0));
                char *str = SvPVbytex_nolen(*type_ptr);
                switch (str[0]) {
                case DC_SIGCHAR_AGGREGATE:
                case DC_SIGCHAR_STRUCT:
                case DC_SIGCHAR_UNION: {
                    warn("here at %s line %d", __FILE__, __LINE__);

                    DCaggr *child = _aggregate(aTHX_ * type_ptr);

                    dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                    // void dcAggrField(DCaggr* ag, DCsigchar type, DCint offset, DCsize array_len,
                    // ...)
                    //  dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                    warn("dcAggrField(agg, DC_SIGCHAR_AGGREGATE, %d, 1, child); at %s line %d",
                         offset, __FILE__, __LINE__);
                } break;
                case DC_SIGCHAR_ARRAY: {
                    // sv_dump(*type_ptr);
                    SV *type = *hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "type", 0);
                    int array_len = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "size", 0));
                    char *str = SvPVbytex_nolen(type);
                    dcAggrField(agg, str[0], offset, array_len);
                    warn("dcAggrField(agg, %c, %zd, %d); at %s line %d", str[0], offset, array_len,
                         __FILE__, __LINE__);
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

SV *agg2sv(pTHX_ DCaggr *agg, SV *type, DCpointer data, size_t size);

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type) {
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
    case DC_SIGCHAR_ARRAY:
    case DC_SIGCHAR_STRUCT:
    case DC_SIGCHAR_UNION: {
        DCaggr *agg = _aggregate(aTHX_ type);
        size_t si = _sizeof(aTHX_ type);
        RETVAL = agg2sv(aTHX_ agg, SvRV(type), ptr, si);
    } break;
    default:
        croak("Oh, this is unexpected: %c", _type[0]);
    }
    return RETVAL;
}

SV *agg2sv(pTHX_ DCaggr *agg, SV *type, DCpointer data, size_t size) {
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
        SV **value_ptr = av_fetch(MUTABLE_AV(*field), 1, 0);

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
            SV *kid = agg2sv(aTHX_(DCaggr *) agg->fields[i].sub_aggr, SvRV(*type_ptr), me,
                             agg->fields[i].size * agg->fields[i].array_len);
            hv_store_ent(RETVAL, *name_ptr, kid, 0);
        } break;
        case DC_SIGCHAR_ENUM: {
            Copy(offset, me, agg->fields[i].array_len, int);
            hv_store_ent(RETVAL, *name_ptr, enum2sv(*value_ptr, *(int *)me), 0);
            break;
        }
        case DC_SIGCHAR_ENUM_UINT: {
            Copy(offset, me, agg->fields[i].array_len, unsigned int);
            hv_store_ent(RETVAL, *name_ptr, enum2sv(*value_ptr, *(unsigned int *)me), 0);
            break;
        }
        case DC_SIGCHAR_ENUM_CHAR: {
            Copy(offset, me, agg->fields[i].array_len, char);
            hv_store_ent(RETVAL, *name_ptr, enum2sv(*value_ptr, *(char *)me), 0);
            break;
        }
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

static DCaggr *sv2ptr(pTHX_ SV *type, SV *data, DCpointer ptr, bool packed, size_t pos) {
    // void *RETVAL;
    // Newxz(RETVAL, 1024, char);

    // warn("pos == %p", pos);

    // if(SvROK(type))
    //     type = SvRV(type);
    // sv_dump(type);

    char *str = SvPVbytex_nolen(type);
    // warn("[c] type: %s, offset: %d", str, pos);
    switch (str[0]) {
    case DC_SIGCHAR_CHAR: {
        char *value = SvPV_nolen(data);
        Copy(value, ptr, 1, char);
        // return I8SIZE;
    } break;
    case DC_SIGCHAR_STRUCT: {
        // sv_dump(type);
        // sv_dump(data);
        //  if (SvTYPE(SvRV(data)) != SVt_PVHV) croak("Expected a hash reference");
        size_t size = _sizeof(aTHX_ type);
        // warn("STRUCT! size: %d", size);
        DCaggr *retval = _aggregate(aTHX_ type);

        HV *hv_type = MUTABLE_HV(SvRV(type));
        HV *hv_data = MUTABLE_HV(SvRV(data));
        // sv_dump(MUTABLE_SV(hv_type));

        SV **sv_fields = hv_fetchs(hv_type, "fields", 0);
        SV **sv_packed = hv_fetchs(hv_type, "packed", 0);

        AV *av_fields = MUTABLE_AV(SvRV(*sv_fields));
        int field_count = av_count(av_fields);

        // warn("field_count [%d]", field_count);

        // warn("size [%d]", size);

        // DumpHex(ptr, size);
        // warn("here at %s line %d", __FILE__, __LINE__);
        // warn("here at %s line %d", __FILE__, __LINE__);

        for (int i = 0; i < field_count; ++i) {
            // warn("here [%d] at %s line %d", i, __FILE__, __LINE__);

            SV **field = av_fetch(av_fields, i, 0);

            AV *key_value = MUTABLE_AV((*field));
            // //sv_dump( MUTABLE_SV((*field)));
            // warn("here at %s line %d", __FILE__, __LINE__);

            SV **name_ptr = av_fetch(key_value, 0, 0);
            SV **type_ptr = av_fetch(key_value, 1, 0);
            char *key = SvPVbytex_nolen(*name_ptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

            // SV * type = *type_ptr;
            // warn("key[%d] %s", i, key);
            // warn("val[%d] %s", i, SvPVbytex_nolen(val));

            if (!hv_exists(hv_data, key, strlen(key)))
                croak("Expected key %s does not exist in given data", key);
            // warn("here at %s line %d", __FILE__, __LINE__);

            SV **_data = hv_fetch(hv_data, key, strlen(key), 0);
            // warn("here at %s line %d", __FILE__, __LINE__);

            char *type = SvPVbytex_nolen(*type_ptr);
            // warn("here at %s line %d", __FILE__, __LINE__);

            // warn("Added %c:'%s' in slot %lu at %s line %d", type[0], key, pos, __FILE__,
            // __LINE__);

            size_t el_len = _sizeof(aTHX_ * type_ptr);

            if (SvOK(data) || SvOK(SvRV(data)))
                sv2ptr(aTHX_ * type_ptr, *(hv_fetch(hv_data, key, strlen(key), 0)),
                       ((DCpointer)(PTR2IV(ptr) + pos)), packed, pos);
            /*
                        warn("padding needed: %l for size of %d at %s line %d",
                             padding_needed_for(PTR2IV(ptr), _sizeof(aTHX_ * type_ptr)),
                             _sizeof(aTHX_ * type_ptr), __FILE__, __LINE__);*/
            pos += el_len;

            /*
            if (!packed)
                pos += padding_needed_for(pos, _sizeof(aTHX_ * type_ptr));
            else
                pos += _sizeof(aTHX_ * type_ptr);*/

            // warn("value of pos is %d at %s line %d", pos, __FILE__, __LINE__);

            // warn("dcAggrField(*agg, DC_SIGCHAR_INT, %d, 1);", pos);
            // dcAggrField(retval, DC_SIGCHAR_INT, 0, 1);

            // //sv_dump(*_data);

            // DumpHex(ptr, size);
        }
        // DumpHex(ptr, pos);
        dcCloseAggr(retval);
        // warn("     dcCloseAggr(agg);");

        // dcAggrField(retval, DC_SIGCHAR_AGGREGATE, pos, 1, retval);
        // warn ("     dcAggrField(*agg, DC_SIGCHAR_AGGREGATE, %d, %d, me);", pos, 1);

        /*
                size_t av_count = av_count(av);
                HV *hash = MUTABLE_HV(data);
                int pos = 0;
                for (int i = 0; i < av_count; i++) {
                    warn("i == %d", i);
                    SV **field_ptr = av_fetch(av, i, 0);
                    AV *field = MUTABLE_AV(*field_ptr);
                    SV **name_ptr = av_fetch(field, 0, 0);
                    SV **type_ptr = av_fetch(field, 1, 0);
                    //sv_dump(*type_ptr);
                    // HeVAL(hv_fetch_ent(MUTABLE_HV(data), *name_ptr, 0, 0)) =
                    // MUTABLE_SV(newAV_mortal());//sv2ptr(*type_ptr, data);
                    // //sv_dump(data);
                    warn("HERE");
                    //sv_dump(*name_ptr);
                    const char *name = SvPV_nolen(*name_ptr);
                    warn("name == %s", name);
                    HV *data_hv = MUTABLE_HV(SvRV(data));
                    warn("fdsafdasfdasfdsa");
                    // return data;

                    SV **idk_wtf = hv_fetch(data_hv, name, strlen(name), 0);

                    //sv_dump(*idk_wtf);

                    // SV * value = sv2ptr(*type_ptr,

                    // const char * _type = SvPVbytex_nolen(type);
                    // SV ** target = hv_fetch(hash, _type, 0, 0);
                    // //sv_dump(*target);

                    pos = sv2ptr(*type_ptr, *idk_wtf, ptr, packed);

                    // //sv_dump(SvRV(field));
                    //  SV * in = sv2ptr((field), data);
                }
                // IV tmp = SvIV((SV*)SvRV(ST(0)));
                // ptr = INT2PTR(DCpointer *, tmp);
                */
        return retval;
    } break;
    case DC_SIGCHAR_ARRAY: {

        // sv_dump(data);
        if (SvTYPE(SvRV(data)) != SVt_PVAV) croak("Expected an array");
        int spot = 1;
        AV *elements = MUTABLE_AV(SvRV(data));
        SV *pointer;
        HV *hv_ptr = MUTABLE_HV(SvRV(type));
        SV **type_ptr = hv_fetchs(hv_ptr, "type", 0);
        SV **size_ptr = hv_fetchs(hv_ptr, "size", 0);
        // //sv_dump(*type_ptr);
        // //sv_dump(*size_ptr);
        size_t av_len = av_count(elements);
        if (SvOK(*size_ptr)) {
            size_t tmp = SvIV(*size_ptr);
            if (av_len != tmp) croak("Expected and array of %ul elements; found %d", tmp, av_len);
        }
        size_t el_len = _sizeof(aTHX_ * type_ptr);

        for (int i = 0; i < av_len; ++i) {
            sv2ptr(aTHX_ * type_ptr, *(av_fetch(elements, i, 0)), ((DCpointer)(PTR2IV(ptr) + pos)),
                   packed, pos);
            pos += (el_len);
        }

        // return _sizeof(aTHX_ type);
    }
    // croak("ARRAY!");

    break;
    case DC_SIGCHAR_CODE:
        croak("CODE!");
        break;
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
        long long value = SvUV(data);
        Copy((char *)(&value), ptr, 1, long long);
    } break;
    case DC_SIGCHAR_ULONGLONG: {
        unsigned long long value = SvUV(data);
        Copy((char *)(&value), ptr, 1, unsigned long long);
    } break;
    case DC_SIGCHAR_FLOAT: {
        float value = SvNV(data);
        Copy((char *)(&value), ptr, 1, float);
        // return FLOATSIZE;
    } break;
    case DC_SIGCHAR_DOUBLE: {
        double value = SvNV(data);
        Copy((char *)(&value), ptr, 1, double);
        // return DOUBLESIZE;
    } break;
    case DC_SIGCHAR_STRING: {
        char *value = SvPV_nolen(data);
        Copy(&value, ptr, 1, intptr_t);
        // return PTRSIZE;
    } break;
    default: {
        // croak("OTHER");
        // sv_dump(type);
        char *str = SvPVbytex_nolen(type);
        warn("char *str = SvPVbytex_nolen(type) == %s", str);
        size_t size;
        size_t array_len = 1; // TODO...
        void *value;

        switch (str[0]) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_CHAR:
        case DC_SIGCHAR_UCHAR:
            size = I8SIZE;
            break;
        case DC_SIGCHAR_FLOAT:
            size = FLOATSIZE;
            break;
        case DC_SIGCHAR_DOUBLE: {
            double value = (double)SvNV(data);
            // Copy(value, pos, 1, double);
            size = DOUBLESIZE;
        } break;

        // DOUBLESIZE
        default:
            croak("%c is not a known type", str[0]);
        }
        // warn("aaaa %p - %d - %d", pos, size, array_len);

        // if (!packed) pos += padding_needed_for(pos, size * array_len);

        warn("bbb");

        // dcAggrField(ag, type, current_offset, array_len);
        // warn("Adding slot! type: %c, offset: 0[%p], array_len: %d", str[0], pos, array_len);
        // croak("%d | %d", current_offset, array_len);

        // value = newSVnv(*((float *)&val));

        // pos += size * array_len;

        // return newSVpv(value, 1);
    }
    }
    // DumpHex(RETVAL, 1024);
    return NULL; // pos;
                 /*return newSV(0);
                 return MUTABLE_SV(newAV_mortal());
                 return (newSVpv((const char *)RETVAL, 1024)); // XXX: Use mock sizeof from elements
                 */
}