#include "../Affix.h"

XS_INTERNAL(Affix_Type_InstanceOf) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    SV *rw_ref = NULL;
    switch (av_count(fields)) {
    case 2: {
        rw_ref = *av_fetch(fields, 1, 0);
    } // fall through
    case 1: {
        SV **type_ref = av_fetch(fields, 0, 0);
        SV *type = *type_ref;
        if (!(SvPOK(type))) croak("InstanceOf[...] expects a class");
        //~ sv_dump(type);
        set_isa(SvPVx_nolen(type), "Affix::InstanceOf");
        hv_stores(RETVAL_HV, "type",
                  sv_bless(newRV_noinc(newSV(0)), gv_stashpv("Affix::Type::Void", GV_ADD)));
        hv_stores(RETVAL_HV, "class", SvREFCNT_inc(type));
        hv_stores(RETVAL_HV, "rw", SvREFCNT_inc(rw_ref == NULL ? newSV_false() : rw_ref));
    } break;
    default:
        croak("InstanceOf[...] expects a single class. e.g. InstanceOf['MyClass']");
    };
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)), gv_stashpv("Affix::Type::InstanceOf", GV_ADD)));
    XSRETURN(1);
}

void boot_Affix_InstanceOf(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);
    EXT_TYPE(InstanceOf, AFFIX_ARG_CPOINTER, AFFIX_ARG_CPOINTER);
    set_isa("Affix::InstanceOf", "Affix::Pointer::Unmanaged");
}
