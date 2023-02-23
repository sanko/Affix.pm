#include "std.h"

#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

int demo(const wchar_t *lhs, const wchar_t *rhs) {
    int rc = wcscmp(lhs, rhs);
    const char *rel = rc < 0 ? "precedes" : rc > 0 ? "follows" : "equals";
    setlocale(LC_ALL, "en_US.utf-8");
    printf("[%ls] %s [%ls]\n", lhs, rel, rhs);
    return rc;
}

DLLEXPORT int check_string(wchar_t *stringx) {
    setlocale(LC_ALL, "en_US.utf-8");
    return demo(L"時空", stringx);
}

DLLEXPORT const wchar_t *get_string() {
    setlocale(LC_ALL, "en_US.utf-8");
    return L"時空";
}
