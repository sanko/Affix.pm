#include "../Affix.h"

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
