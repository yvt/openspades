#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([OpenSpades], [0.0.4], [i@yvt.jp])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_CONFIG_SRCDIR([Sources/Audio/ALDevice.cpp])
# AC_CONFIG_HEADERS([BuildConfig.h])
AC_CANONICAL_HOST

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

#SDL_CFLAGS=""
#SDL_LIBS="-lSDL"

GL_CFLAGS=""
GL_LIBS="-lGL"

GLEW_CFLAGS=""
GLEW_LIBS=""

if test "$prefix" = "NONE"; then
	FLTK_CONFIG="fltk-config"
else
	FLTK_CONFIG="${prefix}/bin/fltk-config"
fi

FLTK_CFLAGS=`${FLTK_CONFIG} --cxxflags --use-images`
FLTK_LIBS=`${FLTK_CONFIG} --ldstaticflags --use-images`

case $host_os in
  darwin* )
        GL_CFLAGS="-I/usr/include" # ??
        GL_LIBS="-framework OpenGL"
        SDL_LIBS="-framework SDL"
		SDL_CFLAGS="-I/Library/Frameworks/SDL.framework/Headers"
		CC="clang -arch i386"
		CXX="clang++ -arch i386"
	    ZLIB_LIBS="-lz"
	    ZLIB_CFLAGS="-I/usr/include" # ??
        ;;
  windows*|mingw*|winnt)
		GL_LIBS="-lopengl32"
		WIN32_LIBS="-lwsock32 -lws2_32 -lwinmm"
		PKG_CHECK_MODULES(GLEW, glew)
		;;
 * )
		PKG_CHECK_MODULES(GLEW, glew)
		;;
esac


PKG_CHECK_MODULES(SDL, sdl)
PKG_CHECK_MODULES(GL, gl)
PKG_CHECK_MODULES(FLTK, fltk)
PKG_CHECK_MODULES(ZLIB, zlib)
# PKG_CHECK_MODULES(PNG, libpng)

AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)

AC_SUBST(GLEW_CFLAGS)
AC_SUBST(GLEW_LIBS)

AC_SUBST(FLTK_CFLAGS)
AC_SUBST(FLTK_LIBS)

AC_SUBST(WIN32_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T


# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

# library function (enet)
AC_CHECK_FUNC(gethostbyaddr_r, [AC_DEFINE(HAS_GETHOSTBYADDR_R)])
AC_CHECK_FUNC(gethostbyname_r, [AC_DEFINE(HAS_GETHOSTBYNAME_R)])
AC_CHECK_FUNC(poll, [AC_DEFINE(HAS_POLL)])
AC_CHECK_FUNC(fcntl, [AC_DEFINE(HAS_FCNTL)])
AC_CHECK_FUNC(inet_pton, [AC_DEFINE(HAS_INET_PTON)])
AC_CHECK_FUNC(inet_ntop, [AC_DEFINE(HAS_INET_NTOP)])

AC_CHECK_MEMBER(struct msghdr.msg_flags, [AC_DEFINE(HAS_MSGHDR_FLAGS)], , [#include <sys/socket.h>])

AC_CHECK_TYPE(socklen_t, [AC_DEFINE(HAS_SOCKLEN_T)], , 
              #include <sys/types.h>
              #include <sys/socket.h>
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
