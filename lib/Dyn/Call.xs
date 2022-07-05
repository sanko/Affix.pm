#include "lib/clutter.h"

/* Global struct registry */
#define MY_CXT_KEY "Dyn::Type::Struct::_guts" XS_VERSION
typedef struct {
    HV * structs;
} my_cxt_t;

START_MY_CXT

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

MODULE = Dyn::Call PACKAGE = Dynamo::Types::Struct

void
new(const char * package, HV * args = newHV_mortal())
PPCODE:
    if(!strcmp(package, "Dynamo::Types::Struct"))
        croak("You must subclass Dynamo::Types::Struct into your own type");
    AV * RETVAL_SV = newAV_mortal();

    av_push(RETVAL_SV, newSViv(16));  // alignment
    //av_push(RETVAL_SV, newAV_mortal()); // values
    // TODO: store layout in thread safe package var

    SV * RETVAL = sv_bless(newRV_inc((SV*) RETVAL_SV), gv_stashpv(package, 1));
    SvREADONLY_on((SV*) RETVAL_SV); // Don't allow more elements to be added
    RETVAL = sv_2mortal(RETVAL);
    ST(0) = RETVAL;
    XSRETURN(1);

MODULE = Dyn::Call PACKAGE = Dynamo::Types::_Scalar

void
new(const char * package, HV * args = newHV_mortal())
PPCODE:
    if(!strcmp(package, "Dynamo::Types::_Scalar"))
        croak("You must subclass Dynamo::Types::_Scalar into your own type");
    AV * RETVAL_SV = newAV_mortal();

    // args{alignment}
    av_push(RETVAL_SV, newSViv(5));                 // type
    av_push(RETVAL_SV, newSViv(MEM_ALIGNBYTES));    // alignment
    av_push(RETVAL_SV, &PL_sv_undef);               // value?

    SV * RETVAL = sv_bless(newRV_inc((SV*) RETVAL_SV), gv_stashpv(package, 1));
    SvREADONLY_on((SV*) RETVAL_SV); // Don't allow more elements to be added
    RETVAL = sv_2mortal(RETVAL);
    ST(0) = RETVAL;
    XSRETURN(1);

size_t
_alignment(AV * s)
ALIAS:
    Dynamo::Types::_Scalar::alignment  = 0
    Dynamo::Types::Pointer::alignment  = 4
    Dynamo::Types::Char::alignment      = 1
    Dynamo::Types::Int::alignment      = 4
    Dynamo::Types::Short::alignment    = 2
    Dynamo::Types::I8::alignment       = 1
    Dynamo::Types::U8::alignment       = 1
    Dynamo::Types::I16::alignment      = 4
    Dynamo::Types::U16::alignment      = 4
    Dynamo::Types::I32::alignment      = 8
    Dynamo::Types::U32::alignment      = 8
    Dynamo::Types::I64::alignment      = 12
    Dynamo::Types::U64::alignment      = 12
    Dynamo::Types::Long::alignment     = 8
    Dynamo::Types::LongLong::alignment = 4
    Dynamo::Types::Float::alignment    = 4
    Dynamo::Types::Double::alignment   = 8
    Dynamo::Types::LongDouble::alignment   = 12
CODE:
    // https://en.wikipedia.org/wiki/Data_structure_alignment
    switch(ix) {
        case 0:  croak("You must define alignment() for your own type"); break;
        default: RETVAL = ix; break;
    }
OUTPUT:
    RETVAL

size_t
_sizeof(AV * s)
ALIAS:
    Dynamo::Types::_Scalar::sizeof  = 0
    Dynamo::Types::Pointer::sizeof  = PTRSIZE
    Dynamo::Types::Int::sizeof      = INTSIZE
    Dynamo::Types::Short::sizeof    = SHORTSIZE
    Dynamo::Types::I8::sizeof       = I8SIZE
    Dynamo::Types::U8::sizeof       = U8SIZE
    Dynamo::Types::I16::sizeof      = I16SIZE
    Dynamo::Types::U16::sizeof      = U16SIZE
    Dynamo::Types::I32::sizeof      = I32SIZE
    Dynamo::Types::U32::sizeof      = U32SIZE
    Dynamo::Types::I64::sizeof      = I64SIZE
    Dynamo::Types::U64::sizeof      = U64SIZE
    Dynamo::Types::Long::sizeof     = LONGSIZE
    Dynamo::Types::LongLong::sizeof = LONGLONGSIZE
    Dynamo::Types::Float::sizeof    = FLOATSIZE
    Dynamo::Types::Double::sizeof   = DOUBLESIZE
    Dynamo::Types::LongDouble::sizeof   = LONG_DOUBLESIZE
CODE:
    switch(ix) {
        case 0:  croak("You must define sizeof() for your own type"); break;
        default: RETVAL = ix; break;
    }
OUTPUT:
    RETVAL

char
_signature(AV * s)
ALIAS:
    Dynamo::Types::_Scalar::signature  = 0
    Dynamo::Types::Pointer::signature  = DC_SIGCHAR_POINTER
    Dynamo::Types::Int::signature      = DC_SIGCHAR_INT
    Dynamo::Types::Short::signature    = DC_SIGCHAR_SHORT
    Dynamo::Types::UShort::signature   = DC_SIGCHAR_USHORT
    Dynamo::Types::I8::signature       = 1
    Dynamo::Types::U8::signature       = 10
    Dynamo::Types::I16::signature      = 40
    Dynamo::Types::U16::signature      = 409
    Dynamo::Types::I32::signature      = 87
    Dynamo::Types::U32::signature      = 89
    Dynamo::Types::I64::signature      = 126
    Dynamo::Types::U64::signature      = 120
    Dynamo::Types::Long::signature     = DC_SIGCHAR_LONG
    Dynamo::Types::LongLong::signature = DC_SIGCHAR_LONGLONG
    Dynamo::Types::Float::signature    = DC_SIGCHAR_FLOAT
    Dynamo::Types::Double::signature   = DC_SIGCHAR_DOUBLE
    Dynamo::Types::LongDouble::signature   = 1278
CODE:
    // https://dyncall.org/pub/dyncall/dyncall/file/tip/dyncall/dyncall_signature.h
    //warn("SIGNATURE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %c", ix);
    switch(ix) {
        case 0:  croak("You must define signature() for your own type"); break;
        default: RETVAL = ix; break;
    }
OUTPUT:
    RETVAL

bool
_check(AV * s, const char * value)
ALIAS:
    Dynamo::Types::_Scalar::check  = 0
    Dynamo::Types::Pointer::check  = 990

    Dynamo::Types::Char::check  = 5
    Dynamo::Types::UChar::check  = 6

    Dynamo::Types::Int::check      = 10
    Dynamo::Types::Short::check    = 20

    Dynamo::Types::I8::check       = 30
    Dynamo::Types::I16::check      = 40
    Dynamo::Types::I32::check      = 50
    Dynamo::Types::I64::check      = 60

    Dynamo::Types::U8::check       = 70
    Dynamo::Types::U16::check      = 80
    Dynamo::Types::U32::check      = 90
    Dynamo::Types::U64::check      = 100

    Dynamo::Types::Long::check     = 110
    Dynamo::Types::LongLong::check = 120
    Dynamo::Types::Float::check    = 130
    Dynamo::Types::Double::check   = 140
    Dynamo::Types::LongDouble::check   = 150
CODE:
    bool float_flag = false, signed_flag = false, numeric_flag = true;
    {
        int length = strlen(value);
        for(int i = 0; i < length; i++) {
            if(!isDIGIT(value[i])) {
                if(value[i] == '.')
                    float_flag = true;
                else if(value[i] == '-')
                    signed_flag = true;
                else {
                    numeric_flag = false;
                    break;
                }
            }
        }
    }
    switch(ix) {
        case 0:  croak("You must define check() for your own type"); break;
        //Dynamo::Types::Pointer::sizeof  = PTRSIZE
        // String: SvPOK
        case 5://Char
            RETVAL =
                numeric_flag && (!float_flag) &&
                SvIV(ST(1)) <= CHAR_MAX &&
                SvIV(ST(1)) >= CHAR_MAX ?
                true : false;
            break;
        case 6://UChar
            RETVAL =
                numeric_flag && (!float_flag) &&
                SvIV(ST(1)) <= PERL_UCHAR_MAX &&
                SvIV(ST(1)) >= PERL_UCHAR_MIN ?
                true : false;
            break;
        case 10://Int
            RETVAL =
                numeric_flag && (!float_flag) &&
                SvIV(ST(1)) <= PERL_INT_MAX &&
                SvIV(ST(1)) >= PERL_INT_MIN ?
                true : false;
            break;
        case 20://Short
            RETVAL = numeric_flag && (!float_flag)&&
                SvIV(ST(1)) <= PERL_SHORT_MAX &&
                SvIV(ST(1)) >= PERL_SHORT_MIN ?
                true : false; break;
        case 30:
            break;

        case 40:
            break;

        case 50:
            break;
        case 60:
            RETVAL = SvIOK(ST(1)) ? true : false;
             break;
        case 70:
                    break;

        case 80:
                    break;

        case 90:
                    break;

        case 100:
            RETVAL = SvIOK_UV(ST(1)) ?
            true : false;
             break;
        case 110: //Long
            RETVAL = numeric_flag && (!float_flag)&&
                SvIV(ST(1)) <= PERL_LONG_MAX &&
                SvIV(ST(1)) >= PERL_LONG_MIN ?
                true : false; break;
                break;
        case 120:
                    break;

        case 130:
            RETVAL =
                numeric_flag ?
                true : false;
            break;
        case 140:
                    break;

        case 150:
            RETVAL = SvNOK(ST(1)) ? true : false; break;
        default:  croak("You must define check() for your own type");break;
    }
OUTPUT:
    RETVAL

BOOT:
    set_isa("Dynamo::Types::Char",  "Dynamo::Types::_Scalar");
    set_isa("Dynamo::Types::UChar", "Dynamo::Types::Char");
    set_isa("Dynamo::Types::Int",   "Dynamo::Types::_Scalar");
    set_isa("Dynamo::Types::Float", "Dynamo::Types::_Scalar");
    set_isa("Dynamo::Types::Double", "Dynamo::Types::Float");

MODULE = Dyn::Call PACKAGE = Dynamo

SV *
_Types()
ALIAS:
    Dynamo::Int = 10
    Dynamo::Char = 20
    Dynamo::UChar = 21
    Dynamo::Float = 50
    Dynamo::Double = 51
CODE:
    switch(ix) {
        case 10:
        case 20:
        case 21:
        case 50:
        croak("TODO???");break;
        case 51:{
                    croak("double");
        } break;
        default:
        croak("TODO...");break;
    }
    RETVAL = newSViv(100);
OUTPUT:
    RETVAL

BOOT:
    export_function("Dynamo::Int", "types");
    export_function("Dynamo::Double", "types");

void
Struct(...)
PPCODE:
    const char * package = "Dyn::Types::Struct";
    int pos = 0;
    if (items == 2) {
        pos = 1;
        package = (const char *)SvPV_nolen(ST(0));
    }
    warn ("Items == %d; pos == %d", items, pos);

    AV *    fields;

    STMT_START {
        SV* const xsub_tmp_sv = ST(pos);
        SvGETMAGIC(xsub_tmp_sv);

        if (SvROK(xsub_tmp_sv) &&
            SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV
            &&
            !(av_count((AV*)SvRV(xsub_tmp_sv)) % 2)
            ) {
            fields = (AV*)SvRV(xsub_tmp_sv);
        }
        else{
            croak("Struct[...]: members is not an even sized list");
        }
    } STMT_END;
        // const char * package, AV * in

    if(!strcmp(package, "Dynamo::Types::Struct"))
        croak("You must subclass Dynamo::Types::Struct into your own type");


    {
    AV * avfields = newAV();
    AV * avtypes  = newAV();
    //AV * modes  = newAV(); // Currently unused

    char * new_ = malloc(strlen(package) + 1);
    strcpy(new_, package);
        strcat(new_, "::new");
    (void)newXSproto_portable(new_, XS_Dyn__Type__Struct_new, file, "$%");

    {
      char * blah = malloc(strlen(package)+1);
                    strcpy(blah, package);
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

                    char * blah = malloc(strlen(package)+1);
                    strcpy(blah, package);
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
    hv_store(MY_CXT.structs, package,  strlen(package), newRV_inc((SV*)classinfo), 0);
    SV * RETVAL;
    RETVAL=MY_CXT.structs;
    }
    /*
    AV * RETVAL_SV = newAV_mortal();


    //in = (AV *)sv_2mortal((SV *)in);

    //av_push(RETVAL_SV, in);

    SV * RETVAL = sv_bless(newRV_inc((SV*) RETVAL_SV), gv_stashpv(package, 1));
    SvREADONLY_on((SV*) RETVAL_SV); // Don't allow more elements to be added


    RETVAL = sv_2mortal(RETVAL);
    ST(0) = RETVAL;
    XSRETURN(1);


    */


BOOT:
    (void)newXSproto_portable("Dynamo::Struct", XS_Dynamo__Types__Struct_new, file, "$;$");
    export_function("Dynamo::Struct", "types");
