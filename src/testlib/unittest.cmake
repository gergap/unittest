# Provides a macro for creating unittest executables using
# the C Unit Test Framework.
# Copyright (C) 2014 Gerhard Gappmeier
#
# Requirements:
# * you need to enable testing using 'enable_testing()' in your top-level
#   CMake file.
# * if you want to support CDash you need to add
#   'include(CTest)' in your top-level cmake file.
#
# Usage:
#  ADD_UNIT_TEST(<TestName> <Source>)
# Example:
#  ADD_UNIT_TEST(String teststring.c)
#
# This creates an executable named test_<TestName> and add a CMake test
# with the same name. Add also adds the test to the INSTALL target, so that
# 'make install' will install them into ${CMAKE_INSTALL_PREFIX}/bin. When
# running the tests using 'make test' it will also use this directory as the
# WORKING_DIRECTORY.
#
# There are two variables you can use to customize the build of unit test
# executables.
#
# * additional sources to compile with each test
#   set(TEST_ADDITIONAL <sources>)
# * libraries to link with each test
#   set(TEST_LIBS <libraries>)
#
# You can change the default working directory of the tests by overriding the
# variable TEST_WORKING_DIRECTORY.
# Note: The default dir may cause problems when using relative INSTALL_PREFIX
# like ../dist. Therefore it is recommended to use absolute paths like
# /usr/local.
#
# You can define paths to prepend to the PATH variable while running the test
# by setting TEST_ENVIRONMENT. On Windows, set this to a semicolon separated
# list, to a colon separated one on all other systems, e.g.:
# Windows: SET(TEST_ENVIRONMENT "${TEST_ENVIRONMENT};${uastack_BINARY_DIR}")
# Other:   SET(TEST_ENVIRONMENT "${TEST_ENVIRONMENT}:${uastack_BINARY_DIR}")

# default value
set(TEST_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)
set(TARGET "root@192.168.10.2")

# macro for creating unit test executables
macro(ADD_UNIT_TEST testname testsrc)
    set(test_${testname}_SRCS ${testsrc})

    if (EXISTS ${testlib_SOURCE_DIR} AND EXISTS ${testlib_BINARY_DIR})
        include_directories(${testlib_SOURCE_DIR})
        include_directories(${testlib_BINARY_DIR})
    endif ()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND EXISTS ${pthread-win32_SOURCE_DIR})
        include_directories(${pthread-win32_SOURCE_DIR})
    endif ()

    add_executable(test_${testname} ${test_${testname}_SRCS} ${TEST_ADDITIONAL})
    target_link_libraries(test_${testname} testlib ${TEST_LIBS})

    install(TARGETS test_${testname} RUNTIME DESTINATION bin)

    if (NOT IS_ABSOLUTE ${TEST_WORKING_DIRECTORY})
        message(STATUS "Relative path passed to ADD_UNIT_TEST for test ${testname}, making it absolute:")
        message(STATUS "Configured value: ${TEST_WORKING_DIRECTORY}")
        set(TEST_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${TEST_WORKING_DIRECTORY}")
        get_filename_component( TEST_WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY} ABSOLUTE )
        message(STATUS "New value: ${TEST_WORKING_DIRECTORY}")
    endif (NOT IS_ABSOLUTE ${TEST_WORKING_DIRECTORY})

    if ((CMAKE_VERSION VERSION_EQUAL "2.8.10" OR CMAKE_VERSION VERSION_GREATER "2.8.10") AND CMAKE_DEBUG_POSTFIX)
        set_target_properties(test_${testname} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
        set(test_COMMAND test_${testname}$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>)
    else()
        set(test_COMMAND test_${testname})
    endif()

    if (TEST_ENVIRONMENT)
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
            # Replacing ; with \; is necessary here, otherwise TEST_ENVIRONMENT2 is treated as a list
            # http://www.cmake.org/pipermail/cmake/2009-May/029427.html
            string(REGEX REPLACE "([^\\\\]);" "\\1\\\\;" TEST_ENVIRONMENT2 "${TEST_ENVIRONMENT};$ENV{PATH}")
        else ()
            set(TEST_ENVIRONMENT2 "${TEST_ENVIRONMENT}:$ENV{PATH}")
        endif ()
    endif ()

    if (NOT CROSS_COMPILING AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "^(QNX|vxWorks)$")
        add_test(NAME test_${testname}
                 COMMAND ${test_COMMAND} -s
                 WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
        if (TEST_ENVIRONMENT2)
            set_tests_properties(test_${testname} PROPERTIES ENVIRONMENT "PATH=${TEST_ENVIRONMENT2}")
        endif ()
    elseif (CROSS_COMPILING)
        add_test(NAME test_${testname}
                 COMMAND ssh ${TARGET} /root/bin/${test_COMMAND} -s
                 WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
        if (TEST_ENVIRONMENT2)
            set_tests_properties(test_${testname} PROPERTIES ENVIRONMENT "PATH=${TEST_ENVIRONMENT2}")
        endif ()
    endif ()
endmacro(ADD_UNIT_TEST)

