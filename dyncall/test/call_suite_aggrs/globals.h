/*

 Package: dyncall
 Library: test
 File: test/call_suite_aggrs/globals.h
 Description:
 License:

   Copyright (c) 2022 Tassilo Philipp <tphilipp@potion-studios.com>

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

/* the 'a'ggregate type points to memory with random data that is big enough to hold all different
 * struct types */
#define DEF_TYPES                                                                                  \
    X(B, DCbool)                                                                                   \
    X(c, char) X(s, short) X(i, int) X(j, long) X(l, long long) X(C, unsigned char)                \
        X(S, unsigned short) X(I, unsigned int) X(J, unsigned long) X(L, unsigned long long)       \
            X(p, void *) X(f, float) X(d, double) X(a, void *)

#define X(CH, T)                                                                                   \
    extern T *K_##CH;                                                                              \
    extern T *V_##CH;
DEF_TYPES
#undef X

typedef void (*funptr)();

extern funptr G_funtab[];
extern const char *G_sigtab[];
extern int G_ncases;
extern int G_maxargs;
extern const char *G_agg_sigs[];
extern int G_agg_sizes[];
extern funptr G_agg_touchAfuncs[];
extern funptr G_agg_cmpfuncs[];
extern int G_naggs;

void init_test_data();
void deinit_test_data();
void clear_V();

int get_max_aggr_size();
