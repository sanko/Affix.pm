/*

 Package: dyncall
 Library: test
 File: test/callback_suite/globals.h
 Description:
 License:

   Copyright (c) 2011-2022 Daniel Adler <dadler@uni-goettingen.de>,
                           Tassilo Philipp <tphilipp@potion-studios.com>

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include "dyncall_types.h"

#define DEF_TYPES                                                                                  \
    X(B, DCbool)                                                                                   \
    X(c, DCchar)                                                                                   \
    X(C, DCuchar) X(s, DCshort) X(S, DCushort) X(i, DCint) X(I, DCuint) X(j, DClong) X(J, DCulong) \
        X(l, DClonglong) X(L, DCulonglong) X(f, DCfloat) X(d, DCdouble) X(p, DCpointer)            \
            X(Z, DCpointer)

#define X(CH, T)                                                                                   \
    extern T *K_##CH;                                                                              \
    extern T *V_##CH;
DEF_TYPES
#undef X

typedef void (*funptr)(void *);

extern funptr G_funtab[];
extern const char *G_sigtab[];
extern int G_ncases;
extern int G_maxargs;

void init_test_data();
void deinit_test_data();
void clear_V();
