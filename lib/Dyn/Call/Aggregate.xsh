MODULE = Dyn::Call   PACKAGE = Dyn::Call::Aggregate

BOOT:
    set_isa("Dyn::Call::Aggregate",  "Dyn::Call::Pointer");

void
new(char * package, HV * args = newHV_mortal())
PPCODE:
    // Do not mention this constructor; prefer dcNewAggr(...)
    struct DCaggr_ * RETVAL;
    Newx(RETVAL, 1, struct DCaggr_);
    SV ** val_ref = hv_fetchs(args, "size", 0);
    if (val_ref != NULL)
        RETVAL->size = (DCsize)SvIV(*val_ref);
    val_ref = hv_fetchs(args, "n_fields", 0);
    if (val_ref != NULL)
        RETVAL->n_fields = (DCsize)SvIV(*val_ref);
    val_ref = hv_fetchs(args, "alignment", 0);
    if (val_ref != NULL)
        RETVAL->alignment = (DCsize)SvIV(*val_ref);
    {
        SV * RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, package, (void*)RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);

DCsize
_aggr(DCaggr * thing)
ALIAS:
    size      = 1
    n_fields  = 2
    alignment = 3
CODE:
    switch(ix) {
        case 1: RETVAL = thing->size;      break;
        case 2: RETVAL = thing->n_fields;  break;
        case 3: RETVAL = thing->alignment; break;
        default:
            croak("Unknown aggr attribute: %d", ix); break;
    }
OUTPUT:
    RETVAL

void
fields(DCaggr * thing)
PREINIT:
    size_t i;
    U8 gimme = GIMME_V;
PPCODE:
    if (gimme == G_ARRAY) {
        EXTEND(SP, thing->n_fields);
        struct DCfield_ * addr;
        for (i = 0; i < thing->n_fields; ++i) {
            SV * field  = sv_newmortal();
            addr = &thing->fields[i];
            sv_setref_pv(field, "Dyn::Call::Field", (void*) addr);
            mPUSHs(newSVsv(field));
        }
    }
    else if (gimme == G_SCALAR)
        mXPUSHi(thing->n_fields);

MODULE = Dyn::Call   PACKAGE = Dyn::Call

DCaggr *
dcNewAggr( DCsize maxFieldCount, DCsize size )

void
dcFreeAggr( DCaggr * ag )
CODE:
    dcFreeAggr(ag);
    SV* sv = (SV*) &PL_sv_undef;
    sv_setsv(ST(0), sv);

void
dcAggrField( DCaggr * ag, DCchar type, DCint offset, DCsize arrayLength, ... )

void
dcCloseAggr( DCaggr * ag )

BOOT:
    export_function("Dyn::Call", "dcNewAggr",   "aggr");
    export_function("Dyn::Call", "dcAggrField", "aggr");
    export_function("Dyn::Call", "dcCloseAggr", "aggr");
    export_function("Dyn::Call", "dcFreeAggr",  "aggr");

INCLUDE: Call/Field.xsh