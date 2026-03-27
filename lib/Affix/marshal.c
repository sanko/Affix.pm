/**
 * @defgroup DataStructures Data Structures
 * @{
 */

/**
 * @struct infixMagic
 * @brief Structure stored in Perl's MAGIC (PERL_MAGIC_ext) to link SVs to C memory.
 *
 * This structure acts as the bridge between a Perl scalar and a specific location
 * in native memory, carrying the necessary type and registry context for lazy binding.
 */
typedef struct {
    void * ptr;              /**< Base pointer to the native memory location. */
    const infix_type * type; /**< infix type definition for this memory. */
    size_t bit_offset;       /**< Bit offset (for bitfields) */
    size_t bit_width;        /**< Bit width (for bitfields, 0 = not a bitfield) */
} infixMagic;

/**
 * @struct CallbackData
 * @brief Context for C closures that execute Perl subroutines.
 *
 * Used by the reverse trampoline system to map native function calls back to Perl.
 */
typedef struct {
    CV * cv; /**< The Perl subroutine (Code Value) to be executed. */
} CallbackData;
/** @} */ /* end of DataStructures */


/**
 * @brief Unwraps named references and enums to find the actual underlying infix type.
 *
 * @param type The (potentially named) infix type to resolve.
 * @return const infix_type* The fully resolved underlying type.
 */
const infix_type * resolve_type(const infix_type * type, const infix_registry_t * reg) {
    while (type) {
        if (type->category == INFIX_TYPE_NAMED_REFERENCE) {
            if (!reg)
                break;
            const infix_type * resolved = infix_registry_lookup_type(reg, type->meta.named_reference.name);
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


/**
 * @def MAKE_PRIMITIVE_DISPATCH
 * @brief Macro to generate MAGIC vtable handlers for primitive types.
 *
 * @param NAME The name suffix for the vtable and handlers.
 * @param C_TYPE The native C type.
 * @param SV_SET The Perl macro to set an SV from a C value.
 * @param SV_GET The Perl macro to get a C value from an SV.
 */
#define MAKE_PRIMITIVE_DISPATCH(NAME, C_TYPE, SV_SET, SV_GET) \
    int get_##NAME(pTHX_ SV * sv, MAGIC * mg) {               \
        infixMagic * im = (infixMagic *)mg->mg_ptr;           \
        SvSMAGICAL_off(sv);                                   \
        SV_SET(sv, *(C_TYPE *)im->ptr);                       \
        SvSMAGICAL_on(sv);                                    \
        return 0;                                             \
    }                                                         \
    int set_##NAME(pTHX_ SV * sv, MAGIC * mg) {               \
        infixMagic * im = (infixMagic *)mg->mg_ptr;           \
        SvGMAGICAL_off(sv);                                   \
        *(C_TYPE *)im->ptr = (C_TYPE)SV_GET(sv);              \
        SvGMAGICAL_on(sv);                                    \
        return 0;                                             \
    }                                                         \
    static MGVTBL vtbl_##NAME = {get_##NAME, set_##NAME, NULL, NULL, NULL}

#ifdef __SIZEOF_INT128__
void sv_setiv128(pTHX_ SV * const sv, const IV i) {}

void sv_setuv128(pTHX_ SV * const sv, const UV i) {}

#define SvIV128 SvIV
#define SvUV128 SvUV
#endif

MAKE_PRIMITIVE_DISPATCH(bool, bool, sv_setbool, SvTRUE);
MAKE_PRIMITIVE_DISPATCH(sint8, int8_t, sv_setiv, SvIV);
MAKE_PRIMITIVE_DISPATCH(uint8, uint8_t, sv_setuv, SvUV);
MAKE_PRIMITIVE_DISPATCH(sint16, int16_t, sv_setiv, SvIV);
MAKE_PRIMITIVE_DISPATCH(uint16, uint16_t, sv_setuv, SvUV);
MAKE_PRIMITIVE_DISPATCH(sint32, int32_t, sv_setiv, SvIV);
MAKE_PRIMITIVE_DISPATCH(uint32, uint32_t, sv_setuv, SvUV);
MAKE_PRIMITIVE_DISPATCH(sint64, int64_t, sv_setiv, SvIV);
MAKE_PRIMITIVE_DISPATCH(uint64, uint64_t, sv_setuv, SvUV);
MAKE_PRIMITIVE_DISPATCH(float, float, sv_setnv, SvNV);
MAKE_PRIMITIVE_DISPATCH(double, double, sv_setnv, SvNV);
#if 0
#ifdef __SIZEOF_INT128__
//~ MAKE_PRIMITIVE_DISPATCH(sint128, signed __int128, sv_setiv128, SvIV128);
//~ MAKE_PRIMITIVE_DISPATCH(uint128, unsigned __int128, sv_setuv128, SvUV128);

/**
 * @brief MAGIC 'get' and 'set' handlers for 128-bit integers via string coercion.
 * @{
 */
int get_128s(pTHX_ SV * sv, MAGIC * mg) {
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    char buf[64];
    unsigned __int128 val = *(unsigned __int128 *)im->ptr;
    sprintf(buf, "0x%016llx%016llx", (unsigned long long)(val >> 64), (unsigned long long)val);
    sv_setpv(sv, buf);
    SvSMAGICAL_on(sv);
    return 0;
}
int get_128u(pTHX_ SV * sv, MAGIC * mg) {
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    int128_to_sv(sv, *(unsigned __int128 *)im->ptr, false);
    SvSMAGICAL_on(sv);
    return 0;
}
int set_128(pTHX_ SV * sv, MAGIC * mg) {
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    SvGMAGICAL_off(sv);
    *(unsigned __int128 *)im->ptr = sv_to_int128(sv);
    SvGMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_sint128 = {get_128s, set_128, NULL, NULL, NULL};
static MGVTBL vtbl_uint128 = {get_128u, set_128, NULL, NULL, NULL};
#endif
#endif
/** @} */

/**
 * @brief Maps an infix primitive type ID to its corresponding MAGIC vtable.
 *
 * @param type The infix type definition.
 * @return MGVTBL* Pointer to the appropriate vtable.
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
#if 0
#ifdef __SIZEOF_INT128__
    case INFIX_PRIMITIVE_SINT128:
        return &vtbl_sint128;
    case INFIX_PRIMITIVE_UINT128:
        return &vtbl_uint128;
#endif
#endif
    default:
        return &vtbl_sint32;
    }
}

/**
 * @brief MAGIC handler for void type (returns undef).
 */
int void_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    SvSMAGICAL_off(sv);
    sv_setsv(sv, &PL_sv_undef);
    SvSMAGICAL_on(sv);
    return 0;
}
static MGVTBL vtbl_void = {void_mg_get, NULL, NULL, NULL, NULL};

/**
 * @brief MAGIC handlers for char[] and uint8_t[] byte arrays (C-Strings).
 * @{
 */
int string_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    const infix_type * t = resolve_type(im->type, MY_CXT.registry);
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
    dMY_CXT;
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    const infix_type * t = resolve_type(im->type, MY_CXT.registry);
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
/** @} */

/**
 * @brief MAGIC handlers for wide char arrays (wchar_t, char16_t, char32_t).
 * Handles UTF-8 <-> UTF-16/32 conversion including surrogates.
 * @{
 */
int wstring_mg_get(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    const infix_type * t = resolve_type(im->type, MY_CXT.registry);
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
int wstring_mg_set(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    const infix_type * t = resolve_type(im->type, MY_CXT.registry);
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
/** @} */

/**
 * @brief MAGIC handlers for bitfields.
 * Handles reading and writing individual bit ranges within 8, 16, 32, or 64-bit words.
 * @{
 */
int get_bitfield(pTHX_ SV * sv, MAGIC * mg) {
    infixMagic * im = (infixMagic *)mg->mg_ptr;
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
int set_bitfield(pTHX_ SV * sv, MAGIC * mg) {
    infixMagic * im = (infixMagic *)mg->mg_ptr;
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
/** @} */

/**
 * @brief Universal closure wrapper for calling Perl subroutines from C.
 *
 * This function is used as the code entry point for reverse trampolines.
 *
 * @param ctx The infix reverse trampoline context.
 * @param ret Pointer to the C return value buffer.
 * @param args Array of pointers to the native C arguments.
 */
void perl_universal_closure(infix_context_t * ctx, void * ret, void ** args) {
    dTHX;
    dSP;
    dMY_CXT;
    CallbackData * d = (CallbackData *)infix_reverse_get_user_data(ctx);
    size_t n = infix_reverse_get_num_args(ctx);
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    for (size_t i = 0; i < n; i++) {
        const infix_type * t = resolve_type(infix_reverse_get_arg_type(ctx, i), MY_CXT.registry);
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
    int count = call_sv((SV *)d->cv, G_SCALAR);
    SPAGAIN;
    if (count == 1 && ret) {
        SV * rs = POPs;
        const infix_type * rt = resolve_type(infix_reverse_get_return_type(ctx), MY_CXT.registry);
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
 * @brief MAGIC handlers for pointers.
 * Supports auto-dereferencing char* to strings and wrapping Perl subs into C callbacks.
 * @{
 */
int get_ptr(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;
    infixMagic * im = (infixMagic *)mg->mg_ptr;
    void * addr = *(void **)im->ptr;
    SvSMAGICAL_off(sv);
    if (!addr) {
        sv_setsv(sv, &PL_sv_undef);
    }
    else {
        const infix_type * p = resolve_type(im->type->meta.pointer_info.pointee_type, MY_CXT.registry);
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
int set_ptr(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;

    infixMagic * im = (infixMagic *)mg->mg_ptr;
    SvGMAGICAL_off(sv);
    if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVCV) {
        CV * cv = (CV *)SvRV(sv);
        SvREFCNT_inc(cv);
        infix_reverse_t * rc = NULL;
        const infix_type * ft_raw = resolve_type(im->type, MY_CXT.registry);
        const infix_type * ft = (ft_raw->category == INFIX_TYPE_POINTER)
            ? resolve_type(ft_raw->meta.pointer_info.pointee_type, MY_CXT.registry)
            : ft_raw;
        size_t n = ft->meta.func_ptr_info.num_args;
        infix_type ** at = n ? (infix_type **)safecalloc(n, sizeof(infix_type *)) : NULL;
        for (size_t i = 0; i < n; i++)
            at[i] = ft->meta.func_ptr_info.args[i].type;
        CallbackData * d = (CallbackData *)safecalloc(1, sizeof(CallbackData));
        d->cv = cv;
        if (infix_reverse_create_closure_manual(&rc,
                                                ft->meta.func_ptr_info.return_type,
                                                at,
                                                n,
                                                ft->meta.func_ptr_info.num_fixed_args,
                                                perl_universal_closure,
                                                d) != INFIX_SUCCESS) {
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
/** @} */

/**
 * @brief MAGIC length handler for arrays and vectors.
 * Enables Perl's `scalar(@array)` and array size reporting.
 */
U32 array_mg_len(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;

    infixMagic * im = (infixMagic *)mg->mg_ptr;
    const infix_type * t = resolve_type(im->type, MY_CXT.registry);
    size_t len = (t->category == INFIX_TYPE_ARRAY) ? t->meta.array_info.num_elements : t->meta.vector_info.num_elements;
    return (U32)(len > 0 ? len - 1 : 0);
}
static MGVTBL vtbl_array = {NULL, NULL, (U32 (*)(pTHX_ SV *, MAGIC *))array_mg_len, NULL, NULL};
void bind_placeholder(pTHX_ SV * sv,
                      void * ptr,
                      const infix_type * type,
                      infix_registry_t * reg,
                      uint8_t bit_offset,
                      uint8_t bit_width,
                      bool prime);
SV * bind_aggregate(pTHX_ void * ptr, const infix_type * type, infix_registry_t * reg);
/**
 * @brief Triggered when a hash/array placeholder is accessed, expanding it into a real Perl hash/array.
 */
int lazy_agg_get(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;

    infixMagic * im = (infixMagic *)mg->mg_ptr;
    SvSMAGICAL_off(sv);
    if (!SvROK(sv)) {
        SV * rv = bind_aggregate(aTHX_ im->ptr, im->type, MY_CXT.registry);
        sv_setsv(sv, rv);
        SvREFCNT_dec(rv);
    }
    SvSMAGICAL_on(sv);
    return 0;
}

int lazy_agg_set(pTHX_ SV * sv, MAGIC * mg) {
    dMY_CXT;

    infixMagic * im = (infixMagic *)mg->mg_ptr;
    const infix_type * type = resolve_type(im->type, MY_CXT.registry);
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
                    /* Write the literal Perl value deep into C memory instantly */
                    SV * temp = newSV(0);
                    sv_setsv(temp, *val_ptr);
                    bind_placeholder(aTHX_ temp,
                                     (char *)im->ptr + m->offset,
                                     m->type,
                                     MY_CXT.registry,
                                     m->bit_offset,
                                     m->bit_width,
                                     false);
                    SvGMAGICAL_off(temp); /* MUST turn off GMAGICAL to avoid fetching old C memory into 'temp' */
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
                step = resolve_type(el_type, MY_CXT.registry)->size;
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
                    bind_placeholder(aTHX_ temp, (char *)im->ptr + (i * step), el_type, MY_CXT.registry, 0, 0, false);
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


/**
 * @brief Connects an SV to a specific native memory location and type using MAGIC.
 *
 * Chooses the appropriate vtable based on the type (primitive, string, array, etc.).
 *
 * @param sv The Perl scalar to bind.
 * @param ptr Pointer to the native memory.
 * @param type The infix type definition.
 * @param reg Registry context.
 * @param bit_offset Offset in bits.
 * @param bit_width Width in bits.
 */
void bind_placeholder(pTHX_ SV * sv,
                      void * ptr,
                      const infix_type * type,
                      infix_registry_t * reg,
                      uint8_t bit_offset,
                      uint8_t bit_width,
                      bool prime) {
    const infix_type * res = resolve_type(type, reg);
    infix_type_category cat = infix_type_get_category(res);
    infixMagic m = {ptr, type, bit_offset, bit_width};
    MGVTBL * v = NULL;

    if (bit_width > 0)
        v = &vtbl_bitfield;
    else if (cat == INFIX_TYPE_PRIMITIVE) {
#if 0
#ifdef __SIZEOF_INT128__
        if (res->meta.primitive_id == INFIX_PRIMITIVE_SINT128)
            v = &vtbl_sint128;
        else if (res->meta.primitive_id == INFIX_PRIMITIVE_UINT128)
            v = &vtbl_uint128;
        else
#endif
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
        const infix_type * el = resolve_type(res->meta.array_info.element_type, reg);
        bool i8 = (el->category == INFIX_TYPE_PRIMITIVE &&
                   (el->meta.primitive_id == INFIX_PRIMITIVE_SINT8 || el->meta.primitive_id == INFIX_PRIMITIVE_UINT8));
        bool iW = false;

        /* Ensure we look at the name of the resolved primitive which was populated by the registry */
        if (el->name) {
            const char * n = el->name;
            if (strEQ(n, "wchar_t") || strEQ(n, "char16_t") || strEQ(n, "char32_t") || strEQ(n, "WChar"))
                iW = true;
        }
        v = i8 ? &string_vtable : iW ? &wstring_vtable : &vtbl_lazy_aggregate;
    }
    else {
        v = &vtbl_lazy_aggregate;
    }

    sv_magicext(sv, NULL, PERL_MAGIC_ext, v, (char *)&m, sizeof(infixMagic));
    SvMAGICAL_on(sv);
    SvGMAGICAL_on(sv);
    SvSMAGICAL_on(sv);

    if (prime && v != &vtbl_lazy_aggregate && v != &vtbl_void)
        v->svt_get(aTHX_ sv, mg_find(sv, PERL_MAGIC_ext));
}
/**
 * @brief Materializes a C struct/array into a Perl hash/array reference.
 *
 * @param ptr Pointer to the native aggregate.
 * @param type The infix type definition for the aggregate.
 * @param reg Registry context.
 * @return SV* A new Perl reference (HV or AV).
 */
SV * bind_aggregate(pTHX_ void * ptr, const infix_type * type, infix_registry_t * reg) {
    const infix_type * res = resolve_type(type, reg);
    infix_type_category cat = infix_type_get_category(res);

    if (cat == INFIX_TYPE_STRUCT || cat == INFIX_TYPE_UNION) {
        HV * hv = newHV();
        size_t count = infix_type_get_member_count(res);
        for (size_t i = 0; i < count; i++) {
            const infix_struct_member * m = infix_type_get_member(res, i);
            SV * v = newSV(0);
            bind_placeholder(aTHX_ v, (char *)ptr + m->offset, m->type, reg, m->bit_offset, m->bit_width, true);
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
            s = resolve_type(et, reg)->size;
        }
        else {
            n = res->meta.vector_info.num_elements;
            et = res->meta.vector_info.element_type;
            s = et->size;
        }
        for (size_t i = 0; i < n; i++) {
            SV * el = newSV(0);
            bind_placeholder(aTHX_ el, (char *)ptr + (i * s), et, reg, 0, 0, true);
            av_push(av, el);
        }
        infixMagic am = {ptr, type, 0, 0};
        sv_magicext((SV *)av, NULL, PERL_MAGIC_ext, &vtbl_array, (char *)&am, sizeof(infixMagic));
        SvMAGICAL_on((SV *)av);
        return newRV_noinc((SV *)av);
    }
    return newSV(0);
}

/**
 * @brief Casts a Perl value (pointer or MAGIC SV) to a named infix type.
 *
 * @param in The input SV (raw address or another infix scalar).
 * @param name The target type name or signature.
 * @param reg The registry.
 * @return SV* A new MAGIC-bound SV of the target type.
 */
SV * cast(pTHX_ SV * in, const char * name, infix_registry_t * reg) {
    void * addr = NULL;
    SV * tgt = SvROK(in) ? SvRV(in) : in;
    MAGIC * mg = SvMAGICAL(tgt) ? mg_find(tgt, PERL_MAGIC_ext) : NULL;

    if (mg && mg->mg_virtual == &vtbl_pointer)
        addr = *(void **)((infixMagic *)mg->mg_ptr)->ptr;
    else
        addr = INT2PTR(void *, SvIV(tgt));

    const infix_type * t = infix_registry_lookup_type(reg, name);
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
            if (infix_type_from_signature(&tt, &ta, name, reg) == INFIX_SUCCESS) {
                t = _copy_type_graph_to_arena(reg->arena, tt);
                infix_arena_destroy(ta);
            }
        }
    }
    if (!t)
        croak("Type not found");

    SV * sv = newSV(0);
    bind_placeholder(aTHX_ sv, addr, t, reg, 0, 0, true);
    return sv;
}
/**
 * @brief Allocates raw zeroed memory.
 * @param sz Size in bytes.
 * @return IV Pointer to allocated memory as an integer.
 */
IV alloc_raw(IV sz) { return PTR2IV(safecalloc(1, sz)); }
/**
 * @brief Returns a static C string pointer for testing.
 * @return IV Pointer to "Hello from C Pointer".
 */
IV get_string_ptr() {
    static char * m = "Hello from C Pointer";
    return PTR2IV(m);
}


/**
 * @brief Extracts a native FILE* pointer from a Perl filehandle.
 * @param fh_ref Reference to a Perl filehandle.
 * @return IV The native FILE* pointer.
 */
IV get_file_ptr(pTHX_ SV * fh_ref) {
    IO * io = sv_2io(fh_ref);
    if (!io)
        return 0;
    return PTR2IV(PerlIO_exportFILE(IoIFP(io), NULL));
}


/**
 * @brief Parses and defines types in the provided registry.
 * @param reg_ptr The registry IV.
 * @param defs The infix DSL type definition string.
 */
XS_INTERNAL(XS_main_define_types) {
    dXSARGS;
    dMY_CXT;
    if (items != 1)
        croak_xs_usage(cv, "defs");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
        const char * defs = (const char *)SvPV_nolen(ST(0));
        I32 * temp;
        temp = PL_markstack_ptr++;
        {
            if (infix_register_types(MY_CXT.registry, defs) != INFIX_SUCCESS)
                croak("Type parse Error");
        }
        if (PL_markstack_ptr != temp) {
            /* truly void, because dXSARGS not invoked */
            PL_markstack_ptr = temp;
            XSRETURN_EMPTY; /* return empty stack */
        }
        /* must have used dXSARGS; list context implied */
        return; /* assume stack size is correct */
        PUTBACK;
        return;
    }
}

/**
 * @brief Returns the size in bytes of a type by name or signature.
 * @param name The type name or signature.
 * @return IV The size in bytes.
 */
XS_INTERNAL(XS_main_sizeof_type) {
    dVAR;
    dXSARGS;
    dMY_CXT;
    if (items != 1)
        croak_xs_usage(cv, "name");
    {
        const char * name = (const char *)SvPV_nolen(ST(0));
        IV RETVAL;
        dXSTARG;
        {
            const infix_type * t = infix_registry_lookup_type(MY_CXT.registry, name);
            if (!t) {
                infix_arena_t * ta;
                infix_type * tt;
                if (infix_type_from_signature(&tt, &ta, name, MY_CXT.registry) == INFIX_SUCCESS) {
                    size_t s = tt->size;
                    infix_arena_destroy(ta);
                    RETVAL = s;
                    TARGi((IV)RETVAL, 1);
                    ST(0) = TARG;
                    XSRETURN(1);
                }
            }
            RETVAL = t ? t->size : 0;
        }
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}

XS_INTERNAL(XS_main_cast); /* prototype to pass -Wmissing-prototypes */
XS_INTERNAL(XS_main_cast) {
    dVAR;
    dXSARGS;
    dMY_CXT;

    if (items != 2)
        croak_xs_usage(cv, "in, name");
    {
        SV * in = ST(0);
        const char * name = (const char *)SvPV_nolen(ST(1));
        SV * RETVAL;

        RETVAL = cast(aTHX_ in, name, MY_CXT.registry);
        RETVAL = sv_2mortal(RETVAL);
        ST(0) = RETVAL;
    }
    XSRETURN(1);
}


XS_INTERNAL(XS_main_alloc_raw); /* prototype to pass -Wmissing-prototypes */
XS_INTERNAL(XS_main_alloc_raw) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "sz");
    {
        IV sz = (IV)SvIV(ST(0));
        IV RETVAL;
        dXSTARG;

        RETVAL = alloc_raw(sz);
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}


XS_INTERNAL(XS_main_get_string_ptr); /* prototype to pass -Wmissing-prototypes */
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

/**
 * @brief Invokes a native C function pointer (int(*)(int,double)).
 * @param addr Address of the native function.
 * @param a First argument (int).
 * @param b Second argument (double).
 * @return int The function result.
 */
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
        {
            int (*f)(int, double) = (int (*)(int, double))addr;
            RETVAL = f(a, b);
        }
        TARGi((IV)RETVAL, 1);
        ST(0) = TARG;
    }
    XSRETURN(1);
}

/**
 * @brief Directly sets a 128-bit memory location.
 * @param addr Base address.
 * @param l Low 64 bits.
 * @param h High 64 bits.
 */
XS_INTERNAL(XS_main_set_mem_u128) {
    dVAR;
    dXSARGS;
    if (items != 3)
        croak_xs_usage(cv, "addr, l, h");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
        IV addr = (IV)SvIV(ST(0));
        IV l = (IV)SvIV(ST(1));
        IV h = (IV)SvIV(ST(2));
        I32 * temp;
        temp = PL_markstack_ptr++;
        {
            unsigned __int128 * p = (unsigned __int128 *)addr;
            *p = ((unsigned __int128)h << 64) | (unsigned __int128)l;
        }
        if (PL_markstack_ptr != temp) {
            /* truly void, because dXSARGS not invoked */
            PL_markstack_ptr = temp;
            XSRETURN_EMPTY; /* return empty stack */
        }
        /* must have used dXSARGS; list context implied */
        return; /* assume stack size is correct */
        PUTBACK;
        return;
    }
}

XS_INTERNAL(XS_main_get_file_ptr); /* prototype to pass -Wmissing-prototypes */
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
