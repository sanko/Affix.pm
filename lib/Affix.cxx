#include "Affix.h"

/* globals */
#define MY_CXT_KEY "Affix::_guts" XS_VERSION

typedef struct {
    DCCallVM *cvm;
} my_cxt_t;

START_MY_CXT

extern "C" void Affix_trigger(pTHX_ CV *cv) {
    dXSARGS;
    dMY_CXT;
    PING;

    Affix *affix = (Affix *)XSANY.any_ptr;
    char **free_strs = NULL;
    void **free_ptrs = NULL;
    PING;

    int num_args = affix->num_args;
    int num_strs = 0, num_ptrs = 0;
    int16_t *arg_types = affix->arg_types;
    bool void_ret = false;
    DCaggr *agg_ = NULL;
    char *package = NULL;
    PING;

    // dcMode(MY_CXT.cvm, affix->call_conv);
    dcReset(MY_CXT.cvm);
    PING;

    switch (affix->ret_type) {
    case AFFIX_TYPE_CUNION:
    case AFFIX_TYPE_CSTRUCT:
    case AFFIX_TYPE_CPPSTRUCT: {
        agg_ = _aggregate(aTHX_ affix->ret_info);
#if DEBUG > 1
        warn("  dcBeginCallAggr(%p, %p);", (void *)MY_CXT.cvm, (void *)agg_);
#endif
        dcBeginCallAggr(MY_CXT.cvm, agg_);
    } break;
    }
#if DEBUG > 1
    warn("args: ");
    DD(MUTABLE_SV(affix->arg_info));
#endif
    { // This is dumb
        int __num_args = num_args;
        if (affix->_cpp_struct && !affix->_cpp_constructor) ++__num_args;
        if (UNLIKELY(items != __num_args)) {
            if (UNLIKELY(items > __num_args))
                croak("Too many arguments; wanted %d, found %d", __num_args, items);
            croak("Not enough arguments; wanted %d, found %d", __num_args, items);
        }
    }

#if DEBUG > 1
    warn("Expecting %d arguments...", num_args);
#endif
    DCpointer this_ptr = NULL;

    for (int arg_pos = affix->_cpp_constructor ? 1 : 0, info_pos = 0; LIKELY(arg_pos < num_args);
         ++arg_pos, ++info_pos) {
#if DEBUG
        warn("arg_pos: %d, num_args: %d, info_pos: %d", arg_pos, num_args, info_pos);
        warn("   type: %d, as_str: %s", arg_types[info_pos], type_as_str(arg_types[info_pos]));
#if DEBUG > 1
        DD(ST(arg_pos));
#endif
#endif
        switch (arg_types[info_pos]) {
        case DC_SIGCHAR_CC_PREFIX: {
            // TODO: Not a fan of this entire section
            // I could/should make CC_* scalarize to ints, and in Affix_affix put _PREFIX in
            // arg_types and the actuall CC object in arg_info
            SV *cc = *av_fetch(affix->arg_info, info_pos, 0);
            char mode = DC_SIGCHAR_CC_DEFAULT;
            if (sv_derived_from(cc, "Affix::Type::CC::DEFALT")) { mode = DC_SIGCHAR_CC_DEFAULT; }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::THISCALL"))) {
                mode = DC_SIGCHAR_CC_THISCALL;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ELLIPSIS"))) {
                mode = DC_SIGCHAR_CC_ELLIPSIS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ELLIPSIS_VARARGS"))) {
                mode = DC_SIGCHAR_CC_ELLIPSIS_VARARGS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::CDECL"))) {
                mode = DC_SIGCHAR_CC_CDECL;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::STDCALL"))) {
                mode = DC_SIGCHAR_CC_STDCALL;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::FASTCALL_MS"))) {
                mode = DC_SIGCHAR_CC_FASTCALL_MS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::FASTCALL_GNU"))) {
                mode = DC_SIGCHAR_CC_FASTCALL_GNU;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::THISCALL_MS"))) {
                mode = DC_SIGCHAR_CC_THISCALL_MS;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::THISCALL_GNU"))) {
                mode = DC_SIGCHAR_CC_THISCALL_GNU;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ARM_ARM "))) {
                mode = DC_SIGCHAR_CC_ARM_ARM;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::ARM_THUMB"))) {
                mode = DC_SIGCHAR_CC_ARM_THUMB;
            }
            else if (UNLIKELY(sv_derived_from(cc, "Affix::Type::CC::SYSCALL"))) {
                mode = DC_SIGCHAR_CC_SYSCALL;
            }
            else { croak("Unknown calling convention"); }
            dcMode(MY_CXT.cvm, dcGetModeFromCCSigChar(mode));
            if (mode != DC_SIGCHAR_CC_ELLIPSIS && mode != DC_SIGCHAR_CC_ELLIPSIS_VARARGS) {
                dcReset(MY_CXT.cvm);
            }
            arg_pos--;
            if (affix->_cpp_constructor) {
                Newxz(this_ptr, 1, intptr_t);
                dcArgPointer(MY_CXT.cvm, this_ptr);
                package = SvPV_nolen(ST(0));
            }
        } break;
        case AFFIX_TYPE_VOID: {
#if DEBUG > 1
            warn("Skipping void parameter");
#endif
            // skip
        } break;
        case AFFIX_TYPE_BOOL: {
            dcArgBool(MY_CXT.cvm, SvTRUE(ST(arg_pos))); // Anything can be a bool
        } break;
        case AFFIX_TYPE_CHAR: {
            if (SvIOK(ST(arg_pos))) { dcArgChar(MY_CXT.cvm, (I8)SvIV(ST(arg_pos))); }
            else {
                STRLEN len;
                char *value = SvPVbyte(ST(arg_pos), len);
                if (len > 1) { warn("Expected a single character; found %ld", len); }
                dcArgChar(MY_CXT.cvm, (I8)value[0]);
            }
        } break;
        case AFFIX_TYPE_UCHAR: {
            if (SvIOK(ST(arg_pos))) { dcArgChar(MY_CXT.cvm, (U8)SvUV(ST(arg_pos))); }
            else {
                STRLEN len;
                char *value = SvPVbyte(ST(arg_pos), len);
                if (len > 1) { warn("Expected a single character; found %ld", len); }
                dcArgChar(MY_CXT.cvm, (U8)value[0]);
            }
        } break;
        case AFFIX_TYPE_WCHAR: {
            if (SvOK(ST(arg_pos))) {
                char *eh = SvPV_nolen(ST(arg_pos));
                PUTBACK;
                const char *pat = "W";
                SSize_t s = unpackstring(pat, pat + 1, eh, eh + WCHAR_T_SIZE + 1, SVt_PVAV);
                SPAGAIN;
                if (UNLIKELY(s != 1)) croak("Failed to unpack wchar_t");
                switch (WCHAR_T_SIZE) {
                case I8SIZE:
                    dcArgChar(MY_CXT.cvm, (char)POPi);
                    break;
                case SHORTSIZE:
                    dcArgShort(MY_CXT.cvm, (short)POPi);
                    break;
                case INTSIZE:
                    dcArgInt(MY_CXT.cvm, (int)POPi);
                    break;
                default:
                    croak("Invalid wchar_t size for argument!");
                }
            }
            else
                dcArgInt(MY_CXT.cvm, 0);
        } break;
        case AFFIX_TYPE_SHORT: {
            dcArgShort(MY_CXT.cvm, (short)(SvIV(ST(arg_pos))));
        } break;
        case AFFIX_TYPE_USHORT: {
            dcArgShort(MY_CXT.cvm, (unsigned short)(SvUV(ST(arg_pos))));
        } break;
        case AFFIX_TYPE_INT: {
#if DEBUG > 1
            warn("  dcArgInt(%p, %ld);", (void *)MY_CXT.cvm, SvIV(ST(arg_pos)));
#endif
            dcArgInt(MY_CXT.cvm, (int)(SvIV(ST(arg_pos))));
        } break;
        case AFFIX_TYPE_UINT: {
#if DEBUG > 1
            warn("  dcArgInt(%p, %lu);", (void *)MY_CXT.cvm, SvUV(ST(arg_pos)));
#endif
            dcArgInt(MY_CXT.cvm, (unsigned int)(SvUV(ST(arg_pos))));
        } break;
        case AFFIX_TYPE_LONG:
            dcArgLong(MY_CXT.cvm, (unsigned long)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_TYPE_ULONG: {
            dcArgLong(MY_CXT.cvm, (unsigned long)(SvUV(ST(arg_pos))));
        } break;
        case AFFIX_TYPE_LONGLONG: {
            dcArgLongLong(MY_CXT.cvm, (I64)(SvIV(ST(arg_pos))));
        } break;
        case AFFIX_TYPE_ULONGLONG:
            dcArgLongLong(MY_CXT.cvm, (U64)(SvUV(ST(arg_pos))));
            break;
        case AFFIX_TYPE_FLOAT: {
            dcArgFloat(MY_CXT.cvm, (float)SvNV(ST(arg_pos)));
        } break;
        case AFFIX_TYPE_DOUBLE: {
            dcArgDouble(MY_CXT.cvm, (double)SvNV(ST(arg_pos)));
        } break;
        case AFFIX_TYPE_ASCIISTR: {
            dcArgPointer(MY_CXT.cvm, SvOK(ST(arg_pos)) ? SvPV_nolen(ST(arg_pos)) : NULL);
        } break;
        case AFFIX_TYPE_STD_STRING: {
            std::string tmp = SvOK(ST(arg_pos)) ? SvPV_nolen(ST(arg_pos)) : NULL;
            dcArgPointer(MY_CXT.cvm, static_cast<void *>(&tmp));
        } break;
        case AFFIX_TYPE_UTF8STR:
        case AFFIX_TYPE_UTF16STR: {
            if (SvOK(ST(arg_pos))) {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                // Newxz(free_ptrs[num_ptrs], _sizeof(aTHX_ newSViv(AFFIX_TYPE_UTF16STR)), char);
                free_ptrs[num_ptrs] =
                    sv2ptr(aTHX_ newSViv(AFFIX_TYPE_UTF16STR), ST(arg_pos), false);
                dcArgPointer(MY_CXT.cvm, *(DCpointer *)(free_ptrs[num_ptrs++]));
            }
            else { dcArgPointer(MY_CXT.cvm, NULL); }
        } break;
        case AFFIX_TYPE_CUNION:
        case AFFIX_TYPE_CSTRUCT:
        case AFFIX_TYPE_CPPSTRUCT: {
            if (!SvOK(ST(arg_pos)) && SvREADONLY(ST(arg_pos)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                if (!SvROK(ST(arg_pos)) || SvTYPE(SvRV(ST(arg_pos))) != SVt_PVHV)
                    croak("Type of arg %d must be an hash ref", arg_pos + 1);
                //~ AV *elements = MUTABLE_AV(SvRV(ST(i)));
                SV **type = av_fetch(affix->arg_info, info_pos, 0);
                DCaggr *agg = _aggregate(aTHX_ * type);
                PING;
                free_ptrs[num_ptrs] = sv2ptr(aTHX_ * type, ST(arg_pos), false);
                dcArgAggr(MY_CXT.cvm, agg, free_ptrs[num_ptrs++]);
            }
        }
        //~ croak("Unhandled arg type at %s line %d", __FILE__, __LINE__);
        break;
        case AFFIX_TYPE_CARRAY: {
            if (!SvOK(ST(arg_pos)) && SvREADONLY(ST(arg_pos)) // explicit undef
            ) {
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else {
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                SV *type = *av_fetch(affix->arg_info, info_pos, 0);
                HV *hv_ptr = MUTABLE_HV(SvRV(type));
                SV *subtype = *hv_fetch(hv_ptr, "type", 4, 0);
                // free_ptrs = (void **)safemalloc(num_args * sizeof(DCpointer));
                if ((SvROK(ST(arg_pos)) && SvTYPE(SvRV(ST(arg_pos))) == SVt_PVAV) ||
                    ((sv_derived_from(subtype, "Affix::Type::Char") ||
                      sv_derived_from(subtype, "Affix::Type::UChar") ||
                      sv_derived_from(subtype, "Affix::Type::WChar")) &&
                     SvPOK(ST(arg_pos)))) {
                    AV *elements = MUTABLE_AV(SvRV(ST(arg_pos)));
                    size_t av_len;
                    if (hv_exists(hv_ptr, "size", 4)) {
                        av_len = SvIV(*hv_fetchs(hv_ptr, "size", 0));
                        if (av_count(elements) != av_len)
                            croak("Expected an array of %lu elements; found %ld", av_len,
                                  av_count(elements));
                    }
                    else if (SvPOK(ST(arg_pos))) {
                        (void)SvPV(ST(arg_pos), av_len);
                        hv_stores(hv_ptr, "dyn_size", newSVuv(av_len));
                    }
                    else {
                        PING;
                        av_len = av_count(elements);
                        hv_stores(hv_ptr, "dyn_size", newSVuv(av_len));
                    }
                    PING;
                    //~ hv_stores(hv_ptr, "sizeof", newSViv(av_len));
                    PING;
                    free_ptrs[num_ptrs] = sv2ptr(aTHX_ type, ST(arg_pos), false);
                    PING;
                    dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs++]);
                    PING;
                }
                else
                    croak("Arg %d must be an array reference", arg_pos + 1);
            }
        } break;
        case AFFIX_TYPE_CALLBACK: {
            if (SvOK(ST(arg_pos))) {
                //~ DCCallback *hold;
                //~ //Newxz(hold, 1, DCCallback);
                //~ sv2ptr(aTHX_ SvRV(*av_fetch(affix->arg_info, info_pos, 0)), ST(arg_pos), hold,
                // false); ~ dcArgPointer(MY_CXT.cvm, hold);
                CallbackWrapper *hold = (CallbackWrapper *)sv2ptr(
                    aTHX_ * av_fetch(affix->arg_info, info_pos, 0), ST(arg_pos), false);
                dcArgPointer(MY_CXT.cvm, hold->cb);
            }
            else
                dcArgPointer(MY_CXT.cvm, NULL);
        } break;
        case AFFIX_TYPE_SV: {
            SV *type = *av_fetch(affix->arg_info, info_pos, 0);
            DCpointer blah = sv2ptr(aTHX_ type, ST(arg_pos), false);
            dcArgPointer(MY_CXT.cvm, blah);
        } break;
        case AFFIX_TYPE_CPOINTER:
        case AFFIX_TYPE_REF: {
#if DEBUG
            warn("AFFIX_TYPE_CPOINTER [%d, %ld/%s]", arg_pos,
                 SvIV(*av_fetch(affix->arg_info, info_pos, 0)),
                 type_as_str(SvIV(*av_fetch(affix->arg_info, info_pos, 0))));
#if DEBUG > 1
            DD(MUTABLE_SV(affix->arg_info));
            DD(*av_fetch(affix->arg_info, info_pos, 0));
#endif
#endif
            if (UNLIKELY(!SvOK(ST(arg_pos)) && SvREADONLY(ST(arg_pos)))) { // explicit undef
                warn("NULL POINTER");
                dcArgPointer(MY_CXT.cvm, NULL);
            }
            else if (sv_derived_from(ST(arg_pos),
                                     "Affix::Pointer")) { // pass pointers directly through
                IV tmp = SvIV(SvRV(ST(arg_pos)));
                dcArgPointer(MY_CXT.cvm, INT2PTR(DCpointer, tmp));
            }
            else {
                PING;
                if (!free_ptrs) Newxz(free_ptrs, num_args, DCpointer);
                SV *type = *av_fetch(affix->arg_info, info_pos, 0);

                if (UNLIKELY(sv_isobject(ST(arg_pos)) &&
                             sv_derived_from(type, "Affix::Type::InstanceOf"))) {
                    SV *cls = *hv_fetch(MUTABLE_HV(SvRV(type)), "class", 5, 0);
                    if (!sv_derived_from_sv(ST(arg_pos), cls, 0)) {
                        croak("Expected a subclass of %s in argument %d", SvPV_nolen(cls),
                              arg_pos + 1);
                    }
                }
                PING;
                free_ptrs[num_ptrs] = sv2ptr(aTHX_ type, ST(arg_pos), false);
                // DumpHex(free_ptrs[num_ptrs], size);
                dcArgPointer(MY_CXT.cvm, free_ptrs[num_ptrs]);
                num_ptrs++;
            }
        } break;
        default:
            croak("Unhandled arg type %s (%d) at %s line %d", type_as_str(arg_types[info_pos]),
                  (arg_types[info_pos]), __FILE__, __LINE__);
            break;
        }
    }

    SV *RETVAL;

    if (UNLIKELY(UNLIKELY(affix->_cpp_constructor) &&
                 LIKELY(this_ptr != NULL && package != NULL))) {
#if DEBUG
        warn("Return type %s from C++ constructor / %p at %s line %d", package, affix->entry_point,
             __FILE__, __LINE__);
#endif
        //~ TODO: Make package inherit from Affix::Pointer::Unmanaged
        dcCallVoid(MY_CXT.cvm, affix->entry_point);
        RETVAL = newSV(1);
        sv_setref_pv(RETVAL, package, this_ptr);
    }
    else {
#if DEBUG
        warn("Return type %d (%s) / %p at %s line %d", affix->ret_type,
             type_as_str(affix->ret_type), affix->entry_point, __FILE__, __LINE__);
#endif
        switch (affix->ret_type) {
        case AFFIX_TYPE_VOID:
            void_ret = true;
            dcCallVoid(MY_CXT.cvm, affix->entry_point);
            break;
        case AFFIX_TYPE_BOOL: // DCbool is an int!
            RETVAL = boolSV((bool)dcCallBool(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_CHAR: {
            char value[1];
            value[0] = dcCallChar(MY_CXT.cvm, affix->entry_point);
            RETVAL = newSV(0);
            sv_setsv(RETVAL, newSVpv(value, 1));
            (void)SvUPGRADE(RETVAL, SVt_PVIV);
            SvIV_set(RETVAL, ((IV)value[0]));
            SvIOK_on(RETVAL);
        } break;
        case AFFIX_TYPE_UCHAR: {
            unsigned char value[1];
            value[0] = dcCallChar(MY_CXT.cvm, affix->entry_point);
            RETVAL = newSV(0);
            sv_setsv(RETVAL, newSVpv((char *)value, 1));
            (void)SvUPGRADE(RETVAL, SVt_PVIV);
            SvIV_set(RETVAL, ((UV)value[0]));
            SvIOK_on(RETVAL);
        } break;
        case AFFIX_TYPE_WCHAR: {
            SV *container;
            RETVAL = newSVpvs("");
            const char *pat = "W";
            switch (WCHAR_T_SIZE) {
            case I8SIZE:
                container = newSViv((char)dcCallChar(MY_CXT.cvm, affix->entry_point));
                break;
            case SHORTSIZE:
                container = newSViv((short)dcCallShort(MY_CXT.cvm, affix->entry_point));
                break;
            case INTSIZE:
                container = newSViv((int)dcCallInt(MY_CXT.cvm, affix->entry_point));
                break;
            default:
                croak("Invalid wchar_t size for argument!");
            }
            sv_2mortal(container);
            packlist(RETVAL, pat, pat + 1, &container, &container + 1);
        } break;
        case AFFIX_TYPE_SHORT:
            RETVAL = newSViv((short)dcCallShort(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_USHORT:
            RETVAL = newSVuv((unsigned short)dcCallShort(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_INT:
            RETVAL = newSViv((signed int)dcCallInt(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_UINT:
            RETVAL = newSVuv((unsigned int)dcCallInt(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_LONG:
            RETVAL = newSViv((long)dcCallLong(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_ULONG:
            RETVAL = newSVuv((unsigned long)dcCallLong(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_LONGLONG:
            RETVAL = newSViv((I64)dcCallLongLong(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_ULONGLONG:
            RETVAL = newSVuv((U64)dcCallLongLong(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_FLOAT:
            RETVAL = newSVnv(dcCallFloat(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_DOUBLE:
            RETVAL = newSVnv(dcCallDouble(MY_CXT.cvm, affix->entry_point));
            break;
        case AFFIX_TYPE_ASCIISTR:
            RETVAL = newSVpv((char *)dcCallPointer(MY_CXT.cvm, affix->entry_point), 0);
            break;
        case AFFIX_TYPE_UTF16STR: {
            wchar_t *str = (wchar_t *)dcCallPointer(MY_CXT.cvm, affix->entry_point);
            RETVAL = wchar2utf(aTHX_ str, wcslen(str));
        } break;
        case AFFIX_TYPE_CPOINTER:
        case AFFIX_TYPE_REF: {
            PING;
            SV *subtype = *hv_fetchs(MUTABLE_HV(SvRV(affix->ret_info)), "type", 0);
            PING;

            DCpointer p = dcCallPointer(MY_CXT.cvm, affix->entry_point);
            PING;
            if (sv_derived_from(subtype, "Affix::Type::Char")) {
                PING;
                RETVAL = ptr2sv(aTHX_ p, affix->ret_info);
            }
            //~
            //~ DCpointer p = dcCallPointer(MY_CXT.cvm, affix->entry_point);
            //~ if(UNLIKELY(sv_derived_from(affix->ret_info, "Affix::Type::InstanceOf"))){
            //~ SV *cls = *hv_fetchs(MUTABLE_HV(SvRV(affix->ret_info)), "class", 0);
            //~ sv_setref_pv(RETVAL, SvPV_nolen(cls), p);
            //~ }
            else {
                PING;
                RETVAL = newSV(1);
                PING;
                sv_setref_pv(RETVAL, "Affix::Pointer::Unmanaged", p);
            }
            //~ }
            //~ else {

            //~ }
            PING;

        } break;
        case AFFIX_TYPE_CUNION:
        case AFFIX_TYPE_CSTRUCT:
        case AFFIX_TYPE_CPPSTRUCT: {
            DCpointer p = safemalloc(_sizeof(aTHX_ affix->ret_info));
#if DEBUG
            warn("  DCpointer p [%p] = safemalloc(%ld);", p, _sizeof(aTHX_ affix->ret_info));
            warn("  dcCallAggr(%p, %p, %p, %p);", (void *)MY_CXT.cvm, (void *)affix->entry_point,
                 (void *)agg_, p);
#endif
            dcCallAggr(MY_CXT.cvm, affix->entry_point, agg_, p);
            PING;
            RETVAL = ptr2sv(aTHX_ p, affix->ret_info);
            PING;
        } break;
        case AFFIX_TYPE_CARRAY: {
            //~ warn ("        _sizeof(aTHX_ affix->ret_info): %d",_sizeof(aTHX_ affix->ret_info));
            // DCpointer p = safemalloc(_sizeof(aTHX_ affix->ret_info));
            // dcCallAggr(MY_CXT.cvm, affix->entry_point, agg_, p);
            DCpointer p = dcCallPointer(MY_CXT.cvm, affix->entry_point);
            RETVAL = ptr2sv(aTHX_ p, affix->ret_info);
        } break;
        case AFFIX_TYPE_STD_STRING:
        default:
            //~ sv_dump(affix->ret_info);
            DD(affix->ret_info);
            PING;
            croak("Unknown return type: %s (%d)", type_as_str(affix->ret_type), affix->ret_type);
            break;
        }
    }
    /*
    #define AFFIX_TYPE_UTF8STR 18
    #define AFFIX_TYPE_CALLBACK 26
    #define AFFIX_TYPE_CPOINTER 28
    #define AFFIX_TYPE_VMARRAY 30
    #define AFFIX_TYPE_CUNION 42
    #define AFFIX_TYPE_CPPSTRUCT 36
    #define AFFIX_TYPE_WCHAR 46
    */
    //
    PING;
    //
    /* Free any memory that we need to. */
    if (free_strs != NULL) {
        for (int i = 0; UNLIKELY(i < num_strs); ++i)
            safefree(free_strs[i]);
        safefree(free_strs);
    }
    {
        for (int i = 0, p = 0; LIKELY(i < items); ++i) {
            PING;
            if (LIKELY(!SvREADONLY(ST(i)))) { // explicit undef
                switch (arg_types[i]) {
                case AFFIX_TYPE_CARRAY: {
                    SV *sv = ptr2sv(aTHX_ free_ptrs[p++], *av_fetch(affix->arg_info, i, 0));
                    if (SvFLAGS(ST(i)) & SVs_TEMP) { // likely a temp ref
                        AV *av = MUTABLE_AV(SvRV(sv));
                        av_clear(MUTABLE_AV(SvRV(ST(i))));
                        size_t av_len = av_count(av);
                        for (size_t q = 0; q < av_len; ++q) {
                            sv_setsv(*av_fetch(MUTABLE_AV(SvRV(ST(i))), q, 1), *av_fetch(av, q, 0));
                        }
                        SvSETMAGIC(SvRV(ST(i)));
                    }
                    else { // scalar ref is faster :D
                        SvSetMagicSV(ST(i), sv);
                    }

                } break;
                case AFFIX_TYPE_CPOINTER:
                case AFFIX_TYPE_REF: {
                    PING;
                    if (sv_derived_from((ST(i)), "Affix::Pointer")) {
                        ;
                        //~ warn("raw pointer");
                    }
                    else if (!SvREADONLY(ST(i))) {
                        //~ DD((ST(i)));
                        //~ DD(SvRV(ST(i)));
                        //~ PING;
#if TIE_MAGIC
                        if (SvOK(ST(i))) {
                            const MAGIC *mg = SvTIED_mg((SV *)SvRV(ST(i)), PERL_MAGIC_tied);
                            if (LIKELY(SvOK(ST(i)) && SvTYPE(SvRV(ST(i))) == SVt_PVHV && mg
                                       //~ &&  sv_derived_from(SvRV(ST(i)), "Affix::Union")
                                       )) { // Already a known union pointer
                            }
                            else {
                                sv_setsv_mg(ST(i), ptr2sv(aTHX_ free_ptrs[p++],
                                                          *av_fetch(affix->arg_info, i, 0)));
                            }
                        }
                        else {
                            sv_setsv_mg(ST(i), ptr2sv(aTHX_ free_ptrs[p++],
                                                      *av_fetch(affix->arg_info, i, 0)));
                        }
#else
                        sv_setsv_mg(ST(i),
                                    ptr2sv(aTHX_ free_ptrs[p++], *av_fetch(affix->arg_info, i, 0)));
#endif
                    }
                } break;
                }
            }
        }
    }

    PING;

    //~ if (UNLIKELY(free_ptrs != NULL)) {
    //~ for (int i = 0; LIKELY(i < num_ptrs); ++i) {
    //~ safefree(free_ptrs[i]);
    //~ }
    //~ safefree(free_ptrs);
    //~ }

    //~ if (agg_ != NULL) {
    //~ dcFreeAggr(agg_);
    //~ agg_ = NULL;
    //~ }
    PING;

    if (UNLIKELY(void_ret)) XSRETURN_EMPTY;
    PING;
    ST(0) = sv_2mortal(RETVAL);
    PING;

    XSRETURN(1);
}

XS_INTERNAL(Affix_affix) {
    dXSARGS;
    dXSI32;
    PING;
    if (items != 4) croak_xs_usage(cv, "$lib, $symbol, @arg_types, $ret_type");
    SV *RETVAL;
    Affix *ret;
    Newx(ret, 1, Affix);

    // Dumb defaults
    ret->num_args = 0;
    ret->arg_info = newAV();
    ret->call_conv = DC_SIGCHAR_CC_DEFAULT;
    ret->_cpp_struct = false;
    ret->_cpp_constructor = false;

    char *prototype = NULL;
    char *perl_name = NULL;
    SV *symbol;
    {
        if (UNLIKELY(SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV)) {
            AV *tmp = MUTABLE_AV(SvRV(ST(1)));
            size_t tmp_len = av_count(tmp);
            if (tmp_len != 2) { croak("Expected a symbol and name"); }
            if (ix == 1 && tmp_len > 1) {
                croak("wrap( ... ) isn't expecting a name and has ignored it");
            }
            symbol = *av_fetch(tmp, 0, false);
            if (!SvPOK(symbol)) { croak("Undefined symbol name"); }
            perl_name = SvPV_nolen(*av_fetch(tmp, 1, false));
        }
        else if (UNLIKELY(!SvPOK(ST(1)))) { croak("Undefined symbol name"); }
        else {
            symbol = ST(1);
            perl_name = SvPV_nolen(symbol);
        }

        STMT_START {
            cv =
                newXSproto_portable(ix == 0 ? perl_name : NULL, Affix_trigger, __FILE__, prototype);
            if (UNLIKELY(cv == NULL))
                croak("ARG! Something went really wrong while installing a new XSUB!");
            XSANY.any_ptr = (DCpointer)ret;
        }
        STMT_END;
        RETVAL =
            sv_bless((UNLIKELY(ix == 1) ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                     gv_stashpv("Affix", GV_ADD));
    }
    char *sym_name = SvPV_nolen(symbol);
    PING;
    // Support for Some::Some(), etc.
    if (instr(sym_name, "::") != NULL) { ret->_cpp_struct = true; }

    // TODO: Support C++ const member functions
    ret->_cpp_const = false;
    PING;

    {
        SV *lib, *ver;
        // affix($lib, ..., ..., ...)
        // affix([$lib, 1.2.0], ..., ..., ...)
        // wrap($lib, ..., ...., ...
        // wrap([$lib, 'v3'], ..., ...., ...
        {
            if (UNLIKELY(SvROK(ST(0)) && SvTYPE(SvRV(ST(0))) == SVt_PVAV)) {
                PING;

                AV *tmp = MUTABLE_AV(SvRV(ST(0)));
                size_t tmp_len = av_count(tmp);
                // Non-fatal
                if (UNLIKELY(!(tmp_len == 1 || tmp_len == 2))) {
                    PING;

                    warn("Expected a lib and version");
                }
                lib = *av_fetch(tmp, 0, false);
                ver = *av_fetch(tmp, 1, false);
            }
            else {
                PING;

                lib = newSVsv(ST(0));
                ver = newSV(0);
            }
            if (sv_isobject(lib) && sv_derived_from(lib, "Affix::Lib")) {
                {
                    PING;

                    IV tmp = SvIV(SvRV(lib));
                    ret->lib_handle = INT2PTR(DLLib *, tmp);
                }
                {
                    PING;

                    char *name;
                    Newxz(name, 1024, char);
                    int len = dlGetLibraryPath(ret->lib_handle, name, 1024);
                    Newxz(ret->lib_name, len + 1, char);
                    Copy(name, ret->lib_name, len, char);
                }
            }
            else {
                PING;

                ret->lib_name = locate_lib(aTHX_ lib, ver);
                PING;

                ret->lib_handle =
#if defined(DC__OS_Win64) || defined(DC__OS_MacOSX)
                    dlLoadLibrary(ret->lib_name);
#else
                    (DLLib *)dlopen(ret->lib_name, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
                if (!ret->lib_handle) { croak("Failed to load lib %s", dlerror()); }
#if 0
#if defined(DC__C_GNU) || defined(DC__C_CLANG)
                {
                    PING;

                    HV *cache = get_hv(form("Affix::Cache::Symbol::%s", ret->lib_name), GV_ADD);
                    PING;

                    DLSyms *syms = dlSymsInit(ret->lib_name);
                    PING;

                    int count = dlSymsCount(syms);
                    PING;

                    for (int i = 0; i < count; ++i) {
                    PING;

                        const char *symbolName = dlSymsName(syms, i);
                    PING;

                        if (strncmp(symbolName, "_Z", 2) != 0) {
                    PING;

                            (void)hv_store(cache, symbolName, strlen(symbolName),
                                           newSVpv(symbolName, 0), 1);
                                               PING;

                            continue;
                        }
                            PING;

                        int status = -1;
                            PING;

                        char *demangled_name = abi::__cxa_demangle(symbolName, NULL, NULL, &status);
                            PING;

                        if (!status) {
                            PING;

                            (void)hv_store(cache, symbolName, strlen(symbolName),
                                           newSVpv(demangled_name, 0), 1);
                                               PING;

                            //safefree(demangled_name);
                                PING;

                        }
                            PING;

                    }
                        PING;

                    dlSymsCleanup(syms);
                        PING;
                }
                    PING;
#endif
#endif
            }
            PING;
        }
        PING;

        //~ sv_dump(lib);
        //~ sv_dump(ver);

        // affix(..., ..., [Int], ...)
        // wrap(..., ..., [], ...)
        {
            STMT_START {
                PING;

                SV *const xsub_tmp_sv = ST(2);
                PING;

                SvGETMAGIC(xsub_tmp_sv);
                PING;
                if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV) {
                    PING;
                    AV *tmp_args = (AV *)SvRV(xsub_tmp_sv);
                    size_t args_len = av_count(tmp_args);
                    Newxz(ret->arg_types, args_len + 1, int16_t); // TODO: safefree
                    PING;

                    Newxz(prototype, args_len + 1, char);
                    SV *tmp_arg;
                    PING;

                    for (size_t i = 0; i < args_len; ++i) {
                        PING;

                        tmp_arg = *av_fetch(tmp_args, i, false);
                        if (LIKELY(SvROK(tmp_arg) &&
                                   sv_derived_from(tmp_arg, "Affix::Type::Base"))) {
                            PING;

                            ret->arg_types[i] = (int16_t)SvIV(tmp_arg);
                            if (UNLIKELY(sv_derived_from(tmp_arg, "Affix::Type::CC"))) {
                                av_store(ret->arg_info, i, newSVsv(tmp_arg));
                                //~ warn("av_store(ret->arg_info, %d, *tmp_arg);", i);
                                if (UNLIKELY(
                                        sv_derived_from(tmp_arg, "Affix::Type::CC::ELLIPSIS")) ||
                                    UNLIKELY(sv_derived_from(
                                        tmp_arg, "Affix::Type::CC::ELLIPSIS_VARARGS"))) {
                                    PING;

                                    prototype[i] = ';';
                                }
                            }
                            else {
                                PING;

                                ++ret->num_args;
                                prototype[i] = '$';
                                PING;
                            }
                            //~ warn("av_store(ret->arg_info, %d, tmp_arg);", i);
                            //~ DD(tmp_arg);
                            av_store(ret->arg_info, i, newSVsv(tmp_arg));
                            PING;
                        }
                        else { croak("Unexpected arg type in slot %ld", i + 1); }
                    }
                }
                else { croak("Expected a list of argument types as an array ref"); }
            }
            STMT_END;
            PING;
        }
        PING;

        // affix(..., $symbol, ..., ...)
        // affix(..., [$symbol, $name], ..., ...)
        // wrap(..., $symbol, ..., ...)
        {
            ret->entry_point = dlFindSymbol(ret->lib_handle, sym_name);
            if (ret->entry_point == NULL) {
                PING;
                ret->entry_point =
                    dlFindSymbol(ret->lib_handle, mangle(aTHX_ "Itanium", RETVAL, sym_name, ST(2)));
            }
            if (ret->entry_point == NULL) {
                ret->entry_point = dlFindSymbol(
                    ret->lib_handle, mangle(aTHX_ "Rust_legacy", RETVAL, sym_name, ST(2)));
            }
            // TODO: D and Swift
            if (ret->entry_point == NULL) { croak("Failed to find symbol named %s", sym_name); }
        }
        PING;

        // affix(..., ..., ..., $ret)
        // wrap(..., ..., ..., $ret)
        if (LIKELY(SvROK(ST(3)) && sv_derived_from(ST(3), "Affix::Type::Base"))) {
            PING;

            ret->ret_info = newSVsv(ST(3));
            ret->ret_type = SvIV(ST(3));
        }
        else { croak("Unknown return type"); }
    }
    PING;

    if (ret->_cpp_constructor) { ++ret->num_args; } // Expect Class->new(...)

#if DEBUG
    warn("lib: %p, entry_point: %p, as: %s, prototype: %s, ix: %d ", (DCpointer)ret->lib_handle,
         ret->entry_point, perl_name, prototype, ix);
#if DEBUG > 1
    DD(MUTABLE_SV(ret->arg_info));
    DD(ret->ret_info);
#endif
#endif
    /*
struct Affix {
    int16_t call_conv;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    void *entry_point;
    AV *arg_info;
    SV *ret_info;
    bool _cpp_const;
    bool _cpp_struct;
};
    */

    ST(0) = sv_2mortal(RETVAL);

    if (prototype) safefree(prototype);
    XSRETURN(1);
}
// Expose internals
#define AFFIX_METHOD_GUTS                                                                          \
    dXSARGS;                                                                                       \
    PERL_UNUSED_VAR(items);                                                                        \
    Affix *affix;                                                                                  \
    CV *THIS;                                                                                      \
    STMT_START {                                                                                   \
        HV *st;                                                                                    \
        GV *gvp;                                                                                   \
        SV *const xsub_tmp_sv = ST(0);                                                             \
        SvGETMAGIC(xsub_tmp_sv);                                                                   \
        THIS = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);                                                  \
        {                                                                                          \
            CV *cv = THIS;                                                                         \
            affix = (Affix *)XSANY.any_ptr;                                                        \
        }                                                                                          \
    }                                                                                              \
    STMT_END;

XS_INTERNAL(Affix_args) {
    AFFIX_METHOD_GUTS
    ST(0) = (MUTABLE_SV(affix->arg_info));
    XSRETURN(1);
}

XS_INTERNAL(Affix_retval) {
    AFFIX_METHOD_GUTS
    ST(0) = affix->ret_info;
    XSRETURN(1);
}

XS_INTERNAL(Affix_lib) {
    AFFIX_METHOD_GUTS
    SV *LIBSV = sv_newmortal();
    sv_setref_pv(LIBSV, "Affix::Lib", (DCpointer)affix->lib_handle);
    ST(0) = LIBSV;
    XSRETURN(1);
}

XS_INTERNAL(Affix_cpp_constructor) {
    AFFIX_METHOD_GUTS
    if (items == 2) affix->_cpp_constructor = SvTRUE(ST(1));
    ST(0) = newSVbool(affix->_cpp_constructor);
    XSRETURN(1);
}

XS_INTERNAL(Affix_cpp_const) {
    AFFIX_METHOD_GUTS
    ST(0) = newSVbool(affix->_cpp_const);
    XSRETURN(1);
}

XS_INTERNAL(Affix_cpp_struct) {
    AFFIX_METHOD_GUTS
    ST(0) = newSVbool(affix->_cpp_struct);
    XSRETURN(1);
}

XS_INTERNAL(Affix_DESTROY) {
    AFFIX_METHOD_GUTS
    /*
    struct Affix {
    int16_t call_conv;
    size_t num_args;
    int16_t *arg_types;
    int16_t ret_type;
    char *lib_name;
    DLLib *lib_handle;
    void *entry_point;
    AV *arg_info;
    SV *ret_info;
    };
            */
    //~ if (ptr->arg_types != NULL) {
    //~ safefree(ptr->arg_types);
    //~ ptr->arg_types = NULL;
    //~ }
    if (affix->lib_handle != NULL) {
        dlFreeLibrary(affix->lib_handle);
        affix->lib_handle = NULL;
    }

    //~ if(ptr->lib_name!=NULL){
    //~ Safefree(ptr->lib_name);
    //~ ptr->lib_name = NULL;
    //~ }

    //~ if(ptr->entry_point)
    //~ Safefree(ptr->entry_point);
    if (affix) { Safefree(affix); }

    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_END) { // cleanup
    dXSARGS;
    PERL_UNUSED_VAR(items);
    dMY_CXT;
    if (MY_CXT.cvm) dcFree(MY_CXT.cvm);
    XSRETURN_EMPTY;
}

// Utilities
XS_INTERNAL(Affix_sv_dump) {
    dVAR;
    dXSARGS;
    PING;

    if (items != 1) croak_xs_usage(cv, "sv");
    SV *sv = ST(0);
    sv_dump(sv);
    XSRETURN_EMPTY;
}

XS_EXTERNAL(boot_Affix) {
    dVAR;

    dXSBOOTARGSXSAPIVERCHK;
    PERL_UNUSED_VAR(items);

#ifdef USE_ITHREADS
    my_perl = (PerlInterpreter *)PERL_GET_CONTEXT;
#endif

    MY_CXT_INIT;

    // Allow user defined value in a BEGIN{ } block
    SV *vmsize = get_sv("Affix::VMSize", 0);
    MY_CXT.cvm = dcNewCallVM(vmsize == NULL ? 8192 : SvIV(vmsize));

    //
    cv = newXSproto_portable("Affix::affix", Affix_affix, __FILE__, "$$@$");
    XSANY.any_i32 = 0;
    export_function("Affix", "affix", "base");
    cv = newXSproto_portable("Affix::wrap", Affix_affix, __FILE__, "$$@$");
    XSANY.any_i32 = 1;
    export_function("Affix", "wrap", "base");

    (void)newXSproto_portable("Affix::args", Affix_args, __FILE__, "$");
    (void)newXSproto_portable("Affix::retval", Affix_retval, __FILE__, "$");
    (void)newXSproto_portable("Affix::lib", Affix_lib, __FILE__, "$");

    (void)newXSproto_portable("Affix::cpp_constructor", Affix_cpp_constructor, __FILE__, "$;$");
    (void)newXSproto_portable("Affix::cpp_const", Affix_cpp_const, __FILE__, "$");
    (void)newXSproto_portable("Affix::cpp_struct", Affix_cpp_struct, __FILE__, "$");

    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, __FILE__, "$");

    (void)newXSproto_portable("Affix::END", Affix_END, __FILE__, "");

    //~ (void)newXSproto_portable("Affix::CLONE", XS_Affix_CLONE, __FILE__, ";@");

    // Utilities
    (void)newXSproto_portable("Affix::sv_dump", Affix_sv_dump, __FILE__, "$");

    //~ export_function("Affix", "DEFAULT_ALIGNMENT", "vars");
    //~ export_constant("Affix", "ALIGNBYTES", "all", AFFIX_ALIGNBYTES);

    //
    boot_Affix_Aggregate(aTHX_ cv);
    boot_Affix_pin(aTHX_ cv);
    boot_Affix_Pointer(aTHX_ cv);
    boot_Affix_Lib(aTHX_ cv);
    boot_Affix_Type(aTHX_ cv);
    boot_Affix_Platform(aTHX_ cv);

    Perl_xs_boot_epilog(aTHX_ ax);
}
