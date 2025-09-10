#include "../Affix.h"

/* Affix::pin( ... ) System
Bind an exported variable to a perl var */

void delete_pin(pTHX_ Affix_Pin * pin) {
    //~ warn(">>> delete_pin");
    if (pin == NULL)
        return;
    if (pin->managed && pin->pointer != NULL)  // TODO: Use type to deeply free pointer
        safefree(pin->pointer);
    /*if (pin->type != NULL)
        destroy_Affix_Type(aTHX_ pin->type);
    safefree(pin);
    pin = NULL;*/
}

int Affix_get_pin(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin * ptr = (Affix_Pin *)mg->mg_ptr;
    if (ptr == NULL || ptr->type == NULL || ptr->pointer == NULL) {
        sv_setsv_mg(sv, &PL_sv_undef);
        return 0;
    }
    SV * val = ptr->type->fetch(aTHX_ ptr->type, ptr->pointer, sv);
    sv_setsv_mg(sv, val);
    return 0;
}

int Affix_set_pin(pTHX_ SV * sv, MAGIC * mg) {
    Affix_Pin * ptr = (Affix_Pin *)mg->mg_ptr;
    if (ptr == NULL || ptr->type == NULL || ptr->pointer == NULL) {
        sv_setsv_mg(sv, &PL_sv_undef);
        return 0;
    }
    ptr->type->store(aTHX_ ptr->type, sv, &ptr->pointer);
    return 0;
}

int Affix_free_pin(pTHX_ SV * sv, MAGIC * mg) {
    PERL_UNUSED_VAR(sv);
    Affix_Pin * pin = (Affix_Pin *)mg->mg_ptr;
    delete_pin(aTHX_ pin);
    return 0;
}

static MGVTBL Affix_pin_vtbl = {
    Affix_get_pin,   // get
    Affix_set_pin,   // set
    NULL,            // len
    NULL,            // clear
    Affix_free_pin,  // free
    NULL,            // copy
    NULL,            // dup
    NULL             // local
};

bool is_pin(pTHX_ SV * sv) {
    return (sv && SvTYPE(sv) >= SVt_PVMG && mg_findext(sv, PERL_MAGIC_ext, &Affix_pin_vtbl)) ? true : false;
}

void pin(pTHX_ Affix_Type * type, SV * sv, DCpointer ptr) {
    MAGIC * mg;
    if (is_pin(aTHX_ sv))
        mg = mg_findext(sv, PERL_MAGIC_ext, &Affix_pin_vtbl);
    else
        mg = sv_magicext(sv, NULL, PERL_MAGIC_ext, &Affix_pin_vtbl, NULL, 0);
    // Newxz(mg->mg_ptr, 1, _Affix_Pin);
    Affix_Pin * pin;
    Newxz(pin, 1, Affix_Pin);
    pin->pointer = ptr;
    pin->managed = false;  // TODO: expose this to user
    pin->type = type;
    mg->mg_ptr = (char *)pin;
    // Move(pin, mg->mg_ptr, sizeof(_Affix_Pin), char);
    mg_magical(sv);
}
////////////////////////////////////////////////////////////////////
/*

int get_pin(pTHX_ SV * sv, MAGIC * mg) {
    _Affix_Pin * ptr = (_Affix_Pin *)mg->mg_ptr;
    SV * val = ptr->type->toSV(aTHX_ ptr->ptr);
    sv_setsv_mg(sv, val);
    return 0;
}

int set_pin(pTHX_ SV * sv, MAGIC * mg) {
    _Affix_Pin * ptr = (_Affix_Pin *)mg->mg_ptr;
    ptr->type->toPtr(aTHX_ sv, ptr->ptr);
    return 0;
}

int free_pin(pTHX_ SV * sv, MAGIC * mg) {
    PERL_UNUSED_VAR(sv);
    _Affix_Pin * pin = (_Affix_Pin *)mg->mg_ptr;
    delete pin;
    return 0;
}

static MGVTBL pin_vtbl = {
    get_pin,   // get
    set_pin,   // set
    NULL,   // len
    NULL,   // clear
    free_pin,  // free
    NULL,   // copy
    NULL,   // dup
    NULL    // local
};

bool is_pin(pTHX_ SV * sv) {
    return (sv && SvTYPE(sv) >= SVt_PVMG && mg_findext(sv, PERL_MAGIC_ext, &pin_vtbl)) ? true : false;
}

DCpointer get_pin_pointer(pTHX_ SV * sv) {
    MAGIC * mg = mg_findext(sv, PERL_MAGIC_ext, &pin_vtbl);
    if (mg == NULL)
        return NULL;
    Affix_Pin * ptr = (Affix_Pin *)mg->mg_ptr;
    return ptr->ptr;
}

Affix_Pin * get_pin(pTHX_ SV * sv) {
    MAGIC * mg = mg_findext(sv, PERL_MAGIC_ext, &pin_vtbl);
    if (mg == NULL)
        return NULL;
    return (Affix_Pin *)mg->mg_ptr;
}

void _pin(pTHX_ SV * sv, Affix_Type * type, DCpointer ptr) {
    MAGIC * mg;
    if (is_pin(aTHX_ sv))
        mg = mg_findext(sv, PERL_MAGIC_ext, &pin_vtbl);
    else
        mg = sv_magicext(sv, NULL, PERL_MAGIC_ext, &pin_vtbl, NULL, 0);
    mg->mg_ptr = (char *)new Affix_Pin(ptr, type);
    mg_magical(sv);
}
*/
XS_INTERNAL(Affix_pin) {
    dXSARGS;
    if (items != 4)
        croak_xs_usage(cv, "var, lib, symbol, type");
    DLLib * _lib;
    // pin( my $integer, 't/src/58_affix_import_vars', 'integer', Int );
    STRLEN len;
    {
        SV * const xsub_tmp_sv = ST(1);
        SvGETMAGIC(xsub_tmp_sv);

        if (!SvOK(xsub_tmp_sv) && SvREADONLY(xsub_tmp_sv))  // explicit undef
            _lib = dlLoadLibrary(NULL);
        else if (sv_isobject(xsub_tmp_sv) && sv_derived_from(xsub_tmp_sv, "Affix::Lib")) {
            IV tmp = SvIV((SV *)SvRV(xsub_tmp_sv));
            _lib = INT2PTR(DLLib *, tmp);
        }
        else if (NULL == (_lib = dlLoadLibrary(SvPVbyte_or_null(ST(1), len)))) {
            /*Stat_t statbuf;
            Zero(&statbuf, 1, Stat_t);
            if (PerlLIO_stat(SvPV_nolen(xsub_tmp_sv), &statbuf) < 0) {
                ENTER;
                SAVETMPS;
                PUSHMARK(SP);
                XPUSHs(xsub_tmp_sv);
                PUTBACK;
                int count = call_pv("Affix::find_library", G_SCALAR);
                SPAGAIN;
                _lib = load_library(SvPV_nolen(POPs));
                PUTBACK;
                FREETMPS;
                LEAVE;
            }*/
        }
        if (!_lib) {
            // TODO: Throw an error
            safefree(_lib);
            croak("Failed to load library");
            XSRETURN_UNDEF;
        }
    }
    char * symbol = SvPVbyte_or_null(ST(2), len);
    DCpointer ptr = dlFindSymbol(_lib, symbol);
    if (ptr == NULL)
        croak("Failed to locate '%s' in %s", symbol, SvPVbyte_or_null(ST(1), len));
    Affix_Type * type = new_Affix_Type(aTHX_ ST(3));

    if (type == NULL)
        XSRETURN_EMPTY;
    //~ type->toPin()
    //~ type->pin(aTHX_ ST(0), ptr);
    pin(aTHX_ type, ST(0), ptr);

    XSRETURN_YES;
}

XS_INTERNAL(Affix_unpin) {
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "var");
    if (mg_findext(ST(0), PERL_MAGIC_ext, &Affix_pin_vtbl) && !sv_unmagicext(ST(0), PERL_MAGIC_ext, &Affix_pin_vtbl))
        XSRETURN_YES;
    XSRETURN_NO;
}

XS_INTERNAL(Affix_is_pin) {
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "var");
    if (is_pin(aTHX_ ST(0)))
        XSRETURN_YES;
    XSRETURN_NO;
}

void boot_Affix_pin(pTHX_ CV * cv) {
    PERL_UNUSED_VAR(cv);
    (void)newXSproto_portable("Affix::pin", Affix_pin, __FILE__, "$$$$");
    export_function("Affix", "pin", "base");
    (void)newXSproto_portable("Affix::unpin", Affix_unpin, __FILE__, "$");
    export_function("Affix", "unpin", "base");
    (void)newXSproto_portable("Affix::is_pin", Affix_is_pin, __FILE__, "$");
}
