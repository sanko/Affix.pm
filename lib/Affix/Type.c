#include "../Affix.h"

/**
 * @brief Callback handler for Perl CVs (code references) used in Affix callback types.
 *
 * This function is invoked by dyncall when a native (C) callback is triggered and the callback
 * is implemented as a Perl code reference (CV). It marshals the arguments from the native call,
 * invokes the Perl code reference with those arguments, and then marshals the return value
 * back to the native side.
 *
 * @param pcb      Pointer to the DCCallback structure (unused).
 * @param args     Pointer to the DCArgs structure containing the native arguments.
 * @param result   Pointer to the DCValue structure where the native return value should be stored.
 * @param userdata Pointer to an Affix_Type_Callback structure containing the Perl CV and type info.
 *
 * @return DCsigchar The dyncall signature character representing the return type.
 *
 * The function:
 *   - Extracts the Perl interpreter context and callback metadata.
 *   - Sets up the Perl stack and pushes arguments converted from native to Perl.
 *   - Calls the Perl code reference with the arguments.
 *   - Converts the Perl return value back to a native value and stores it in `result`.
 *   - Returns the dyncall signature character for the return type.
 *
 * If the Perl code reference is not valid, or if the return context is void but a value is returned,
 * the function will croak with an error so this might be a good place to wrap in a try/catch block or even eval.
 */
DCsigchar _handle_CV(DCCallback * pcb, DCArgs * args, DCValue * result, DCpointer userdata) {
    PERL_UNUSED_VAR(pcb);
    Affix_Type_Callback * cb = (Affix_Type_Callback *)userdata;
    dTHXa(cb->perl);
    SV * out = MUTABLE_SV(cb->cv);
    DCsigchar r = cb->ret->type;

    if (SvROK(out) && SvTYPE(SvRV(out)) == SVt_PVCV) {
        dSP;
        int count;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        EXTEND(SP, cb->arg_count);

        for (size_t i = 0; i < cb->arg_count; i++)
            PUSHs(cb->args[i]->cb_pass(aTHX_ cb->args[i], args));

        PUTBACK;
        count = call_sv((cb->cv), cb->cb_context);
        SPAGAIN;
        if (count == (cb->cb_context & G_VOID))
            croak("Big trouble\n");
        // TODO: Don't pop if we're expecting void context
        SV * ret = POPs;
        r = cb->ret->cb_call(aTHX_ cb->ret, result, ret);
        PUTBACK;
        FREETMPS;
        LEAVE;
    }
    return r;
}

/**
 * @brief Pushes a Reset operation onto the affix stack.
 *
 * This function is responsible for handling the logic required to push
 * a Reset operation within the affix system.
 *
 * @note Ensure that the affix stack is properly initialized before calling this function.
 */
AFFIX_PUSH(Reset) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    warn("*** Reset");
    dcReset(cvm);
    return 0;
}

/**
 * @brief Pushes an Aggregate initialization operation onto the affix stack.
 *
 * This function is responsible for handling the logic required to push
 * an Aggregate initialization operation within the affix system.
 *
 * @note Ensure that the affix stack is properly initialized before calling this function.
 */
AFFIX_PUSH(InitAggr) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    warn ("*** BeginCallAggr");
    dcBeginCallAggr(cvm, affix->pop->data.aggregate_type->ag);
    return 0;
}

/**
 * @brief Pushes a calling convention onto the affix stack.
 *
 * This function is responsible for handling the logic required to push
 * a calling convention operation within the affix system.
 *
 * @note Ensure that the affix stack is properly initialized before calling this function.
 */
AFFIX_PUSH(Default) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    warn("*** Default mode");
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_DEFAULT));
    return 0;
}

AFFIX_PUSH(This) {  //
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_THISCALL));
    // TODO: I should be checking for ST(st) being a pointer to and object of some sort and return 1
    return 0;
}
AFFIX_PUSH(Ellipsis) {  //
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_ELLIPSIS));
    // TODO: I should be looping through the rest of ST() and return the tally
    return 0;
}
AFFIX_PUSH(Varargs) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_ELLIPSIS_VARARGS));
    // TODO: I should be looping through the rest of ST() and return the tally
    return 0;
}
AFFIX_PUSH(CDecl) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_CDECL));
    return 0;
}
AFFIX_PUSH(Stdcall) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_STDCALL));
    return 0;
}
AFFIX_PUSH(FastcallMS) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_FASTCALL_MS));
    return 0;
}
AFFIX_PUSH(FastcallGNU) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_FASTCALL_GNU));
    return 0;
}
AFFIX_PUSH(ThisMS) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_THISCALL_MS));
    // TODO: I should be checking for ST(st) being a pointer to and object of some sort and return 1
    return 0;
}
AFFIX_PUSH(ThisGNU) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_THISCALL_GNU));
    // TODO: I should be checking for ST(st) being a pointer to and object of some sort and return 1
    return 0;
}
AFFIX_PUSH(Arm) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_ARM_ARM));
    return 0;
}
AFFIX_PUSH(Thumb) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_ARM_THUMB));
    return 0;
}
AFFIX_PUSH(Syscall) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    dcMode(cvm, dcGetModeFromCCSigChar(DC_SIGCHAR_CC_SYSCALL));
    return 0;
}

Affix_Type * _reset() {
    Affix_Type * ret = NULL;
    Newxz(ret, 1, Affix_Type);
    if (ret != NULL) {
        ret->type = RESET_FLAG;
        ret->pass = _pass_Reset;
    }
    return ret;
}
Affix_Type * _aggr() {
    Affix_Type * ret = NULL;
    Newxz(ret, 1, Affix_Type);
    if (ret != NULL) {
        // ret->type = STRUCT_FLAG;
        ret->pass = _pass_InitAggr;
    }
    return ret;
}
#define ASSIGN_TYPE_HANDLERS(Name)     \
    retval->fetch = _fetch_##Name;     \
    retval->store = _store_##Name;     \
    retval->pass = _pass_##Name;       \
    retval->call = _call_##Name;       \
    retval->cb_call = _cb_call_##Name; \
    retval->cb_pass = _cb_pass_##Name

#define ASSIGN_MODE_HANDLER(Name, Flag) \
                                        \
    retval->pass = _pass_##Name;        \
    retval->type = MODE_FLAG;

/**
 * @brief Clones an Affix_Type object, performing a deep copy of its nested structures.
 *
 * This function creates a new Affix_Type object that is a deep copy of the
 * original. It handles different types of Affix_Type (e.g., arrays, aggregates,
 * callbacks, pointers) and recursively clones their associated data.
 *
 * For Perl SVs (CVs, return values), their reference counts are incremented.
 *
 * For Dyncall aggregate (DCaggr) objects (STRUCT_FLAG/UNION_FLAG), a new
 * DCaggr object is created and fields are added to it.
 *
 * @param original A pointer to the original Affix_Type object to clone.
 * @return A pointer to the newly created, cloned Affix_Type object, or NULL if
 * the original was NULL or memory allocation failed.
 */


Affix_Type * clone_Affix_Type(pTHX_ const Affix_Type * original) {
    if (original == NULL) {
        return NULL;
    }

    Affix_Type * clone = (Affix_Type *)safemalloc(sizeof(Affix_Type));
    if (clone == NULL) {
        return NULL;  // Allocation failed
    }

    // Copy scalar members
    clone->type = original->type;
    clone->size = original->size;
    clone->align = original->align;
    clone->offset = original->offset;
    clone->store = original->store;
    clone->fetch = original->fetch;
    clone->call = original->call;
    clone->pass = original->pass;
    clone->cb_call = original->cb_call;
    clone->cb_pass = original->cb_pass;

    // Clone the union member based on the type
    switch (original->type) {
    case POINTER_FLAG:
        clone->data.pointer_type = clone_Affix_Type(aTHX_ original->data.pointer_type);
        break;
    case ARRAY_FLAG:  // Array
        if (original->data.array_type != NULL) {
            clone->data.array_type = (Affix_Type_Array *)safemalloc(sizeof(Affix_Type_Array));
            if (clone->data.array_type == NULL) {
                safefree(clone);
                return NULL;  // Allocation failed
            }
            clone->data.array_type->type = clone_Affix_Type(aTHX_ original->data.array_type->type);
            clone->data.array_type->length = original->data.array_type->length;
        }
        else
            clone->data.array_type = NULL;
        break;
    case STRUCT_FLAG:  // Aggregate (struct or hash)
    case UNION_FLAG:
        if (original->data.aggregate_type != NULL) {
            Newxz(clone->data.aggregate_type, 1, Affix_Type_Aggregate);
            if (clone->data.aggregate_type == NULL) {
                safefree(clone);
                return NULL;  // Allocation failed
            }
            clone->data.aggregate_type->field_count = original->data.aggregate_type->field_count;
            if (original->data.aggregate_type->fields != NULL) {
                Newxz(clone->data.aggregate_type->fields,
                      original->data.aggregate_type->field_count,
                      Affix_Type_Aggregate_Fields);
                if (clone->data.aggregate_type->fields == NULL) {
                    safefree(clone->data.aggregate_type);
                    safefree(clone);
                    return NULL;  // Allocation failed
                }
                for (size_t i = 0; i < original->data.aggregate_type->field_count; ++i) {
                    if (original->data.aggregate_type->fields[i].name != NULL) {
                        clone->data.aggregate_type->fields[i].name =
                            strdup(original->data.aggregate_type->fields[i].name);
                        if (clone->data.aggregate_type->fields[i].name == NULL) {
                            // Handle allocation failure, free previously allocated memory
                            for (size_t j = 0; j < i; ++j)
                                safefree(clone->data.aggregate_type->fields[j].name);
                            safefree(clone->data.aggregate_type->fields);
                            safefree(clone->data.aggregate_type);
                            safefree(clone);
                            return NULL;
                        }
                    }
                    else
                        clone->data.aggregate_type->fields[i].name = NULL;

                    clone->data.aggregate_type->fields[i].type =
                        clone_Affix_Type(aTHX_ original->data.aggregate_type->fields[i].type);
                }
            }
            else
                clone->data.aggregate_type->fields = NULL;
        }
        else
            clone->data.aggregate_type = NULL;

        break;
    case CV_FLAG:  // Callback
        if (original->data.callback_type != NULL) {
            Newxz(clone->data.callback_type, 1, Affix_Type_Callback);
            if (clone->data.callback_type == NULL) {
                safefree(clone);
                return NULL;  // Allocation failed
            }
            // Note: We are NOT cloning SV* cv, SV* retval, or dTHXfield(perl).
            // These are Perl-specific and their duplication might not be meaningful
            // or could lead to issues if not handled carefully within the Perl context.
            clone->data.callback_type->cv = original->data.callback_type->cv;
            clone->data.callback_type->arg_count = original->data.callback_type->arg_count;
            clone->data.callback_type->cb_context = original->data.callback_type->cb_context;
            if (original->data.callback_type->args != NULL && original->data.callback_type->arg_count > 0) {
                Newxz(clone->data.callback_type->args, original->data.callback_type->arg_count, Affix_Type *);
                if (clone->data.callback_type->args == NULL) {
                    safefree(clone->data.callback_type);
                    safefree(clone);
                    return NULL;  // Allocation failed
                }
                for (size_t i = 0; i < original->data.callback_type->arg_count; ++i) {
                    clone->data.callback_type->args[i] = clone_Affix_Type(aTHX_ original->data.callback_type->args[i]);
                }
            }
            else
                clone->data.callback_type->args = NULL;

            clone->data.callback_type->ret = clone_Affix_Type(aTHX_ original->data.callback_type->ret);
            clone->data.callback_type->retval = original->data.callback_type->retval;
            // clone->data.callback_type->perl = original->data.callback_type->perl; // dTHXfield - not directly
            // clonable
        }
        else
            clone->data.callback_type = NULL;

        break;
    default:
        // Handle other types if they exist in your system
        break;
    }
    return clone;
}

Affix_Type * xclone_Affix_Type(pTHX_ const Affix_Type * original) {
    if (original == NULL)
        return NULL;

    Affix_Type * clone = (Affix_Type *)safemalloc(sizeof(Affix_Type));
    if (clone == NULL)
        return NULL;  // Allocation failed

    // Copy base fields
    clone->type = original->type;
    clone->dc_type = original->dc_type;
    clone->size = original->size;
    clone->align = original->align;
    clone->offset = original->offset;

    // Copy function pointers
    clone->store = original->store;
    clone->fetch = original->fetch;
    clone->call = original->call;
    clone->pass = original->pass;
    clone->cb_call = original->cb_call;
    clone->cb_pass = original->cb_pass;

    // Clone the union member based on the type
    switch (original->type) {
    case POINTER_FLAG:
        clone->data.pointer_type = clone_Affix_Type(aTHX_ original->data.pointer_type);
        if (original->data.pointer_type && !clone->data.pointer_type) {
            safefree(clone);
            return NULL;
        }
        break;
    case ARRAY_FLAG:  // Array
        Newxz(clone->data.array_type, 1, Affix_Type_Array);
        if (clone->data.array_type == NULL) {
            safefree(clone);
            return NULL;
        }
        clone->data.array_type->length = original->data.array_type->length;
        clone->data.array_type->type = clone_Affix_Type(aTHX_ original->data.array_type->type);
        if (original->data.array_type->type && !clone->data.array_type->type) {
            safefree(clone->data.array_type);
            safefree(clone);
            return NULL;
        }
        break;
    case STRUCT_FLAG:
    case UNION_FLAG:
        Newxz(clone->data.aggregate_type, 1, Affix_Type_Aggregate);
        if (clone->data.aggregate_type == NULL) {
            safefree(clone);
            return NULL;
        }
        clone->data.aggregate_type->field_count = original->data.aggregate_type->field_count;

        if (original->data.aggregate_type->fields != NULL) {
            Newxz(clone->data.aggregate_type->fields,
                  original->data.aggregate_type->field_count,
                  Affix_Type_Aggregate_Fields);
            if (clone->data.aggregate_type->fields == NULL) {
                safefree(clone->data.aggregate_type);
                safefree(clone);
                return NULL;
            }
            clone->data.aggregate_type->ag = dcNewAggr(original->size, original->align);
            if (!clone->data.aggregate_type->ag) {
                safefree(clone->data.aggregate_type);
                safefree(clone);
                return NULL;
            }

            for (size_t i = 0; i < original->data.aggregate_type->field_count; ++i) {
                if (original->data.aggregate_type->fields[i].name != NULL) {
                    Newxz(clone->data.aggregate_type->fields[i].name,
                          strlen(original->data.aggregate_type->fields[i].name) + 1,
                          char);
                    if (clone->data.aggregate_type->fields[i].name == NULL) {
                        for (size_t j = 0; j < i; ++j) {
                            safefree(clone->data.aggregate_type->fields[j].name);
                            destroy_Affix_Type(aTHX_ clone->data.aggregate_type->fields[j].type);
                        }
                        dcFreeAggr(clone->data.aggregate_type->ag);
                        safefree(clone->data.aggregate_type->fields);
                        safefree(clone->data.aggregate_type);
                        safefree(clone);
                        return NULL;
                    }
                    clone->data.aggregate_type->fields[i].name = strdup(original->data.aggregate_type->fields[i].name);
                }
                else
                    clone->data.aggregate_type->fields[i].name = NULL;

                clone->data.aggregate_type->fields[i].type =
                    clone_Affix_Type(aTHX_ original->data.aggregate_type->fields[i].type);
                if (original->data.aggregate_type->fields[i].type && !clone->data.aggregate_type->fields[i].type) {
                    // Handle error: free current field's name and previous allocations
                    if (clone->data.aggregate_type->fields[i].name) {
                        safefree(clone->data.aggregate_type->fields[i].name);
                    }
                    for (size_t j = 0; j < i; ++j) {
                        safefree(clone->data.aggregate_type->fields[j].name);
                        destroy_Affix_Type(aTHX_ clone->data.aggregate_type->fields[j].type);
                    }
                    safefree(clone->data.aggregate_type->fields);
                    dcFreeAggr(clone->data.aggregate_type->ag);  // Free DCaggr
                    safefree(clone->data.aggregate_type);
                    safefree(clone);
                    return NULL;
                }

                // Add field to the new Dyncall aggregate object.
                // If the field type is an array, its `dc_type` will be the element's dyncall type,
                // and its `size` will be the total size of the array (element_size * count).
                // The `align` will be the element's alignment. This is correctly handled by dyncall's dcAggrField.
                if (clone->data.aggregate_type->fields[i].type) {
                    dcAggrField(clone->data.aggregate_type->ag,
                                clone->data.aggregate_type->fields[i].type->dc_type,
                                clone->data.aggregate_type->fields[i].type->size,
                                clone->data.aggregate_type->fields[i].type->align);
                    // TODO: Might need to handle array length
                }
            }
            dcCloseAggr(clone->data.aggregate_type->ag);
        }
        else
            clone->data.aggregate_type->fields = NULL;
        break;
    case CV_FLAG:  // Callback
        Newxz(clone->data.callback_type, 1, Affix_Type_Callback);
        if (clone->data.callback_type == NULL) {
            safefree(clone);
            return NULL;
        }
        clone->data.callback_type->arg_count = original->data.callback_type->arg_count;
        clone->data.callback_type->cb_context = original->data.callback_type->cb_context;
        storeTHX(clone->data.callback_type->perl);  // Copy Perl context if MULTIPLICITY is enabled

        // Note: We are NOT cloning SV* cv, SV* retval, or dTHXfield(perl).
        // These are Perl-specific and their duplication might not be meaningful
        // or could lead to issues if not handled carefully within the Perl context.
        clone->data.callback_type->cv = SvREFCNT_inc(original->data.callback_type->cv);
        if (original->data.callback_type->args != NULL && original->data.callback_type->arg_count > 0) {
            Newxz(clone->data.callback_type->args, original->data.callback_type->arg_count, Affix_Type *);
            if (clone->data.callback_type->args == NULL) {
                SvREFCNT_dec(clone->data.callback_type->cv);
                safefree(clone->data.callback_type);
                safefree(clone);
                return NULL;
            }
            for (size_t i = 0; i < original->data.callback_type->arg_count; ++i) {
                clone->data.callback_type->args[i] = clone_Affix_Type(aTHX_ original->data.callback_type->args[i]);
                if (original->data.callback_type->args[i] && !clone->data.callback_type->args[i]) {
                    // Cleanup if cloning an arg fails
                    for (size_t j = 0; j < i; ++j) {
                        destroy_Affix_Type(aTHX_ clone->data.callback_type->args[j]);
                    }
                    safefree(clone->data.callback_type->args);
                    if (clone->data.callback_type->cv)
                        SvREFCNT_dec(clone->data.callback_type->cv);
                    safefree(clone->data.callback_type);
                    safefree(clone);
                    return NULL;
                }
            }
        }
        else
            clone->data.callback_type->args = NULL;

        clone->data.callback_type->ret = clone_Affix_Type(aTHX_ original->data.callback_type->ret);
        if (original->data.callback_type->ret && !clone->data.callback_type->ret) {
            if (clone->data.callback_type->args) {
                for (size_t j = 0; j < original->data.callback_type->arg_count; ++j) {
                    destroy_Affix_Type(aTHX_ clone->data.callback_type->args[j]);
                }
                safefree(clone->data.callback_type->args);
            }
            if (clone->data.callback_type->cv)
                SvREFCNT_dec(clone->data.callback_type->cv);
            safefree(clone->data.callback_type);
            safefree(clone);
            return NULL;
        }

        // Increment reference count for the Perl return value SV
        if (original->data.callback_type->retval)
            clone->data.callback_type->retval = SvREFCNT_inc(original->data.callback_type->retval);
        else
            clone->data.callback_type->retval = NULL;
        break;
    default:
        // Handle other types if they exist in your system
        break;
    }
    return clone;
}
/**
 * @brief Destroys an affix type object and frees associated memory.
 *
 * This function releases all resources allocated for the specified affix type.
 * After calling this function, the affix type pointer should not be used.
 *
 * @param affix_type Pointer to the affix type object to be destroyed.
 */
void destroy_Affix_Type(pTHX_ Affix_Type * type) {
    if (type == NULL)
        return;
    switch (type->type) {
    case POINTER_FLAG:
        if (type->data.pointer_type) {
            destroy_Affix_Type(aTHX_ type->data.pointer_type);
            type->data.pointer_type = NULL;
        }
        break;
    case ARRAY_FLAG:
        if (type->data.array_type) {
            if (type->data.array_type->type)
                destroy_Affix_Type(aTHX_ type->data.array_type->type);
            safefree(type->data.array_type);
            type->data.array_type = NULL;
        }
        break;
    case STRUCT_FLAG:  // TODO: Add other aggregates
    case UNION_FLAG:
        if (type->data.aggregate_type) {
            if (type->data.aggregate_type->ag) {
                dcFreeAggr(type->data.aggregate_type->ag);  // Correctly free DCaggr using dcFreeAggr
                type->data.aggregate_type->ag = NULL;
            }
            if (type->data.aggregate_type->fields) {
                // Iterate through fields, freeing their names and recursively destroying their types.
                for (size_t i = 0; i < type->data.aggregate_type->field_count; ++i) {
                    if (type->data.aggregate_type->fields[i].name) {
                        safefree(type->data.aggregate_type->fields[i].name);
                        type->data.aggregate_type->fields[i].name = NULL;
                    }
                    // Recursively destroy field type only if it's not NULL.
                    if (type->data.aggregate_type->fields[i].type) {
                        destroy_Affix_Type(aTHX_ type->data.aggregate_type->fields[i].type);
                    }
                }
                safefree(type->data.aggregate_type->fields);  // Free the array of field structs
                type->data.aggregate_type->fields = NULL;
            }
            safefree(type->data.aggregate_type);  // Free the aggregate struct itself
            type->data.aggregate_type = NULL;
        }
        break;
    case CV_FLAG:
        // If it's a callback type, decrement Perl SV ref counts, destroy argument types, and return type.
        if (type->data.callback_type != NULL) {
            if (type->data.callback_type->cv != NULL) {
                SvREFCNT_dec(type->data.callback_type->cv);  // Decrement Perl CV reference count
                type->data.callback_type->cv = NULL;
            }
            if (type->data.callback_type->args != NULL) {
                // Destroy each argument type in the array
                for (size_t i = 0; i < type->data.callback_type->arg_count; ++i) {
                    // Recursively destroy arg type only if it's not NULL.
                    if (type->data.callback_type->args[i]) {
                        destroy_Affix_Type(aTHX_ type->data.callback_type->args[i]);
                    }
                }
                dcFreeMem(type->data.callback_type->args);  // Free the array of arg type pointers
                type->data.callback_type->args = NULL;
            }
            if (type->data.callback_type->ret != NULL) {
                destroy_Affix_Type(aTHX_ type->data.callback_type->ret);  // Recursively destroy return type
                type->data.callback_type->ret = NULL;
            }
            if (type->data.callback_type->retval != NULL) {
                // Only decrement if it's not a known global constant SV
                // These typically have SvREFCNT_max set or are not
                // meant for explicit decrementing from C unless *this*
                // module specifically incremented them.
                // This is a common source of double-free if `retval` points
                // to &PL_sv_no, &PL_sv_yes, &PL_sv_undef, etc.
                if (type->data.callback_type->retval != &PL_sv_no && type->data.callback_type->retval != &PL_sv_yes &&
                    type->data.callback_type->retval != &PL_sv_undef) {
                    SvREFCNT_dec(type->data.callback_type->retval);  // Decrement Perl SV reference count
                }
                type->data.callback_type->retval = NULL;  // Nullify pointer after potential free
            }
            dcFreeMem(type->data.callback_type);  // Free the callback struct itself
            type->data.callback_type = NULL;
        }
        break;
    default:  // TODO: ???
        break;
    }
    safefree(type);
    type = NULL;
}

POINTER_STORE(Void) {
    if (SvOK(data)) {
        SV * const xsub_tmp_sv = data;
        SvGETMAGIC(xsub_tmp_sv);
        if ((SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV &&
             sv_derived_from(xsub_tmp_sv, "Affix::Pointer"))) {
            croak("Wrong!");
            //~ SV * ptr_sv = AXT_POINTER_ADDR(xsub_tmp_sv);
            //~ if (SvOK(ptr_sv)) {
            //~ IV tmp = SvIV(MUTABLE_SV(SvRV(ptr_sv)));
            //~ target = INT2PTR(DCpointer, tmp);
            //~ }
        }
        else if (SvTYPE(data) != SVt_NULL) {
            size_t len = 0;
            DCpointer ptr_ = SvPVbyte(data, len);
            if (*target == NULL)
                Newxz(*target, len, char);
            Copy(ptr_, *target, len, char);
        }
        else
            croak("Data type mismatch for %c [%d]", type->type, SvTYPE(data));
    }
    else
        *target = NULL;
    //~
    return;
}

POINTER_FETCH(Void) {
    PERL_UNUSED_VAR(type);
    // TODO: If already an Affix::Pointer, update the address (if different)
    if (sv == NULL)
        sv = newSV(0);
    Affix_Pointer * pointer = NULL;
    Newxz(pointer, 1, Affix_Pointer);
    if (pointer == NULL)
        croak("Falled to allocate pointer");
    pointer->address = data;
    pointer->count = 1;
    if (!SvROK(sv))
        sv = newRV(sv);
    sv_setref_pv(sv, "Affix::Pointer", (DCpointer)pointer);
    return sv;
}

AFFIX_PUSH(Void) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    PERL_UNUSED_VAR(cvm);
    return 1; /* no-op */
}

AFFIX_CALL(Void) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    //~ PING;
    dcCallVoid(cvm, entrypoint);
    //~ PING;
    return NULL;
}

CALLBACK_CALL(Void) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    //~ PING;
    croak("Incomplete");
}
CALLBACK_PUSH(Void) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    // XXX - ...uh, skip?
    return newSV(0);
}

#define XXX_TO_POINTER(Name, ctype, toc, size)                              \
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {                    \
        AV * av = MUTABLE_AV(SvRV(data));                                   \
        size_t len = av_count(av);                                          \
        ctype * ret = (ctype *)safecalloc(len, size);                       \
        SV ** element = NULL;                                               \
        for (size_t i = 0; i < len; i++) {                                  \
            element = av_fetch(av, i, 0);                                   \
            if (element != NULL) {                                          \
                ret[i] = toc(*element);                                     \
                pin(aTHX_ clone_Affix_Type(aTHX_ type), *element, &ret[i]); \
            }                                                               \
        }                                                                   \
        if (*target == NULL)                                                \
            Newxz(*target, len, ctype);                                     \
        Move(ret, *target, len, ctype);                                     \
        return;                                                             \
    }                                                                       \
    ctype value = toc(data);                                                \
    if (*target == NULL)                                                    \
        Newxz(*target, 1, ctype);                                           \
    Copy(&value, *target, 1, ctype);                                        \
    pin(aTHX_ clone_Affix_Type(aTHX_ type), data, *target);                 \
    return;

#define X_TO_POINTER(Name, ctype, toc, size)                                \
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {                    \
        AV * av = MUTABLE_AV(SvRV(data));                                   \
        size_t len = av_count(av);                                          \
        ctype * ret = (ctype *)safecalloc(len, size);                       \
        SV ** element = NULL;                                               \
        for (size_t i = 0; i < len; i++) {                                  \
            element = av_fetch(av, i, 0);                                   \
            if (element != NULL) {                                          \
                ret[i] = toc(*element);                                     \
                pin(aTHX_ clone_Affix_Type(aTHX_ type), *element, &ret[i]); \
            }                                                               \
        }                                                                   \
        if (*target == NULL)                                                \
            Newxz(*target, len, ctype);                                     \
        Move(ret, *target, len, ctype);                                     \
        return;                                                             \
    }                                                                       \
    ctype value = toc(data);                                                \
    if (*target == NULL)                                                    \
        Newxz(*target, 1, ctype);                                           \
    Copy(&value, *target, 1, ctype);                                        \
    pin(aTHX_ clone_Affix_Type(aTHX_ type), data, *target);                 \
    return;


//~ #define X_TO_POINTER(Name, ctype, toc, size)                                            \
   //~ if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {                                \
       //~ PING;                                                                           \
       //~ warn("array");                                                                  \
       //~ AV * av = MUTABLE_AV(SvRV(data));                                               \
       //~ size_t len = av_count(av);                                                      \
       //~ size_t offset = 0;                                                              \
       //~ if (*target == NULL)                                                            \
           //~ Newxz(*target, len, ctype);                                                 \
       //~ SV ** element = NULL;                                                           \
       //~ ctype tmp;                                                                      \
       //~ for (size_t i = 0; i < len; i++) {                                              \
           //~ element = av_fetch(av, i, 0);                                               \
           //~ if (element != NULL) {                                                      \
               //~ sv_dump(*element);                                                      \
               //~ tmp = toc(*element);                                                    \
               //~ warn("offset: %llu or %llu or %p",                                      \
                    //~ size * i,                                                          \
                    //~ PTR2IV(*target) + (size * i),                                      \
                    //~ INT2PTR(DCpointer, PTR2IV(*target) + (size * i)));                 \
               //~ Copy(&tmp, INT2PTR(DCpointer, PTR2IV(*target) + (size * i)), 1, ctype); \
               //~ DumpHex(*target, size * len); /*ret[i] = toc(*element); */              \
               //~ /*pin(aTHX_ clone_Affix_Type(aTHX_ type), *element, &ret[i]);*/              \
           //~ }                                                                           \
       //~ }                                                                               \
       //~ return;                                                                         \
   //~ }                                                                                   \
   //~ PING;                                                                               \
   //~ warn("not an array");                                                               \
   //~ ctype value = toc(data);                                                            \
   //~ if (*target == NULL)                                                                \
       //~ Newxz(*target, 1, ctype);                                                       \
   //~ Copy(&value, *target, 1, ctype);                                                    \
   //~ pin(aTHX_ clone_Affix_Type(aTHX_ type), data, *target);
//~ return;


//
POINTER_STORE(Bool) {
    X_TO_POINTER(Bool, bool, SvTRUE, SIZEOF_BOOL);
    return;
}
POINTER_FETCH(Bool) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    // warn("Here! Getting bool value from %p", data);
    // DumpHex(data, sizeof(bool));
    return newSVbool(*(bool *)data);
}

AFFIX_PUSH(Bool) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgBool(cvm, (bool)SvTRUE(ST(st)));
    return 1;
}
AFFIX_CALL(Bool) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVbool((bool)dcCallBool(cvm, entrypoint));
}
CALLBACK_CALL(Bool) {
    PERL_UNUSED_VAR(type);
    result->B = SvTRUE(sv);
    return 'B';
}
CALLBACK_PUSH(Bool) {
    PERL_UNUSED_VAR(type);
    return newSVbool((bool)dcbArgBool(args));
}

POINTER_STORE(SChar) {
    PERL_UNUSED_VAR(type);
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(data));
        size_t len = av_len(av);
        signed char * ret = (signed char *)safecalloc(len, SIZEOF_SCHAR);
        SV ** element = NULL;
        for (size_t i = 0; i < len; i++) {
            element = av_fetch(av, i, 0);
            if (element != NULL)
                ret[i] = SvIOK(*element) ? SvIV(*element) : SvPVbyte_nolen(*element)[0];
        }
        Move(ret, *target, len, signed char);
        return;
    }
    STRLEN len;
    signed char * ret = SvPVbyte_or_null(data, len);
    Newxz(*target, len + 1, signed char);
    Copy(ret, *target, len, signed char);
    return;
}
POINTER_FETCH(SChar) {
    PERL_UNUSED_VAR(sv);
    PERL_UNUSED_VAR(type);
    if (data == NULL)
        return newSV(0);
    return newSVpv((char *)data, 0);
}
AFFIX_PUSH(SChar) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    if (LIKELY(SvIOK(sv))) {
        dcArgChar(cvm, (signed char)SvIV(sv));
        return 1;
    }
    if (SvPOK(sv)) {
        signed char * str = SvPVbytex_nolen(sv);
        dcArgChar(cvm, (signed char)str[0]);
        return 1;
    }
    return 0; /* TODO: Throw something */
}
AFFIX_CALL(SChar) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    signed char value[1];
    value[0] = (signed char)dcCallChar(cvm, entrypoint);
    if (value[0] < 0)  // Perl chokes on negative chars
        return newSViv((IV)value[0]);
    SV * sv = newSV(0);
    sv_setsv(sv, newSVpv((char *)value, 1));
    (void)SvUPGRADE(sv, SVt_PVIV);
    SvIV_set(sv, ((IV)value[0]));
    SvIandPOK_on(sv);
    return sv;
}
CALLBACK_CALL(SChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
}
CALLBACK_PUSH(SChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete: pass SChar to callback");
}
///
POINTER_STORE(UChar) {
    PERL_UNUSED_VAR(type);
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(data));
        size_t len = av_len(av);
        unsigned char * ret = (unsigned char *)safecalloc(len, SIZEOF_SCHAR);
        SV ** element = NULL;
        for (size_t i = 0; i < len; i++) {
            element = av_fetch(av, i, 0);
            if (element != NULL)
                ret[i] = SvUOK(*element) ? SvUV(*element) : SvPVbyte_nolen(*element)[0];
        }
        Move(ret, *target, len, unsigned char);
        return;
    }
    STRLEN len;
    unsigned char * ret = (unsigned char *)SvPVbyte_or_null(data, len);
    Newxz(*target, len + 1, unsigned char);
    Copy(ret, *target, len, unsigned char);
    return;
}
POINTER_FETCH(UChar) {
    PERL_UNUSED_VAR(sv);
    PERL_UNUSED_VAR(type);
    if (data == NULL)
        return newSV(0);
    return newSVpv((unsigned char *)data, 0);
}
AFFIX_PUSH(UChar) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    if (LIKELY(SvIOK(sv))) {
        dcArgChar(cvm, (unsigned char)SvUV(sv));
        return 1;
    }
    if (SvPOK(sv)) {
        unsigned char * str = SvPVbytex_nolen(sv);
        dcArgChar(cvm, (unsigned char)str[0]);
        return 1;
    }
    return 0; /* TODO: Throw something */
}
AFFIX_CALL(UChar) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    unsigned char value[1];
    value[0] = (unsigned char)dcCallChar(cvm, entrypoint);
    SV * sv = newSV(0);
    sv_setsv(sv, newSVpv((char *)value, 1));
    (void)SvUPGRADE(sv, SVt_PVIV);
    SvIV_set(sv, ((UV)value[0]));
    SvIandPOK_on(sv);
    return sv;
}
CALLBACK_CALL(UChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
}
CALLBACK_PUSH(UChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete: pass UChar to callback");
}

#if CHAR_MIN < 0
void (*_store_Char)(pTHX_ Affix_Type *, SV *, DCpointer *) = _store_SChar;
SV * (*_fetch_Char)(pTHX_ Affix_Type *, DCpointer, SV *) = _fetch_SChar;
size_t (*_pass_Char)(pTHX_ Affix *, Affix_Type *, DCCallVM *, Stack_off_t) = _pass_SChar;
SV * (*_call_Char)(pTHX_ Affix *, Affix_Type *, DCCallVM *, DCpointer) = _call_SChar;
DCsigchar (*_cb_call_Char)(pTHX_ Affix_Type *, DCValue *, SV *) = _cb_call_SChar;
SV * (*_cb_pass_Char)(pTHX_ Affix_Type *, DCArgs *) = _cb_pass_SChar;
#else
void (*_store_Char)(pTHX_ Affix_Type *, SV *, DCpointer *) = _store_UChar;
SV * (*_fetch_Char)(pTHX_ Affix_Type *, DCpointer, SV *) = _fetch_UChar;
size_t (*_pass_Char)(pTHX_ Affix *, Affix_Type *, DCCallVM *, Stack_off_t) = _pass_UChar;
SV * (*_call_Char)(pTHX_ Affix *, Affix_Type *, DCCallVM *, DCpointer) = _call_UChar;
DCsigchar (*_cb_call_Char)(pTHX_ Affix_Type *, DCValue *, SV *) = _cb_call_UChar;
SV * (*_cb_pass_Char)(pTHX_ Affix_Type *, DCArgs *) = _cb_pass_UChar;
#endif

POINTER_STORE(WChar) {
    PERL_UNUSED_VAR(type);
    STRLEN len;  // We're gonna cheat here, so get ready...
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(data));
        size_t a_len = av_len(av);
        wchar_t *ret = (wchar_t *)safecalloc(a_len + 1, SIZEOF_WCHAR), *tmp;
        SV ** element = NULL;
        for (size_t i = 0; i <= a_len; i++) {
            element = av_fetch(av, i, 0);
            if (element != NULL) {
                if (SvIOK(*element))
                    ret[i] = SvIV(*element);
                else {
                    (void)SvPVutf8x(*element, len);
                    tmp = utf2wchar(aTHX_ * element, len);
                    ret[i] = tmp[0];
                }
            }
        }
        Copy(ret, *target, len, wchar_t);
        return;
    }
    (void)SvPVutf8x(data, len);
    *target = utf2wchar(aTHX_ data, len);
    return;
}

POINTER_FETCH(WChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    PERL_UNUSED_VAR(data);
    PING;
    croak("Incomplete!");
    return NULL;
}
AFFIX_PUSH(WChar) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    if (SvOK(sv)) {
        wchar_t * str = utf2wchar(aTHX_ sv, 1);
#if WCHAR_MAX == LONG_MAX
        // dcArgLong(cvm, str[0]);
#elif WCHAR_MAX == INT_MAX
        // dcArgInt(cvm, str[0]);
#elif WCHAR_MAX == SHORT_MAX
        // dcArgShort(cvm, str[0]);
#else
        // dcArgChar(cvm, str[0]);
#endif
        dcArgLongLong(cvm, str[0]);
        safefree(str);
    }
    else
        dcArgLongLong(cvm, 0);
    return 1;
}
AFFIX_CALL(WChar) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    wchar_t value[1];
    value[0] = (wchar_t)dcCallLongLong(cvm, entrypoint);
    SV * sv = wchar2utf(aTHX_ value, 1);
    (void)SvUPGRADE(sv, SVt_PVIV);
    SvIV_set(sv, ((IV)value[0]));
    SvIandPOK_on(sv);
    return sv;
}
CALLBACK_CALL(WChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
}
CALLBACK_PUSH(WChar) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete");
    return NULL;
}

//
POINTER_STORE(Short) {
    X_TO_POINTER(Short, short, SvIV, SIZEOF_SHORT);
}
POINTER_FETCH(Short) {
    PERL_UNUSED_VAR(sv);
    PERL_UNUSED_VAR(type);
    return newSViv(*(short *)data);
}
AFFIX_PUSH(Short) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgShort(cvm, (short)SvIV(ST(st)));
    return 1;
}
AFFIX_CALL(Short) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSViv((short)dcCallShort(cvm, entrypoint));
}
CALLBACK_CALL(Short) {
    PERL_UNUSED_VAR(type);
    result->s = SvIV(sv);
    return 's';
}
CALLBACK_PUSH(Short) {
    PERL_UNUSED_VAR(type);
    return newSViv((short)dcbArgShort(args));
}

//
POINTER_STORE(UShort) {
    X_TO_POINTER(UShort, unsigned short, SvUV, SIZEOF_USHORT);
}
POINTER_FETCH(UShort) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVuv(*(unsigned short *)data);
}
AFFIX_PUSH(UShort) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgShort(cvm, (unsigned short)SvUV(ST(st)));
    return 1;
}
AFFIX_CALL(UShort) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned short)dcCallShort(cvm, entrypoint));
}
CALLBACK_CALL(UShort) {
    PERL_UNUSED_VAR(type);
    result->S = SvUV(sv);
    return 'S';
}
CALLBACK_PUSH(UShort) {
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned short)dcbArgShort(args));
}

//
POINTER_STORE(Int) {
    X_TO_POINTER(Int, int, SvIV, SIZEOF_INT);
}
POINTER_FETCH(Int) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSViv(*(int *)data);
}
AFFIX_PUSH(Int) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgInt(cvm, (int)SvIV(ST(st)));
    return 1;
}
AFFIX_CALL(Int) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSViv((int)dcCallInt(cvm, entrypoint));
}
CALLBACK_CALL(Int) {
    PERL_UNUSED_VAR(type);
    result->i = SvIV(sv);
    return 'i';
}
CALLBACK_PUSH(Int) {
    PERL_UNUSED_VAR(type);
    return newSViv((int)dcbArgInt(args));
}

//
POINTER_STORE(UInt) {
    X_TO_POINTER(UInt, unsigned int, SvUV, SIZEOF_UINT);
}
POINTER_FETCH(UInt) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVuv(*(unsigned int *)data);
}
AFFIX_PUSH(UInt) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgInt(cvm, (unsigned int)SvUV(ST(st)));
    return 1;
}
AFFIX_CALL(UInt) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned int)dcCallInt(cvm, entrypoint));
}
CALLBACK_CALL(UInt) {
    PERL_UNUSED_VAR(type);
    result->I = SvUV(sv);
    return 'I';
}
CALLBACK_PUSH(UInt) {
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned int)dcbArgInt(args));
}

//
POINTER_STORE(Long) {
    X_TO_POINTER(Long, long, SvIV, SIZEOF_LONG);
}
POINTER_FETCH(Long) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSViv(*(long *)data);
}
AFFIX_PUSH(Long) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgLong(cvm, (long)SvIV(ST(st)));
    return 1;
}
AFFIX_CALL(Long) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSViv((long)dcCallLong(cvm, entrypoint));
}
CALLBACK_CALL(Long) {
    PERL_UNUSED_VAR(type);
    result->j = SvIV(sv);
    return 'j';
}
CALLBACK_PUSH(Long) {
    PERL_UNUSED_VAR(type);
    return newSViv((long)dcbArgLong(args));
}

//
POINTER_STORE(ULong) {
    X_TO_POINTER(ULong, unsigned long, SvUV, SIZEOF_ULONG);
}
POINTER_FETCH(ULong) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVuv(*(unsigned long *)data);
}
AFFIX_PUSH(ULong) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgLong(cvm, (unsigned long)SvUV(ST(st)));
    return 1;
}
AFFIX_CALL(ULong) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned long)dcCallLong(cvm, entrypoint));
}
CALLBACK_CALL(ULong) {
    PERL_UNUSED_VAR(type);
    result->J = SvUV(sv);
    return 'J';
}
CALLBACK_PUSH(ULong) {
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned long)dcbArgLong(args));
}

//
POINTER_STORE(LongLong) {
    X_TO_POINTER(LongLong, long long, SvIV, SIZEOF_LONGLONG);
}
POINTER_FETCH(LongLong) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSViv(*(long long *)data);
}
AFFIX_PUSH(LongLong) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgLongLong(cvm, (long long)SvIV(ST(st)));
    return 1;
}
AFFIX_CALL(LongLong) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSViv((long long)dcCallLongLong(cvm, entrypoint));
}
CALLBACK_CALL(LongLong) {
    PERL_UNUSED_VAR(type);
    result->l = SvIV(sv);
    return 'l';
}
CALLBACK_PUSH(LongLong) {
    PERL_UNUSED_VAR(type);
    return newSViv((long long)dcbArgLongLong(args));
}

//
POINTER_STORE(ULongLong) {
    X_TO_POINTER(ULongLong, unsigned long long, SvUV, SIZEOF_ULONGLONG);
}
POINTER_FETCH(ULongLong) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVuv(*(unsigned long long *)data);
}
AFFIX_PUSH(ULongLong) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgLongLong(cvm, (unsigned long long)SvUV(ST(st)));
    return 1;
}
AFFIX_CALL(ULongLong) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned long long)dcCallLongLong(cvm, entrypoint));
}
CALLBACK_CALL(ULongLong) {
    PERL_UNUSED_VAR(type);
    result->L = SvUV(sv);
    return 'L';
}
CALLBACK_PUSH(ULongLong) {
    PERL_UNUSED_VAR(type);
    return newSVuv((unsigned long long)dcbArgLongLong(args));
}

//
POINTER_STORE(Float) {
    X_TO_POINTER(Float, float, SvNV, SIZEOF_FLOAT);
}
POINTER_FETCH(Float) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVnv(*(float *)data);
}
AFFIX_PUSH(Float) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgFloat(cvm, (float)SvNV(ST(st)));
    return 1;
}
AFFIX_CALL(Float) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVnv((float)dcCallFloat(cvm, entrypoint));
}
CALLBACK_CALL(Float) {
    PERL_UNUSED_VAR(type);
    result->f = SvNV(sv);
    return 'f';
}
CALLBACK_PUSH(Float) {
    PERL_UNUSED_VAR(type);
    return newSVnv((float)dcbArgFloat(args));
}

//
POINTER_STORE(Double) {
    X_TO_POINTER(Double, double, SvNV, SIZEOF_DOUBLE);
}
POINTER_FETCH(Double) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVnv(*(double *)data);
}
AFFIX_PUSH(Double) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    dcArgDouble(cvm, (double)SvNV(ST(st)));
    return 1;
}
AFFIX_CALL(Double) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVnv((double)dcCallDouble(cvm, entrypoint));
}
CALLBACK_CALL(Double) {
    PERL_UNUSED_VAR(type);
    result->d = SvNV(sv);
    return 'd';
}
CALLBACK_PUSH(Double) {
    PERL_UNUSED_VAR(type);
    return newSVnv((double)dcbArgDouble(args));
}

//
POINTER_STORE(Size_t) {
    X_TO_POINTER(Size_t, size_t, SvUV, SIZEOF_SIZE_T);
}
POINTER_FETCH(Size_t) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVuv(*(size_t *)data);
}
AFFIX_PUSH(Size_t) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
#if SIZE_MAX == UINT32_MAX
    dcArgInt
#elif SIZE_MAX == UINT64_MAX
    dcArgLong
#else  // Something's not right
    dcArgLongLong
#endif
        (cvm, (size_t)SvUV(ST(st)));
    return 1;
}
AFFIX_CALL(Size_t) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    return newSVuv((size_t)
#if SIZE_MAX == UINT32_MAX
                       dcCallInt
#elif SIZE_MAX == UINT64_MAX
                       dcCallLong
#else  // Something's not right
                       dcCallLongLong
#endif
                   (cvm, entrypoint));
}
CALLBACK_CALL(Size_t) {
    PERL_UNUSED_VAR(type);
#if SIZE_MAX == UINT32_MAX
    result->I = SvUV(sv);
    return 'I';
#elif SIZE_MAX == UINT64_MAX
    result->J = SvUV(sv);
    return 'J';
#else  // Something's not right
    result->L = SvUV(sv);
    return 'L';
#endif
}
CALLBACK_PUSH(Size_t) {
    PERL_UNUSED_VAR(type);
    return newSVuv((size_t)
#if SIZE_MAX == UINT32_MAX
                       dcbArgInt
#elif SIZE_MAX == UINT64_MAX
                       dcbArgLong
#else  // Something's not right
                       dcbArgLongLong
#endif
                   (args));
}

//
POINTER_STORE(SSize_t) {
    X_TO_POINTER(SSize_t, ssize_t, SvIV, SIZEOF_SSIZE_T);
}
POINTER_FETCH(SSize_t) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    return newSVuv(*(ssize_t *)data);
}
AFFIX_PUSH(SSize_t) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
#if SSIZE_MAX == INT32_MAX
    dcArgInt
#elif SSIZE_MAX == INT64_MAX
    dcArgLong
#else  // Something's not right
    dcArgLongLong
#endif
        (cvm, (ssize_t)SvIV(ST(st)));
    return 1;
}
AFFIX_CALL(SSize_t) {
    PERL_UNUSED_VAR(type);
    return newSViv((ssize_t)
#if SIZE_MAX == UINT32_MAX
                       dcCallInt
#elif SIZE_MAX == UINT64_MAX
                       dcCallLong
#else  // Something's not right
                       dcCallLongLong
#endif
                   (cvm, entrypoint));
}
CALLBACK_CALL(SSize_t) {
    PERL_UNUSED_VAR(type);
#if SSIZE_MAX == INT32_MAX
    result->i = SvIV(sv);
    return 'i';
#elif SSIZE_MAX == INT64_MAX
    result->j = SvIV(sv);
    return 'j';
#else  // Something's not right
    result->l = SvIV(sv);
    return 'l';
#endif
}
CALLBACK_PUSH(SSize_t) {
    PERL_UNUSED_VAR(type);
    return newSViv((ssize_t)
#if SSIZE_MAX == INT32_MAX
                       dcbArgInt
#elif SSIZE_MAX == INT64_MAX
                       dcbArgLong
#else  // Something's not right
                       dcbArgLongLong
#endif
                   (args));
}

/*
POINTER_STORE(Struct) {
    PERL_UNUSED_VAR(type);
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(data));
        SV ** element = NULL;
        size_t a_len = av_len(av);
        if (*target == NULL)
            *target = safecalloc(a_len, type->size);
        IV position = PTR2IV(*target);
        for (size_t i = 0; i < a_len; i++) {
            element = av_fetch(av, i, 0);
            if (element != NULL) {
                DCpointer ptr = INT2PTR(DCpointer, position + (i * type->size));
                type->store(aTHX_ type, *element, &ptr);
            }
        }
        return;
    }
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVHV) {
        SV * value = NULL;
        HV * hv_data = NULL;
        if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVHV)
            hv_data = MUTABLE_HV(SvRV(data));
        else
            croak("Expected a hash reference for struct/union store, got something else");
        if (*target == NULL)
            *target = safecalloc(1, type->size);
        IV position = PTR2IV(*target);
        for (size_t i = 0; i < type->data.aggregate_type->field_count; i++) {
            value = hv_existsor(hv_data, type->data.aggregate_type->fields[i].name, NULL);

            // We store the address if it's a pointer but... I don't like this
            if (value != NULL) {
                if (type->data.aggregate_type->fields[i].type->type == POINTER_FLAG) {
                    DCpointer p = NULL;
                    type->data.aggregate_type->fields[i].type->store(
                        aTHX_ type->data.aggregate_type->fields[i].type, value, &p);
                    Copy(&p,
                         INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset)),
                         1,
                         intptr_t);
                }
                else {
                    DCpointer ptr = INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset));
                    type->data.aggregate_type->fields[i].type->store(
                        aTHX_ type->data.aggregate_type->fields[i].type, value, &ptr);
                }
            }
        }
        return;
    }
    if (target == NULL)
        croak("Oh, no...");
    if (*target == NULL)
        *target = safecalloc(1, type->size);


    return;
}
*/

POINTER_STORE(Struct) {
    PERL_UNUSED_VAR(type);
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(data));
        SV ** element = NULL;
        size_t a_len = av_len(av);
        if (*target == NULL)
            *target = safecalloc(a_len, type->size);
        IV position = PTR2IV(*target);
        for (size_t i = 0; i < a_len; i++) {
            element = av_fetch(av, i, 0);
            if (element != NULL) {
                DCpointer ptr = INT2PTR(DCpointer, position + (i * type->size));
                type->store(aTHX_ type, *element, &ptr);
            }
        }
        return;
    }
    if (target == NULL)
        croak("Oh, no...");
    if (*target == NULL)
        *target = safecalloc(1, type->size);

    SV * value = NULL;
    HV * hv_data = NULL;
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVHV)
        hv_data = MUTABLE_HV(SvRV(data));
    else
        croak("Expected a hash reference for struct/union store, got something else");

    IV position = PTR2IV(*target);
    for (size_t i = 0; i < type->data.aggregate_type->field_count; i++) {
        value = hv_existsor(hv_data, type->data.aggregate_type->fields[i].name, NULL);

        // We store the address if it's a pointer but... I don't like this
        if (value != NULL) {
            if (type->data.aggregate_type->fields[i].type->type == POINTER_FLAG) {
                DCpointer p = NULL;
                type->data.aggregate_type->fields[i].type->store(
                    aTHX_ type->data.aggregate_type->fields[i].type, value, &p);
                Copy(&p,
                     INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset)),
                     1,
                     intptr_t);
            }
            else {
                DCpointer ptr = INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset));
                type->data.aggregate_type->fields[i].type->store(
                    aTHX_ type->data.aggregate_type->fields[i].type, value, &ptr);
            }
        }
    }
    return;
}


POINTER_FETCH(Struct) {
    if (!sv)
        sv = newSV(0);
    HV * ret = newHV_mortal();
    SV * element = NULL;
    IV position = PTR2IV(data);
    DCpointer pointer = NULL;
    for (size_t i = 0; i < type->data.aggregate_type->field_count; i++) {
        pointer = INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset));
        // if (pointer == NULL) continue; // Don't bother with null pointers
        if (pointer != NULL)
            element = type->data.aggregate_type->fields[i].type->fetch(
                aTHX_ type->data.aggregate_type->fields[i].type, pointer, NULL);

        else
            element = newSV(0);
        (void)hv_store(ret,
                       type->data.aggregate_type->fields[i].name,
                       strlen(type->data.aggregate_type->fields[i].name),
                       element,
                       0);
    }
    sv_setsv_mg(sv, newRV(MUTABLE_SV(ret)));
    return sv;
}

AFFIX_PUSH(Struct) {  //
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    warn("*** PUSH(Struct)");
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    DCpointer ptr = NULL;

    if (SvROK(sv)) {
        // If it's a blessed struct object, extract the pointer directly
        if (sv_derived_from(sv, "Affix::Struct")) {
            ptr = INT2PTR(DCpointer, SvIV(SvRV(sv)));
            warn("[AFFIX_PUSH(Struct)] Marshaled hashref to struct at: %p", ptr);
        }
        // If it's a reference to a hash, marshal it
        if (SvTYPE(SvRV(sv)) == SVt_PVHV) {
            type->store(aTHX_ type, sv, &ptr);
            warn("[AFFIX_PUSH(Struct)] Marshaled hashref to struct at: %p", ptr);
            dcArgAggr(cvm, type->data.aggregate_type->ag, ptr);
            return 1;
        }
        // If it's a reference, marshal it
        if (SvROK(SvRV(sv))) {
            type->store(aTHX_ type, SvRV(sv), &ptr);
            warn("[AFFIX_PUSH(Struct)] Nested reference, marshaled to: %p", ptr);
            // pin(aTHX_ type, SvRV(sv), raw);
        }
        else
            croak("Expected a hash reference for struct/union as an argument, got something else");
    }
    else  // Otherwise, treat as a hash directly
        type->store(aTHX_ type, sv, &ptr);

    dcArgAggr(cvm, type->data.aggregate_type->ag, ptr);
    return 1;
}

AFFIX_CALL(Struct) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    warn("*** CALL(Struct)");
    DCpointer ret = NULL;
    ret = safemalloc(type->size);
    if (ret == NULL)
        croak("Failed to allocate memory for struct return value.");
    /*if(type->data.aggregate_type->ag== NULL)
        croak("Oh, junk");*/
    (void)dcCallAggr(cvm, entrypoint, type->data.aggregate_type->ag, ret);
    DumpHex(ret, type->size);
    SV * ret_sv = type->fetch(aTHX_ type, ret, NULL);
    return ret_sv;
}

CALLBACK_CALL(Struct) {
    PING;
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
};
CALLBACK_PUSH(Struct) {
    PERL_UNUSED_VAR(args);
    PERL_UNUSED_VAR(type);
    PING;
    croak("Incomplete");
}

// Almost exactly like Struct but... offsets are all zero and we only bother with one field
POINTER_STORE(Union) {
    PERL_UNUSED_VAR(type);
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(data));
        SV ** element = NULL;
        size_t a_len = av_len(av);
        if (*target == NULL)
            *target = safecalloc(a_len, type->size);
        IV position = PTR2IV(target);
        for (size_t i = 0; i < a_len; i++) {
            element = av_fetch(av, i, 0);
            if (element != NULL) {
                DCpointer ptr = INT2PTR(DCpointer, position + (i * type->size));
                type->store(aTHX_ type, *element, &ptr);
            }
        }
        return;
    }

    if (target == NULL)
        croak("Oh, no...");
    if (*target == NULL)
        *target = safecalloc(1, type->size);
    SV * value = NULL;
    HV * hv_data = MUTABLE_HV(SvRV(data));
    IV position = PTR2IV(*target);
    for (size_t i = 0; i < type->data.aggregate_type->field_count; i++) {
        value = hv_existsor(hv_data, type->data.aggregate_type->fields[i].name, NULL);
        // We store the address if it's a pointer but... I don't like this
        if (value != NULL) {
            if (type->data.aggregate_type->fields[i].type->type == POINTER_FLAG) {
                DCpointer p = NULL;
                type->data.aggregate_type->fields[i].type->store(
                    aTHX_ type->data.aggregate_type->fields[i].type, value, &p);
                Copy(&p,
                     INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset)),
                     1,
                     intptr_t);
            }
            else {
                DCpointer ptr = INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset));
                type->data.aggregate_type->fields[i].type->store(
                    aTHX_ type->data.aggregate_type->fields[i].type, value, &ptr);
            }
            return;  // Union so... one is enough
        }
    }
    return;
}

POINTER_FETCH(Union) {
    if (!sv)
        sv = newSV(0);
    HV * ret = newHV_mortal();
    SV * element = NULL;
    IV position = PTR2IV(data);
    DCpointer pointer = NULL;
    for (size_t i = 0; i < type->data.aggregate_type->field_count; i++) {
        pointer = INT2PTR(DCpointer, position + (type->data.aggregate_type->fields[i].type->offset));
        element = type->data.aggregate_type->fields[i].type->fetch(
            aTHX_ type->data.aggregate_type->fields[i].type, pointer, NULL);
        (void)hv_store(ret,
                       type->data.aggregate_type->fields[i].name,
                       strlen(type->data.aggregate_type->fields[i].name),
                       element,
                       0);
    }
    sv_setsv_mg(sv, newRV(MUTABLE_SV(ret)));
    return sv;
}

AFFIX_PUSH(Union) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    if (SvROK(sv) && SvROK(SvRV(sv))) {
        DCpointer raw = NULL;
        type->store(aTHX_ type, SvRV(sv), &raw);
        pin(aTHX_ type, SvRV(sv), raw);
        dcArgPointer(cvm, raw);
        return 1;
    }
    DCpointer ptr = NULL;
    type->store(aTHX_ type, sv, &ptr);
    dcArgAggr(cvm, type->data.aggregate_type->ag, ptr);
    return 1;
}

AFFIX_CALL(Union) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    DCpointer ret = NULL;
    ret = safemalloc(type->size);
    (void)dcCallAggr(cvm, entrypoint, type->data.aggregate_type->ag, ret);
    SV * ret_sv = type->fetch(aTHX_ type, ret, NULL);
    return ret_sv;
}

CALLBACK_CALL(Union) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    croak("Incomplete");
};
CALLBACK_PUSH(Union) {
    PING;
    PERL_UNUSED_VAR(args);
    PING;
    PERL_UNUSED_VAR(type);
    PING;
    croak("Incomplete");
}

///
POINTER_STORE(SV) {
    PERL_UNUSED_VAR(type);
    if (*target == NULL)
        Newxz(*target, 1, SV);
    Copy(data, *target, 1, SV);
}
POINTER_FETCH(SV) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    if (!sv)
        sv = newSV(0);
    sv_setsv_mg(sv, MUTABLE_SV(data));
    return sv;
}
AFFIX_PUSH(SV) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    PERL_UNUSED_VAR(cvm);
    PING;
    croak("Incomplete");
}
AFFIX_CALL(SV) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(cvm);
    PERL_UNUSED_VAR(entrypoint);
    PING;
    croak("Incomplete");
}
CALLBACK_CALL(SV) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;

    croak("Incomplete");
}
CALLBACK_PUSH(SV) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;

    croak("Incomplete");
}

POINTER_STORE(CV) {
    if (SvOK(data)) {
        /*HV * field = MUTABLE_HV(SvRV(type));  // Make broad assumptions
        //~ SV **ret = hv_fetchs(field, "ret", 0);
        SV ** args = hv_fetchs(field, "args", 0);
        SV ** sig = hv_fetchs(field, "sig", 0);
        */
        Affix_Type_Callback * cb;
        Newxz(cb, 1, Affix_Type_Callback);
        {
            cb->cv = SvREFCNT_inc_NN(data);
            cb->arg_count = type->data.callback_type->arg_count;
            Newxz(cb->args, cb->arg_count, Affix_Type *);
            for (size_t i = 0; i < cb->arg_count; i++)
                cb->args[i] = clone_Affix_Type(aTHX_ type->data.callback_type->args[i]);
            cb->ret = clone_Affix_Type(aTHX_ type->data.callback_type->ret);
            cb->retval = newSV(0);
            cb->cb_context = type->data.callback_type->cb_context;
            storeTHX(cb->perl);
        }
        // TODO: Generate an accurate signature
        *target = dcbNewCallback("Z", _handle_CV, cb);
        /*
        cb->arg_info = MUTABLE_AV(SvRV(*args));
        size_t arg_count = av_count(cb->arg_info);
        Newxz(cb->sig, arg_count, char);
        for (size_t i = 0; i < arg_count; ++i) {
            cb->sig[i] = type_as_dc(SvIV(*av_fetch(cb->arg_info, i, 0)));
        }
        cb->sig = SvPV_nolen(*sig);
        cb->sig_len = strchr(cb->sig, ')') - cb->sig;
        cb->ret = cb->sig[cb->sig_len + 1];
        cb->cv = SvREFCNT_inc(data);
        storeTHX(cb->perl);
    */
        //~ Newxz(target, 1, Affix_Type_CallbackWrapper);
        //~ ((Affix_Type_CallbackWrapper *)target)->cb = dcbNewCallback(cb->sig, cbHandler, callback);
    }
    else
        Newxz(*target, 1, intptr_t);

    /*
            Callback *cb = (Callback *)dcbGetUserData((DCCallback *)((CallbackWrapper *)ptr)->cb);
            SvSetSV(retval, cb->cv);*/

    //~ Copy(data, target, 1, DCCallback);
    return;
}
POINTER_FETCH(CV) {
    PERL_UNUSED_VAR(type);
    if (!sv)
        sv = newSV(0);
    Affix_Type_Callback * cb = (Affix_Type_Callback *)dcbGetUserData((DCCallback *)data);
    if (cb != NULL)
        sv_setsv_mg(sv, MUTABLE_SV(cb->cv));
    return sv;
}

AFFIX_PUSH(CV) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(st);
    Stack_off_t ax = 0;
    DCpointer ptr = NULL;
    type->store(aTHX_ type, ST(st), &ptr);
    dcArgPointer(cvm, ptr);
    return 1;
}
AFFIX_CALL(CV) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(cvm);
    PERL_UNUSED_VAR(entrypoint);
    PING;
    croak("Incomplete");
}

CALLBACK_CALL(CV) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
}
CALLBACK_PUSH(CV) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete");
}

POINTER_STORE(Pointer) {
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV && (type->data.pointer_type->type == POINTER_FLAG)) {
        AV * list = MUTABLE_AV(SvRV(data));
        size_t length = av_count(list);
        if (*target == NULL)
            Newxz(*target, length + 1, DCpointer);
        DCpointer next;
        SV ** _tmp;
        for (size_t i = 0; i <= length; i++) {
            _tmp = av_fetch(list, i, 0);
            if (UNLIKELY(_tmp == NULL))
                break;
            type->data.pointer_type->store(aTHX_ type->data.pointer_type, *_tmp, &next);
            Copy(&next, INT2PTR(DCpointer, PTR2IV(*target) + (i * SIZEOF_INTPTR_T)), 1, DCpointer);
        }
        return;
    }
    else {
        DCpointer next = NULL;
        type->data.pointer_type->store(aTHX_ type->data.pointer_type, data, &next);
        // DumpHex(next, 16);
        if (*target == NULL)
            Newxz(*target, 1, DCpointer);
        Copy(&next, *target, 1, DCpointer);
        return;
    }
}

POINTER_FETCH(Pointer) {
    if (type->type == STRUCT_FLAG || type->type == UNION_FLAG)
        return MUTABLE_SV(newHV_mortal());
    if (sv != NULL && SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVAV) {
        AV * av = MUTABLE_AV(SvRV(sv));
        size_t len = av_len(av);
        IV position = PTR2IV(data);
        for (size_t i = 0; i <= len; i++) {

            av_store(av,
                     i,
                     type->data.pointer_type->fetch(aTHX_ type->data.pointer_type,
                                                    INT2PTR(DCpointer, position + (i * type->data.pointer_type->size)),
                                                    NULL));
        }
        return newRV(MUTABLE_SV(av));
    }
    return type->data.pointer_type->fetch(aTHX_ type->data.pointer_type, data, sv);
}

AFFIX_PUSH(Pointer) {
    PERL_UNUSED_VAR(affix);
    // TODO: If it's a blessed Affix::Pointer, pass that address
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    DCpointer ptr = NULL;
    //~ PING;

    bool is_rv = false;
    if (!SvOK(sv)) {
        //~ PING;
        ptr = safemalloc(type->size);
        // TODO: pin the sv unless read only
        dcArgPointer(cvm, ptr);
        return 1;
    }
    //~ PING;
    //~ sv_dump(sv);

    if (sv_derived_from(sv, "Affix::Pointer")) {
        //~ PING;
        Affix_Pointer * pointer;
        pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(sv)));
        ptr = pointer->address;
        //~ DumpHex(ptr, /*type->data.pointer_type->size*/ 32);
        dcArgPointer(cvm, ptr);
        return 1;
    }

    if (SvROK(sv) && is_pin(aTHX_ SvRV(sv))) {
        croak("Ref to PIN!");
    }

    if (is_pin(aTHX_ sv)) {
        croak("PIN!");
    }

    if (SvROK(sv) && SvTYPE(SvRV(sv)) < SVt_PVAV) {
        PING;  // TODO: Check that pointer_type is another pointer?

        is_rv = true;
        //~ sv = SvRV(sv);
        //~ PING;
        type->data.pointer_type->store(aTHX_ type->data.pointer_type, SvRV(sv), &ptr);
        PING;
        sv_dump(sv);
        //~ PING;
        pin(aTHX_ type->data.pointer_type, SvRV(sv), ptr);
        PING;
        sv_dump(sv);
        dcArgPointer(cvm, ptr);
        //~ PING;
        return 1;
    }

    type->data.pointer_type->store(aTHX_ type->data.pointer_type, sv, &ptr);


    PING;

    dcArgPointer(cvm, ptr);
    return 1;
    //~ sv_dump(sv);
    //~ croak("Fuck");
}

/*
AFFIX_PUSH(Pointer) {
    PERL_UNUSED_VAR(affix);
    PING;
    // TODO: If it's a blessed Affix::Pointer, pass that address
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    DCpointer ptr = NULL;
    PING;
    if (SvOK(sv)) {
        PING;
        if (SvROK(sv) && SvTYPE(SvRV(sv)) < SVt_PVAV) {
            PING;
            if (sv_derived_from(sv, "Affix::Pointer")) {
                PING;

                Affix_Pointer * pointer;
                pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(sv)));
                ptr = pointer->address;
            }
            else if (SvIOK(sv)) {
                PING;

                IV tmp = SvIV((SV *)(sv));
                ptr = INT2PTR(DCpointer, tmp);
                PING;
            }
            else {
                PING;
                sv_dump(sv);
                PING;
                type->data.pointer_type->store(aTHX_ type->data.pointer_type, SvRV(sv), &ptr);
                PING;
                pin(aTHX_ type->data.pointer_type, SvRV(sv), &ptr);
                PING;
                dcArgPointer(cvm, ptr);
                PING;
                return 1;
            }
        }
        else{
            PING;
            type->data.pointer_type->store(aTHX_ type->data.pointer_type, sv, &ptr);
        }
        PING;
    }
    PING;

    dcArgPointer(cvm, ptr);
    PING;

    return 1;
}
/*

AFFIX_PUSH(Pointer) {
    PERL_UNUSED_VAR(affix);
    // TODO: If it's a blessed Affix::Pointer, pass that address
    Stack_off_t ax = 0;
    SV * sv = ST(st);
    DCpointer ptr = NULL;
    PING;

    /*
if the subtype is a pointer and the SV is a reference
    deref the sv and pass it along to the subtype's store method

if it's explicit undef
    pointer is NULL
if it's a reference
    get reference depth
    if it's a pin
        get pointer from pin
    if it's a blessed Affix::Pointer
        get Affix_Pointer from IV
        bet pointer from Affix_Pointer
    if it's an IV
        get pointer from INT2PTR
    else
        store pointer from sv
if it's a scalar
    store pointer from sv
if it's a string
    store pointer from sv
if it's a number
    store pointer from sv

    * /

    PING;
    // Check for undef

    // If it's a pin, we need to extract the pointer from it
    if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVAV && (type->data.pointer_type->type == POINTER_FLAG)) {
        PING;
        AV * list = MUTABLE_AV(SvRV(sv));
        size_t length = av_count(list);
         if (ptr == NULL)
             Newxz(ptr, length , DCpointer);
        PING;
        DCpointer next;
        SV ** _tmp;
        for (size_t i = 0; i <= length; i++) {
            PING;
            _tmp = av_fetch(list, i, 0);
            if (UNLIKELY(_tmp == NULL))
                break;
            type->data.pointer_type->store(aTHX_ type->data.pointer_type, *_tmp, &next);
            Copy(&next, INT2PTR(DCpointer, PTR2IV(ptr) + (i * SIZEOF_INTPTR_T)), 1, DCpointer);
        }
        return 1;
    }

    PING;
    // If it's a reference, we need to extract the pointer from it
    if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVAV && (type->data.pointer_type->type != POINTER_FLAG)) {
        PING;
        type->data.pointer_type->store(aTHX_ type->data.pointer_type, SvRV(sv), &ptr);
        PING;
        dcArgPointer(cvm, ptr);
        return 1;
    }
    // If it's an explicit undef, we need to pass NULL
    if (SvOK(sv) && SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_NULL) {
        PING;
        ptr = NULL;
        dcArgPointer(cvm, ptr);
        return 1;
    }
    // If it's a pin, we need to extract the pointer from it
    if (SvOK(sv)) {
        PING;
        if (is_pin(aTHX_ sv)) {
            sv_dump(sv);
            PING;
            croak("Pin");
        }
        else if (sv_derived_from(sv, "Affix::Pointer")) {
            warn("Pointer object!");
            PING;

            Affix_Pointer * pointer;
            pointer = INT2PTR(Affix_Pointer *, SvIV(SvRV(sv)));
            ptr = pointer->address;
            PING;
        }
        else if (SvIOK(sv)) {
            PING;
            IV tmp = SvIV((SV *)(sv));
            ptr = INT2PTR(DCpointer, tmp);
            PING;
        }
        else if (SvROK(sv)) {
            PING;
            sv_dump(sv);
            sv_dump(SvRV(sv));
            // Defensive: Only call store if SvRV(sv) is not NULL
            SV * inner = SvRV(sv);
            if (SvOK(inner)) {
                type->data.pointer_type->store(aTHX_ type->data.pointer_type, inner, &ptr);
            } else {
                ptr = NULL;
            }
            PING;
            DumpHex(ptr, 32);
            pin(aTHX_ type->data.pointer_type, SvRV(sv), ptr);
            PING;
            dcArgPointer(cvm, &ptr);
            PING;
            return 1;
        }
        else {
            PING;
            type->data.pointer_type->store(aTHX_ type->data.pointer_type, sv, &ptr);
        }
        PING;
    }
    PING;
    dcArgPointer(cvm, ptr);
    PING;

    return 1;
}*/
AFFIX_CALL(Pointer) {
    PERL_UNUSED_VAR(affix);
    DCpointer ret = NULL;
    // ret = safemalloc(type->size);
    /*if(type->data.aggregate_type->ag== NULL)
        croak("Oh, junk");*/
    //~ PING;
    ret = dcCallPointer(cvm, entrypoint);
    //~ PING;
    SV * ret_sv = type->data.pointer_type->fetch(aTHX_ type->data.pointer_type, ret, newSV(0));
    //~ PING;
    return ret_sv;
}
CALLBACK_CALL(Pointer) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
}
CALLBACK_PUSH(Pointer) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete");
    return newSV(0);
}
POINTER_STORE(Array) {
    PING;
    if (SvROK(data) && SvTYPE(SvRV(data)) == SVt_PVAV
        //&& (type->data.array_type->type->type == POINTER_FLAG)
    ) {
        PING;
        AV * list = MUTABLE_AV(SvRV(data));
        size_t length = type->data.array_type->length ? type->data.array_type->length : av_count(list);
        PING;
        warn("length: %u", length);
        warn("size: %u", (length * type->data.array_type->type->size));
        PING;
        if (*target == NULL) {
            PING;
            *target = safecalloc(length, type->data.array_type->type->size);
            PING;
        }
        PING;
        DCpointer next = NULL;
        SV ** _tmp;
        PING;

        for (size_t i = 0; i < length; i++) {
            PING;
            _tmp = av_fetch(list, i, 0);
            if (UNLIKELY(_tmp == NULL))
                break;
            next = INT2PTR(DCpointer, PTR2IV(*target) + (i * type->data.array_type->type->size));
            PING;
            type->data.array_type->type->store(aTHX_ type->data.array_type->type, *_tmp, &next);
            PING;
        }
        PING;
        return;
    }
    PING;
    if (SvPOK(data)) {
        if (*target == NULL)
            *target = safecalloc(type->data.array_type->length, type->data.array_type->type->size);
        DCpointer next = NULL;
        next = safecalloc(type->data.array_type->length, type->data.array_type->type->size);
        type->data.array_type->type->store(aTHX_ type->data.array_type->type, data, &next);
        Copy(next, *target, type->data.array_type->length, char);
        return;
    }
    if (*target == NULL)
        Newxz(*target, 1, DCpointer);
    DCpointer next = NULL;
    type->data.array_type->type->store(aTHX_ type->data.array_type->type, data, &next);
    Copy(&next, *target, 1, DCpointer);
}

POINTER_FETCH(Array) {
    PING;
    if (type->data.array_type->type->type == CHAR_FLAG)
        return newSVpv(data, type->data.array_type->length);
    PING;

    AV * av = sv != NULL && SvOK(sv) ? MUTABLE_AV(sv) : newAV_mortal();
    PING;
    IV position = PTR2IV(data);
    PING;
    DumpHex(data, 32);
    DCpointer ptr = NULL;

    for (size_t i = 0; i < type->data.array_type->length; i++) {
        PING;
        ptr = INT2PTR(DCpointer, position + (i * type->data.array_type->type->size));

        warn("Loop %u of %u", i + 1, type->data.array_type->length);
        DumpHex(ptr, 32);
        if (ptr != NULL)
            av_store(av, i, type->data.array_type->type->fetch(aTHX_ type->data.array_type->type, ptr, NULL));
        PING;
    }
    PING;

    return newRV(MUTABLE_SV(av));
}

AFFIX_PUSH(Array) {
    PERL_UNUSED_VAR(affix);
    Stack_off_t ax = 0;
    DCpointer ptr = NULL;
    PING;
    type->store(aTHX_ type, ST(st), &ptr);
    PING;
    dcArgPointer(cvm, ptr);
    PING;
    return 1;
}

AFFIX_CALL(Array) {
    PERL_UNUSED_VAR(affix);
    DCpointer ret = NULL;
    PING;
    // ret = safemalloc(type->size);
    /*if(type->data.aggregate_type->ag== NULL)
        croak("Oh, junk");*/
    PING;
    ret = dcCallPointer(cvm, entrypoint);
    PING;
    SV * ret_sv = type->fetch(aTHX_ type, ret, NULL);
    PING;
    return ret_sv;
}

CALLBACK_CALL(Array) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
};

CALLBACK_PUSH(Array) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete");
}

POINTER_STORE(String) {
    PING;
    PERL_UNUSED_VAR(type);
    PING;
    char * value = SvPV_nolen(data);
    PING;
    Copy(&value, *target, 1, intptr_t);
    PING;
    return;
}

POINTER_FETCH(String) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    PING;
    size_t len = strlen(*(DCpointer *)data);
    PING;
    return newSVpvn_utf8(*(DCpointer *)data, len, is_utf8_string((U8 *)*(DCpointer *)data, len));
}

AFFIX_PUSH(String) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    Stack_off_t ax = 0;
    size_t len = 0;
    DCpointer target = NULL;
    if (SvPOK(ST(st))) {
        DCpointer ptr_ = SvPVbyte(ST(st), len);
        Newxz(target, len + 1, char);
        Copy(ptr_, target, len, char);
    }
    dcArgPointer(cvm, target);
    return 1;
}

AFFIX_CALL(String) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    DCpointer ret = dcCallPointer(cvm, entrypoint);
    if (ret == NULL)
        return sv_2mortal(newSV(0));
    return sv_2mortal(newSVpv(ret, 0));
}

CALLBACK_PUSH(String) {
    PERL_UNUSED_VAR(type);
    DCpointer ptr = dcbArgPointer(args);
    if (ptr == NULL)
        return newSV(0);
    STRLEN len = strlen(ptr);
    return sv_2mortal(newSVpvn_utf8(ptr, len, is_utf8_string((U8 *)ptr, len)));
}

CALLBACK_CALL(String) {
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(sv);
    PERL_UNUSED_VAR(type);
    PING;
    croak("Incomplete");
}

//
POINTER_STORE(WString) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(data);
    PERL_UNUSED_VAR(target);
    PING;
    croak("Incomplete");
}
POINTER_FETCH(WString) {
    PING;
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(data);
    PERL_UNUSED_VAR(sv);

    croak("Incomplete");
}
AFFIX_PUSH(WString) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    Stack_off_t ax = 0;
    STRLEN len;
    DCpointer target = NULL;
    if (SvPOK(ST(st))) {
        (void)SvPVutf8x(ST(st), len);
        target = utf2wchar(aTHX_ ST(st), len + 1);
    }
    dcArgPointer(cvm, target);
    return 1;
}
AFFIX_CALL(WString) {
    PERL_UNUSED_VAR(affix);
    PERL_UNUSED_VAR(type);
    DCpointer ret = dcCallPointer(cvm, entrypoint);
    if (ret == NULL)
        return sv_2mortal(newSV(0));
    wchar_t * value = (wchar_t *)ret;
    return wchar2utf(aTHX_ value, wcslen(value));
}

CALLBACK_CALL(WString) {
    PERL_UNUSED_VAR(result);
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(sv);
    PING;
    croak("Incomplete");
}

CALLBACK_PUSH(WString) {
    PERL_UNUSED_VAR(type);
    PERL_UNUSED_VAR(args);
    PING;
    croak("Incomplete");
}

#define typeset(Name, UC)                \
    case UC##_FLAG:                      \
        align = ALIGNOF_##UC;            \
        size = SIZEOF_##UC;              \
        typestr = "Affix::Type::" #Name; \
        break
#define modeset(Name, UC)                \
    case UC##_FLAG:                      \
        typestr = "Affix::Mode::" #Name; \
        break

XS_INTERNAL(Affix_Type_) {
    dXSARGS;
    dXSI32;
    PERL_UNUSED_VAR(items);
    HV * type = newHV();
    hv_stores(type, "flag", newSViv(ix));
    size_t align = 0, size = 0;
    const char * typestr;
    switch (ix) {
    case VOID_FLAG:
        typestr = "Affix::Type::Void";
        break;
        typeset(Bool, BOOL);
        typeset(SChar, SCHAR);
        typeset(Char, CHAR);
        typeset(UChar, UCHAR);
        typeset(WChar, WCHAR);
        typeset(Short, SHORT);
        typeset(UShort, USHORT);
        typeset(Int, INT);
    case ENUM_FLAG:  // Enum[...]
        align = ALIGNOF_INT;
        size = SIZEOF_INT;
        typestr = "Affix::Type::Enum";
        {
            //~ bool packed = false;
            // TODO: Check SvROK && SvTYPE(SvRV()) == AV
            AV * fields_ = MUTABLE_AV(SvRV(ST(0)));
            AV * fields = newAV();
            SV * sv_val;
            size_t s_fields = av_count(fields_);
            for (size_t i = 0; i < s_fields; ++i) {
                // TODO: Check SvROK && SvTYPE(SvRV()) == AV
                sv_val = newSVsv(*av_fetch(fields_, i, 0));
                av_push(fields, newSVsv(sv_val));
            }
            hv_stores(type, "fields", newRV_noinc(MUTABLE_SV(fields)));
        }
        break;

        typeset(UInt, UINT);
        typeset(Long, LONG);
        typeset(ULong, ULONG);
        typeset(LongLong, LONGLONG);
        typeset(ULongLong, ULONGLONG);
        typeset(Size_t, SIZE_T);
        typeset(SSize_t, SSIZE_T);
        typeset(Float, FLOAT);
        typeset(Double, DOUBLE);
    case POINTER_FLAG:
        align = ALIGNOF_INTPTR_T;
        size = SIZEOF_INTPTR_T;
        typestr = "Affix::Type::Pointer";
        {
            AV * type_len = MUTABLE_AV(SvRV(ST(0)));
            // TODO: check == 1
            HV * hv_type = MUTABLE_HV(SvRV(newSVsv(*av_fetch(type_len, 0, 0))));
            align = SvIV(hv_existsor(hv_type, "alignment", newSViv(1)));
            hv_stores(type, "subtype", newRV_noinc(MUTABLE_SV(hv_type)));
        }
        break;
    // Aggregates
    case STRUCT_FLAG:
    case UNION_FLAG:
        typestr = ix == STRUCT_FLAG ? "Affix::Type::Struct" : "Affix::Type::Union";
        {
            bool packed = false;
            // TODO: Check SvROK && SvTYPE(SvRV()) == AV
            AV * fields_ = MUTABLE_AV(SvRV(ST(0)));
            AV * fields = newAV();
            size_t s_fields = av_count(fields_);  // TODO: check % 2
            SV * sv_key;
            HV * hv_val;
            size_t offset = 0, max_alignment = 0;
            //~ size_t alignment = packed ? 1 : AFFIX_ALIGNBYTES;

            for (size_t i = 0; i < s_fields; i += 2) {
                // TODO: Check SvROK && SvTYPE(SvRV()) == AV
                sv_key = newSVsv(*av_fetch(fields_, i, 0));
                hv_val = MUTABLE_HV(SvRV(newSVsv(*av_fetch(fields_, i + 1, 0))));
                size_t field_size = SvIV(*hv_fetchs(hv_val, "sizeof", 0));
                size_t field_alignment = SvIV(*hv_fetchs(hv_val, "alignment", 0));
                if (!packed) {
                    max_alignment = field_alignment > max_alignment ? field_alignment : max_alignment;
                    if (ix != UNION_FLAG)
                        offset += (field_alignment - (offset % field_alignment)) % field_alignment;
                }
                av_push(fields, sv_key);
                hv_stores(hv_val, "offset", newSViv(offset));
                av_push(fields, newRV_noinc(MUTABLE_SV(hv_val)));
                if (ix == UNION_FLAG)
                    size = size > field_size ? size : field_size;
                else {
                    offset += field_size;
                    size = offset;
                }
            }
            if (!packed && max_alignment > 1) {
                size += (max_alignment - (size % max_alignment)) % max_alignment;
            }

            hv_stores(type, "fields", newRV_noinc(MUTABLE_SV(fields)));
            align = size < AFFIX_ALIGNBYTES ? size : AFFIX_ALIGNBYTES;
        }
        break;
    case ARRAY_FLAG:
        typestr = "Affix::Type::Array";
        {
            bool packed = true;  // C dictates contiguous blocks of data
            AV * type_len = MUTABLE_AV(SvRV(ST(0)));
            // TODO: check == 2
            HV * hv_type = MUTABLE_HV(SvRV(newSVsv(*av_fetch(type_len, 0, 0))));
            size_t s_len = av_count(type_len) == 2 ? SvIV(*av_fetch(type_len, 1, 0)) : 0;
            size_t s_sizeof = SvIV(*hv_fetchs(hv_type, "sizeof", 0));
            align = SvIV(*hv_fetchs(hv_type, "alignment", 0));
            for (size_t i = 0; i < s_len; ++i) {
                if (!packed)
                    size += (align - (size % align)) % align;
                size += s_sizeof;
            }
            if (!packed && align > 1)
                size += (align - (size % align)) % align;

            hv_stores(type, "count", newSViv(s_len));
            hv_stores(type, "subtype", newRV_noinc(MUTABLE_SV(hv_type)));
        }
        break;
    case STRING_FLAG:
        align = ALIGNOF_INTPTR_T;
        size = SIZEOF_INTPTR_T;
        typestr = "Affix::Type::String";
        break;
    case WSTRING_FLAG:
        align = ALIGNOF_INTPTR_T;
        size = SIZEOF_INTPTR_T;
        typestr = "Affix::Type::WString";
        break;

    // Perl
    case SV_FLAG:
        align = ALIGNOF_SV;
        size = SIZEOF_SV;
        typestr = "Affix::Type::SV";
        break;
    case CV_FLAG:
        align = ALIGNOF_INTPTR_T;
        size = SIZEOF_INTPTR_T;
        typestr = "Affix::Type::CV";
        {
            AV * args_ret = MUTABLE_AV(SvRV(ST(0)));
            // TODO: Check av_len
            SV **args, **ret;
            args = av_fetch(args_ret, 0, 0);
            ret = av_fetch(args_ret, 1, 0);
            if (args != NULL)  // TODO: Check SvTYPE
                hv_stores(type, "arguments", newSVsv(*args));
            if (ret != NULL)  // TODO: Check SvTYPE
                hv_stores(type, "return", newSVsv(*ret));
        }
        break;
    // dyncall stuff
    case RESET_FLAG:
        typestr = "Affix::Instruction::Reset";
        break;
        modeset(Default, DEFAULT);
        modeset(This, THIS);
        modeset(Ellipsis, ELLIPSIS);
        modeset(Varargs, VARARGS);
        modeset(CDecl, CDECL);
        modeset(Stdcall, STDCALL);
        modeset(FastcallMS, MSFASTCALL);
        modeset(FastcallGNU, GNUFASTCALL);
        modeset(ThisMS, MSTHIS);
        modeset(ThisGNU, GNUTHIS);
        modeset(Arm, ARM);
        modeset(Thumb, THUMB);
        modeset(Syscall, SYSCALL);
    default:
        croak("Oh, no... unknown type? What's %d [%c]", ix, ix);
    }
    if (align)
        hv_stores(type, "alignment", newSViv(align));
    if (size)
        hv_stores(type, "sizeof", newSViv(size));
    ST(0) = sv_2mortal(sv_bless(newRV_inc(MUTABLE_SV(type)), gv_stashpv(typestr, GV_ADD)));
    XSRETURN(1);
}

/**
 * @file Affix/Type.c
 * @brief Contains implementation of functions and types related to Affix type handling.
 *
 * This file provides the core logic for managing and manipulating Affix types,
 * including their creation, destruction, and any associated operations.
 *
 * Detailed descriptions of each function and data structure can be found in their respective
 * documentation comments within this file.
 */

Affix_Type * new_Affix_Type(pTHX_ SV * type) {
    Affix_Type * retval = NULL;
    //~ DD(newRV(type));
    if (!SvROK(type) || SvTYPE(SvRV(type)) != SVt_PVHV) {
        warn("new_Affix_Type: Expected blessed hash reference for Affix::Type, got something else.");
        return NULL;
    }

    HV * hv_type = MUTABLE_HV(SvRV(type));
    SV * align = hv_existsor(hv_type, "alignment", newSViv(0));
    SV * offset = hv_existsor(hv_type, "offset", newSViv(0));
    SV * flag = hv_existsor(hv_type, "flag", newSViv(0));
    SV * size = hv_existsor(hv_type, "sizeof", newSViv(1));
    SV * subtype = hv_existsor(hv_type, "subtype", NULL);
    SV * fields = hv_existsor(hv_type, "fields", NULL);
    // CV*
    SV * ret = hv_existsor(hv_type, "return", NULL);
    SV * args = hv_existsor(hv_type, "arguments", NULL);
    // Array
    SV * count = hv_existsor(hv_type, "count", NULL);
    //
    //~ DD(type);
    Newxz(retval, 1, Affix_Type);
    if (!retval)
        return NULL;

    *retval = (Affix_Type){.type = (char)SvIV(flag),
                           .dc_type = atype_to_dtype((char)SvIV(flag)),
                           .size = SvIV(size),
                           .align = SvIV(align),
                           .offset = SvIV(offset)};
    //~ warn("retval->type: %d [%c]", retval->type, retval->type);
    switch (retval->type) {
    case VOID_FLAG:
        ASSIGN_TYPE_HANDLERS(Void);
        break;
    case BOOL_FLAG:
        ASSIGN_TYPE_HANDLERS(Bool);
        break;
    case SCHAR_FLAG:
        ASSIGN_TYPE_HANDLERS(SChar);
        break;
    case CHAR_FLAG:
        ASSIGN_TYPE_HANDLERS(Char);
        break;
    case UCHAR_FLAG:
        ASSIGN_TYPE_HANDLERS(UChar);
        break;
    case WCHAR_FLAG:
        ASSIGN_TYPE_HANDLERS(WChar);
        break;
    case SHORT_FLAG:
        ASSIGN_TYPE_HANDLERS(Short);
        break;
    case USHORT_FLAG:
        ASSIGN_TYPE_HANDLERS(UShort);
        break;
    case INT_FLAG:
        ASSIGN_TYPE_HANDLERS(Int);
        break;
    case UINT_FLAG:
        ASSIGN_TYPE_HANDLERS(UInt);
        break;
    case LONG_FLAG:
        ASSIGN_TYPE_HANDLERS(Long);
        break;
    case ULONG_FLAG:
        ASSIGN_TYPE_HANDLERS(ULong);
        break;
    case LONGLONG_FLAG:
        ASSIGN_TYPE_HANDLERS(LongLong);
        break;
    case ULONGLONG_FLAG:
        ASSIGN_TYPE_HANDLERS(ULongLong);
        break;
    case SIZE_T_FLAG:
        ASSIGN_TYPE_HANDLERS(Size_t);
        break;
    case SSIZE_T_FLAG:
        ASSIGN_TYPE_HANDLERS(SSize_t);
        break;
    case FLOAT_FLAG:
        ASSIGN_TYPE_HANDLERS(Float);
        break;
    case DOUBLE_FLAG:
        ASSIGN_TYPE_HANDLERS(Double);
        break;
    case STRING_FLAG:
        ASSIGN_TYPE_HANDLERS(String);
        break;
    case WSTRING_FLAG:
        ASSIGN_TYPE_HANDLERS(WString);
        break;
    case SV_FLAG:
        ASSIGN_TYPE_HANDLERS(SV);
        break;
    case CV_FLAG:
        ASSIGN_TYPE_HANDLERS(CV);


        {

            AV * av_args = MUTABLE_AV(SvRV(args));
            //~ SV ** arg_type;
            size_t arg_count = av_count(av_args);
            //~ warn("Field count: %u", field_count);
            Newxz(retval->data.callback_type, 1, Affix_Type_Callback);
            if (!retval->data.callback_type) {
                safefree(retval);
                return NULL;
            }
            retval->data.callback_type->ret = new_Affix_Type(aTHX_ ret);
            retval->data.callback_type->arg_count = arg_count;

            retval->data.callback_type->cb_context = (arg_count ? 0 : G_NOARGS) |
                (retval->data.callback_type->ret->type == VOID_FLAG ? G_VOID : G_SCALAR) | G_KEEPERR | G_EVAL;
            /*  G_VOID
                G_SCALAR
                G_NOARGS
                G_EVAL
                G_KEEPERR
            */
            Newxz(retval->data.callback_type->args, retval->data.callback_type->arg_count, Affix_Type *);
            if (!retval->data.callback_type->args) {
                safefree(retval->data.callback_type);
                safefree(retval);
                return NULL;
            }
            //~ Affix_Type_Aggregate
            //~ Affix_Type_Aggregate_Fields
            // TODO: This is where I could throw a signature together for dyncall and one for perl
            for (size_t i = 0; i < arg_count; i++)
                retval->data.callback_type->args[i] = new_Affix_Type(aTHX_ * av_fetch(av_args, i, 0));
        }


        break;
    case POINTER_FLAG:
        ASSIGN_TYPE_HANDLERS(Pointer);
        retval->data.pointer_type = new_Affix_Type(aTHX_ subtype);
        if (!retval->data.pointer_type) {
            safefree(retval);
            return NULL;
        }
        break;
    case ARRAY_FLAG:
        ASSIGN_TYPE_HANDLERS(Array);
        Newx(retval->data.array_type, 1, Affix_Type_Array);
        if (!retval->data.array_type) {
            safefree(retval);
            return NULL;
        }
        if (count == &PL_sv_undef || !SvIOK(count)) {
            safefree(retval->data.array_type);
            safefree(retval);
            return NULL;
        }
        retval->data.array_type->length = (size_t)SvIV(count);
        retval->data.array_type->type = new_Affix_Type(aTHX_ subtype);
        if (!retval->data.array_type->type) {
            safefree(retval->data.array_type);
            safefree(retval);
            return NULL;
        }
        break;
    case STRUCT_FLAG:
    case UNION_FLAG:
        if (retval->type == STRUCT_FLAG) {
            ASSIGN_TYPE_HANDLERS(Struct);
        }
        else {
            ASSIGN_TYPE_HANDLERS(Union);
        }

        Newx(retval->data.aggregate_type, 1, Affix_Type_Aggregate);  // Use Newx
        if (!retval->data.aggregate_type) {
            Safefree(retval);  // Use Safefree
            return NULL;
        }
        memset(retval->data.aggregate_type, 0, sizeof(Affix_Type_Aggregate));  // Initialize

        // Get 'fields' arrayref
        if (fields == &PL_sv_undef || !SvROK(fields) || SvTYPE(SvRV(fields)) != SVt_PVAV) {
            Safefree(retval->data.aggregate_type);  // Use Safefree
            Safefree(retval);                       // Use Safefree
            return NULL;
        }
        AV * fields_av = (AV *)SvRV(fields);
        Size_t num_elements = av_len(fields_av) + 1;  // Number of elements in the Perl arrayref

        if (num_elements % 2 != 0) {
            Safefree(retval->data.aggregate_type);  // Use Safefree
            Safefree(retval);                       // Use Safefree
            return NULL;
        }

        size_t field_count = num_elements / 2;
        retval->data.aggregate_type->field_count = field_count;

        if (field_count > 0) {
            Newxz(retval->data.aggregate_type->fields, field_count, Affix_Type_Aggregate_Fields);
            if (!retval->data.aggregate_type->fields) {
                safefree(retval->data.aggregate_type);
                safefree(retval);
                return NULL;
            }
        }
        else
            retval->data.aggregate_type->fields = NULL;

        PING;
        warn("*** dcNewAggr(%d, %d);", retval->data.aggregate_type->field_count, retval->size);
        retval->data.aggregate_type->ag = dcNewAggr(retval->data.aggregate_type->field_count, retval->size);
        if (!retval->data.aggregate_type->ag) {
            if (retval->data.aggregate_type->fields)
                safefree(retval->data.aggregate_type->fields);
            safefree(retval->data.aggregate_type);
            safefree(retval);
            return NULL;
        }

        for (size_t i = 0; i < field_count; ++i) {
            SV ** name_sv_ptr = av_fetch(fields_av, 2 * i, 0);
            if (!name_sv_ptr || !SvPOK(*name_sv_ptr)) {
                for (size_t j = 0; j < i; ++j) {
                    safefree(retval->data.aggregate_type->fields[j].name);
                    destroy_Affix_Type(aTHX_ retval->data.aggregate_type->fields[j].type);
                }
                if (retval->data.aggregate_type->fields)
                    safefree(retval->data.aggregate_type->fields);
                dcFreeAggr(retval->data.aggregate_type->ag);
                safefree(retval->data.aggregate_type);
                safefree(retval);
                return NULL;
            }
            STRLEN name_len;
            char * name_str = SvPV(*name_sv_ptr, name_len);
            Newx(retval->data.aggregate_type->fields[i].name, name_len + 1, char);
            if (!retval->data.aggregate_type->fields[i].name) {
                for (size_t j = 0; j < i; ++j) {
                    safefree(retval->data.aggregate_type->fields[j].name);
                    destroy_Affix_Type(aTHX_ retval->data.aggregate_type->fields[j].type);
                }
                if (retval->data.aggregate_type->fields)
                    Safefree(retval->data.aggregate_type->fields);
                dcFreeAggr(retval->data.aggregate_type->ag);
                safefree(retval->data.aggregate_type);
                safefree(retval);
                return NULL;
            }
            strcpy(retval->data.aggregate_type->fields[i].name, name_str);

            SV ** field_type_sv_ptr = av_fetch(fields_av, 2 * i + 1, 0);
            if (!field_type_sv_ptr || !SvROK(*field_type_sv_ptr)) {
                safefree(retval->data.aggregate_type->fields[i].name);
                for (size_t j = 0; j < i; ++j) {
                    Safefree(retval->data.aggregate_type->fields[j].name);
                    destroy_Affix_Type(aTHX_ retval->data.aggregate_type->fields[j].type);
                }
                if (retval->data.aggregate_type->fields)
                    Safefree(retval->data.aggregate_type->fields);
                dcFreeAggr(retval->data.aggregate_type->ag);
                safefree(retval->data.aggregate_type);
                safefree(retval);
                return NULL;
            }
            retval->data.aggregate_type->fields[i].type = new_Affix_Type(aTHX_ * field_type_sv_ptr);
            if (!retval->data.aggregate_type->fields[i].type) {
                safefree(retval->data.aggregate_type->fields[i].name);
                for (size_t j = 0; j < i; ++j) {
                    safefree(retval->data.aggregate_type->fields[j].name);
                    destroy_Affix_Type(aTHX_ retval->data.aggregate_type->fields[j].type);
                }
                if (retval->data.aggregate_type->fields)
                    safefree(retval->data.aggregate_type->fields);
                dcFreeAggr(retval->data.aggregate_type->ag);
                safefree(retval->data.aggregate_type);
                safefree(retval);
                return NULL;
            }

            PING;
            warn("*** Pushing field onto aggregate: %s, type: %c, offset: %d",
                 retval->data.aggregate_type->fields[i].name,
                 retval->data.aggregate_type->fields[i].type->type,
                 retval->data.aggregate_type->fields[i].type->offset);
            if (retval->type == UNION_FLAG || retval->type == STRUCT_FLAG)
                dcAggrField(retval->data.aggregate_type->ag,
                            retval->data.aggregate_type->fields[i].type->dc_type,
                            retval->data.aggregate_type->fields[i].type->offset,
                            1,
                            retval->data.aggregate_type->ag);
            else
                dcAggrField(retval->data.aggregate_type->ag,
                            retval->data.aggregate_type->fields[i].type->dc_type,
                            retval->data.aggregate_type->fields[i].type->offset,
                            1);
        }
        dcCloseAggr(retval->data.aggregate_type->ag);
        break;
    case RESET_FLAG:
        retval->pass = _pass_Reset;
        break;
    case DEFAULT_FLAG:
        ASSIGN_MODE_HANDLER(Default, DC_SIGCHAR_CC_DEFAULT);
        break;
    case THIS_FLAG:
        ASSIGN_MODE_HANDLER(This, DC_SIGCHAR_CC_THISCALL);
        break;
    case ELLIPSIS_FLAG:
        ASSIGN_MODE_HANDLER(Ellipsis, DC_SIGCHAR_CC_ELLIPSIS);
        break;
    case VARARGS_FLAG:
        ASSIGN_MODE_HANDLER(Varargs, DC_SIGCHAR_CC_ELLIPSIS_VARARGS);
        break;
    case CDECL_FLAG:
        ASSIGN_MODE_HANDLER(CDecl, DC_SIGCHAR_CC_CDECL);
        break;
    case STDCALL_FLAG:
        ASSIGN_MODE_HANDLER(Stdcall, DC_SIGCHAR_CC_STDCALL);
        break;
    case MSFASTCALL_FLAG:
        ASSIGN_MODE_HANDLER(FastcallMS, DC_SIGCHAR_CC_FASTCALL_MS);
        break;
    case GNUFASTCALL_FLAG:
        ASSIGN_MODE_HANDLER(FastcallGNU, DC_SIGCHAR_CC_FASTCALL_GNU);
        break;
    case MSTHIS_FLAG:
        ASSIGN_MODE_HANDLER(ThisMS, DC_SIGCHAR_CC_THISCALL_MS);
        break;
    case GNUTHIS_FLAG:
        ASSIGN_MODE_HANDLER(ThisGNU, DC_SIGCHAR_CC_THISCALL_GNU);
        break;
    case ARM_FLAG:
        ASSIGN_MODE_HANDLER(Arm, DC_SIGCHAR_CC_ARM_ARM);
        break;
    case THUMB_FLAG:
        ASSIGN_MODE_HANDLER(Thumb, DC_SIGCHAR_CC_ARM_THUMB);
        break;
    case SYSCALL_FLAG:
        ASSIGN_MODE_HANDLER(Syscall, DC_SIGCHAR_CC_SYSCALL);
        break;
    default:
        croak("Unhandled type: %c", (char)retval->type);
        break;
    }

    return retval;

    if (subtype) {
        if (count != NULL && SvIOK(count)) {  // Array[ Type, Len ]
            Newxz(retval->data.array_type, 1, Affix_Type_Array);
            if (!retval->data.array_type) {
                safefree(retval);
                return NULL;
            }
            retval->data.array_type->length = SvIV(count);
            retval->data.array_type->type = new_Affix_Type(aTHX_ subtype);
        }
        else
            retval->data.pointer_type = new_Affix_Type(aTHX_ subtype);
    }
    else if (fields) {
        //~ PING;
        AV * av_fields = MUTABLE_AV(SvRV(fields));
        SV **field_name, **field_type;
        size_t field_count = av_count(av_fields);
        //~ warn("Field count: %u", field_count);
        Newxz(retval->data.aggregate_type, 1, Affix_Type_Aggregate);
        retval->data.aggregate_type->field_count = field_count / 2;
        Newxz(
            retval->data.aggregate_type->fields, retval->data.aggregate_type->field_count, Affix_Type_Aggregate_Fields);
        retval->data.aggregate_type->ag = dcNewAggr(retval->data.aggregate_type->field_count, retval->size);
        for (size_t i = 0, index = 0; i < field_count; i += 2, index++) {
            field_name = av_fetch(av_fields, i, 0);
            field_type = av_fetch(av_fields, i + 1, 0);
            retval->data.aggregate_type->fields[index].name = SvPVbyte_nolen(*field_name);
            retval->data.aggregate_type->fields[index].type = new_Affix_Type(aTHX_ * field_type);
            if (retval->type == UNION_FLAG || retval->type == STRUCT_FLAG)
                dcAggrField(retval->data.aggregate_type->ag,
                            retval->data.aggregate_type->fields[index].type->dc_type,
                            retval->data.aggregate_type->fields[index].type->offset,
                            1,
                            retval->data.aggregate_type->ag);
            else
                dcAggrField(retval->data.aggregate_type->ag,
                            retval->data.aggregate_type->fields[index].type->dc_type,
                            retval->data.aggregate_type->fields[index].type->offset,
                            1);
        }
        dcCloseAggr(retval->data.aggregate_type->ag);
    }
    else if (ret && args) {
        AV * av_args = MUTABLE_AV(SvRV(args));
        //~ SV ** arg_type;
        size_t arg_count = av_count(av_args);
        //~ warn("Field count: %u", field_count);
        Newxz(retval->data.callback_type, 1, Affix_Type_Callback);
        if (!retval->data.callback_type) {
            safefree(retval);
            return NULL;
        }
        retval->data.callback_type->ret = new_Affix_Type(aTHX_ ret);
        retval->data.callback_type->arg_count = arg_count;

        retval->data.callback_type->cb_context = (arg_count ? 0 : G_NOARGS) |
            (retval->data.callback_type->ret->type == VOID_FLAG ? G_VOID : G_SCALAR) | G_KEEPERR | G_EVAL;
        /*  G_VOID
            G_SCALAR
            G_NOARGS
            G_EVAL
            G_KEEPERR
        */
        Newxz(retval->data.callback_type->args, retval->data.callback_type->arg_count, Affix_Type *);
        if (!retval->data.callback_type->args) {
            safefree(retval->data.callback_type);
            safefree(retval);
            return NULL;
        }
        //~ Affix_Type_Aggregate
        //~ Affix_Type_Aggregate_Fields
        // TODO: This is where I could throw a signature together for dyncall and one for perl
        for (size_t i = 0; i < arg_count; i++)
            retval->data.callback_type->args[i] = new_Affix_Type(aTHX_ * av_fetch(av_args, i, 0));
    }
    return retval;
}

XS_INTERNAL(Affix_Type_END) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    //~ dMY_CXT;
    XSRETURN_EMPTY;
}

// Painfully cheap typedef system
extern void Affix_typedef_(pTHX_ CV * cv) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    ST(0) = (SV *)XSANY.any_ptr;
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_typedef) {
    dXSARGS;
    //~ dXSI32;
    if (items != 2)
        croak_xs_usage(cv, "$name, $type");
    SV * name = ST(0);

    STMT_START {
        cv = newXSproto_portable(SvPV_nolen(name), Affix_typedef_, __FILE__, "");
        if (UNLIKELY(cv == NULL))
            croak("ARG! Something went really wrong while installing a new XSUB!");
        SV * clone = newSVsv(ST(1));
        {
            HV * hv = (HV *)SvRV(clone);
            hv_stores(hv, "typedef", newSVsv(name));
        }
        XSANY.any_ptr = (DCpointer)clone;
    }
    STMT_END;
    ST(0) = ST(1);
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_sizeof) {
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "$type");
    ST(0) = *hv_fetchs(MUTABLE_HV(SvRV(ST(0))), "sizeof", 1);
    XSRETURN(1);
}

XS_INTERNAL(Affix_Type_Pointer_new) {
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$type, @data");
    // Create a new Affix::Pointer object
    SV * type_sv = ST(0);
    AV * data_av = MUTABLE_AV(ST(1));
    Affix_Type * type = new_Affix_Type(aTHX_ type_sv);
}

XS_INTERNAL(Affix_Type_Struct_new) {
    dXSARGS;
    sv_dump(ST(0));
    // sv_dump(ST(1));
    if (items != 2)
        croak_xs_usage(cv, "$type, %data");
    // Create a new Affix::Pointer object
    SV * type_sv = ST(0);
    SV * _typedef = hv_existsor(MUTABLE_HV(SvRV(type_sv)), "typedef", NULL);
    SV * data_sv = ST(1);
    if (!(SvROK(data_sv) && SvTYPE(SvRV(data_sv)) == SVt_PVHV))
        croak("%s->new(...) requires a hash", SvPV_nolen(type_sv));
    sv_dump(type_sv);

    Affix_Type * type = new_Affix_Type(aTHX_ type_sv);
    if (type == NULL)
        croak("Failed to create new %s", SvPV_nolen(_typedef));

    DCpointer ptr = NULL;
    type->store(aTHX_ type, data_sv, &ptr);
    if (ptr == NULL)
        croak("Failed to store data in %s", SvPV_nolen(_typedef));

    SV * sv = newSV(0);
    pin(aTHX_ type, sv, ptr);
    sv_setref_pv(sv, SvPV_nolen(_typedef), INT2PTR(DCpointer, ptr));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

SV * gen_dualvar(pTHX_ IV iv, const char * pv) {
    SV * sv = newSVpvn_share(pv, strlen(pv), 0);
    (void)SvUPGRADE(sv, SVt_PVIV);
    SvIV_set(sv, iv);
    SvIandPOK_on(sv);
    return sv;
}

///////////////////////////////////////////
#define param_type(Name, Flag, Sig)                                        \
    set_isa("Affix::Type::" #Name, "Affix::Type");                         \
    export_function("Affix", #Name, "types");                              \
    cv = newXSproto_portable("Affix::" #Name, Affix_Type_, __FILE__, Sig); \
    XSANY.any_i32 = Flag

#define simple_type(Name, Flag) param_type(Name, Flag, "")

void boot_Affix_Type(pTHX_ CV * cv) {
    (void)newXSproto_portable("Affix::Type::END", Affix_Type_END, __FILE__, "");

    // Fundamentals, etc.
    simple_type(Void, VOID_FLAG);
    simple_type(Bool, BOOL_FLAG);
    simple_type(Char, CHAR_FLAG);
    simple_type(SChar, SCHAR_FLAG);
    simple_type(UChar, UCHAR_FLAG);
    simple_type(WChar, WCHAR_FLAG);
    simple_type(Short, SHORT_FLAG);
    simple_type(UShort, USHORT_FLAG);
    simple_type(Int, INT_FLAG);
    simple_type(UInt, UINT_FLAG);
    simple_type(Long, LONG_FLAG);
    simple_type(ULong, ULONG_FLAG);
    simple_type(LongLong, LONGLONG_FLAG);
    simple_type(ULongLong, ULONGLONG_FLAG);
    simple_type(Float, FLOAT_FLAG);
    simple_type(Double, DOUBLE_FLAG);
    param_type(Pointer, POINTER_FLAG, "$");
    param_type(Size_t, SIZE_T_FLAG, "");  // prevent preprocessor mangling
    param_type(SSize_t, SSIZE_T_FLAG, "");

    // Aggregates
    param_type(Struct, STRUCT_FLAG, "$");
    param_type(Union, UNION_FLAG, "$");
    param_type(Array, ARRAY_FLAG, "$");
    simple_type(String, STRING_FLAG);
    simple_type(WString, WSTRING_FLAG);

    // Perl
    simple_type(SV, SV_FLAG);

    // CV
    param_type(Callback, CV_FLAG, "$");

    // Enumerations
    param_type(Enum, ENUM_FLAG, "$");

    // Flags
    simple_type(Reset, RESET_FLAG);
    simple_type(Default, DEFAULT_FLAG);
    simple_type(This, THIS_FLAG);
    simple_type(Ellipsis, ELLIPSIS_FLAG);
    simple_type(Varargs, VARARGS_FLAG);
    simple_type(CDecl, CDECL_FLAG);
    simple_type(STDcall, STDCALL_FLAG);
    simple_type(FastcallMS, MSFASTCALL_FLAG);
    simple_type(FastcallGNU, GNUFASTCALL_FLAG);
    simple_type(ThisMS, MSTHIS_FLAG);
    simple_type(ThisGNU, GNUTHIS_FLAG);
    simple_type(Arm, ARM_FLAG);
    simple_type(Thumb, THUMB_FLAG);
    simple_type(Syscall, SYSCALL_FLAG);

    (void)newXSproto_portable("Affix::Type::Pointer::new", Affix_Type_Pointer_new, __FILE__, "$;$");

    (void)newXSproto_portable("Affix::Type::Struct::new", Affix_Type_Struct_new, __FILE__, "$;%");

    (void)newXSproto_portable("Affix::typedef", Affix_Type_typedef, __FILE__, "$$");
    export_function("Affix", "typedef", "type");
    (void)newXSproto_portable("Affix::Type::sizeof", Affix_Type_sizeof, __FILE__, "$$");
    export_function("Affix", "sizeof", "type");


    //~ (void)newXSproto_portable("Affix::gen_dualvar", Affix_gen_dualvar, __FILE__, "$$");
    //~ (void)newXSproto_portable("Affix::gen_enum", Affix_gen_enum, __FILE__, "$");
    //~ (void)newXSproto_portable("Affix::get_enum", Affix_get_enum, __FILE__, "$$");
    //~ (void)newXSproto_portable("Affix::Enum::DESTROY", Affix_Enum_DESTROY, __FILE__, "$;$");
}
