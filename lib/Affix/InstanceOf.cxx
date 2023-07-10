#include "../Affix.h"

XS_INTERNAL(Affix_Type_InstanceOf) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    HV *RETVAL_HV = newHV();
    AV *fields = MUTABLE_AV(SvRV(ST(0)));
    bool rw = false;
    switch (av_count(fields)) {
    case 2: {
        SV **rw_ref = av_fetch(fields, 1, 0);
        rw = SvTRUE(*rw_ref);
    } // fall through
    case 1: {
        SV **type_ref = av_fetch(fields, 0, 0);
        SV *type = *type_ref;
        if (!(sv_isobject(type) && sv_derived_from(type, "Affix::Type::Base")))
            croak("Pointer[...] expects a subclass of Affix::Type::Base");
        //~ sv_dump(type);
        hv_stores(RETVAL_HV, "type", SvREFCNT_inc(type));
    } break;
    default:
        croak("Pointer[...] expects a single type. e.g. Pointer[Int]");
    };
    ST(0) = sv_2mortal(
        sv_bless(newRV_inc(MUTABLE_SV(RETVAL_HV)),
                 gv_stashpv(rw ? "Affix::Type::RWPointer" : "Affix::Type::Pointer", GV_ADD)));
    XSRETURN(1);
}

void boot_Affix_InstanceOf(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);

    EXT_TYPE(InstanceOf, AFFIX_ARG_CPOINTER, AFFIX_ARG_CPOINTER);

    set_isa("Affix::InstanceOf", "Affix::Pointer");
}
