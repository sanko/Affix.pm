#include "std.h"

/* Use typedef to declare the name "my_function_t"
   as an alias whose type is a function that takes
   one integer argument and returns an integer */
typedef double my_function_t(int, int);

DLLEXPORT double pointer_test(double *dbl, int arr[5], int size,
                              my_function_t *my_function_pointer) {
    if (dbl == NULL) return -1;
    // for (int i = 0; i < size; ++i)
    //     warn("# arr[%d] == %d", i, arr[i]);

    /* Invoke the function via the global function
       pointer variable. */
    double ret = my_function_pointer(4, 8);
    *dbl = ret * 2;

    return 900;
}
