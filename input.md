ANSI C based Unit Test Framework
================================

(C) 2014 Gerhard Gappmeier <gappy1502@gmx.net>

License
-------

GPLv2, see LICENSE file.

Concept
-------

I developed this because there was no good ANSI based unittest framework
available which fulfills my requirements.

This unittest framework is designed portable and small so that it works also
for embedded projects on micro controllers. In the minimum configuration it
only requires one c file and one h file (testlib.h, testlib.c). You can simply
compile this with your micro controller project.

On PC based platforms like Linux and Windows this offers even more
functionality.

# Features:

* portable design
* good and easy integration into CMake
* data-driven tests
* supports terminal colors (Linux only)
* optional benchmark functionality which allows to measure performance of
  individual tests.
* commandline arguments to list test functions, execute individual tests and
  datasets. (optional, requires getopt())
* well documented

# Documentation

You can build the API documentation using "make doc". This requires doxygen to
be installed.

You can also read the [Online Reference Documentation](http://gergap.github.io/unittest/doc/html/index.html)


# Examples

You find unit test examples in src/test.

A minimum example of writing a unit test is a simple as that:

    #include <testlib.h>
    void test_helloworld()
    {
        UVERIFY2(true, "Hello World");
    }
    void register_tests()
    {
        UREGISTER_TEST(test_helloworld);
    }
    UTEST_MAIN()

# Installation

The easiest way to get the unittest framework is using git.

    git clone https://github.com/gergap/unittest
    cd unittest
    git submodule init
    git submodule update

Then follow the build instructions in INSTALL file.

