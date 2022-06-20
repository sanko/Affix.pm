/*

 Package: dyncall
 Library: test
 File: test/callback_plain_c++/test_main.c
 Description:

   Tests only C++ specifics:
   - method callback handlers (= standard dyncallback handler, but testing
     calling convention mode setting and hidden this ptr)
   - calling functions or methods that take/return non-trivial C++ aggregates

 License:

   Copyright (c) 2022 Tassilo Philipp <tphilipp@potion-studios.com>

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include "../../dyncallback/dyncall_callback.h"
#include "../common/platformInit.h"
#include "../common/platformInit.c" /* Impl. for functions only used in this translation unit */


#if defined(DC__Feature_AggrByVal)
struct Triv {
  float t;
};

struct NonTriv {
  int i, j;
  static int a, b;
  NonTriv(int a, int b) : i(a),j(b) { printf("%s\n", "NonTriv::NonTriv(int,int) called"); }
  NonTriv(const NonTriv& rhs) { printf("%s\n", "NonTriv::NonTriv(const NonTriv&) called"); i = a++; j = b++; }
};
int NonTriv::a = 13;
int NonTriv::b = 37;



char cbNonTrivAggrArgHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  int     arg1;
  NonTriv arg2(0, 0);
  NonTriv arg3(0, 0);
  Triv    arg4;
  double  arg5;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  // the non-triv arg4 test basically assures that the aggr description list is
  // respected (with first two being NULL as non-trivial aggrs)
  arg1 = dcbArgInt(args);                   printf("1st argument: %d\n", arg1);
  arg2 = *(NonTriv*)dcbArgAggr(args, NULL); printf("2nd argument: %d %d\n", arg2.i, arg2.j);
  arg3 = *(NonTriv*)dcbArgAggr(args, NULL); printf("3nd argument: %d %d\n", arg3.i, arg3.j);
  dcbArgAggr(args, (DCpointer)&arg4);       printf("4th argument: %f\n", arg4.t);
  arg5 = dcbArgDouble(args);                printf("5th argument: %f\n", arg5);

  // the non-triv aggregate members are 14, 38 and 15, 39, respectively, b/c of
  // copy-ctor call on *passing* them to this handler (not b/c of assignment
  // operator, above); see above where 13 and 37 are initialized
  result->d = *ud + arg1 + arg2.i + arg2.j + arg3.i + arg3.j + arg4.t + arg5;
  return 'd';
}


int testNonTrivAggrArgsCallback()
{
  DCCallback* cb;
  Triv t = { 1.75f };
  NonTriv a(1, 2);
  NonTriv b(a);

  int ret = 1;
  double result = 0;
  int userdata = 1337;

  DCaggr *triv_aggr = dcNewAggr(1, sizeof(t));
  dcAggrField(triv_aggr, DC_SIGCHAR_FLOAT, offsetof(Triv, t), 1);
  dcCloseAggr(triv_aggr);

  DCaggr *aggrs[3] = { NULL, NULL, triv_aggr }; // NULL b/c non-trivial aggrs, see manpage

  cb = dcbNewCallback2("iAAAd)d", &cbNonTrivAggrArgHandler, &userdata, aggrs);

  result = ((double(*)(int, NonTriv, NonTriv, Triv, double))cb)(123, a, b, t, 4.5);
  dcbFreeCallback(cb);

  printf("successfully returned from callback\n");
  printf("retval (should be 1572.25): %f\n", result);

  ret = result == 1572.25 && ret;

  return ret;
}



char cbNonTrivAggrReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  printf("reached callback\n");
  dcbReturnAggr(args, result, NULL);    // this sets result->p to the non-triv aggr space allocated by the calling convention
  *(NonTriv*)result->p = NonTriv(1, 3); // explicit non-copy ctor and assignment operator, so not using NonTriv's statics a and b

  return 'A';
}


int testNonTrivAggrReturnCallback()
{
  int ret = 1;

  {
    DCCallback* cb;
    DCaggr *aggrs[1] = { NULL }; // one non-triv aggr
    cb = dcbNewCallback2(")A", &cbNonTrivAggrReturnHandler, NULL, aggrs);

    NonTriv result = ((NonTriv(*)())cb)(); // potential copy elision on construction

    int a = NonTriv::a-1;
    int b = NonTriv::b-1;
    printf("successfully returned from callback 1/2\n");
    printf("retval w/ potential retval optimization and copy-init (should be %d %d for init or %d %d for copy, both allowed by C++): %d %d\n", 1, 3, a, b, result.i, result.j);

    ret = ((result.i == 1 && result.j == 3) || (result.i == a && result.j == b)) && ret;

	// avoid copy elision on construction
	result.i = result.j = -77;
	result = ((NonTriv(*)())cb)(); // potential copy elision

    a = NonTriv::a-1;
    b = NonTriv::b-1;
    printf("successfully returned from callback 2/2\n");
    printf("retval w/ potential retval optimization and copy-init (should be %d %d for init or %d %d for copy, both allowed by C++): %d %d\n", 1, 3, a, b, result.i, result.j);

    dcbFreeCallback(cb);

    ret = ((result.i == 1 && result.j == 3) || (result.i == a && result.j == b)) && ret;
  }

  return ret;
}
#endif


int main()
{
  int result = 1;

  dcTest_initPlatform();

#if defined(DC__Feature_AggrByVal)
  result = testNonTrivAggrArgsCallback() && result;
  result = testNonTrivAggrReturnCallback() && result;
#endif

  printf("result: callback_plain_c++: %d\n", result);

  dcTest_deInitPlatform();

  return !result;
}

