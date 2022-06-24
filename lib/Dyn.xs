#include "lib/xshelper.h"

#include <dynload.h>
#include <dyncall.h>
#include <dyncall_value.h>
#include <dyncall_callf.h>

#include "lib/types.h"

#define META_ATTR "ATTR_SUB"

typedef struct Call {
    DLLib * lib;
    const char * lib_name;
    const char * name;
    const char * sym_name;
    char * sig;
    size_t sig_len;
    char ret;
    DCCallVM * cvm;
    void * fptr;
    char * perl_sig;
} Call;

typedef struct Delayed {
    const char * library;
    const char * library_version;
    const char * package;
    const char * symbol;
    const char * signature;
    const char * name;
    struct Delayed * next;
} Delayed;

Delayed * delayed; // Not thread safe

static
char * clean(char *str) {
    char *end;
    while (isspace(*str) || *str == '"' || *str == '\'') str = str + 1;
    end = str + strlen(str) - 1;
    while (end > str && (isspace(*end) || *end == ')' || *end == '"' || *end == '\'')) end = end - 1;
    *(end+1) = '\0';
    return str;
}

typedef struct xStruct {
    size_t size;
} xStruct;

void push(pTHX_ Call * call, SV * val, char type) {
    switch(type){
        case DC_SIGCHAR_VOID:
            // TODO: Should I pass a NULL here?
            break;
		case DC_SIGCHAR_BOOL:
			dcArgBool(call->cvm, SvTRUE(val));
			break;
		case DC_SIGCHAR_CHAR:
			dcArgChar(call->cvm, (char) SvIV(val));
			break;
		case DC_SIGCHAR_UCHAR:
			dcArgChar(call->cvm, (unsigned char) SvIV(val));
			break;
		case DC_SIGCHAR_SHORT:
			dcArgShort(call->cvm, (short) SvIV(val));
			break;
		case DC_SIGCHAR_USHORT:
			dcArgShort(call->cvm, (unsigned short) SvUV(val));
			break;
		case DC_SIGCHAR_INT:
			dcArgInt(call->cvm, (int) SvIV(val));
			break;
		case DC_SIGCHAR_UINT:
			dcArgInt(call->cvm, (unsigned int) SvUV(val));
			break;
		case DC_SIGCHAR_LONG:
			dcArgLong(call->cvm, (long) SvNV(val));
			break;
		case DC_SIGCHAR_ULONG:
			dcArgLong(call->cvm, (unsigned long) SvNV(val));
			break;
		case DC_SIGCHAR_LONGLONG:
			dcArgLongLong(call->cvm, (long long) SvNV(val));
			break;
		case DC_SIGCHAR_ULONGLONG:
			dcArgLongLong(call->cvm, (unsigned long long) SvNV(val));
			break;
		case DC_SIGCHAR_FLOAT:
			dcArgFloat(call->cvm, (float) SvNV(val));
			break;
		case DC_SIGCHAR_DOUBLE:
			dcArgDouble(call->cvm, (double) SvNV(val));
			break;
		case DC_SIGCHAR_POINTER:
			warn("passing pointer at %s line %d", __FILE__, __LINE__);
			{
				IV tmp = SvIV( (SV*) SvRV( val ) );
				int * arg = INT2PTR(int *, tmp);
				dcArgPointer(call->cvm, arg);
			}
			break;
		case DC_SIGCHAR_STRING:
			dcArgPointer(call->cvm, SvPVutf8_nolen(val));
			break;
		case DC_SIGCHAR_AGGREGATE: /* XXX: dyncall structs/union/array aren't ready yet*/
			warn("passing aggregate");
			DCaggr * ag;
			xStruct  structure;
			memset(&structure, 0, sizeof (xStruct));
			structure.size=0;
			warn("passing aggregate 1");
			ag = dcNewAggr(1, sizeof(xStruct));
			warn("passing aggregate 2");
			dcAggrField(ag, 'i', 0, 0);
			warn("passing aggregate 3");
			dcCloseAggr(ag);
			warn("passing aggregate 4");
			dcArgAggr(call->cvm, ag, &structure /*val*/);
			warn("passing aggregate 5");
			break;
        default:
            warn("Unknown signature character: %c at %s line %d", type, __FILE__, __LINE__);
			break;
        }
}

SV * retval(pTHX_ Call *call) {
    //warn("Here I am! at %s line %d", __FILE__, __LINE__);
	switch (call->ret) {
		case DC_SIGCHAR_VOID:
			dcCallVoid(call->cvm, call->fptr);
            return NULL;
        case DC_SIGCHAR_BOOL:
            return boolSV(dcCallBool(call->cvm, call->fptr));
         case DC_SIGCHAR_CHAR:
            return newSVnv((char) dcCallChar(call->cvm, call->fptr));
        case DC_SIGCHAR_UCHAR:
            return newSVuv((u_char) dcCallChar(call->cvm, call->fptr));
        case DC_SIGCHAR_SHORT:
            return newSViv((short) dcCallShort(call->cvm, call->fptr));
        case DC_SIGCHAR_USHORT:
            return newSVuv((u_short) dcCallShort(call->cvm, call->fptr));
        case DC_SIGCHAR_INT:
            return newSViv(dcCallInt(call->cvm, call->fptr));
        case DC_SIGCHAR_UINT:
            return newSVuv((u_int) dcCallInt(call->cvm, call->fptr));
        case DC_SIGCHAR_LONG:
            return newSViv(dcCallLong(call->cvm, call->fptr));
        case DC_SIGCHAR_ULONG:
            return newSVuv((u_long) dcCallLong(call->cvm, call->fptr));
        case DC_SIGCHAR_LONGLONG:
            return newSViv(dcCallLongLong(call->cvm, call->fptr));
        case DC_SIGCHAR_ULONGLONG:
            return newSVuv( dcCallLongLong(call->cvm, call->fptr));
        case DC_SIGCHAR_FLOAT:
			return newSVnv(dcCallFloat(call->cvm, call->fptr));
		case DC_SIGCHAR_DOUBLE:
            return newSVnv(dcCallDouble(call->cvm, call->fptr));
        case DC_SIGCHAR_POINTER: ; // empty statement before decl. [C89 vs. C99]
            SV * retval;
            retval = sv_newmortal();
            if (0)
                sv_setref_pv(retval, "Dyn::pointer", (void*) dcCallPointer(call->cvm, call->fptr));
            else
                sv_setpv(retval, (const char *) dcCallPointer(call->cvm, call->fptr));
            return retval;
        case DC_SIGCHAR_STRING:
            return newSVpv(dcCallPointer(call->cvm, call->fptr), 0);
        case DC_SIGCHAR_AGGREGATE: /* TODO: dyncall structs/union/array aren't ready upstream yet*/
            warn("dyncall aggregate types (structs/union/array) aren't ready upstream yet at %s line %d", __FILE__, __LINE__);
            break;
		default:
			warn("Unknown return character: %c at %s line %d", call->ret, __FILE__, __LINE__);
	};
	return NULL;
}

// TODO: This might need to return values in arg pointers
#define _call_ \
    if (call != NULL) {\
        dcReset(call->cvm);\
        const char * sig_ptr = call->sig;\
        int sig_len = call->sig_len;\
        int pos = 0;\
        char ch;\
        for (ch = *sig_ptr; pos < /*sig_len;*/ items; ch = *++sig_ptr) {\
			push(aTHX_ (Call *)call, ST(pos), ch);\
            ++pos;\
        }\
        /*warn("ret == %c", call->ret);*/\
		SV * ret = retval(aTHX_ call);\
        if (ret !=NULL ) {\
            ST(0) = ret;\
            XSRETURN(1);\
        }\
        else\
            XSRETURN_EMPTY;\
    /*//warn("here at %s line %d", __FILE__, __LINE__);*/\
    }\
    else\
        croak("Function is not attached! This is a serious bug!");\
    /*//warn("here at %s line %d", __FILE__, __LINE__);*/

static Call *
_load(pTHX_ DLLib * lib, const char * symbol, const char * sig) {
    if(lib == NULL)
        return NULL;
    //warn("_load(..., %s, %s)", symbol, sig);
    Call * RETVAL;
    Newx(RETVAL, 1, Call);
    RETVAL->lib = lib;
    RETVAL->cvm = dcNewCallVM(1024);
    //warn("100");
    if(RETVAL->cvm == NULL)
        return NULL;
    //warn("101");
    RETVAL->fptr = dlFindSymbol(RETVAL->lib, symbol );
    if (RETVAL->fptr == NULL) // TODO: throw warning
        return NULL;
    //warn("102");
    Newxz( RETVAL->sig, strlen(sig), char ); // Dumb
    //RETVAL->sig = sig;
    RETVAL->sig_len = strlen(RETVAL->sig);
    Newxz( RETVAL->perl_sig, strlen(sig), char ); // Dumb
    int i, sig_pos;
    sig_pos = 0;
    //warn("103");
    for (i = 0; sig[i + 1] != '\0'; ++i ) {
        switch (sig[i]) {
            case DC_SIGCHAR_CC_PREFIX:
                ++i;
                switch(sig[i]) {
                    case DC_SIGCHAR_CC_DEFAULT:
                        dcMode(RETVAL->cvm, DC_CALL_C_DEFAULT);
						break;
					case DC_SIGCHAR_CC_THISCALL:
						dcMode(RETVAL->cvm, DC_CALL_C_DEFAULT_THIS);
						break;
                    case DC_SIGCHAR_CC_ELLIPSIS:
                        dcMode(RETVAL->cvm, DC_CALL_C_ELLIPSIS);
						break;
                    case DC_SIGCHAR_CC_ELLIPSIS_VARARGS:
                        dcMode(RETVAL->cvm, DC_CALL_C_ELLIPSIS_VARARGS);
						break;
                    case DC_SIGCHAR_CC_CDECL:
                        dcMode(RETVAL->cvm, DC_CALL_C_X86_CDECL);
						break;
                    case DC_SIGCHAR_CC_STDCALL:
                        dcMode(RETVAL->cvm, DC_CALL_C_X86_WIN32_STD);
						break;
                    case DC_SIGCHAR_CC_FASTCALL_MS:
                        dcMode(RETVAL->cvm, DC_CALL_C_X86_WIN32_FAST_MS);
						break;
                    case DC_SIGCHAR_CC_FASTCALL_GNU:
                        dcMode(RETVAL->cvm, DC_CALL_C_X86_WIN32_FAST_GNU);
						break;
                    case DC_SIGCHAR_CC_THISCALL_MS:
                        dcMode(RETVAL->cvm, DC_CALL_C_X86_WIN32_THIS_MS);
						break;
                    case DC_SIGCHAR_CC_THISCALL_GNU:
                        dcMode(RETVAL->cvm, DC_CALL_C_X86_WIN32_FAST_GNU);
						break;
                    case DC_SIGCHAR_CC_ARM_ARM:
                        dcMode(RETVAL->cvm, DC_CALL_C_ARM_ARM);
						break;
                    case DC_SIGCHAR_CC_ARM_THUMB:
                        dcMode(RETVAL->cvm, DC_CALL_C_ARM_THUMB);
						break;
                    case DC_SIGCHAR_CC_SYSCALL:
                        dcMode(RETVAL->cvm, DC_CALL_SYS_DEFAULT);
						break;
                    default:
						warn("Unknown signature character: %c at %s line %d", sig[i], __FILE__, __LINE__);
                        break;
                };
                break;
            case DC_SIGCHAR_VOID:
            case DC_SIGCHAR_BOOL:
            case DC_SIGCHAR_CHAR:
            case DC_SIGCHAR_UCHAR:
            case DC_SIGCHAR_SHORT:
            case DC_SIGCHAR_USHORT:
            case DC_SIGCHAR_INT:
            case DC_SIGCHAR_UINT:
            case DC_SIGCHAR_LONG:
            case DC_SIGCHAR_ULONG:
            case DC_SIGCHAR_LONGLONG:
            case DC_SIGCHAR_ULONGLONG:
            case DC_SIGCHAR_FLOAT:
            case DC_SIGCHAR_DOUBLE:
            case DC_SIGCHAR_POINTER:
            case DC_SIGCHAR_STRING:
            case DC_SIGCHAR_AGGREGATE:
                RETVAL->perl_sig[sig_pos] = '$';
                RETVAL->sig[sig_pos]      = sig[i];
                ++sig_pos;
                break;
            case DC_SIGCHAR_ENDARG:
                RETVAL->ret = sig[i + 1];
                break;
			case '(': // Start of signature
				break;
            default:
				warn("Unknown signature character: %c at %s line %d", sig[i], __FILE__, __LINE__);
                break;
        };
    }
    //warn("104");
    //warn("Now: %s|%s|%c", RETVAL->perl_sig, RETVAL->sig, RETVAL->ret);
    return RETVAL;
}

MODULE = Dyn PACKAGE = Dyn

void
DESTROY(...)
CODE:
    Call * call;
    call = (Call *) XSANY.any_ptr;
    if (call == NULL)      return;
    if (call->lib != NULL) dlFreeLibrary( call->lib );
    if (call->cvm != NULL) dcFree(call->cvm);
    if (call->sig != NULL)       Safefree( call->sig );
    if (call->perl_sig != NULL ) Safefree( call->perl_sig );
    Safefree(call);

void
call_Dyn(...)
CODE:
    Call * call;
    call = (Call *) XSANY.any_ptr;
    _call_

SV *
wrap(lib, const char * func_name, const char * sig, ...)
CODE:
    DLLib * lib;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::DLLib")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        lib = dlLoadLibrary( (const char *) SvPV_nolen(ST(0)) );

    Call * call = _load(aTHX_ lib, func_name, sig);
    CV * cv;
    STMT_START {
        cv = newXSproto_portable(NULL, XS_Dyn_call_Dyn, (char*)__FILE__, call->perl_sig);
        if (cv == NULL)
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (void *) call;
    } STMT_END;
    RETVAL = sv_bless(newRV_inc((SV*) cv), gv_stashpv((char *) "Dyn", 1));
OUTPUT:
    RETVAL

void
call_attach(Call * call, ...)
PPCODE:
    ////warn("call_attach( ... )");
    _call_

SV *
attach(lib, const char * symbol_name, const char * sig, const char * func_name = NULL)
PREINIT:
    Call * call;
    DLLib * lib;
    //  $lib_file, 'add_i', '(ii)i' , '__add_i'
CODE:
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::DLLib")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        lib = dlLoadLibrary( (const char *)SvPV_nolen(ST(0)) );

    ////warn("ix == %d | items == %d", ix, items);
    if (func_name == NULL)
        func_name = symbol_name;

    call = _load(aTHX_ lib, symbol_name, sig);
    if (call == NULL)
        croak("Failed to attach %s", symbol_name);
    /* Create a new XSUB instance at runtime and set it's XSANY.any_ptr to contain the
     * necessary user data. name can be NULL => fully anonymous sub!
    **/
    CV * cv;
    STMT_START {
        cv = newXSproto_portable(func_name, XS_Dyn_call_Dyn, (char*)__FILE__, call->perl_sig);
        ////warn("N");
        if (cv == NULL)
            croak("ARG! Something went really wrong while installing a new XSUB!");
        ////warn("Q");
        XSANY.any_ptr = (void *) call;
    } STMT_END;
    RETVAL = newRV_inc((SV*) cv);
OUTPUT:
    RETVAL

void
__install_sub( char * package, char * library, char * library_version, char * signature, char * symbol, char * full_name )
PREINIT:
    Delayed * _now;
    Newx(_now, 1, Delayed);
CODE:
    Newx(_now->package, strlen(package) +1, char);
    memcpy((void *) _now->package, package, strlen(package)+1);
    Newx(_now->library, strlen(library) +1, char);
    memcpy((void *) _now->library, library, strlen(library)+1);
    Newx(_now->library_version, strlen(library_version) +1, char);
    memcpy((void *) _now->library_version, library_version, strlen(library_version)+1);
    Newx(_now->signature, strlen(signature)+1, char);
    memcpy((void *) _now->signature, signature, strlen(signature)+1);
    Newx(_now->symbol, strlen(symbol) +1, char);
    memcpy((void *) _now->symbol, symbol, strlen(symbol)+1);
    Newx(_now->name, strlen(full_name)+1, char);
    memcpy((void *) _now->name, full_name, strlen(full_name)+1);
    _now->next = delayed;
    delayed = _now;

void
END( ... )
PPCODE:
    Delayed * holding;
    while (delayed != NULL) {
        //warn ("killing %s...", delayed->name);
        Safefree(delayed->package);
        Safefree(delayed->library);
        Safefree(delayed->library_version);
        Safefree(delayed->signature);
        Safefree(delayed->symbol);
        Safefree(delayed->name);
        holding = delayed->next;
        Safefree(delayed);
        delayed = holding;
    }

void
AUTOLOAD( ... )
PPCODE:
    char* autoload = SvPV_nolen( sv_mortalcopy( get_sv( "Dyn::AUTOLOAD", TRUE ) ) );
    ////warn("$AUTOLOAD? %s", autoload);
    {   Delayed * _prev = delayed;
        Delayed * _now  = delayed;
        while (_now != NULL) {
            if (strcmp(_now->name, autoload) == 0) {
                //warn(" signature: %s", _now->signature);
                //warn(" name:      %s", _now->name);
                //warn(" symbol:    %s", _now->symbol);
                //warn(" library:   %s", _now->library);
                //if (_now->library_version != NULL)
                //    warn (" version:  %s", _now->library_version);
                //warn(" package:   %s", _now->package);
                SV * lib;
                //if (strstr(_now->library, "{")) {
                    char eval[1024]; // idk
                    sprintf(eval, "package %s{sub{sub{Dyn::guess_library_name(%s,%s)}}->()->();};",
                        _now->package, _now->library,
                        _now->library_version
                    );
                    //warn("eval: %s", eval);
                    lib = eval_pv( eval, FALSE ); // TODO: Cache this?
                //}
                //else
                //    lib = newSVpv(_now->library, strlen(_now->library));
                //SV * lib = get_sv(_now->library, TRUE);
                //warn("     => %s", (const char *) SvPV_nolen(lib));
                char *sig, ret, met;
				const char * lib_name = SvPV_nolen(lib);
                DLLib * _lib = dlLoadLibrary( lib_name );
                if (_lib == NULL) {
#if defined(_WIN32) || defined(__WIN32__)
				unsigned int err = GetLastError();
				warn ("GetLastError() == %d", err);
#endif
                    croak("Failed to load %s", lib_name);
				}
                Call * call = _load(aTHX_ _lib, _now->symbol, _now->signature );

                //warn("Z");
                if (call != NULL) {
                    CV * cv;
                    //warn("Y");
                    STMT_START {
                       // //warn("M");
                        cv = newXSproto_portable(autoload, XS_Dyn_call_Dyn, (char*)__FILE__, call->perl_sig);
                        ////warn("N");
                        if (cv == NULL)
                            croak("ARG! Something went really wrong while installing a new XSUB!");
                        ////warn("Q");
                        XSANY.any_ptr = (void *) call;
                        ////warn("O");
                    } STMT_END;
                    ////warn("P");
                    ////warn("AUTOLOAD( ... )");
                    _call_
                    _prev->next = _now->next;

                    Safefree(_now->library);
                    Safefree(_now->package);
                    Safefree(_now->signature);
                    Safefree(_now->symbol);
                    Safefree(_now->name);
                    Safefree(_now);
                }
                else
                    croak("Oops!");
                ////warn("A");
                //if (_prev = NULL)
                //    _prev = _now;
                return;
             }
            _prev = _now;
            _now  = _now->next;
        }
    }
    die("Undefined subroutine &%s", autoload);

BOOT:
    delayed = NULL;
