#include "../Affix.h"

// Perl utils
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
        (void)hv_store(_export, _tag, strlen(_tag), newRV_noinc(av), 0);
    }
}

void export_constant_char(const char *package, const char *name, const char *_tag, char val) {
    dTHX;
    register_constant(package, name, newSVpv(&val, 1));
    export_function(package, name, _tag);
}

void export_constant(const char *package, const char *name, const char *_tag, double val) {
    dTHX;
    register_constant(package, name, newSVnv(val));
    export_function(package, name, _tag);
}

void set_isa(const char *klass, const char *parent) {
    dTHX;
    gv_stashpv(parent, GV_ADD | GV_ADDMULTI);
    av_push(get_av(form("%s::ISA", klass), TRUE), newSVpv(parent, 0));
}

// ctypes utils
size_t padding_needed_for(size_t offset, size_t alignment) {
    if (alignment == 0) return 0;
    size_t misalignment = offset % alignment;
    if (misalignment) // round to the next multiple of alignment
        return alignment - misalignment;
    return 0; // already a multiple of alignment*/
}

size_t _sizeof(pTHX_ SV *type) {
    int _type = SvIV(type);
    switch (_type) {
    case AFFIX_TYPE_VOID:
        return 0;
    case AFFIX_TYPE_BOOL:
        return BOOL_SIZE;
    case AFFIX_TYPE_CHAR:
    case AFFIX_TYPE_UCHAR:
        return I8SIZE;
    case AFFIX_TYPE_SHORT:
    case AFFIX_TYPE_USHORT:
        return SHORTSIZE;
    case AFFIX_TYPE_INT:
    case AFFIX_TYPE_UINT:
        return INTSIZE;
    case AFFIX_TYPE_LONG:
    case AFFIX_TYPE_ULONG:
        return LONGSIZE;
    case AFFIX_TYPE_LONGLONG:
    case AFFIX_TYPE_ULONGLONG:
        return LONGLONGSIZE;
    case AFFIX_TYPE_FLOAT:
        return FLOAT_SIZE;
    case AFFIX_TYPE_DOUBLE:
        return DOUBLE_SIZE;
    case AFFIX_TYPE_CSTRUCT:
    case AFFIX_TYPE_CUNION:
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
    case AFFIX_TYPE_CARRAY:
        PING;
#ifdef DEBUG
        DD(type);
#endif

        if (LIKELY(hv_exists(MUTABLE_HV(SvRV(type)), "sizeof", 6)))
            return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "sizeof", 0));
        {
            PING;
            size_t type_alignof = _alignof(aTHX_ * hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0));
            PING;
            size_t array_length = SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "dyn_size", 0));
            PING;
            bool packed = SvTRUE(*hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0));
            PING;
            size_t type_sizeof = _sizeof(aTHX_ * hv_fetchs(MUTABLE_HV(SvRV(type)), "type", 0));
            PING;
            size_t array_sizeof = 0;
            for (size_t i = 0; i < array_length; ++i) {
                array_sizeof += type_sizeof;
                array_sizeof += packed ? 0
                                       : padding_needed_for(array_sizeof, type_alignof > type_sizeof
                                                                              ? type_sizeof
                                                                              : type_alignof);
            }
            PING;
            return array_sizeof;
        }
        PING;
        croak("Do some math!");
        return 0;
    case AFFIX_TYPE_CALLBACK: // automatically wrapped in a DCCallback pointer
    case AFFIX_TYPE_CPOINTER:
    case AFFIX_TYPE_ASCIISTR:
    case AFFIX_TYPE_UTF8STR:
    case AFFIX_TYPE_UTF16STR:
    case AFIX_ARG_STD_STRING:
    //~ case AFFIX_TYPE_ANY:
    case AFFIX_TYPE_SV:
        return INTPTR_T_SIZE;
    case AFFIX_TYPE_WCHAR:
        return WCHAR_T_SIZE;
    default:
        croak("Failed to gather sizeof info for unknown type: %d", _type);
        return -1;
    }
}

size_t _alignof(pTHX_ SV *type) {
    int _type = SvIV(type);
    switch (_type) {
    case AFFIX_TYPE_VOID:
        return 0;
    case AFFIX_TYPE_BOOL:
        return BOOL_ALIGN;
    case AFFIX_TYPE_CHAR:
    case AFFIX_TYPE_UCHAR:
        return I8ALIGN;
    case AFFIX_TYPE_SHORT:
    case AFFIX_TYPE_USHORT:
        return SHORTALIGN;
    case AFFIX_TYPE_INT:
    case AFFIX_TYPE_UINT:
        return INTALIGN;
    case AFFIX_TYPE_LONG:
    case AFFIX_TYPE_ULONG:
        return LONGALIGN;
    case AFFIX_TYPE_LONGLONG:
    case AFFIX_TYPE_ULONGLONG:
        return LONGLONGALIGN;
    case AFFIX_TYPE_FLOAT:
        return FLOAT_ALIGN;
    case AFFIX_TYPE_DOUBLE:
        return DOUBLE_ALIGN;
    case AFFIX_TYPE_CSTRUCT:
    case AFFIX_TYPE_CUNION:
    case AFFIX_TYPE_CARRAY:
        //~ sv_dump(type);
        //~ DD(type);
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "align", 0));
    case AFFIX_TYPE_CALLBACK: // automatically wrapped in a DCCallback pointer
    case AFFIX_TYPE_CPOINTER:
    case AFFIX_TYPE_ASCIISTR:
    case AFFIX_TYPE_UTF8STR:
    case AFFIX_TYPE_UTF16STR:
    case AFIX_ARG_STD_STRING:
    //~ case AFFIX_TYPE_ANY:
    case AFFIX_TYPE_SV:
        return INTPTR_T_ALIGN;
    case AFFIX_TYPE_WCHAR:
        return WCHAR_T_ALIGN;
    default:
        croak("Failed to gather alignment info for unknown type: %d", _type);
        return -1;
    }
}

size_t _offsetof(pTHX_ SV *type) {
    if (hv_exists(MUTABLE_HV(SvRV(type)), "offset", 6))
        return SvUV(*hv_fetchs(MUTABLE_HV(SvRV(type)), "offset", 0));
    return 0;
}

const char *type_as_str(int type) {
    switch (type) {
    case AFFIX_TYPE_SV:
        return "Any";
    case AFFIX_TYPE_VOID:
        return "Void";
    case AFFIX_TYPE_BOOL:
        return "Bool";
    case AFFIX_TYPE_CHAR:
        return "Char";
    case AFFIX_TYPE_SHORT:
        return "Short";
    case AFFIX_TYPE_INT:
        return "Int";
    case AFFIX_TYPE_LONG:
        return "Long";
    case AFFIX_TYPE_LONGLONG:
        return "LongLong";
    case AFFIX_TYPE_FLOAT:
        return "Float";
    case AFFIX_TYPE_DOUBLE:
        return "Double";
    case AFFIX_TYPE_ASCIISTR:
    case AFFIX_TYPE_UTF8STR:
        return "Str";
    case AFFIX_TYPE_UTF16STR:
        return "WStr";
    case AFFIX_TYPE_CSTRUCT:
        return "Struct";
    case AFFIX_TYPE_CARRAY:
        return "ArrayRef";
    case AFFIX_TYPE_CALLBACK:
        return "CodeRef";
    case AFFIX_TYPE_CPOINTER:
        return "Pointer";
    /*case  AFFIX_TYPE_VMARRAY 30*/
    case AFFIX_TYPE_UCHAR:
        return "UChar";
    case AFFIX_TYPE_USHORT:
        return "UShort";
    case AFFIX_TYPE_UINT:
        return "UInt";
    case AFFIX_TYPE_ULONG:
        return "ULong";
    case AFFIX_TYPE_ULONGLONG:
        return "ULongLong";
    case AFFIX_TYPE_CUNION:
        return "Union";
    /*case  AFFIX_TYPE_CPPSTRUCT 44*/
    case AFFIX_TYPE_WCHAR:
        return "WChar";
    case AFIX_ARG_STD_STRING:
        return "std::string";
    default:
        return "Unknown";
    }
}

int type_as_dc(int type) {
    switch (type) {
    case AFFIX_TYPE_VOID:
        return DC_SIGCHAR_VOID;
    case AFFIX_TYPE_BOOL:
        return DC_SIGCHAR_BOOL;
    case AFFIX_TYPE_CHAR:
        return DC_SIGCHAR_CHAR;
    case AFFIX_TYPE_SHORT:
        return DC_SIGCHAR_SHORT;
    case AFFIX_TYPE_INT:
        return DC_SIGCHAR_INT;
    case AFFIX_TYPE_LONG:
        return DC_SIGCHAR_LONG;
    case AFFIX_TYPE_LONGLONG:
        return DC_SIGCHAR_LONGLONG;
    case AFFIX_TYPE_FLOAT:
        return DC_SIGCHAR_FLOAT;
    case AFFIX_TYPE_DOUBLE:
        return DC_SIGCHAR_DOUBLE;
    case AFFIX_TYPE_ASCIISTR:
    case AFFIX_TYPE_UTF8STR:
        return DC_SIGCHAR_STRING;
    case AFFIX_TYPE_UTF16STR:
    case AFFIX_TYPE_CARRAY:
    case AFFIX_TYPE_CALLBACK:
    case AFFIX_TYPE_CPOINTER:
    case AFFIX_TYPE_SV:
    case AFIX_ARG_STD_STRING:
        return DC_SIGCHAR_POINTER;
    /*case  AFFIX_TYPE_VMARRAY 30*/
    case AFFIX_TYPE_UCHAR:
        return DC_SIGCHAR_UCHAR;
    case AFFIX_TYPE_USHORT:
        return DC_SIGCHAR_USHORT;
    case AFFIX_TYPE_UINT:
        return DC_SIGCHAR_UINT;
    case AFFIX_TYPE_ULONG:
        return DC_SIGCHAR_ULONG;
    case AFFIX_TYPE_ULONGLONG:
        return DC_SIGCHAR_ULONGLONG;
    case AFFIX_TYPE_CSTRUCT:
    case AFFIX_TYPE_CUNION:
        return DC_SIGCHAR_AGGREGATE;
    /*case  AFFIX_TYPE_CPPSTRUCT 44*/
    case AFFIX_TYPE_WCHAR:
        return (int)AFFIX_TYPE_WCHAR;
    //~ DC_SIGCHAR_POINTER;
    default:
        return -1;
    }
}

// Debugging
void _DumpHex(pTHX_ const void *addr, size_t len, const char *file, int line) {
    fflush(stdout);
    int perLine = 16;
    // Silently ignore silly per-line values.
    if (perLine < 4 || perLine > 64) perLine = 16;
    size_t i;
    U8 *buff;
    Newxz(buff, perLine + 1, U8);
    const U8 *pc = (const U8 *)addr;
    printf("Dumping %lu bytes from %p at %s line %d\n", len, addr, file, line);
    // Length checks.
    if (len == 0) croak("ZERO LENGTH");
    for (i = 0; i < len; i++) {
        if ((i % perLine) == 0) { // Only print previous-line ASCII buffer for
            // lines beyond first.
            if (i != 0) printf(" | %s\n", buff);
            printf("#  %03zu ", i); // Output the offset of current line.
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
