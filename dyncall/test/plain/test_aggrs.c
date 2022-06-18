/*

 Package: dyncall
 Library: test
 File: test/plain/test_aggrs.c
 Description:
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




#include "../../dyncall/dyncall.h"
#include "../../dyncall/dyncall_signature.h"
#include "../../dyncall/dyncall_aggregate.h"
#include <stdio.h>


#if defined(DC__Feature_AggrByVal)

#if !defined(DC__OS_Win32)
#  define __cdecl
#endif

typedef struct {
	unsigned char a;
} U8;

typedef struct {
	unsigned char a;
	double b;
} U8_Double;

typedef struct {
	float a;
	float b;
} Float_Float;

typedef struct {
	double a;
	unsigned char b;
} Double_U8;

typedef struct {
	float f;
} NestedFloat;

typedef struct {
	int a;
	NestedFloat b;
} Int_NestedFloat;

typedef struct {
	double f;
} NestedDouble;

typedef struct {
	int a;
	NestedDouble b;
} Int_NestedDouble;

typedef struct {
	double a;
	double b;
	double c;
} Three_Double;

typedef struct {
	int       a;
	long long b;
} Int_LongLong;

/* large struct: more than 8 int/ptr and 8 fp args, more than are passed by reg for both win and sysv for example */
typedef struct {
	double       a;
	double       b;
	double       c;
	long long    d;
	char         e;
	char         f;
	double       g;
	double       h;
	double       i;
	float        j;
	int          k;
	float        l;
	double       m;
	short        n;
	long         o;
	int          p;
	unsigned int q;
	long long    r;
} More_Than_Regs;


static U8               __cdecl fun_return_u8(unsigned char a)                        { U8               r;  r.a = a;                       return r; }
static U8_Double        __cdecl fun_return_u8_double(unsigned char a, double b)       { U8_Double        r;  r.a = a;  r.b   = b;           return r; }
static Double_U8        __cdecl fun_return_double_u8(double a, unsigned char b)       { Double_U8        r;  r.a = a;  r.b   = b;           return r; }
static Int_NestedFloat  __cdecl fun_return_int_nested_float(int a, float b)           { Int_NestedFloat  r;  r.a = a;  r.b.f = b;           return r; }
static Int_NestedDouble __cdecl fun_return_int_nested_double(int a, double b)         { Int_NestedDouble r;  r.a = a;  r.b.f = b;           return r; }
static Three_Double     __cdecl fun_return_three_double(double a, double b, double c) { Three_Double     r;  r.a = a;  r.b   = b;  r.c = c; return r; }


int testAggrReturns()
{
	int ret = 1;

	DCCallVM* vm = dcNewCallVM(4096);
	dcMode(vm,DC_CALL_C_DEFAULT);
	{
		U8 expected = fun_return_u8(5), returned = { 124 };

		DCaggr *s = dcNewAggr(1, sizeof(expected));

		dcAggrField(s, DC_SIGCHAR_UCHAR, offsetof(U8, a), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcBeginCallAggr(vm, s);
		dcArgChar(vm, expected.a);

		dcCallAggr(vm, (DCpointer) &fun_return_u8, s, &returned);

		dcFreeAggr(s);

		printf("r:{C}  (cdecl): %d\n", (returned.a == expected.a));
		ret = returned.a == expected.a && ret;
	}
	{
		U8_Double expected = fun_return_u8_double(5, 5.5), returned = { 6, 7.8 };

		DCaggr *s = dcNewAggr(2, sizeof(expected));

		dcAggrField(s, DC_SIGCHAR_UCHAR,  offsetof(U8_Double, a), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(U8_Double, b), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcBeginCallAggr(vm, s);
		dcArgChar(vm, expected.a);
		dcArgDouble(vm, expected.b);

		dcCallAggr(vm, (DCpointer) &fun_return_u8_double, s, &returned);

		dcFreeAggr(s);

		printf("r:{Cd}  (cdecl): %d\n", (returned.a == expected.a && returned.b == expected.b));
		ret = returned.a == expected.a && returned.b == expected.b && ret;
	}
	{
		Double_U8 expected = fun_return_double_u8(5.5, 42), returned = { 6.7, 8 };

		DCaggr *s = dcNewAggr(2, sizeof(expected));

		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Double_U8, a), 1);
		dcAggrField(s, DC_SIGCHAR_UCHAR,  offsetof(Double_U8, b), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcBeginCallAggr(vm, s);
		dcArgDouble(vm, expected.a);
		dcArgChar(vm, expected.b);

		dcCallAggr(vm, (DCpointer) &fun_return_double_u8, s, &returned);

		dcFreeAggr(s);

		printf("r:{dC}  (cdecl): %d\n", (returned.a == expected.a && returned.b == expected.b));
		ret = returned.a == expected.a && returned.b == expected.b && ret;
	}
	{
		Int_NestedFloat expected = fun_return_int_nested_float(24, 2.5f), returned = { 25, { 3.5f } };
		DCaggr *s, *s_;

		s_ = dcNewAggr(1, sizeof(NestedFloat));
		dcAggrField(s_, DC_SIGCHAR_FLOAT, offsetof(NestedFloat, f), 1);
		dcCloseAggr(s_);

		s = dcNewAggr(2, sizeof(expected));
		dcAggrField(s, DC_SIGCHAR_INT, offsetof(Int_NestedFloat, a), 1);
		dcAggrField(s, DC_SIGCHAR_AGGREGATE, offsetof(Int_NestedFloat, b), 1, s_);
		dcCloseAggr(s);

		dcReset(vm);
		dcBeginCallAggr(vm, s);
		dcArgInt(vm, expected.a);
		dcArgFloat(vm, expected.b.f);

		dcCallAggr(vm, (DCpointer) &fun_return_int_nested_float, s, &returned);

		dcFreeAggr(s_);
		dcFreeAggr(s);

		printf("r:{i{f}}  (cdecl): %d\n", (returned.a == expected.a && returned.b.f == expected.b.f));
		ret = returned.a == expected.a && returned.b.f == expected.b.f && ret;
	}
	{
		Int_NestedDouble expected = fun_return_int_nested_double(24, 2.5), returned = { 25, { 3.5f } };
		DCaggr *s, *s_;

		s_ = dcNewAggr(1, sizeof(NestedDouble));
		dcAggrField(s_, DC_SIGCHAR_DOUBLE, offsetof(NestedDouble, f), 1);
		dcCloseAggr(s_);

		s = dcNewAggr(2, sizeof(expected));
		dcAggrField(s, DC_SIGCHAR_INT, offsetof(Int_NestedDouble, a), 1);
		dcAggrField(s, DC_SIGCHAR_AGGREGATE, offsetof(Int_NestedDouble, b), 1, s_);
		dcCloseAggr(s);

		dcReset(vm);
		dcBeginCallAggr(vm, s);
		dcArgInt(vm, expected.a);
		dcArgDouble(vm, expected.b.f);

		dcCallAggr(vm, (DCpointer) &fun_return_int_nested_double, s, &returned);

		dcFreeAggr(s_);
		dcFreeAggr(s);

		printf("r:{i{d}}  (cdecl): %d\n", (returned.a == expected.a && returned.b.f == expected.b.f));
		ret = returned.a == expected.a && returned.b.f == expected.b.f && ret;
	}
	{
		Three_Double expected = fun_return_three_double(1.5, 2.5, 3.5), returned = { 2.5, 3.5, 4.5 };

		DCaggr *s = dcNewAggr(3, sizeof(expected));

		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, a), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, b), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, c), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcBeginCallAggr(vm, s);
		dcArgDouble(vm, expected.a);
		dcArgDouble(vm, expected.b);
		dcArgDouble(vm, expected.c);

		dcCallAggr(vm, (DCpointer) &fun_return_three_double, s, &returned);

		dcFreeAggr(s);

		printf("r:{ddd}  (cdecl): %d\n", (returned.a == expected.a && returned.b == expected.b && returned.c == expected.c));
		ret = returned.a == expected.a && returned.b == expected.b && returned.c == expected.c && ret;
	}

	dcFree(vm);

	return ret;
}

static double __cdecl fun_take_u8(U8 s)                                                                                { return s.a; }
static double __cdecl fun_take_u8_double(U8_Double s)                                                                  { return s.a + s.b; }
static double __cdecl fun_take_float_float(Float_Float s)                                                              { return s.a + s.b; }
static double __cdecl fun_take_double_u8(Double_U8 s)                                                                  { return s.a + s.b; }
static double __cdecl fun_take_int_nested_float(Int_NestedFloat s)                                                     { return s.a + s.b.f; }
static double __cdecl fun_take_int_nested_double(Int_NestedDouble s)                                                   { return s.a + s.b.f; }
static double __cdecl fun_take_three_double(Three_Double s)                                                            { return s.a + s.b + s.c; }
static double __cdecl fun_take_mixed_fp(double a, float b, float c, int d, float e, double f, float g, Three_Double s) { return a + 2.*b + 3.*c + 4.*d + 5.*e + 6.*f + 7.*g + 8.*s.a + 9.*s.b + 10.*s.c; }
static int    __cdecl fun_take_iiiii_il(int a, int b, int c, int d, int e, Int_LongLong f)                             { return a + b + c + d + e + f.a + (int)f.b; }
static double __cdecl fun_take_more_than_regs(More_Than_Regs s)                                                        { return s.a + s.b + s.c + s.d + s.e + s.f + s.g + s.h + s.i + s.j + s.k + s.l + s.m + s.n + s.o + s.p + s.q + s.r; }


int testAggrParameters()
{
	int ret = 1;

	DCCallVM* vm = dcNewCallVM(4096);
	dcMode(vm,DC_CALL_C_DEFAULT);
	{
		U8 t = { 5 };
		double returned;

		DCaggr *s = dcNewAggr(1, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_UCHAR, offsetof(U8, a), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_u8);

		dcFreeAggr(s);

		printf("{C}  (cdecl): %d\n", returned == t.a);
		ret = returned == t.a && ret;
	}
	{
		U8_Double t = { 5, 5.5 };
		double returned;

		DCaggr *s = dcNewAggr(2, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_UCHAR,  offsetof(U8_Double, a), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(U8_Double, b), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_u8_double);

		dcFreeAggr(s);

		printf("{Cd}  (cdecl): %d\n", returned == t.a + t.b);
		ret = returned == t.a + t.b && ret;
	}
	{
		Float_Float t = { 1.5, 5.5 };
		double returned;

		DCaggr *s = dcNewAggr(2, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_FLOAT, offsetof(Float_Float, a), 1);
		dcAggrField(s, DC_SIGCHAR_FLOAT, offsetof(Float_Float, b), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_float_float);

		dcFreeAggr(s);

		printf("{ff}  (cdecl): %d\n", returned == t.a + t.b);
		ret = returned == t.a + t.b && ret;
	}
	{
		Double_U8 t = { 5.5, 42 };
		double returned;

		DCaggr *s = dcNewAggr(2, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Double_U8, a), 1);
		dcAggrField(s, DC_SIGCHAR_UCHAR,  offsetof(Double_U8, b), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_double_u8);

		dcFreeAggr(s);

		printf("{dC}  (cdecl): %d\n", returned == t.a + t.b);
		ret = returned == t.a + t.b && ret;
	}
	{
		Int_NestedFloat t = { 24, { 2.5f } };
		double returned;
		DCaggr *s, *s_;

		s_ = dcNewAggr(1, sizeof(NestedFloat));
		dcAggrField(s_, DC_SIGCHAR_FLOAT, offsetof(NestedFloat, f), 1);
		dcCloseAggr(s_);

		s = dcNewAggr(2, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_INT, offsetof(Int_NestedFloat, a), 1);
		dcAggrField(s, DC_SIGCHAR_AGGREGATE, offsetof(Int_NestedFloat, b), 1, s_);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_int_nested_float);

		dcFreeAggr(s_);
		dcFreeAggr(s);

		printf("{i{f}}  (cdecl): %d\n", returned == t.a + t.b.f);
		ret = returned == t.a + t.b.f && ret;
	}
	{
		Int_NestedDouble t = { 24, { 2.5} };
		double returned;
		DCaggr *s, *s_;

		s_ = dcNewAggr(1, sizeof(NestedDouble));
		dcAggrField(s_, DC_SIGCHAR_DOUBLE, offsetof(NestedDouble, f), 1);
		dcCloseAggr(s_);

		s = dcNewAggr(2, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_INT, offsetof(Int_NestedDouble, a), 1);
		dcAggrField(s, DC_SIGCHAR_AGGREGATE, offsetof(Int_NestedDouble, b), 1, s_);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_int_nested_double);

		dcFreeAggr(s_);
		dcFreeAggr(s);

		printf("{i{d}}  (cdecl): %d\n", returned == t.a + t.b.f);
		ret = returned == t.a + t.b.f && ret;
	}
	{
		Three_Double t = { 1.5, 2.5, 3.5 };
		double returned;

		DCaggr *s = dcNewAggr(3, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, a), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, b), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, c), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_three_double);

		dcFreeAggr(s);

		printf("{fff}  (cdecl): %d\n", returned == t.a + t.b + t.c);
		ret = returned == t.a + t.b + t.c && ret;
	}
	{
		/* w/ some prev params, so not fitting into float regs anymore (on win and sysv) */
		Three_Double t = { 1.5, 2.5, 3.5 };
		double returned;

		DCaggr *s = dcNewAggr(3, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, a), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, b), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, c), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgDouble(vm, 234.4);
		dcArgFloat(vm, 34.4f);
		dcArgFloat(vm, 4.0f);
		dcArgInt(vm, -12);
		dcArgFloat(vm, -83.9f);
		dcArgDouble(vm, -.9);
		dcArgFloat(vm, .6f);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_mixed_fp) + 84.;
		if(returned < 0.)
			returned = -returned;

		dcFreeAggr(s);

		printf("dffifdf{fff}  (cdecl): %d\n", returned < .00001);
		ret = returned < .00001 && ret;
	}
	{
		Int_LongLong t = { -17, 822LL };
		int returned;

		DCaggr *s = dcNewAggr(2, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_INT,      offsetof(Int_LongLong, a), 1);
		dcAggrField(s, DC_SIGCHAR_LONGLONG, offsetof(Int_LongLong, b), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgInt(vm, 23);
		dcArgInt(vm, -211);
		dcArgInt(vm, 111);
		dcArgInt(vm, 34);
		dcArgInt(vm, -19290);
		dcArgAggr(vm, s, &t);
		returned = dcCallInt(vm, (DCpointer) &fun_take_iiiii_il);

		dcFreeAggr(s);

		printf("iiiii{il}  (cdecl): %d\n", returned == -18528);
		ret = returned == -18528 && ret;
	}
	{
		More_Than_Regs t = { 1., 2., 3., 4, 5, 6, 7., 8., 9., 10.f, 11, 12.f, 13., 14, 15, 16, 17, 18 };
		double returned;

		DCaggr *s = dcNewAggr(18, sizeof(t));
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, a), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, b), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, c), 1);
		dcAggrField(s, DC_SIGCHAR_LONGLONG, offsetof(More_Than_Regs, d), 1);
		dcAggrField(s, DC_SIGCHAR_CHAR,     offsetof(More_Than_Regs, e), 1);
		dcAggrField(s, DC_SIGCHAR_CHAR,     offsetof(More_Than_Regs, f), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, g), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, h), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, i), 1);
		dcAggrField(s, DC_SIGCHAR_FLOAT,    offsetof(More_Than_Regs, j), 1);
		dcAggrField(s, DC_SIGCHAR_INT,      offsetof(More_Than_Regs, k), 1);
		dcAggrField(s, DC_SIGCHAR_FLOAT,    offsetof(More_Than_Regs, l), 1);
		dcAggrField(s, DC_SIGCHAR_DOUBLE,   offsetof(More_Than_Regs, m), 1);
		dcAggrField(s, DC_SIGCHAR_SHORT,    offsetof(More_Than_Regs, n), 1);
		dcAggrField(s, DC_SIGCHAR_LONG,     offsetof(More_Than_Regs, o), 1);
		dcAggrField(s, DC_SIGCHAR_INT,      offsetof(More_Than_Regs, p), 1);
		dcAggrField(s, DC_SIGCHAR_UINT,     offsetof(More_Than_Regs, q), 1);
		dcAggrField(s, DC_SIGCHAR_LONGLONG, offsetof(More_Than_Regs, r), 1);
		dcCloseAggr(s);

		dcReset(vm);
		dcArgAggr(vm, s, &t);
		returned = dcCallDouble(vm, (DCpointer) &fun_take_more_than_regs);

		dcFreeAggr(s);

		printf("{dddlccdddfifdsjiIl}  (cdecl): %d\n", returned == 171.);
		ret = returned == 171. && ret;
	}

	dcFree(vm);

	return ret;
}

#endif

