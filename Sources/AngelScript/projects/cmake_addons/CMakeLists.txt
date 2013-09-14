cmake_minimum_required(VERSION 2.6)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
cmake_policy(SET CMP0003 NEW)

project(Angelscript_addons)

set(ANGELSCRIPT_SOURCE
        ../../addons/scriptany.cpp
        ../../addons/scriptarray.cpp
        ../../addons/scriptbuilder.cpp
        ../../addons/scriptdictionary.cpp
        ../../addons/scripthandle.cpp
        ../../addons/scripthelper.cpp
        ../../addons/scriptmath.cpp
        ../../addons/scriptmathcomplex.cpp
        ../../addons/scriptstdstring.cpp
        ../../addons/scriptstdstring_utils.cpp
        ../../addons/weakref.cpp
)

set(ANGELSCRIPT_HEADERS
        ../../include/angelscript.h
        ../../addons/scriptany.h
        ../../addons/scriptarray.h
        ../../addons/scriptbuilder.h
        ../../addons/scriptdictionary.h
        ../../addons/scripthandle.h
        ../../addons/scripthelper.h
        ../../addons/scriptmath.h
        ../../addons/scriptmathcomplex.h
        ../../addons/scriptstdstring.h
        ../../addons/weakref.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_definitions("-D_CRT_SECURE_NO_WARNINGS -DANGELSCRIPT_EXPORT -D_LIB")

# Fix x64 issues on Linux
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" AND NOT APPLE)
        add_definitions(-fPIC)
endif()

add_library(Angelscript_addons STATIC ${ANGELSCRIPT_SOURCE} ${ANGELSCRIPT_HEADERS})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

find_package(Threads)
target_link_libraries(Angelscript_addons ${CMAKE_THREAD_LIBS_INIT})

if(MSVC)
        set_target_properties(Angelscript_addons PROPERTIES COMPILE_FLAGS "/MP")
endif(MSVC)

set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)

