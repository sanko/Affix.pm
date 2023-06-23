#include "std.h"
#include <stdlib.h>

typedef struct {
    int myNum;
    const char *myString;
} MyStruct;

DLLEXPORT MyStruct get_struct() {
    MyStruct myObj;
    myObj.myNum = 15;
    //    .myString = "Some text"};
    myObj.myString = (const char *)malloc(15);
    strcpy(myObj.myString, "This is a test");
    return myObj;
}
