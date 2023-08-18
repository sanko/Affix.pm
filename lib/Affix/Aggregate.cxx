#include "../Affix.h"

DCaggr *_aggregate(pTHX_ SV *type) {
#if DEBUG
    warn("_aggregate(...)");
#endif
    int t = SvIV(type);
    size_t size = _sizeof(aTHX_ type);
    switch (t) {
    case AFFIX_TYPE_CSTRUCT:
    case AFFIX_TYPE_CPPSTRUCT:
    case AFFIX_TYPE_CARRAY:
    case AFFIX_TYPE_CUNION: {
        HV *hv_type = MUTABLE_HV(SvRV(type));
        SV **agg_sv_ptr = hv_fetch(hv_type, "aggregate", 9, 0);
        if (agg_sv_ptr != NULL) {
            PING;
            SV *agg_sv = *agg_sv_ptr;
            if (sv_derived_from(agg_sv, "Affix::Aggregate")) {
                IV tmp = SvIV((SV *)SvRV(agg_sv));
                return INT2PTR(DCaggr *, tmp);
            }
            else
                croak("Oh, no...");
        }
        else {
            PING;
            SV **idk_wtf = hv_fetchs(MUTABLE_HV(SvRV(type)), "fields", 0);

            //~ if (t == AFFIX_TYPE_CSTRUCT) {
            //~ SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
            //~ }
            AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
            size_t field_count = av_count(idk_arr);
            DCaggr *agg = dcNewAggr(field_count, size);
#if DEBUG
            warn("DCaggr *agg [%p] = dcNewAggr(%ld, %ld);", (void *)agg, field_count, size);
#endif
            for (size_t i = 0; i < field_count; ++i) {
                SV **field_ptr = av_fetch(idk_arr, i, 0);
                AV *field = MUTABLE_AV(SvRV(*field_ptr));
                SV **type_ptr = av_fetch(field, 1, 0);
                size_t offset = _offsetof(aTHX_ * type_ptr);
                int _t = SvIV(*type_ptr);
                switch (_t) {
                case AFFIX_TYPE_CSTRUCT:
                case AFFIX_TYPE_CUNION: {
                    DCaggr *child = _aggregate(aTHX_ * type_ptr);
#if DEBUG
                    warn("  dcAggrField(%p, DC_SIGCHAR_AGGREGATE, %ld, 1, child);", (void *)agg,
                         offset);
#endif
                    dcAggrField(agg, DC_SIGCHAR_AGGREGATE, offset, 1, child);
                } break;
                case AFFIX_TYPE_CARRAY: {
                    int array_len = SvIV(*hv_fetchs(MUTABLE_HV(SvRV(*type_ptr)), "size", 0));
#if DEBUG
                    warn("  dcAggrField(%p, '%c', %ld, %d);", (void *)agg, type_as_dc(_t), offset,
                         array_len);
#endif
                    dcAggrField(agg, type_as_dc(_t), offset, array_len);
                } break;
                default: {
#if DEBUG
                    warn("  dcAggrField(%p, %c, %ld, 1);", (void *)agg, type_as_dc(_t), offset);
#endif
                    dcAggrField(agg, type_as_dc(_t), offset, 1);
                } break;
                }
            }
#ifdef DEBUG
            warn("dcCloseAggr(%p);", (void *)agg);
#endif
            dcCloseAggr(agg);
            {
                SV *RETVALSV;
                RETVALSV = newSV(1);
                sv_setref_pv(RETVALSV, "Affix::Aggregate", (DCpointer)agg);
                hv_stores(MUTABLE_HV(SvRV(type)), "aggregate", newSVsv(RETVALSV));
            }
            return agg;
        }
    } break;
    default: {
        croak("Unsupported aggregate: %s at %s line %d", type_as_str(t), __FILE__, __LINE__);
        break;
    }
    }
    PING;
    return NULL;
}

XS_INTERNAL(Affix_Aggregate_FETCH) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    SV *RETVAL = newSV(0);
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*hv_fetchs(h, "type", 0)))), "fields", 0);
    SV **ptr_ptr = hv_fetchs(h, "pointer", 0);
    IV tmp = SvIV(SvRV(*ptr_ptr));
    char *key = SvPV_nolen(ST(1));
#if DEBUG > 1
    warn("Affix::Aggregate::FETCH( '%s' ) @ %p", key, INT2PTR(DCpointer, tmp));
    sv_dump(ST(0));
#endif
    AV *types = MUTABLE_AV(SvRV(*type_ptr));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(key, SvPV(*name, PL_na)) == 0) {
            SV *_type = *av_fetch(MUTABLE_AV(SvRV(*elm)), 1, 0);
            size_t offset = _offsetof(aTHX_ _type); // meaningless for union
            sv_setsv(RETVAL, sv_2mortal(ptr2sv(aTHX_ INT2PTR(DCpointer, tmp + offset), _type)));
            break;
        }
    }
    ST(0) = RETVAL;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Aggregate_EXISTS) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    char *u = SvPV_nolen(ST(1));
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(u, SvPV(*name, PL_na)) == 0) { XSRETURN_YES; }
    }
    XSRETURN_NO;
}

XS_INTERNAL(Affix_Aggregate_FIRSTKEY) {
    dVAR;
    dXSARGS;
    if (items != 1) croak_xs_usage(cv, "union");
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        ST(0) = sv_mortalcopy(*name);
        XSRETURN(1);
    }
    XSRETURN_UNDEF;
}

XS_INTERNAL(Affix_Aggregate_NEXTKEY) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **type_ptr = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*type_ptr))), "fields", 0);
    char *u = SvPV_nolen(ST(1));
    AV *types = MUTABLE_AV(SvRV(*type));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(u, SvPV(*name, PL_na)) == 0 && i + 1 < size) {
            elm = av_fetch(types, i + 1, 0);
            name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
            ST(0) = sv_mortalcopy(*name);
            XSRETURN(1);
        }
    }
    XSRETURN_UNDEF;
}

// TODO: DESTROY and free the pointer

void boot_Affix_Aggregate(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);
    (void)newXSproto_portable("Affix::Aggregate::FETCH", Affix_Aggregate_FETCH, __FILE__, "$$");
    (void)newXSproto_portable("Affix::Aggregate::EXISTS", Affix_Aggregate_EXISTS, __FILE__, "$$");
    (void)newXSproto_portable("Affix::Aggregate::FIRSTKEY", Affix_Aggregate_FIRSTKEY, __FILE__,
                              "$");
    (void)newXSproto_portable("Affix::Aggregate::NEXTKEY", Affix_Aggregate_NEXTKEY, __FILE__, "$$");
}