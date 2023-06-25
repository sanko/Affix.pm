// TODO: Rename this file
#ifndef AFFIX_H_SEEN
#define AFFIX_H_SEEN

// https://mikeash.com/pyblog/friday-qa-2014-08-15-swift-name-mangling.html
// https://gcc.gnu.org/git?p=gcc.git;a=blob_plain;f=gcc/cp/mangle.cc;hb=HEAD
// https://rust-lang.github.io/rfcs/2603-rust-symbol-name-mangling-v0.html

#define AFFIX_ABI_C 'c'
#define AFFIX_ABI_ITANIUM 'I' // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
#define AFFIX_ABI_GCC AFFIX_ABI_ITANIUM
#define AFFIX_ABI_MSVC AFFIX_ABI_ITANIUM
#define AFFIX_ABI_RUST 'r' // legacy
#define AFFIX_ABI_SWIFT                                                                            \
    's' // https://github.com/apple/swift/blob/main/docs/ABI/Mangling.rst#identifiers
#define AFFIX_ABI_D 'd' // https://dlang.org/spec/abi.html#name_mangling

/* Useful but undefined in perlapi */
#define FLOAT_SIZE sizeof(float)
#define BOOL_SIZE sizeof(bool)         // ha!
#define DOUBLE_SIZE sizeof(double)     // ugh...
#define INTPTR_T_SIZE sizeof(intptr_t) // ugh...
#define WCHAR_T_SIZE sizeof(wchar_t)

SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv);
void *sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed);
static size_t _alignof(pTHX_ SV *type);

SV *wchar2utf(pTHX_ wchar_t *src, int len) {
#if _WIN32
    size_t outlen = WideCharToMultiByte(CP_UTF8, 0, src, len, NULL, 0, NULL, NULL);
    char *r = (char *)safecalloc(outlen + 1, sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, src, len, r, outlen, NULL, NULL);
    SV *RETVAL = newSVpvn_utf8(r, strlen(r), true);
    safefree((DCpointer)r);
#else
    SV *RETVAL = newSV(0);
    U8 *dst = (U8 *)safecalloc(len + 1, WCHAR_T_SIZE);
    U8 *d = dst;
    while (*src)
        d = uvchr_to_utf8(d, *src++);
    sv_setpv(RETVAL, (char *)dst);
    sv_utf8_decode(RETVAL);
    safefree(dst);
#endif
    return RETVAL;
}

wchar_t *utf2wchar(pTHX_ SV *src, int len) {
    wchar_t *RETVAL = (wchar_t *)safemalloc((len + 1) * WCHAR_T_SIZE);
#ifdef _WIN32
    MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, SvPV_nolen(src), -1, RETVAL, len + 1);
#else
    U8 *raw = (U8 *)SvPV_nolen(src);
    wchar_t *p = RETVAL;
    if (SvUTF8(src)) {
        STRLEN len;
        while (*raw) {
            *p++ = utf8_to_uvchr_buf(raw, raw + WCHAR_T_SIZE, &len);
            raw += len;
        }
    }
    else {
        while (*raw) {
            *p++ = (wchar_t)*raw++;
        }
    }
    *p = 0;
#endif
    return RETVAL;
}
#endif
