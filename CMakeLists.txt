cmake_minimum_required(VERSION 2.8.8)
project(OpenSpades)

set(OpenSpades_VERSION_MAJOR 0)
set(OpenSpades_VERSION_MINOR 0)
set(OpenSpades_VERSION_REVISION 8)

set(CMAKE_PREFIX_PATH Sources/Externals)

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

include(FindCURL)
if(NOT CURL_FOUND)
	message(FATAL_ERROR "cURL not found, please install it (and make sure it's in your path)")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wall")
endif()

if(MSVC)
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4068 /DNOMINMAX /D_USE_MATH_DEFINES" )
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)	#msvc / xcode: group files in project


#mainly for ENet...
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckTypeSize)
check_function_exists("fcntl" HAS_FCNTL)
check_function_exists("poll" HAS_POLL)
check_function_exists("gethostbyname_r" HAS_GETHOSTBYNAME_R)
check_function_exists("gethostbyaddr_r" HAS_GETHOSTBYADDR_R)
check_function_exists("inet_pton" HAS_INET_PTON)
check_function_exists("inet_ntop" HAS_INET_NTOP)
check_struct_has_member("struct msghdr" "msg_flags" "sys/types.h;sys/socket.h" HAS_MSGHDR_FLAGS)
set(CMAKE_EXTRA_INCLUDE_FILES "sys/types.h" "sys/socket.h")
check_type_size("socklen_t" HAS_SOCKLEN_T BUILTIN_TYPES_ONLY)
unset(CMAKE_EXTRA_INCLUDE_FILES)

if(HAS_FCNTL)
    add_definitions(-DHAS_FCNTL=1)
endif()
if(HAS_POLL)
    add_definitions(-DHAS_POLL=1)
endif()
if(HAS_GETHOSTBYNAME_R)
    add_definitions(-DHAS_GETHOSTBYNAME_R=1)
endif()
if(HAS_GETHOSTBYADDR_R)
    add_definitions(-DHAS_GETHOSTBYADDR_R=1)
endif()
if(HAS_INET_PTON)
    add_definitions(-DHAS_INET_PTON=1)
endif()
if(HAS_INET_NTOP)
    add_definitions(-DHAS_INET_NTOP=1)
endif()
if(HAS_MSGHDR_FLAGS)
    add_definitions(-DHAS_MSGHDR_FLAGS=1)
endif()
if(HAS_SOCKLEN_T)
    add_definitions(-DHAS_SOCKLEN_T=1)
endif()


set(OPENSPADES_RESDIR "" CACHE STRING "Resource dir to use while developing")
if(OPENSPADES_RESDIR)
	set(RESDIR ${OPENSPADES_RESDIR})
	set(RESDIR_DEFINED TRUE)
	mark_as_advanced(RESDIR)
	mark_as_advanced(RESDIR_DEFINED)
endif()


if(CMAKE_CXX_COMPILER_ID)
	if(CMAKE_CXX_COMPILER_VERSION)
		set(_version " ${CMAKE_CXX_COMPILER_VERSION}")
	else()
		set(_version "")
	endif()
	set(OPENSPADES_COMPILER ${CMAKE_CXX_COMPILER_ID}${_version})
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
include_directories("${CURL_INCLUDE_DIRS}")

add_subdirectory(Resources)
add_subdirectory(Sources)


