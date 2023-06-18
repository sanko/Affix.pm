SV *wchar2utf(pTHX_ const wchar_t *str, int len);
wchar_t *utf2wchar(const char *str, int len);
SV *ptr2sv(pTHX_ DCpointer ptr, SV *type_sv);
void *sv2ptr(pTHX_ SV *type_sv, SV *data, DCpointer ptr, bool packed);
static size_t _alignof(pTHX_ SV *type);
