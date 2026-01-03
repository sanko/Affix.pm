
#include "std.h"
// ext: .c
#undef warn
#include <EXTERN.h>
#include <perl.h>
static PerlInterpreter * my_perl;

#define NO_XSLOCKS
#include <XSUB.h>

// Takes an SV*, increments it if it's an integer
DLLEXPORT void inc_sv(SV * sv) {
    if (SvIOK(sv)) {
        int val = SvIV(sv);
        sv_setiv(sv, val + 1);
    }
}

// Returns a new SV* (Mortal)
DLLEXPORT SV * make_sv(int val) { return sv_2mortal(newSViv(val)); }
