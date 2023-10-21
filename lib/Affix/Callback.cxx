#include "../Affix.h"

char cbHandler(DCCallback *cb, DCArgs *args, DCValue *result, DCpointer userdata) {
    PERL_UNUSED_VAR(cb);
    Callback *cbx = (Callback *)userdata;
    dTHXa(cbx->perl);
    dSP;
    int count;
    char ret_c = cbx->ret;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    EXTEND(SP, (int)cbx->sig_len);
    for (size_t i = 0; i < cbx->sig_len; ++i) {
        switch (cbx->sig[i]) {
        case DC_SIGCHAR_VOID:
            // TODO: push undef?
            break;
        case DC_SIGCHAR_BOOL:
            mPUSHs(boolSV(dcbArgBool(args)));
            break;
        case DC_SIGCHAR_CHAR:
        case DC_SIGCHAR_UCHAR: {
            char *c = (char *)safemalloc(sizeof(char) * 2);
            c[0] = dcbArgChar(args);
            c[1] = 0;
            SV *w = newSVpv(c, 1);
            SvUPGRADE(w, SVt_PVNV);
            SvIVX(w) = SvIV(newSViv(c[0]));
            SvIOK_on(w);
            mPUSHs(w);
        } break;
        case WCHAR_FLAG: {
            wchar_t *c;
            Newx(c, 2, wchar_t);
            c[0] = (wchar_t)dcbArgLong(args);
            c[1] = 0;
            SV *w = wchar2utf(aTHX_ c, 1);
            SvUPGRADE(w, SVt_PVNV);
            SvIVX(w) = SvIV(newSViv(c[0]));
            SvIOK_on(w);
            mPUSHs(w);
        } break;
        case DC_SIGCHAR_SHORT:
            mPUSHi((IV)dcbArgShort(args));
            break;
        case DC_SIGCHAR_USHORT:
            mPUSHu((UV)dcbArgShort(args));
            break;
        case DC_SIGCHAR_INT:
            mPUSHi((IV)dcbArgInt(args));
            break;
        case DC_SIGCHAR_UINT:
            mPUSHu((UV)dcbArgInt(args));
            break;
        case DC_SIGCHAR_LONG:
            mPUSHi((IV)dcbArgLong(args));
            break;
        case DC_SIGCHAR_ULONG:
            mPUSHu((UV)dcbArgLong(args));
            break;
        case DC_SIGCHAR_LONGLONG:
            mPUSHi((IV)dcbArgLongLong(args));
            break;
        case DC_SIGCHAR_ULONGLONG:
            mPUSHu((UV)dcbArgLongLong(args));
            break;
        case DC_SIGCHAR_FLOAT:
            mPUSHn((NV)dcbArgFloat(args));
            break;
        case DC_SIGCHAR_DOUBLE:
            mPUSHn((NV)dcbArgDouble(args));
            break;
        case DC_SIGCHAR_POINTER: {
            DCpointer ptr = dcbArgPointer(args);
            if (ptr != NULL) {
                SV *type = *av_fetch(cbx->arg_info, i, 0);
                switch (SvIV(type)) {
                case CODEREF_FLAG: {
                    Callback *cb = (Callback *)dcbGetUserData((DCCallback *)ptr);
                    mPUSHs(cb->cv);
                } break;
                default: {
                    mPUSHs(ptr2sv(aTHX_ ptr, type));
                } break;
                }
            }
            else { mPUSHs(newSV(0)); }
        } break;
        case DC_SIGCHAR_STRING: {
            DCpointer ptr = dcbArgPointer(args);
            PUSHs(newSVpv((char *)ptr, 0));
        } break;
        case WSTRING_FLAG: {
            DCpointer ptr = dcbArgPointer(args);
            SV **type_sv = av_fetch(cbx->arg_info, i, 0);
            PUSHs(ptr2sv(aTHX_ ptr, *type_sv));
        } break;
        //~ case DC_SIGCHAR_INSTANCEOF: {
        //~ DCpointer ptr = dcbArgPointer(args);
        //~ HV *blessed = MUTABLE_HV(SvRV(*av_fetch(cbx->args, i, 0)));
        //~ SV **package = hv_fetchs(blessed, "package", 0);
        //~ PUSHs(sv_setref_pv(newSV(1), SvPV_nolen(*package), ptr));
        //~ } break;
        //~ case DC_SIGCHAR_ENUM:
        //~ case DC_SIGCHAR_ENUM_UINT: {
        //~ PUSHs(enum2sv(aTHX_ * av_fetch(cbx->args, i, 0), dcbArgInt(args)));
        //~ } break;
        //~ case DC_SIGCHAR_ENUM_CHAR: {
        //~ PUSHs(enum2sv(aTHX_ * av_fetch(cbx->args, i, 0), dcbArgChar(args)));
        //~ } break;
        //~ case DC_SIGCHAR_ANY: {
        //~ DCpointer ptr = dcbArgPointer(args);
        //~ SV *sv = newSV(0);
        //~ if (ptr != NULL && SvOK(MUTABLE_SV(ptr))) { sv = MUTABLE_SV(ptr); }
        //~ PUSHs(sv);
        //~ } break;
        default:
            croak("Unhandled callback arg. Type: %c [%s]", cbx->sig[i], cbx->sig);
            break;
        }
    }

    PUTBACK;
    if (cbx->ret == DC_SIGCHAR_VOID) {
        count = call_sv(cbx->cv, G_VOID);
        SPAGAIN;
    }
    else {
        count = call_sv(cbx->cv, G_SCALAR);
        SPAGAIN;
        if (count != 1) croak("Big trouble: %d returned items", count);
        SV *ret = POPs;
        switch (ret_c) {
        case DC_SIGCHAR_VOID:
            break;
        case DC_SIGCHAR_BOOL:
            result->B = SvTRUEx(ret);
            break;
        case DC_SIGCHAR_CHAR:
            result->c = SvIOK(ret) ? SvIV(ret) : 0;
            break;
        case DC_SIGCHAR_UCHAR:
            result->C = SvIOK(ret) ? ((UV)SvUVx(ret)) : 0;
            break;
        case WCHAR_FLAG: {
            ret_c = DC_SIGCHAR_LONG; // Fake it
            if (SvPOK(ret)) {
                STRLEN len;
                (void)SvPVx(ret, len);
                result->L = utf2wchar(aTHX_ ret, len)[0];
            }
            else { result->L = 0; }
        } break;
        case DC_SIGCHAR_SHORT:
            result->s = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_USHORT:
            result->S = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_INT:
            result->i = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_UINT:
            result->I = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_LONG:
            result->j = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_ULONG:
            result->J = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_LONGLONG:
            result->l = SvIOK(ret) ? SvIVx(ret) : 0;
            break;
        case DC_SIGCHAR_ULONGLONG:
            result->L = SvIOK(ret) ? SvUVx(ret) : 0;
            break;
        case DC_SIGCHAR_FLOAT:
            result->f = SvNOK(ret) ? SvNVx(ret) : 0.0;
            break;
        case DC_SIGCHAR_DOUBLE:
            result->d = SvNOK(ret) ? SvNVx(ret) : 0.0;
            break;
        case DC_SIGCHAR_POINTER: {
            if (SvOK(ret)) {
                if (sv_derived_from(ret, "Affix::Pointer")) {
                    IV tmp = SvIV((SV *)SvRV(ret));
                    result->p = INT2PTR(DCpointer, tmp);
                }
                else
                    croak("Returned value is not a Affix::Pointer or subclass");
            }
            else
                result->p = NULL; // ha.
        } break;
        case DC_SIGCHAR_STRING:
            result->Z = SvPOK(ret) ? SvPVx_nolen_const(ret) : NULL;
            break;
        //~ case DC_SIGCHAR_WIDE_STRING:
        //~ result->p = SvPOK(ret) ? (DCpointer)SvPVx_nolen_const(ret) : NULL;
        //~ ret_c = DC_SIGCHAR_POINTER;
        //~ break;
        //~ case DC_SIGCHAR_STRUCT:
        //~ case DC_SIGCHAR_UNION:
        //~ case DC_SIGCHAR_INSTANCEOF:
        //~ case DC_SIGCHAR_ANY:
        //~ result->p = SvPOK(ret) ?  sv2ptr(aTHX_ ret, _instanceof(aTHX_ cbx->retval), false):
        // NULL; ~ ret_c = DC_SIGCHAR_POINTER; ~ break;
        default:
            croak("Unhandled return from callback: %c", ret_c);
        }
    }
    PUTBACK;

    FREETMPS;
    LEAVE;

    return ret_c;
}
