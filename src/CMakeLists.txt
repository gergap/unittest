project(unittest C)
cmake_minimum_required(VERSION 2.8)

# add "make doc" build target
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOC_DIR ${CMAKE_SOURCE_DIR}/../doc)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${DOC_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-Wall -Wextra)
endif()

# enable testing
enable_testing()
# enable CDash
include(CTest)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_subdirectory(third-party/pthread-win32)
endif()
add_subdirectory(testlib)
add_subdirectory(test)


