cmake_minimum_required(VERSION 2.8)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
cmake_policy(SET CMP0003 NEW)

project(angelscript_add_on)

set(ANGELSCRIPT_SOURCE
        ../../add_on/scriptany/scriptany.cpp
        ../../add_on/scriptarray/scriptarray.cpp
        ../../add_on/scriptbuilder/scriptbuilder.cpp
        ../../add_on/scriptdictionary/scriptdictionary.cpp
        ../../add_on/scripthandle/scripthandle.cpp
        ../../add_on/scripthelper/scripthelper.cpp
        ../../add_on/scriptmath/scriptmath.cpp
        ../../add_on/scriptmath/scriptmathcomplex.cpp
        ../../add_on/scriptstdstring/scriptstdstring.cpp
        ../../add_on/scriptstdstring/scriptstdstring_utils.cpp
        ../../add_on/weakref/weakref.cpp
)

set(ANGELSCRIPT_HEADERS
        ../../include/angelscript.h
        ../../add_on/scriptany/scriptany.h
        ../../add_on/scriptarray/scriptarray.h
        ../../add_on/scriptbuilder/scriptbuilder.h
        ../../add_on/scriptdictionary/scriptdictionary.h
        ../../add_on/scripthandle/scripthandle.h
        ../../add_on/scripthelper/scripthelper.h
        ../../add_on/scriptmath/scriptmath.h
        ../../add_on/scriptmath/scriptmathcomplex.h
        ../../add_on/scriptstdstring/scriptstdstring.h
        ../../add_on/weakref/weakref.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_definitions("-D_CRT_SECURE_NO_WARNINGS -DANGELSCRIPT_EXPORT -D_LIB")

# Fix x64 issues on Linux
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" AND NOT APPLE)
        add_definitions(-fPIC)
endif()

add_library(angelscript_add_on STATIC ${ANGELSCRIPT_SOURCE} ${ANGELSCRIPT_HEADERS})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

find_package(Threads)
target_link_libraries(angelscript_add_on ${CMAKE_THREAD_LIBS_INIT})

if(MSVC)
        set_target_properties(angelscript_add_on PROPERTIES COMPILE_FLAGS "/MP")
endif(MSVC)

set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)

