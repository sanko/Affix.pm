#include "std.h"

DLLEXPORT char *dbl_ptr(double *dbl) {
    // warn("# dbl == %f", *dbl);
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
        warn("In: %f", *dbl);
        *dbl = 3.493;
        return *dbl * 5.25;
    }
    /* Invoke the function via the global function
       pointer variable. */
    double ret = my_function_pointer(4, 8);
    *dbl = ret * 2;

    return 900;
}
