/**
 * @struct Affix_Pin_2_Point_Oh
 * @brief The internal payload attached to Perl Scalars via Perl Magic (PERL_MAGIC_ext).
 * @details This struct holds the raw C pointer, type metadata, and bitfield offsets.
 * When Perl attempts to read or write a scalar, the VTable fetches this struct to
 * map the scalar value directly to native C memory.
 */
typedef struct {
    void * ptr;              /**< Pointer to the actual C memory backing this variable. */
    const infix_type * type; /**< Pointer to the parsed AST type node. */
    uint8_t bit_offset;      /**< Bitfield offset (0 for standard types). */
    uint8_t bit_width;       /**< Bitfield width in bits (0 for standard types). */
} Affix_Pin_2_Point_Oh;
typedef uint16_t float16_t;

bool is_pin_v2(pTHX_ SV * sv);
static void * _extract_pointer_value(pTHX_ SV * sv, MAGIC * ignore_mg);


/**
 * @brief Converts IEEE 754 half-precision (16-bit) to single-precision (32-bit) float.
 * @details Half-precision has 1 sign bit, 5 exponent bits, and 10 mantissa bits.
 * @param h The 16-bit half-precision value (stored as an unsigned integer).
 * @return The 32-bit single-precision float.
 */
float float16_to_float32(float16_t h) {
    uint32_t h_exp = (h >> 10) & 0x1f;         /* Extract 5-bit exponent */
    uint32_t h_sig = h & 0x3ff;                /* Extract 10-bit mantissa (significand) */
    uint32_t sign = (uint32_t)(h >> 15) << 31; /* Shift sign bit to 32-bit position */
    if (h_exp == 0) {
        if (h_sig == 0)
            return (sign) ? -0.0f : 0.0f; /* Signed zero */
        /* Subnormal number: normalize it */
        while (!(h_sig & 0x400)) {
            h_sig <<= 1;
            h_exp--;
        }
        h_exp++;
        h_sig &= ~0x400;
    }
    else if (h_exp == 0x1f) {
        /* Infinity or NaN */
        uint32_t f_nan = (h_sig == 0) ? 0x7f800000 : 0x7fc00000;
        union {
            uint32_t u;
            float f;
        } u;
        u.u = sign | f_nan;
        return u.f;
    }
    /* Adjust exponent bias from 15 to 127 and align mantissa to 23 bits */
    uint32_t f_exp = (h_exp + (127 - 15)) << 23;
    uint32_t f_sig = h_sig << 13;
    union {
        uint32_t u;
        float f;
    } u;
    u.u = sign | f_exp | f_sig;
    return u.f;
}
/**
 * @brief Converts IEEE 754 single-precision (32-bit) float to half-precision (16-bit).
 * @details Rounds standard floats down to the limited bounds of a half-precision float.
 * @param f The 32-bit single-precision float.
 * @return The 16-bit half-precision value.
 */
float16_t float32_to_float16(float f) {
    union {
        float f;
        uint32_t u;
    } u;
    u.f = f;
    uint32_t sign = (u.u >> 16) & 0x8000;
    uint32_t exp = (u.u >> 23) & 0xff;
    uint32_t sig = u.u & 0x7fffff;
    /* Handle Inf and NaN */
    if (exp == 0xff)
        return sign | 0x7c00 | (sig ? 0x200 : 0);
    int e = (int)exp - 127 + 15; /* Re-bias exponent to 15 */
    if (e >= 31)
        return sign | 0x7c00; /* Overflow to Infinity */
    if (e <= 0) {
        /* Underflow to Subnormal */
        if (e < -10)
            return sign;
        sig |= 0x800000;
        sig >>= (1 - e);
        return sign | (sig >> 13);
    }
    return sign | (e << 10) | (sig >> 13);
}

static MGVTBL vtbl_sint8, vtbl_uint8, vtbl_sint16, vtbl_uint16, vtbl_sint32, vtbl_uint32, vtbl_sint64, vtbl_uint64,
    vtbl_float, vtbl_double, vtbl_float16, vtbl_bool, vtbl_sint128, vtbl_uint128, vtbl_void, vtbl_bitfield,
    vtbl_pointer, vtbl_array, string_vtable, wstring_vtable, vtbl_lazy_aggregate;

const infix_type * resolve_type(pTHX_ const infix_type * type);
SV * bind_aggregate(pTHX_ void * ptr, const infix_type * type, SV * owner);
void bind_placeholder(
    pTHX_ SV * sv, void * ptr, const infix_type * type, uint8_t bit_offset, uint8_t bit_width, bool prime, SV * owner);

/**
 * @brief Heuristic algorithm to determine if an array type should be treated as a string.
 * @details C has no real strings, only arrays. This checks if the array consists of
 * integer types matching known string types (char, char16_t, wchar_t).
 * @param type The infix_type definition.
 * @param out_wide Pointer to an int that will be set to 1 if the string is wide (UTF-16/32).
 * @return 1 if it's a string array, 0 otherwise.
 */
int is_string_array(pTHX_ const infix_type * type, int * out_wide) {
    if (type->category != INFIX_TYPE_ARRAY)
        return 0;

    const infix_type * el_raw = type->meta.array_info.element_type;
    const infix_type * el_res = resolve_type(aTHX_ el_raw);

    if (el_res->category != INFIX_TYPE_PRIMITIVE)
        return 0;

    // Standard 8-bit ints/chars are always treated as standard C strings
    if (el_res->meta.primitive_id == INFIX_PRIMITIVE_SINT8 || el_res->meta.primitive_id == INFIX_PRIMITIVE_UINT8) {
        if (out_wide)
            *out_wide = 0;
        return 1;
    }

    // Check explicitly named types (e.g. char16_t, WChar, wchar_t, etc.)
    const char * n = infix_type_get_name(el_raw);
    if (!n)
        n = infix_type_get_name(el_res);

    if (n) {
        /* Use a robust substring check to catch all variations of "char" / "WChar" */
        if (strstr(n, "char") || strstr(n, "Char") || strstr(n, "CHAR") || strEQ(n, "WChar") || strEQ(n, "wchar_t")) {
            if (el_res->size == 2 || el_res->size == 4) {
                if (out_wide)
                    *out_wide = 1;
                return 1;
            }
        }
    }

    return 0;
}
/**
 * @brief Recursively resolves named references and enums to their underlying AST types.
 * @param type The infix_type definition to resolve.
 * @return The resolved infix_type.
 */
const infix_type * resolve_type(pTHX_ const infix_type * type) {
    dMY_CXT;
    while (type) {
        if (type->category == INFIX_TYPE_NAMED_REFERENCE) {
            if (!MY_CXT.registry)
                break;
            const infix_type * resolved = infix_registry_lookup_type(MY_CXT.registry, type->meta.named_reference.name);
            if (resolved)
                type = resolved;
            else
                break;
        }
        else if (type->category == INFIX_TYPE_ENUM)
            type = type->meta.enum_info.underlying_type;
        else
            break;
    }
    return type;
}
/*
   FAST DISPATCH VTABLES:
   These Virtual Tables hook into Perl's SV read/write events. Instead of
   allocating a new SV on every read, they intercept reads and map them directly
   from the underlying C memory, creating zero-copy bindings.
*/
#define MAKE_PRIMITIVE_DISPATCH(NAME, C_TYPE, SV_SET, SV_GET)           \
    int get_##NAME(pTHX_ SV * sv, MAGIC * mg) {                         \
        Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr; \
        SvSMAGICAL_off(sv);                                             \
        if (!im->ptr)                                                   \
            sv_setsv(sv, &PL_sv_undef);                                 \
        else {                                                          \
            SV_SET(sv, *(C_TYPE *)im->ptr);                             \
        }                                                               \
        SvSMAGICAL_on(sv);                                              \
        return 0;                                                       \
    }                                                                   \
    int set_##NAME(pTHX_ SV * sv, MAGIC * mg) {                         \
        Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr; \
        if (!im->ptr)                                                   \
            return 0;                                                   \
        SvGMAGICAL_off(sv);                                             \
        *(C_TYPE *)im->ptr = (C_TYPE)SV_GET(sv);                        \
        SvGMAGICAL_on(sv);                                              \
        return 0;                                                       \
    }                                                                   \
    static MGVTBL vtbl_##NAME = {get_##NAME, set_##NAME, NULL, NULL, NULL};
MAKE_PRIMITIVE_DISPATCH(sint8, int8_t, sv_setiv, SvIV)
MAKE_PRIMITIVE_DISPATCH(uint8, uint8_t, sv_setuv, SvUV)
MAKE_PRIMITIVE_DISPATCH(sint16, int16_t, sv_setiv, SvIV)
MAKE_PRIMITIVE_DISPATCH(uint16, uint16_t, sv_setuv, SvUV)
MAKE_PRIMITIVE_DISPATCH(sint32, int32_t, sv_setiv, SvIV)
MAKE_PRIMITIVE_DISPATCH(uint32, uint32_t, sv_setuv, SvUV)
MAKE_PRIMITIVE_DISPATCH(sint64, int64_t, sv_setiv, SvIV)
MAKE_PRIMITIVE_DISPATCH(uint64, uint64_t, sv_setuv, SvUV)
MAKE_PRIMITIVE_DISPATCH(float, float, sv_setnv, SvNV)
MAKE_PRIMITIVE_DISPATCH(double, double, sv_setnv, SvNV)
/**
 * @brief VTable GET handler for half-precision floats.
 */
int get_float16(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    if (!im->ptr)
        sv_setsv(sv, &PL_sv_undef);
    else
        sv_setnv(sv, (NV)float16_to_float32(*(float16_t *)im->ptr));
    SvSMAGICAL_on(sv);
    return 0;
}
/**
 * @brief VTable SET handler for half-precision floats.
 */
int set_float16(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    SvGMAGICAL_off(sv);
    *(float16_t *)im->ptr = float32_to_float16((float)SvNV(sv));
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_float16 = {get_float16, set_float16, NULL, NULL, NULL};
/**
 * @brief VTable Handlers for Booleans
 */
int get_bool(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    if (!im->ptr)
        sv_setsv(sv, &PL_sv_undef);
    else
        sv_setiv(sv, *(bool *)im->ptr ? 1 : 0);
    SvSMAGICAL_on(sv);
    return 0;
}
int set_bool(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    SvGMAGICAL_off(sv);
    *(bool *)im->ptr = SvTRUE(sv);
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_bool = {get_bool, set_bool, NULL, NULL, NULL};
/**
 * @brief Converts a native 128-bit unsigned integer to a base-10 string SV.
 * @details Perl lacks internal 128-bit support, so it must be marshalled as a string.
 * @param sv The destination Perl Scalar.
 * @param val The native 128-bit value.
 * @param is_signed If true, checks the sign bit and prepends '-'.
 */
void alt_int128_to_sv(pTHX_ SV * sv, unsigned __int128 val, bool is_signed) {
    char buf[64];
    char * p = buf + 63;
    *p = '\0';
    bool neg = false;
    /* Safely evaluate 128-bit negative bounds via casting */
    if (is_signed && (__int128)val < 0) {
        neg = true;
        val = -(__int128)val;
    }
    if (val == 0) {
        *--p = '0';
    }
    else {
        while (val > 0) {
            *--p = (char)((val % 10) + '0');
            val /= 10;
        }
    }
    if (neg)
        *--p = '-';
    sv_setpv(sv, p);
}
/**
 * @brief Parses a base-10 or base-16 Perl String into a native 128-bit integer.
 * @param sv The source Perl string.
 * @return The parsed native 128-bit value.
 */
unsigned __int128 _alt_sv_to_int128(pTHX_ SV * sv) {
    STRLEN len;
    char * p = SvPV(sv, len);
    unsigned __int128 res = 0;
    bool neg = false;
    while (len > 0 && isSPACE(*p)) {
        p++;
        len--;
    } /* Trim leading whitespace */
    if (len > 0 && *p == '-') {
        neg = true;
        p++;
        len--;
    }
    /* Support Hexadecimal 0x format natively */
    if (len >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        len -= 2;
        while (len > 0) {
            int v = (*p >= '0' && *p <= '9') ? (*p - '0')
                : (*p >= 'a' && *p <= 'f')   ? (*p - 'a' + 10)
                : (*p >= 'A' && *p <= 'F')   ? (*p - 'A' + 10)
                                             : -1;
            if (v == -1)
                break;
            res = res * 16 + v;
            p++;
            len--;
        }
    }
    else {
        while (len > 0 && *p >= '0' && *p <= '9') {
            res = res * 10 + (*p - '0');
            p++;
            len--;
        }
    }
    return neg ? (unsigned __int128)(-(__int128)res) : res;
}
int get_128s(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    if (!im->ptr)
        sv_setsv(sv, &PL_sv_undef);
    else
        alt_int128_to_sv(aTHX_ sv, *(unsigned __int128 *)im->ptr, true);
    SvSMAGICAL_on(sv);
    return 0;
}
int get_128u(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    if (!im->ptr)
        sv_setsv(sv, &PL_sv_undef);
    else
        alt_int128_to_sv(aTHX_ sv, *(unsigned __int128 *)im->ptr, false);
    SvSMAGICAL_on(sv);
    return 0;
}
int set_128(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    SvGMAGICAL_off(sv);
    *(unsigned __int128 *)im->ptr = _alt_sv_to_int128(aTHX_ sv);
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_sint128 = {get_128s, set_128, NULL, NULL, NULL};
static MGVTBL vtbl_uint128 = {get_128u, set_128, NULL, NULL, NULL};
/**
 * @brief Dynamic lookup to associate an Infix Type AST with a Perl Magic VTable.
 */
MGVTBL * get_primitive_vtable(const infix_type * type) {
    switch (type->meta.primitive_id) {
    case INFIX_PRIMITIVE_SINT8:
        return &vtbl_sint8;
    case INFIX_PRIMITIVE_UINT8:
        return &vtbl_uint8;
    case INFIX_PRIMITIVE_SINT16:
        return &vtbl_sint16;
    case INFIX_PRIMITIVE_UINT16:
        return &vtbl_uint16;
    case INFIX_PRIMITIVE_SINT32:
        return &vtbl_sint32;
    case INFIX_PRIMITIVE_UINT32:
        return &vtbl_uint32;
    case INFIX_PRIMITIVE_SINT64:
        return &vtbl_sint64;
    case INFIX_PRIMITIVE_UINT64:
        return &vtbl_uint64;
    case INFIX_PRIMITIVE_FLOAT:
        return &vtbl_float;
    case INFIX_PRIMITIVE_DOUBLE:
        return &vtbl_double;
    case INFIX_PRIMITIVE_FLOAT16:
        return &vtbl_float16;
    case INFIX_PRIMITIVE_BOOL:
        return &vtbl_bool;
    case INFIX_PRIMITIVE_SINT128:
        return &vtbl_sint128;
    case INFIX_PRIMITIVE_UINT128:
        return &vtbl_uint128;
    default:
        return &vtbl_sint32;
    }
}
int void_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    SvSMAGICAL_off(sv);
    sv_setsv(sv, &PL_sv_undef);
    SvSMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_void = {void_mg_get, NULL, NULL, NULL, NULL};
/*SPECIALIZED VTABLES (Strings, Bitfields, Pointers, Aggregates)  */
int string_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    SvSMAGICAL_off(sv);
    char * target = (char *)im->ptr;
    if (!target || max_len == 0) {
        sv_setpvn(sv, "", 0);
    }
    else {
        size_t actual = 0;
        while (actual < max_len && target[actual] != '\0')
            actual++;
        sv_setpvn(sv, target, actual);
    }
    SvSMAGICAL_on(sv);
    return 0;
}
int string_mg_set(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    if (!im->ptr || max_len == 0)
        return 0;
    SvGMAGICAL_off(sv);
    STRLEN len;
    char * str = SvPV(sv, len);
    /* Ensure safety against buffer overflow while writing C strings */
    if (len >= max_len)
        len = max_len - 1;
    strncpy((char *)im->ptr, str, len);
    ((char *)im->ptr)[len] = '\0';
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL string_vtable = {string_mg_get, string_mg_set, NULL, NULL, NULL};
/**
 * @brief Reads UTF-16/32 Wide Strings from C and converts them into a Perl UTF-8 String.
 */
int wstring_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    size_t el_sz = t->meta.array_info.element_type->size;
    SvSMAGICAL_off(sv);
    size_t act = 0;
    if (!im->ptr) {
        sv_setpvn(sv, "", 0);
        SvSMAGICAL_on(sv);
        return 0;
    }
    /* Find null terminator */
    for (; act < max_len; act++)
        if (el_sz == 2 && ((uint16_t *)im->ptr)[act] == 0)
            break;
        else if (el_sz == 4 && ((uint32_t *)im->ptr)[act] == 0)
            break;
    /* Allocate temp buffer for UTF-8 (max 4 bytes per character) */
    U8 * buf = (U8 *)safemalloc(act * UTF8_MAXBYTES + 1);
    U8 * d = buf;
    for (size_t i = 0; i < act; i++) {
        UV cp = (el_sz == 2) ? ((uint16_t *)im->ptr)[i] : ((uint32_t *)im->ptr)[i];
        /* Combine Surrogate Pairs for UTF-16 */
        if (el_sz == 2 && cp >= 0xD800 && cp <= 0xDBFF && i + 1 < act) {
            uint16_t low = ((uint16_t *)im->ptr)[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                i++;
            }
        }
        d = uvchr_to_utf8(d, cp);
    }
    *d = '\0';
    sv_setpvn(sv, (char *)buf, d - buf);
    SvUTF8_on(sv);
    safefree(buf);
    SvSMAGICAL_on(sv);
    return 0;
}
/**
 * @brief Converts a Perl UTF-8 string into a C UTF-16/32 Array.
 */
int wstring_mg_set(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    size_t el_sz = t->meta.array_info.element_type->size;
    if (max_len == 0)
        return 0;
    SvGMAGICAL_off(sv);
    STRLEN len;
    char * str = SvPVutf8(sv, len);
    U8 * p = (U8 *)str;
    U8 * pend = p + len;
    size_t i = 0;
    while (p < pend && i < max_len - 1) {
        STRLEN rlen;
        UV cp = utf8_to_uvchr_buf(p, pend, &rlen);
        if (rlen == 0)
            break;
        p += rlen;
        if (cp == 0)
            break;
        /* Split UTF-8 back into UTF-16 Surrogate Pairs if applicable */
        if (el_sz == 2 && cp > 0xFFFF) {
            /* Truncation safety: Don't write half a pair if buffer is almost full */
            if (i < max_len - 2) {
                cp -= 0x10000;
                ((uint16_t *)im->ptr)[i++] = 0xD800 + (cp >> 10);
                ((uint16_t *)im->ptr)[i++] = 0xDC00 + (cp & 0x3FF);
            }
            else
                break;
        }
        else {
            if (el_sz == 2)
                ((uint16_t *)im->ptr)[i++] = (uint16_t)cp;
            else
                ((uint32_t *)im->ptr)[i++] = (uint32_t)cp;
        }
    }
    /* Terminate string */
    if (el_sz == 2)
        ((uint16_t *)im->ptr)[i] = 0;
    else
        ((uint32_t *)im->ptr)[i] = 0;
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL wstring_vtable = {wstring_mg_get, wstring_mg_set, NULL, NULL, NULL};
/**
 * @brief VTable GET handler for Bitfields
 */
int get_bitfield(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    const infix_type * type = resolve_type(aTHX_ im->type);
    SvSMAGICAL_off(sv);
    if (!im->ptr) {
        sv_setsv(sv, &PL_sv_undef);
        SvSMAGICAL_on(sv);
        return 0;
    }
    uint64_t val = 0;
    size_t sz = type->size;
    if (sz == 1)
        val = *(uint8_t *)im->ptr;
    else if (sz == 2)
        val = *(uint16_t *)im->ptr;
    else if (sz == 4)
        val = *(uint32_t *)im->ptr;
    else if (sz == 8)
        val = *(uint64_t *)im->ptr;
    /* Extract using mask */
    val = (val >> im->bit_offset) & ((im->bit_width == 64) ? ~0ULL : ((1ULL << im->bit_width) - 1));
    sv_setuv(sv, val);
    SvSMAGICAL_on(sv);
    return 0;
}
/**
 * @brief VTable SET handler for Bitfields (With safe mask overflow bounding)
 */
int set_bitfield(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    const infix_type * type = resolve_type(aTHX_ im->type);
    SvGMAGICAL_off(sv);
    uint64_t val = 0;
    size_t sz = type->size;
    if (sz == 1)
        val = *(uint8_t *)im->ptr;
    else if (sz == 2)
        val = *(uint16_t *)im->ptr;
    else if (sz == 4)
        val = *(uint32_t *)im->ptr;
    else if (sz == 8)
        val = *(uint64_t *)im->ptr;
    /* Calculate bitmask and apply overflow protection */
    uint64_t wmask = (im->bit_width == 64) ? ~0ULL : ((1ULL << im->bit_width) - 1);
    uint64_t mask = wmask << im->bit_offset;
    uint64_t new_bits = (SvUV(sv) & wmask) << im->bit_offset;
    val = (val & ~mask) | new_bits;
    if (sz == 1)
        *(uint8_t *)im->ptr = (uint8_t)val;
    else if (sz == 2)
        *(uint16_t *)im->ptr = (uint16_t)val;
    else if (sz == 4)
        *(uint32_t *)im->ptr = (uint32_t)val;
    else if (sz == 8)
        *(uint64_t *)im->ptr = (uint64_t)val;
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_bitfield = {get_bitfield, set_bitfield, NULL, NULL, NULL};
/**
 * @brief Universal closure trampoline for FFI Perl callbacks.
 * @details This function is invoked from native C space when a callback fires. It converts
 * the raw C arguments back into Perl variables, executes the Perl subroutine, and marshalls
 * the result back into the expected C return type.
 */
void perl_universal_closure(infix_context_t * ctx, void * ret, void ** args) {
    dTHX;
    dSP;
    CV * perl_sub = (CV *)infix_reverse_get_user_data(ctx);
    size_t n = infix_reverse_get_num_args(ctx);
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    /* Push C arguments onto the Perl Stack */
    for (size_t i = 0; i < n; i++) {
        const infix_type * t = resolve_type(aTHX_ infix_reverse_get_arg_type(ctx, i));
        if (t->category == INFIX_TYPE_PRIMITIVE) {
            infix_primitive_type_id id = t->meta.primitive_id;
            switch (id) {
            case INFIX_PRIMITIVE_SINT8:
            case INFIX_PRIMITIVE_SINT16:
            case INFIX_PRIMITIVE_SINT32:
                XPUSHs(sv_2mortal(newSViv(*(int32_t *)args[i])));
                break;
            case INFIX_PRIMITIVE_SINT64:
                XPUSHs(sv_2mortal(newSViv(*(int64_t *)args[i])));
                break;
            case INFIX_PRIMITIVE_UINT8:
            case INFIX_PRIMITIVE_UINT16:
            case INFIX_PRIMITIVE_UINT32:
                XPUSHs(sv_2mortal(newSVuv(*(uint32_t *)args[i])));
                break;
            case INFIX_PRIMITIVE_UINT64:
                XPUSHs(sv_2mortal(newSVuv(*(uint64_t *)args[i])));
                break;
            case INFIX_PRIMITIVE_FLOAT:
            case INFIX_PRIMITIVE_DOUBLE:
                XPUSHs(sv_2mortal(newSVnv(*(double *)args[i])));
                break;
            case INFIX_PRIMITIVE_FLOAT16:
                XPUSHs(sv_2mortal(newSVnv(float16_to_float32(*(float16_t *)args[i]))));
                break;
            case INFIX_PRIMITIVE_BOOL:
                XPUSHs(sv_2mortal(newSViv(*(bool *)args[i] ? 1 : 0)));
                break;
            case INFIX_PRIMITIVE_SINT128:
                {
                    SV * v = newSV(0);
                    alt_int128_to_sv(aTHX_ v, *(unsigned __int128 *)args[i], true);
                    XPUSHs(sv_2mortal(v));
                    break;
                }
            case INFIX_PRIMITIVE_UINT128:
                {
                    SV * v = newSV(0);
                    alt_int128_to_sv(aTHX_ v, *(unsigned __int128 *)args[i], false);
                    XPUSHs(sv_2mortal(v));
                    break;
                }
            default:
                XPUSHs(&PL_sv_undef);
            }
        }
        else if (t->category == INFIX_TYPE_POINTER) {
            XPUSHs(sv_2mortal(newSViv(PTR2IV(*(void **)args[i]))));
        }
        else
            XPUSHs(&PL_sv_undef);
    }
    PUTBACK;
    call_sv((SV *)perl_sub, G_SCALAR);
    SPAGAIN;
    /* Retrieve Perl return value and pass it back to C */
    if (ret) {
        SV * rs = POPs;
        const infix_type * rt = resolve_type(aTHX_ infix_reverse_get_return_type(ctx));
        if (rt->category == INFIX_TYPE_PRIMITIVE) {
            infix_primitive_type_id id = rt->meta.primitive_id;
            if (id == INFIX_PRIMITIVE_SINT32)
                *(int32_t *)ret = (int32_t)SvIV(rs);
            else if (id == INFIX_PRIMITIVE_SINT64)
                *(int64_t *)ret = (int64_t)SvIV(rs);
            else if (id == INFIX_PRIMITIVE_DOUBLE)
                *(double *)ret = SvNV(rs);
            else if (id == INFIX_PRIMITIVE_FLOAT16)
                *(float16_t *)ret = float32_to_float16((float)SvNV(rs));
            else if (id == INFIX_PRIMITIVE_BOOL)
                *(bool *)ret = SvTRUE(rs);
            else if (id == INFIX_PRIMITIVE_SINT128 || id == INFIX_PRIMITIVE_UINT128)
                *(unsigned __int128 *)ret = _alt_sv_to_int128(aTHX_ rs);
        }
        else if (rt->category == INFIX_TYPE_POINTER) {
            *(void **)ret = INT2PTR(void *, SvIV(rs));
        }
    }
    PUTBACK;
    FREETMPS;
    LEAVE;
}

/**
 * @brief Non-destructive vivification helper.
 * Transforms a placeholder scalar into a reference without stripping magic.
 */
static void _vivify_magic_rv(pTHX_ SV * sv, SV * rv) {
    if (!SvROK(rv)) {
        sv_setsv(sv, rv);
        return;
    }
    SV * target = SvRV(rv);
    SvRV_set(sv, SvREFCNT_inc(target));
    SvROK_on(sv);
}


int get_ptr(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    SV * owner = mg->mg_obj;

    if (SvROK(sv))
        return 0; /* Already vivified */

    SvSMAGICAL_off(sv);
    void * addr = *(void **)im->ptr;

    if (!addr) {
        sv_setsv(sv, &PL_sv_undef);
    }
    else {
        const infix_type * type = resolve_type(aTHX_ im->type);
        const infix_type * pointee = resolve_type(aTHX_ type->meta.pointer_info.pointee_type);
        infix_type_category cat = infix_type_get_category(pointee);

        if (cat == INFIX_TYPE_PRIMITIVE &&
            (pointee->meta.primitive_id == INFIX_PRIMITIVE_SINT8 ||
             pointee->meta.primitive_id == INFIX_PRIMITIVE_UINT8)) {
            sv_setpv(sv, (char *)addr);
        }
        else if (cat == INFIX_TYPE_STRUCT || cat == INFIX_TYPE_UNION || cat == INFIX_TYPE_ARRAY ||
                 cat == INFIX_TYPE_VECTOR) {
            SV * rv = bind_aggregate(aTHX_ addr, pointee, owner);
            sv_setsv(sv, rv);
            SvREFCNT_dec(rv);
        }
        else {
            sv_setuv(sv, PTR2UV(addr));
        }
    }
    SvSMAGICAL_on(sv);
    return 0;
}

int set_ptr(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im || !im->ptr)
        return 0;

    SvGMAGICAL_off(sv);
    void * new_addr = NULL;

    if (!SvOK(sv)) {
        new_addr = NULL;
    }
    else {
        void * extracted = _extract_pointer_value(aTHX_ sv, mg);
        if (extracted) {
            new_addr = extracted;
        }
        else if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVCV) {
            CV * cv = (CV *)SvRV(sv);
            SvREFCNT_inc(cv);
            infix_reverse_t * rc = NULL;
            const infix_type * ft_raw = resolve_type(aTHX_ im->type);
            const infix_type * ft = (ft_raw->category == INFIX_TYPE_POINTER)
                ? resolve_type(aTHX_ ft_raw->meta.pointer_info.pointee_type)
                : ft_raw;
            size_t n = ft->meta.func_ptr_info.num_args;
            infix_type ** at = n ? (infix_type **)safecalloc(n, sizeof(infix_type *)) : NULL;
            for (size_t i = 0; i < n; i++)
                at[i] = ft->meta.func_ptr_info.args[i].type;
            infix_reverse_create_closure_manual(&rc,
                                                ft->meta.func_ptr_info.return_type,
                                                at,
                                                n,
                                                ft->meta.func_ptr_info.num_fixed_args,
                                                perl_universal_closure,
                                                cv);
            if (at)
                safefree(at);
            new_addr = infix_reverse_get_code(rc);
        }
        else if (SvPOK(sv) && !SvROK(sv) && !sv_isobject(sv)) {
            const infix_type * ft_raw = resolve_type(aTHX_ im->type);
            if (ft_raw->category == INFIX_TYPE_POINTER) {
                const infix_type * pointee = resolve_type(aTHX_ ft_raw->meta.pointer_info.pointee_type);
                if (pointee->category == INFIX_TYPE_PRIMITIVE &&
                    (pointee->meta.primitive_id == INFIX_PRIMITIVE_SINT8 ||
                     pointee->meta.primitive_id == INFIX_PRIMITIVE_UINT8)) {
                    new_addr = (void *)SvPV_nolen(sv);
                }
                else {
                    new_addr = INT2PTR(void *, SvUV(sv));
                }
            }
            else {
                new_addr = INT2PTR(void *, SvUV(sv));
            }
        }
        else {
            new_addr = INT2PTR(void *, SvUV(sv));
        }
    }

    *(void **)im->ptr = new_addr;
    SvGMAGICAL_on(sv);
    return 0;
}

static MGVTBL vtbl_pointer = {get_ptr, set_ptr, NULL, NULL, NULL};


int array_mg_fetch(pTHX_ SV * sv, MAGIC * mg) {
    /* This is triggered when someone does $array[i] */
    /* However, for MAGIC_ext on AVs, Perl doesn't call svt_get for indices. */
    /* To truly protect indices, we'd need to hook into the AV's vtable. */
    /* For now, we will focus on the bind_aggregate logic to prevent creation of OOB pins. */
    return 0;
}
/**
 * @brief Returns the length of an Infix C array so Perl's `scalar(@arr)` works.
 */
U32 array_mg_len(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t len = (t->category == INFIX_TYPE_ARRAY) ? t->meta.array_info.num_elements : t->meta.vector_info.num_elements;
    return (U32)(len > 0 ? len - 1 : 0);
}
static MGVTBL vtbl_array = {NULL, NULL, (U32 (*)(pTHX_ SV *, MAGIC *))array_mg_len, NULL, NULL};
/**
 * @brief Lazy-loads child structures from memory when a Perl user tries to interact with a struct.
 */


int lazy_agg_get(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (SvROK(sv))
        return 0;

    SvSMAGICAL_off(sv);
    if (!im->ptr) {
        sv_setsv(sv, &PL_sv_undef);
    }
    else {
        SV * rv = bind_aggregate(aTHX_ im->ptr, im->type, mg->mg_obj);
        sv_setsv(sv, rv);
        SvREFCNT_dec(rv);
    }
    SvSMAGICAL_on(sv);
    return 0;
}
/**
 * @brief Performs deep writes to structs when a user assigns a hash (`$struct = { x => 1 }`).
 */
int lazy_agg_set(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_2_Point_Oh * im = (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
    if (!im->ptr)
        return 0;
    SV * owner = mg->mg_obj;
    const infix_type * type = resolve_type(aTHX_ im->type);
    infix_type_category cat = infix_type_get_category(type);
    SvGMAGICAL_off(sv);
    SvSMAGICAL_off(sv);
    if (SvROK(sv)) {
        SV * rv = SvRV(sv);
        if ((cat == INFIX_TYPE_STRUCT || cat == INFIX_TYPE_UNION) && SvTYPE(rv) == SVt_PVHV) {
            HV * user_hv = (HV *)rv;
            size_t count = infix_type_get_member_count(type);
            for (size_t i = 0; i < count; i++) {
                const infix_struct_member * m = infix_type_get_member(type, i);
                SV ** val_ptr = hv_fetch(user_hv, m->name ? m->name : "", strlen(m->name ? m->name : ""), 0);
                if (val_ptr && *val_ptr) {
                    SV * temp = newSV(0);
                    sv_setsv(temp, *val_ptr);
                    bind_placeholder(
                        aTHX_ temp, (char *)im->ptr + m->offset, m->type, m->bit_offset, m->bit_width, false, owner);
                    SvGMAGICAL_off(temp); /* Bypass placeholder recursion */
                    MAGIC * cmg = mg_find(temp, PERL_MAGIC_ext);
                    if (cmg && cmg->mg_virtual && cmg->mg_virtual->svt_set)
                        cmg->mg_virtual->svt_set(aTHX_ temp, cmg);
                    SvREFCNT_dec(temp);
                }
            }
        }
        else if ((cat == INFIX_TYPE_ARRAY || cat == INFIX_TYPE_VECTOR || cat == INFIX_TYPE_COMPLEX) &&
                 SvTYPE(rv) == SVt_PVAV) {
            AV * user_av = (AV *)rv;
            size_t max_len, step;
            const infix_type * el_type;
            if (cat == INFIX_TYPE_COMPLEX) {
                max_len = 2;
                el_type = type->meta.complex_info.base_type;
                step = el_type->size;
            }
            else if (cat == INFIX_TYPE_ARRAY) {
                max_len = type->meta.array_info.num_elements;
                el_type = type->meta.array_info.element_type;
                step = resolve_type(aTHX_ el_type)->size;
            }
            else {
                max_len = type->meta.vector_info.num_elements;
                el_type = type->meta.vector_info.element_type;
                step = el_type->size;
            }
            SSize_t user_len = av_len(user_av) + 1;
            size_t copy_len = ((size_t)user_len < max_len) ? (size_t)user_len : max_len;
            for (size_t i = 0; i < copy_len; i++) {
                SV ** val_ptr = av_fetch(user_av, i, 0);
                if (val_ptr && *val_ptr) {
                    SV * temp = newSV(0);
                    sv_setsv(temp, *val_ptr);
                    bind_placeholder(aTHX_ temp, (char *)im->ptr + (i * step), el_type, 0, 0, false, owner);
                    SvGMAGICAL_off(temp);
                    MAGIC * cmg = mg_find(temp, PERL_MAGIC_ext);
                    if (cmg && cmg->mg_virtual && cmg->mg_virtual->svt_set)
                        cmg->mg_virtual->svt_set(aTHX_ temp, cmg);
                    SvREFCNT_dec(temp);
                }
            }
        }
    }
    //~ sv_setsv(sv, &PL_sv_undef);
    SvGMAGICAL_on(sv);
    SvSMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_lazy_aggregate = {lazy_agg_get, lazy_agg_set, NULL, NULL, NULL};
/* BINDERS & MEMORY MAPPING */
/**
 * @brief Binds a single C value (primitive or placeholder) to a Perl SV with magic.
 * @param sv The Perl Scalar to bind.
 * @param ptr Pointer to the raw C memory.
 * @param type The infix_type definition.
 * @param bit_offset Offset in bits (for bitfields).
 * @param bit_width Width in bits (for bitfields).
 * @param prime If true, immediately synchronize the SV with C memory.
 * @param owner The "root" SV that owns the C memory (lifeline reference tracking).
 */
void bind_placeholder(
    pTHX_ SV * sv, void * ptr, const infix_type * type, uint8_t bit_offset, uint8_t bit_width, bool prime, SV * owner) {
    const infix_type * res = resolve_type(aTHX_ type);
    infix_type_category cat = infix_type_get_category(res);
    Affix_Pin_2_Point_Oh m;
    m.ptr = ptr;
    m.type = type;
    m.bit_offset = bit_offset;
    m.bit_width = bit_width;
    MGVTBL * v = NULL;
    int is_wide = 0;
    if (bit_width > 0)
        v = &vtbl_bitfield;
    else if (cat == INFIX_TYPE_PRIMITIVE) {
        if (res->meta.primitive_id == INFIX_PRIMITIVE_SINT128)
            v = &vtbl_sint128;
        else if (res->meta.primitive_id == INFIX_PRIMITIVE_UINT128)
            v = &vtbl_uint128;
        else
            v = get_primitive_vtable(res);
    }
    else if (cat == INFIX_TYPE_POINTER || cat == INFIX_TYPE_REVERSE_TRAMPOLINE)
        v = &vtbl_pointer;
    else if (cat == INFIX_TYPE_VOID)
        v = &vtbl_void;
    else if (is_string_array(aTHX_ type, &is_wide))
        v = is_wide ? &wstring_vtable : &string_vtable;
    else
        v = &vtbl_lazy_aggregate;
    /* Attach the VTable to the Variable */
    sv_magicext(sv, owner, PERL_MAGIC_ext, v, (char *)&m, sizeof(Affix_Pin_2_Point_Oh));
    SvMAGICAL_on(sv);
    SvGMAGICAL_on(sv);
    SvSMAGICAL_on(sv);
    if (prime && v != &vtbl_lazy_aggregate && v != &vtbl_void)
        v->svt_get(aTHX_ sv, mg_find(sv, PERL_MAGIC_ext));
}
/**
 * @brief Recursively constructs a Perl tree (Hash/Array) mapping to a C aggregate.
 * @param ptr Pointer to the raw C memory.
 * @param type The aggregate (struct/union/array) type.
 * @param owner The "root" SV that owns the C memory (lifeline).
 * @return A new reference to the constructed Perl aggregate.
 */
SV * bind_aggregate(pTHX_ void * ptr, const infix_type * type, SV * owner) {
    if (!ptr)
        return &PL_sv_undef;  // Don't wrap NULL aggregates

    const infix_type * res = resolve_type(aTHX_ type);
    infix_type_category cat = infix_type_get_category(res);
    if (cat == INFIX_TYPE_STRUCT || cat == INFIX_TYPE_UNION) {
        HV * hv = newHV();
        size_t count = infix_type_get_member_count(res);
        for (size_t i = 0; i < count; i++) {
            const infix_struct_member * m = infix_type_get_member(res, i);
            SV * v = newSV(0);
            bind_placeholder(
                aTHX_ v, ptr ? ((char *)ptr + m->offset) : NULL, m->type, m->bit_offset, m->bit_width, true, owner);
            hv_store(hv, m->name ? m->name : "", strlen(m->name ? m->name : ""), v, 0);
        }
        Affix_Pin_2_Point_Oh m = {.ptr = ptr, .type = type};
        sv_magicext((SV *)hv, owner, PERL_MAGIC_ext, &vtbl_lazy_aggregate, (char *)&m, sizeof(Affix_Pin_2_Point_Oh));

        return newRV_noinc((SV *)hv);
    }
    else if (cat == INFIX_TYPE_ARRAY || cat == INFIX_TYPE_VECTOR || cat == INFIX_TYPE_COMPLEX) {
        if (is_string_array(aTHX_ type, NULL)) {
            SV * sv = newSV(0);
            bind_placeholder(aTHX_ sv, ptr, type, 0, 0, true, owner);
            return sv;
        }
        AV * av = newAV();
        size_t n, s;
        const infix_type * et;
        if (cat == INFIX_TYPE_COMPLEX) {
            n = 2;
            et = res->meta.complex_info.base_type;
            s = et->size;
        }
        else if (cat == INFIX_TYPE_ARRAY) {
            n = res->meta.array_info.num_elements;
            et = res->meta.array_info.element_type;
            s = resolve_type(aTHX_ et)->size;
        }
        else {
            n = res->meta.vector_info.num_elements;
            et = res->meta.vector_info.element_type;
            s = et->size;
        }
        for (size_t i = 0; i < n; i++) {
            SV * el = newSV(0);
            bind_placeholder(aTHX_ el, ptr ? ((char *)ptr + (i * s)) : NULL, et, 0, 0, true, owner);
            av_push(av, el);
        }
        Affix_Pin_2_Point_Oh am = {.ptr = ptr, .type = type};
        sv_magicext((SV *)av, owner, PERL_MAGIC_ext, &vtbl_array, (char *)&am, sizeof(Affix_Pin_2_Point_Oh));
        return newRV_noinc((SV *)av);
    }
    return newSV(0);
}
/* Native ABI Verification Logic */
typedef struct {
    double x;
    double y;
} NativePos;
void verify_and_mutate_struct_arg(pTHX_ SV * input) {
    dMY_CXT;
    const infix_type * type = infix_registry_lookup_type(MY_CXT.registry, "Pos");
    NativePos stack_struct = {0.0, 0.0};
    if (SvROK(input) && SvTYPE(SvRV(input)) == SVt_PVHV) {
        SV * proxy = newSV(0);
        sv_setsv(proxy, input);
        bind_placeholder(aTHX_ proxy, &stack_struct, type, 0, 0, false, NULL);
        MAGIC * mg = mg_find(proxy, PERL_MAGIC_ext);
        if (mg && mg->mg_virtual && mg->mg_virtual->svt_set)
            mg->mg_virtual->svt_set(aTHX_ proxy, mg);
        SvREFCNT_dec(proxy);
    }
    stack_struct.x += 1.0;
    stack_struct.y *= 2.0;
    if (SvROK(input) && SvTYPE(SvRV(input)) == SVt_PVHV) {
        SV * proxy_rv = bind_aggregate(aTHX_ & stack_struct, type, NULL);
        HV * target_hv = (HV *)SvRV(input);
        HV * source_hv = (HV *)SvRV(proxy_rv);
        hv_iterinit(source_hv);
        HE * entry;
        while ((entry = hv_iternext(source_hv))) {
            I32 klen;
            char * kstr = hv_iterkey(entry, &klen);
            SV * val = hv_iterval(source_hv, entry);
            hv_store(target_hv, kstr, klen, newSVsv(val), 0);
        }
        SvREFCNT_dec(proxy_rv);
    }
}
/* PUBLIC PERL FFI API */
/**
 * @brief Registers multiple types into the global Infix Type Registry.
 * @param defs The Infix type definition string.
 */
void define_types(pTHX_ const char * defs) {
    dMY_CXT;
    if (infix_register_types(MY_CXT.registry, defs) != INFIX_SUCCESS)
        croak("Parse Error");
}
/**
 * @brief Returns the layout size of a type by name.
 * @param name Type name or AST signature.
 * @return Size in bytes.
 */
IV sizeof_type(pTHX_ const char * name) {
    dMY_CXT;
    const infix_type * t = infix_registry_lookup_type(MY_CXT.registry, name);
    if (!t) {
        infix_arena_t * ta;
        infix_type * tt;
        if (infix_type_from_signature(&tt, &ta, name, MY_CXT.registry) == INFIX_SUCCESS) {
            size_t s = tt->size;
            infix_arena_destroy(ta);
            return s;
        }
    }
    return t ? t->size : 0;
}
/**
 * @brief Returns the byte offset of a member in a struct/union.
 * @param type_name The aggregate type name.
 * @param member_name The member field name.
 * @return Offset in bytes, or -1 if not found.
 */
IV offsetof_member(pTHX_ const char * type_name, const char * member_name) {
    dMY_CXT;
    const infix_type * t = resolve_type(aTHX_ infix_registry_lookup_type(MY_CXT.registry, type_name));
    if (!t || (t->category != INFIX_TYPE_STRUCT && t->category != INFIX_TYPE_UNION))
        return -1;
    for (size_t i = 0; i < t->meta.aggregate_info.num_members; i++)
        if (strEQ(t->meta.aggregate_info.members[i].name, member_name))
            return (IV)t->meta.aggregate_info.members[i].offset;
    return -1;
}
/**
 * @brief Casts a raw pointer (or memory block) to a magic-bound Perl variable mapping its layout.
 * @param in The input SV (integer address or Affix::Memory managed object).
 * @param name The struct or primitive type name to cast the memory into.
 * @return A magic-bound SV tracking the memory block natively.
 */
SV * cast(pTHX_ SV * in, const char * name) {
    dMY_CXT;
    void * addr = NULL;
    SV * tgt = SvROK(in) ? SvRV(in) : in;
    MAGIC * mg = SvMAGICAL(tgt) ? mg_find(tgt, PERL_MAGIC_ext) : NULL;
    SV * owner = NULL;
    if (SvROK(in) && sv_derived_from(in, "Affix::Memory")) {
        owner = SvRV(in);
        if (SvTYPE(owner) == SVt_PVAV) {
            SV ** p_ptr = av_fetch((AV *)owner, 0, 0);
            addr = (p_ptr && *p_ptr) ? INT2PTR(void *, SvIV(*p_ptr)) : NULL;
        }
        else {
            addr = INT2PTR(void *, SvIV(owner));
        }
    }
    else if (mg && mg->mg_virtual == &vtbl_pointer)
        addr = *(void **)((Affix_Pin_2_Point_Oh *)mg->mg_ptr)->ptr;
    else
        addr = INT2PTR(void *, SvIV(tgt));
    const infix_type * t = infix_registry_lookup_type(MY_CXT.registry, name);
    if (!t) {
        if (strEQ(name, "sint8"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT8);
        else if (strEQ(name, "uint8"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_UINT8);
        else if (strEQ(name, "sint16"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT16);
        else if (strEQ(name, "uint16"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_UINT16);
        else if (strEQ(name, "sint32"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT32);
        else if (strEQ(name, "uint32"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_UINT32);
        else if (strEQ(name, "sint64"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT64);
        else if (strEQ(name, "uint64"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_UINT64);
        else if (strEQ(name, "sint128"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT128);
        else if (strEQ(name, "uint128"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_UINT128);
        else if (strEQ(name, "float16"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_FLOAT16);
        else if (strEQ(name, "float"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_FLOAT);
        else if (strEQ(name, "double"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_DOUBLE);
        else if (strEQ(name, "bool"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_BOOL);
        else if (strEQ(name, "void"))
            t = infix_type_create_void();
        else if (strEQ(name, "int"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT32);
        else if (strEQ(name, "char"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_SINT8);
        else {
            infix_arena_t * ta;
            infix_type * tt;
            if (infix_type_from_signature(&tt, &ta, name, MY_CXT.registry) == INFIX_SUCCESS) {
                t = _copy_type_graph_to_arena(MY_CXT.registry->arena, tt);
                infix_arena_destroy(ta);
            }
        }
    }
    if (!t)
        croak("Type not found");
    SV * sv = newSV(0);
    bind_placeholder(aTHX_ sv, addr, t, 0, 0, true, owner);
    return sv;
}
/**
 * @brief Wraps an existing C pointer with a custom destructor callback into an object payload.
 * @param ptr_iv The raw pointer (as integer).
 * @param dtor_iv The destructor callback pointer (as integer).
 * @return An Affix::Memory object representing the mapped block.
 */
SV * wrap_owned(pTHX_ IV ptr_iv, IV dtor_iv) {
    AV * av = newAV();
    av_push(av, newSViv(ptr_iv));
    av_push(av, newSViv(dtor_iv));
    SV * rv = newRV_noinc((SV *)av);
    sv_bless(rv, gv_stashpv("Affix::Memory", GV_ADD));
    return rv;
}
/**
 * @brief Allocates zeroed C memory and wraps it into a Perl Affix::Memory object.
 * @param size Memory allocation size in bytes.
 * @return An Affix::Memory object representation.
 */
SV * alloc_owned(pTHX_ IV size) {
    void * ptr = safecalloc(1, size);
    SV * sv = newSViv(PTR2IV(ptr));
    SV * rv = newRV_noinc(sv);
    sv_bless(rv, gv_stashpv("Affix::Memory", GV_ADD));
    return rv;
}
/**
 * @brief Garbage Collector Hook: Frees C memory owned by an Affix::Memory object.
 * @details Can fall back to standard `safefree` or use a custom C++ destructor mapping if passed via `wrap_owned`.
 * @param rv The Affix::Memory reference triggered by DESTROY.
 */
void free_owned(pTHX_ SV * rv) {
    if (!SvROK(rv))
        return;
    SV * sv = SvRV(rv);
    if (SvTYPE(sv) == SVt_PVAV) {
        AV * av = (AV *)sv;
        SV ** r_ptr = av_fetch(av, 0, 0);
        SV ** r_dtor = av_fetch(av, 1, 0);
        if (r_ptr && *r_ptr && SvOK(*r_ptr)) {
            void * ptr = INT2PTR(void *, SvIV(*r_ptr));
            if (ptr && ptr != NULL) {
                if (r_dtor && *r_dtor && SvOK(*r_dtor) && SvIV(*r_dtor) != 0) {
                    void (*custom_dtor)(void *) = INT2PTR(void (*)(void *), SvIV(*r_dtor));
                    custom_dtor(ptr);
                }
                else {
                    safefree(ptr);
                }
            }
            sv_setiv(*r_ptr, 0); /* Zero the pointer to avert Double-Frees */
        }
    }
    else {
        void * ptr = INT2PTR(void *, SvIV(sv));
        if (ptr) {
            safefree(ptr);
            sv_setiv(sv, 0);
        }
    }
}
IV alloc_raw(pTHX_ IV sz) { return PTR2IV(safecalloc(1, sz)); }
void set_mem_u128(IV addr, IV l, IV h) {
    unsigned __int128 * p = (unsigned __int128 *)addr;
    *p = ((unsigned __int128)h << 64) | (unsigned __int128)l;
}
IV get_string_ptr() {
    static char * m = "Hello from C Pointer";
    return PTR2IV(m);
}
int test_invoke_callback(IV addr, int a, double b) {
    int (*f)(int, double) = (int (*)(int, double))addr;
    return f(a, b);
}
/**
 * @brief Verifies native callback invocation for 128-bit function pointers.
 */
SV * test_invoke_callback_128(pTHX_ IV addr, SV * arg_sv) {
    unsigned __int128 (*f)(__int128) = (unsigned __int128 (*)(__int128))addr;
    __int128 arg = (__int128)_alt_sv_to_int128(aTHX_ arg_sv);
    unsigned __int128 res = f(arg);
    SV * ret = newSV(0);
    alt_int128_to_sv(aTHX_ ret, res, false); /* Callback return value is uint128 */
    return ret;
}
IV get_file_ptr(pTHX_ SV * fh_ref) {
    IO * io = sv_2io(fh_ref);
    if (!io)
        return 0;
    return PTR2IV(PerlIO_exportFILE(IoIFP(io), NULL));
}
typedef struct {
    unsigned __int128 val;
    int id;
} BigData;
void mutate_big_data_native(BigData * d) {
    d->val += 1; /* Add 1 to the 128-bit int natively */
    d->id = 777;
}
void verify_marshalling_128(pTHX_ SV * input) {
    dMY_CXT;
    const infix_type * type = infix_registry_lookup_type(MY_CXT.registry, "BigData");
    BigData stack_struct = {0, 0};
    if (SvROK(input)) {
        SV * proxy = newSV(0);
        sv_setsv(proxy, input);
        bind_placeholder(aTHX_ proxy, &stack_struct, type, 0, 0, false, NULL);
        MAGIC * mg = mg_find(proxy, PERL_MAGIC_ext);
        if (mg && mg->mg_virtual->svt_set)
            mg->mg_virtual->svt_set(aTHX_ proxy, mg);
        SvREFCNT_dec(proxy);
    }
    mutate_big_data_native(&stack_struct);
    if (SvROK(input)) {
        SV * proxy_rv = bind_aggregate(aTHX_ & stack_struct, type, NULL);
        HV * target_hv = (HV *)SvRV(input);
        HV * source_hv = (HV *)SvRV(proxy_rv);
        hv_iterinit(source_hv);
        HE * entry;
        while ((entry = hv_iternext(source_hv))) {
            I32 klen;
            char * kstr = hv_iterkey(entry, &klen);
            SV * val = hv_iterval(source_hv, entry);
            hv_store(target_hv, kstr, klen, newSVsv(val), 0);
        }
        SvREFCNT_dec(proxy_rv);
    }
}
/*   Mock C++ Object For Custom Destructor Test   */
typedef struct {
    int value;
} MockCxxObj;
static int mock_cxx_dtor_calls = 0;
IV mock_cxx_new(int v) {
    MockCxxObj * obj = safemalloc(sizeof(MockCxxObj));
    obj->value = v;
    return PTR2IV(obj);
}
void mock_cxx_delete(void * ptr) {
    mock_cxx_dtor_calls++;
    safefree(ptr);
}
IV get_mock_cxx_dtor() { return PTR2IV(mock_cxx_delete); }
int get_mock_cxx_dtor_calls() { return mock_cxx_dtor_calls; }
XS_INTERNAL(XS_main_verify_and_mutate_struct_arg) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "input");

    SP -= items;
    {
        SV * input = ST(0);
        I32 * temp;
        temp = PL_markstack_ptr++;
        verify_and_mutate_struct_arg(aTHX_ input);
        if (PL_markstack_ptr != temp) {

            PL_markstack_ptr = temp;
            XSRETURN_EMPTY;
        }

        return;
        PUTBACK;
        return;
    }
}
XS_INTERNAL(XS_main_define_types) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "defs");

    SP -= items;
    {
        const char * defs = (const char *)SvPV_nolen(ST(0));
        I32 * temp;
        temp = PL_markstack_ptr++;
        define_types(aTHX_ defs);
        if (PL_markstack_ptr != temp) {

            PL_markstack_ptr = temp;
            XSRETURN_EMPTY;
        }

        return;
        PUTBACK;
        return;
    }
}
XS_INTERNAL(XS_main_sizeof_type) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "name");
    {
        const char * name = (const char *)SvPV_nolen(ST(0));
        IV RETVAL;
        dXSTARG;
        RETVAL = sizeof_type(aTHX_ name);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_offsetof_member) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "type_name, member_name");
    {
        const char * type_name = (const char *)SvPV_nolen(ST(0));
        const char * member_name = (const char *)SvPV_nolen(ST(1));
        IV RETVAL;
        dXSTARG;
        RETVAL = offsetof_member(aTHX_ type_name, member_name);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_cast) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "in, name");
    {
        SV * in = ST(0);
        const char * name = (const char *)SvPV_nolen(ST(1));
        SV * RETVAL;
        RETVAL = cast(aTHX_ in, name);
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_wrap_owned) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "ptr_iv, dtor_iv");
    {
        IV ptr_iv = (IV)SvIV(ST(0));
        IV dtor_iv = (IV)SvIV(ST(1));
        SV * RETVAL;
        RETVAL = wrap_owned(aTHX_ ptr_iv, dtor_iv);
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_alloc_owned) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "size");
    {
        IV size = (IV)SvIV(ST(0));
        SV * RETVAL;
        RETVAL = alloc_owned(aTHX_ size);
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_free_owned) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "rv");

    SP -= items;
    {
        SV * rv = ST(0);
        I32 * temp;
        temp = PL_markstack_ptr++;
        free_owned(aTHX_ rv);
        if (PL_markstack_ptr != temp) {

            PL_markstack_ptr = temp;
            XSRETURN_EMPTY;
        }

        return;
        PUTBACK;
        return;
    }
}
XS_INTERNAL(XS_main_alloc_raw) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "sz");
    {
        IV sz = (IV)SvIV(ST(0));
        IV RETVAL;
        dXSTARG;
        RETVAL = alloc_raw(aTHX_ sz);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_set_mem_u128) {
    dVAR;
    dXSARGS;
    if (items != 3)
        croak_xs_usage(cv, "addr, l, h");

    SP -= items;
    {
        IV addr = (IV)SvIV(ST(0));
        IV l = (IV)SvIV(ST(1));
        IV h = (IV)SvIV(ST(2));
        I32 * temp;
        temp = PL_markstack_ptr++;
        set_mem_u128(addr, l, h);
        if (PL_markstack_ptr != temp) {

            PL_markstack_ptr = temp;
            XSRETURN_EMPTY;
        }

        return;
        PUTBACK;
        return;
    }
}
XS_INTERNAL(XS_main_get_string_ptr) {
    dVAR;
    dXSARGS;
    if (items != 0)
        croak_xs_usage(cv, "");
    {
        IV RETVAL;
        dXSTARG;
        RETVAL = get_string_ptr();
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_test_invoke_callback) {
    dVAR;
    dXSARGS;
    if (items != 3)
        croak_xs_usage(cv, "addr, a, b");
    {
        IV addr = (IV)SvIV(ST(0));
        int a = (int)SvIV(ST(1));
        double b = (double)SvNV(ST(2));
        int RETVAL;
        dXSTARG;
        RETVAL = test_invoke_callback(addr, a, b);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_test_invoke_callback_128) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "addr, arg_sv");
    {
        IV addr = (IV)SvIV(ST(0));
        SV * arg_sv = ST(1);
        SV * RETVAL;
        RETVAL = test_invoke_callback_128(aTHX_ addr, arg_sv);
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_get_file_ptr) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "fh_ref");
    {
        SV * fh_ref = ST(0);
        IV RETVAL;
        dXSTARG;
        RETVAL = get_file_ptr(aTHX_ fh_ref);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_verify_marshalling_128) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "input");

    SP -= items;
    {
        SV * input = ST(0);
        I32 * temp;
        temp = PL_markstack_ptr++;
        verify_marshalling_128(aTHX_ input);
        if (PL_markstack_ptr != temp) {

            PL_markstack_ptr = temp;
            XSRETURN_EMPTY;
        }

        return;
        PUTBACK;
        return;
    }
}
XS_INTERNAL(XS_main_mock_cxx_new) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "v");
    {
        int v = (int)SvIV(ST(0));
        IV RETVAL;
        dXSTARG;
        RETVAL = mock_cxx_new(v);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_mock_cxx_delete) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "ptr");

    SP -= items;
    {
        void * ptr = INT2PTR(void *, SvIV(ST(0)));
        I32 * temp;
        temp = PL_markstack_ptr++;
        mock_cxx_delete(ptr);
        if (PL_markstack_ptr != temp) {

            PL_markstack_ptr = temp;
            XSRETURN_EMPTY;
        }

        return;
        PUTBACK;
        return;
    }
}
XS_INTERNAL(XS_main_get_mock_cxx_dtor) {
    dVAR;
    dXSARGS;
    if (items != 0)
        croak_xs_usage(cv, "");
    {
        IV RETVAL;
        dXSTARG;
        RETVAL = get_mock_cxx_dtor();
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}
XS_INTERNAL(XS_main_get_mock_cxx_dtor_calls) {
    dVAR;
    dXSARGS;
    if (items != 0)
        croak_xs_usage(cv, "");
    {
        int RETVAL;
        dXSTARG;
        RETVAL = get_mock_cxx_dtor_calls();
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}


/**
 * @brief Helper to verify if a VTable belongs to the 2.0 system.
 */
static inline int is_v2_vtable(MGVTBL * v) {
    return (v == &vtbl_sint8 || v == &vtbl_uint8 || v == &vtbl_sint16 || v == &vtbl_uint16 || v == &vtbl_sint32 ||
            v == &vtbl_uint32 || v == &vtbl_sint64 || v == &vtbl_uint64 || v == &vtbl_sint128 || v == &vtbl_uint128 ||
            v == &vtbl_float || v == &vtbl_double || v == &vtbl_float16 || v == &vtbl_bool || v == &vtbl_void ||
            v == &vtbl_bitfield || v == &vtbl_pointer || v == &vtbl_array || v == &string_vtable ||
            v == &wstring_vtable || v == &vtbl_lazy_aggregate);
}
/**
 * @brief Internal helper to safely extract a pointer address, ignoring a specific magic struct.
 * This prevents a pointer field from accidentally extracting its own address during assignment.
 */
static void * _extract_pointer_value(pTHX_ SV * sv, MAGIC * ignore_mg) {
    if (!sv || !SvOK(sv))
        return NULL;

    /* 1. Affix::Memory Handle */
    if (sv_isobject(sv) && sv_derived_from(sv, "Affix::Memory")) {
        SV * rv = SvRV(sv);
        if (SvTYPE(rv) == SVt_PVAV) {
            SV ** p_ptr = av_fetch((AV *)rv, 0, 0);
            return (p_ptr && *p_ptr) ? INT2PTR(void *, SvUV(*p_ptr)) : NULL;
        }
        return INT2PTR(void *, SvUV(rv));
    }

    /* 2. Magic on the Referent (The vivified Hash or Array) */
    /* MUST BE CHECKED BEFORE THE SCALAR ITSELF! */
    if (SvROK(sv)) {
        SV * target = SvRV(sv);
        if (SvMAGICAL(target)) {
            MAGIC * mg = mg_find(target, PERL_MAGIC_ext);
            if (mg && mg != ignore_mg && is_v2_vtable(mg->mg_virtual))
                return ((Affix_Pin_2_Point_Oh *)mg->mg_ptr)->ptr;
        }
    }

    /* 3. Magic on the SV itself (The Scalar Pin) */
    if (SvMAGICAL(sv)) {
        MAGIC * mg = mg_find(sv, PERL_MAGIC_ext);
        if (mg && mg != ignore_mg && is_v2_vtable(mg->mg_virtual))
            return ((Affix_Pin_2_Point_Oh *)mg->mg_ptr)->ptr;
    }

    /* 4. Raw integer */
    if (SvIOK(sv))
        return INT2PTR(void *, SvUV(sv));
    return NULL;
}

/**
 * @brief The internal C function to extract a pointer from the 2.0 memory system.
 * @param sv The SV to inspect (Handle or Magical Variable).
 * @return The raw C pointer, or NULL if not found.
 */
void * get_address_v2(pTHX_ SV * sv) { return _extract_pointer_value(aTHX_ sv, NULL); }

/**
 * @brief Checks if a Perl Scalar is managed by the new 2.0 memory system.
 * @details This identifies SVs that are directly mapped to C memory via the
 * Affix_Pin_2_Point_Oh struct and its associated VTables or an Affix::Memory handle.
 * @param sv The Perl Scalar to check.
 * @return true if it is a v2.0 pin, false otherwise.
 */

bool is_pin_v2(pTHX_ SV * sv) {
    if (!sv)
        return false;
    if (sv_isobject(sv) && sv_derived_from(sv, "Affix::Memory"))
        return true;

    /* Check referent first to match extraction priority */
    if (SvROK(sv)) {
        SV * target = SvRV(sv);
        if (SvMAGICAL(target)) {
            MAGIC * mg = mg_find(target, PERL_MAGIC_ext);
            if (mg && is_v2_vtable(mg->mg_virtual))
                return true;
        }
    }
    if (SvMAGICAL(sv)) {
        MAGIC * mg = mg_find(sv, PERL_MAGIC_ext);
        if (mg && is_v2_vtable(mg->mg_virtual))
            return true;
    }
    return false;
}


/**
 * @brief Utility to extract the 2.0 pin payload from an SV.
 * @param sv The Perl Scalar.
 * @return Pointer to the Affix_Pin_2_Point_Oh struct, or NULL if not a v2 pin.
 */
Affix_Pin_2_Point_Oh * get_pin_v2(pTHX_ SV * sv) {
    if (!is_pin_v2(aTHX_ sv))
        return NULL;

    SV * target = SvROK(sv) ? SvRV(sv) : sv;
    MAGIC * mg = mg_find(target, PERL_MAGIC_ext);
    return (Affix_Pin_2_Point_Oh *)mg->mg_ptr;
}
/**
 * @brief Perl-level check for v2.0 magic-bound memory.
 * @usage Affix::is_pin($var)
 */
XS_INTERNAL(XS_main_is_pin) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "sv");
    if (is_pin_v2(aTHX_ ST(0)))
        XSRETURN_YES;
    XSRETURN_NO;
}

/**
 * @brief Returns the raw memory address of a v2.0 pin as an integer.
 * @usage my $addr = Affix::address_v2($var);
 */
XS_INTERNAL(XS_main_address) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "sv");

    void * addr = get_address_v2(aTHX_ ST(0));
    if (addr) {
        ST(0) = sv_2mortal(newSVuv(PTR2UV(addr)));
        XSRETURN(1);
    }
    XSRETURN_UNDEF;
}
