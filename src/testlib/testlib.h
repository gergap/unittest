#ifndef _TESTLIB_H_
#define _TESTLIB_H_

#include <test_config.h>

#include <stdbool.h>
#include <stdlib.h> /* for size_t */
#include <stdint.h>

/* This file uses the 'do {...} while (0)' trick to create a regular statement
 * instead of a compound statement. This way can use this macro in code like
 *
 * if (<condition>)
 *   CALL_FUNCS(a);
 * else
 *   bar;
 *
 * Macros simply using '{ ... }' will not work this way, because the last ';' in
 * the compound statement terminates the IF. This will lead to a compiler error.
 *
 * Howver, using MS compilers on Warning Level 4, 'while (0)' creates a warninig
 * 'warning C4127: conditional expression is constant'. Therefore we create a
 * new macro ONCE which does disable the warning just for this case.
 */
#ifdef _MSC_VER
#  define ONCE __pragma(warning(push)) \
               __pragma(warning(disable:4127)) \
               while (0) \
               __pragma(warning(pop))
#else
#  define ONCE while (0)
#endif

/** \file */

/** Implements a main() function which executes the tests.
 * You still have to provide a function register_tests(). This called from the
 * provided main function to all registering test functions.
 */
#define UTEST_MAIN() int main(int argc, char *argv[]) { \
        return testlib_main(argc, argv); \
    }

/** Checks the given \c condition and terminates the application if false.
 * Only use this for fatal errors where continuing test is not possible.
 */
#define UFATAL(condition) \
    do { \
        if (testlib_fatal(condition, #condition, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** The UVERIFY() macros checks whether the \c condition is true or not. If it is
 * true, execution continues. If not, a failure is recored in the test log and
 * the test won't be executed further. The test framework will continue to
 * execute the next test.
 *
 * Example:
 * @code
 * UVERIFY(result != -1);
 * @endcode
 */
#define UVERIFY(condition) \
    do { \
        if (testlib_verify(condition, #condition, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** The UVERIFY2() macro behaves exactly like UVERIFY(), except that it outputs
 * a verbose message when condition is false. The message is a plain C string.
 *
 * Example:
 * @code
 * int result = sem_init(...);
 * UVERIFY(result != -1, "sem_init failed");
 * @endcode
 */
#define UVERIFY2(condition, info) \
    do { \
        if (testlib_verify2(condition, #condition, info, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** The UCOMPARE() macro compares an \c actual value to an \c expected value.
 * If \c actual and \c expected are identical, execution continues. If not, a
 * failure is recorded in the test log and the test won't be executed further.
 *
 * UCOMPARE tries to output the contents of the values if the comparison fails,
 * so it is visible from the test log why the comparison failed.
 *
 * It is important that the first argument is the actual value and the second
 * argument is the expected value. Otherwise the output might be confusing.
 *
 * Example:
 * \snippet main.c UCOMPARE example
 */
#define UCOMPARE(actual, expected) \
    do { \
        if (testlib_compare(actual, expected, #actual, #expected, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** UCOMPARE64 behaves exactly like #UCOMPARE but works with 64bit ints instead of int.
 */
#define UCOMPARE64(actual, expected) \
    do { \
        if (testlib_compare64(actual, expected, #actual, #expected, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** UCOMPAREF behaves exactly like #UCOMPARE but works with floats instead ints.
 *
 * UCOMPAREF compares the values using \c == operator. This often makes no
 * sense for computed values due to rounding errors in floating point numbers.
 * Therefore #UFUZZY_COMPAREF is provided.
 *
 * -# Use UCOMPAREF for exact bitwise comparison, e.g. when testing encoding and
 *  decoding routines of network protocols where you expect that the data is not
 *  modified.
 * -# Use #UFUZZY_COMPAREF for computed values in algorithms.
 *
 * Example:
 * \snippet main.c UCOMPAREF example
 */
#define UCOMPAREF(actual, expected) \
    do { \
        if (testlib_comparef(actual, expected, #actual, #expected, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** UFUZZY_COMPAREF Compares the floating point value \c actual and \c expected
 * and returns true if they are considered equal, otherwise false.
 *
 * Note that comparing values where either \c actual or \c expected is 0.0 will
 * not work. The solution to this is to compare against values greater than or
 * equal to 1.0.
 *
 * @code
 * // Instead of comparing with 0.0
 * UFUZZY_COMPAREF(tmp, result); // This will return false
 * // Compare adding 1 to both values will fix the problem
 * UFUZZY_COMPAREF(1 + tmp, 1 + result); // This will return true
 * @endcode
 *
 * The two numbers are compared in a relative way, where the exactness is
 * stronger the smaller the numbers are.
 *
 * Example:
 * \snippet main.c UFUZZY_COMPAREF example
 */
#define UFUZZY_COMPAREF(actual, expected) \
    do { \
        if (testlib_fuzzy_comparef(actual, expected, #actual, #expected, \
            __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** UCOMPARESTR behaves exactly like UCOMPARE but works with C strings instead
 * of ints.
 *
 * Example:
 * \snippet main.c UCOMPARESTR example
 */
#define UCOMPARESTR(actual, expected) \
    do { \
        if (testlib_comparestr(actual, expected, #actual, #expected, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** UCOMPAREMEM behaves exactly like UCOMPARESTR but takes the lengths instead
 * of relying on zero terminated strings.
 *
 * Example:
 * \snippet main.c UCOMPAREMEM example
 */
#define UCOMPAREMEM(actual, actuallen, expected, expectedlen) \
    do { \
        if (testlib_comparemem(actual, actuallen, expected, expectedlen, __FILE__, __LINE__)) { \
            return; \
        } \
    } ONCE
/** Gives the test a human readable name. */
#define UREGISTER_NAME(name) \
    do { \
        testlib_register_name(name); \
    } ONCE
/** Registers a global test init function.
 * This is called before the first test starts.
 *
 * You can use this to allocate resources that are required for all tests.
 *
 * Example:
 * \snippet main.c register init
 *
 * @see UREGISTER_CLEANUP()
 */
#define UREGISTER_INIT(func) \
    do { \
        testlib_register_init(func); \
    } ONCE
/** Registers a global test cleanup functions.
 * This is called after the last test as finished.
 *
 * You can free the resources here that where allocated in your test_init
 * function.
 *
 * Example:
 * \snippet main.c register cleanup
 *
 * @see UREGISTER_INIT()
 */
#define UREGISTER_CLEANUP(func) \
    do { \
        testlib_register_cleanup(func); \
    } ONCE
/** Registers a test function.
 * All registered functions will be executed by testlib_run_tests().
 *
 * Example:
 * \snippet main.c register test
 */
#define UREGISTER_TEST(test) \
    do { \
        testlib_register_test(test, #test, 0, 0); \
    } ONCE
/** Registers a data-driven test function.
 * All registered functions will be executed by testlib_run_tests().
 *
 * Example:
 * \snippet main.c register datadriven test
 */
#define UREGISTER_DATADRIVEN_TEST(test, testdata) \
    do { \
        testlib_register_datadriven_test(test, #test, testdata, #testdata, 0, 0); \
    } ONCE
/** Registers a test function.
 * This macro behaves exactly like UREGISTER_TEST(), but additionally allows to
 * specify \c init and \c cleanup functions which are called before and after
 * the test function respectively.
 *
 * Example:
 * \snippet main.c register test
 */
#define UREGISTER_TEST2(test, init, cleanup) \
    do { \
        testlib_register_test(test, #test, init, cleanup); \
    } ONCE
/** Registers a data-driven test function.
 * This macro behaves exactly like UREGISTER_DATADRIVEN_TEST(), but additionally
 * allows to specify \c init and \c cleanup functions which are called before
 * and after the test function respectively.
 * Note that \c init is called one time before the test function is called the
 * 1st time and \c cleanup is called after the testfunction has been called the
 * last time. Thus \c init and \c cleanup are not called for every dataset.
 * Note also that \c init is called after \c testdata is called.
 *
 * Example:
 * \snippet main.c register datadriven test
 */
#define UREGISTER_DATADRIVEN_TEST2(test, testdata, init, cleanup) \
    do { \
        testlib_register_datadriven_test(test, #test, testdata, #testdata, init, cleanup); \
    } ONCE


/** The UEXPECT_FAIL() macro marks the next UCOMPARE() or UVERIFY() as an
 * expected failure. Instead of adding a failure to the test log, an expected
 * failure will be reported.
 *
 * The parameter \c dataIndex describes for which entry in the test data the
 * failure is expected. Pass an empty string ("") if the failure is expected for
 * all entries or if no test data exists.
 *
 * \c comment will be appended to the test log for the expected failure.
 *
 * \c mode is a #testlib_fail_mode and sets whether the test should continue to
 * execute or not.
 *
 * Rationale: If you have a test failing it is better to mark it as XFAIL then
 * disabling or skipping it. This way the test passes, but the error is still
 * reported so that it cannot be forgotten. This is typically used for
 * non-critical problems that cannot be easily fixed, and so have been deferred
 * to the be fixed in the next version.
 *
 * Example:
 * \snippet main.c UEXPECT_FAIL example
 */
#define UEXPECT_FAIL(dataIndex, comment, mode) \
    do { \
        testlib_expect_fail(dataIndex, comment, mode); \
    } ONCE

/** This enum describes the modes for handling an expected failure of the
 * UVERIFY() or UCOMPARE() macros.
 */
enum testlib_fail_mode {
    Abort = 0, /**< Aborts the execution of the test. Use this mode when it
                * doesn't make sense to execute the test any further after
                * the expected failure. */
    Continue   /**< Continues execution of the test after the expected failure. */
};

#ifdef ENABLE_BENCHMARK
/** Performs a benchmark test.
 *
 * The UBENCHMARK macro calls the test code \c n times to get reasonable
 * performance results. The macro determins the correct \c n automatically
 * by starting with \c n = 1, then it doubles \c n (2, 4, 8, 16, ...) until the
 * measured time is over a configurable threshold (50ms by default).
 *
 * It then outputs the result like this.
 * <pre>
 * RESULT : 0.000254 msecs per iteration (total: 66.518000, iterations: 262144)
 * </pre>
 *
 * Example:
 * @code
 * void test_foo()
 * {
 *     struct list l;
 *     struct list_el *e;
 *
 *     UVERIFY(list_load_from_file(&l, "/tmp/foo.txt"));
 *
 *     // perform normal unittest
 *     e = list_find(&l, "key");
 *     UVERIFY(e != NULL);
 *     UCOMPARESTR(list_name(e), "last element");
 *
 *     // now lets measure the find performance
 *     UBENCHMARK {
 *         list_find(&l, "key");
 *     }
 *
 *    list_clear(&l);
 * }
 * @endcode
 */
# define UBENCHMARK for (test_benchmark_start(); test_benchmark_done(); test_benchmark_next())
void test_benchmark_start();
bool test_benchmark_done();
void test_benchmark_next();
#else
# define UBENCHMARK while (0)
#endif

int testlib_fatal(int condition, const char *scondition, const char *file, int line);
int testlib_verify(int condition, const char *scondition, const char *file, int line);
int testlib_verify2(int condition, const char *scondition, const char *info, const char *file, int line);
int testlib_compare(int actual, int expected, const char *sactual, const char *sexpected, const char *file, int line);
int testlib_compare64(int64_t actual, int64_t expected, const char *sactual, const char *sexpected, const char *file, int line);
int testlib_comparef(double actual, double expected, const char *sactual, const char *sexpected, const char *file, int line);
int testlib_fuzzy_comparef(double actual, double expected, const char *sactual, const char *sexpected, const char *file, int line);
int testlib_comparestr(const char *actual, const char *expected, const char *sactual, const char *sexpected, const char *file, int line);
int testlib_comparemem(const unsigned char *actual, size_t actuallen, const unsigned char *expected, size_t expectedlen, const char *file, int line);
void testlib_expect_fail(const char *dataIndex, const char *comment, enum testlib_fail_mode mode);

void testlib_add_column(const char *name, const char *fmt);
void testlib_add_row(const char *name, ...);
void *testlib_fetch(const char *name);
int testlib_fetch_int(const char *name);
unsigned int testlib_fetch_uint(const char *name);
double testlib_fetch_double(const char *name);

typedef void (*testfunction)();

void testlib_register_name(const char *name);
void testlib_register_init(testfunction func);
void testlib_register_cleanup(testfunction func);
void testlib_register_test(testfunction test, const char *stest, testfunction init, testfunction cleanup);
void testlib_register_datadriven_test(testfunction test, const char *stest, testfunction testdata, const char *stestdata, testfunction init, testfunction cleanup);
void testlib_run_tests(const char *testname, const char *testset);
void testlib_list_tests();
int testlib_verbose();
int testlib_silent();
void testlib_info(const char *msg);

struct testlib_stat {
    int num_tests;
    int num_passed;
    int num_failed;
    int num_skipped;
};

const struct testlib_stat *testlib_result();
int testlib_main(int argc, char *argv[]);

#define UINFO(msg) testlib_info(msg)

#endif /* _TESTLIB_H_ */

