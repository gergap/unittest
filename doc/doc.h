/** @mainpage
 * @author Gerhard Gappmeier
 * @copyright (C) 2014 Gerhard Gappmeier
 *
 * The C Unit Test Framework is a portable unit test framework written in ANSI C
 * which is intended to test other portable C code. The design is heavily based
 * on the Qt TestLib Framework and JUnit.
 *
 * When developing portable ANSI C code it makes no sense to use a unit test
 * framework that requires JAVA, C++ or any other third-party library like Qt.
 * The unittests must be as portable as the code that should be tested, so that
 * it is possible to cross-compile the tests with your code and run them on the
 * target device.
 *
 * @tableofcontents
 *
 * @section features Features
 *
 * - Portable ANSI C Code
 * - Very lightweight - This framework consists only of one source and one
 *   header file (testlib.h, testlib.c).
 * - Easy to use
 * - It supports \ref datadriven "data-driven tests"
 * - It supports marking checks as expected failures (See #UEXPECT_FAIL)
 * - It supports correct exit codes which is important for batch processing
 * - It can easily be integrated into CMake/CDash
 * - Support for terminal colors (Linux only, this requires one additional
 *   header: linuxtermcolors.h )
 *
 * @section requirements Requirements
 *
 * - C99 compiler like GCC
 *
 * Notes on Windows: Windows is also supported, but because many C99 features
 * are missing this project comes with some additional headers and sources to
 * make it working on Windows. This violates the one header and one source
 * rule, but it works.
 *
 * Notes on UTEST_MAIN(): The test_main() implementation uses getopt() which
 * conforms to POSIX.2 and POSIX.1-2001. getopt() is not available on all
 * systems (e.g. Windows), that's why this project ships with an alternative
 * implementation that was tested on Windows. It is plain C and should work also
 * on other systems.
 * Anyway you can use the test framework also without using UTEST_MAIN() by
 * providing your own main function and calling testlib_run_tests().
 *
 * @section intro Introduction
 *
 * A unit test may contain multiple test suites, where each test suite contains
 * a set of test cases. Typically for each module/datastructure that should be
 * tested you create an own test suite. E.g. a StringTest, a LinkedListTest, etc.
 *
 * One executing a test suite all according tests will be executed. This way you
 * can run tests for single software components easily without specifying all
 * the single test cases.
 *
 * For debugging purpose it is of course possible to run only a single test
 * case.
 *
 * @section howto Howto write unit tests
 *
 * The example below shows a very basic working unit test example.
 *
 * \include hello.c
 *
 * The test function is a simple void function without arguments. For each
 * test you can create such a function. In a test function you can add multiple
 * checks like UVERIFY2(). The table below gives you an overview about available
 * check macros.
 *
 * The next step is to provide a register_tests() function which is called by
 * the main function. You can use the UREGISTER_TEST() and
 * UREGISTER_DATADRIVEN_TEST() macros to make your functions known to the
 * framework.
 *
 * The macro UTEST_MAIN() provides a default main implementation which processes
 * command line arguments and executes the tests.
 *
 * Macro       | Description
 * ------------|------------
 * #UVERIFY     | Checks if a condition is true or not.
 * #UVERIFY2    | Like UVERIFY but with additional info.
 * #UCOMPARE    | Compares an actual value with an expected value.
 * #UCOMPAREF   | Same as UCOMPARE but for floats.
 * #UFUZZY_COMPAREF | Performs a fuzzy compare to avoid rounding errors.
 * #UCOMPARESTR | Same as UCOMPARE but for strings.
 * #UFATAL      | Only for fatal errors. This stops test execution.
 *
 * When comparing data values you should always use UCOMPARE() and not
 * UVERIFY(x == 5);
 *
 * @section datadriven Writing data-driven unit tests
 *
 * When you need to test a function with many different values to test all
 * corner-cases it is cumbersome to repeat the same test code again and again.
 * Instead you should write a data-driven test.
 *
 * With data-driven tests you can write one single test function which is called
 * multiple times by the test framework with different datasets.
 * For data-driven test you must provide another function which sets up the test
 * data. By convention you should give this function the same name as the test
 * function with a \c _data suffix appended.
 *
 * The example below demonstrates howto implement a simple data-drive test.
 *
 * \snippet main.c datadriven test example
 *
 * Note: the UEXPECT_FAIL() macro marks one check to be expected to fail. So
 * known bugs can be excluded from the result to avoid being reported by the
 * buildbot. This way is preferred over disabling or skipping test cases.
 * -# As you can see in this example it possible to disable only one dataset, and
 * the other sets are still tested.
 * -# This way the problem still gets reported
 * in the test output and cannot be forgotten.
 *
 * To register this data driven test you need to use the
 * UREGISTER_DATADRIVEN_TEST() macro which takes two arguments.
 *
 * \snippet main.c register datadriven test
 *
 * @section running Running the unit tests
 *
 * When running the test executable with th option -h all options are listed.
 *
 * @code
 * $> ./test -h
 * @endcode
 * \c Output:
 * \htmlinclude demo_help.html
 *
 * So to execute all tests simply run the executable without any arguments.
 *
 * @code
 * $> ./test
 * @endcode
 * \c Output:
 * \htmlinclude demo.html
 *
 * @section cmake Integration in CMake
 *
 * On the CMake Dashboard (CDash) it is not useful to see every test case in
 * the test result table. Nor it useful to only see one result for the whole
 * unittest.
 * The recommended way is to show each test suite as a separate row in the test
 * result table. By clicking on the test name you can inspect the output of this
 * test and see the results of each single test case.
 *
 * To achieve this you need to add an ADD_TEST call for each test suite. This
 * can be done using a single large unittest executable with an argument which
 * specifies which suite should be run, or you can generate independent test
 * executables for each test suite. This is shown below.
 *
 * To be able to compile individual test executables you can use the UTEST_MAIN()
 * macro. This provides a default main implementation.
 *
 * @subsection cmake_step1 Step 1 - Enable testing support
 *
 * To enable the CMake test target (make test) you need to call enable_testing()
 * in your toplevel CMake file.
 *
 * @code
 * project(demo C)
 * cmake_minimum_required(VERSION 2.8)
 *
 * # enable test target
 * enable_testing()
 *
 * # build my cool libraries
 * add_subdirectory(foo)
 * add_subdirectory(bar)
 * @endcode
 *
 * \subsection cmake_step2 Step 2 - Enable CDash support (optional)
 *
 * If you want to support CDash to upload test results to the dashboard using
 * 'make Nightly', 'make Continuous', etc. you need to include CTest.
 *
 * @code
 * project(demo C)
 * cmake_minimum_required(VERSION 2.8)
 *
 * # enable test target
 * enable_testing()
 *
 * # enable CDash
 * include(CTest)
 *
 * # build my cool libraries
 * add_subdirectory(foo)
 * add_subdirectory(bar)
 * @endcode
 *
 * \subsection cmake_step3 Step 3 - Add unittests
 *
 * Using the provided \c unittest CMake Module it is easy to add new tests.
 *
 * @code
 * project(demo C)
 * cmake_minimum_required(VERSION 2.8)
 * # configure search path to find unittest.cmake
 * set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake)
 * # include unittest module
 * include(unittest)
 *
 * # enable test target
 * enable_testing()
 *
 * # enable CDash
 * include(CTest)
 *
 * # configure search path to unittest headers
 * include_directories(testlib)
 *
 * # build my cool libraries
 * add_subdirectory(foo)
 * add_subdirectory(bar)
 *
 * # add unit tests
 * ADD_UNIT_TEST(Foo footest.c)
 * ADD_UNIT_TEST(Bar bartest.c)
 * @endcode
 *
 */
