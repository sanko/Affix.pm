#include "lib/clutter.h"

MODULE = Dyn::Load PACKAGE = Dyn::Load

DLLib *
dlLoadLibrary(const char * libpath = NULL)
CODE:
    RETVAL =
#if defined(_WIN32) || defined(_WIN64)
        dlLoadLibrary( libpath );
#else
        (DLLib*)dlopen( libpath, RTLD_LAZY/* RTLD_NOW|RTLD_GLOBAL */);
#endif
OUTPUT:
    RETVAL

void
dlFreeLibrary(DLLib * pLib)
CODE:
    dlFreeLibrary(pLib);
    SV* sv = (SV*) &PL_sv_undef;
    sv_setsv(ST(0), sv);

DCpointer
dlFindSymbol(DLLib * pLib, const char * pSymbolName);

int
dlGetLibraryPath(DLLib * pLib, char * sOut, int bufSize);
OUTPUT:
    RETVAL
    sOut

DLSyms *
dlSymsInit(const char * libPath);

void
dlSymsCleanup(DLSyms * pSyms);
CODE:
    dlSymsCleanup(pSyms);
    SV* sv = (SV*) &PL_sv_undef;
    sv_setsv(ST(0), sv);

int
dlSymsCount(DLSyms * pSyms);

const char*
dlSymsName(DLSyms * pSyms, int index);

const char*
dlSymsNameFromValue(DLSyms * pSyms, void * value)

BOOT:
    export_function("Dyn::Load", "dlLoadLibrary", "default");
    export_function("Dyn::Load", "dlFreeLibrary", "default");
    export_function("Dyn::Load", "dlFindSymbol", "default");
    export_function("Dyn::Load", "dlGetLibraryPath", "default");
    export_function("Dyn::Load", "dlSymsInit", "default");
    export_function("Dyn::Load", "dlSymsCleanup", "default");
    export_function("Dyn::Load", "dlSymsCount", "default");
    export_function("Dyn::Load", "dlSymsName", "default");
    export_function("Dyn::Load", "dlSymsNameFromValue", "default");
