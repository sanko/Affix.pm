#include "../Affix.h"

DCaggr *_aggregate(pTHX_ SV *type) {
#if DEBUG
    warn("_aggregate(%s)", AXT_STRINGIFY(type));
#endif
    DCaggr *retval = NULL;
    size_t size, array_len;
    switch (AXT_NUMERIC(type)) {
    case ARRAY_FLAG: {
        array_len = AXT_ARRAYLEN(type);
        if (!array_len) return NULL;
    }
    // fall-through
    case STRUCT_FLAG:
    case CPPSTRUCT_FLAG:
    case UNION_FLAG: {
        size = AXT_SIZEOF(type);
        SV **agg_sv_ptr = AXT_AGGREGATE(type);
        if (agg_sv_ptr != NULL && SvOK(*agg_sv_ptr)) {
            PING;
            sv_dump(*agg_sv_ptr);
            if (sv_derived_from(*agg_sv_ptr, "Affix::Pointer")) {
                IV tmp = SvIV((SV *)SvRV(*agg_sv_ptr));
                return INT2PTR(DCaggr *, tmp);
            }
            else
                croak("Oh, no...");
        }
        else {
            PING;
            SV *fields = AXT_SUBTYPE(type);
            sv_dump(fields);
            /*

            //~ if (t == STRUCT_FLAG) {
            //~ SV **sv_packed = hv_fetchs(MUTABLE_HV(SvRV(type)), "packed", 0);
            //~ }
            AV *idk_arr = MUTABLE_AV(SvRV(*idk_wtf));
            size_t field_count = av_count(idk_arr);
            retval = dcNewAggr(field_count, size);
#if DEBUG
            warn("*retval [%p] = dcNewAggr(%ld, %ld);", (DCpointer)retval, field_count, size);
#endif
            for (size_t i = 0; i < field_count; ++i) {
                SV **field_ptr = av_fetch(idk_arr, i, 0);
                AV *field = MUTABLE_AV(SvRV(*field_ptr));
                SV **subtype = av_fetch(field, 1, 0);
                size_t offset = AXT_OFFSET(*subtype);
                int _t = SvIV(*subtype);
                switch (_t) {
                case STRUCT_FLAG:
                case CPPSTRUCT_FLAG:
                case UNION_FLAG: {
                    DCaggr *child = _aggregate(aTHX_ * subtype);
#if DEBUG
                    warn("  dcAggrField(%p, DC_SIGCHAR_AGGREGATE, %ld, 1, child);",
(DCpointer)retval, offset); #endif dcAggrField(retval, DC_SIGCHAR_AGGREGATE, offset, 1, child); }
break; case ARRAY_FLAG: { #if DEBUG warn("  dcAggrField(%p, '%c', %ld, %d);", (DCpointer)retval,
type_as_dc(_t), offset, array_len); #endif dcAggrField(retval, type_as_dc(_t), offset, array_len);
                } break;
                default: {
#if DEBUG
                    warn("  dcAggrField(%p, %c, %ld, 1);", (void *)retval, type_as_dc(_t), offset);
#endif
                    dcAggrField(retval, type_as_dc(_t), offset, 1);
                } break;
                }
            }
#ifdef DEBUG
            warn("dcCloseAggr(%p);", (DCpointer)retval);
#endif
            dcCloseAggr(retval);
            {
                SV *RETVALSV;
                RETVALSV = newSV(1);
                sv_setref_pv(RETVALSV, "Affix::Pointer", (DCpointer)retval);
                av_store(MUTABLE_AV(SvRV(type)), 7, newSVsv(RETVALSV));
            }*/
#if DEBUG
            warn("/_aggregate(%s)", AXT_STRINGIFY(type));
#endif
        }
    } break;
    default: {
        croak("Unsupported aggregate: %s at %s line %d", AXT_STRINGIFY(type), __FILE__, __LINE__);
        break;
    }
    }
#if DEBUG
    warn("/_aggregate(%s) == NULL", AXT_STRINGIFY(type));
#endif
    return retval;
}

XS_INTERNAL(Affix_Aggregate_FETCH) {
    dVAR;
    dXSARGS;
    if (items != 2) croak_xs_usage(cv, "union, key");
    SV *RETVAL = newSV(0);
    HV *h = MUTABLE_HV(SvRV(ST(0)));
    SV **subtype = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*hv_fetchs(h, "type", 0)))), "fields", 0);
    SV **ptr_ptr = hv_fetchs(h, "pointer", 0);
    IV tmp = SvIV(SvRV(*ptr_ptr));
    char *key = SvPV_nolen(ST(1));
#if DEBUG > 1
    warn("Affix::Aggregate::FETCH( '%s' ) @ %p", key, INT2PTR(DCpointer, tmp));
    sv_dump(ST(0));
#endif
    AV *types = MUTABLE_AV(SvRV(*subtype));
    SSize_t size = av_count(types);
    for (SSize_t i = 0; i < size; ++i) {
        SV **elm = av_fetch(types, i, 0);
        SV **name = av_fetch(MUTABLE_AV(SvRV(*elm)), 0, 0);
        if (strcmp(key, SvPV(*name, PL_na)) == 0) {
            SV *_type = *av_fetch(MUTABLE_AV(SvRV(*elm)), 1, 0);
            size_t offset = AXT_OFFSET(_type); // meaningless for union
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
    SV **subtype = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*subtype))), "fields", 0);
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
    SV **subtype = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*subtype))), "fields", 0);
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
    SV **subtype = hv_fetchs(h, "type", 0);
    SV **type = hv_fetchs(MUTABLE_HV(SvRV(SvRV(*subtype))), "fields", 0);
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
