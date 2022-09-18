#include "lib/clutter.h"

MODULE = Dyn::Callback PACKAGE = Dyn::Callback

BOOT:
#ifdef USE_ITHREADS
    my_perl = (PerlInterpreter *) PERL_GET_CONTEXT;
#endif

DCCallback *
dcbNewCallback(const char * signature, SV * funcptr, ...);
PREINIT:
    dTHX;
    _callback * container;
#ifdef USE_ITHREADS
    PERL_SET_CONTEXT(my_perl);
#endif
CODE:
    container = (_callback *) safemalloc(sizeof(_callback));
    if (!container) // OOM
        XSRETURN_UNDEF;
    container->cvm = dcNewCallVM(1024);
    dcMode(container->cvm, 0); // TODO: Use correct value according to signature
    dcReset(container->cvm);
    container->signature = signature;
    container->cb = SvREFCNT_inc(funcptr);
    container->userdata = items > 2 ? newRV_inc(ST(2)): &PL_sv_undef;
    int i;
    for (i = 0; container->signature[i+1] != '\0'; ++i ) {
        //warn("here at %s line %d.", __FILE__, __LINE__);
        if (container->signature[i] == ')') {
            container->ret_type = container->signature[i+1];
            break;
        }
    }
    //warn("signature: %s at %s line %d.", signature, __FILE__, __LINE__);
    RETVAL = dcbNewCallback(signature, callback_handler, (void *) container);
OUTPUT:
    RETVAL

void
dcbInitCallback(DCCallback * pcb, const char * signature, DCCallbackHandler * funcptr, ...);
PREINIT:
    dTHX;
    _callback * container;
#ifdef USE_ITHREADS
    PERL_SET_CONTEXT(my_perl);
#endif
CODE:
    container = (_callback*) dcbGetUserData(pcb);
    container->signature = signature;
    container->cb = SvREFCNT_inc((SV *) funcptr);
    container->userdata = items > 3 ? newRV_inc(ST(3)): &PL_sv_undef;
    int i;
    for (i = 0; container->signature[i+1] != '\0'; ++i ) {
        //warn("here at %s line %d.", __FILE__, __LINE__);
        if (container->signature[i] == ')') {
            container->ret_type = container->signature[i+1];
            break;
        }
    }
    dcbInitCallback(pcb, signature, callback_handler, (void *) container);

void
dcbFreeCallback(DCCallback * pcb);
PREINIT:
    dTHX;
#ifdef USE_ITHREADS
    PERL_SET_CONTEXT(my_perl);
#endif
CODE:
    _callback * container = ((_callback*) dcbGetUserData(pcb));
    dcFree(container->cvm);
    dcbFreeCallback( pcb );
    // TODO: Free SVs

SV *
dcbGetUserData(DCCallback * pcb);
PREINIT:
    dTHX;
#ifdef USE_ITHREADS
    PERL_SET_CONTEXT(my_perl);
#endif
INIT:
    RETVAL = (SV*) &PL_sv_undef;
CODE:
    _callback * container = ((_callback*) dcbGetUserData(pcb));
    if (SvOK(container->userdata))
        RETVAL = //SvRV(container->userdata);
            SvREFCNT_inc(SvRV(container->userdata));
OUTPUT:
    RETVAL

=pod

Less Perl, more C.

=cut

SV *
call(DCCallback * self, ... )
PREINIT:
    dTHX;
#ifdef USE_ITHREADS
    PERL_SET_CONTEXT(my_perl);
#endif
    //AV * args = newAV();
CODE:
    RETVAL = newSV(0);
    _callback * container = ((_callback*) dcbGetUserData(self));
    const char * signature = container->signature;
    //warn("Callback sig: %s", signature);
    int done = 0;
    int i;
    dcReset(container->cvm); // Get it ready to call again
    int tally = 0;
    //warn("here at %s line %d.", __FILE__, __LINE__);
    for (i = 1; signature[i] != '\0'; ++i) {
        //warn ("i: %d vs items: %d", i, items);
        //if (i > items - 1) // TODO: Don't do this is signature is var_list, etc.
        //    croak("Incorrect number of arguments for callback. Expected %d but were handed %d.", i, items - 1);
        tally++;
        //warn("Checking char: %c", signature[i - 1]);
        //warn("DC_SIGCHAR_CHAR == %c", DC_SIGCHAR_INT);
        switch(signature[i - 1]) {
            case DC_SIGCHAR_VOID:
                //dcArgVoid(container->cvm, self);
                tally--;
                break;
            case DC_SIGCHAR_BOOL:
                dcArgBool(container->cvm, (bool)SvTRUE(ST(i)));
                break;
            case DC_SIGCHAR_CHAR:
                dcArgChar(container->cvm,  (char)(SvIOK(ST(i)) ?SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_UCHAR:
                dcArgChar(container->cvm,(unsigned char) (SvUOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_SHORT:
                dcArgShort(container->cvm, (short)(SvIOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_USHORT:
                dcArgShort(container->cvm, (unsigned short)(SvUOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_INT:
                dcArgInt(container->cvm, (int)(SvIOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_UINT:
                dcArgInt(container->cvm, (unsigned int)(SvUOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_LONG:
                dcArgLong(container->cvm, (long)(SvIOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_ULONG:
                dcArgLong(container->cvm, (unsigned long)(SvUOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_LONGLONG:
                dcArgLongLong(container->cvm, (long long)(SvIOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_ULONGLONG:
                dcArgLongLong(container->cvm, (unsigned long long)(SvUOK(ST(i)) ? SvIV(ST(i)) : 0));
                break;
            case DC_SIGCHAR_FLOAT:
                dcArgFloat(container->cvm, (float)(SvNIOK(ST(i)) ? SvNV(ST(i)) : 0.0f));
                break;
            case DC_SIGCHAR_DOUBLE:
                dcArgDouble(container->cvm, (double)(SvNIOK(ST(i)) ? SvNV(ST(i)) : 0.0f));
                break;
            case DC_SIGCHAR_POINTER:
                warn("Unhandled arg type [%c] at %s line %d.", container->ret_type, __FILE__, __LINE__);
                break;
            case DC_SIGCHAR_STRING:
                dcArgPointer(container->cvm, SvPV_nolen(ST(i)));   break;
            case DC_SIGCHAR_AGGREGATE:
                warn("Unhandled arg type [%c] at %s line %d.", container->ret_type, __FILE__, __LINE__);
                break;
            case DC_SIGCHAR_ENDARG:
                if (tally > items) // TODO: Don't do this is signature is var_list, etc.
                    croak("Not enough arguments for callback");
                else if (tally < items)
                    croak("Too many arguments for callback");
                done++;
                break;
            case '(':
                break;
                // skip it for now
            default:
                warn("Unhandled arg type [%c] at %s line %d.", container->ret_type, __FILE__, __LINE__);
                break;
        }
        //warn("Done: %s", (done? "Yes": "No"));
        if (done) break;
    }
    //warn("Return type: %c at %s line %d.", container->ret_type, __FILE__, __LINE__);
    switch(container->ret_type) {
        case DC_SIGCHAR_VOID:
            dcCallVoid(container->cvm, self);
            XSRETURN_UNDEF;
            break;
        case DC_SIGCHAR_BOOL:
            RETVAL = newSViv(dcCallBool(container->cvm, self)); break; // TODO: Make this a real t/f
        case DC_SIGCHAR_CHAR:
            RETVAL = newSViv(dcCallChar(container->cvm, self)); break;
        case DC_SIGCHAR_UCHAR:
            RETVAL = newSVuv(dcCallChar(container->cvm, self)); break;
        case DC_SIGCHAR_SHORT:
            RETVAL = newSViv(dcCallShort(container->cvm, self)); break;
        case DC_SIGCHAR_USHORT:
            RETVAL = newSVuv(dcCallShort(container->cvm, self)); break;
        case DC_SIGCHAR_INT:
            RETVAL = newSViv(dcCallInt(container->cvm, self)); break;
        case DC_SIGCHAR_UINT:
            RETVAL = newSVuv(dcCallInt(container->cvm, self)); break;
        case DC_SIGCHAR_LONG:
            RETVAL = newSViv(dcCallLong(container->cvm, self)); break;
        case DC_SIGCHAR_ULONG:
            RETVAL = newSVuv(dcCallLong(container->cvm, self)); break;
        case DC_SIGCHAR_LONGLONG:
            RETVAL = newSViv(dcCallLongLong(container->cvm, self)); break;
        case DC_SIGCHAR_ULONGLONG:
            RETVAL = newSVuv(dcCallLongLong(container->cvm, self)); break;
        case DC_SIGCHAR_FLOAT:
            RETVAL = newSVnv(dcCallFloat(container->cvm, self)); break;
        case DC_SIGCHAR_DOUBLE:
            RETVAL = newSVnv(dcCallDouble(container->cvm, self)); break;
        case DC_SIGCHAR_POINTER:
            warn("Unhandled return type [%c] at %s line %d.", container->ret_type, __FILE__, __LINE__);
            XSRETURN_UNDEF;
            break;
        case DC_SIGCHAR_STRING:
            RETVAL = newSVpv((const char *) dcCallPointer(container->cvm, self), 0);
            break;
        case DC_SIGCHAR_AGGREGATE:
            warn("Unhandled return type [%c] at %s line %d.", container->ret_type, __FILE__, __LINE__);
            XSRETURN_UNDEF;
            break;
        default:
            warn("Unhandled return type [%c] at %s line %d.", container->ret_type, __FILE__, __LINE__);
            XSRETURN_UNDEF;
            break;
    }
    //warn("here at %s line %d.", __FILE__, __LINE__);
OUTPUT:
    RETVAL

void
Ximport( const char * package, ... )
CODE:
    const PERL_CONTEXT * cx_caller = caller_cx( 0, NULL );
    char *caller = HvNAME((HV*) CopSTASH(cx_caller->blk_oldcop));

    warn("Import from %s! items == %d", caller, items);

    int item;
    for (item = 1; item < items; ++item)
        warn("  item %d: %s", item, SvPV_nolen(ST(item)));

    //export_sub(ctx_stash, caller_stash, name);