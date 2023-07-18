#include "std.h"

DLLEXPORT const char *ppi(int **v) {
    warn("HERE "
         "%p!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
         v);
    warn("**v == %d", **v);
    switch (**v) {
    case 1:
        return "One";
    case 1000:
        return "Thousand";
    default:
        return "Ouch!";
    }
    warn("HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
         "!");
}

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
    student s1("vipul", 100);
    student s2("yash", 100.0);

    s1.getinfo();
    s2.getinfo();

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
