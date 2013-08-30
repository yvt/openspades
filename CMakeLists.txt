cmake_minimum_required(VERSION 2.8)
project(OpenSpades)

set(OpenSpades_VERSION_MAJOR 0)
set(OpenSpades_VERSION_MINOR 0)
set(OpenSpades_VERSION_REVISION 7)

include(FindSDL)
if(NOT SDL_FOUND)
	message(FATAL_ERROR "OpenSDL not found, set ENV{SDLDIR} to point to OpenSDL, and optionally set the cmake var SDL_LIBRARY_TEMP to the lib dir")
endif()

include(FindOpenGL)
if(NOT OPENGL_FOUND AND NOT OPENGL_XMESA_FOUND AND NOT OPENGL_GLU_FOUND)
	message(FATAL_ERROR "OpenGL not found, please install it")
endif()

include(cmake/FindGLEW2.cmake)
if(NOT GLEW_FOUND)
	message(FATAL_ERROR "GLEW not found, please install it and make sure CMake can find it (add it to the PATH)")
endif()

include(FindOpenAL)
if(OPENAL_FOUND)
	set(HAVE_OPENAL TRUE)
else()
	message(STATUS "OpenAL not found, set ENV{OPENALDIR} to point to OpenAL")
endif()

set(FLTK_SKIP_OPENGL TRUE)
set(FLTK_SKIP_FORMS TRUE)
set(FLTK_SKIP_FLUID TRUE)
include(FindFLTK)
if(NOT FLTK_FOUND)
	message(FATAL_ERROR "FLTK not found, manually set FLTK_INCLUDE_DIR in CMake")
endif()

set(FLTK_OS_LIBS ${FLTK_BASE_LIBRARY} ${FLTK_IMAGES_LIBRARY})
find_library(FLTK_PNG NAMES fltkpng fltkpngd fltk_png PATHS ${FLTK_LIBRARY_SEARCH_PATH})
if(FLTK_PNG)
	list(APPEND FLTK_OS_LIBS ${FLTK_PNG})
endif()
find_library(FLTK_JPEG NAMES fltkjpeg fltkjpegd fltk_jpeg PATHS ${FLTK_LIBRARY_SEARCH_PATH})
if(FLTK_JPEG)
	list(APPEND FLTK_OS_LIBS ${FLTK_JPEG})
endif()


include(FindZLIB)
if(NOT ZLIB_FOUND)
	message(FATAL_ERROR "ZLIB not found, manually set ZLIB_ROOT in CMake")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wall")
endif()

if(MSVC)
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4068 /DNOMINMAX /D_USE_MATH_DEFINES" )
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)	#msvc / xcode: group files in project



include(CheckIncludeFiles)
check_include_files(fcntl.h HAS_FCNTL_H)
check_include_files(poll.h HAS_POLL)
check_include_files(unistd.h HAS_UNISTD_H)

include(CheckFunctionExists)
check_function_exists(gethostbyaddr_r HAS_GETHOSTBYADDR_R)
check_function_exists(gethostbyname_r HAS_GETHOSTBYNAME_R)
check_function_exists(inet_pton HAS_INET_PTON)
check_function_exists(inet_ntop HAS_INET_NTOP)

set(OPENSPADES_RESDIR "" CACHE STRING "Resource dir to use while developing")
if(OPENSPADES_RESDIR)
	set(RESDIR ${OPENSPADES_RESDIR})
	set(RESDIR_DEFINED TRUE)
	mark_as_advanced(RESDIR)
	mark_as_advanced(RESDIR_DEFINED)
endif()


configure_file("${PROJECT_SOURCE_DIR}/OpenSpades.h.in" "${PROJECT_BINARY_DIR}/OpenSpades.h")
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${SDL_INCLUDE_DIR}")
if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
endif()
include_directories("${GLEW_INCLUDE_DIR}")
include_directories("${FLTK_INCLUDE_DIR}")
include_directories("${ZLIB_INCLUDE_DIR}")

add_subdirectory(Resources)
add_subdirectory(Sources)


#EXECUTABLE_OUTPUT_PATH

#if test "$prefix" = "NONE"; then
#	FLTK_CONFIG="fltk-config"
#else
#	FLTK_CONFIG="${prefix}/bin/fltk-config"
#fi

#FLTK_CFLAGS=`${FLTK_CONFIG} --cxxflags --use-images`
#FLTK_LIBS=`${FLTK_CONFIG} --ldflags --use-images`


#AC_CHECK_MEMBER(struct msghdr.msg_flags, [AC_DEFINE(HAS_MSGHDR_FLAGS)], , [#include <sys/socket.h>])

#AC_CHECK_TYPE(socklen_t, [AC_DEFINE(HAS_SOCKLEN_T)], , 
              #include <sys/types.h>
              #include <sys/socket.h>
#)
