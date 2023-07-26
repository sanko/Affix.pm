#include "std.h"

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

bool testBoolPointer(bool value, bool *ptr) {
    if (value == *ptr) return true;
    return false;
}

bool testBoolPointerPointer(bool value, bool **ptr) {
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
int main() {
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
