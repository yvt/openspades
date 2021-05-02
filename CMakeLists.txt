cmake_minimum_required(VERSION 2.8)
project(OpenSpades)

set(OpenSpades_VERSION_MAJOR 0)
set(OpenSpades_VERSION_MINOR 1)
set(OpenSpades_VERSION_REVISION 5)

set(OPENSPADES_FULL_VERSION "${OpenSpades_VERSION_MAJOR}.${OpenSpades_VERSION_MINOR}.${OpenSpades_VERSION_REVISION}")

set(OS_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(VCPKG_TARGET_TRIPLET)
	set(USE_VCPKG ON)
else()
	set(USE_VCPKG OFF)
endif()

# Prefer GLVND OpenGL
if(POLICY CMP0072)
	cmake_policy(SET CMP0072 NEW)
endif()

if(USE_VCPKG)
    find_package(Ogg CONFIG REQUIRED)
    find_package(Opus CONFIG REQUIRED)
    find_package(sdl2-image CONFIG REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(freetype CONFIG REQUIRED)

 	if(NOT APPLE)
 		find_package(GLEW REQUIRED)
 		set(GLEW_LIBRARY GLEW::GLEW)

 		find_package(CURL REQUIRED)
 		set(CURL_LIBRARY CURL::libcurl)
 	endif()

    set(SDL2_LIBRARY SDL2::SDL2main SDL2::SDL2-static)
    set(SDL2_IMAGE_LIBRARY SDL2::SDL2_image)
    set(FREETYPE_LIBRARIES freetype)

    if(APPLE)
    	# Use system libraries
		include(FindOpenGL)
		include(FindCURL)
    endif()

	FIND_LIBRARY(OpusFile_LIBRARY
	  NAMES opusfile
	  PATH_SUFFIXES lib64 lib
	  PATHS ${CMAKE_PREFIX_PATH}
	)
else()
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
	if(NOT APPLE)
		include(cmake/FindGLEW2.cmake)
		if(NOT GLEW_FOUND)
			message(FATAL_ERROR "GLEW not found, please install it and make sure CMake can find it (add it to the PATH)")
		endif()
	endif()

	include(FindZLIB)
	if(NOT ZLIB_FOUND)
		message(FATAL_ERROR "ZLIB not found, manually set ZLIB_ROOT in CMake")
	endif()

	include(FindCURL)
	if(NOT CURL_FOUND)
		message(FATAL_ERROR "cURL not found, please install it (and make sure it's in your path)")
	endif()

	include(FindFreetype)
	if(NOT FREETYPE_FOUND)
		message(FATAL_ERROR "FreeType not found, please install it (and make sure it's in your path)")
	endif()

	include(cmake/FindOpus.cmake)
	if(NOT OpusFile_FOUND)
		message(FATAL_ERROR "libopus/libopusfile not found, please install it (and make sure it's in your path)")
	endif()
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

	if(CMAKE_BUILD_TYPE MATCHES "Release")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
		set(CMAKE_EXE_LINKER_FLAGS "-s")
	endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	# Clang C++11 support
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

	add_definitions(
		-Wall
		-Wextra
		-Wno-unused-parameter
	)

	if(CMAKE_BUILD_TYPE MATCHES "Release")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
		set(CMAKE_EXE_LINKER_FLAGS "-s")
	endif()

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

option(OPENSPADES_RESOURCES "Build game assets" ON)
option(OPENSPADES_NONFREE_RESOURCES "Download non-GPL game assets" ON)
option(OPENSPADES_YSR "Download YSRSpades (closed-source audio backend; macOS only)" ON)

# note that all paths are without trailing slash
set(OPENSPADES_INSTALL_DOC       "share/doc/openspades" CACHE STRING "Directory for installing documentation. ")
set(OPENSPADES_INSTALL_MENU      "share/menu"           CACHE STRING "Directory for installing menu file. " )
set(OPENSPADES_INSTALL_MAN       "share/man/man6"       CACHE STRING "Directory for installing man file. " )
set(OPENSPADES_INSTALL_RESOURCES "share/games/openspades/Resources" CACHE STRING "Directory for installing Resources. ")
set(OPENSPADES_INSTALL_ICONS     "share/icons/hicolor"  CACHE STRING "Directory for installing icons. ")
set(OPENSPADES_INSTALL_PIXMAP    "share/pixmaps"        CACHE STRING "Directory for installing .xpm icon. ")
set(OPENSPADES_INSTALL_BINARY    "games"                CACHE STRING "Directory for installing result binary executable. ")
set(OPENSPADES_INSTALL_DESKTOP   "share/applications"   CACHE STRING "Directory for installing .desktop file. ")
set(OPENSPADES_INSTALL_LIBS      "lib/openspades"       CACHE STRING "Directory for installing various non-media stuff. Currently not used")

set(OPENSPADES_RESDIR "" CACHE STRING "Resource dir to use while developing")
set(OPENSPADES_EXTRATITLE "" CACHE STRING "Extra info to add to the package name")
if(OPENSPADES_RESDIR)
	set(RESDIR ${OPENSPADES_RESDIR})
	set(RESDIR_DEFINED TRUE)
	mark_as_advanced(RESDIR)
	mark_as_advanced(RESDIR_DEFINED)
endif()


if(UNIX)
	EXECUTE_PROCESS(COMMAND "date" "+%Y" OUTPUT_VARIABLE CMAKE_CURRENT_YEAR)
	mark_as_advanced(CMAKE_CURRENT_YEAR)
endif(UNIX)

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
include_directories(BEFORE "${PROJECT_BINARY_DIR}")
if(USE_VCPKG)
	include_directories("${PROJECT_SOURCE_DIR}/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/include")
else()
	include_directories("${SDL2_INCLUDE_DIR}")
	include_directories("${SDL2_IMAGE_INCLUDE_DIR}")
	if(OPENGL_INCLUDE_DIR)
		include_directories("${OPENGL_INCLUDE_DIR}")
	endif()
	if(NOT APPLE)
	include_directories("${GLEW_INCLUDE_DIR}")
	endif()
	include_directories("${ZLIB_INCLUDE_DIR}")
	include_directories(${CURL_INCLUDE_DIRS})
	include_directories(${FREETYPE_INCLUDE_DIRS})
	include_directories(${OpusFile_INCLUDE_DIR})
endif()

add_subdirectory(Resources)
add_subdirectory(Sources)

if(UNIX AND NOT APPLE)
	# various texts
	INSTALL(FILES AUTHORS LICENSE README.md DESTINATION ${OPENSPADES_INSTALL_DOC}/)

	# menu file
	INSTALL(FILES Resources/Unix/Menu/openspades DESTINATION ${OPENSPADES_INSTALL_MENU}/)

	# man file
	INSTALL(FILES ${CMAKE_BINARY_DIR}/Resources/Unix/Man/openspades.6.gz DESTINATION ${OPENSPADES_INSTALL_MAN}/)

	# changelog file
	INSTALL(FILES ${CMAKE_BINARY_DIR}/Resources/Unix/Debian/changelog.gz DESTINATION ${OPENSPADES_INSTALL_DOC}/)

	# copyright file
	INSTALL(FILES ${CMAKE_BINARY_DIR}/Resources/Unix/Debian/copyright DESTINATION ${OPENSPADES_INSTALL_DOC}/)

	if(OPENSPADES_RESOURCES)
		# install asset paks (including non-GPL one)
		install(FILES ${PAK_FILES} DESTINATION ${OPENSPADES_INSTALL_RESOURCES})
	endif(OPENSPADES_RESOURCES)

	# Package info (used for automatic update check)
	INSTALL(FILES Resources/PackageInfo.json DESTINATION ${OPENSPADES_INSTALL_RESOURCES})

	# install icons
	INSTALL(
		DIRECTORY ${CMAKE_BINARY_DIR}/Resources/Icons/hicolor/
		DESTINATION ${OPENSPADES_INSTALL_ICONS}
		FILES_MATCHING PATTERN "*.png")

	INSTALL(FILES Resources/Icons/openspades.xpm DESTINATION ${OPENSPADES_INSTALL_PIXMAP}/)

	# binary file
	INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/bin/openspades DESTINATION ${OPENSPADES_INSTALL_BINARY})

	# desktop file
	INSTALL(FILES Resources/Unix/Desktop/openspades.desktop DESTINATION ${OPENSPADES_INSTALL_DESKTOP}/)

	# Currently there is no files in lib/openspades. But if you reading this message in future and want to
	# place somewhere libs, binaries or other non-media stuff, place them in lib/openspades. Thank you.

endif(UNIX AND NOT APPLE)

if(APPLE)
	# Create a macOS application bundle
	set_target_properties(OpenSpades PROPERTIES
		MACOSX_BUNDLE ON
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/XSpades/XSpades-Info.plist
		MACOSX_BUNDLE_BUNDLE_VERSION ${OPENSPADES_FULL_VERSION}
		MACOSX_BUNDLE_LONG_VERSION_STRING ${OPENSPADES_FULL_VERSION}
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${OPENSPADES_FULL_VERSION}
		MACOSX_BUNDLE_GUI_IDENTIFIER jp.yvt.openspades
		MACOSX_BUNDLE_BUNDLE_NAME OpenSpades)

	set(BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/OpenSpades.icns)
	list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
	if(OPENSPADES_RESOURCES)
		list(APPEND BUNDLE_RESOURCES ${PAK_FILES})
		set_source_files_properties(${PAK_FILES}
			PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		add_dependencies(OpenSpades OpenSpades_Resources_DevPaks OpenSpades_Resources)
	endif(OPENSPADES_RESOURCES)
	list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/PackageInfo.json)
	set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/Resources/PackageInfo.json
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	if (OPENSPADES_YSR)
		list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/libysrspades.dylib)

		# Download `libysrspades.dylib` automatically
		add_dependencies(OpenSpades OpenSpades_YSRSpades_Prebuilt)
		add_custom_target(OpenSpades_YSRSpades_Prebuilt ALL COMMENT "Downloading libysrspades.dylib"
			DEPENDS ${PROJECT_BINARY_DIR}/libysrspades.dylib)
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/libysrspades.dylib
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
			COMMAND
				wget -c
				https://openspadesmedia.yvt.jp/development-packages/YSRSpades/libysrspades.dylib
			)
	endif (OPENSPADES_YSR)

	set_target_properties(OpenSpades PROPERTIES RESOURCE "${BUNDLE_RESOURCES}")
	target_sources(OpenSpades PRIVATE "${BUNDLE_RESOURCES}")
endif(APPLE)

#see http://www.cmake.org/cmake/help/v2.8.11/cpack.html
set(CPACK_PACKAGE_NAME "OpenSpades")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source clone of Ace of Spades")
set(CPACK_PACKAGE_DESCRIPTION "Open-source clone of Ace of Spades - free FPS sandbox game")
set(CPACK_PACKAGE_VENDOR "yvt, assorted")
set(CPACK_PACKAGE_CONTACT "yvt <i@yvt.jp>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "openspades-${OPENSPADES_FULL_VERSION}")
SET(CPACK_PACKAGE_FILE_NAME "openspades-${OPENSPADES_FULL_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenSpades ${OPENSPADES_FULL_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${OpenSpades_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OpenSpades_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OpenSpades_VERSION_REVISION})
SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
SET(CPACK_TOPLEVEL_TAG "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if(WIN32 AND NOT UNIX)

	set(CPACK_NSIS_DISPLAY_NAME "OpenSpades ${OPENSPADES_FULL_VERSION}")
	# There is a bug in NSIS that does not handle full unix paths properly. Make
	# sure there is at least one set of four (4) backlasshes.
	set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\openspades.exe")
	# TODO: set(CPACK_PACKAGE_ICON  "")
	set(CPACK_NSIS_HELP_LINK "https://sites.google.com/a/yvt.jp/openspades/")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://buildandshoot.com/viewforum.php?f=57")
	SET(CPACK_NSIS_CONTACT "i@yvt.jp")
	# TODO: SET(CPACK_NSIS_CREATE_ICONS_EXTRA "")
	# TODO: SET(CPACK_NSIS_DELETE_ICONS_EXTRA "")
	# TODO: SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "openspades.exe")
	set(CPACK_NSIS_MODIFY_PATH ON)

elseif(UNIX AND NOT APPLE)

	# usage: `cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. ; make package`
	INCLUDE(InstallRequiredSystemLibraries)
	set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Open-source clone of Ace of Spades - free FPS sandbox game\n OpenSpades is a clone of Voxlap Ace of Spades 0.75, which is a free \n multiplayer game created by Ben Aksoy.\n .\n Here are some features:\n .\n   - Ability to connect to vanilla/pyspades server\n   - Better performance on some modern computers/netbooks\n   - Sophisticated graphics\n   - Fully open source (GPLv3 compliant)\n   - Cross-platform")
	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Unix/Control/postinst)
	set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
	SET(CPACK_STRIP_FILES "bin/openspades")
	SET(CPACK_SET_DESTDIR "on")
	SET(CPACK_GENERATOR "DEB;TGZ")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS
		"libc6 (>= 2.19), libgcc1 (>= 1:4.9), libstdc++6 (>= 4.8.2), zlib1g (>= 1:1.2.8), libcurl3 (>= 7.35.0), libglew1.10 (>= 1.10.0), libsdl2-2.0-0 (>= 2.0.2), libsdl2-image-2.0-0 (>= 2.0.0), libfreetype6 (>= 2.4.0)") # TODO: add libopus/libopusfile
	SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	SET(CPACK_DEBIAN_PACKAGE_SECTION "games")
	SET(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "")
	set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")
	SET(CPACK_COMPONENTS_ALL ALL)

endif(WIN32 AND NOT UNIX)

set(CPACK_PACKAGE_EXECUTABLES "openspades" "Launch OpenSpades")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)		#everyone hates a dir inside a zip named exactly like the zip
include(CPack)

