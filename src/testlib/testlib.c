#include "testlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_GETOPT
#include <getopt.h>
#endif
#include <inttypes.h>
#ifdef __linux__
# include <unistd.h> /* for isatty */
#endif
#ifdef _MSC_VER
# define inline __inline
#endif
#ifdef ENABLE_BENCHMARK
# include "timer.h"
#endif
#if defined (_MSC_VER) && !defined (_WIN32_WCE)
# include <crtdbg.h>
#endif

#ifdef ENABLE_COLOR
# include "linuxtermcolors.h"
char PASS[]    = BOLDGREEN"PASS"RESET;
char FAIL[]    = BOLDRED"FAIL!"RESET;
char XFAIL[]   = BOLDRED"XFAIL!"RESET;
char SKIPPED[] = BOLDYELLOW"SKIPPED"RESET;
char RESULT[]  = BOLDBLUE"RESULT"RESET;
static void testlib_disable_color()
{
    strncpy(PASS,    "PASS",    sizeof(PASS));
    strncpy(FAIL,    "FAIL!",   sizeof(FAIL));
    strncpy(XFAIL,   "XFAIL!",  sizeof(XFAIL));
    strncpy(SKIPPED, "SKIPPED", sizeof(SKIPPED));
    strncpy(RESULT,  "RESULT", sizeof(RESULT));
}
#else
const char PASS[]    = "PASS";
const char FAIL[]    = "FAIL!";
const char XFAIL[]   = "XFAIL!";
const char SKIPPED[] = "SKIPPED";
const char RESULT[]  = "RESULT";
#endif

static inline double uAbs(double f)
{
    return f >= 0 ? f : -f;
}
static inline double uMin(double a, double b)
{
    return (a < b) ? a : b;
}
static inline double uMax(double a, double b)
{
    return (a < b) ? b : a;
}
static inline float uAbsf(float f)
{
    return f >= 0 ? f : -f;
}
static inline float uMinf(float a, float b)
{
    return (a < b) ? a : b;
}
static inline float uMaxf(float a, float b)
{
    return (a < b) ? b : a;
}
static inline bool uFuzzyCompare(double p1, double p2)
{
    return (uAbs(p1 - p2) <= 0.000000000001 * uMin(uAbs(p1), uAbs(p2)));
}
static inline bool uFuzzyComparef(float p1, float p2)
{
    return (uAbsf(p1 - p2) <= 0.00001f * uMinf(uAbsf(p1), uAbsf(p2)));
}
static inline bool uFuzzyIsNull(double d)
{
    return uAbs(d) <= 0.000000000001;
}
static inline bool uFuzzyIsNullf(float f)
{
    return uAbs(f) <= 0.00001f;
}

/** @file
 *  \todo add test suites
 *  \todo add support for test skipping
 */

/** \internal Maximum number of columns in testdataentry */
#define MAX_COLUMNS 10

/** \internal Union for storing different test data values. */
union testdatavalue {
    int i;
    unsigned int u;
    double f;
    void *ptr;
    char *str;
};

/** \internal one test data row with n columns */
struct testdataentry {
    const char *name; /**< dataset name */
    union testdatavalue columns[MAX_COLUMNS]; /**< column data */
};

/** \internal structure for storing all test data for the one
 * data-driven test case
 */
struct testdata {
    int num_columns;
    int num_rows;
    int current_row;
    const char *columns[MAX_COLUMNS];
    const char *fmt[MAX_COLUMNS];
    struct testdataentry *rows;
};

/** \internal list entry for singly lineked list.
 * This entry contains all data for one test case
 */
struct list_element {
    testfunction fctTestInit;  /**< function pointer to test init (optional) */
    testfunction fctTestCleanup; /**< function pointer to test cleanup (opt) */
    testfunction fctTest;      /**< function pointer to test case */
    testfunction fctTestData;  /**< function pointer to test case data setup */
    const char *testname;      /**< name of test function */
    const char *testdataname;  /**< name of test data function */
    struct list_element *next; /**< pointer to next list element */
};

/** Verbosity level for output function.
 * The verbosity level can be increased by calling testlib_verbose.
 *
 * By default if a test fails you get detailed information about the problem:
 * @code
 * FAIL! : test_toupper(umlauts) Compared values are not the same.
 *    Actual   (tmp): öäü
 *    Expected (result): ÖÄÜ
 *    Loc: [/home/gergap/work/unittest/src/test/main.c(41)]
 * @endcode
 *
 * If the test passes you only see the test result.
 * @code
 * PASS   : test_toupper
 * @endcode
 *
 * By increasing the verbosity leve you can get more information also for passed
 * tests. You can increase the verbosity level by calling testlib_verbose().
 */
enum VerbosityLevel {
    LEVEL_RESULT = 0, /**< report results */
    LEVEL_GOODRESULT, /**< report also good results if tests that have PASSED */
    LEVEL_INFO,       /**< report additional info (e.g. actual/expected values
                       * for XFAIL) */
    LEVEL_LOC,        /**< report locality information also for PASSED tests */
    LEVEL_FUNC,       /**< report test function entry and leave */
    LEVEL_DEBUG       /**< report additional debug info */
};

static char g_testname[30] = "<unnamed>";
static testfunction g_init = 0;
static testfunction g_cleanup = 0;
static struct list_element *g_first_test = 0;
static struct list_element *g_last_test = 0;
static struct testdata g_data;
static int g_verbose = LEVEL_GOODRESULT;
static const char *g_current_test_name = 0;
static struct testlib_stat g_results;
static int g_testsuccess = 0; /**< flag used to indicate if the current test succeeded. */
/* values to handle expected failures */
static int g_expectfail  = 0;
static const char *g_dataIndex;
static const char *g_comment;
static enum testlib_fail_mode g_mode;
static const char *g_dataset_override;

#ifdef ENABLE_BENCHMARK
struct benchmark {
    unsigned int i;              /**< iteration counter */
    unsigned int n;              /**< number of iterations */
    unsigned int threshold;      /**< timer threshhold that must be reached */
    struct timer t;              /**< measurement timer */
};
static struct benchmark g_benchmark =  { 0, 0, 50000, TIMER_STATIC_INITIALIZER };
#endif /* ENABLE_BENCHMARK */

/** Increase verbosity level to get more output. */
int testlib_verbose()
{
    return ++g_verbose;
}

/** Decrease verbosity level to get less output. */
int testlib_silent()
{
    return --g_verbose;
}

/** \internal use to output results and diagnostic info. */
static void output(int level, const char *fmt, ...)
{
    va_list ap;

    if (level > g_verbose) return;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void testlib_info(const char *msg)
{
    output(LEVEL_INFO, msg);
}

#ifdef ENABLE_HEXDUMP
/** \internal helper function to simplify snprintf handling. */
static int append(char **dst, size_t *n, const char *fmt, ...)
{
    va_list args;
    int ret;

    if ((*n) == 0) return 0;

    va_start(args, fmt);
    ret = vsnprintf(*dst, *n, fmt, args);
    if (ret > 0) {
        (*dst) += ret;
        (*n) -= ret;
    }
    va_end(args);

    return ret;
}

/** \internal hexdump like output of data */
static void dump_line(char *dst, size_t n, const uint8_t *text, size_t count)
{
    static char HEX[] = "0123456789ABCDEF";
    size_t i;

    /* dump hex */
    for (i = 0; i < count; i++)
    {
        unsigned char byte = text[i];
        append(&dst, &n, "%c%c ", HEX[byte >> 4], HEX[byte & 0xf]);
        if (i == 7)
        {
            append(&dst, &n, " "); /* gap at 8 byte*/
        }
    }
    /* fill with spaces */
    for (; i < 16; i++)
    {
        append(&dst, &n, "   ");
        if (i == 7)
        {
            append(&dst, &n, " "); /* gap at 8 byte*/
        }
    }
    append(&dst, &n, " |");
    /* dump ASCII */
    for (i = 0; i < count; i++)
    {
        unsigned char byte = text[i];
        if (byte >= 32 && byte < 128)
        {
            append(&dst, &n, "%c", byte);
        }
        else
        {
            append(&dst, &n, ".");
        }
        if (i == 7)
        {
            append(&dst, &n, " ");      /* gap at 8 byte*/
        }
    }
    append(&dst, &n, "|\n");
}

static void hexdump(char *dst, size_t n, const uint8_t *text, size_t count)
{
    size_t num;

    while (count > 0)
    {
        num = (count > 16) ? 16 : count;
        dump_line(dst, n, text, num);
        text += num;
        count -= num;
    }
}
#endif /* ENABLE_HEXDUMP */

static void testdata_init(struct testdata *data)
{
    memset(data, 0, sizeof(*data));
}

static void testdata_clear(struct testdata *data)
{
    if (data->rows) free(data->rows);
    memset(data, 0, sizeof(*data));
}

/** Adds a new data column for data-driven tests.
* Call this function only in a test-data preperation function.
*
* @param name Name of the column. Used also in testlib_fetch().
* @param fmt Printf like format specifier.
*
* The following format specifiers are currently supported:
*
* Fmt        | Description
* -----------|------------
*  \%s       | C String
*  \%p       | Pointer type
*  \%i / \%d | int
*  \%u       | unsigned int
*  \%f       | double
*
* Note: In C \c float values passed via ... to a variadic function are promoted
* to \c double. \c char and \c short are promoted to \c int. That's why there
* are only #testlib_fetch_int and #testlib_fetch_double functions and no
* functions for \c float, \c short and \c char.
*/
void testlib_add_column(const char *name, const char *fmt)
{
    UFATAL(g_data.num_columns < MAX_COLUMNS);
    g_data.columns[g_data.num_columns] = name;
    g_data.fmt[g_data.num_columns] = fmt;
    g_data.num_columns++;
}

/** Adds a new data row for data-driven tests.
* Call this function only in a test-data preperation function.
*
* @param name Name of the test set. This is used in the output to see with
* what dataset the test fails.
* @param ... The subsequent arguments must match the number of specified columns
* and type. @see testlib_add_column
*/
void testlib_add_row(const char *name, ...)
{
    struct testdataentry *tmp;
    int row = g_data.num_rows;
    int col;
    va_list ap;

    g_data.num_rows++;
    tmp = realloc(g_data.rows, sizeof(*tmp) * g_data.num_rows);
    UFATAL(tmp != 0);

    g_data.rows = tmp;
    tmp[row].name = name;
    va_start(ap, name);
    for (col = 0; col < g_data.num_columns; col++) {
        switch (g_data.fmt[col][1]) {
        case 'd':
        case 'i':
        case 'c': /* ‘char’ is promoted to ‘int’ when passed through ‘... */
            tmp[row].columns[col].i = va_arg(ap, int);
            break;
        case 'u':
            tmp[row].columns[col].u = va_arg(ap, unsigned int);
            break;
        case 's':
            tmp[row].columns[col].str = va_arg(ap, char *);
            break;
        case 'p':
            tmp[row].columns[col].ptr = va_arg(ap, void *);
            break;
        case 'f': /* ‘float’ is promoted to ‘double’ when passed through ‘...’ */
            tmp[row].columns[col].f = va_arg(ap, double);
            break;
        default:
            UFATAL(0 && "invalid format specifier");
            break;
        }
    }
    va_end(ap);
}

static int testlib_find_col(const char *name)
{
    int col = 0;

    while (col < g_data.num_columns) {
        if (strcmp(g_data.columns[col], name) == 0) {
            return col;
        }
        col++;
    }

    return -1;
}

/** \internal Returns the name of the current data set in data-driven tests.
 * Returns an empty string if the current test is no data-driven test.
 * While g_init or g_cleanup is being called, this function returns an
 * appropriate string if set in g_dataset_override.
 */
static const char *testlib_current_dataset()
{
    if (g_data.num_rows == 0) {
        if (g_dataset_override != 0) {
            return g_dataset_override;
        } else {
            return "";
        }
    }
    return g_data.rows[g_data.current_row].name;
}

/** Returns the data for the given column \c name of the current test dataset.
 * Only call this in data-driven test functions.
 *
 * The test function is called once for each dataset.
 *
 * @param name the column name registered with testlib_add_column()
 * @return Returns the stored pointer or NULL if not found. This function can be
 * used for \%s and \%p format specifiers.
 */
void *testlib_fetch(const char *name)
{
    int row = g_data.current_row;
    int col = testlib_find_col(name);

    if (col != -1) {
        return g_data.rows[row].columns[col].ptr;
    }

    return 0;
}

/** This function behaves exactly like testlib_fetch() but returns an int value
 * previously stored in a \%i or \%d column.
 *
 * @param name the column name registered with testlib_add_column()
 * @return the stored value or 0 if not found.
 */
int testlib_fetch_int(const char *name)
{
    int row = g_data.current_row;
    int col = testlib_find_col(name);

    if (col != -1) {
        return g_data.rows[row].columns[col].i;
    }

    return 0;
}

/** This function behaves exactly like testlib_fetch() but returns an unsigned int value
 * previously stored in a \%u column.
 *
 * @param name the column name registered with testlib_add_column()
 * @return the stored value or 0 if not found.
 */
unsigned int testlib_fetch_uint(const char *name)
{
    int row = g_data.current_row;
    int col = testlib_find_col(name);

    if (col != -1) {
        return g_data.rows[row].columns[col].u;
    }

    return 0;
}

/** This function behaves exactly like testlib_fetch() but returns a double value
 * previously stored in a \%f column.
 *
 * @param name the column name registered with testlib_add_column()
 * @return the stored value or 0.0 if not found.
 */
double testlib_fetch_double(const char *name)
{
    int row = g_data.current_row;
    int col = testlib_find_col(name);

    if (col != -1) {
        return g_data.rows[row].columns[col].f;
    }

    return 0;
}

/** \internal Returns 1 if the current test is expected to fail, else 0 is returned. */
static int testlib_is_expect_fail()
{
    /* if g_expectfail is not set, return zero */
    if (g_expectfail == 0) return 0;
    /* if g_dataIndex == "" always return 1 */
    if (g_dataIndex[0] == 0) return 1;
    /* return 1 if the current dataset matches the expected name */
    if (strcmp(g_dataIndex, testlib_current_dataset()) == 0) return 1;
    return 0;
}

/** \internal Resets expect_fail flag gloabaly. This is called after a test has
 * completeley finished.
 */
static void testlib_reset_expect_fail_forced()
{
    g_expectfail = 0;
    g_dataIndex = 0;
    g_comment = 0;
    g_mode = Abort;
}

/** \internal Resets the current expect_fail flag. This is called in UVERIFY() and
 * UCOMAPRE() macros.
 * If dataIndex == "", which means we expect this to fail for all datasets
 * this function does not reset the flag, otherwise it does.
 */
static void testlib_reset_expect_fail()
{
    if (g_dataIndex[0] == 0) return;
    g_expectfail = 0;
    g_dataIndex = 0;
    g_comment = 0;
    g_mode = Abort;
}

void testlib_expect_fail(
    const char *dataIndex,
    const char *comment,
    enum testlib_fail_mode mode)
{
    g_expectfail = 1;
    g_dataIndex  = (dataIndex == 0) ? "" : dataIndex;
    g_comment    = comment;
    g_mode       = mode;
}

/**
* \internal This function checks if the \c condition is met.
*
* This function is called by UFATAL macro. Always use this macro instead of
* calling this function directly. This check is similar to UVERIFY, but UFATAL
* terminates the application immediatley. This is used for fatal errors during
* test initialization like out-of-memory, or configuration errors where it makes
* no sense to continue the test.
*
* @param condition Condition to check
* @param scondition Textual representation of the condition
* @param file filename which is printed to stderr if the conditation is false
* @param line line number which is printed when the condition is false
*
* @return Zero if condition is met, otherwise the program terminates.
*/
int testlib_fatal(
    int condition, const char *scondition,
    const char *file, int line)
{
    if (!condition) {
        fprintf(stderr,
                "fatal error in %s:%i: %s. Fix your test code. Test terminates now.\n",
                file, line, scondition);
        exit(EXIT_FAILURE);
    }
    return 0;
}

/**
* \internal This function checks if the \c condition is met.
*
* This function is called by UVERIFY macro. Always use this macro instead of
* calling this function directly. If \c condition is false the current test case
* is marked as failed and the test function returns.
*
* @param condition Condition to check
* @param scondition Textual representation of the condition
* @param file filename which is printed to stderr if the conditation is false
* @param line line number which is printed when the condition is false
*
* @return Zero if condition is met, otherwise the program terminates.
*/
int testlib_verify(
    int condition, const char *scondition,
    const char *file, int line)
{
    if (condition) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_LOC, "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) '%s' returned FALSE.\n", XFAIL,
                   g_current_test_name, testlib_current_dataset(), scondition);
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) '%s' returned FALSE.\n", FAIL,
                   g_current_test_name, testlib_current_dataset(), scondition);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
        }
        return 1;
    }
    return 0;
}

int testlib_verify2(
    int condition, const char *scondition, const char *info,
    const char *file, int line)
{
    if (condition) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_LOC, "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) '%s' returned FALSE. (%s)\n",
                   XFAIL, g_current_test_name, testlib_current_dataset(),
                   scondition, info);
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) '%s' returned FALSE. (%s)\n",
                   FAIL, g_current_test_name, testlib_current_dataset(),
                   scondition, info);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
        }
        return 1;
    }
    return 0;
}

int testlib_compare(
    int actual, int expected, const char *sactual, const char *sexpected,
    const char *file, int line)
{
    if (actual == expected) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_INFO,   "   Actual   (%s): %i\n", sactual, actual);
        output(LEVEL_INFO,   "   Expected (%s): %i\n", sexpected, expected);
        output(LEVEL_LOC,    "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) Compared values are not the same.\n",
                   XFAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_INFO,   "   Actual   (%s): %i\n", sactual, actual);
            output(LEVEL_INFO,   "   Expected (%s): %i\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) Compared values are not the same.\n",
                   FAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   Actual   (%s): %i\n", sactual, actual);
            output(LEVEL_RESULT, "   Expected (%s): %i\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
        }
        return 1;
    }
    return 0;
}

int testlib_compare64(
    int64_t actual, int64_t expected, const char *sactual, const char *sexpected,
    const char *file, int line)
{
    if (actual == expected) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_INFO,   "   Actual   (%s): %"PRIi64"\n", sactual, actual);
        output(LEVEL_INFO,   "   Expected (%s): %"PRIi64"\n", sexpected, expected);
        output(LEVEL_LOC,    "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) Compared values are not the same.\n",
                   XFAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_INFO,   "   Actual   (%s): %"PRIi64"\n", sactual, actual);
            output(LEVEL_INFO,   "   Expected (%s): %"PRIi64"\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) Compared values are not the same.\n",
                   FAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   Actual   (%s): %"PRIi64"\n", sactual, actual);
            output(LEVEL_RESULT, "   Expected (%s): %"PRIi64"\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
        }
        return 1;
    }
    return 0;
}

int testlib_comparef(
    double actual, double expected, const char *sactual, const char *sexpected,
    const char *file, int line)
{
    if (actual == expected) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_INFO,   "   Actual   (%s): %f\n", sactual, actual);
        output(LEVEL_INFO,   "   Expected (%s): %f\n", sexpected, expected);
        output(LEVEL_LOC,    "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) Compared values are not the same.\n",
                   XFAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_INFO,   "   Actual   (%s): %f\n", sactual, actual);
            output(LEVEL_INFO  , "   Expected (%s): %f\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) Compared values are not the same.\n",
                   FAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   Actual   (%s): %f\n", sactual, actual);
            output(LEVEL_RESULT, "   Expected (%s): %f\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
            return 1;
        }
    }
    return 0;
}

int testlib_fuzzy_comparef(
    double actual, double expected, const char *sactual, const char *sexpected,
    const char *file, int line)
{
    if (uFuzzyCompare(actual, expected)) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_INFO,   "   Actual   (%s): %f\n", sactual, actual);
        output(LEVEL_INFO,   "   Expected (%s): %f\n", sexpected, expected);
        output(LEVEL_LOC,    "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) Compared values are not the same.\n",
                   XFAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_INFO,   "   Actual   (%s): %f\n", sactual, actual);
            output(LEVEL_INFO  , "   Expected (%s): %f\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) Compared values are not the same.\n",
                   FAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   Actual   (%s): %f\n", sactual, actual);
            output(LEVEL_RESULT, "   Expected (%s): %f\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
            return 1;
        }
    }
    return 0;
}

int testlib_comparestr(
    const char *actual, const char *expected,
    const char *sactual, const char *sexpected,
    const char *file, int line)
{
    if (strcmp(actual, expected) == 0) {
        output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
            g_current_test_name, testlib_current_dataset());
        output(LEVEL_INFO,  "   Actual   (%s): %s\n", sactual, actual);
        output(LEVEL_INFO , "   Expected (%s): %s\n", sexpected, expected);
        output(LEVEL_LOC,    "   Loc: [%s(%i)]\n", file, line);
    } else {
        if (testlib_is_expect_fail()) {
            output(LEVEL_RESULT, "%s : %s(%s) Compared values are not the same.\n",
                   XFAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   %s\n", g_comment);
            output(LEVEL_INFO,   "   Actual   (%s): %s\n", sactual, actual);
            output(LEVEL_INFO  , "   Expected (%s): %s\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            testlib_reset_expect_fail();
            if (g_mode == Continue)
                return 0;
        } else {
            output(LEVEL_RESULT, "%s  : %s(%s) Compared values are not the same.\n",
                   FAIL, g_current_test_name, testlib_current_dataset());
            output(LEVEL_RESULT, "   Actual   (%s): %s\n", sactual, actual);
            output(LEVEL_RESULT, "   Expected (%s): %s\n", sexpected, expected);
            output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
            g_testsuccess = 0;
        }
        return 1;
    }
    return 0;
}

int testlib_comparemem(
    const unsigned char *actual, size_t actuallen,
    const unsigned char *expected, size_t expectedlen,
    const char *file, int line)
{
#ifdef ENABLE_HEXDUMP
    char dumpA[70];
    char dumpB[70];

    hexdump(dumpA, sizeof(dumpA), actual, actuallen < 16 ? actuallen : 16);
    hexdump(dumpB, sizeof(dumpB), expected, expectedlen < 16 ? expectedlen : 16);
#endif /* ENABLE_HEXDUMP */

    if (actuallen == expectedlen) {
        if (memcmp(actual, expected, actuallen) == 0) {
            output(LEVEL_GOODRESULT, "%s   : %s(%s)\n", PASS,
                g_current_test_name, testlib_current_dataset());
#ifdef ENABLE_HEXDUMP
            output(LEVEL_INFO, "   Actual   : %s\n", dumpA);
            output(LEVEL_INFO, "   Expected : %s\n", dumpB);
#endif /* ENABLE_HEXDUMP */
            output(LEVEL_LOC,    "   Loc: [%s(%i)]\n", file, line);
        } else {
            if (testlib_is_expect_fail()) {
                output(LEVEL_RESULT, "%s : %s(%s) Compared values are not the same.\n",
                       XFAIL, g_current_test_name, testlib_current_dataset());
                output(LEVEL_RESULT, "   %s\n", g_comment);
                output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
#ifdef ENABLE_HEXDUMP
                output(LEVEL_RESULT, "   Actual   : %s\n", dumpA);
                output(LEVEL_RESULT, "   Expected : %s\n", dumpB);
#endif /* ENABLE_HEXDUMP */
                testlib_reset_expect_fail();
                if (g_mode == Continue)
                    return 0;
            } else {
                output(LEVEL_RESULT, "%s  : %s(%s) Compared values are not the same.\n",
                       FAIL, g_current_test_name, testlib_current_dataset());
#ifdef ENABLE_HEXDUMP
                output(LEVEL_RESULT, "   Actual   : %s\n", dumpA);
                output(LEVEL_RESULT, "   Expected : %s\n", dumpB);
#endif /* ENABLE_HEXDUMP */
                output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
                g_testsuccess = 0;
            }
            return 1;
        }
    } else {
        output(LEVEL_RESULT, "%s  : %s(%s) Lengths are not the same.\n",
               FAIL, g_current_test_name, testlib_current_dataset());
        output(LEVEL_RESULT, "   Actual   %i\n", actuallen);
        output(LEVEL_RESULT, "   Expected %i\n", expectedlen);
        output(LEVEL_RESULT, "   Loc: [%s(%i)]\n", file, line);
        g_testsuccess = 0;
    }
    return 0;
}

void testlib_register_name(const char *name)
{
    strncpy(g_testname, name, sizeof(g_testname));
    g_testname[sizeof(g_testname)-1] = 0;
}

void testlib_register_init(testfunction func)
{
    g_init = func;
}

void testlib_register_cleanup(testfunction func)
{
    g_cleanup = func;
}

void testlib_register_test(testfunction test, const char *stest,
                           testfunction init, testfunction cleanup)
{
    testlib_register_datadriven_test(test, stest, 0, 0, init, cleanup);
}

void testlib_register_datadriven_test(
    testfunction test, const char *stest,
    testfunction testdata, const char *stestdata,
    testfunction init, testfunction cleanup)
{
    if (g_first_test == 0) {
        g_first_test = malloc(sizeof(struct list_element));
        g_last_test = g_first_test;
    } else {
        g_last_test->next = malloc(sizeof(struct list_element));
        g_last_test = g_last_test->next;
    }

    g_last_test->fctTestInit = init;
    g_last_test->fctTestCleanup = cleanup;
    g_last_test->fctTest = test;
    g_last_test->testname = stest;
    g_last_test->fctTestData = testdata;
    g_last_test->testdataname = stestdata;

    g_last_test->next = 0;
}

/** Executes registered test functions.
 * If \c testname is NULL all functions are executed, otherwise only the one
 * matching \c testname.
 * If \c testset is NULL all datasets are used for data-driven tests, otherwise
 * only the one matching \c testset.
 */
void testlib_run_tests(const char *testname, const char *testset)
{
    struct list_element *cur = g_first_test, *tmp;
    int i;

    /* count registered tests for statistic output */
    cur = g_first_test;
    while (cur) {
        g_results.num_tests++;
        cur = cur->next;
    }

    /* initialize test suite */
    if (g_init) {
        g_current_test_name = g_testname;
        g_dataset_override = "initialization";
        g_testsuccess = 1;
        /* no need for a return value in g_init as the test macros set g_testsuccess accordingly */
        g_init();
        g_dataset_override = 0;
        if (g_testsuccess != 1) {
            output(LEVEL_RESULT, "Test initialization failed, test aborted.\n");
            goto out;
        }
    }

    /* run tests */
    cur = g_first_test;
    while (cur) {
        g_current_test_name = cur->testname;
        if (testname && strcmp(cur->testname, testname) != 0) {
            cur = cur->next;
            continue;
        }
        g_testsuccess = 1;
        testlib_reset_expect_fail_forced();
        if (cur->fctTestData) {
            testdata_init(&g_data);
            output(LEVEL_FUNC, ">      : %s entering\n", cur->testdataname);
            cur->fctTestData();
            output(LEVEL_FUNC, "<      : %s returned\n", cur->testdataname);

            if (cur->fctTestInit) {
                cur->fctTestInit();
                if (g_testsuccess != 1) {
                    output(LEVEL_RESULT, "Initializing test %s failed, skipping test.\n", cur->testname);
                }
            }

            if (g_testsuccess == 1) {
                for (i = 0; i < g_data.num_rows; ++i) {
                    g_data.current_row = i;
                    if (testset && strcmp(g_data.rows[i].name, testset) != 0) {
                        continue;
                    }
                    output(LEVEL_FUNC, ">      : %s entering\n", cur->testname);
                    cur->fctTest();
                    output(LEVEL_FUNC, "<      : %s returned\n", cur->testname);
                }
            }

            if (cur->fctTestCleanup) {
                int tmp_testsuccess = g_testsuccess;
                g_testsuccess = 1;
                cur->fctTestCleanup();
                if (g_testsuccess != 1) {
                    output(LEVEL_RESULT, "Cleaning up test %s failed.\n", cur->testname);
                } else {
                    g_testsuccess = tmp_testsuccess;
                }
            }

            testdata_clear(&g_data);
        } else {
            output(LEVEL_FUNC, ">      : %s entering\n", cur->testname);
            if (cur->fctTestInit) cur->fctTestInit();
            cur->fctTest();
            if (cur->fctTestCleanup) cur->fctTestCleanup();
            output(LEVEL_FUNC, "<      : %s returned\n", cur->testname);
        }
        if (g_testsuccess)
            g_results.num_passed++;
        else
            g_results.num_failed++;
        cur = cur->next;
    }
out:
    if (g_cleanup) {
        g_current_test_name = g_testname;
        g_dataset_override = "cleanup";
        g_testsuccess = 1;
        /* no need for a return value in g_cleanup as the test macros set g_testsuccess accordingly */
        g_cleanup();
        g_dataset_override = 0;
        if (g_testsuccess != 1) {
            output(LEVEL_RESULT, "Test cleanup failed, test aborted.\n");
        }
    }

    /* cleanup registered tests */
    cur = g_first_test;
    while (cur) {
        tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

/** Lists all registered test functions. */
void testlib_list_tests()
{
    struct list_element *cur = g_first_test;

    while (cur) {
        printf("%s\n", cur->testname);
        cur = cur->next;
    }
}

/** Returns the test result statistic. */
const struct testlib_stat *testlib_result() {
    return &g_results;
}

#ifdef ENABLE_BENCHMARK
void test_benchmark_start()
{
    g_benchmark.n = 1;
    g_benchmark.i = 1;
    timer_start(&g_benchmark.t);
}

bool test_benchmark_done()
{
    uint64_t time;
    double f, fTotal;

    if (g_benchmark.i == 0) {
        timer_stop(&g_benchmark.t);
        time = timer_compute_time(&g_benchmark.t);
        if (time > g_benchmark.threshold) {
            fTotal = (double)time;
            fTotal /= 1000.0;
            f = fTotal;
            f /= g_benchmark.n;
            printf("%s : %s(%s) %f msecs per iteration (total: %f, iterations: %i)\n",
                   RESULT, g_current_test_name, testlib_current_dataset(), f, fTotal, g_benchmark.n);
            return false;
        }
        /* restart test with higher iteration counter */
        g_benchmark.n *= 2;
        g_benchmark.i = g_benchmark.n;
        timer_start(&g_benchmark.t);
    }

    return true;
}

void test_benchmark_next()
{
    --g_benchmark.i;
}
#endif /* ENABLE_BENCHMARK */

void register_tests();

#ifdef HAVE_GETOPT
/** \internal print test usage */
static void usage(const char *app)
{
    printf("C Unit Test Framework 0.1\n");
    printf("Copyright (C) 2014 Gerhard Gappmeier\n");
    printf("This is free software; see the source for copying conditions. There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    printf("Usage: %s [options] [testfunction] [testset]\n", app);
    printf("  By default all testfunctions will be run.\n\n");
    printf("Options:\n");
    printf("-l      : print a list of all testfunctions\n");
    printf("-v      : increase verbosity of output (can be specified multiple times)\n");
    printf("-s      : decrease verbosity of output\n");
#ifdef ENABLE_COLOR
    printf("-c WHEN : colorize the output. WHEN defaults to 'auto' or can be 'never' or 'always'.\n");
    printf("          If WHEN=auto it tries to detect if stdout goes to a terminal. If so color output\n"
           "          is enabled, otherwise it's disabled.\n");
#endif /* ENABLE_COLOR */
    printf("-h      : Shows this help\n");
}

/** Test main implementation used by UTEST_MAIN. */
int testlib_main(int argc, char *argv[])
{
    int opt;
    int list = 0;
    const struct testlib_stat *result;
    const char *testname = 0;
    const char *testset = 0;
    int ret = EXIT_SUCCESS;
#ifdef ENABLE_COLOR
    int enable_color = 1;

# ifdef __linux__
    if (isatty(STDOUT_FILENO) == 0) {
        enable_color = 0;
    }
# endif /* __linux__ */
#endif /* ENABLE_COLOR */

#if defined (_MSC_VER) && !defined (_WIN32_WCE)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    /* intialize testname with a default */
    testname = strrchr(argv[0], '/');
    if (testname) {
        testname++;
        strncpy(g_testname, testname, sizeof(g_testname));
        testname = 0;
    } else {
        strncpy(g_testname, argv[0], sizeof(g_testname));
    }
    g_testname[sizeof(g_testname)-1] = 0;

    while ((opt = getopt(argc, argv, "vshlc:")) != -1) {
        switch (opt) {
        case 'v':
            testlib_verbose();
            break;
        case 's':
            testlib_silent();
            break;
        case 'l':
            list = 1;
            break;
#ifdef ENABLE_COLOR
        case 'c':
            if (strcmp(optarg, "never") == 0) {
                enable_color = 0;
            } else if (strcmp(optarg, "always") == 0) {
                enable_color = 1;
            } else if (strcmp(optarg, "auto") != 0) {
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
#endif /* ENABLE_COLOR */
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        default: /* '?' */
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        testname = argv[optind++];
    }
    if (optind < argc) {
        testset = argv[optind++];
    }

#ifdef ENABLE_COLOR
    if (enable_color == 0) testlib_disable_color();
#endif /* ENABLE_COLOR */

    register_tests();

    if (list) {
        testlib_list_tests();
    } else {
        int run;
        printf("********* Start testing of %s *********\n", g_testname);

        testlib_run_tests(testname, testset);
        result = testlib_result();
        run = result->num_passed + result->num_failed + result->num_skipped;
        printf("Test finished: %i tests of %i were run.\n", run, result->num_tests);
        printf("  %i PASSED.\n", result->num_passed);
        printf("  %i FAILED.\n", result->num_failed);
        printf("  %i SKIPPED.\n", result->num_skipped);
        printf("********* Finished testing of %s *********\n", g_testname);
        if (result->num_failed > 0)
            ret = EXIT_FAILURE;
    }

    return ret;
}

#else /* HAVE_GETOPT */
/** Test main implementation used by UTEST_MAIN.
 * This is a simplified version of testlib_main for systems which have
 * no getopt() implementation or if you simply want to avoid the additional
 * code of getopt().
 * Of course here you have no commandline option parsing, but it still
 * allows to specify one testname.
 */
int testlib_main(int argc, char *argv[])
{
    const struct testlib_stat *result;
    const char *testname = 0;
    const char *testset = 0;
    int ret = EXIT_SUCCESS;

    if (argc > 1) {
        testname = argv[1];
    }

    register_tests();

    testlib_run_tests(testname, testset);

    result = testlib_result();
    printf("Test finished: %i tests were run.\n", result->num_tests);
    printf("  %i PASSED.\n", result->num_passed);
    printf("  %i FAILED.\n", result->num_failed);
    printf("  %i SKIPPED.\n", result->num_skipped);
    if (result->num_failed > 0)
        ret = EXIT_FAILURE;

    return ret;
}
#endif /* HAVE_GETOPT */
