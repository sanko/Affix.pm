#include "std.h"
#include <algorithm>
#include <stdlib.h>
#include <vector>

class MyClass
{
  public:
    int value;
    char *name;
    MyClass(int value) { this->value = value; }
    ~MyClass() { warn("Deleting MyClass object"); }
    static MyClass *create() { return new MyClass(42); }
    DLLEXPORT char *get_name();
    DLLEXPORT int set_name(char *n);
    DLLEXPORT void DESTROY();
};

DLLEXPORT int MyClass::set_name(char *n) {
    this->name = n;
    return strlen(this->name);
}
DLLEXPORT char *MyClass::get_name() {
    return this->name;
}

DLLEXPORT void MyClass::DESTROY() {
    warn("MyClass::DESTROY");
    delete this;
}

DLLEXPORT MyClass *get_class() {
    MyClass *my_class = MyClass::create();
    // warn("%s", my_class->value);
    // delete my_class;
    // return 0;
    return my_class;
}
