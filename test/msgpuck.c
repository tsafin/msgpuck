/*
 * Copyright (c) 2013-2016 MsgPuck Authors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#include "msgpuck.h"
#include "test.h"

#define BUF_MAXLEN ((1L << 18) - 1)
#define STRBIN_MAXLEN (BUF_MAXLEN - 10)

static char buf[BUF_MAXLEN + 1];
static char str[STRBIN_MAXLEN];
static char *data = buf + 1; /* use unaligned address to fail early */

#define header() note("*** %s ***", __func__)
#define footer() note("*** %s: done ***", __func__)

#define SCALAR(x) x
#define COMPLEX(x)

#define DEFINE_TEST(_type, _complex, _v, _r, _rl) ({                           \
	const char *d1 = mp_encode_##_type(data, (_v));                        \
	const char *d2 = data;                                                 \
	_complex(const char *d3 = data);                                       \
	_complex(const char *d4 = data);                                       \
	note(""#_type" "#_v"");                                                \
	is(mp_check_##_type(data, d1), 0, "mp_check_"#_type"("#_v") == 0");    \
	is(mp_decode_##_type(&d2), (_v), "mp_decode(mp_encode("#_v")) == "#_v);\
	_complex(mp_next(&d3));                                                \
	_complex(ok(!mp_check(&d4, d3 + _rl), "mp_check("#_v")"));             \
	is((d1 - data), (_rl), "len(mp_encode_"#_type"("#_v")");               \
	is(d1, d2, "len(mp_decode_"#_type"("#_v"))");                          \
	_complex(is(d1, d3, "len(mp_next_"#_type"("#_v"))"));                  \
	_complex(is(d1, d4, "len(mp_check_"#_type"("#_v"))"));                 \
	is(mp_sizeof_##_type(_v), _rl, "mp_sizeof_"#_type"("#_v")");           \
	is(memcmp(data, (_r), (_rl)), 0, "mp_encode("#_v") == "#_r);           \
	})


#define DEFINE_TEST_STRBIN(_type, _vl) ({                                      \
	note(""#_type" len="#_vl"");                                           \
	char *s1 = str;                                                        \
	for (uint32_t i = 0; i < _vl; i++) {                                   \
		s1[i] = 'a' + i % 26;                                          \
	}                                                                      \
	const char *d1 = mp_encode_##_type(data, s1, _vl);                     \
	const char *d2;                                                        \
	uint32_t len2;                                                         \
	d2 = data;                                                             \
	const char *s2 = mp_decode_##_type(&d2, &len2);                        \
	is(_vl, len2, "len(mp_decode_"#_type"(x, %u))", _vl);                  \
	d2 = data;                                                             \
	(void) mp_decode_strbin(&d2, &len2);                                   \
	is(_vl, len2, "len(mp_decode_strbin(x, %u))", _vl);                    \
	const char *d3 = data;                                                 \
	mp_next(&d3);                                                          \
	const char *d4 = data;                                                 \
	ok(!mp_check(&d4, d3 + _vl),                                           \
		"mp_check_"#_type"(mp_encode_"#_type"(x, "#_vl"))");           \
	is(d1, d2, "len(mp_decode_"#_type"(x, "#_vl")");                       \
	is(d1, d3, "len(mp_next_"#_type"(x, "#_vl")");                         \
	is(d1, d4, "len(mp_check_"#_type"(x, "#_vl")");                        \
	is(mp_sizeof_##_type(_vl), (uint32_t) (d1 - data),                     \
		"mp_sizeof_"#_type"("#_vl")");                                 \
	is(memcmp(s1, s2, _vl), 0, "mp_encode_"#_type"(x, "#_vl") == x");      \
})

#define test_uint(...)   DEFINE_TEST(uint, SCALAR, __VA_ARGS__)
#define test_int(...)    DEFINE_TEST(int, SCALAR, __VA_ARGS__)
#define test_bool(...)   DEFINE_TEST(bool, SCALAR, __VA_ARGS__)
#define test_float(...)  DEFINE_TEST(float, SCALAR, __VA_ARGS__)
#define test_double(...) DEFINE_TEST(double, SCALAR, __VA_ARGS__)
#define test_strl(...)   DEFINE_TEST(strl, COMPLEX, __VA_ARGS__)
#define test_binl(...)   DEFINE_TEST(binl, COMPLEX, __VA_ARGS__)
#define test_array(...)  DEFINE_TEST(array, COMPLEX, __VA_ARGS__)
#define test_map(...)    DEFINE_TEST(map, COMPLEX, __VA_ARGS__)
#define test_str(...)    DEFINE_TEST_STRBIN(str, __VA_ARGS__)
#define test_bin(...)    DEFINE_TEST_STRBIN(bin, __VA_ARGS__)

static int
test_uints(void)
{
	plan(135);
	header();

	test_uint(0U, "\x00", 1);
	test_uint(1U, "\x01", 1);
	test_uint(0x7eU, "\x7e", 1);
	test_uint(0x7fU, "\x7f", 1);

	test_uint(0x80U, "\xcc\x80", 2);
	test_uint(0xfeU, "\xcc\xfe", 2);
	test_uint(0xffU, "\xcc\xff", 2);

	test_uint(0xfffeU, "\xcd\xff\xfe", 3);
	test_uint(0xffffU, "\xcd\xff\xff", 3);

	test_uint(0x10000U, "\xce\x00\x01\x00\x00", 5);
	test_uint(0xfffffffeU, "\xce\xff\xff\xff\xfe", 5);
	test_uint(0xffffffffU, "\xce\xff\xff\xff\xff", 5);

	test_uint(0x100000000ULL,
	     "\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9);
	test_uint(0xfffffffffffffffeULL,
	     "\xcf\xff\xff\xff\xff\xff\xff\xff\xfe", 9);
	test_uint(0xffffffffffffffffULL,
	     "\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 9);

	footer();
	return check_plan();
}

static int
test_ints(void)
{
	plan(153);
	header();

	test_int(-0x01, "\xff", 1);
	test_int(-0x1e, "\xe2", 1);
	test_int(-0x1f, "\xe1", 1);
	test_int(-0x20, "\xe0", 1);
	test_int(-0x21, "\xd0\xdf", 2);

	test_int(-0x7f, "\xd0\x81", 2);
	test_int(-0x80, "\xd0\x80", 2);

	test_int(-0x81, "\xd1\xff\x7f", 3);
	test_int(-0x7fff, "\xd1\x80\x01", 3);
	test_int(-0x8000, "\xd1\x80\x00", 3);

	test_int(-0x8001, "\xd2\xff\xff\x7f\xff", 5);
	test_int(-0x7fffffff, "\xd2\x80\x00\x00\x01", 5);
	test_int(-0x80000000LL, "\xd2\x80\x00\x00\x00", 5);

	test_int(-0x80000001LL,
	     "\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 9);
	test_int(-0x80000001LL,
	     "\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 9);
	test_int(-0x7fffffffffffffffLL,
	     "\xd3\x80\x00\x00\x00\x00\x00\x00\x01", 9);
	test_int((int64_t)-0x8000000000000000LL,
	     "\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 9);

	footer();
	return check_plan();
}

static int
test_bools(void)
{
	plan(18);
	header();

	test_bool(true, "\xc3", 1);
	test_bool(false, "\xc2", 1);

	footer();
	return check_plan();
}

static int
test_floats(void)
{
	plan(27);
	header();

	test_float((float) 1.0, "\xca\x3f\x80\x00\x00", 5);
	test_float((float) 3.141593, "\xca\x40\x49\x0f\xdc", 5);
	test_float((float) -1e38f, "\xca\xfe\x96\x76\x99", 5);

	footer();
	return check_plan();
}

static int
test_doubles(void)
{
	plan(27);
	header();

	test_double((double) 1.0,
		    "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00", 9);
	test_double((double) 3.141592653589793,
		    "\xcb\x40\x09\x21\xfb\x54\x44\x2d\x18", 9);
	test_double((double) -1e99,
		    "\xcb\xd4\x7d\x42\xae\xa2\x87\x9f\x2e", 9);

	footer();
	return check_plan();
}

static int
test_nils(void)
{
	plan(6);
	header();

	const char *d1 = mp_encode_nil(data);
	const char *d2 = data;
	const char *d3 = data;
	const char *d4 = data;
	note("nil");
	mp_decode_nil(&d2);
	mp_next(&d3);
	ok(!mp_check(&d4, d3 + 1), "mp_check_nil()");
	is((d1 - data), 1, "len(mp_encode_nil() == 1");
	is(d1, d2, "len(mp_decode_nil()) == 1");
	is(d1, d3, "len(mp_next_nil()) == 1");
	is(d1, d4, "len(mp_check_nil()) == 1");
	is(mp_sizeof_nil(), 1, "mp_sizeof_nil() == 1");

	footer();
	return check_plan();
}

static int
test_arrays(void)
{
	plan(54);
	header();

	test_array(0, "\x90", 1);
	test_array(1, "\x91", 1);
	test_array(15, "\x9f", 1);
	test_array(16, "\xdc\x00\x10", 3);
	test_array(0xfffe, "\xdc\xff\xfe", 3);
	test_array(0xffff, "\xdc\xff\xff", 3);
	test_array(0x10000, "\xdd\x00\x01\x00\x00", 5);
	test_array(0xfffffffeU, "\xdd\xff\xff\xff\xfe", 5);
	test_array(0xffffffffU, "\xdd\xff\xff\xff\xff", 5);

	footer();
	return check_plan();
}

static int
test_maps(void)
{
	plan(54);
	header();

	test_map(0, "\x80", 1);
	test_map(1, "\x81", 1);
	test_map(15, "\x8f", 1);
	test_map(16, "\xde\x00\x10", 3);
	test_map(0xfffe, "\xde\xff\xfe", 3);
	test_map(0xffff, "\xde\xff\xff", 3);
	test_map(0x10000, "\xdf\x00\x01\x00\x00", 5);
	test_map(0xfffffffeU, "\xdf\xff\xff\xff\xfe", 5);
	test_map(0xffffffffU, "\xdf\xff\xff\xff\xff", 5);

	footer();
	return check_plan();
}

static int
test_strls(void)
{
	plan(78);
	header();

	test_strl(0x00U, "\xa0", 1);
	test_strl(0x01U, "\xa1", 1);
	test_strl(0x1eU, "\xbe", 1);
	test_strl(0x1fU, "\xbf", 1);

	test_strl(0x20U, "\xd9\x20", 2);
	test_strl(0xfeU, "\xd9\xfe", 2);
	test_strl(0xffU, "\xd9\xff", 2);

	test_strl(0x0100U, "\xda\x01\x00", 3);
	test_strl(0xfffeU, "\xda\xff\xfe", 3);
	test_strl(0xffffU, "\xda\xff\xff", 3);

	test_strl(0x00010000U, "\xdb\x00\x01\x00\x00", 5);
	test_strl(0xfffffffeU, "\xdb\xff\xff\xff\xfe", 5);
	test_strl(0xffffffffU, "\xdb\xff\xff\xff\xff", 5);

	footer();
	return check_plan();
}

static int
test_binls(void)
{
	plan(78);
	header();

	test_binl(0x00U, "\xc4\x00", 2);
	test_binl(0x01U, "\xc4\x01", 2);
	test_binl(0x1eU, "\xc4\x1e", 2);
	test_binl(0x1fU, "\xc4\x1f", 2);

	test_binl(0x20U, "\xc4\x20", 2);
	test_binl(0xfeU, "\xc4\xfe", 2);
	test_binl(0xffU, "\xc4\xff", 2);

	test_binl(0x0100U, "\xc5\x01\x00", 3);
	test_binl(0xfffeU, "\xc5\xff\xfe", 3);
	test_binl(0xffffU, "\xc5\xff\xff", 3);

	test_binl(0x00010000U, "\xc6\x00\x01\x00\x00", 5);
	test_binl(0xfffffffeU, "\xc6\xff\xff\xff\xfe", 5);
	test_binl(0xffffffffU, "\xc6\xff\xff\xff\xff", 5);

	footer();
	return check_plan();
}

static int
test_strs(void)
{
	plan(96);
	header();

	test_str(0x01);
	test_str(0x1e);
	test_str(0x1f);
	test_str(0x20);
	test_str(0xfe);
	test_str(0xff);
	test_str(0x100);
	test_str(0x101);
	test_str(0xfffe);
	test_str(0xffff);
	test_str(0x10000);
	test_str(0x10001);

	footer();
	return check_plan();
}

static int
test_bins(void)
{
	plan(96);
	header();

	test_bin(0x01);
	test_bin(0x1e);
	test_bin(0x1f);
	test_bin(0x20);
	test_bin(0xfe);
	test_bin(0xff);
	test_bin(0x100);
	test_bin(0x101);
	test_bin(0xfffe);
	test_bin(0xffff);
	test_bin(0x10000);
	test_bin(0x10001);

	footer();
	return check_plan();
}

static void
test_next_on_array(uint32_t count)
{
	note("next/check on array(%u)", count);
	char *d1 = data;
	d1 = mp_encode_array(d1, count);
	for (uint32_t i = 0; i < count; i++) {
		d1 = mp_encode_uint(d1, i % 0x7f); /* one byte */
	}
	uint32_t len = count + mp_sizeof_array(count);
	const char *d2 = data;
	const char *d3 = data;
	ok(!mp_check(&d2, data + BUF_MAXLEN), "mp_check(array %u))", count);
	is((d1 - data), (ptrdiff_t)len, "len(array %u) == %u", count, len);
	is((d2 - data), (ptrdiff_t)len, "len(mp_check(array %u)) == %u", count, len);
	mp_next(&d3);
	is((d3 - data), (ptrdiff_t)len, "len(mp_next(array %u)) == %u", count, len);
}

static int
test_next_on_arrays(void)
{
	plan(52);
	header();

	test_next_on_array(0x00);
	test_next_on_array(0x01);
	test_next_on_array(0x0f);
	test_next_on_array(0x10);
	test_next_on_array(0x11);
	test_next_on_array(0xfe);
	test_next_on_array(0xff);
	test_next_on_array(0x100);
	test_next_on_array(0x101);
	test_next_on_array(0xfffe);
	test_next_on_array(0xffff);
	test_next_on_array(0x10000);
	test_next_on_array(0x10001);

	footer();
	return check_plan();
}

static void
test_next_on_map(uint32_t count)
{
	note("next/check on map(%u)", count);
	char *d1 = data;
	d1 = mp_encode_map(d1, count);
	for (uint32_t i = 0; i < 2 * count; i++) {
		d1 = mp_encode_uint(d1, i % 0x7f); /* one byte */
	}
	uint32_t len = 2 * count + mp_sizeof_map(count);
	const char *d2 = data;
	const char *d3 = data;
	ok(!mp_check(&d2, data + BUF_MAXLEN), "mp_check(map %u))", count);
	is((d1 - data), (ptrdiff_t)len, "len(map %u) == %u", count, len);
	is((d2 - data), (ptrdiff_t)len, "len(mp_check(map %u)) == %u", count, len);
	mp_next(&d3);
	is((d3 - data), (ptrdiff_t)len, "len(mp_next(map %u)) == %u", count, len);
}

static int
test_next_on_maps(void)
{
	plan(52);
	header();

	test_next_on_map(0x00);
	test_next_on_map(0x01);
	test_next_on_map(0x0f);
	test_next_on_map(0x10);
	test_next_on_map(0x11);
	test_next_on_map(0xfe);
	test_next_on_map(0xff);
	test_next_on_map(0x100);
	test_next_on_map(0x101);
	test_next_on_map(0xfffe);
	test_next_on_map(0xffff);
	test_next_on_map(0x10000);
	test_next_on_map(0x10001);

	footer();
	return check_plan();
}

static void
test_compare_uint(uint64_t a, uint64_t b)
{
	char bufa[9];
	char bufb[9];
	mp_encode_uint(bufa, a);
	mp_encode_uint(bufb, b);
	int r = mp_compare_uint(bufa, bufb);
	if (a < b) {
		ok(r < 0, "mp_compare_uint(%"PRIu64", %" PRIu64 ") < 0", a, b);
	} else if (a > b) {
		ok(r > 0, "mp_compare_uint(%"PRIu64", %" PRIu64") > 0", a, b);
	} else {
		ok(r == 0, "mp_compare_uint(%"PRIu64", %"PRIu64") == 0", a, b);
	}
}

static int
test_compare_uints(void)
{
	plan(227);
	header();

	test_compare_uint(0, 0);
	test_compare_uint(0, 0);

	uint64_t nums[] = {
		0, 1, 0x7eU, 0x7fU, 0x80U, 0xfeU, 0xffU, 0xfffeU, 0xffffU,
		0x10000U, 0xfffffffeU, 0xffffffffU, 0x100000000ULL,
		0xfffffffffffffffeULL, 0xffffffffffffffffULL
	};

	int count = sizeof(nums) / sizeof(*nums);
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < count; j++) {
			test_compare_uint(nums[i], nums[j]);
		}
	}

	footer();
	return check_plan();
}

static bool
fequal(float a, float b)
{
	return a > b ? a - b < 1e-5f : b - a < 1e-5f;
}

static bool
dequal(double a, double b)
{
	return a > b ? a - b < 1e-10 : b - a < 1e-10;
}

static int
test_format(void)
{
	plan(230);
	header();

	const size_t buf_size = 1024;
	char buf[buf_size];
	size_t sz;
	const char *fmt;
	const char *p, *c, *e;
	uint32_t len = 0;

	fmt = "%d %u %i  %ld %lu %li  %lld %llu %lli"
	      "%hd %hu %hi  %hhd %hhu %hhi";
	sz = mp_format(buf, buf_size, fmt, 1, 2, 3,
		       (long)4, (long)5, (long)6,
		       (long long)7, (long long)8, (long long)9,
		       (short)10, (short)11, (short)12,
		       (char)13, (char)14, (char)15);
	p = buf;
	for (unsigned i = 0; i < 15; i++) {
		ok(mp_typeof(*p) == MP_UINT, "Test type on step %d", i);
		ok(mp_decode_uint(&p) == i + 1, "Test value on step %d", i);
	}
	sz = mp_format(buf, buf_size, fmt, -1, -2, -3,
		       (long)-4, (long)-5, (long)-6,
		       (long long)-7, (long long)-8, (long long)-9,
		       (short)-10, (unsigned short)-11, (short)-12,
		       (signed char)-13, (unsigned char)-14, (signed char)-15);
	p = buf;
	for (int i = 0; i < 15; i++) {
		uint64_t expects[5] = { UINT_MAX - 1,
					ULONG_MAX - 4,
					ULLONG_MAX - 7,
					USHRT_MAX - 10,
					UCHAR_MAX - 13 };
		if (i % 3 == 1) {
			ok(mp_typeof(*p) == MP_UINT, "Test type on step %d", i);
			ok(mp_decode_uint(&p) == expects[i / 3],
			   "Test value on step %d", i);
		} else {
			ok(mp_typeof(*p) == MP_INT, "Test type on step %d", i);
			ok(mp_decode_int(&p) == - i - 1,
			   "Test value on step %d", i);
		}
	}

	fmt = "%d NIL [%d %b %b] this is test"
		"[%d %%%% [[ %d {%s %f %%  %.*s %lf %.*s NIL} %d ]] %d%d%d]";
#define TEST_PARAMS 0, 1, true, false, -1, 2, \
	"flt", 0.1, 6, "double#ignored", 0.2, 0, "ignore", 3, 4, 5, 6
	sz = mp_format(buf, buf_size, fmt, TEST_PARAMS);
	p = buf;
	e = buf + sz;

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 0, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_NIL, "type");
	mp_decode_nil(&p);

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 1, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_BOOL, "type");
	ok(mp_decode_bool(&p) == true, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_BOOL, "type");
	ok(mp_decode_bool(&p) == false, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 5, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_INT, "type");
	ok(mp_decode_int(&p) == -1, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 1, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 2, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_MAP, "type");
	ok(mp_decode_map(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 3, "decode");
	ok(memcmp(c, "flt", 3) == 0, "compare");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_FLOAT, "type");
	ok(fequal(mp_decode_float(&p), 0.1), "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 6, "decode");
	ok(memcmp(c, "double", 6) == 0, "compare");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_DOUBLE, "type");
	ok(dequal(mp_decode_double(&p), 0.2), "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 0, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_NIL, "type");
	mp_decode_nil(&p);

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 4, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 5, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 6, "decode");

	ok(p == e, "nothing more");

	ok(sz < 50, "no magic detected");

	for (size_t lim = 0; lim <= 50; lim++) {
		memset(buf, 0, buf_size);
		size_t test_sz = mp_format(buf, lim, fmt, TEST_PARAMS);
		ok(test_sz == sz, "return value on step %d", (int)lim);
		bool all_zero = true;
		for(size_t z = lim; z < buf_size; z++)
			all_zero = all_zero && (buf[z] == 0);
		ok(all_zero, "buffer overflow on step %d", (int)lim);

	}

#undef TEST_PARAMS

	footer();
	return check_plan();
}

int
test_mp_print()
{
	plan(1);
	header();

	char data[512];

	char *d = data;
	d = mp_encode_array(d, 6);
	d = mp_encode_int(d, -5);
	d = mp_encode_uint(d, 42);
	d = mp_encode_str(d, "kill bill", 9);
	d = mp_encode_map(d, 6);
	d = mp_encode_str(d, "bool true", 9);
	d = mp_encode_bool(d, true);
	d = mp_encode_str(d, "bool false", 10);
	d = mp_encode_bool(d, false);
	d = mp_encode_str(d, "null", 4);
	d = mp_encode_nil(d);
	d = mp_encode_str(d, "float", 5);
	d = mp_encode_float(d, 3.14);
	d = mp_encode_str(d, "double", 6);
	d = mp_encode_double(d, 3.14);
	d = mp_encode_uint(d, 100);
	d = mp_encode_uint(d, 500);
	*d++ = 0xd4; /* let's pack smallest fixed ext */
	*d++ = 0;
	*d++ = 0;
	char bin[] = "\x12test\x34\b\t\n\"bla\\-bla\"\f\r";
	d = mp_encode_bin(d, bin, sizeof(bin));

	const char *expected =
		"[-5, 42, \"kill bill\", "
		"{\"bool true\": true, \"bool false\": false, \"null\": null, "
		"\"float\": 3.14, \"double\": 3.14, 100: 500}, undefined, "
		"\"\\u0012test4\\b\\t\\n\\\"bla\\\\-bla\\\"\\f\\r\\u0000\"]";
	FILE *tmpf = tmpfile();
	if (tmpf != NULL) {
		mp_fprint(tmpf, data);
		(void) rewind(tmpf);
		memset(data, 0, sizeof(data));
		if (fgets(data, sizeof(data), tmpf) != NULL) {
			ok(strcmp(data, expected) == 0, "identical");
		}
		fclose(tmpf);
	}
	footer();
	return check_plan();
}

int main()
{
	plan(17);

	test_uints();
	test_ints();
	test_bools();
	test_floats();
	test_doubles();
	test_nils();
	test_strls();
	test_binls();
	test_strs();
	test_bins();
	test_arrays();
	test_maps();
	test_next_on_arrays();
	test_next_on_maps();
	test_compare_uints();
	test_format();
	test_mp_print();

	return check_plan();
}
