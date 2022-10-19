#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h> // offsetof

#define warn(FORMAT, ...)                                                                          \
    fprintf(stderr, FORMAT " in %s at line %i\n", ##__VA_ARGS__, __FILE__, __LINE__)

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
typedef signed __int64 int64_t;
#else
#define DLLEXPORT extern
#include <inttypes.h>
#include <sys/types.h>
#endif
