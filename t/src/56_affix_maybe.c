#include "std.h"

DLLEXPORT bool CheckNullPtr(int i) {
    warn("# i == %d", i);
    if (i == 10) return true;
    return false;
}
