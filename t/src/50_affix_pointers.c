#include "std.h"
#include <stdlib.h>

typedef struct {
    bool B;
    char c;
    unsigned char C;
    short s;
    unsigned short S;
    int i;
    unsigned int I;
    long j;
    unsigned long J;
    long long l;
    unsigned long long L;
    float f;
    double d;
    int *p;
    char *Z;
    struct {
        int i;
    } A;
    union
    {
        int i;
        struct {
            void *ptr;
            long l;
        } structure;
    } u;
} massive;

int _p = 303;
massive retval = {.B = true,
                  .c = 'c',
                  .C = 'C',
                  .s = 3,
                  .S = 5,
                  .i = -987,
                  .I = 678,
                  .j = 13579,
                  .J = 24680,
                  .l = -1234567890,
                  .L = 9876543210,
                  .f = 9.9843,
                  .d = 1.246,
                  .p = &_p,
                  .Z = "Just a little test",
                  .u = {.i = 5},
                  .A = {.i = 50}};

#if __GNUC__ < 3
#define PZ "Z"
#else
#define PZ "z"
#endif

DLLEXPORT massive *massive_ptr() {
    warn("    # sizeof in C:    %" PZ "u", sizeof(massive));
    warn("    # offset.B:       %" PZ "u", offsetof(massive, B));
    warn("    # offset.c:       %" PZ "u", offsetof(massive, c));
    warn("    # offset.C:       %" PZ "u", offsetof(massive, C));
    warn("    # offset.s:       %" PZ "u", offsetof(massive, s));
    warn("    # offset.S:       %" PZ "u", offsetof(massive, S));
    warn("    # offset.i:       %" PZ "u", offsetof(massive, i));
    warn("    # offset.I:       %" PZ "u", offsetof(massive, I));
    warn("    # offset.j:       %" PZ "u", offsetof(massive, j));
    warn("    # offset.J:       %" PZ "u", offsetof(massive, J));
    warn("    # offset.Z:       %" PZ "u", offsetof(massive, Z));

    /*retval.c = -100;
    retval.C = 100;
    retval.s = -30;
    retval.S = 40;
    retval.Z = "Hi!";*/
    //(massive*) malloc(sizeof(massive));
    warn("Z.i: %d", retval.A.i);
    warn("Z: %s", retval.Z);
    return &retval;
}
DLLEXPORT bool sptr(massive *sptr) {
    if (!strcmp(sptr->Z, "Works!")) return true;
    return false;
}

DLLEXPORT char *dbl_ptr(double *dbl) {
    warn("# dbl_ptr( %p )", (void *)dbl);
    if (dbl == NULL)
        return "NULL";
    else if (*dbl == 0) {
        *dbl = 1000;
        return "empty";
    }
    else if (*dbl == 100) {
        *dbl = 1000;
        return "one hundred";
    }
    else if (*dbl == 100.04) {
        *dbl = 10000;
        return "one hundred and change";
    }
    else if (*dbl == 9) {
        *dbl = 9876.543;
        return "nine";
    }
    return "fallback";
}

/* Use typedef to declare the name "my_function_t"
   as an alias whose type is a function that takes
   one integer argument and returns an integer */
typedef double my_function_t(int, int);

DLLEXPORT double pointer_test(double *dbl, int arr[5], int size,
                              my_function_t *my_function_pointer) {

    if (dbl == NULL) return -1;
    if (*dbl == 90) return 501;
    // for (int i = 0; i < size; ++i)
    //     warn("# arr[%d] == %d", i, arr[i]);
    if (*dbl >= 590343.12351) {
        warn("# In: %f", *dbl);
        *dbl = 3.493;
        return *dbl * 5.25;
    }
    /* Invoke the function via the global function
       pointer variable. */
    double ret = 0;
    if (my_function_pointer != NULL) ret = my_function_pointer(4, 8);
    *dbl = ret * 2;
    return 900;
}

typedef struct {
    int i;
    char *Z;

} test;
DLLEXPORT bool demo(test in) {
    warn("# offsetof: %" PZ "u", offsetof(test, Z));
    warn("# i: %d", in.i);
    warn("# Z: %s", in.Z);
    return !strcmp(in.Z, "Here. There. Everywhere.");
}

DLLEXPORT void *set_deep_pointer(int number, size_t depth) {
    void *ptr = NULL;
    void **temp = &ptr;
    for (size_t i = 0; i < depth; ++i) {
        warn("============== i: %ld, depth: %ld", i, depth);
        *temp = malloc(sizeof(void *));
        temp = (void **)(*temp);
        DumpHex(temp, 16);
    }
    *temp = malloc(sizeof(int));
    *((int *)*temp) = number;
    return ptr;
}

DLLEXPORT int get_deep_pointer(void *pointer, size_t depth) {
    DumpHex(pointer, 16);
    if (depth == 0) { return *((int *)pointer); }
    return get_deep_pointer(*((void **)pointer), depth - 1);
}

#include <stdlib.h>

DLLEXPORT void *get_deep(void) {
    //~ warn("zero");
    void *ret = malloc(sizeof(int));
    //~ warn("one");
    *((int *)ret) = 3;
    //~ warn("two");
    //~ warn("***** ret == %p", ret);
    //~ DumpHex(ret, sizeof(int));
    return ret;
}
