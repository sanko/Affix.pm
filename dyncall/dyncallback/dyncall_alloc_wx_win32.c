/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_alloc_wx_win32.c
 Description: Allocate write/executable memory  - Implementation for win32 platform
 License:

   Copyright (c) 2007-2018 Daniel Adler <dadler@uni-goettingen.de>,
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

#include "dyncall_alloc_wx.h"
#include <windows.h>
#include <assert.h>

DCerror dcAllocWX(size_t size, void **ptr) {
    LPVOID p = VirtualAlloc(0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (p == NULL) return -1;
    *ptr = p;
    return 0;
}

DCerror dcInitExecWX(void *p, size_t size) {
    return 0;
}

void dcFreeWX(void *p, size_t size) {
    BOOL b = VirtualFree(p, 0, MEM_RELEASE);
    assert(b);
}
