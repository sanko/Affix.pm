#include "lib/clutter.h"

void unroll_aggregate(void *ptr, DCaggr *ag, SV *obj) {
 warn("unroll_aggregate");
    //*(int*)ptr = 42;
    char *base;
    size_t offset;
    int *b;
    // get base address
    base = (char *)ptr;
    DCsize i = ag->n_fields;
    warn("ag->n_fields == %d", ag->n_fields);
    for (int i = 0; i < ag->n_fields; ++i) {
        warn("i==%d", i);
        switch (ag->fields[i].type) {
        case DC_SIGCHAR_BOOL:
            warn("bool!!!!!");
            break;
        case DC_SIGCHAR_UCHAR:
            warn("uchar!!!!!");
            // and the offset to member_b
            offset = ag->fields[i].offset;

            // Compute address of member_b
            b = (int *)(base + offset);

            //warn(".a == %c", (unsigned char)b);
            // av_store(obj, i,newSVuv((unsigned char) b));
            break;
        default:
            warn("fallthrough");
            break;
        }
    }
}

/*
#include "object_pad.h"

#include "/home/sanko/Downloads/libui-ng-master/ui.h"

uiMultilineEntry *e;

int sayTime(void *data)
{
	time_t t;
	char *s;

	t = time(NULL);
	s = ctime(&t);

	uiMultilineEntryAppend(e, s);
	return 1;
}

int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

void saySomething(uiButton *b, void *data)
{
	uiMultilineEntryAppend(e, "Saying something\n");
}

*/

MODULE = Dyn::Call   PACKAGE = Dyn::Call

DCCallVM *
dcNewCallVM(DCsize size);

void
dcFree(DCCallVM * vm)
CODE:
    dcFree(vm);
    SV* sv = (SV*) &PL_sv_undef;
    sv_setsv(ST(0), sv);

void
dcReset(DCCallVM * vm);

void
dcMode(DCCallVM * vm, DCint mode);

void
dcBeginCallAggr(DCCallVM * vm, DCaggr * ag);

void
dcArgBool(DCCallVM * vm, DCbool arg);

void
dcArgChar(DCCallVM * vm, DCchar arg);

void
dcArgShort(DCCallVM * vm, DCshort arg);

void
dcArgInt(DCCallVM * vm, DCint arg);

void
dcArgLong(DCCallVM * vm, DClong arg);

void
dcArgLongLong(DCCallVM * vm, DClonglong arg);

void
dcArgFloat(DCCallVM * vm, DCfloat arg);

void
dcArgDouble(DCCallVM * vm, DCdouble arg);

void
dcArgPointer(DCCallVM * vm, arg);
CODE:
    if (sv_derived_from(ST(1), "Dyn::Callback") ) {
        IV tmp = SvIV((SV*)SvRV(ST(1)));
        DCCallback * arg = INT2PTR(DCCallback *, tmp);
        dcArgPointer(vm, arg);
    }
    else if (sv_derived_from(ST(1), "Dyn::Call::Pointer")){
        IV tmp = SvIV((SV*)SvRV(ST(1)));
        DCpointer arg = INT2PTR(DCpointer, tmp);
        dcArgPointer(vm, arg);
    }
    else
        croak("arg is not of type Dyn::Call::Pointer or Dyn::Callback");

void
dcArgAggr(DCCallVM * vm, DCaggr * s, void * value);

void
dcArgString(DCCallVM * vm, char * arg);
CODE:
    dcArgPointer(vm, arg);

void
dcCallVoid(DCCallVM * vm, DCpointer funcptr);

DCbool
dcCallBool(DCCallVM * vm, DCpointer funcptr);

DCchar
dcCallChar(DCCallVM * vm, DCpointer funcptr);

DCshort
dcCallShort(DCCallVM * vm, DCpointer funcptr);

DCint
dcCallInt(DCCallVM * vm, DCpointer funcptr);

DClong
dcCallLong(DCCallVM * vm, DCpointer funcptr);

DClonglong
dcCallLongLong(DCCallVM * vm, DCpointer funcptr);

DCfloat
dcCallFloat(DCCallVM * vm, DCpointer funcptr);

DCdouble
dcCallDouble(DCCallVM * vm, DCpointer funcptr);

DCpointer
dcCallPointer(DCCallVM * vm, DCpointer funcptr);

DCpointer
dcCallAggr(DCCallVM* vm, DCpointer funcptr, DCaggr* ag, DCpointer ret);

const char *
dcCallString(DCCallVM * vm, DCpointer funcptr);
CODE:
    RETVAL = (const char *) dcCallPointer(vm, funcptr);
OUTPUT:
    RETVAL

=todo

void
dcArgF(DCCallVM * vm, const DCsigchar * signature, ...);

void
dcVArgF(DCCallVM * vm, const DCsigchar * signature, va_list args);

=cut

void
dcCallF(DCCallVM * vm, DCValue * result, DCpointer funcptr, const DCsigchar * signature);
OUTPUT:
    result

=todo

dcCallF(DCCallVM * vm, DCValue * result, DCpointer funcptr, const DCsigchar * signature, ...);

void
dcVCallF(DCCallVM * vm, DCValue * result, DCpointer funcptr, const DCsigchar * signature, va_list args);

=cut

DCint
dcGetError(DCCallVM* vm);





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





=cut

void letsgo(DCpointer * in)
CODE:
	uiInitOptions * o;
    o = (uiInitOptions *) in;
	uiWindow *w;
	uiBox *b;
	uiButton *btn;

	memset(o, 0, sizeof (uiInitOptions));
	if (uiInit(o) != NULL)
		abort();

	w = uiNewWindow("Hello", 320, 240, 0);
	uiWindowSetMargined(w, 1);

	b = uiNewVerticalBox();
	uiBoxSetPadded(b, 1);
	uiWindowSetChild(w, uiControl(b));

	e = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(e, 1);

	btn = uiNewButton("Say Something");
	uiButtonOnClicked(btn, saySomething, NULL);
	uiBoxAppend(b, uiControl(btn), 0);

	uiBoxAppend(b, uiControl(e), 1);

	uiTimer(1000, sayTime, NULL);

	uiWindowOnClosing(w, onClosing, NULL);
	uiControlShow(uiControl(w));
	uiMain();

=cut

MODULE = Dyn::Call   PACKAGE = Dyn::Type::Pointer

BOOT:
    set_isa("Dyn::Type::Pointer",  "Dyn::Call::Pointer");

void
new(char * package, size_t size = 0)
PPCODE:
    DCpointer * RETVAL;
    Newx(RETVAL, size, DCpointer);
    {
        SV * RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, package, (void*)RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);


SV *
cast(DCpointer * obj, char * pkg)
PREINIT:
    dMY_CXT;
CODE:
    warn("Tick");



    /*DCpointer *  pointer;

  // Dyn::Call | DCCallVM * | DCCallVMPtr
  if (sv_derived_from(ST(0), "Dyn::Call::Pointer")){
    IV tmp = SvIV((SV*)SvRV(ST(0)));
    vm = INT2PTR(DCpointer *, tmp);
  }
  else
    croak("obj is not of type Dyn::Call::Pointer");*/

    DumpHex(obj, sizeof(obj));

    SV** classinfo_ref = hv_fetch(MY_CXT.structs, pkg, strlen(pkg), 0);
    warn("Tick");

    HV * action;
    if (classinfo_ref && SvOK(*classinfo_ref))
        action  = (HV*) SvRV(*classinfo_ref);
    else
        croak("Attempt to cast a pointer to %s which is undefined", pkg);


    warn("Tick");
    SV ** fields_ref = hv_fetchs(action, "types", 0);

    AV * fields;
    if (fields_ref && SvOK(*fields_ref)) {
        fields = (AV*) SvRV(*fields_ref);
    }
    else
        croak("Attempt to cast a pointer to %s which is undefined", pkg);

    warn("Tick");


        warn("Tick fdasfdsa");
        AV * ref = newAV();

        int offset= 0;
    for (int i = 0; i < av_count(fields); i++) {
            warn("Tick lllklkokoljk");

        void * holder;
        SV ** field = av_fetch(fields, i, 0);
        char    type = (char)*SvPV_nolen(*field);


    warn("Tick safdafdfdsafds");

        switch(type) {
            case DC_SIGCHAR_UCHAR:{
                warn("Tick reeeeee");

                                warn("Tick ssssss");

                char * holder = (char *) safemalloc(1);
                                warn("Tick rrrrrrr (%d, %p, 1, char)", obj + offset, holder);

                Copy(obj + offset, holder, 1, char);
                                warn("Tick qqqq");

                offset += padding_needed_for( offset, SHORTSIZE );
                av_push(ref, newSVpv(holder, 1));

                warn("---> %s", holder);
            break;}
            default:
                warn("UGH!");
            break;
        }
        //Copy(obj + offset, holder, size, type);
    }

    RETVAL    = sv_bless(newRV_noinc((SV*)ref), gv_stashpv(pkg, TRUE));


    //RETVAL = newSVsv(*fields);
    /*
    for (size_t i = 0; i < av_count((AV*)fields); i++ )
        warn("Tick [%d]", i);
            warn("Tick");

    RETVAL = sv_bless(newRV_noinc((SV*)ref), gv_stashpv(pkg, TRUE));
            warn("Tick");*/



    /*
    hv_store(classinfo,      "fields", 6,           newRV_inc((SV*)avfields),  0);
    hv_store(classinfo,      "types",  5,           newRV_inc((SV*)avtypes),   0);
    hv_store(MY_CXT.structs, pkg,      strlen(pkg), newRV_inc((SV*)classinfo), 0);
    //RETVAL=MY_CXT.structs;*/
    /*
    if(!sv_derived_from(ST(1), "Dyn::Type::Struct"))
        croak("obj is not of type Dyn::Type::Struct");
    if (!(SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVAV))
        croak("invalid instance method invocant: no array ref supplied");

    //SV ** ptr = av_fetch((AV*)SvRV(obj), ix, 1);
    if(ptr == NULL)
        XSRETURN_UNDEF;
    RETVAL = newSVsv(*ptr);*/
    /*{
        CV * cv;
        int i = 0;
        while(av_count(fields)) {
            if (av_count(fields) % 2) {
                SV * sv_type = av_shift(fields);
                if (!sv_type)
                    warn("NOT OKAY!");
                else
                    av_push(avtypes, sv_type);
            }
            else {
                SV * sv_field = av_shift(fields);
                if (!sv_field)
                    warn("NOT OKAY!");
                else {
                    av_push(avfields, sv_field);
                    cv = newXSproto_portable(form("%s::%s", pkg, SvPV_nolen(sv_field)), XS_Dyn__Type__Struct_get, file, "$");
                    XSANY.any_i32 = i++; // Use perl's ALIAS api to pseudo-index aggr's data
                }
            }
        }
    }
*/
OUTPUT:
    RETVAL

# TODO: DESTROY

MODULE = Dyn::Call   PACKAGE = Dyn::Call::Field

void
new(char * package, HV * args = newHV_mortal())
PPCODE:
    DCfield * RETVAL;
    Newx(RETVAL, 1, DCfield);
    SV ** val_ref = hv_fetchs(args, "offset", 0);
    if (val_ref != NULL)
        RETVAL->offset = (DCsize)SvIV(*val_ref);
    val_ref = hv_fetchs(args, "size", 0);
    if (val_ref != NULL)
        RETVAL->size = (DCsize)SvIV(*val_ref);
    val_ref = hv_fetchs(args, "alignment", 0);
    if (val_ref != NULL)
        RETVAL->alignment = (DCsize)SvIV(*val_ref);
    val_ref = hv_fetchs(args, "array_len", 0);
    if (val_ref != NULL)
        RETVAL->array_len = (DCsize)SvIV(*val_ref);
    val_ref = hv_fetchs(args, "type", 0);
    if (val_ref != NULL)
        RETVAL->type = (DCsigchar)*SvPV_nolen(*val_ref);
    // TODO: unwrap     const DCaggr* sub_aggr;

    {
        SV * RETVALSV;
        RETVALSV = sv_newmortal();
        sv_setref_pv(RETVALSV, package, (void*)RETVAL);
        ST(0) = RETVALSV;
    }
    XSRETURN(1);

DCsize
_field(DCfield * thing, int newvalue = 0)
ALIAS:
    offset    = 1
    size      = 2
    alignment = 3
    array_len = 4
CODE:
    warn ("items == %d",items);
    if(items == 2) {
        switch(ix) {
            case 1: thing->offset   = newvalue; break;
            case 2: thing->size     = newvalue; break;
            case 3: thing->alignment= newvalue; break;
            case 4: thing->array_len= newvalue; break;
            default:
                croak("Unknown field attribute: %d", ix); break;
        }
    }
    switch(ix) {
        case 1: RETVAL = thing->offset;    break;
        case 2: RETVAL = thing->size;      break;
        case 3: RETVAL = thing->alignment; break;
        case 4: RETVAL = thing->array_len; break;
        default:
            croak("Unknown field attribute: %d", ix); break;
    }
OUTPUT:
    RETVAL

DCsigchar
type(DCfield * thing, DCsigchar newvalue = (char)0)
CODE:
    if(items == 2)
        thing->type = (char)*SvPV_nolen(ST(1));
    RETVAL = thing->type;
OUTPUT:
    RETVAL

const DCaggr *
sub_aggr(DCfield * thing, DCaggr * aggr = NULL)
CODE:
    if(items == 2)
        thing->sub_aggr = aggr;
    RETVAL = thing->sub_aggr;
OUTPUT:
    RETVAL

MODULE = Dyn::Call   PACKAGE = Dyn::Call::Aggr

BOOT:
    set_isa("Dyn::Call::Aggr",  "Dyn::Call::Pointer");




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

MODULE = Dyn::Call   PACKAGE = Dyn::Type::Struct

BOOT:
    set_isa("Dyn::Type::Struct",  "Dyn::Call::Pointer");

void
new(const char * pkg, HV * data = newHV())
PREINIT:
    dMY_CXT;
PPCODE:
    HV * classinfo = newHV();
    AV * ref = newAV();
    ST(0)    = sv_bless(newRV_noinc((SV*)ref), gv_stashpv(pkg, TRUE));
    XSRETURN(1);

SV *
get(SV * obj)
PREINIT:
    dXSI32;
CODE:
    if(!sv_derived_from(ST(0), "Dyn::Type::Struct"))
        croak("obj is not of type Dyn::Type::Struct");
    if (!(SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVAV))
        croak("invalid instance method invocant: no array ref supplied");
    SV ** ptr = av_fetch((AV*)SvRV(obj), ix, 1);
    if(ptr == NULL)
        XSRETURN_UNDEF;
    RETVAL = newSVsv(*ptr);
OUTPUT:
    RETVAL

HV *
add_fields(const char * pkg, AV * fields)
PREINIT:
    dMY_CXT;
CODE:
    if(av_count(fields) % 2)
        Perl_croak_nocontext("%s: %s must be an even sized list",
				"Dyn::Type::Struct::add_fields",
				"fields");
    AV * avfields = newAV_mortal();
    AV * avtypes  = newAV_mortal();
    //AV * modes  = newAV(); // Currently unused
    (void)newXSproto_portable(form("%s::new", pkg), XS_Dyn__Type__Struct_new, file, "$%");
    set_isa(pkg,  "Dyn::Type::Struct");
    {
        CV * cv;
        int i = 0;
        while(av_count(fields)) {
            if (av_count(fields) % 2) {
                SV * sv_type = av_shift(fields);
                if (!sv_type)
                    warn("NOT OKAY!");
                else
                    av_push(avtypes, sv_type);
            }
            else {
                SV * sv_field = av_shift(fields);
                if (!sv_field)
                    warn("NOT OKAY!");
                else {
                    av_push(avfields, sv_field);
                    cv = newXSproto_portable(form("%s::%s", pkg, SvPV_nolen(sv_field)), XS_Dyn__Type__Struct_get, file, "$");
                    XSANY.any_i32 = i++; // Use perl's ALIAS api to pseudo-index aggr's data
                }
            }
        }
    }

    HV * classinfo = newHV();
    hv_store(classinfo,      "fields", 6,           newRV_inc((SV*)avfields),  0);
    hv_store(classinfo,      "types",  5,           newRV_inc((SV*)avtypes),   0);
    hv_store(MY_CXT.structs, pkg,      strlen(pkg), newRV_inc((SV*)classinfo), 0);
    RETVAL=MY_CXT.structs;
OUTPUT:
    RETVAL

void
DESTROY(void * ptr)
CODE:
    //free(ptr);

void
CLONE(...)
CODE:
      MY_CXT_CLONE;

BOOT:
{
    MY_CXT_INIT;
    {
        dMY_CXT;
        MY_CXT.structs = newHV();
    }

    HV *stash = gv_stashpv("Dyn::Call", 0);
    // Supported Calling Convention Modes
    newCONSTSUB(stash, "DC_CALL_C_DEFAULT", newSViv(DC_CALL_C_DEFAULT));
    newCONSTSUB(stash, "DC_CALL_C_ELLIPSIS", newSViv(DC_CALL_C_ELLIPSIS));
    newCONSTSUB(stash, "DC_CALL_C_ELLIPSIS_VARARGS", newSViv(DC_CALL_C_ELLIPSIS_VARARGS));
    newCONSTSUB(stash, "DC_CALL_C_X86_CDECL", newSViv(DC_CALL_C_X86_CDECL));
    newCONSTSUB(stash, "DC_CALL_C_X86_WIN32_STD", newSViv(DC_CALL_C_X86_WIN32_STD));
    newCONSTSUB(stash, "DC_CALL_C_X86_WIN32_FAST_MS", newSViv(DC_CALL_C_X86_WIN32_FAST_MS));
    newCONSTSUB(stash, "DC_CALL_C_X86_WIN32_FAST_GNU", newSViv(DC_CALL_C_X86_WIN32_FAST_GNU));
    newCONSTSUB(stash, "DC_CALL_C_X86_WIN32_THIS_MS", newSViv(DC_CALL_C_X86_WIN32_THIS_MS));
    newCONSTSUB(stash, "DC_CALL_C_X86_WIN32_THIS_GNU", newSViv(DC_CALL_C_X86_WIN32_THIS_GNU));
    newCONSTSUB(stash, "DC_CALL_C_X64_WIN64", newSViv(DC_CALL_C_X64_WIN64));
    newCONSTSUB(stash, "DC_CALL_C_X64_SYSV", newSViv(DC_CALL_C_X64_SYSV));
    newCONSTSUB(stash, "DC_CALL_C_PPC32_DARWIN", newSViv(DC_CALL_C_PPC32_DARWIN));
    newCONSTSUB(stash, "DC_CALL_C_PPC32_OSX", newSViv(DC_CALL_C_PPC32_OSX));
    newCONSTSUB(stash, "DC_CALL_C_ARM_ARM_EABI", newSViv(DC_CALL_C_ARM_ARM_EABI));
    newCONSTSUB(stash, "DC_CALL_C_ARM_THUMB_EABI", newSViv(DC_CALL_C_ARM_THUMB_EABI));
    newCONSTSUB(stash, "DC_CALL_C_ARM_ARMHF", newSViv(DC_CALL_C_ARM_ARMHF));
    newCONSTSUB(stash, "DC_CALL_C_MIPS32_EABI", newSViv(DC_CALL_C_MIPS32_EABI));
    newCONSTSUB(stash, "DC_CALL_C_MIPS32_PSPSDK", newSViv(DC_CALL_C_MIPS32_PSPSDK));
    newCONSTSUB(stash, "DC_CALL_C_PPC32_SYSV", newSViv(DC_CALL_C_PPC32_SYSV));
    newCONSTSUB(stash, "DC_CALL_C_PPC32_LINUX", newSViv(DC_CALL_C_PPC32_LINUX));
    newCONSTSUB(stash, "DC_CALL_C_ARM_ARM", newSViv(DC_CALL_C_ARM_ARM));
    newCONSTSUB(stash, "DC_CALL_C_ARM_THUMB", newSViv(DC_CALL_C_ARM_THUMB));
    newCONSTSUB(stash, "DC_CALL_C_MIPS32_O32", newSViv(DC_CALL_C_MIPS32_O32));
    newCONSTSUB(stash, "DC_CALL_C_MIPS64_N32", newSViv(DC_CALL_C_MIPS64_N32));
    newCONSTSUB(stash, "DC_CALL_C_MIPS64_N64", newSViv(DC_CALL_C_MIPS64_N64));
    newCONSTSUB(stash, "DC_CALL_C_X86_PLAN9", newSViv(DC_CALL_C_X86_PLAN9));
    newCONSTSUB(stash, "DC_CALL_C_SPARC32", newSViv(DC_CALL_C_SPARC32));
    newCONSTSUB(stash, "DC_CALL_C_SPARC64", newSViv(DC_CALL_C_SPARC64));
    newCONSTSUB(stash, "DC_CALL_C_ARM64", newSViv(DC_CALL_C_ARM64));
    newCONSTSUB(stash, "DC_CALL_C_PPC64", newSViv(DC_CALL_C_PPC64));
    newCONSTSUB(stash, "DC_CALL_C_PPC64_LINUX", newSViv(DC_CALL_C_PPC64_LINUX));
    newCONSTSUB(stash, "DC_CALL_SYS_DEFAULT", newSViv(DC_CALL_SYS_DEFAULT));
    newCONSTSUB(stash, "DC_CALL_SYS_X86_INT80H_LINUX", newSViv(DC_CALL_SYS_X86_INT80H_LINUX));
    newCONSTSUB(stash, "DC_CALL_SYS_X86_INT80H_BSD", newSViv(DC_CALL_SYS_X86_INT80H_BSD));
    newCONSTSUB(stash, "DC_CALL_SYS_PPC32", newSViv(DC_CALL_SYS_PPC32));
    newCONSTSUB(stash, "DC_CALL_SYS_PPC64", newSViv(DC_CALL_SYS_PPC64));

    // Signature characters
    newCONSTSUB(stash, "DC_SIGCHAR_VOID", newSVpv(form("%c",DC_SIGCHAR_VOID), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_BOOL", newSVpv(form("%c",DC_SIGCHAR_BOOL), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CHAR", newSVpv(form("%c",DC_SIGCHAR_CHAR), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_UCHAR", newSVpv(form("%c",DC_SIGCHAR_UCHAR), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_SHORT", newSVpv(form("%c",DC_SIGCHAR_SHORT), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_USHORT", newSVpv(form("%c",DC_SIGCHAR_USHORT), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_INT", newSVpv(form("%c",DC_SIGCHAR_INT), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_UINT", newSVpv(form("%c",DC_SIGCHAR_UINT), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_LONG", newSVpv(form("%c",DC_SIGCHAR_LONG), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_ULONG", newSVpv(form("%c",DC_SIGCHAR_ULONG), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_LONGLONG", newSVpv(form("%c",DC_SIGCHAR_LONGLONG), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_ULONGLONG", newSVpv(form("%c",DC_SIGCHAR_ULONGLONG), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_FLOAT", newSVpv(form("%c",DC_SIGCHAR_FLOAT), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_DOUBLE", newSVpv(form("%c",DC_SIGCHAR_DOUBLE), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_POINTER", newSVpv(form("%c",DC_SIGCHAR_POINTER), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_STRING", newSVpv(form("%c",DC_SIGCHAR_STRING), 1));/* in theory same as 'p', but convenient to disambiguate */
    newCONSTSUB(stash, "DC_SIGCHAR_AGGREGATE", newSVpv(form("%c",DC_SIGCHAR_AGGREGATE), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_ENDARG", newSVpv(form("%c",DC_SIGCHAR_ENDARG), 1));/* also works for end struct */

    /* calling convention / mode signatures */
    newCONSTSUB(stash, "DC_SIGCHAR_CC_PREFIX", newSVpv(form("%c",DC_SIGCHAR_CC_PREFIX), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_DEFAULT", newSVpv(form("%c",DC_SIGCHAR_CC_DEFAULT), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ELLIPSIS", newSVpv(form("%c",DC_SIGCHAR_CC_ELLIPSIS), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ELLIPSIS_VARARGS", newSVpv(form("%c",DC_SIGCHAR_CC_ELLIPSIS_VARARGS), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_CDECL", newSVpv(form("%c",DC_SIGCHAR_CC_CDECL), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_STDCALL", newSVpv(form("%c",DC_SIGCHAR_CC_STDCALL), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_FASTCALL_MS", newSVpv(form("%c",DC_SIGCHAR_CC_FASTCALL_MS), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_FASTCALL_GNU", newSVpv(form("%c",DC_SIGCHAR_CC_FASTCALL_GNU), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_THISCALL_MS", newSVpv(form("%c",DC_SIGCHAR_CC_THISCALL_MS), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_THISCALL_GNU", newSVpv(form("%c",DC_SIGCHAR_CC_THISCALL_GNU), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ARM_ARM", newSVpv(form("%c",DC_SIGCHAR_CC_ARM_ARM), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ARM_THUMB", newSVpv(form("%c",DC_SIGCHAR_CC_ARM_THUMB), 1));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_SYSCALL", newSVpv(form("%c",DC_SIGCHAR_CC_SYSCALL), 1));

    // Error codes
    newCONSTSUB(stash, "DC_ERROR_NONE", newSViv(DC_ERROR_NONE));
    newCONSTSUB(stash, "DC_ERROR_UNSUPPORTED_MODE", newSViv(DC_ERROR_UNSUPPORTED_MODE));

    //void export_constant(const char * package, const char *name, const char *_tag, double val) {
    export_function("Dyn::Call", "dcArgBool", "bin");
    export_function("Dyn::Call", "dcArgChar", "bin");
    export_function("Dyn::Call", "dcArgShort", "bin");
    export_function("Dyn::Call", "dcArgInt", "bin");
    export_function("Dyn::Call", "dcArgLong", "bin");
    export_function("Dyn::Call", "dcArgLongLong", "bin");
    export_function("Dyn::Call", "dcArgFloat", "bin");
    export_function("Dyn::Call", "dcArgDouble", "bin");
    export_function("Dyn::Call", "dcArgPointer", "bin");
    export_function("Dyn::Call", "dcArgString", "bin");
    export_function("Dyn::Call", "dcArgAggr", "bin");

    export_function("Dyn::Call", "dcNewCallVM", "callvm");
    export_function("Dyn::Call", "dcFree", "callvm");
    export_function("Dyn::Call", "dcMode", "callvm");
    export_function("Dyn::Call", "dcReset", "callvm");

    export_function("Dyn::Call", "dcCallVoid", "call");
    export_function("Dyn::Call", "dcCallChar", "call");
    export_function("Dyn::Call", "dcCallInt", "call");
    export_function("Dyn::Call", "dcCallPointer", "call");
    export_function("Dyn::Call", "dcCallAggr", "call");
    export_function("Dyn::Call", "dcCallString", "call");

    export_function("Dyn::Call", "dcNewAggr", "aggregates");
    export_function("Dyn::Call", "dcAggrField", "aggregates");
    export_function("Dyn::Call", "dcCloseAggr", "aggregates");
    export_function("Dyn::Call", "dcFreeAggr", "aggregates");
    export_function("Dyn::Call", "dcBeginCallAggr", "aggregates");

    export_function("Dyn::Call", "DC_CALL_C_DEFAULT", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ELLIPSIS", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ELLIPSIS_VARARGS", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_CDECL", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_WIN32_STD", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_WIN32_FAST_MS", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_WIN32_FAST_GNU", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_WIN32_THIS_MS", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_WIN32_THIS_GNU", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X64_WIN64", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X64_SYSV", "vars");
    export_function("Dyn::Call", "DC_CALL_C_PPC32_DARWIN", "vars");
    export_function("Dyn::Call", "DC_CALL_C_PPC32_OSX", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ARM_ARM_EABI", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ARM_THUMB_EABI", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ARM_ARMHF", "vars");
    export_function("Dyn::Call", "DC_CALL_C_MIPS32_EABI", "vars");
    export_function("Dyn::Call", "DC_CALL_C_MIPS32_PSPSDK", "vars");
    export_function("Dyn::Call", "DC_CALL_C_PPC32_SYSV", "vars");
    export_function("Dyn::Call", "DC_CALL_C_PPC32_LINUX", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ARM_ARM", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ARM_THUMB", "vars");
    export_function("Dyn::Call", "DC_CALL_C_MIPS32_O32", "vars");
    export_function("Dyn::Call", "DC_CALL_C_MIPS64_N32", "vars");
    export_function("Dyn::Call", "DC_CALL_C_MIPS64_N64", "vars");
    export_function("Dyn::Call", "DC_CALL_C_X86_PLAN9", "vars");
    export_function("Dyn::Call", "DC_CALL_C_SPARC32", "vars");
    export_function("Dyn::Call", "DC_CALL_C_SPARC64", "vars");
    export_function("Dyn::Call", "DC_CALL_C_ARM64", "vars");
    export_function("Dyn::Call", "DC_CALL_C_PPC64", "vars");
    export_function("Dyn::Call", "DC_CALL_C_PPC64_LINUX", "vars");
    export_function("Dyn::Call", "DC_CALL_SYS_DEFAULT", "vars");
    export_function("Dyn::Call", "DC_CALL_SYS_X86_INT80H_LINUX", "vars");
    export_function("Dyn::Call", "DC_CALL_SYS_X86_INT80H_BSD", "vars");
    export_function("Dyn::Call", "DC_CALL_SYS_PPC32", "vars");
    export_function("Dyn::Call", "DC_CALL_SYS_PPC64", "vars");

    export_function("Dyn::Call", "DC_ERROR_NONE", "vars");
    export_function("Dyn::Call", "DC_ERROR_UNSUPPORTED_MODE", "vars");

    export_function("Dyn::Call", "DC_SIGCHAR_VOID", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_BOOL", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CHAR", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_UCHAR", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_SHORT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_USHORT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_INT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_UINT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_LONG", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_ULONG", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_LONGLONG", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_ULONGLONG", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_FLOAT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_DOUBLE", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_POINTER", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_STRING", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_STRUCT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_ENDARG", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_PREFIX", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_DEFAULT", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_ELLIPSIS", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_ELLIPSIS_VARARGS", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_CDECL", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_STDCALL", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_FASTCALL_MS", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_FASTCALL_GNU", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_THISCALL_MS", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_THISCALL_GNU", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_ARM_ARM", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_ARM_THUMB", "vars");
    export_function("Dyn::Call", "DC_SIGCHAR_CC_SYSCALL", "vars");
    export_function("Dyn::Call", "DEFAULT_ALIGNMENT", "vars");
}

INCLUDE: Call/Pointer.xsh