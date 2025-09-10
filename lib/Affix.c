#include "Affix.h"

/*
G|-------------------0----------------|--0---4----------------------------||
D|.----------0---3-------0---3---0----|----------3---0-------0---3---0---.||
A|.------2----------------------------|----------------------------------.||
E|---3--------------------------------|------------------3----------------||
*/

START_MY_CXT

DLLib * load_library(const char * lib) {
    return
#if defined(DC__OS_Win64) || defined(DC__OS_MacOSX)
        dlLoadLibrary(lib);
#else
        (DLLib *)dlopen(lib, RTLD_LAZY /* RTLD_NOW|RTLD_GLOBAL */);
#endif
}

void free_library(DLLib * lib) {
    dlFreeLibrary(lib);
}

DCpointer find_symbol(DLLib * lib, const char * name) {
    return dlFindSymbol(lib, name);
}

XS_INTERNAL(Affix_dlerror) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    dXSI32;
    PERL_UNUSED_VAR(ix);
#if defined(DC__OS_Win64)
    const
#endif
        char * ret = dlerror();
    if (!ret)
        XSRETURN_UNDEF;
    SV * sv_ret = sv_2mortal(newSVpv(ret, 0));
    ST(0) = sv_ret;
    XSRETURN(1);
}

XS_INTERNAL(Affix_load_library) {
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "$lib");
    DCpointer lib;
    if (!SvOK(ST(0)) && SvREADONLY(ST(0)))  // explicit undef
        lib = load_library(NULL);
    else
        lib = load_library(SvPV_nolen(ST(0)));
    if (!lib)
        XSRETURN_UNDEF;
    SV * LIBSV = sv_newmortal();
    sv_setref_pv(LIBSV, "Affix::Lib", lib);
    ST(0) = LIBSV;
    XSRETURN(1);
}

XS_INTERNAL(Affix_list_symbols) {
    /* dlSymsName(...) is not thread-safe on MacOS */
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "lib");
    AV * RETVAL;
    DLLib * lib;
    if (SvROK(ST(0))) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        croak("lib is not of type Affix::Lib");
    RETVAL = newAV_mortal();
    char * name;
    Newxz(name, 1024, char);
    int len = dlGetLibraryPath(lib, name, 1024);
    if (len == 0)
        croak("Failed to get library name");
    DLSyms * syms = dlSymsInit(name);
    int count = dlSymsCount(syms);
    for (int i = 0; i < count; ++i) {
        const char * symbolName = dlSymsName(syms, i);
        if (strlen(symbolName))
            av_push(RETVAL, newSVpv(symbolName, 0));
    }
    dlSymsCleanup(syms);
    safefree(name);
    ST(0) = newRV_noinc(MUTABLE_SV(RETVAL));
    XSRETURN(1);
}

XS_INTERNAL(Affix_find_symbol) {
    dVAR;
    dXSARGS;
    if (items != 2)
        croak_xs_usage(cv, "$lib, $symbol");
    DLLib * lib = NULL;
    {
        if (!SvOK(ST(0)) && SvREADONLY(ST(0)))  // explicit undef
            lib = load_library(NULL);
        else if (sv_isobject(ST(0)) && sv_derived_from(ST(0), "Affix::Lib")) {
            IV tmp = SvIV((SV *)SvRV(ST(0)));
            lib = INT2PTR(DLLib *, tmp);
        }
        else if (SvPOK(ST(0)) || (sv_isobject(ST(0)) && sv_derived_from(ST(0), "Path::Tiny")))
            lib = load_library(SvPV_nolen(ST(0)));
        if (lib == NULL)
            XSRETURN_UNDEF;
    }

    DCpointer lib_handle = find_symbol(lib, SvPV_nolen(ST(1)));
    if (!lib_handle) {
        croak("Failed to load lib %s", dlerror());
    }
    SV * LIBSV = sv_newmortal();
    sv_setref_pv(LIBSV, NULL, (DCpointer)lib_handle);
    ST(0) = LIBSV;
    XSRETURN(1);
}

XS_INTERNAL(Affix_free_library) {
    dVAR;
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "lib");
    DLLib * lib;
    //~ if (sv_derived_from(ST(0), "Affix::Lib")) {
    if (SvROK(ST(0))) {
        IV tmp = SvIV((SV *)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        croak("lib is not of type Affix::Lib");
    if (lib != NULL)
        free_library(lib);
    lib = NULL;
    XSRETURN_EMPTY;
}

void destroy_affix(pTHX_ Affix * affix) {
    if (affix == NULL)
        return;
    if (affix->push != NULL) {
        for (Stack_off_t i = 0; i < affix->instructions; i++)
            destroy_Affix_Type(aTHX_ affix->push[i]);
        safefree(affix->push);
    }
    affix->push = NULL;
    if (affix->pop != NULL)
        destroy_Affix_Type(aTHX_ affix->pop);
    affix->pop = NULL;
    if (affix->lvalues != NULL) {
        for (size_t i = 0; i < affix->lvalue_count; i++) {
            if (affix->lvalues[i] != NULL)
                safefree(affix->lvalues[i]);
            affix->lvalues[i] = NULL;
        }
        safefree(affix->lvalues);
        affix->lvalues = NULL;
    }
    safefree(affix);
    affix = NULL;
}

int xxxxx(DCCallVM * xvm, DCpointer symbol, DCaggr * aggr) {
    
    DCCallVM * vm = dcNewCallVM(32768);
    dcReset(vm);
    // dcMode(vm, DC_CALL_C_DEFAULT);

    struct S xs = {//  { 56, -23, 0 },
                  // 'q',
                  //   -6.28,
                  false,
                  true};

    DCaggr * a = dcNewAggr(2, sizeof(struct S));
    //    dcAggrField(a, DC_SIGCHAR_CHAR,   offsetof(struct S, x), 3);
    //    dcAggrField(a, DC_SIGCHAR_CHAR,   offsetof(struct S, x), 1);
    // dcAggrField(a, DC_SIGCHAR_DOUBLE, offsetof(struct S, y), 1);
    dcAggrField(a, DC_SIGCHAR_BOOL, offsetof(struct S, z), 1);
    dcAggrField(a, DC_SIGCHAR_BOOL, offsetof(struct S, q), 1);
    dcCloseAggr(a);

    // dcMode(vm, DC_CALL_C_DEFAULT);
    dcBeginCallAggr(vm, a);

    //~ dcArgInt(vm, 999);
    // dcArgAggr(vm, a, &s);
    DCpointer s = NULL;
    Newxz(s, 2, bool);
    bool tf = true;
    Copy(&tf, s, 1, bool);
    dcArgAggr(vm, a, s);


    //~ dcCallVoid(vm, (DCpointer)&process_bool_struct);
    // dcCallVoid(vm, symbol);
    DCpointer ptr = safemalloc(sizeof (struct S));
    ptr =  dcCallAggr(vm, (DCpointer)process_bool_struct, a, ptr);


    dcFreeAggr(a);

    return;


    // 1. Initialize DCState
    if (!vm) {
        fprintf(stderr, "Error: Could not create DCCallVM.\n");
        return 1;
    }

    symbol = &process_bool_struct;

    dcMode(vm, DC_CALL_C_DEFAULT);

    // 2. Define the aggregate for MyBoolStruct
    DCaggr * myBoolStructAggr = dcNewAggr(2, sizeof(MyBoolStruct));

    // Add fields to the aggregate definition.
    dcAggrField(myBoolStructAggr, DC_SIGCHAR_BOOL, offsetof(MyBoolStruct, value1), 1);
    dcAggrField(myBoolStructAggr, DC_SIGCHAR_BOOL, offsetof(MyBoolStruct, value2), 1);

    // Close the aggregate definition. This finalizes it.
    dcCloseAggr(myBoolStructAggr);

    dcReset(vm);  // Reset the dyncall stack for arguments for this call.

    // 5. Perform the dynamic call, expecting an aggregate return value
    // Initiate the call with dcBeginCallAggr, specifying the return aggregate type.
    dcBeginCallAggr(vm, myBoolStructAggr);

    // 3. Prepare the input struct
    MyBoolStruct originalStruct = {true, true};
    printf("Original struct in main:\n");
    printf("  value1: %s\n", originalStruct.value1 ? "true" : "false");
    printf("  value2: %s\n", originalStruct.value2 ? "true" : "false");

    // 4. Push arguments onto the dyncall stack

    // Use dcArgAggr to push the entire struct using its aggregate definition.
    dcArgAggr(vm, myBoolStructAggr, &originalStruct);

    DCpointer ret = NULL;
    // Execute the call using dcCallAggr.

    // dcCallAggr(vm, symbol, myBoolStructAggr, &returnedStruct);
    dcCallVoid(vm, symbol);

    // 6. Retrieve and print the returned struct
    printf("\nReturned struct in main:\n");
    printf("  value1: %s\n", returnedStruct.value1 ? "true" : "false");
    printf("  value2: %s\n", returnedStruct.value2 ? "true" : "false");

    // 7. Cleanup
    dcFreeAggr(myBoolStructAggr);  // Free the aggregate definition
    // dcFree(vm); // Free the call VM

    return 0;
}













extern void _Affix_trigger(pTHX_ CV * cv) {
    dXSARGS;
    Affix * affix = (Affix *)XSANY.any_ptr;
    if (UNLIKELY(items != affix->args)) {
        const GV * gv = CvNAMED(cv) ? NULL : cv->sv_any->xcv_gv_u.xcv_gv;
        const HV * const stash = GvSTASH(gv);
        if (HvNAME_get(stash))
            Perl_croak_nocontext("Wrong number of arguments to %" HEKf "::%" HEKf " expected: %" IVdf ", found %d",
                                 HEKfARG(HvNAME_HEK(stash)),
                                 HEKfARG(GvNAME_HEK(gv)),
                                 affix->args,
                                 items);
        else
            Perl_croak_nocontext("Wrong number of arguments to %" HEKf " expected: %" IVdf ", found %d",
                                 HEKfARG(GvNAME_HEK(gv)),
                                 affix->args,
                                 items);
    }
#ifndef AFFIX_PROFILE
    dMY_CXT;
    DCCallVM * cvm = MY_CXT.cvm;
#else
    DCCallVM * cvm = dcNewCallVM(1024 * 8);
    dcMode(cvm, DC_CALL_C_DEFAULT);
    ST(0) = newSVnv(100.0);
#endif

    // return;
    for (Stack_off_t st = 0, instruction = 0; instruction < affix->instructions; instruction++)
        st += affix->push[instruction]->pass(aTHX_ affix,
                                             affix->push[instruction],
                                             cvm,
                                             ax +
#ifndef AFFIX_PROFILE
                                                 st
#else
                                                 0
#endif
        );
    PL_stack_sp = PL_stack_base + ax +
        (((ST(0) = affix->pop->call(aTHX_ affix, affix->pop, cvm, affix->symbol)) != NULL) ? 0 : -1);   
        return;
}

XS_INTERNAL(Affix_affix) {
    // ix == 0 if Affix::affix
    // ix == 1 if Affix::wrap
    dXSARGS;
    dXSI32;
    PERL_UNUSED_VAR(items);
    Affix * affix = NULL;
    char *prototype = NULL, *rename = NULL;
    STRLEN len;
    DLLib * libhandle = NULL;
    DCpointer funcptr = NULL;
    SV * const xsub_tmp_sv = ST(0);
    SvGETMAGIC(xsub_tmp_sv);
    if (!SvOK(xsub_tmp_sv) && SvREADONLY(xsub_tmp_sv))  // explicit undef
        libhandle = load_library(NULL);
    else if (sv_isobject(xsub_tmp_sv) && sv_derived_from(xsub_tmp_sv, "Affix::Lib")) {
        IV tmp = SvIV((SV *)SvRV(xsub_tmp_sv));
        libhandle = INT2PTR(DLLib *, tmp);
    }
    else if (NULL == (libhandle = load_library(SvPV_nolen(xsub_tmp_sv)))) {
        Stat_t statbuf;
        Zero(&statbuf, 1, Stat_t);
        if (PerlLIO_stat(SvPV_nolen(xsub_tmp_sv), &statbuf) < 0) {
            ENTER;
            SAVETMPS;
            PUSHMARK(SP);
            XPUSHs(xsub_tmp_sv);
            PUTBACK;
            SSize_t count = call_pv("Affix::find_library", G_SCALAR);
            SPAGAIN;
            if (count == 1)
                libhandle = load_library(SvPV_nolen(POPs));
            PUTBACK;
            FREETMPS;
            LEAVE;
        }
    }

    if (libhandle == NULL)
        croak("Failed to load %s", SvPVbyte_or_null(ST(0), len));
    if ((SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV) && av_count(MUTABLE_AV(SvRV(ST(1)))) == 2) {
        rename = SvPVbyte_or_null(*av_fetch(MUTABLE_AV(SvRV(ST(1))), 1, 0), len);
        funcptr = find_symbol(libhandle, SvPVbyte_or_null(*av_fetch(MUTABLE_AV(SvRV(ST(1))), 0, 0), len));
    }
    else {
        rename = SvPVbyte_or_null(ST(1), len);
        funcptr = find_symbol(libhandle, SvPVbyte_or_null(ST(1), len));
    }
    if (funcptr == NULL)
        croak("Failed to locate %s in %s", SvPVbyte_or_null(ST(1), len), SvPVbyte_or_null(ST(0), len));
    if (!(SvROK(ST(2)) && SvTYPE(SvRV(ST(2))) == SVt_PVAV))
        croak("Expected args as a list of types");
    Newxz(affix, 1, Affix);
    affix->pop = new_Affix_Type(aTHX_ ST(3));
    if (affix == NULL)
        XSRETURN_EMPTY;
    //~ PING;
    //~ DD(ST(2));
    //~ affix->push[0] = (aTHX_ new Affix_Reset());
    AV * args = MUTABLE_AV(SvRV(ST(2)));

    size_t count = av_count(args);
    {
        //~ warn("affix->instructions: %d", affix->instructions);


        bool aggr_ret =
            (affix->pop->type == STRUCT_FLAG || affix->pop->type == CPPSTRUCT_FLAG || affix->pop->type == UNION_FLAG)
            ? true
            : false;

        Newxz(affix->push, count + 1 + (aggr_ret ? 1 : 0), Affix_Type *);


        //~ Newxz(affix->push, count + 1 + (affix->pop->type == DC_SIGCHAR_AGGREGATE ? 1 : 0), Affix_Type *);
        affix->push[affix->instructions++] = _reset();  // Off on the right foot
        if (aggr_ret)
            affix->push[affix->instructions++] = _aggr();
        Newxz(prototype, count + 1, char);
        for (size_t instruction = 0; instruction < count; instruction++) {
            affix->push[affix->instructions] = new_Affix_Type(aTHX_ * av_fetch(args, instruction, 0));

            //~ DD(*av_fetch(args, instruction - 1, 0));
            //~ warn(">>> st: 0, type: %c, instruction: %u, affix->instructions: %u",
            //~ affix->push[affix->instructions]->type,
            //~ instruction,
            //~ count);

            if (affix->push[affix->instructions] == NULL) {
                sv_dump(*av_fetch(args, instruction - 1, 0));
                croak("Oh, man...");
            }

            //~ warn("instruction %d is %d [%c]", i, affix->push[i]->type, affix->push[i]->type);
            //~ warn("affix->push[%d]->type: %c [%d]", i, affix->push[i]->type, affix->push[i]->type);
            //~ switch() {
            Affix_Type * hold = affix->push[affix->instructions];
            //~ }
            switch (hold->type) {

            case RESET_FLAG:
            case MODE_FLAG:
                /*
                #define  ''  //
                #define THIS_FLAG '*'
                #define ELLIPSIS_FLAG 'e'
                #define VARARGS_FLAG '.'
                #define CDECL_FLAG 'D'
                #define STDCALL_FLAG 'T'
                #define MSFASTCALL_FLAG '='
                #define GNUFASTCALL_FLAG '3'  //
                #define MSTHIS_FLAG '+'
                #define GNUTHIS_FLAG '#'  //
                #define ARM_FLAG 'r'
                #define THUMB_FLAG 'g'  // ARM
                #define SYSCALL_FLAG 'H'*/
                break;
            default:
                prototype[affix->args++] = '$';  // TODO: proper prototype

                break;
            }
            affix->instructions++;
            //~ if (affix->push[i]->type != RESET_FLAG && affix->push[i]->type != MODE_FLAG)
        }
        //~ warn("prototype: '%s'", prototype);
    }
    affix->symbol = funcptr;
    STMT_START {
        cv = newXSproto_portable(ix == 0 ? rename : NULL, _Affix_trigger, __FILE__, prototype);
        if (UNLIKELY(cv == NULL))
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (DCpointer)affix;
    }
    STMT_END;
    ST(0) = sv_2mortal(sv_bless((UNLIKELY(ix == 1) ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                                gv_stashpv("Affix", GV_ADD)));
    safefree(prototype);
    XSRETURN(1);
}

XS_INTERNAL(Affix_DESTROY) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    Affix * affix;

    STMT_START {  // peel this grape
        HV * st;
        GV * gvp;
        SV * const xsub_tmp_sv = ST(0);
        SvGETMAGIC(xsub_tmp_sv);
        CV * cv = sv_2cv(xsub_tmp_sv, &st, &gvp, 0);
        affix = (Affix *)XSANY.any_ptr;
    }
    STMT_END;
    destroy_affix(aTHX_ affix);
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_END) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    dMY_CXT;
    if (MY_CXT.cvm) {
        dcReset(MY_CXT.cvm);
        dcFree(MY_CXT.cvm);
    }
    XSRETURN_EMPTY;
}

// Utils
XS_INTERNAL(Affix_sv_dump) {
    dXSARGS;
    if (items != 1)
        croak_xs_usage(cv, "$sv");
    sv_dump(ST(0));
    XSRETURN_EMPTY;
}

XS_INTERNAL(Affix_gen_dualvar) {
    Stack_off_t ax = 0;
    // dAXMARK;  // ST(...)
    ST(0) = sv_2mortal(gen_dualvar(aTHX_ SvIV(ST(0)), SvPVbyte_nolen(ST(1))));
    XSRETURN(1);
}

// Cribbed from Perl::Destruct::Level so leak testing works without yet another prereq
XS_INTERNAL(Affix_set_destruct_level) {
    dXSARGS;
    // TODO: report this with a warn(...)
    if (items != 1)
        croak_xs_usage(cv, "level");
    PL_perl_destruct_level = SvIV(ST(0));
    XSRETURN_EMPTY;
}


XS_INTERNAL(Affix_jitter);


void boot_Affix(pTHX_ CV * cv) {
    dXSBOOTARGSXSAPIVERCHK;
    PERL_UNUSED_VAR(items);
    // PERL_UNUSED_VAR(items);
#ifdef USE_ITHREADS  // Windows...
    my_perl = (PerlInterpreter *)PERL_GET_CONTEXT;
#endif
    MY_CXT_INIT;
    // Allow user defined value in a BEGIN{ } block
    SV * vmsize = get_sv("Affix::VMSize", 0);
    MY_CXT.cvm = dcNewCallVM(vmsize == NULL ? 8192 : SvIV(vmsize));
    dcMode(MY_CXT.cvm, DC_CALL_C_DEFAULT);
    // Start exposing API
    // Affix::affix( lib, symbol, [args], return )
    //             ( [lib, version], symbol, [args], return )
    //             ( lib, [symbol, name], [args], return )
    //             ( [lib, version], [symbol, name], [args], return )
    cv = newXSproto_portable("Affix::affix", Affix_affix, __FILE__, "$$$$");
    XSANY.any_i32 = 0;
    export_function("Affix", "affix", "core");
    // Affix::wrap(  lib, symbol, [args], return )
    //             ( [lib, version], symbol, [args], return )
    cv = newXSproto_portable("Affix::wrap", Affix_affix, __FILE__, "$$$$");
    XSANY.any_i32 = 1;
    export_function("Affix", "wrap", "core");

    (void)newXSproto_portable("Affix::DESTROY", Affix_DESTROY, __FILE__, "$;$");
    (void)newXSproto_portable("Affix::END", Affix_END, __FILE__, "");

    (void)newXSproto_portable("Affix::set_destruct_level", Affix_set_destruct_level, __FILE__, "$");
    (void)newXSproto_portable("Affix::sv_dump", Affix_sv_dump, __FILE__, "$");
    (void)newXSproto_portable("Affix::gen_dualvar", Affix_gen_dualvar, __FILE__, "$$");

    (void)newXSproto_portable("Affix::load_library", Affix_load_library, __FILE__, "$");
    (void)newXSproto_portable("Affix::free_library", Affix_free_library, __FILE__, "$;$");
    (void)newXSproto_portable("Affix::list_symbols", Affix_list_symbols, __FILE__, "$");
    (void)newXSproto_portable("Affix::find_symbol", Affix_find_symbol, __FILE__, "$$");
    (void)newXSproto_portable("Affix::dlerror", Affix_dlerror, __FILE__, "");


    // Jitter
    cv = newXSproto_portable("Affix::jitter", Affix_jitter, __FILE__, "$$$$");
    XSANY.any_i32 = 0;
    // export_function("Affix", "affix", "core");
    // Affix::wrap(  lib, symbol, [args], return )
    //             ( [lib, version], symbol, [args], return )
    cv = newXSproto_portable("Affix::jitter_wrap", Affix_jitter, __FILE__, "$$$$");
    XSANY.any_i32 = 1;
    // export_function("Affix", "wrap", "core");

    // boot other packages
    //~ boot_Affix_Lib(aTHX_ cv);
    boot_Affix_Platform(aTHX_ cv);
    boot_Affix_Pointer(aTHX_ cv);
    boot_Affix_pin(aTHX_ cv);
    //~ boot_Affix_Callback(aTHX_ cv);
    boot_Affix_Type(aTHX_ cv);

    //
    Perl_xs_boot_epilog(aTHX_ ax);
}



#include "Affix/platform.h"


// Platform-specific includes for memory allocation
#if defined(PLATFORM_OS_WINDOWS)
#include <windows.h> // For VirtualAlloc, VirtualFree
#else
#include <sys/mman.h> // For mmap, munmap, mprotect (Linux/Unix-like systems)
#include <unistd.h>   // For sysconf (_SC_PAGESIZE)
#endif


// No, wait, this is the goods.
typedef struct _Jitter {
    size_t lvalue_count, args;
    DCpointer symbol;
    Affix_Type ** push;
    Affix_Type * pop;
    DCpointer * lvalues;
    Stack_off_t instructions;
    //
    DCpointer jit;
    size_t jit_len;
} Jitter;


/*
 * allocate_executable_memory(Jitter * jitter, char* code_bytes, STRLEN code_len, void** mem_out)
 *
 * Helper function to allocate memory with execute, read, and write permissions.
 * This is crucial for JIT compilation.
 *
 * Arguments:
 * jitter
 * code_bytes: A pointer to the raw machine code bytes to be copied.
 * code_len: The length of the machine code in bytes.
 *
 * Returns:
 * 0 on success, -1 on failure.
 * /
int allocate_executable_memory(pTHX_ Jitter * jitter, char* code_bytes, size_t code_len) {
#if defined(PLATFORM_OS_WINDOWS) // Windows: Use VirtualAlloc for memory allocation with specific protections.
    jitter->jit = VirtualAlloc(NULL, code_len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (jitter->jit == NULL) 
        croak("VirtualAlloc failed. Error: %lu", GetLastError());
#else // Linux, *BSD, macOS, and other POSIX-like systems: Use mmap.
    jitter->jit = mmap(NULL, code_len, PROT_READ | PROT_WRITE | PROT_EXEC, JIT_MMAP_FLAGS, -1, 0);
    if (jitter->jit == MAP_FAILED) 
        croak("mmap failed during jit init.");   
    #if PLATFORM_OS_MACOS_ARM64
        // On Apple Silicon, disable write protection before writing, then re-enable.
        pthread_jit_write_protect_np(0);
    #endif 
#endif    
    memcpy(jitter->jit, code_bytes, code_len);// Copy the machine code into the newly allocated executable buffer.
#if defined(PLATFORM_OS_MACOS_ARM64)
    // On Apple Silicon, re-enable write protection and flush the instruction cache.
    pthread_jit_write_protect_np(1);
    sys_icache_invalidate(jitter->jit, code_len);
#elif defined(PLATFORM_OS_LINUX) && defined(PLATFORM_ARCH_ARM64)
    // For other ARM64 systems (like Linux), use GCC/Clang builtins to clear the cache.
    __builtin___clear_cache((char*)jitter->jit, (char*)jitter->jit + code_len);
#endif
    // Attempt to lock memory to prevent swapping (optional, requires privileges)
#if defined(PLATFORM_OS_WINDOWS)
    if (!VirtualLock(jitter->jit, code_len)) 
        warn("Warning: VirtualLock failed. Error: %lu. Memory may be swapped.", GetLastError());
#else
    if (mlock(jitter->jit, code_len) != 0) 
        croak("Warning: mlock failed. Memory may be swapped.");
#endif
    // Remove write permissions after copying the code for security.
#if defined(PLATFORM_OS_WINDOWS)
    DWORD old_protect;
    if (!VirtualProtect(jitter->jit, code_len, PAGE_EXECUTE_READ, &old_protect)) 
        croak("VirtualProtect failed to remove write permissions. Error: %lu", GetLastError());
#elif PLATFORM_OS_MACOS_ARM64
    // On Apple Silicon, pthread_jit_write_protect_np already handles this.
    // No additional mprotect needed here.
#else
    if (mprotect(jitter->jit, code_len, PROT_READ | PROT_EXEC) != 0) 
        croak("mprotect failed to remove write permissions.");
#endif
    return 0;
}

*/

// --- Consolidated Platform-Specific Instruction Emitters ---
// These functions use #ifdef blocks internally to emit the correct machine code
// for the target architecture.






#if defined (PLATFORM_ARCH_AMD64)
#elif defined(PLATFORM_ARCH_ARM64)
#else
#endif


// Define the signature of the generated trampoline function.
// It takes a void** array of argument pointers and returns a void*.
// NOTE: The return type of the trampoline itself is always void* for simplicity
// in this example, as it's a generic machine code stub. The actual C function
// it calls will return its specific type.
typedef void* (*GeneratedTrampolineFunc)(void** args_array);

// --- Low-Level Assembler Builder Helper Functions ---
// These functions append raw bytes to the code buffer.
// Endianness: These functions append in little-endian byte order,
// which is standard for x86-64 and modern ARM64 systems.

// Helper to ensure buffer has enough capacity and append a byte
void _append_byte(unsigned char** buffer, size_t* current_size, size_t* capacity, unsigned char byte) {
    if (*current_size >= *capacity) {
        *capacity *= 2;
        *buffer = (unsigned char*)realloc(*buffer, *capacity);
        if (*buffer == NULL) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
    }
    (*buffer)[(*current_size)++] = byte;
}

// Helper to append a sequence of bytes
void _append_bytes(unsigned char** buffer, size_t* current_size, size_t* capacity, const unsigned char* bytes, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        _append_byte(buffer, current_size, capacity, bytes[i]);
    }
}

// Helper to append a 64-bit unsigned integer (little-endian)
void _append_uint64(unsigned char** buffer, size_t* current_size, size_t* capacity, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        _append_byte(buffer, current_size, capacity, (unsigned char)((val >> (i * 8)) & 0xFF));
    }
}

// Helper to append a 32-bit unsigned integer (little-endian)
void _append_uint32(unsigned char** buffer, size_t* current_size, size_t* capacity, uint32_t val) {
    for (int i = 0; i < 4; ++i) {
        _append_byte(buffer, current_size, capacity, (unsigned char)((val >> (i * 8)) & 0xFF));
    }
}

// --- Consolidated Platform-Specific Instruction Emitters ---
// These functions use #ifdef blocks internally to emit the correct machine code
// for the target architecture.

#if defined(PLATFORM_ARCH_AMD64)
    // x86-64 Register Definitions (ModR/M numbers)
    enum X64_GPR_REG {
        X64_REG_RAX = 0, X64_REG_RCX = 1, X64_REG_RDX = 2, X64_REG_RBX = 3,
        X64_REG_RSP = 4, X64_REG_RBP = 5, X64_REG_RSI = 6, X64_REG_RDI = 7,
        // R8-R15 require REX.B bit set (0x01) and their value 0-7
        X64_REG_R8_MODRM = 0,  X64_REG_R9_MODRM = 1,  X64_REG_R10_MODRM = 2, X64_REG_R11_MODRM = 3,
        X64_REG_R12_MODRM = 4, X64_REG_R13_MODRM = 5, X64_REG_R14_MODRM = 6, X64_REG_R15_MODRM = 7
    };

    // x86-64 XMM Register Definitions
    enum X64_XMM_REG {
        X64_XMM0 = 0, X64_XMM1 = 1, X64_XMM2 = 2, X64_XMM3 = 3,
        X64_XMM4 = 4, X64_XMM5 = 5, X64_XMM6 = 6, X64_XMM7 = 7
    };

    // Argument registers (integer/pointer) for different x64 ABIs
    static const unsigned char x64_linux_int_arg_regs[] = { X64_REG_RDI, X64_REG_RSI, X64_REG_RDX, X64_REG_RCX, X64_REG_R8_MODRM, X64_REG_R9_MODRM };
    static const unsigned char x64_windows_int_arg_regs[] = { X64_REG_RCX, X64_REG_RDX, X64_REG_R8_MODRM, X64_REG_R9_MODRM };
    static const int X64_LINUX_MAX_INT_ARGS = sizeof(x64_linux_int_arg_regs) / sizeof(x64_linux_int_arg_regs[0]);
    static const int X64_WINDOWS_MAX_INT_ARGS = sizeof(x64_windows_int_arg_regs) / sizeof(x64_windows_int_arg_regs[0]);

    // Floating-point argument registers for x64 ABIs
    static const unsigned char x64_float_arg_regs[] = { X64_XMM0, X64_XMM1, X64_XMM2, X64_XMM3, X64_XMM4, X64_XMM5, X64_XMM6, X64_XMM7 };
    static const int X64_LINUX_MAX_FLOAT_ARGS = sizeof(x64_float_arg_regs) / sizeof(x64_float_arg_regs[0]);
    static const int X64_WINDOWS_MAX_FLOAT_ARGS = 4; // Windows x64 only uses XMM0-XMM3 for float args

#elif defined(PLATFORM_ARCH_ARM64) // Use PLATFORM_ARCH_ARM64 from platform.h
    // ARM64 Register Definitions
    enum ARM64_GPR_REG {
        ARM64_REG_X0_MODRM = 0, ARM64_REG_X1_MODRM = 1, ARM64_REG_X2_MODRM = 2, ARM64_REG_X3_MODRM = 3,
        ARM64_REG_X4_MODRM = 4, ARM64_REG_X5_MODRM = 5, ARM64_REG_X6_MODRM = 6, ARM64_REG_X7_MODRM = 7,
        ARM64_REG_X9_MODRM = 9, ARM64_REG_X10_MODRM = 10, ARM64_REG_X16_MODRM = 16, ARM64_REG_X30_MODRM = 30 // LR - Link Register
    };

    // ARM64 V (Floating-point) Register Definitions
    enum ARM64_V_REG {
        ARM64_V0 = 0, ARM64_V1 = 1, ARM64_V2 = 2, ARM64_V3 = 3,
        ARM64_V4 = 4, ARM64_V5 = 5, ARM64_V6 = 6, ARM64_V7 = 7
    };

    // Argument registers (integer/pointer) for ARM64 ABI
    static const unsigned char arm64_int_arg_regs[] = {
        ARM64_REG_X0_MODRM, ARM64_REG_X1_MODRM, ARM64_REG_X2_MODRM, ARM64_REG_X3_MODRM,
        ARM64_REG_X4_MODRM, ARM64_REG_X5_MODRM, ARM64_REG_X6_MODRM, ARM64_REG_X7_MODRM
    };
    static const int ARM64_MAX_INT_ARGS = sizeof(arm64_int_arg_regs) / sizeof(arm64_int_arg_regs[0]);

    // Floating-point argument registers for ARM64 ABI
    static const unsigned char arm64_float_arg_regs[] = { ARM64_V0, ARM64_V1, ARM64_V2, ARM64_V3, ARM64_V4, ARM64_V5, ARM64_V6, ARM64_V7 };
    static const int ARM64_MAX_FLOAT_ARGS = sizeof(arm64_float_arg_regs) / sizeof(arm64_float_arg_regs[0]);
#endif


// Emits `movabs reg, imm64` (move 64-bit immediate to 64-bit register)
// For x86-64: REX.W + (REX.B if reg is R8-R15) + B8+reg (where reg is 0-7) + imm64
// This function is specialized for the target function address register (R10 on x64, X16 on ARM64)
void emit_load_target_addr_instr(unsigned char** buffer, size_t* current_size, size_t* capacity, uint64_t target_addr) {
#if defined(PLATFORM_ARCH_AMD64)
    unsigned char reg_num = X64_REG_R10_MODRM; // Target function address goes to R10
    unsigned char rex_byte = 0x08; // REX.W (64-bit operand size)
    // R10 is an extended register, so REX.B is needed. Its ModR/M value is 2.
    rex_byte |= 0x01; // Set REX.B bit
    _append_byte(buffer, current_size, capacity, 0x40|rex_byte);
    _append_byte(buffer, current_size, capacity, 0xB8 + reg_num); // B8+reg is mov reg, imm64
    _append_uint64(buffer, current_size, capacity, target_addr);
#elif defined(PLATFORM_ARCH_ARM64)
    // LDR X16, [PC, #offset] (load X16 from PC-relative literal pool)
    // The actual 64-bit literal will be appended immediately after the BLR instruction.
    // The offset for LDR is calculated from the PC of the LDR instruction to the literal.
    // If literal is immediately after BLR, and BLR is immediately after LDR,
    // then literal is 4 bytes (BLR) + 4 bytes (LDR itself) = 8 bytes from LDR's PC.
    // So offset is 8.
    uint32_t instr = 0x58000000; // Base opcode for LDR (literal)
    instr |= (ARM64_REG_X16_MODRM & 0x1F);   // bits 4:0 for Rt (destination register)
    instr |= ((8 / 4) & 0x7FFFF) << 5; // bits 23:5 for immediate (scaled by 4)
    _append_uint32(buffer, current_size, capacity, instr);
#else
    fprintf(stderr, "Error: emit_load_target_addr_instr not implemented for this architecture.\n");
    exit(EXIT_FAILURE);
#endif
}

// Emits `mov reg_dest, [reg_src + offset]` (load from memory with size)
// For x86-64: Uses MOV r/m, r opcodes with size prefixes.
// For ARM64: Uses LDR Xd, [Xn, #imm12 * scale] with appropriate register and scale.
void emit_load_gpr_from_mem_offset_sized(unsigned char** buffer, size_t* current_size, size_t* capacity,
                                         unsigned char reg_dest_num, unsigned char reg_src_num, int32_t offset, size_t data_size) {
#if defined(PLATFORM_ARCH_AMD64)
    unsigned char rex_byte = 0x00; // Base REX prefix
    unsigned char opcode = 0x8B;   // MOV r/m, r (general purpose)

    // Handle REX prefix for extended registers (R8-R15)
    if (reg_dest_num >= 8) { rex_byte |= 0x04; reg_dest_num -= 8; } // REX.R for dest
    if (reg_src_num >= 8) { rex_byte |= 0x01; reg_src_num -= 8; }  // REX.B for src

    switch (data_size) {
        case 1: // byte
            rex_byte &= ~0x08; // Clear REX.W (not 64-bit operand)
            opcode = 0x8A; // MOV r8, r/m8
            break;
        case 2: // word
            _append_byte(buffer, current_size, capacity, 0x66); // Operand-size override prefix (for 16-bit)
            rex_byte &= ~0x08; // Clear REX.W
            opcode = 0x8B; // MOV r16, r/m16
            break;
        case 4: // dword
            rex_byte &= ~0x08; // Clear REX.W
            opcode = 0x8B; // MOV r32, r/m32
            break;
        case 8: // qword
            rex_byte |= 0x08; // Set REX.W (64-bit operand size)
            opcode = 0x8B; // MOV r64, r/m64
            break;
        default:
            fprintf(stderr, "Error: Unsupported GPR load size %zu for x86-64.\n", data_size);
            exit(EXIT_FAILURE);
    }

    if (rex_byte != 0x00) { // Only append REX if necessary
        _append_byte(buffer, current_size, capacity, 0x40 | rex_byte);
    }
    _append_byte(buffer, current_size, capacity, opcode);
    // ModR/M byte: Mod=10 (disp32), Reg=reg_dest_num, R/M=reg_src_num
    _append_byte(buffer, current_size, capacity, 0x80 | (reg_dest_num << 3) | (reg_src_num));
    _append_uint32(buffer, current_size, capacity, offset); // 32-bit displacement
#elif defined(PLATFORM_ARCH_ARM64)
    uint32_t instr;
    int scale = 0; // Scale for immediate offset

    switch (data_size) {
        case 1: // byte (LDRB)
            instr = 0x38400000; // LDRB Wt, [Xn, #imm]
            scale = 1;
            break;
        case 2: // halfword (LDRH)
            instr = 0x78400000; // LDRH Wt, [Xn, #imm]
            scale = 2;
            break;
        case 4: // word (LDR)
            instr = 0xB9400000; // LDR Wt, [Xn, #imm]
            scale = 4;
            break;
        case 8: // doubleword (LDR)
            instr = 0xF9400000; // LDR Xt, [Xn, #imm]
            scale = 8;
            break;
        default:
            fprintf(stderr, "Error: Unsupported GPR load size %zu for ARM64.\n", data_size);
            exit(EXIT_FAILURE);
    }

    instr |= (reg_dest_num & 0x1F); // Rt
    instr |= (reg_src_num & 0x1F) << 5; // Rn
    instr |= ((offset / scale) & 0xFFF) << 10; // imm12, scaled
    _append_uint32(buffer, current_size, capacity, instr);
#else
    fprintf(stderr, "Error: emit_load_gpr_from_mem_offset_sized not implemented for this architecture.\n");
    exit(EXIT_FAILURE);
#endif
}

// Emits `movss/movsd xmm_dest, [reg_src + offset]` (load float/double from memory)
void emit_load_float_reg_from_mem_offset(unsigned char** buffer, size_t* current_size, size_t* capacity,
                                         unsigned char xmm_dest_num, unsigned char reg_src_num, int32_t offset, size_t data_size) {
#if defined(PLATFORM_ARCH_AMD64)
    unsigned char rex_byte = 0x40; // Base REX prefix
    if (xmm_dest_num >= 8) { rex_byte |= 0x04; xmm_dest_num -= 8; } // REX.R for XMM dest
    if (reg_src_num >= 8) { rex_byte |= 0x01; reg_src_num -= 8; }  // REX.B for GPR src

    if (data_size == sizeof(double)) { // movsd
        _append_byte(buffer, current_size, capacity, 0x40|rex_byte ); 
        _append_byte(buffer, current_size, capacity, 0xF2); // Mandatory prefix for movsd
        _append_byte(buffer, current_size, capacity, 0x0F); // SSE opcode prefix
        _append_byte(buffer, current_size, capacity, 0x10); // movsd opcode
    } else if (data_size == sizeof(float)) { // movss
        _append_byte(buffer, current_size, capacity, 0x40 | rex_byte); 
        _append_byte(buffer, current_size, capacity, 0xF3); // Mandatory prefix for movss
        _append_byte(buffer, current_size, capacity, 0x0F); // SSE opcode prefix
        _append_byte(buffer, current_size, capacity, 0x10); // movss opcode
    } else {
        fprintf(stderr, "Error: Unsupported float size %zu for x86-64 float load.\n", data_size);
        exit(EXIT_FAILURE);
    }

    // ModR/M byte: Mod=10 (disp32), Reg=xmm_dest_num, R/M=reg_src_num
    _append_byte(buffer, current_size, capacity, 0x80 | (xmm_dest_num << 3) | (reg_src_num));
    _append_uint32(buffer, current_size, capacity, offset); // 32-bit displacement
#elif defined(PLATFORM_ARCH_ARM64)
    uint32_t instr;
    if (data_size == sizeof(double)) { // LDR Dt, [Xn, #imm]
        instr = 0xF9400000; // Base for LDR Dt, [Xn, #imm] (scaled by 8)
    } else if (data_size == sizeof(float)) { // LDR St, [Xn, #imm]
        instr = 0xB9400000; // Base for LDR St, [Xn, #imm] (scaled by 4)
    } else {
        fprintf(stderr, "Error: Unsupported float size %zu for ARM64 float load.\n", data_size);
        exit(EXIT_FAILURE);
    }

    instr |= (xmm_dest_num & 0x1F); // Rt (destination register)
    instr |= (reg_src_num & 0x1F) << 5; // Rn (base register)
    instr |= ((offset / (data_size == sizeof(double) ? 8 : 4)) & 0xFFF) << 10; // imm12, scaled
    _append_uint32(buffer, current_size, capacity, instr);
#else
    fprintf(stderr, "Error: emit_load_float_reg_from_mem_offset not implemented for this architecture.\n");
    exit(EXIT_FAILURE);
#endif
}


// Emits `mov reg_dest, reg_src` (move one register to another)
// For x86-64: MOV r64, r64 (0x8B opcode) with ModR/M byte (Mod=11).
// For ARM64: ORR Rd, XZR, Rn (0x2A0003E0 base opcode)
void emit_mov_reg_reg(unsigned char** buffer, size_t* current_size, size_t* capacity,
                                 unsigned char reg_dest_num, unsigned char reg_src_num) {
#if defined(PLATFORM_ARCH_AMD64)
    unsigned char rex_byte = 0x00; 
    if (reg_dest_num >= 8) { rex_byte |= 0x04; reg_dest_num -= 8; } // REX.R for dest
    if (reg_src_num >= 8) { rex_byte |= 0x01; reg_src_num -= 8; }  // REX.B for src
   rex_byte |= 0x08; // Always set REX.W for 64-bit mov
    _append_byte(buffer, current_size, capacity, 0x40 | rex_byte);
    _append_byte(buffer, current_size, capacity, 0x8B); // MOV r64, r/m64
    // ModR/M byte: Mod=11 (register-direct), Reg=reg_dest_num, R/M=reg_src_num
    _append_byte(buffer, current_size, capacity, 0xC0 | (reg_dest_num << 3) | (reg_src_num));
#elif defined(PLATFORM_ARCH_ARM64)
    uint32_t instr = 0x2A0003E0; // Base for ORR with XZR (X0, XZR, X0)
    instr |= (reg_dest_num & 0x1F); // Rd
    instr |= ((0x1F) & 0x1F) << 16; // Rn (XZR)
    instr |= (reg_src_num & 0x1F) << 5; // Rm (source register)
    _append_uint32(buffer, current_size, capacity, instr);
#else
    fprintf(stderr, "Error: emit_mov_reg_reg not implemented for this architecture.\n");
    exit(EXIT_FAILURE);
#endif
}


// Emits `call reg` (call function at address in register)
// For x86-64: FF /2 (ModR/M byte with reg field 2)
// For ARM64: BLR reg (0xD63F0000 base opcode)
void emit_call_reg(unsigned char** buffer, size_t* current_size, size_t* capacity, unsigned char reg_num) {
#if defined(PLATFORM_ARCH_AMD64)
unsigned char rex_byte = 0x00; // Initialize to 0x00
    if (reg_num >= 8) { rex_byte |= 0x01; reg_num -= 8; } // REX.B for R/M field (the register being called)
    _append_byte(buffer, current_size, capacity, 0x40 | rex_byte);
    _append_byte(buffer, current_size, capacity, 0xFF);
    _append_byte(buffer, current_size, capacity, 0xD0 + reg_num); // 0xD0 is ModR/M for CALL RAX, etc.
#elif defined(PLATFORM_ARCH_ARM64)
    uint32_t instr = 0xD63F0000; // Base opcode for BLR
    instr |= (reg_num & 0x1F) << 5; // bits 9:5 for Rn (source register)
    _append_uint32(buffer, current_size, capacity, instr);
#else
    fprintf(stderr, "Error: emit_call_reg not implemented for this architecture.\n");
    exit(EXIT_FAILURE);
#endif
}

// Emits `ret` (return from function)
// For x86-64: C3 opcode
// For ARM64: D65F03C0 opcode
void emit_ret(unsigned char** buffer, size_t* current_size, size_t* capacity) {
#if defined(PLATFORM_ARCH_AMD64)
    _append_byte(buffer, current_size, capacity, 0xC3); // ret opcode
#elif defined(PLATFORM_ARCH_ARM64)
    _append_uint32(buffer, current_size, capacity, 0xD65F03C0); // ret opcode
#else
    fprintf(stderr, "Error: emit_ret not implemented for this architecture.\n");
    exit(EXIT_FAILURE);
#endif
}


// --- Main Trampoline Generator Function ---
// This function dynamically generates the trampoline machine code.
// The generated trampoline function will take 'void** args_array' as its
// only argument, pull values from it, and pass them to the target_func_ptr.
// It returns a void* (intended for integer/pointer return types).
//
// Arguments:
//   target_func_ptr: A pointer to the C function to be called.
//   return_type_affix: An Affix_Type pointer describing the return type of target_func_ptr.
//   arg_types_affix: An array of Affix_Type pointers, each describing an argument type.
//   num_args: The number of arguments in arg_types_affix.
GeneratedTrampolineFunc generate_trampoline(void* target_func_ptr, Affix_Type* return_type_affix, Affix_Type** arg_types_affix, int num_args) {
    size_t initial_capacity = 256; // Increased capacity for argument marshalling
    unsigned char* code_buffer = (unsigned char*)malloc(initial_capacity);
    size_t current_size = 0;
    size_t capacity = initial_capacity;

    if (code_buffer == NULL) {
        perror("malloc failed");
        return NULL;
    }

    // Debugging output
    printf("Attempting to generate trampoline for current platform...\n");

    // --- Platform-Specific Setup & Argument Register Management ---
#if defined(PLATFORM_ARCH_AMD64)
    #if defined(PLATFORM_OS_WINDOWS) // Use PLATFORM_OS_WINDOWS from platform.h
        printf("Platform: Windows x64\n");
        const unsigned char* current_int_arg_regs = x64_windows_int_arg_regs;
        const int MAX_INT_ARGS = X64_WINDOWS_MAX_INT_ARGS;
        const unsigned char* current_float_arg_regs = x64_float_arg_regs;
        const int MAX_FLOAT_ARGS = X64_WINDOWS_MAX_FLOAT_ARGS;
        const unsigned char TRAMPOLINE_ARGS_ARRAY_REG = X64_REG_RCX; // Trampoline's first arg (args_array)
        const unsigned char SCRATCH_REG_BASE = X64_REG_R11_MODRM; // Scratch for args_array pointer
        const unsigned char SCRATCH_REG_ARG_PTR = X64_REG_R12_MODRM; // Scratch for individual arg pointer
        const unsigned char TARGET_FUNC_REG = X64_REG_R10_MODRM; // Register to hold target function address (ModR/M value)

        // Save the args_array pointer from the trampoline's first argument register
        // into a scratch register, so the argument registers are free for the target call.
        // mov SCRATCH_REG_BASE, TRAMPOLINE_ARGS_ARRAY_REG
        emit_mov_reg_reg(&code_buffer, &current_size, &capacity, SCRATCH_REG_BASE, TRAMPOLINE_ARGS_ARRAY_REG);

    #elif defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_MACOS) // Added PLATFORM_OS_MACOS
        printf("Platform: Linux/macOS x64 (System V ABI)\n"); // Updated message
        const unsigned char* current_int_arg_regs = x64_linux_int_arg_regs;
        const int MAX_INT_ARGS = X64_LINUX_MAX_INT_ARGS;
        const unsigned char* current_float_arg_regs = x64_float_arg_regs;
        const int MAX_FLOAT_ARGS = X64_LINUX_MAX_FLOAT_ARGS;
        const unsigned char TRAMPOLINE_ARGS_ARRAY_REG = X64_REG_RDI; // Trampoline's first arg (args_array)
        const unsigned char SCRATCH_REG_BASE = X64_REG_R11_MODRM; // Scratch for args_array pointer
        const unsigned char SCRATCH_REG_ARG_PTR = X64_REG_R12_MODRM; // Scratch for individual arg pointer
        const unsigned char TARGET_FUNC_REG = X64_REG_R10_MODRM; // Register to hold target function address (ModR/M value)

        // Save the args_array pointer from the trampoline's first argument register
        // into a scratch register, so the argument registers are free for the target call.
        // mov SCRATCH_REG_BASE, TRAMPOLINE_ARGS_ARRAY_REG
        emit_mov_reg_reg(&code_buffer, &current_size, &capacity, SCRATCH_REG_BASE, TRAMPOLINE_ARGS_ARRAY_REG);
    #endif

    // Generate code to load arguments into appropriate registers
    int current_int_arg_idx = 0;
    int current_float_arg_idx = 0;
  printf("DEBUG: arg_types_affix base address: %p\n", (void*)arg_types_affix);
    for (int i = 0; i < num_args; ++i) {
         printf("DEBUG: arg_types_affix[%d] address: %p\n", i, (void*)arg_types_affix[i]);
        Affix_Type* arg_type = arg_types_affix[i];
        if (arg_type == NULL) {
            fprintf(stderr, "Error: Affix_Type for argument %d is NULL.\n", i);
            free(code_buffer);
            return NULL;
        }
 printf("DEBUG: arg_type pointer: %p, user_type_flag: '%c', dc_type: '%c', size: %zu\n",

               (void*)arg_type, arg_type->type, arg_type->dc_type, arg_type->size);
        // Load pointer to arg_i from args_array into SCRATCH_REG_ARG_PTR
        // mov SCRATCH_REG_ARG_PTR, [SCRATCH_REG_BASE + 8 * i]
        emit_load_gpr_from_mem_offset_sized(&code_buffer, &current_size, &capacity,
                                     SCRATCH_REG_ARG_PTR, SCRATCH_REG_BASE, i * 8, sizeof(void*));

        // Handle arguments based on their dc_type
        switch (arg_type->dc_type) {
            case 'b': // bool, signed char
            case 'c': // char
            case 'h': // unsigned char
            case 'w': // wchar_t (treated as short/int)
            case 's': // short
            case 't': // unsigned short
            case 'i': // int
            case 'j': // unsigned int
            case 'l': // long
            case 'm': // unsigned long
            case 'x': // long long
            case 'y': // unsigned long long
            case 'o': // size_t
            case 'O': // ssize_t
            case 'p': // pointer types (void*, char*, SV*, etc.)
            case 'Z': // C string (char*) - handled as pointer
            case '$': // Perl SV* - handled as pointer
            case '&': // Perl CV* - handled as pointer
            case '@': // Array (pointer to first element)
            case 'E': // Enum (underlying integer type)
                if (current_int_arg_idx >= MAX_INT_ARGS) {
                    fprintf(stderr, "Error: Too many integer/pointer arguments for x86-64 register calling convention (max %d).\n", MAX_INT_ARGS);
                    free(code_buffer);
                    return NULL;
                }
                // Load value from SCRATCH_REG_ARG_PTR into target argument register
                // mov current_int_arg_regs[current_int_arg_idx], [SCRATCH_REG_ARG_PTR]
                emit_load_gpr_from_mem_offset_sized(&code_buffer, &current_size, &capacity,
                                             current_int_arg_regs[current_int_arg_idx], SCRATCH_REG_ARG_PTR, 0, arg_type->size);
                current_int_arg_idx++;
                break;
            case 'd': // double
            case 'f': // float
                if (current_float_arg_idx >= MAX_FLOAT_ARGS) {
                    fprintf(stderr, "Error: Too many floating-point arguments for x86-64 register calling convention (max %d).\n", MAX_FLOAT_ARGS);
                    free(code_buffer);
                    return NULL;
                }
                // Load value from SCRATCH_REG_ARG_PTR into target XMM register
                emit_load_float_reg_from_mem_offset(&code_buffer, &current_size, &capacity,
                                                    current_float_arg_regs[current_float_arg_idx], SCRATCH_REG_ARG_PTR, 0, arg_type->size);
                current_float_arg_idx++;
                break;
            case 'A': // Struct by value (using 'A' for STRUCT_FLAG)
            case 'u': // Union by value (using 'u' for UNION_FLAG)
            case 'T': // Generic aggregate by value (dyncall 'T')
                // ABI-specific struct passing rules are complex.
                // This implementation attempts to pass structs that fit into general-purpose registers.
                // For larger structs or those with specific alignment/composition rules,
                // a full FFI library would typically manage a hidden pointer on the stack.

                size_t struct_bytes_remaining = arg_type->size;
                size_t current_struct_offset = 0;

                while (struct_bytes_remaining > 0) {
                    if (current_int_arg_idx >= MAX_INT_ARGS) {
                        fprintf(stderr, "Warning: Struct/Union argument %d (size %zu) is too large to fit in available GPRs on this architecture. Remaining %zu bytes will NOT be passed correctly.\n", i, arg_type->size, struct_bytes_remaining);
                        // In a real FFI, these remaining bytes would be pushed onto the stack.
                        // For this example, we stop marshalling this struct.
                        struct_bytes_remaining = 0; // Stop processing this struct
                        break;
                    }

                    size_t load_chunk_size = 8; // Default to 8-byte chunks (qword)
                    if (struct_bytes_remaining < 8) {
                        load_chunk_size = struct_bytes_remaining; // Load remaining bytes
                    }

                    // Load a chunk of the struct into the next available GPR
                    emit_load_gpr_from_mem_offset_sized(&code_buffer, &current_size, &capacity,
                                                 current_int_arg_regs[current_int_arg_idx], SCRATCH_REG_ARG_PTR, current_struct_offset, load_chunk_size);

                    current_int_arg_idx++;
                    current_struct_offset += load_chunk_size;
                    struct_bytes_remaining -= load_chunk_size;
                }
                break;
            case 'v': // void (should not be an argument type)
                //fprintf(stderr, "Error: Void type found as argument %d.\n", i);
                //free(code_buffer);
                // return NULL;
                break;
            default:
                // warn("Error: Unsupported argument type '%c' for argument %d on x86-64.\n", arg_type->type, i);
                //free(code_buffer);
                //return NULL;
        }
    }

    // Load the target function's address into TARGET_FUNC_REG
    emit_load_target_addr_instr(&code_buffer, &current_size, &capacity, (uint64_t)target_func_ptr);

    // Call the target function
    emit_call_reg(&code_buffer, &current_size, &capacity, TARGET_FUNC_REG);

    // Return from the trampoline (RAX implicitly holds the return value for the caller)
    // For floating point return values, XMM0 holds the result.
    emit_ret(&code_buffer, &current_size, &capacity);

#elif defined(PLATFORM_ARCH_ARM64)
    #if defined(PLATFORM_OS_WINDOWS) // Use PLATFORM_OS_WINDOWS from platform.h
        printf("Platform: Windows ARM64\n");
        const unsigned char* current_int_arg_regs = arm64_int_arg_regs;
        const int MAX_INT_ARGS = ARM64_MAX_INT_ARGS;
        const unsigned char* current_float_arg_regs = arm64_float_arg_regs;
        const int MAX_FLOAT_ARGS = ARM64_MAX_FLOAT_ARGS;
        const unsigned char TRAMPOLINE_ARGS_ARRAY_REG = ARM64_REG_X0_MODRM; // Trampoline's first arg (args_array)
        const unsigned char SCRATCH_REG_BASE = ARM64_REG_X9_MODRM; // Scratch for args_array pointer
        const unsigned char SCRATCH_REG_ARG_PTR = ARM64_REG_X10_MODRM; // Scratch for individual arg pointer
        const unsigned char TARGET_FUNC_REG = ARM64_REG_X16_MODRM; // Register to hold target function address

    #elif defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_MACOS) // Added PLATFORM_OS_MACOS
        printf("Platform: Linux/macOS ARM64 (System V ABI)\n"); // Updated message
        const unsigned char* current_int_arg_regs = arm64_int_arg_regs;
        const int MAX_INT_ARGS = ARM64_MAX_INT_ARGS;
        const unsigned char* current_float_arg_regs = arm64_float_arg_regs;
        const int MAX_FLOAT_ARGS = ARM64_MAX_FLOAT_ARGS;
        const unsigned char TRAMPOLINE_ARGS_ARRAY_REG = ARM64_REG_X0_MODRM; // Trampoline's first arg (args_array)
        const unsigned char SCRATCH_REG_BASE = ARM64_REG_X9_MODRM; // Scratch for args_array pointer
        const unsigned char SCRATCH_REG_ARG_PTR = ARM64_REG_X10_MODRM; // Scratch for individual arg pointer
        const unsigned char TARGET_FUNC_REG = ARM64_REG_X16_MODRM; // Register to hold target function address
    #endif

    // Save the args_array pointer from the trampoline's first argument register (X0)
    // into a scratch register (X9).
    // mov X9, X0
    emit_mov_reg_reg(&code_buffer, &current_size, &capacity, SCRATCH_REG_BASE, TRAMPOLINE_ARGS_ARRAY_REG);

    // Generate code to load arguments into appropriate registers
    int current_int_arg_idx = 0;
    int current_float_arg_idx = 0;

    for (int i = 0; i < num_args; ++i) {
        Affix_Type* arg_type = arg_types_affix[i];
        if (arg_type == NULL) {
            fprintf(stderr, "Error: Affix_Type for argument %d is NULL.\n", i);
            free(code_buffer);
            return NULL;
        }

        // Load pointer to arg_i from args_array into SCRATCH_REG_ARG_PTR (X10)
        // ldr X10, [X9, #8 * i]
        emit_load_gpr_from_mem_offset_sized(&code_buffer, &current_size, &capacity,
                                       SCRATCH_REG_ARG_PTR, SCRATCH_REG_BASE, i * 8, sizeof(void*));

        // Handle arguments based on their dc_type
        switch (arg_type->dc_type) {
            case 'b': // bool, signed char
            case 'c': // char
            case 'h': // unsigned char
            case 'w': // wchar_t (treated as short/int)
            case 's': // short
            case 't': // unsigned short
            case 'i': // int
            case 'j': // unsigned int
            case 'l': // long
            case 'm': // unsigned long
            case 'x': // long long
            case 'y': // unsigned long long
            case 'o': // size_t
            case 'O': // ssize_t
            case 'p': // pointer types (void*, char*, SV*, etc.)
            case 'Z': // C string (char*) - handled as pointer
            case '$': // Perl SV* - handled as pointer
            case '&': // Perl CV* - handled as pointer
            case '@': // Array (pointer to first element)
            case 'E': // Enum (underlying integer type)
                if (current_int_arg_idx >= MAX_INT_ARGS) {
                    fprintf(stderr, "Error: Too many integer/pointer arguments for ARM64 register calling convention (max %d).\n", MAX_INT_ARGS);
                    free(code_buffer);
                    return NULL;
                }
                // Load value from SCRATCH_REG_ARG_PTR (X10) into target argument register
                // ldr current_int_arg_regs[current_int_arg_idx], [X10, #0]
                emit_load_gpr_from_mem_offset_sized(&code_buffer, &current_size, &capacity,
                                               current_int_arg_regs[current_int_arg_idx], SCRATCH_REG_ARG_PTR, 0, arg_type->size);
                current_int_arg_idx++;
                break;
            case 'd': // double
            case 'f': // float
                if (current_float_arg_idx >= MAX_FLOAT_ARGS) {
                    fprintf(stderr, "Error: Too many floating-point arguments for ARM64 register calling convention (max %d).\n", MAX_FLOAT_ARGS);
                    free(code_buffer);
                    return NULL;
                }
                // Load value from SCRATCH_REG_ARG_PTR into target V register
                emit_load_float_reg_from_mem_offset(&code_buffer, &current_size, &capacity,
                                                    current_float_arg_regs[current_float_arg_idx], SCRATCH_REG_ARG_PTR, 0, arg_type->size);
                current_float_arg_idx++;
                break;
            case 'A': // Struct by value (using 'A' for STRUCT_FLAG)
            case 'u': // Union by value (using 'u' for UNION_FLAG)
            case 'T': // Generic aggregate by value (dyncall 'T')
                // ABI-specific struct passing rules are complex.
                // This implementation attempts to pass structs that fit into general-purpose registers.
                // For larger structs or those with specific alignment/composition rules,
                // a full FFI library would typically manage a hidden pointer on the stack.

                size_t struct_bytes_remaining = arg_type->size;
                size_t current_struct_offset = 0;

                while (struct_bytes_remaining > 0) {
                    if (current_int_arg_idx >= MAX_INT_ARGS) {
                        fprintf(stderr, "Warning: Struct/Union argument %d (size %zu) is too large to fit in available GPRs on this architecture. Remaining %zu bytes will NOT be passed correctly.\n", i, arg_type->size, struct_bytes_remaining);
                        // In a real FFI, these remaining bytes would be pushed onto the stack.
                        // For this example, we stop marshalling this struct.
                        struct_bytes_remaining = 0; // Stop processing this struct
                        break;
                    }

                    size_t load_chunk_size = 8; // Default to 8-byte chunks (qword)
                    if (struct_bytes_remaining < 8) {
                        load_chunk_size = struct_bytes_remaining; // Load remaining bytes
                    }

                    // Load a chunk of the struct into the next available GPR
                    emit_load_gpr_from_mem_offset_sized(&code_buffer, &current_size, &capacity,
                                                 current_int_arg_regs[current_int_arg_idx], SCRATCH_REG_ARG_PTR, current_struct_offset, load_chunk_size);

                    current_int_arg_idx++;
                    current_struct_offset += load_chunk_size;
                    struct_bytes_remaining -= load_chunk_size;
                }
                break;
            case 'v': // void (should not be an argument type)
                fprintf(stderr, "Error: Void type found as argument %d.\n", i);
                free(code_buffer);
                return NULL;
            default:
                fprintf(stderr, "Error: Unsupported argument type '%c' for argument %d on ARM64.\n", arg_type->dc_type, i);
                free(code_buffer);
                return NULL;
        }
    }

    // Load the target function's address into TARGET_FUNC_REG (X16)
    // This involves an LDR literal instruction followed by the 64-bit address.
    // LDR X16, [PC, #offset] -> offset is 8 bytes (BLR X16 + literal itself)
    emit_load_target_addr_instr(&code_buffer, &current_size, &capacity, (uint64_t)target_func_ptr);
    _append_uint64(&code_buffer, &current_size, &capacity, (uint64_t)target_func_ptr); // The literal value

    // Call the target function
    // blr X16
    emit_call_reg(&code_buffer, &current_size, &capacity, TARGET_FUNC_REG);

    // Return from the trampoline (X0 implicitly holds the return value for the caller)
    // For floating point return values, V0 holds the result.
    emit_ret(&code_buffer, &current_size, &capacity);

#else
    fprintf(stderr, "Error: Unsupported architecture or operating system for trampoline generation.\n");
    free(code_buffer);
    return NULL;
#endif

    // --- Allocate executable memory ---
    void* exec_mem = NULL;
    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;

#if defined(PLATFORM_OS_MACOS)
    // On macOS, MAP_JIT is required for executable memory for JIT-compiled code.
    // This is especially critical on Apple Silicon (ARM64).
    mmap_flags |= MAP_JIT;
#endif

#ifdef _WIN32
    exec_mem = VirtualAlloc(NULL, current_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (exec_mem == NULL) {
        fprintf(stderr, "VirtualAlloc failed: %lu\n", GetLastError());
        free(code_buffer);
        return NULL;
    }
#else
    // Get the page size for mprotect operations
    long page_size = sysconf(_SC_PAGESIZE);
    // Calculate page-aligned start and size for mmap
    size_t aligned_size = ((current_size + page_size - 1) / page_size) * page_size;

    exec_mem = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                          mmap_flags, -1, 0); // Use mmap_flags here
    if (exec_mem == MAP_FAILED) {
        perror("mmap failed");
        free(code_buffer);
        return NULL;
    }
#endif

    // --- Apple Silicon Write Protection Handling ---
#if defined(PLATFORM_OS_MACOS) && defined(PLATFORM_ARCH_ARM64)
    // On Apple Silicon, memory pages marked PROT_EXEC cannot be written to directly.
    // We need to temporarily change permissions to allow writing, then revert.
    long page_size_as = sysconf(_SC_PAGESIZE);
    void* page_aligned_start_as = (void*)((uintptr_t)exec_mem & ~(page_size_as - 1));
    size_t protected_size_as = ((uintptr_t)exec_mem + current_size + page_size_as - 1) - (uintptr_t)page_aligned_start_as;
    protected_size_as = (protected_size_as / page_size_as) * page_size_as;

    // Temporarily enable write permission for code modification
    if (mprotect(page_aligned_start_as, protected_size_as, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
        perror("mprotect failed to enable write permissions on Apple Silicon");
        free(code_buffer);
        munmap(exec_mem, aligned_size); // Use aligned_size for munmap
        return NULL;
    }
#endif

    memcpy(exec_mem, code_buffer, current_size);
    free(code_buffer); // Free the temporary buffer

    printf("Generated trampoline at address: %p\n", exec_mem);
    printf("Generated machine code bytes (%zu bytes): ", current_size);
    for (size_t i = 0; i < current_size; ++i) {
        printf("%02x ", ((unsigned char*)exec_mem)[i]);
    }
    printf("\n");

    // --- Re-enable read-execute permission, revoke write on Apple Silicon ---
#if defined(PLATFORM_OS_MACOS) && defined(PLATFORM_ARCH_ARM64)
    if (mprotect(page_aligned_start_as, protected_size_as, PROT_READ | PROT_EXEC) == -1) {
        perror("mprotect failed to set read-execute permissions after writing on Apple Silicon");
        // This is a critical error, but for an example, we might proceed or exit.
    } else {
        printf("Memory protection reverted to PROT_READ | PROT_EXEC on Apple Silicon.\n");
    }
#endif

    // --- Memory Locking (for other Unix-like systems, if not already handled by MAP_JIT/mprotect above) ---
    // This block is primarily for Linux/other Unix where MAP_JIT isn't a thing
    // but we still want to revoke write permissions after writing.
#if !defined(_WIN32) && !(defined(PLATFORM_OS_MACOS) && defined(PLATFORM_ARCH_ARM64))
    // Get the page size
    long page_size_other = sysconf(_SC_PAGESIZE);
    // Calculate the page-aligned start address and size to protect
    void* page_aligned_start_other = (void*)((uintptr_t)exec_mem & ~(page_size_other - 1));
    size_t protected_size_other = ((uintptr_t)exec_mem + current_size + page_size_other - 1) - (uintptr_t)page_aligned_start_other;
    protected_size_other = (protected_size_other / page_size_other) * page_size_other; // Ensure it's a multiple of page_size

    if (mprotect(page_aligned_start_other, protected_size_other, PROT_READ | PROT_EXEC) == -1) {
        perror("mprotect failed to set read-execute permissions");
        // This is a non-fatal error for this example, but should be handled in production.
    } else {
        printf("Memory locked to PROT_READ | PROT_EXEC.\n");
    }
#endif

    return (GeneratedTrampolineFunc)exec_mem;
}

// --- Memory Deallocation Helper ---
// This function correctly deallocates memory based on the OS.
void free_trampoline_memory(void* ptr, size_t size) {
    if (ptr == NULL) return;
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE); // Size must be 0 for MEM_RELEASE
#else
    // For munmap, the address must be page-aligned and size must be multiple of page size.
    // Our generated code is small, so we'll just unmap the page it's on.
    long page_size = sysconf(_SC_PAGESIZE);
    void* page_aligned_addr = (void*)((uintptr_t)ptr & ~(page_size - 1));
    // The size passed to munmap must be the same as the size passed to mmap.
    // Since we aligned the size for mmap, we need to use that aligned size here.
    // For now, we'll use a page size as a safe bet for small allocations.
    munmap(page_aligned_addr, page_size); // Unmap the entire page
#endif
}


void greet_perl_string(){

    dTHX;
    warn("HERE!");
}
// Target 1: void greet_perl_string(SV* name_sv)
// Takes a Perl SV, extracts its string value, and prints it.
void greet_perl_string_(SV* name_sv) {
    dTHX; // Obtain Perl thread context
    STRLEN len;
    char* name_c_str = SvPV(name_sv, len);

    warn("  [Target Function] greet_perl_string() called.\n");
    warn("  Hello from Perl string: '%s' (length: %zu)\n", name_c_str, len);
}
extern void _exec_jit(pTHX_ CV * cv) {
    dXSARGS;
    Jitter * jitter = (Jitter *)XSANY.any_ptr;





//  new_Affix_Type(aTHX_ * av_fetch(args, instruction, 0));
{ 
    Affix_Type* arg_types1[] = {
        jitter->push[0]
    };
    int num_args1 = sizeof(arg_types1) / sizeof(arg_types1[0]);

    SV* hello_sv = newSVpv("World", 0);
    void* args1[] = { &hello_sv }; // Array of pointers to arguments
    void ** aaaa = NULL;
    GeneratedTrampolineFunc call_greet_perl_string = generate_trampoline((void*)greet_perl_string, 
    jitter->pop,    
    arg_types1, num_args1);
    if (call_greet_perl_string) {
        call_greet_perl_string(aaaa);

        printf("Trampoline call to greet_perl_string completed.\n\n");
        SvREFCNT_dec(hello_sv);
        free_trampoline_memory(call_greet_perl_string, 0);
    }
    // free(ret_type1);
    for(int i=0; i<num_args1; ++i) free(arg_types1[i]);
}









    
    PL_stack_sp = PL_stack_base + ax +
        (((ST(0) = newSViv(100)) != NULL) ? 0 : -1);    
}

void free_jitter(Jitter * jitter) {
    // Attempt to unlock memory before freeing
#if defined(PLATFORM_OS_WINDOWS)
    if (!VirtualUnlock(jitter->jit, jitter->jit_len)) 
        fprintf(stderr, "Warning: VirtualUnlock failed. Error: %lu. Memory might remain locked.\n", GetLastError());
    VirtualFree(jitter->jit, 0, MEM_RELEASE); 
#else
    if (munlock(jitter->jit, jitter->jit_len) != 0) 
        croak("Warning: munlock failed. Memory might remain locked."); 
    munmap(jitter->jit, jitter->jit_len);       
#endif
    safefree(jitter);
}


SV* execute_jit_code(Jitter*jitter, SV* code_sv, SV* return_type_sv, SV* args_array_ref_sv, SV* arg_types_for_c_array_ref_sv) {
    STRLEN code_len;
    char* code_bytes = SvPV(code_sv, code_len);
    if (code_len == 0) {
        fprintf(stderr, "C: Received empty machine code string.\n");
        return newSViv(-1); // Return an error value
    }


    IV return_type = SvIV(return_type_sv);
        SV* perl_return_sv = NULL; // The SV we will return to Perl











//  new_Affix_Type(aTHX_ * av_fetch(args, instruction, 0));
{ 
    Affix_Type* arg_types1[] = {
        jitter->push[0]
    };
    int num_args1 = sizeof(arg_types1) / sizeof(arg_types1[0]);

    SV* hello_sv = newSVpv("World", 0);
    void* args1[] = { &hello_sv }; // Array of pointers to arguments
    GeneratedTrampolineFunc call_greet_perl_string = generate_trampoline((void*)greet_perl_string, 
    jitter->pop,    
    arg_types1, num_args1);
    if (call_greet_perl_string) {
        call_greet_perl_string(args1);
        printf("Trampoline call to greet_perl_string completed.\n\n");
        SvREFCNT_dec(hello_sv);
        free_trampoline_memory(call_greet_perl_string, 0);
    }
    // free(ret_type1);
    for(int i=0; i<num_args1; ++i) free(arg_types1[i]);
}







    return perl_return_sv; // Return the Perl SV
}

XS_INTERNAL(Affix_jitter) {
    // ix == 0 if Affix::affix
    // ix == 1 if Affix::wrap
    dXSARGS;
    dXSI32;
    PERL_UNUSED_VAR(items);
    Jitter * affix = NULL;
    char *prototype = NULL, *rename = NULL;
    STRLEN len;
    DLLib * libhandle = NULL;
    DCpointer funcptr = NULL;
    SV * const xsub_tmp_sv = ST(0);
    SvGETMAGIC(xsub_tmp_sv);
    if (!SvOK(xsub_tmp_sv) && SvREADONLY(xsub_tmp_sv))  // explicit undef
        libhandle = load_library(NULL);
    else if (sv_isobject(xsub_tmp_sv) && sv_derived_from(xsub_tmp_sv, "Affix::Lib")) {
        IV tmp = SvIV((SV *)SvRV(xsub_tmp_sv));
        libhandle = INT2PTR(DLLib *, tmp);
    }
    else if (NULL == (libhandle = load_library(SvPV_nolen(xsub_tmp_sv)))) {
        Stat_t statbuf;
        Zero(&statbuf, 1, Stat_t);
        if (PerlLIO_stat(SvPV_nolen(xsub_tmp_sv), &statbuf) < 0) {
            ENTER;
            SAVETMPS;
            PUSHMARK(SP);
            XPUSHs(xsub_tmp_sv);
            PUTBACK;
            SSize_t count = call_pv("Affix::find_library", G_SCALAR);
            SPAGAIN;
            if (count == 1)
                libhandle = load_library(SvPV_nolen(POPs));
            PUTBACK;
            FREETMPS;
            LEAVE;
        }
    }

    if (libhandle == NULL)
        croak("Failed to load %s", SvPVbyte_or_null(ST(0), len));
    if ((SvROK(ST(1)) && SvTYPE(SvRV(ST(1))) == SVt_PVAV) && av_count(MUTABLE_AV(SvRV(ST(1)))) == 2) {
        rename = SvPVbyte_or_null(*av_fetch(MUTABLE_AV(SvRV(ST(1))), 1, 0), len);
        funcptr = find_symbol(libhandle, SvPVbyte_or_null(*av_fetch(MUTABLE_AV(SvRV(ST(1))), 0, 0), len));
    }
    else {
        rename = SvPVbyte_or_null(ST(1), len);
        funcptr = find_symbol(libhandle, SvPVbyte_or_null(ST(1), len));
    }
    if (funcptr == NULL)
        croak("Failed to locate %s in %s", SvPVbyte_or_null(ST(1), len), SvPVbyte_or_null(ST(0), len));
    if (!(SvROK(ST(2)) && SvTYPE(SvRV(ST(2))) == SVt_PVAV))
        croak("Expected args as a list of types");
    Newxz(affix, 1, Affix);
    affix->pop = new_Affix_Type(aTHX_ ST(3));
    if (affix == NULL)
        XSRETURN_EMPTY;
    //~ PING;
    //~ DD(ST(2));
    //~ affix->push[0] = (aTHX_ new Affix_Reset());
    AV * args = MUTABLE_AV(SvRV(ST(2)));

    size_t count = av_count(args);
    {
        //~ warn("affix->instructions: %d", affix->instructions);

        bool aggr_ret =
            (affix->pop->type == STRUCT_FLAG || affix->pop->type == CPPSTRUCT_FLAG || affix->pop->type == UNION_FLAG)
            ? true
            : false;

        Newxz(affix->push, count + 1 + (aggr_ret ? 1 : 0), Affix_Type *);
        //~ Newxz(affix->push, count + 1 + (affix->pop->type == DC_SIGCHAR_AGGREGATE ? 1 : 0), Affix_Type *);
        affix->push[affix->instructions++] = _reset();  // Off on the right foot
        if (aggr_ret)
            affix->push[affix->instructions++] = _aggr();
        Newxz(prototype, count + 1, char);
        for (size_t instruction = 0; instruction < count; instruction++) {
            affix->push[affix->instructions] = new_Affix_Type(aTHX_ * av_fetch(args, instruction, 0));

            //~ DD(*av_fetch(args, instruction - 1, 0));
            //~ warn(">>> st: 0, type: %c, instruction: %u, affix->instructions: %u",
            //~ affix->push[affix->instructions]->type,
            //~ instruction,
            //~ count);

            if (affix->push[affix->instructions] == NULL) {
                sv_dump(*av_fetch(args, instruction - 1, 0));
                croak("Oh, man...");
            }

            //~ warn("instruction %d is %d [%c]", i, affix->push[i]->type, affix->push[i]->type);
            //~ warn("affix->push[%d]->type: %c [%d]", i, affix->push[i]->type, affix->push[i]->type);
            //~ switch() {
            Affix_Type * hold = affix->push[affix->instructions];
            //~ }
            switch (hold->type) {

            case RESET_FLAG:
            case MODE_FLAG:
                /*
                #define  ''  //
                #define THIS_FLAG '*'
                #define ELLIPSIS_FLAG 'e'
                #define VARARGS_FLAG '.'
                #define CDECL_FLAG 'D'
                #define STDCALL_FLAG 'T'
                #define MSFASTCALL_FLAG '='
                #define GNUFASTCALL_FLAG '3'  //
                #define MSTHIS_FLAG '+'
                #define GNUTHIS_FLAG '#'  //
                #define ARM_FLAG 'r'
                #define THUMB_FLAG 'g'  // ARM
                #define SYSCALL_FLAG 'H'*/
                break;
            default:
                prototype[affix->args++] = '$';  // TODO: proper prototype

                break;
            }
            affix->instructions++;
            //~ if (affix->push[i]->type != RESET_FLAG && affix->push[i]->type != MODE_FLAG)
        }
        //~ warn("prototype: '%s'", prototype);
    }
    affix->symbol = funcptr;

    STMT_START {
        cv = newXSproto_portable(ix == 0 ? rename : NULL, _exec_jit, __FILE__, prototype);
        if (UNLIKELY(cv == NULL))
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (DCpointer)affix;
    }
    STMT_END;
    ST(0) = sv_2mortal(sv_bless((UNLIKELY(ix == 1) ? newRV_noinc(MUTABLE_SV(cv)) : newRV_inc(MUTABLE_SV(cv))),
                                gv_stashpv("Affix", GV_ADD)));
    safefree(prototype);
    XSRETURN(1);
}