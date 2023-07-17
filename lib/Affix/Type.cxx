#include "../Affix.h"

#define TYPE(NAME, AFFIX_CHAR, DC_CHAR)                                                            \
    {                                                                                              \
        set_isa("Affix::Type::" #NAME, "Affix::Type::Base");                                       \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::" #NAME, Affix_Type_##NAME, __FILE__, "");            \
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

#define CC_TYPE(NAME, DC_CHAR)                                                                     \
    {                                                                                              \
        set_isa("Affix::Type::CC::" #NAME, "Affix::Type::CC");                                     \
        /* Allow type constructors to be overridden */                                             \
        cv = get_cv("Affix::" #NAME, 0);                                                           \
        if (cv == NULL) {                                                                          \
            cv = newXSproto_portable("Affix::CC_" #NAME, Affix_CC_##NAME, __FILE__, "");           \
            XSANY.any_i32 = (int)DC_CHAR;                                                          \
        }                                                                                          \
        export_function("Affix", "CC_" #NAME, "types");                                            \
        export_function("Affix", "CC_" #NAME, "cc");                                               \
        /* types objects can stringify to sigchars */                                              \
        cv = newXSproto_portable("Affix::Type::CC::" #NAME "::(\"\"", Affix_Type_asint, __FILE__,  \
                                 ";$");                                                            \
        XSANY.any_i32 = (int)DC_SIGCHAR_CC_PREFIX;                                                 \
        /* Making a sub named "Affix::Type::Int::()" allows the package */                         \
        /* to be findable via fetchmethod(), and causes */                                         \
        /* overload::Overloaded("Affix::Type::Int") to return true. */                             \
        (void)newXSproto_portable("Affix::Type::CC::" #NAME "::()", Affix_Type_asint, __FILE__,    \
                                  ";$");                                                           \
        XSANY.any_i32 = (int)DC_SIGCHAR_CC_PREFIX;                                                 \
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

// I might need to cram more context into these in the future
// so I'm wrapping the superclass this way
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
        else if (sv_derived_from(type, "Affix::Type::Struct") ||
                 sv_derived_from(type, "Affix::Type::Union")) {
            HV *href = MUTABLE_HV(SvRV(type));
            hv_stores(href, "typedef", newSVpv(name, 0));
        }
    }
    else { croak("Expected a subclass of Affix::Type::Base"); }
    sv_setsv_mg(ST(1), type);
    SvSETMAGIC(ST(1));
    XSRETURN_EMPTY;
}

#define SIMPLE_TYPE(TYPE)                                                                          \
    XS_INTERNAL(Affix_Type_##TYPE) {                                                               \
        dXSARGS;                                                                                   \
        PERL_UNUSED_VAR(items);                                                                    \
        ST(0) = sv_2mortal(                                                                        \
            sv_bless(newRV_inc(MUTABLE_SV(newHV())), gv_stashpv("Affix::Type::" #TYPE, GV_ADD)));  \
        XSRETURN(1);                                                                               \
    }

#define CC(TYPE)                                                                                   \
    XS_INTERNAL(Affix_CC_##TYPE) {                                                                 \
        dXSARGS;                                                                                   \
        PERL_UNUSED_VAR(items);                                                                    \
        ST(0) = sv_2mortal(sv_bless(newRV_inc(MUTABLE_SV(newHV())),                                \
                                    gv_stashpv("Affix::Type::CC::" #TYPE, GV_ADD)));               \
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
SIMPLE_TYPE(Any);
SIMPLE_TYPE(StdStr);

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

void boot_Affix_Type(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);

    TYPE(Any, AFFIX_TYPE_SV, DC_SIGCHAR_SV);
    TYPE(Void, AFFIX_TYPE_VOID, DC_SIGCHAR_VOID);
    TYPE(Bool, AFFIX_TYPE_BOOL, DC_SIGCHAR_BOOL);
    TYPE(Char, AFFIX_TYPE_CHAR, DC_SIGCHAR_CHAR);
    EXT_TYPE(CharEnum, AFFIX_TYPE_CHAR, DC_SIGCHAR_CHAR);
    TYPE(UChar, AFFIX_TYPE_UCHAR, DC_SIGCHAR_CHAR);
    switch (WCHAR_T_SIZE) {
    case I8SIZE:
        TYPE(WChar, AFFIX_TYPE_WCHAR, DC_SIGCHAR_CHAR);
        break;
    case SHORTSIZE:
        TYPE(WChar, AFFIX_TYPE_WCHAR, DC_SIGCHAR_SHORT);
        break;
    case INTSIZE:
        TYPE(WChar, AFFIX_TYPE_WCHAR, DC_SIGCHAR_INT);
        break;
    default:
        warn("Invalid wchar_t size (%ld)! This is a bug. Report it.", WCHAR_T_SIZE);
    }
    TYPE(Short, AFFIX_TYPE_SHORT, DC_SIGCHAR_SHORT);
    TYPE(UShort, AFFIX_TYPE_USHORT, DC_SIGCHAR_SHORT);
    TYPE(Int, AFFIX_TYPE_INT, DC_SIGCHAR_INT);
    EXT_TYPE(Enum, AFFIX_TYPE_INT, DC_SIGCHAR_INT);
    EXT_TYPE(IntEnum, AFFIX_TYPE_INT, DC_SIGCHAR_INT);
    TYPE(UInt, AFFIX_TYPE_UINT, DC_SIGCHAR_INT);
    EXT_TYPE(UIntEnum, AFFIX_TYPE_UINT, DC_SIGCHAR_UINT);
    TYPE(Long, AFFIX_TYPE_LONG, DC_SIGCHAR_LONG);
    TYPE(ULong, AFFIX_TYPE_ULONG, DC_SIGCHAR_LONG);
    TYPE(LongLong, AFFIX_TYPE_LONGLONG, DC_SIGCHAR_LONGLONG);
    TYPE(ULongLong, AFFIX_TYPE_ULONGLONG, DC_SIGCHAR_LONGLONG);
#if Size_t_size == INTSIZE
    TYPE(Size_t, AFFIX_TYPE_SIZE_T, DC_SIGCHAR_UINT);
    TYPE(SSize_t, AFFIX_TYPE_SSIZE_T, DC_SIGCHAR_INT);
#elif Size_t_size == LONGSIZE
    TYPE(Size_t, AFFIX_TYPE_SIZE_T, DC_SIGCHAR_ULONG);
    TYPE(SSize_t, AFFIX_TYPE_SSIZE_T, DC_SIGCHAR_LONG);
#elif Size_t_size == LONGLONGSIZE
    TYPE(Size_t, AFFIX_TYPE_SIZE_T, DC_SIGCHAR_ULONGLONG);
    TYPE(SSize_t, AFFIX_TYPE_SSIZE_T, DC_SIGCHAR_LONGLONG);
#else // quadmath is broken
    TYPE(Size_t, AFFIX_TYPE_SIZE_T, DC_SIGCHAR_ULONGLONG);
    TYPE(SSize_t, AFFIX_TYPE_SSIZE_T, DC_SIGCHAR_LONGLONG);
#endif
    TYPE(Float, AFFIX_TYPE_FLOAT, DC_SIGCHAR_FLOAT);
    TYPE(Double, AFFIX_TYPE_DOUBLE, DC_SIGCHAR_DOUBLE);
    TYPE(Str, AFFIX_TYPE_ASCIISTR, DC_SIGCHAR_STRING);
    TYPE(WStr, AFFIX_TYPE_UTF16STR, DC_SIGCHAR_POINTER);

    TYPE(StdStr, AFIX_ARG_STD_STRING, DC_SIGCHAR_POINTER);

    /*

    #define AFFIX_TYPE_UTF8STR 18
    */
    EXT_TYPE(Struct, AFFIX_TYPE_CSTRUCT, AFFIX_TYPE_CSTRUCT);
    EXT_TYPE(ArrayRef, AFFIX_TYPE_CARRAY, AFFIX_TYPE_CARRAY);
    EXT_TYPE(CodeRef, AFFIX_TYPE_CALLBACK, AFFIX_TYPE_CALLBACK);

    /*
    #define AFFIX_TYPE_VMARRAY 30
    */
    EXT_TYPE(Union, AFFIX_TYPE_CUNION, AFFIX_TYPE_CUNION);

    /*
    #define AFFIX_TYPE_CPPSTRUCT 44
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

    (void)newXSproto_portable("Affix::typedef", Affix_typedef, __FILE__, "$$");
    export_function("Affix", "typedef", "all");

    boot_Affix_Type_InstanceOf(aTHX_ cv);
}
