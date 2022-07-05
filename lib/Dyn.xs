#include "lib/clutter.h"

#define META_ATTR "ATTR_SUB"

typedef struct Call
{
    DLLib *lib;
    const char *lib_name;
    const char *name;
    const char *sym_name;
    char * sig;
    size_t sig_len;
    char ret;
    DCCallVM *cvm;
    void *fptr;
    char *perl_sig;
} Call;

typedef struct Delayed
{
    const char *library;
    const char *library_version;
    const char *package;
    const char *symbol;
    const char *signature;
    const char *name;
    struct Delayed *next;
} Delayed;

Delayed *delayed; // Not thread safe










static char * clean(char *str) {
    char *end;
    while (isspace(*str) || *str == '"' || *str == '\'')
        str = str + 1;
    end = str + strlen(str) - 1;
    while (end > str && (isspace(*end) || *end == ')' || *end == '"' || *end == '\''))
        end = end - 1;
    *(end + 1) = '\0';
    return str;
}

void push_aggr(pTHX_ I32 ax, int pos, DCaggr *ag, void *struct_rep) {
    struct DCfield_ *addr;
    int size = 0;
    for (int x = 0; x < ag->n_fields; x++) {
        addr = &ag->fields[x];
        warn("[%d] %d", x, addr->size);
        size += addr->size;
    }

    struct_rep = saferealloc(struct_rep, size);
    warn("sizeof(&ag) == %d", sizeof(&ag));
    warn("&ag->size == %d", &ag->size);

    int d = 42;
    memcpy(struct_rep, &d, size); // OK
}

void push(pTHX_ Call *call, I32 ax) {
    const char *sig_ptr = call->sig;
    int sig_len = call->sig_len, pos = 0;
    char ch;
    for (ch = *sig_ptr; pos < sig_len - 1; ch = *++sig_ptr,++pos ) {
        switch (ch) {
        case DC_SIGCHAR_VOID:
            // TODO: Should I pass a NULL here?
            break;
        case DC_SIGCHAR_BOOL:
            dcArgBool(call->cvm, SvTRUE(ST(pos)));
            break;
        case DC_SIGCHAR_CHAR:
            dcArgChar(call->cvm, (char)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_UCHAR:
            dcArgChar(call->cvm, (unsigned char)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_SHORT:
            dcArgShort(call->cvm, (short)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_USHORT:
            dcArgShort(call->cvm, (unsigned short)SvUV(ST(pos)));
            break;
        case DC_SIGCHAR_INT:
            dcArgInt(call->cvm, (int)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_UINT:
            dcArgInt(call->cvm, (unsigned int)SvUV(ST(pos)));
            break;
        case DC_SIGCHAR_LONG:
            dcArgLong(call->cvm, (long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_ULONG:
            dcArgLong(call->cvm, (unsigned long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_LONGLONG:
            dcArgLongLong(call->cvm, (long long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_ULONGLONG:
            dcArgLongLong(call->cvm, (unsigned long long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_FLOAT:
            dcArgFloat(call->cvm, (float)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_DOUBLE:
            dcArgDouble(call->cvm, (double)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_POINTER:
            //warn("passing pointer at %s line %d", __FILE__, __LINE__);
            {
                IV tmp = SvIV((SV *)SvRV(ST(pos)));
                int *arg = INT2PTR(int *, tmp);
                dcArgPointer(call->cvm, arg);
            }
            break;
        case DC_SIGCHAR_STRING:
            dcArgPointer(call->cvm, SvPVutf8_nolen(ST(pos)));
            break;
        case DC_SIGCHAR_AGGREGATE: {
            ; // empty statement before decl. [C89 vs. C99]
              /* XXX: dyncall structs/union/array aren't ready yet*/
            void *struct_rep;
            Newxz(struct_rep, 0, int); // ha!
            DCaggr *ag;
            if (sv_derived_from(ST(pos), "Dyn::Call::Aggr")) {
                IV tmp = SvIV((SV *)SvRV(ST(pos)));
                ag = INT2PTR(DCaggr *, tmp);
            }
            else
                croak("expected an aggregate but this is not of type Dyn::Call::Aggr");

            push_aggr(aTHX_ ax, pos, ag, struct_rep);
            dcArgAggr(call->cvm, ag, struct_rep);

            Safefree(struct_rep);
            break;
        }
        case '{': {
            warn("here");
            AV * values;
            STMT_START {
                SV* const xsub_tmp_sv = ST(pos);
                SvGETMAGIC(xsub_tmp_sv);
                if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV)
                    values = (AV*)SvRV(xsub_tmp_sv);
                else
                    Perl_croak_nocontext("struct values must be passed as an array reference");
            } STMT_END;

            warn("hash character: %c at %s line %d", ch, __FILE__, __LINE__);
            void *hash_rep;
            Newxz(hash_rep, 0, int); // ha!
            DCaggr *ag = dcNewAggr(0,0); // XXX - wrong size; maybe expect an AV * and use count from that?

            size_t offset=0;

            for (ch = *++sig_ptr; pos < sig_len - 2; ch = *++sig_ptr, pos++) {
                warn("    hash content character: %c at %s line %d", ch, __FILE__, __LINE__);

                switch (ch) {
                case DC_SIGCHAR_VOID:
                    // TODO: Should I pass a NULL here?
                    continue;
                case DC_SIGCHAR_BOOL:
                    dcArgBool(call->cvm, SvTRUE(ST(pos)));
                    continue;
                case DC_SIGCHAR_CHAR:{
                    offset += padding_needed_for( offset, 1 );

                    dcAggrField(ag, ch, offset, 1);
                    DCfield *field = &ag->fields[ag->n_fields - 1];
                    warn("size: %d | offset: %d", field->size, offset);
                    hash_rep = saferealloc(hash_rep, offset + field->size);
                    SV * s =av_shift(values);
                    char d = (char) SvIV(s);
                    warn("char == %c [%d]", d, d);
                    memcpy(hash_rep + offset, &d, field->size);
                    offset+=field->size;
                    }
                    continue;
                case DC_SIGCHAR_UCHAR:
                    dcArgChar(call->cvm, (unsigned char)SvIV(ST(pos)));
                    continue;
                case DC_SIGCHAR_SHORT:{
                    offset += padding_needed_for( offset, 2 );

                    dcAggrField(ag, ch, offset, 1);
                    DCfield * field = &ag->fields[ag->n_fields - 1];
                    warn("size: %d | offset: %d", field->size, offset);

                    hash_rep = saferealloc(hash_rep, offset + field->size);
                    SV * s =av_shift(values);
                    short d = (short) SvIV(s);
                    memcpy(hash_rep + offset, &d, field->size);
                    offset+=field->size * 1;
                }
                    continue;
                case DC_SIGCHAR_USHORT:
                    dcArgShort(call->cvm, (unsigned short)SvUV(ST(pos)));
                    continue;
                case DC_SIGCHAR_INT:{
                    offset += padding_needed_for( offset, 4 );
                    dcAggrField(ag, ch, offset, 1);
                    DCfield *field = &ag->fields[ag->n_fields - 1];
                    warn("size: %d | offset: %d", field->size, offset);
                    hash_rep = saferealloc(hash_rep, offset + field->size);
                    SV * s =av_shift(values);
                    int d = (int) SvIV(s);
                    memcpy(hash_rep + offset, &d, field->size);
                    offset+=field->size * 1;
                }
                    continue;
                case DC_SIGCHAR_UINT:
                    dcArgInt(call->cvm, (unsigned int)SvUV(ST(pos)));
                    continue;
                case DC_SIGCHAR_LONG:
                    dcArgLong(call->cvm, (long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_ULONG:
                    dcArgLong(call->cvm, (unsigned long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_LONGLONG:
                    dcArgLongLong(call->cvm, (long long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_ULONGLONG:
                    dcArgLongLong(call->cvm, (unsigned long long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_FLOAT:
                    dcArgFloat(call->cvm, (float)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_DOUBLE:
                    dcArgDouble(call->cvm, (double)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_POINTER:
                    warn("passing pointer at %s line %d", __FILE__, __LINE__);
                    {
                        IV tmp = SvIV((SV *)SvRV(ST(pos)));
                        int *arg = INT2PTR(int *, tmp);
                        dcArgPointer(call->cvm, arg);
                    }
                    continue;
                case DC_SIGCHAR_STRING:
                    dcArgPointer(call->cvm, SvPVutf8_nolen(ST(pos)));
                    continue;
                case DC_SIGCHAR_AGGREGATE:
                    warn("gotta go deeper at %s line %d", __FILE__, __LINE__);

                    continue;
                default:
                    //warn("no idea what to do here at %s line %d", __FILE__, __LINE__);

                    continue;
                }
                break;
            }

            dcCloseAggr(ag);
            dcArgAggr(call->cvm, ag, hash_rep);
                    warn("gotta go at %s line %d", __FILE__, __LINE__);

            //Safefree(hash_rep);
            break;
        }
        case '<':
            warn("union character: %c at %s line %d", ch, __FILE__, __LINE__);

            break;
        case '[':
            warn("array character: %c at %s line %d", ch, __FILE__, __LINE__);

            break;
        default:
            warn("unhandled signature character: %c at %s line %d", ch, __FILE__, __LINE__);
            break;
        }
    }

    return;
}

SV *retval(pTHX_ Call *call) {
    // warn("Here I am! at %s line %d", __FILE__, __LINE__);
    //  TODO: Also sort out pointers that might be return values?
    switch (call->ret) {
    case DC_SIGCHAR_VOID:
        dcCallVoid(call->cvm, call->fptr);
        return NULL;
    case DC_SIGCHAR_BOOL:
        return boolSV(dcCallBool(call->cvm, call->fptr));
    case DC_SIGCHAR_CHAR:
        return newSVnv(dcCallChar(call->cvm, call->fptr));
    case DC_SIGCHAR_UCHAR:
        return newSVuv(dcCallChar(call->cvm, call->fptr));
    case DC_SIGCHAR_SHORT:
        return newSViv(dcCallShort(call->cvm, call->fptr));
    case DC_SIGCHAR_USHORT:
        return newSVuv(dcCallShort(call->cvm, call->fptr));
    case DC_SIGCHAR_INT:
        return newSViv(dcCallInt(call->cvm, call->fptr));
    case DC_SIGCHAR_UINT:
        return newSVuv(dcCallInt(call->cvm, call->fptr));
    case DC_SIGCHAR_LONG:
        return newSViv(dcCallLong(call->cvm, call->fptr));
    case DC_SIGCHAR_ULONG:
        return newSVuv(dcCallLong(call->cvm, call->fptr));
    case DC_SIGCHAR_LONGLONG:
        return newSViv(dcCallLongLong(call->cvm, call->fptr));
    case DC_SIGCHAR_ULONGLONG:
        return newSVuv(dcCallLongLong(call->cvm, call->fptr));
    case DC_SIGCHAR_FLOAT:
        return newSVnv(dcCallFloat(call->cvm, call->fptr));
    case DC_SIGCHAR_DOUBLE:
        return newSVnv(dcCallDouble(call->cvm, call->fptr));
    case DC_SIGCHAR_POINTER:; // empty statement before decl. [C89 vs. C99]
        SV *retval;
        retval = sv_newmortal();
        if (0)
            sv_setref_pv(retval, "Dyn::pointer", (void *)dcCallPointer(call->cvm, call->fptr));
        else
            sv_setpv(retval, (const char *)dcCallPointer(call->cvm, call->fptr));
        return retval;
    case DC_SIGCHAR_STRING:
        return newSVpv(dcCallPointer(call->cvm, call->fptr), 0);
    case DC_SIGCHAR_AGGREGATE: /* TODO: dyncall structs/union/array aren't ready upstream yet*/
        warn(
            "dyncall aggregate types (structs/union/array) aren't ready upstream yet at %s line %d",
            __FILE__, __LINE__);
        break;
    default:
        warn("Unknown return character: %c at %s line %d", call->ret, __FILE__, __LINE__);
    };
    return NULL;
}

// TODO: This might need to return values in arg pointers
#define _call_                                                                                     \
    if (call != NULL) {                                                                            \
        dcReset(call->cvm);                                                                        \
        push(aTHX_(Call *) call, ax);                                                              \
        /*warn("ret == %c", call->ret);*/                                                          \
        SV *ret = retval(aTHX_ call);                                                              \
        if (ret != NULL) {                                                                         \
            ST(0) = ret;                                                                           \
            XSRETURN(1);                                                                           \
        }                                                                                          \
        else                                                                                       \
            XSRETURN_EMPTY;                                                                        \
        /*//warn("here at %s line %d", __FILE__, __LINE__);*/                                      \
    }                                                                                              \
    else                                                                                           \
        croak("Function is not attached! This is a serious bug!");                                 \
    /*//warn("here at %s line %d", __FILE__, __LINE__);*/

static Call *_load(pTHX_ DLLib *lib, const char *symbol, const char *sig) {
    if (lib == NULL) return NULL;
    // warn("_load(..., %s, %s)", symbol, sig);
    Call *RETVAL;
    Newx(RETVAL, 1, Call);
    RETVAL->lib = lib;
    RETVAL->cvm = dcNewCallVM(1024);
    //RETVAL->retval = newSV();
    if (RETVAL->cvm == NULL) return NULL;
    RETVAL->fptr = dlFindSymbol(RETVAL->lib, symbol);
    if (RETVAL->fptr == NULL) // TODO: throw warning
        return NULL;
    Newxz(RETVAL->sig, strlen(sig), char);      // Dumb
    Newxz(RETVAL->perl_sig, strlen(sig), char); // Dumb
    int i, sig_pos, depth;
    sig_pos = depth = 0;
    for (i = 0; sig[i + 1] != '\0'; ++i) {
        switch (sig[i]) {
        case DC_SIGCHAR_CC_PREFIX:
            ++i;
            switch (sig[i]) {
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
        case DC_SIGCHAR_DOUBLE: {
            RETVAL->sig[sig_pos] = sig[i];
            ++sig_pos;
                    }

                break;
        case DC_SIGCHAR_POINTER:
        case DC_SIGCHAR_STRING:
        case DC_SIGCHAR_AGGREGATE:
            if (depth == 0) RETVAL->perl_sig[sig_pos] = '$';
            RETVAL->sig[sig_pos] = sig[i];
            ++sig_pos;
            break;
        case '<': // union
            if (depth == 0) RETVAL->perl_sig[sig_pos] = '$';
            RETVAL->sig[sig_pos] = sig[i];
            ++sig_pos;
            depth++;
            break;
        case '{': // struct
            if (depth == 0) RETVAL->perl_sig[sig_pos] = '%';
            RETVAL->sig[sig_pos] = sig[i];
            ++sig_pos;
            depth++;
            break;
        case '[': // array
            if (depth == 0) RETVAL->perl_sig[sig_pos] = '@';
            RETVAL->sig[sig_pos] = sig[i];
            ++sig_pos;
            depth++;
            break;
        case '>':
        case '}':
        case ']':
            RETVAL->sig[sig_pos] = sig[i];
            ++sig_pos;
            --depth;
            break;
        case DC_SIGCHAR_ENDARG:
            RETVAL->sig_len = sig_pos + 1;
            RETVAL->ret = sig[i + 1];
            break;
        case '(': // Start of signature

            break;
        default:
            warn("Unknown signature character: %c at %s line %d", sig[i], __FILE__, __LINE__);
            break;
        };
    }
    // warn("Now: %s|%s|%c", RETVAL->perl_sig, RETVAL->sig, RETVAL->ret);
    return RETVAL;
}




MODULE = Dyn PACKAGE = Dyn

void
test(AV * values, int lettter)
CODE:
    ;

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
PPCODE:
    Call * call = XSANY.any_ptr;
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


