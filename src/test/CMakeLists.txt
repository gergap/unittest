project(test C)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
include(unittest)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # we are adding some C99 stuff, that is still missing on Windows
    # Come on MS, it's 2014 and VS still only supports C89!!!
    include_directories(../testlib/win32)
endif()

set(TEST_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

ADD_UNIT_TEST(Test main.c)
ADD_UNIT_TEST(Hello hello.c)

