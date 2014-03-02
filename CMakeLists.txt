cmake_minimum_required(VERSION 2.8)
project(OpenSpades)

set(OpenSpades_VERSION_MAJOR 0)
set(OpenSpades_VERSION_MINOR 0)
set(OpenSpades_VERSION_REVISION 10)

set(OS_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_PREFIX_PATH Sources/Externals)

include(cmake/FindSDL2.cmake)
if(NOT SDL2_FOUND)
	message(FATAL_ERROR "SDL 2.0 not found, set ENV{SDL2DIR} to point to SDL 2.0, and optionally set the cmake var SDL2_LIBRARY_TEMP to the lib dir")
endif()

include(cmake/FindSDL2_image.cmake)
if(NOT SDL2_IMAGE_FOUND)
	message(FATAL_ERROR "SDL_image 2.0 not found, set ENV{SDL2DIR} to point to SDL 2.0, and optionally set the cmake var SDL2_LIBRARY_TEMP to the lib dir")
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
set(FLTK_SKIP_IMAGES TRUE)
include(FindFLTK)
if(NOT FLTK_FOUND)
	message(FATAL_ERROR "FLTK not found, manually set FLTK_INCLUDE_DIR in CMake")
endif()

set(FLTK_OS_LIBS ${FLTK_BASE_LIBRARY})


include(FindZLIB)
if(NOT ZLIB_FOUND)
	message(FATAL_ERROR "ZLIB not found, manually set ZLIB_ROOT in CMake")
endif()

include(FindCURL)
if(NOT CURL_FOUND)
	message(FATAL_ERROR "cURL not found, please install it (and make sure it's in your path)")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	# GCC C++11 support
	# from http://stackoverflow.com/questions/10984442/how-to-detect-c11-support-of-a-compiler-with-cmake
	#
	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
	if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
		message(STATUS "C++11 activated.")
		add_definitions("-std=gnu++11")
	elseif(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
		message(WARNING "C++0x activated. If you get any errors update to a compiler which fully supports C++11")
		add_definitions("-std=gnu++0x")
	else ()
		message(FATAL_ERROR "C++11 needed. Therefore a gcc compiler with a version higher than 4.3 is needed.")
	endif()

	add_definitions(
		-Wall
		-Wextra
		-Wno-unknown-pragmas #learn_more: output is spammed like shit by all those #pragma mark items
	)

elseif(MSVC)
	add_definitions(
		/D_CRT_SECURE_NO_WARNINGS
		/wd4068
		/DNOMINMAX
		/D_USE_MATH_DEFINES
	)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")	#static link to msv runtime (MSVCR(P)xxx.dll)
	#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
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
set(OPENSPADES_EXTRATITLE "" CACHE STRING "Extra info to add to the package name")
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
configure_file("${PROJECT_SOURCE_DIR}/OpenSpades.rc.in" "${PROJECT_BINARY_DIR}/OpenSpades.rc")
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${SDL2_INCLUDE_DIR}")
include_directories("${SDL2_IMAGE_INCLUDE_DIR}")
if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
endif()
include_directories("${GLEW_INCLUDE_DIR}")
include_directories("${FLTK_INCLUDE_DIR}")
include_directories("${ZLIB_INCLUDE_DIR}")
include_directories("${CURL_INCLUDE_DIRS}")

add_subdirectory(Resources)
add_subdirectory(Sources)

install(FILES AUTHORS ChangeLog LICENSE README.md DESTINATION bin)

#see http://www.cmake.org/cmake/help/v2.8.11/cpack.html

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenSpades")
set(CPACK_PACKAGE_VENDOR "yvt, assorted")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${OpenSpades_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OpenSpades_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OpenSpades_VERSION_REVISION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
if(WIN32 AND NOT UNIX)
	# There is a bug in NSIS that does not handle full unix paths properly. Make
	# sure there is at least one set of four (4) backlasshes.
	set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\OpenSpades.exe")
	set(CPACK_NSIS_HELP_LINK "https://sites.google.com/a/yvt.jp/openspades/")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://buildandshoot.com/viewforum.php?f=57")
	#SET(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
	set(CPACK_NSIS_MODIFY_PATH ON)
	set(CPACK_PACKAGING_INSTALL_PREFIX "/")
else()
	set(CPACK_STRIP_FILES "bin/OpenSpades")
	set(CPACK_SOURCE_STRIP_FILES "")
endif(WIN32 AND NOT UNIX)
set(CPACK_PACKAGE_EXECUTABLES "OpenSpades" "OpenSpades")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)		#everyone hates a dir inside a zip named exactly like the zip
include(CPack)

