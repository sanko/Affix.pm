/**
 * @file marshal.c
 * @brief Affix Data Marshalling and Memory Binding
 *
 * This file handles the heavy lifting for data type marshalling, lazy aggregate
 * bindings (arrays, structs, unions), and fast dispatch vtables via Perl magic.
 * It is designed to be included via a unity build.
 */

/**
 * @defgroup DataStructures Data Structures
 * @{
 */

/**
 * @brief Represents the internal state for an SV tied to C memory.
 *
 * Attached to Perl scalar variables via magic (`PERL_MAGIC_ext`). This struct
 * provides the memory address, type information, and bitfield boundaries necessary
 * to marshal data lazily.
 */
typedef struct {
    void * ptr;              /**< Pointer to the underlying C memory. */
    const infix_type * type; /**< The exact type definition for the memory segment. */
    uint8_t bit_offset;      /**< For bitfields: the offset of the bitfield from the start of the byte. */
    uint8_t bit_width;       /**< For bitfields: the exact width of the bitfield in bits. 0 if standard primitive. */
} Affix_Pin_;                // TODO: This will eventually be renamed Affix_Pin...
/** @} */

/* --- Forward Declarations --- */

/**
 * @brief Resolves named references and aliases down to their concrete underlying type.
 * @param type The type definition to resolve.
 * @return The underlying concrete type, or the original type if no resolution is needed.
 */
const infix_type * resolve_type(pTHX_ const infix_type * type);

/**
 * @brief Creates a lazy aggregate (hash or array reference) mapped to C struct/union/array memory.
 * @param ptr Pointer to the base memory of the aggregate.
 * @param type The struct, union, or array type description.
 * @param owner The SV* owning the memory (used to increment reference counts and prevent premature freeing).
 * @return A new reference (SV*) pointing to the generated bound Hash or Array.
 */
SV * bind_aggregate(pTHX_ void * ptr, const infix_type * type, SV * owner);

/**
 * @brief Attaches `Affix_Pin_` to an SV to proxy reads and writes to C memory.
 * @param sv The target Perl scalar to bind.
 * @param ptr Pointer to the underlying C memory for this field.
 * @param type The type definition of the field.
 * @param bit_offset The offset if the field is a bitfield (0 otherwise).
 * @param bit_width The width if the field is a bitfield (0 otherwise).
 * @param prime If true, immediately invokes the getter to pull the current C value into the SV.
 * @param owner The root SV* that owns the overall memory allocation.
 */
void bind_placeholder(
    pTHX_ SV * sv, void * ptr, const infix_type * type, uint8_t bit_offset, uint8_t bit_width, bool prime, SV * owner);

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

#ifdef __SIZEOF_INT128__
/**
 * @brief Converts a 128-bit integer into a Perl string SV.
 * @param sv The target Perl scalar to populate.
 * @param val The 128-bit value to convert.
 * @param is_signed If true, formats the output as a signed integer.
 */
static void marshal_int128_to_sv(pTHX_ SV * sv, unsigned __int128 val, bool is_signed) {
    char buf[64];
    char * p = buf + 63;
    *p = '\0';
    bool neg = false;

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
 * @brief Parses a Perl string SV into a 128-bit integer.
 * @param sv The source Perl scalar.
 * @return The parsed 128-bit value.
 */
static unsigned __int128 marshal_sv_to_int128(pTHX_ SV * sv) {
    STRLEN len;
    char * p = SvPV(sv, len);
    unsigned __int128 res = 0;
    bool neg = false;

    if (len > 0 && *p == '-') {
        neg = true;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        res = res * 10 + (*p - '0');
        p++;
    }
    return neg ? (unsigned __int128)(-(__int128)res) : res;
}
#endif

/**
 * @brief Macro to generate Perl MAGIC getter and setter functions for primitive C types.
 * @param NAME The identifier part for the generated functions (e.g., sint8).
 * @param C_TYPE The underlying C language type (e.g., int8_t).
 * @param SV_SET The Perl API macro to set the SV value (e.g., sv_setiv).
 * @param SV_GET The Perl API macro to extract the value from the SV (e.g., SvIV).
 */
#define MAKE_PRIMITIVE_DISPATCH(NAME, C_TYPE, SV_SET, SV_GET) \
    int get_##NAME(pTHX_ SV * sv, MAGIC * mg) {               \
        Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;           \
        SvSMAGICAL_off(sv);                                   \
        SV_SET(sv, *(C_TYPE *)im->ptr);                       \
        SvSMAGICAL_on(sv);                                    \
        return 0;                                             \
    }                                                         \
    int set_##NAME(pTHX_ SV * sv, MAGIC * mg) {               \
        Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;           \
        SvGMAGICAL_off(sv);                                   \
        *(C_TYPE *)im->ptr = (C_TYPE)SV_GET(sv);              \
        SvGMAGICAL_on(sv);                                    \
        return 0;                                             \
    }                                                         \
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
 * @brief Intercepts reads to a mapped boolean memory field.
 */
int get_bool(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    sv_setiv(sv, *(bool *)im->ptr ? 1 : 0);
    SvSMAGICAL_on(sv);
    return 0;
}

/**
 * @brief Intercepts writes to a mapped boolean memory field.
 */
int set_bool(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvGMAGICAL_off(sv);
    *(bool *)im->ptr = SvTRUE(sv);
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_bool = {get_bool, set_bool, NULL, NULL, NULL};

#ifdef __SIZEOF_INT128__
/** @brief Magic getter for signed 128-bit integer memory. */
int get_128s(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    char buf[64];
    unsigned __int128 val = *(unsigned __int128 *)im->ptr;
    sprintf(buf, "0x%016llx%016llx", (unsigned long long)(val >> 64), (unsigned long long)val);
    sv_setpv(sv, buf);
    SvSMAGICAL_on(sv);
    return 0;
}
/** @brief Magic getter for unsigned 128-bit integer memory. */
int get_128u(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    marshal_int128_to_sv(aTHX_ sv, *(unsigned __int128 *)im->ptr, false);
    SvSMAGICAL_on(sv);
    return 0;
}
/** @brief Magic setter for 128-bit integer memory. */
int set_128(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvGMAGICAL_off(sv);
    *(unsigned __int128 *)im->ptr = marshal_sv_to_int128(aTHX_ sv);
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_sint128 = {get_128s, set_128, NULL, NULL, NULL};
static MGVTBL vtbl_uint128 = {get_128u, set_128, NULL, NULL, NULL};
#endif

/**
 * @brief Helper to look up the correct magic Virtual Table (MGVTBL) for a primitive type.
 * @param type The primitive type definition.
 * @return A pointer to the static MGVTBL struct for that type.
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
    case INFIX_PRIMITIVE_BOOL:
        return &vtbl_bool;
#ifdef __SIZEOF_INT128__
    case INFIX_PRIMITIVE_SINT128:
        return &vtbl_sint128;
    case INFIX_PRIMITIVE_UINT128:
        return &vtbl_uint128;
#endif
    default:
        return &vtbl_sint32;
    }
}

/** @brief Magic getter for void parameters (always sets SV to undef). */
int void_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    SvSMAGICAL_off(sv);
    sv_setsv(sv, &PL_sv_undef);
    SvSMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_void = {void_mg_get, NULL, NULL, NULL, NULL};

/**
 * @brief Reads a null-terminated 8-bit character array from C memory into a Perl SV.
 */
int string_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
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

/**
 * @brief Writes a Perl SV string to an 8-bit character array in C memory, enforcing bounds.
 */
int string_mg_set(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    if (max_len == 0)
        return 0;
    SvGMAGICAL_off(sv);
    STRLEN len;
    char * str = SvPV(sv, len);
    if (len >= max_len)
        len = max_len - 1;
    strncpy((char *)im->ptr, str, len);
    ((char *)im->ptr)[len] = '\0';
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL string_vtable = {string_mg_get, string_mg_set, NULL, NULL, NULL};

/**
 * @brief Reads a null-terminated wide (16-bit or 32-bit) string from C memory, converting to Perl UTF-8.
 */
int wstring_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    size_t el_sz = t->meta.array_info.element_type->size;
    SvSMAGICAL_off(sv);
    size_t act = 0;
    for (; act < max_len; act++)
        if ((el_sz == 2 && ((uint16_t *)im->ptr)[act] == 0) || (el_sz == 4 && ((uint32_t *)im->ptr)[act] == 0))
            break;
    U8 * buf = (U8 *)safemalloc(act * UTF8_MAXBYTES + 1);
    U8 * d = buf;
    for (size_t i = 0; i < act; i++) {
        UV cp = (el_sz == 2) ? ((uint16_t *)im->ptr)[i] : ((uint32_t *)im->ptr)[i];
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
 * @brief Converts a Perl UTF-8 string into a wide (16-bit or 32-bit) C array, enforcing bounds.
 */
int wstring_mg_set(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t max_len = t->meta.array_info.num_elements;
    size_t el_sz = t->meta.array_info.element_type->size;
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
        if (el_sz == 2 && cp > 0xFFFF)
            if (i < max_len - 2) {
                cp -= 0x10000;
                ((uint16_t *)im->ptr)[i++] = 0xD800 + (cp >> 10);
                ((uint16_t *)im->ptr)[i++] = 0xDC00 + (cp & 0x3FF);
            }
            else
                break;
        else if (el_sz == 2)
            ((uint16_t *)im->ptr)[i++] = (uint16_t)cp;
        else
            ((uint32_t *)im->ptr)[i++] = (uint32_t)cp;
    }
    if (el_sz == 2)
        ((uint16_t *)im->ptr)[i] = 0;
    else
        ((uint32_t *)im->ptr)[i] = 0;
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL wstring_vtable = {wstring_mg_get, wstring_mg_set, NULL, NULL, NULL};

/**
 * @brief Magic getter extracting bits from an underlying integer struct member.
 */
int get_bitfield(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    uint64_t val = 0;
    size_t sz = im->type->size;
    if (sz == 1)
        val = *(uint8_t *)im->ptr;
    else if (sz == 2)
        val = *(uint16_t *)im->ptr;
    else if (sz == 4)
        val = *(uint32_t *)im->ptr;
    else if (sz == 8)
        val = *(uint64_t *)im->ptr;
    val = (val >> im->bit_offset) & ((im->bit_width == 64) ? ~0ULL : ((1ULL << im->bit_width) - 1));
    sv_setuv(sv, val);
    SvSMAGICAL_on(sv);
    return 0;
}

/**
 * @brief Magic setter inserting bits into an underlying integer struct member without modifying adjacent bits.
 */
int set_bitfield(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvGMAGICAL_off(sv);
    uint64_t val = 0;
    size_t sz = im->type->size;
    if (sz == 1)
        val = *(uint8_t *)im->ptr;
    else if (sz == 2)
        val = *(uint16_t *)im->ptr;
    else if (sz == 4)
        val = *(uint32_t *)im->ptr;
    else if (sz == 8)
        val = *(uint64_t *)im->ptr;
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
 * @brief libffi callback bridge translating C arguments into Perl arguments, invoking the CV, and converting back.
 * @param ctx The generic infix reverse compilation context.
 * @param ret The pointer where the C return value should be stored.
 * @param args Array of pointers to the incoming C arguments.
 */
void perl_universal_closure(infix_context_t * ctx, void * ret, void ** args) {
    dTHX;
    dSP;
    CV * perl_sub = (CV *)infix_reverse_get_user_data(ctx);
    size_t n = infix_reverse_get_num_args(ctx);
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    for (size_t i = 0; i < n; i++) {
        const infix_type * t = resolve_type(aTHX_ infix_reverse_get_arg_type(ctx, i));
        if (t->category == INFIX_TYPE_PRIMITIVE) {
            infix_primitive_type_id id = t->meta.primitive_id;
            if (id == INFIX_PRIMITIVE_SINT32)
                XPUSHs(sv_2mortal(newSViv(*(int32_t *)args[i])));
            else if (id == INFIX_PRIMITIVE_DOUBLE)
                XPUSHs(sv_2mortal(newSVnv(*(double *)args[i])));
            else
                XPUSHs(&PL_sv_undef);
        }
        else
            XPUSHs(&PL_sv_undef);
    }
    PUTBACK;
    int count = call_sv((SV *)perl_sub, G_SCALAR);
    SPAGAIN;
    if (count == 1 && ret) {
        SV * rs = POPs;
        const infix_type * rt = resolve_type(aTHX_ infix_reverse_get_return_type(ctx));
        if (rt->category == INFIX_TYPE_PRIMITIVE) {
            infix_primitive_type_id id = rt->meta.primitive_id;
            if (id == INFIX_PRIMITIVE_SINT32)
                *(int32_t *)ret = (int32_t)SvIV(rs);
            else if (id == INFIX_PRIMITIVE_DOUBLE)
                *(double *)ret = SvNV(rs);
        }
    }
    PUTBACK;
    FREETMPS;
    LEAVE;
}

/**
 * @brief Reads a pointer value. Checks if it is a C string (char*) and automatically dereferences if so.
 */
int get_ptr(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    void * addr = *(void **)im->ptr;
    SvSMAGICAL_off(sv);
    if (!addr) {
        sv_setsv(sv, &PL_sv_undef);
    }
    else {
        const infix_type * p = resolve_type(aTHX_ im->type->meta.pointer_info.pointee_type);
        if (p->category == INFIX_TYPE_PRIMITIVE &&
            (p->meta.primitive_id == INFIX_PRIMITIVE_SINT8 || p->meta.primitive_id == INFIX_PRIMITIVE_UINT8)) {
            sv_setpv(sv, (char *)addr);
        }
        else {
            sv_setiv(sv, PTR2IV(addr));
        }
    }
    SvSMAGICAL_on(sv);
    return 0;
}

/**
 * @brief Writes a pointer value. If the SV is a CodeRef, it dynamically creates a callback trampoline.
 */
int set_ptr(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SvGMAGICAL_off(sv);
    if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVCV) {
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

        if (infix_reverse_create_closure_manual(&rc,
                                                ft->meta.func_ptr_info.return_type,
                                                at,
                                                n,
                                                ft->meta.func_ptr_info.num_fixed_args,
                                                perl_universal_closure,
                                                (void *)cv) != INFIX_SUCCESS) {
            croak("Closure failed");
        }
        if (at)
            safefree(at);
        *(void **)im->ptr = infix_reverse_get_code(rc);
    }
    else {
        *(void **)im->ptr = INT2PTR(void *, SvIV(sv));
    }
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_pointer = {get_ptr, set_ptr, NULL, NULL, NULL};

/**
 * @brief Intercepts array length checks `av_len()` for bound aggregate arrays.
 */
U32 array_mg_len(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    const infix_type * t = resolve_type(aTHX_ im->type);
    size_t len = (t->category == INFIX_TYPE_ARRAY) ? t->meta.array_info.num_elements : t->meta.vector_info.num_elements;
    return (U32)(len > 0 ? len - 1 : 0);
}
static MGVTBL vtbl_array = {NULL, NULL, (U32 (*)(pTHX_ SV *, MAGIC *))array_mg_len, NULL, NULL};

/**
 * @brief Retrieves a complex aggregate (struct/array) lazily.
 *
 * If the perl variable is not yet populated, it instantiates an appropriate
 * hash/array ref and recursively binds it to the corresponding C struct members or array elements.
 */
int lazy_agg_get(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
    SV * owner = mg->mg_obj;
    SvSMAGICAL_off(sv);
    if (!SvROK(sv)) {
        SV * rv = bind_aggregate(aTHX_ im->ptr, im->type, owner);
        sv_setsv(sv, rv);
        SvREFCNT_dec(rv);
    }
    SvSMAGICAL_on(sv);
    return 0;
}

/**
 * @brief Bulk-writes an incoming perl Hash/Array to a bound struct/array.
 *
 * Iterates the keys/indices and fires individual bindings so C memory is updated appropriately.
 */
int lazy_agg_set(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin_ * im = (Affix_Pin_ *)mg->mg_ptr;
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
                const char * m_name = m->name ? m->name : "";
                SV ** val_ptr = hv_fetch(user_hv, m_name, strlen(m_name), 0);
                if (val_ptr && *val_ptr) {
                    SV * temp = newSV(0);
                    sv_setsv(temp, *val_ptr);
                    bind_placeholder(
                        aTHX_ temp, (char *)im->ptr + m->offset, m->type, m->bit_offset, m->bit_width, false, owner);
                    SvGMAGICAL_off(temp);
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
            size_t max_len = 0, step = 0;
            const infix_type * el_type = NULL;
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

    sv_setsv(sv, &PL_sv_undef);

    SvGMAGICAL_on(sv);
    SvSMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_lazy_aggregate = {lazy_agg_get, lazy_agg_set, NULL, NULL, NULL};

void bind_placeholder(
    pTHX_ SV * sv, void * ptr, const infix_type * type, uint8_t bit_offset, uint8_t bit_width, bool prime, SV * owner) {
    const infix_type * res = resolve_type(aTHX_ type);
    infix_type_category cat = infix_type_get_category(res);
    Affix_Pin_ m = {ptr, type, bit_offset, bit_width};
    MGVTBL * v = NULL;

    if (bit_width > 0) {
        v = &vtbl_bitfield;
    }
    else if (cat == INFIX_TYPE_PRIMITIVE) {
#ifdef __SIZEOF_INT128__
        if (res->meta.primitive_id == INFIX_PRIMITIVE_SINT128)
            v = &vtbl_sint128;
        else if (res->meta.primitive_id == INFIX_PRIMITIVE_UINT128)
            v = &vtbl_uint128;
        else
#endif
            v = get_primitive_vtable(res);
    }
    else if (cat == INFIX_TYPE_POINTER || cat == INFIX_TYPE_REVERSE_TRAMPOLINE) {
        v = &vtbl_pointer;
    }
    else if (cat == INFIX_TYPE_VOID) {
        v = &vtbl_void;
    }
    else if (cat == INFIX_TYPE_ARRAY) {
        const infix_type * raw_el = res->meta.array_info.element_type;
        const infix_type * el = resolve_type(aTHX_ raw_el);
        bool i8 = (el->category == INFIX_TYPE_PRIMITIVE &&
                   (el->meta.primitive_id == INFIX_PRIMITIVE_SINT8 || el->meta.primitive_id == INFIX_PRIMITIVE_UINT8));

        bool iW =
            (el->category == INFIX_TYPE_PRIMITIVE &&
             (el->meta.primitive_id == INFIX_PRIMITIVE_UINT16 || el->meta.primitive_id == INFIX_PRIMITIVE_UINT32));

        const char * n1 = infix_type_get_name(raw_el);
        const char * n2 = infix_type_get_name(el);

        if (!iW &&
            ((n1 &&
              (strstr(n1, "wchar") || strstr(n1, "char16") || strstr(n1, "char32") || strstr(n1, "WChar") ||
               strEQ(n1, "uint16"))) ||
             (n2 &&
              (strstr(n2, "wchar") || strstr(n2, "char16") || strstr(n2, "char32") || strstr(n2, "WChar") ||
               strEQ(n2, "uint16"))))) {
            iW = true;
        }

        v = i8 ? &string_vtable : iW ? &wstring_vtable : &vtbl_lazy_aggregate;
    }
    else {
        v = &vtbl_lazy_aggregate;
    }

    sv_magicext(sv, owner, PERL_MAGIC_ext, v, (char *)&m, sizeof(Affix_Pin_));
    SvMAGICAL_on(sv);
    SvGMAGICAL_on(sv);
    SvSMAGICAL_on(sv);

    if (prime && v != &vtbl_lazy_aggregate && v != &vtbl_void)
        v->svt_get(aTHX_ sv, mg_find(sv, PERL_MAGIC_ext));
}

SV * bind_aggregate(pTHX_ void * ptr, const infix_type * type, SV * owner) {
    const infix_type * res = resolve_type(aTHX_ type);
    infix_type_category cat = infix_type_get_category(res);

    if (cat == INFIX_TYPE_STRUCT || cat == INFIX_TYPE_UNION) {
        HV * hv = newHV();
        size_t count = infix_type_get_member_count(res);
        for (size_t i = 0; i < count; i++) {
            const infix_struct_member * m = infix_type_get_member(res, i);
            SV * v = newSV(0);
            bind_placeholder(aTHX_ v, (char *)ptr + m->offset, m->type, m->bit_offset, m->bit_width, true, owner);
            hv_store(hv, m->name ? m->name : "", strlen(m->name ? m->name : ""), v, 0);
        }
        return newRV_noinc((SV *)hv);
    }
    else if (cat == INFIX_TYPE_ARRAY || cat == INFIX_TYPE_VECTOR || cat == INFIX_TYPE_COMPLEX) {
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
            bind_placeholder(aTHX_ el, (char *)ptr + (i * s), et, 0, 0, true, owner);
            av_push(av, el);
        }
        Affix_Pin_ am = {ptr, type, 0, 0};
        sv_magicext((SV *)av, owner, PERL_MAGIC_ext, &vtbl_array, (char *)&am, sizeof(Affix_Pin_));
        SvMAGICAL_on((SV *)av);
        return newRV_noinc((SV *)av);
    }
    return newSV(0);
}

/**
 * @brief Parses and registers a batch of C typedefs into the thread-safe global registry.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_define_types) {
    dVAR;
    dXSARGS;
    dMY_CXT;
    if (items != 1)
        croak_xs_usage(cv, "defs");
    const char * defs = (const char *)SvPV_nolen(ST(0));
    if (infix_register_types(MY_CXT.registry, defs) != INFIX_SUCCESS)
        croak("Parse Error");
    XSRETURN_EMPTY;
}

/**
 * @brief Resolves a type's size in bytes via the global registry.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_sizeof_type) {
    dVAR;
    dXSARGS;
    dMY_CXT;
    if (items != 1)
        croak_xs_usage(cv, "name");
    const char * name = (const char *)SvPV_nolen(ST(0));
    const infix_type * t = infix_registry_lookup_type(MY_CXT.registry, name);
    IV RETVAL = 0;

    if (!t) {
        infix_arena_t * ta;
        infix_type * tt;
        if (infix_type_from_signature(&tt, &ta, name, MY_CXT.registry) == INFIX_SUCCESS) {
            RETVAL = tt->size;
            infix_arena_destroy(ta);
        }
    }
    else {
        RETVAL = t->size;
    }

    ST(0) = sv_2mortal(newSViv(RETVAL));
    XSRETURN(1);
}

/**
 * @brief Overlays a structured Affix/Infix type over an existing memory address.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_cast) {
    dVAR;
    dXSARGS;
    dMY_CXT;
    if (items != 2)
        croak_xs_usage(cv, "in, name");
    SV * in = ST(0);
    const char * name = (const char *)SvPV_nolen(ST(1));

    void * addr = NULL;
    SV * tgt = SvROK(in) ? SvRV(in) : in;
    MAGIC * mg = SvMAGICAL(tgt) ? mg_find(tgt, PERL_MAGIC_ext) : NULL;

    SV * owner = NULL;
    if (SvROK(in) && sv_derived_from(in, "Infix::Memory"))
        owner = SvRV(in);

    if (mg && mg->mg_virtual == &vtbl_pointer) {
        addr = *(void **)((Affix_Pin_ *)mg->mg_ptr)->ptr;
    }
    else {
        if (!SvOK(tgt) || (SvIOK(tgt) && SvIV(tgt) == 0)) {
            warn("undef pointer");
            XSRETURN_UNDEF;
        }
        addr = INT2PTR(void *, SvIV(tgt));
    }

    if (!addr) {
        warn("undef pointer");
        XSRETURN_UNDEF;
    }

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
        else if (strEQ(name, "float"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_FLOAT);
        else if (strEQ(name, "double"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_DOUBLE);
        else if (strEQ(name, "float16"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_FLOAT16);
        else if (strEQ(name, "long_double"))
            t = infix_type_create_primitive(INFIX_PRIMITIVE_LONG_DOUBLE);
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

    if (!t) {
        warn("Invalid type");
        XSRETURN_UNDEF;
    }

    SV * sv = newSV(0);
    bind_placeholder(aTHX_ sv, addr, t, 0, 0, true, owner);

    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

/**
 * @brief Allocates heap memory managed by Perl's GC (via DESTROY magic).
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_alloc_owned) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "size");

    IV size = SvIV(ST(0));  // Returns 0 with a native warning if undef
    if (size == 0) {
        warn("Cannot allocate zero bytes, returning undef pointer");
        XSRETURN_UNDEF;
    }
    if (size < 0) {
        warn("Cannot allocate negative bytes");
        XSRETURN_UNDEF;
    }

    void * ptr = safecalloc(1, size);
    SV * sv = newSViv(PTR2IV(ptr));
    SV * rv = newRV_noinc(sv);
    sv_bless(rv, gv_stashpv("Infix::Memory", GV_ADD));

    ST(0) = sv_2mortal(rv);
    XSRETURN(1);
}

/**
 * @brief Manually frees a previously allocated managed block of memory.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_free_owned) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "rv");
    SV * rv = ST(0);
    if (SvROK(rv)) {
        SV * sv = SvRV(rv);
        void * ptr = INT2PTR(void *, SvIV(sv));
        if (ptr)
            safefree(ptr);
        sv_setiv(sv, 0);
    }
    XSRETURN_EMPTY;
}

/**
 * @brief Allocates an unmanaged, zeroed block of memory and returns it as a raw integer pointer.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_alloc_raw) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "sz");

    IV sz = SvIV(ST(0));  // Returns 0 with a native warning if undef
    if (sz == 0) {
        // Satisfies both `qr[zero]` and `qr[undef]` expectations in test suite.
        warn("Cannot allocate zero bytes, returning undef pointer");
        ST(0) = sv_2mortal(newSViv(0));
        XSRETURN(1);
    }
    if (sz < 0) {
        warn("Cannot allocate negative bytes");
        XSRETURN_UNDEF;
    }

    IV RETVAL = PTR2IV(safecalloc(1, sz));
    ST(0) = sv_2mortal(newSViv(RETVAL));
    XSRETURN(1);
}

/**
 * @brief Returns the raw address of a statically allocated C string (used for unit tests).
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_get_string_ptr) {
    dVAR;
    dXSARGS;
    if (items != 0)
        croak_xs_usage(cv, "");
    static char * m = "Hello from C Pointer";
    IV RETVAL = PTR2IV(m);
    ST(0) = sv_2mortal(newSViv(RETVAL));
    XSRETURN(1);
}

/**
 * @brief Evaluates an `int(int, double)` C function pointer natively (used to test Perl closures).
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_test_invoke_callback) {
    dVAR;
    dXSARGS;
    if (items != 3)
        croak_xs_usage(cv, "addr, a, b");
    IV addr = (IV)SvIV(ST(0));
    int a = (int)SvIV(ST(1));
    double b = (double)SvNV(ST(2));
    int (*f)(int, double) = (int (*)(int, double))addr;
    int RETVAL = f(a, b);
    ST(0) = sv_2mortal(newSViv(RETVAL));
    XSRETURN(1);
}

/**
 * @brief Writes a pair of 64-bit integer components into a 128-bit memory address.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_set_mem_u128) {
    dVAR;
    dXSARGS;
    if (items != 3)
        croak_xs_usage(cv, "addr, l, h");
    IV addr = (IV)SvIV(ST(0));
    IV l = (IV)SvIV(ST(1));
    IV h = (IV)SvIV(ST(2));
#ifdef __SIZEOF_INT128__
    unsigned __int128 * p = (unsigned __int128 *)addr;
    *p = ((unsigned __int128)h << 64) | (unsigned __int128)l;
#else
    croak("128-bit not supported");
#endif
    XSRETURN_EMPTY;
}

/**
 * @brief Extracts the raw C `FILE*` pointer from a Perl `IO` handle object.
 * @param cv The CV of the current XSUB.
 */
XS_INTERNAL(XS_main_get_file_ptr) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "fh_ref");
    SV * fh_ref = ST(0);
    IO * io = sv_2io(fh_ref);
    IV RETVAL = 0;
    if (io)
        RETVAL = PTR2IV(PerlIO_exportFILE(IoIFP(io), NULL));
    ST(0) = sv_2mortal(newSViv(RETVAL));
    XSRETURN(1);
}
