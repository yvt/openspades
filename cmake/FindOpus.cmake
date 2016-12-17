FIND_PATH(Opus_INCLUDE_DIR opus.h
  HINTS
  $ENV{OPUSDIR}
  PATH_SUFFIXES include/opus include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include/SDL2
  /usr/include/SDL2
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(Opus_LIBRARY
  NAMES opus
  HINTS
  $ENV{OPUSDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
)

FIND_LIBRARY(OpusFile_LIBRARY
  NAMES opusfile
  HINTS
  $ENV{OPUSDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
)

set(Opus_LIBRARIES ${Opus_LIBRARY} ${OpusFile_LIBRARY})

set(Opus_FOUND "NO")
if(Opus_INCLUDE_DIR AND Opus_LIBRARY AND OpusFile_LIBRARY)
  set(Opus_FOUND "YES")
endif()

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Opus
                                  REQUIRED_VARS Opus_LIBRARY OpusFile_LIBRARY Opus_INCLUDE_DIR)

