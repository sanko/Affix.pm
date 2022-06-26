#include "lib/clutter.h"

/* Global struct registry */
#define MY_CXT_KEY "Dyn::Type::Struct::_guts" XS_VERSION
typedef struct {
    HV * structs;
} my_cxt_t;

START_MY_CXT

#if PERL_VERSION_LE(5, 8, 999) /* PERL_VERSION_LT is 5.33+ */
    char* file = __FILE__;
#else
    const char* file = __FILE__;
#endif

void unroll_aggregate(void *ptr, DCaggr *ag, SV *obj) {
    // warn(".a == %c", struct_rep.a);
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

            warn(".a == %c", (unsigned char)b);
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

void *
dcAllocMem(size_t size)
CODE:
    dcAllocMem(RETVAL, size, void *);
OUTPUT:
    RETVAL

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
    else if (sv_derived_from(ST(1), "Dyn::pointer")){
        IV tmp = SvIV((SV*)SvRV(ST(1)));
        DCpointer arg = INT2PTR(DCpointer, tmp);
        dcArgPointer(vm, arg);
    }
    else
        croak("arg is not of type Dyn::pointer");

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
dcCallAggr(DCCallVM * vm, DCpointer funcptr, DCaggr * ag, SV * obj = NULL);
CODE:
     //aggr_ptr struct_rep;
    void * struct_rep;
    struct_rep = malloc(sizeof(&ag));
    warn ("sizeof(&ag) == %d", sizeof(&ag));
    RETVAL = dcCallAggr(vm, funcptr, ag, &struct_rep);
    if ((obj != NULL) && sv_isobject(obj) && sv_derived_from(obj, "Dyn::Type::Struct"))
      unroll_aggregate(struct_rep, ag, obj);
OUTPUT:
    RETVAL

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
dcAggrField( DCaggr * ag, DCsigchar type, DCint offset, DCsize arrayLength, ... )

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


MODULE = Dyn::Call   PACKAGE = Dyn::Call::Field

DCsize
_field(DCfield * thing)
ALIAS:
    offset    = 1
    size      = 2
    alignment = 3
    array_len = 4
CODE:
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
type(DCfield * thing)
CODE:
    RETVAL = thing->type;
OUTPUT:
    RETVAL

const DCaggr *
sub_aggr(DCfield * thing)
CODE:
    RETVAL = thing->sub_aggr;
OUTPUT:
    RETVAL

MODULE = Dyn::Call   PACKAGE = Dyn::Call::Aggr

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
    AV * avfields = newAV();
    AV * avtypes  = newAV();
    //AV * modes  = newAV(); // Currently unused

    char * new_ = malloc(strlen(pkg) + 1);
    strcpy(new_, pkg);
		strcat(new_, "::new");
    (void)newXSproto_portable(new_, XS_Dyn__Type__Struct_new, file, "$%");

    {
      char * blah = malloc(strlen(pkg)+1);
                    strcpy(blah, pkg);
                    strcat(blah, "::ISA");
    AV *isa = perl_get_av(blah,1);
    av_push(isa, newSVpv("Dyn::Type::Struct", strlen("Dyn::Type::Struct")) );
    }
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

                    char * blah = malloc(strlen(pkg)+1);
                    strcpy(blah, pkg);
                    strcat(blah, "::");
                    strcat(blah, SvPV_nolen(sv_field));
                    cv = newXSproto_portable(blah, XS_Dyn__Type__Struct_get, file, "$");
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
    newCONSTSUB(stash, "DC_SIGCHAR_VOID", newSViv(DC_SIGCHAR_VOID));
    newCONSTSUB(stash, "DC_SIGCHAR_BOOL", newSViv(DC_SIGCHAR_BOOL));
    newCONSTSUB(stash, "DC_SIGCHAR_CHAR", newSViv(DC_SIGCHAR_CHAR));
    newCONSTSUB(stash, "DC_SIGCHAR_UCHAR", newSViv(DC_SIGCHAR_UCHAR));
    newCONSTSUB(stash, "DC_SIGCHAR_SHORT", newSViv(DC_SIGCHAR_SHORT));
    newCONSTSUB(stash, "DC_SIGCHAR_USHORT", newSViv(DC_SIGCHAR_USHORT));
    newCONSTSUB(stash, "DC_SIGCHAR_INT", newSViv(DC_SIGCHAR_INT));
    newCONSTSUB(stash, "DC_SIGCHAR_UINT", newSViv(DC_SIGCHAR_UINT));
    newCONSTSUB(stash, "DC_SIGCHAR_LONG", newSViv(DC_SIGCHAR_LONG));
    newCONSTSUB(stash, "DC_SIGCHAR_ULONG", newSViv(DC_SIGCHAR_ULONG));
    newCONSTSUB(stash, "DC_SIGCHAR_LONGLONG", newSViv(DC_SIGCHAR_LONGLONG));
    newCONSTSUB(stash, "DC_SIGCHAR_ULONGLONG", newSViv(DC_SIGCHAR_ULONGLONG));
    newCONSTSUB(stash, "DC_SIGCHAR_FLOAT", newSViv(DC_SIGCHAR_FLOAT));
    newCONSTSUB(stash, "DC_SIGCHAR_DOUBLE", newSViv(DC_SIGCHAR_DOUBLE));
    newCONSTSUB(stash, "DC_SIGCHAR_POINTER", newSViv(DC_SIGCHAR_POINTER));
    newCONSTSUB(stash, "DC_SIGCHAR_STRING", newSViv(DC_SIGCHAR_STRING));/* in theory same as 'p', but convenient to disambiguate */
    newCONSTSUB(stash, "DC_SIGCHAR_AGGREGATE", newSViv(DC_SIGCHAR_AGGREGATE));
    newCONSTSUB(stash, "DC_SIGCHAR_ENDARG", newSViv(DC_SIGCHAR_ENDARG));/* also works for end struct */

    /* calling convention / mode signatures */
    newCONSTSUB(stash, "DC_SIGCHAR_CC_PREFIX", newSViv(DC_SIGCHAR_CC_PREFIX));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_DEFAULT", newSViv(DC_SIGCHAR_CC_DEFAULT));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ELLIPSIS", newSViv(DC_SIGCHAR_CC_ELLIPSIS));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ELLIPSIS_VARARGS", newSViv(DC_SIGCHAR_CC_ELLIPSIS_VARARGS));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_CDECL", newSViv(DC_SIGCHAR_CC_CDECL));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_STDCALL", newSViv(DC_SIGCHAR_CC_STDCALL));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_FASTCALL_MS", newSViv(DC_SIGCHAR_CC_FASTCALL_MS));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_FASTCALL_GNU", newSViv(DC_SIGCHAR_CC_FASTCALL_GNU));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_THISCALL_MS", newSViv(DC_SIGCHAR_CC_THISCALL_MS));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_THISCALL_GNU", newSViv(DC_SIGCHAR_CC_THISCALL_GNU));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ARM_ARM", newSViv(DC_SIGCHAR_CC_ARM_ARM));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_ARM_THUMB", newSViv(DC_SIGCHAR_CC_ARM_THUMB));
    newCONSTSUB(stash, "DC_SIGCHAR_CC_SYSCALL", newSViv(DC_SIGCHAR_CC_SYSCALL));

    // Error codes
    newCONSTSUB(stash, "DC_ERROR_NONE", newSViv(DC_ERROR_NONE));
    newCONSTSUB(stash, "DC_ERROR_UNSUPPORTED_MODE", newSViv(DC_ERROR_UNSUPPORTED_MODE));
}

