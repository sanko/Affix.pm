//#!gcc

//&& ./a.out

// C Program to demonstrate working of anonymous union
#include <stdio.h>
struct Struct {
	union {
		char   alpha;
		int    num;
        char * string;
        void * pointer;
	};
};
typedef struct
{int num;} blah;
struct TYPEA {
  char data[30]; // or whatever
};

struct TYPEB {
  double x, y; // or whatever
};

struct some_info {
  int type; // set accordingly
  union {
    struct TYPEA a;
    struct TYPEB b;
  } data; // access with some_info_object.data.a or some_info_object.data.b
};
int grab_int (void* i) {
    blah *obj = (blah *) i;

    	printf(".num = %d\n", obj->num);

return obj->num;

}
int main()
{
	struct Struct x, y;
	x.num = 65;
	y.alpha = 'A';

	// Note that members of union are accessed directly
	printf("y.alpha = %c, x.num = %d\n", y.alpha, x.num);
    int xjkl = grab_int(&x);

    int size = 15;
    struct Struct fun[size];
    fun[0].alpha = 100;
    fun[1].num  = 200;
    fun[2].string= "test";
    fun[3].pointer=&y;
	printf("fun[0].alpha = %c, .num = %d, .pointer=%s\n",
        fun[0].alpha,
        fun[1].num,
        fun[2].string);

	return 0;
}

