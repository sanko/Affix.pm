#include "std.h"

DLLEXPORT
bool test(bool value) {
    // return the opposite; makes sure we're updating ST(0)
    if (value) return false;
    return true;
}

DLLEXPORT
bool test(int pos, bool *ptr) {
    if (ptr[pos]) return true;
    return false;
}

DLLEXPORT
bool test(int x, int y, bool **ptr) {
    /*for (int i = 0; i < 3; i++) {
        printf("# ");
        for (int j = 0; j < 3; j++) {
            if (ptr[i][j]) { printf("X "); }
            else { printf("O "); }
        }
        printf("\n");
    }*/
    if (ptr[x][y]) return true;
    return false;
}

DLLEXPORT
bool test(int x, int y, int z, bool ***ptr) {
    /*
    bool ***board = (bool ***) malloc(sizeof(bool **) * 3);
    for (int i = 0; i < 3; i++) {
      board[i] = (bool **) malloc(sizeof(bool *) * 3);
      for (int j = 0; j < 3; j++) {
        board[i][j] = (bool *) malloc(sizeof(bool) * 3);
        for (int k = 0; k < 3; k++) {
          board[i][j][k] =  (i + j * k) % 2;
        }
      }
    }
    */
#ifdef DEBUG
    printf("#[\n");
    for (int i = 0; i < 3; i++) {
        printf("#  [\n");
        for (int j = 0; j < 3; j++) {
            printf("#    [");
            for (int k = 0; k < 3; k++) {
                printf("%c%d,", (i == x && j == y && k == z) ? '*' : ' ', ptr[i][j][k]);
            }
            printf(" ],\n");
        }
        printf("#  ],\n");
    }
    printf("#]\n");
    fflush(stdout);
#endif
    if (ptr[x][y][z]) return true;
    return false;
}

DLLEXPORT
bool *Ret_BoolPtr() {
    bool *arr = (bool *)malloc(sizeof(bool) * 4);
    int i;
    for (i = 0; i < 4; i++) {
        arr[i] = (i % 2 == 0);
    }

    return arr;
}
DLLEXPORT
bool **Ret_BoolPtrPtr() {
    bool **board = (bool **)malloc(sizeof(bool *) * 3);
    for (int i = 0; i < 3; i++) {
        board[i] = (bool *)malloc(sizeof(bool) * 3);
        for (int j = 0; j < 3; j++) {
            board[i][j] = i == j;
        }
    }
    return board;
}

DLLEXPORT
char test(char check) {
    if (check == 'M') return 'Y';
    return 'N';
}

DLLEXPORT
char test(int pos, char *word) {
    return word[pos];
}
DLLEXPORT
char *test(int x, char **names) {
    return names[x];
}
DLLEXPORT
char *test(int x, int y, char ***names) {
    return names[x][y];
}
DLLEXPORT
char test(int x, int y, int pos, char ***names) {
    return names[x][y][pos];
}

DLLEXPORT
char ***Ret_CharPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;
    char c = 'a';

    char ***array = (char ***)malloc(sizeof(char **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (char **)malloc(sizeof(char *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (char *)malloc(sizeof(char) * depth + 1);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = c++; //'a' + i + j + k; // i + j + k;
            }
            array[i][j][depth] = '\0';
        }
    }
#ifdef DEBUG
    printf("#[\n");
    for (int i = 0; i < rows; i++) {
        printf("#  [");
        for (int j = 0; j < cols; j++) {
            // printf("#    [");
            // for (int k = 0; k < 3; k++) {
            printf(" '%s',", array[i][j]);
            //}
            // printf(" ],\n");
        }
        printf("  ],\n");
    }
    printf("#]\n");
    fflush(stdout);
#endif
    return array;
}

DLLEXPORT
unsigned char test(unsigned char check) {
    if (check == 'M') return 'Y';
    return 'N';
}

DLLEXPORT
unsigned char test(int pos, unsigned char *ptr) {
    if (ptr[pos] == 'A') return 'a';
    return ptr[pos];
}

DLLEXPORT
unsigned char ***Ret_UCharPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;
    unsigned char c = 'a';

    unsigned char ***array = (unsigned char ***)malloc(sizeof(unsigned char **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (unsigned char **)malloc(sizeof(unsigned char *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (unsigned char *)malloc(sizeof(unsigned char) * depth + 1);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = c++; //'a' + i + j + k; // i + j + k;
            }
            array[i][j][depth] = '\0';
        }
    }
#ifdef DEBUG
    printf("#[\n");
    for (int i = 0; i < rows; i++) {
        printf("#  [");
        for (int j = 0; j < cols; j++) {
            // printf("#    [");
            // for (int k = 0; k < 3; k++) {
            printf(" '%s',", array[i][j]);
            //}
            // printf(" ],\n");
        }
        printf("  ],\n");
    }
    printf("#]\n");
    fflush(stdout);
#endif
    return array;
}
// Short
DLLEXPORT
short test(short value) {
    // return the opposite; makes sure we're updating ST(0)
    return -value;
}

DLLEXPORT
short test(int pos, short *ptr) {
    return -ptr[pos];
}
DLLEXPORT
short ***Ret_ShortPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;
    short c = 'a';

    short ***array = (short ***)malloc(sizeof(short **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (short **)malloc(sizeof(short *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (short *)malloc(sizeof(short) * depth + 1);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = c * (c % 2 ? 1 : -1); //'a' + i + j + k; // i + j + k;
                c++;
            }
            array[i][j][depth] = '\0';
        }
    }
#ifdef DEBUG
    printf("#[\n");
    for (int i = 0; i < rows; i++) {
        printf("#  [");
        for (int j = 0; j < cols; j++) {
            // printf("#    [");
            // for (int k = 0; k < 3; k++) {
            printf(" '%s',", array[i][j]);
            //}
            // printf(" ],\n");
        }
        printf("  ],\n");
    }
    printf("#]\n");
    fflush(stdout);
#endif
    return array;
}
// UShort
DLLEXPORT
unsigned short test(unsigned short value) {
    return value;
}

DLLEXPORT
unsigned short test(int pos, unsigned short *ptr) {
    return ptr[pos];
}
DLLEXPORT
unsigned short ***Ret_UShortPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;
    unsigned short c = 'a';

    unsigned short ***array = (unsigned short ***)malloc(sizeof(unsigned short **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (unsigned short **)malloc(sizeof(unsigned short *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (unsigned short *)malloc(sizeof(unsigned short) * depth + 1);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = c++;
            }
            array[i][j][depth] = '\0';
        }
    }
#ifdef DEBUG
    printf("#[\n");
    for (int i = 0; i < rows; i++) {
        printf("#  [");
        for (int j = 0; j < cols; j++) {
            // printf("#    [");
            // for (int k = 0; k < 3; k++) {
            printf(" '%s',", array[i][j]);
            //}
            // printf(" ],\n");
        }
        printf("  ],\n");
    }
    printf("#]\n");
    fflush(stdout);
#endif
    return array;
}

// Int
DLLEXPORT
int test(int value) {
    // return the opposite; makes sure we're updating ST(0)
    return -value;
}

DLLEXPORT
int test(int pos, int *ptr) {
    return -ptr[pos];
}

DLLEXPORT
int test(int x, int y, int **ptr) {
    return ptr[x][y];
}

DLLEXPORT
int test(int x, int y, int depth, int ***ptr) {
    return ptr[x][y][depth];
}

DLLEXPORT
int **Ret_IntPtrPtr() {
    int rows = 3;
    int cols = 3;
    int **array = (int **)malloc(sizeof(int *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (int *)malloc(sizeof(int) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %d, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

DLLEXPORT
int ***Ret_IntPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;

    int ***array = (int ***)malloc(sizeof(int **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (int **)malloc(sizeof(int *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (int *)malloc(sizeof(int) * depth);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = i * cols * depth + j * depth + k;
            }
        }
    }
    return array;
}

// Long
DLLEXPORT
long test(long value) {
    return -value;
}

DLLEXPORT
long test(int pos, long *ptr) {
    return ptr[pos];
}

DLLEXPORT
long test(int x, int y, long **ptr) {
    return ptr[x][y];
}
DLLEXPORT
long test(int x, int y, int z, long ***ptr) {
    return ptr[x][y][z];
}

DLLEXPORT
long **Ret_LongPtrPtr() {
    int rows = 3;
    int cols = 3;
    long **array = (long **)malloc(sizeof(long *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (long *)malloc(sizeof(long) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %d, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

DLLEXPORT
long ***Ret_LongPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;

    long ***array = (long ***)malloc(sizeof(long **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (long **)malloc(sizeof(long *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (long *)malloc(sizeof(long) * depth);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = i * cols * depth + j * depth + k; // needlessly complex...
            }
        }
    }
    return array;
}

// ULong
DLLEXPORT
unsigned long test(int pos, unsigned long *ptr) {
    return ptr[pos];
}

DLLEXPORT
unsigned long test(int x, int y, int z, unsigned long ***ptr) {
    return ptr[x][y][z];
}

DLLEXPORT
unsigned long **Ret_ULongPtrPtr() {
    int rows = 3;
    int cols = 3;
    unsigned long **array = (unsigned long **)malloc(sizeof(unsigned long *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (unsigned long *)malloc(sizeof(unsigned long) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %d, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

// LongLong
DLLEXPORT
long long test(int pos, long long *ptr) {
    return ptr[pos];
}

DLLEXPORT
long long test(int x, int y, int z, long long ***ptr) {
    return ptr[x][y][z];
}

DLLEXPORT
long long **Ret_LongLongPtrPtr() {
    int rows = 3;
    int cols = 3;
    long long **array = (long long **)malloc(sizeof(long long *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (long long *)malloc(sizeof(long long) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %d, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

// ULongLong
DLLEXPORT
unsigned long long test(int pos, unsigned long long *ptr) {
    return ptr[pos];
}

DLLEXPORT
unsigned long long test(int x, int y, int z, unsigned long long ***ptr) {
    return ptr[x][y][z];
}

DLLEXPORT
unsigned long long **Ret_ULongLongPtrPtr() {
    int rows = 3;
    int cols = 3;
    unsigned long long **array = (unsigned long long **)malloc(sizeof(unsigned long long *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (unsigned long long *)malloc(sizeof(unsigned long long) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %d, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

// Float
DLLEXPORT
float test(int pos, float *ptr) {
    return ptr[pos];
}

DLLEXPORT
float test(int x, int y, int z, float ***ptr) {
    return ptr[x][y][z];
}

DLLEXPORT
float **Ret_FloatPtrPtr() {
    int rows = 3;
    int cols = 3;
    float **array = (float **)malloc(sizeof(float *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (float *)malloc(sizeof(float) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = 1 + (i * .1) + (j * .01);
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %f, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}
// Double
DLLEXPORT
double test(int pos, double *ptr) {
    return ptr[pos];
}

DLLEXPORT
double test(int x, int y, int z, double ***ptr) {
    return ptr[x][y][z];
}

DLLEXPORT
double **Ret_DoublePtrPtr() {
    int rows = 3;
    int cols = 3;
    double **array = (double **)malloc(sizeof(double *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (double *)malloc(sizeof(double) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = 1 + (i * .1) + (j * .01);
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %f, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

// Str
static const char *common_names[][2] = {
    {"John", "Doe"},       {"Mary", "Smith"},     {"Michael", "Brown"}, {"Susan", "Williams"},
    {"David", "Johnson"},  {"Jennifer", "Jones"}, {"William", "Davis"}, {"Elizabeth", "Wilson"},
    {"Richard", "Miller"}, {"Linda", "Moore"}};

DLLEXPORT
char ***Ret_ArrayStr() {
    char ***name_array = (char ***)malloc(sizeof(char **) * 10);
    for (int i = 0; i < 10; i++) {
        name_array[i] = (char **)malloc(sizeof(char *) * 2);
        name_array[i][0] = (char *)common_names[i][0];
        name_array[i][1] = (char *)common_names[i][1];
        warn("# %d: %s, %s", i, name_array[i][1], name_array[i][0]);
    }
    return name_array;
}
// WChar

// WStr
/* Mangler doesn't yet understand std::
// Function to reverse a wide string
std::wstring reverse_string(std::wstring str) {
  // Create a reversed string
  std::wstring reversed_str;

  // Loop through the original string from back to front
  for (int i = str.length() - 1; i >= 0; i--) {
    // Add the current character to the reversed string
    reversed_str += str[i];
  }

  // Return the reversed string
  return reversed_str;
}*/

#include <wchar.h>
DLLEXPORT
wchar_t *reverse(wchar_t *str) {
    int len = wcslen(str);
    wchar_t *reversed_str = (wchar_t *)malloc(sizeof(wchar_t) * len);
    warn("# len: %d", len);
    for (int i = 0, j = len - 1; i < len; i++, j--) {
        reversed_str[i] = str[j];
    }
    reversed_str[len] = L'\0';
    return reversed_str;
}
DLLEXPORT
const wchar_t **Ret_WStrPtr() {
    static const wchar_t *phrases[] = {L"안녕하세요", L"감사합니다", L"미안합니다",
                                       L"잘 부탁합니다", L"안녕히 계세요"};
    return phrases;
}

//

DLLEXPORT
unsigned int test(unsigned int value) {
    // return the opposite; makes sure we're updating ST(0)
    return value;
}

DLLEXPORT
unsigned int test(int pos, unsigned int *ptr) {
    return ptr[pos];
}

DLLEXPORT
unsigned int test(int x, int y, unsigned int **ptr) {
    return ptr[x][y];
}

DLLEXPORT
unsigned int test(int x, int y, int depth, unsigned int ***ptr) {
    return ptr[x][y][depth];
}

DLLEXPORT
unsigned int **Ret_UIntPtrPtr() {
    int rows = 3;
    int cols = 3;
    unsigned int **array = (unsigned int **)malloc(sizeof(unsigned int *) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (unsigned int *)malloc(sizeof(unsigned int) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
#ifdef DEBUG
    printf("# [\n");
    for (int i = 0; i < 3; i++) {
        printf("#   [");
        for (int j = 0; j < 3; j++) {
            printf(" %d, ", array[i][j]);
        }
        printf("], # %p\n", array[i]);
    }
    printf("# ]\n");
    fflush(stdout);
#endif
    return array;
}

DLLEXPORT
unsigned int ***Ret_UIntPtrPtrPtr() {
    int rows = 3;
    int cols = 3;
    int depth = 3;

    unsigned int ***array = (unsigned int ***)malloc(sizeof(unsigned int **) * rows);
    for (int i = 0; i < rows; i++) {
        array[i] = (unsigned int **)malloc(sizeof(unsigned int *) * cols);
        for (int j = 0; j < cols; j++) {
            array[i][j] = (unsigned int *)malloc(sizeof(unsigned int) * depth);
            for (int k = 0; k < depth; k++) {
                array[i][j][k] = i * cols * depth + j * depth + k;
            }
        }
    }
    return array;
}

//

DLLEXPORT
bool test(char value, char *ptr) {
    if (ptr == NULL) return false;
    if (value == *ptr) return true;
    return false;
}
DLLEXPORT
int test(char *name, char **names) {
    for (int i = 0; names[i] != NULL; ++i) {
        if (strcmp(names[i], name) == 0) return i + 1;
    }
    return -1;
}
DLLEXPORT
int xxxtest(char *value, char **ptr, char *yep) {
    int ret = 0;

    return ret;
}

DLLEXPORT
bool test(char *value, char *ptr) {
    int result = strcmp(value, ptr);
    // Check the result.
    if (result == 0) { return true; }
    else if (result < 0) {
        // printf("String 1 is less than string 2.\n");
    }
    else {
        // printf("String 1 is greater than string 2.\n");
    }
    return false;
}

DLLEXPORT
char *Ret_CharPtr(char in) {
    if (in == '?') return NULL;
    char *retval = (char *)malloc(sizeof(char) * 17);
    if (in == 'm')
        strcpy(retval, "This is string m");
    else
        strcpy(retval, "This is string ?");
    //*retval = in;
    return retval;
}
DLLEXPORT
char **Ret_CharPtrPtr(char in) {
    char **retval = (char **)malloc(sizeof(char *));
    *retval = (char *)malloc(sizeof(char));
    *(*retval) = in;
    return retval;
}
DLLEXPORT
char **store_strings(void) {
    char **pointers = (char **)malloc(2 * sizeof(char *));

    char *string1 = (char *)malloc(17 * sizeof(char));
    char *string2 = (char *)malloc(17 * sizeof(char));

    strcpy(string1, "This is string 1");
    strcpy(string2, "This is string 2");

    // Store the pointers to the strings.
    pointers[0] = string1;
    pointers[1] = string2;

    // Return the pointer.
    return pointers;
}

DLLEXPORT
bool test(double value, double *ptr) {
    if (value == *ptr) return true;
    return false;
}
DLLEXPORT
bool test(double value, double **ptr) {
    if (value == **ptr) return true;
    return false;
}

DLLEXPORT
bool testIntPointer(int value, int *ptr) {
    //~ warn("# %d == %d = %b", value, *ptr, (*ptr == value));
    if (value == *ptr) return true;
    return false;
}
DLLEXPORT
bool testIntPointerPointer(int value, int **ptr) {
    if (value == **ptr) return true;
    return false;
}
/*
bool testLong(long value, long *ptr) {
    return (*ptr == value);
}

bool testLongPointer(long value, long **ptr) {
    return (**ptr == value);
}

bool testFloat(float value, float *ptr) {
    return (*ptr == value);
}

bool testFloatPointer(float value, float **ptr) {
    return (**ptr == value);
}

bool testDouble(double value, double *ptr) {
    return (*ptr == value);
}

bool testDoublePointer(double value, double **ptr) {
    return (**ptr == value);
}

bool testChar(char value, char *ptr) {
    return (*ptr == value);
}

bool testCharPointer(char value, char **ptr) {
    return (**ptr == value);
}

DLLEXPORT const char *pi(int *v) {
    switch (*v) {
    case 1:
        return "One";
    case 1000:
        return "Thousand";
    default:
        return "Ouch!";
    }
}
DLLEXPORT const char *ppi(int **v) {
    switch (**v) {
    case 1:
        return "One";
    case 1000:
        return "Thousand";
    default:
        return "Ouch!";
    }
}

DLLEXPORT void *take_star_sv(void *blah) {

    return blah;

    // return "Hi";
}
DLLEXPORT const char *take_star_star_sv(void **blah) {
    return "Hi";
}
*/
extern "C" {
DLLEXPORT int take_pointer_to_array_of_ints(int *blah[10]) {
    int retval = 0;
    //~ warn("Heeloooo %p", blah);
    //~ DumpHex(*(void **)blah, 40);
    for (int i = 0; i < 10; i++) {
        warn("# %d ", (*blah)[i]);
        retval += (*blah)[i];
    }
    return retval;
}
}

/*




















*/

static void *sv_holder;
DLLEXPORT int set_sv_pointer(void *in) {
    sv_holder = in;
    return 1;
}

DLLEXPORT
void swapnum(int &i, int &j) {
    warn("IN");
    int temp = i;
    i = j;
    j = temp;
    warn("HERE!");
}

/*
DLLEXPORT void * get_sv_pointer(){
    return sv_holder;
}
*/

union funn
{
    int i;
};

typedef union
{
    int i;
} OOK;

DLLEXPORT void get_sv_pointer(union funn i) {
    return;
}

DLLEXPORT void get_sv_pointer(OOK *i) {
    return;
}

#include <iostream>
#include <string>
#include <typeinfo>
using namespace std;

// defining class template
template <typename t> class student
{
  private:
    string student_name;
    t total_marks;

  public:
    student();
    // parameterized constructor
    /*student(const char *c, t m) {
        warn("Working [const char *, double]");
        warn("STUDENT! %s", c);
        string n = c;

        student_name = n;
        total_marks = m;
    }*/
    student(string c, t m) {
        warn("Working [string, double]");
        student_name = c;
        total_marks = m;
    }
    DLLEXPORT
    void getinfo() {
        cout << "STUDENT NAME: " << student_name << endl;
        cout << "TOTAL MARKS: " << total_marks << endl;
        cout << "Type ID: " << typeid(total_marks).name() << endl;
    }
};

DLLEXPORT
int mainX() {
    // similar to student <int> s1("vipul",100);
    student<int> s1("vipul", 100);
    student<double> s2("yash", 100.0);

    s1.getinfo();
    s2.getinfo();

    int *arr[10];

    for (int i = 0; i < 10; i++) {
        arr[i] = (int *)malloc(sizeof(int));
    }

    for (int i = 0; i < 10; i++) {
        *arr[i] = i * i;
    }

    take_pointer_to_array_of_ints(arr);

    return 0;
}

DLLEXPORT
void take_string(char *test) {
    warn("line: %s", test);
}
DLLEXPORT
void take_string(string test) {
    warn("line: %s", test.c_str());
}

// Union
union A
{
    int x;
    int y[4];
};
DLLEXPORT
size_t union_A_sizeof() {
    return sizeof(union A);
}

struct B {
    A a;
};
union C
{
    B b;
    int k;
};
DLLEXPORT
size_t union_C_sizeof() {
    return sizeof(union C);
}

DLLEXPORT
int test(C c) {
    c.b.a.y[3] += 4;
    return c.b.a.y[3];
}

DLLEXPORT
C Ret_Union() {
    C c;
    c.b.a.y[3] = 4;
    return c;
}

DLLEXPORT
void Update_UnionPtr(C *c) {
    c->k = 100;
    return;
}

union C2
{
    int i;
    float j;
};

DLLEXPORT
int test(int index, C2 *c) {
    return c[index].i;
}
DLLEXPORT
union C2 *Ret_UnionPtr(int n) {
    union C2 *unions = (union C2 *)malloc(n * sizeof(union C2));
    int i = 5;
    float j = 3.0L;
    for (int index = 0; index < n; ++index) {
        if (index % 2) {
            unions[index].i = i;
            i *= 2;
            //~ warn("%d.i == %d", index, unions[index].i);
        }
        else {
            unions[index].j = j;
            j *= 2;
            //~ warn("%d.j == %f", index, unions[index].j);
        }
    }
    return unions;
}

typedef int A1[];
typedef int A2[5];

DLLEXPORT
int test(A1 a, int size) {
    int ret = 0;
    for (int i = 0; i < size; i++)
        ret += a[i];
    return ret;
}

DLLEXPORT
int test(A2 a) {
    int ret = 0;
    for (int i = 0; i < 5; i++)
        ret += a[i];
    return ret;
}

//~ _Z4testPA10_i
DLLEXPORT
int test(int numbers[5][10]) {
    int ret = 0;
    int rows = 10;
    int cols = 5;
    warn("HERE");
    DumpHex(numbers, sizeof(int **));

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            warn("[i: %d, j: %d]", i, j);

            warn("# numbers[%d][%d] == %d", i, j, numbers[i][j]);

            ret += numbers[i][j];
        }
    }
    warn("RET: %d", ret);
    return ret;
}

DLLEXPORT
int *Ret_A1() {
    int size = 100;
    int *ret = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++) {
        ret[i] = i;
        //~ warn("# ret[%d] == %d", i, ret[i]);
    }
    return ret;
}

DLLEXPORT
int *Ret_A2() {
    int size = 5;
    int *ret = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++) {
        ret[i] = i;
        //~ warn("# ret[%d] == %d", i, ret[i]);
    }
    return ret;
}

DLLEXPORT
int **Ret_A3() {
    int rows = 10;
    int cols = 5;
    int **ret = (int **)malloc(sizeof(int *) * rows);
    for (int i = 0; i < rows; i++) {
        ret[i] = (int *)malloc(sizeof(int) * cols);
        for (int j = 0; j < cols; j++) {
            ret[i][j] = i * cols + j;
            //~ warn("# ret[%d, %d] == %d", i, j, ret[i][j]);
        }
    }
    return ret;
}

struct S1 {
    int i;
};

DLLEXPORT
int test(S1 s) {
    return s.i;
}

DLLEXPORT
int test(int index, S1 *s) {
    return s[index].i;
}

DLLEXPORT
S1 Ret_Struct() {
    S1 ret = {300};
    return ret;
}

DLLEXPORT
S1 *Ret_StructPtr() {
    S1 *ret = new S1;
    ret->i = 500;
    return ret;
}

DLLEXPORT
S1 **Ret_StructPtrPtr(int size) {
    S1 **ret = new S1 *[size];
    for (int i = 0; i < size; i++) {
        ret[i] = new S1();
        ret[i]->i = i;
        //~ warn("# ret[%d]->i == %d", i, ret[i]->i);
    }
    return ret;
}

DLLEXPORT
int test(S1 **list) {
    int ret = 0;
    for (int i = 0; i < 5; ++i) {
        ret += list[i]->i;
    }
    return ret;
}

/*
#include <cxxabi.h>
#include <stdio.h>
#include <stdlib.h>

DLLEXPORT
int main() {
    const char *mangled_name =
        "_ZNK3MapI10StringName3RefI8GDScriptE10ComparatorIS0_E16DefaultAllocatorE3hasERKS0_";
    int status = -1;
    char *demangled_name = abi::__cxa_demangle(mangled_name, NULL, NULL, &status);
    printf("Demangled: %s\n", demangled_name);
    free(demangled_name);
    return 0;
}
*/
