project(test C)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
include(unittest)

set(TEST_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

ADD_UNIT_TEST(Test main.c)
ADD_UNIT_TEST(Hello hello.c)

