#include "../Affix.h"

char *locate_lib(pTHX_ SV *_lib, SV *_ver) {
    // Use perl to get the actual path to the library
    dSP;
    int count;
    char *retval = NULL;
    //~ if (!SvOK(_lib)) {
    //~ GV *tmpgv = gv_fetchpvs("\030", GV_ADD | GV_NOTQUAL, SVt_PV); /* $^X */
    //~ _lib = GvSV(tmpgv);
    //~ }
    if (SvOK(_lib) /*&& SvREADONLY(_lib)*/) {
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        XPUSHs(_lib);
        XPUSHs(_ver);
        PUTBACK;
        count = call_pv("Affix::locate_lib", G_SCALAR);
        SPAGAIN;
        if (count == 1) {
            SV *ret = POPs;
            if (SvOK(ret)) {
                STRLEN len;
                //~ sv_dump(ret);
                char *__lib = SvPVx(ret, len);
                if (len) {
                    Newxz(retval, len + 1, char);
                    Copy(__lib, retval, len, char);
                }
            }
        }
        PUTBACK;
        FREETMPS;
        LEAVE;
    }
    return retval;
}


void boot_Affix_Lib(pTHX_ CV *cv) {
    PERL_UNUSED_VAR(cv);

    (void)newXSproto_portable("Affix::load_lib", Affix_load_lib, __FILE__, "$;$");
    export_function("Affix", "load_lib", "lib");
    (void)newXSproto_portable("Affix::Lib::list_symbols", Affix_Lib_list_symbols, __FILE__, "$");
    (void)newXSproto_portable("Affix::Lib::path", Affix_Lib_path, __FILE__, "$");
    (void)newXSproto_portable("Affix::Lib::free", Affix_Lib_free, __FILE__, "$;$");

    set_isa("Affix::Lib", "Affix::Pointer");
}
