#include <testlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> /* toupper */

/** Some dummy code which should be tested. */
void string_toupper(char *dest, int len, const char *src)
{
    int index = 0;

    while (index < len - 1 && src[index] != 0) {
        dest[index] = toupper(src[index]);
        index++;
    }

    dest[index] = 0;
}

/** [datadriven test example] */
/** test data for test_toupper */
void test_toupper_data()
{
    testlib_add_column("string", "%s");
    testlib_add_column("result", "%s");

    testlib_add_row("all lower", "hello", "HELLO");
    testlib_add_row("mixed",     "Hello", "HELLO");
    testlib_add_row("all upper", "HELLO", "HELLO");
    testlib_add_row("umlauts",   "öäü",   "ÖÄÜ");
}

/** test case for testing string_toupper */
void test_toupper()
{
    char *string = testlib_fetch("string");
    char *result = testlib_fetch("result");
    char  tmp[50];

    /** [UCOMPARESTR example] */
    string_toupper(tmp, sizeof(tmp), string);
    UEXPECT_FAIL("umlauts", "We can't handle umlauts yet. Will be fixed in the next release", Continue);
    UCOMPARESTR(tmp, result);
    /** [UCOMPARESTR example] */
}
/** [datadriven test example] */

/** test case for testing memcpy */
void test_memcpy()
{
    unsigned char src[] = {0x01, 0x02, 0x03, 0x04};
    unsigned char dst[] = {0x00, 0x00, 0x00, 0x00};

    /** [UCOMPAREMEM example] */
    memcpy(dst, src, sizeof(dst));
    UCOMPAREMEM(dst, sizeof(dst), src, sizeof(src));
    /** [UCOMPAREMEM example] */
}

/** another simple test case. */
/** [a simple test case] */
void test_fopen()
{
    FILE *f = fopen("/etc/passwd", "r");
    UVERIFY(f != NULL);
    fclose(f);
}
/** [a simple test case] */

void test_foo_init()
{
    printf("Prepare test_foo\n");
}

void test_foo_cleanup()
{
    printf("Cleanup test_foo\n");
}

void test_foo()
{
    FILE *f = fopen("/etc/shadow", "r");
    /** [UEXPECT_FAIL example] */
    UEXPECT_FAIL("", "Will fix in the next release", Abort);
    UVERIFY(f != NULL);
    /** [UEXPECT_FAIL example] */
    fclose(f);
}

/** test data for another test case. */
void test_multiplication_data()
{
    testlib_add_column("a", "%i");
    testlib_add_column("b", "%i");
    testlib_add_column("result", "%i");

    testlib_add_row("all zero", 0, 0, 0);
    testlib_add_row("1st zero", 1, 0, 0);
    testlib_add_row("2nd zero", 0, 1, 0);
    testlib_add_row("1st neutral",  1, 5, 5);
    testlib_add_row("2nd neutral",  5, 1, 5);
    testlib_add_row("15",       5, 3, 15);
}

/** another data-driven test */
void test_multiplication()
{
    int a = testlib_fetch_int("a");
    int b = testlib_fetch_int("b");
    int result = testlib_fetch_int("result");
    int tmp;

    /** [UCOMPARE example] */
    tmp = a * b;
    UCOMPARE(tmp, result);
    /** [UCOMPARE example] */
}

void test_multiplicationf_init()
{
    printf("Prepare test_multiplicationf\n");
}

void test_multiplicationf_cleanup()
{
    printf("Cleanup test_multiplicationf\n");
}

/** test data for another test case. */
void test_multiplicationf_data()
{
    testlib_add_column("a", "%f");
    testlib_add_column("b", "%f");
    testlib_add_column("result", "%f");

    testlib_add_row("all zero", 0.0, 0.0, 0.0);
    testlib_add_row("1st zero", 1.0, 0.0, 0.0);
    testlib_add_row("2nd zero", 0.0, 1.0, 0.0);
    testlib_add_row("1st neutral",  1.0, 5.0, 5.0);
    testlib_add_row("2nd neutral",  5.0, 1.0, 5.0);
    testlib_add_row("15",       5.0, 3.0, 15.0);
}

/** [UFUZZY_COMPAREF example] */
void test_multiplicationf()
{
    double a = testlib_fetch_double("a");
    double b = testlib_fetch_double("b");
    double result = testlib_fetch_double("result");
    double tmp;

    tmp = a * b;
    UFUZZY_COMPAREF(1 + tmp, 1 + result);
}
/** [UFUZZY_COMPAREF example] */

/* dummy encoder routine to demostrate test_encoder. */
int encode_double(char *buf, size_t size, double val)
{
    if (size < sizeof(double)) return -1;
    *((double*)buf) = val;
    return 0;
}

/* dummy decoder routine to demostrate test_encoder. */
int decode_double(char *buf, size_t size, double *val)
{
    if (size < sizeof(double)) return -1;
    *val = *((double*)buf);
    return 0;
}

void test_encoder_data()
{
    testlib_add_column("val", "%f");

    testlib_add_row("zero",      0.0);
    testlib_add_row("negative", -1.23);
    testlib_add_row("postive",   1.23);
}

/** [UCOMPAREF example] */
void test_encoder()
{
    double val = testlib_fetch_double("val");
    double tmp;
    char buf[20];
    int ret;

    ret = encode_double(buf, sizeof(buf), val);
    UVERIFY2(ret == 0, "encode_double failed");
    ret = decode_double(buf, sizeof(buf), &tmp);
    UVERIFY2(ret == 0, "decode_double failed");
    UCOMPAREF(tmp, val);
}
/** [UCOMPAREF example] */

void test_init()
{
    printf("test_init\n");
}

void test_cleanup()
{
    printf("test_cleanup\n");
}

void register_tests()
{
    /** [register init] */
    UREGISTER_INIT(test_init);
    /** [register init] */
    /** [register cleanup] */
    UREGISTER_CLEANUP(test_cleanup);
    /** [register cleanup] */
    /** [register datadriven test] */
    UREGISTER_DATADRIVEN_TEST(test_toupper, test_toupper_data);
    /** [register datadriven test] */
    /** [register test] */
    UREGISTER_TEST(test_fopen);
    UREGISTER_TEST(test_memcpy);
    /** [register test] */
    UREGISTER_TEST2(test_foo, test_foo_init, test_foo_cleanup);
    UREGISTER_DATADRIVEN_TEST(test_multiplication, test_multiplication_data);
    UREGISTER_DATADRIVEN_TEST2(test_multiplicationf, test_multiplicationf_data,
                               test_multiplicationf_init, test_multiplicationf_cleanup);
    UREGISTER_DATADRIVEN_TEST(test_encoder, test_encoder_data);
}

UTEST_MAIN()

