
FIND_PATH(XEXT_INCLUDE_DIR NAMES X11/extensions/Xext.h
	/usr/include
	/usr/local/include
	/sw/include
	/opt/local/include
	DOC "The directory where Xext.h resides")
FIND_LIBRARY( XEXT_LIBRARY
	NAMES XEXT Xext
	PATHS
	/usr/lib64
	/usr/lib
	/usr/local/lib64
	/usr/local/lib
	/sw/lib
	/opt/local/lib
	DOC "The Xext library")

IF (XEXT_INCLUDE_DIR)
	SET( XEXT_FOUND 1 CACHE STRING "Set to 1 if XEXT is found, 0 otherwise")
ELSE (XEXT_INCLUDE_DIR)
	SET( XEXT_FOUND 0 CACHE STRING "Set to 1 if XEXT is found, 0 otherwise")
ENDIF (XEXT_INCLUDE_DIR)

MARK_AS_ADVANCED( XEXT_FOUND )
