#include "std.h"

class MyClass
{                         // The class
  public:                 // Access specifier
    int myNum;            // Attribute (int variable)
    const char *myString; // Attribute (string variable)
    int speed(int maxSpeed);
    int speed();
};

int MyClass::speed() {
    return 0;
}
int MyClass::speed(int maxSpeed) {
    return maxSpeed;
}

DLLEXPORT MyClass setup() {
    MyClass myObj = {.myNum = 15, .myString = "Some text"};
    return myObj;
}

DLLEXPORT MyClass setup(int i) {
    MyClass myObj = {.myNum = i, .myString = "Some different text"};
    return myObj;
}

struct jkkl {
    int i;
};

class enclose
{
    struct nested // private member
    {
        void get_it() {}
    };

  public:
    static nested f() { return nested{}; }
};

int main() {
    // enclose::nested n1 = enclose::f(); // error: 'nested' is private

    enclose::f().get_it();  // OK: does not name 'nested'
    auto n2 = enclose::f(); // OK: does not name 'nested'
    n2.get_it();
}

DLLEXPORT void take_array1(int i[8]) {}
DLLEXPORT void take_array2(int i[]) {}

union A
{
    int x;
    int y[4];
};
struct B {
    A a;
};
union C
{
    B b;
    int k;
};
DLLEXPORT
int f() {
    C c;               // does not start lifetime of any union member
    c.b.a.y[3] = 4;    // OK: "c.b.a.y[3]", names union members c.b and c.b.a.y;
                       // This creates objects to hold union members c.b and c.b.a.y
    return c.b.a.y[3]; // OK: c.b.a.y refers to newly created object
}

struct X {
    const int a;
    int b;
};
union Y
{
    X x;
    int k;
};
DLLEXPORT
void g(Y hi) {
    Y y = {{1, 2}}; // OK, y.x is active union member
    int n = y.x.a;
    y.k = 4;   // OK: ends lifetime of y.x, y.k is active member of union
    y.x.b = n; // undefined behavior: y.x.b modified outside its lifetime,
               // "y.x.b" names y.x, but X's default constructor is deleted,
               // so union member y.x's lifetime does not implicitly start
}

typedef struct {

} Structure;

DLLEXPORT
void test_function(bool a, char b, unsigned char c, short d, unsigned short e, int f,
                   unsigned int g, long h, unsigned long i, long long j, unsigned long long k,
                   float l, double m, void *n, Structure o, Structure *p, void *q, void *r,
                   void **s, Structure t) {
    ;
}