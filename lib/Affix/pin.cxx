#include "../Affix.h"

/* Affix::pin( ... ) System
Bind an exported variable to a perl var */

typedef struct { // Used in CUnion and pin()
    void *ptr;
    SV *type_sv;
} var_ptr;

int get_pin(pTHX_ SV *sv, MAGIC *mg) {
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    SV *val = ptr2sv(aTHX_ ptr->ptr, ptr->type_sv);
    sv_setsv((sv), val);
    return 0;
}

int set_pin(pTHX_ SV *sv, MAGIC *mg) {
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    if (SvOK(sv)) {
        DCpointer block = sv2ptr(aTHX_ ptr->type_sv, sv, false);
        Move(block, ptr->ptr, /* cache this? */ AFFIX_SIZEOF(ptr->type_sv), char);
        safefree(block);
    }
    return 0;
}

int free_pin(pTHX_ SV *sv, MAGIC *mg) {
    PERL_UNUSED_VAR(sv);
    var_ptr *ptr = (var_ptr *)mg->mg_ptr;
    sv_2mortal(ptr->type_sv);
    safefree(ptr);
    return 0;
}

static MGVTBL pin_vtbl = {
    get_pin,  // get
    set_pin,  // set
    NULL,     // len
    NULL,     // clear
    free_pin, // free
    NULL,     // copy
    NULL,     // dup
    NULL      // local
};

XS_INTERNAL(Affix_pin) {
    dXSARGS;
    if (items != 4) croak_xs_usage(cv, "var, lib, symbol, type");
    DLLib *_lib;
    // pin( my $integer, 't/src/58_affix_import_vars', 'integer', Int );

    if (!SvOK(ST(1)))
        _lib = NULL;
    else if (SvROK(ST(1)) && sv_derived_from(ST(1), "Affix::Lib")) {
        IV tmp = SvIV(MUTABLE_SV(SvRV(ST(1))));
        _lib = INT2PTR(DLLib *, tmp);
    }
    else {
        SV *lib, *ver;
        if (UNLIKELY(SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV)) {
            AV *tmp = MUTABLE_AV(SvRV(ST(1)));
            size_t tmp_len = av_count(tmp);
            // Non-fatal
            if (UNLIKELY(!(tmp_len == 1 || tmp_len == 2))) { warn("Expected a lib and version"); }
            lib = *av_fetch(tmp, 0, false);
            ver = *av_fetch(tmp, 1, false);
        }
        else {
            lib = newSVsv(ST(1));
            ver = newSV(0);
        }
        const char *_libpath = locate_lib(aTHX_ lib, ver);
        _lib =
#if defined(DC__OS_Win64) || defined(DC__OS_MacOSX)
            dlLoadLibrary(_libpath);
#else
            (DLLib *)dlopen(_libpath, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
        if (_lib == NULL) { croak("Failed to load %s: %s", _libpath, dlerror()); }
    }
    const char *symbol = (const char *)SvPV_nolen(ST(2));
    DCpointer ptr = dlFindSymbol(_lib, symbol);
    if (ptr == NULL) { croak("Failed to locate '%s'", symbol); }
    SV *sv = ST(0);
    MAGIC *mg = sv_magicext(sv, NULL, PERL_MAGIC_ext, &pin_vtbl, NULL, 0);
    {
        var_ptr *_ptr;
        Newx(_ptr, 1, var_ptr);
        _ptr->ptr = ptr;
        _ptr->type_sv = newSVsv(ST(3));
        mg->mg_ptr = (char *)_ptr;
    }
    XSRETURN_YES;
}

XS_INTERNAL(Affix_unpin) {
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "var");
    if (mg_findext(ST(0), PERL_MAGIC_ext, &pin_vtbl) &&
        !sv_unmagicext(ST(0), PERL_MAGIC_ext, &pin_vtbl))
        XSRETURN_YES;
    XSRETURN_NO;
}

void boot_Affix_pin(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);
    (void)newXSproto_portable("Affix::pin", Affix_pin, __FILE__, "$$$$");
    export_function("Affix", "pin", "base");
    (void)newXSproto_portable("Affix::unpin", Affix_unpin, __FILE__, "$");
    export_function("Affix", "unpin", "base");
}
