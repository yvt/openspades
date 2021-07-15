FIND_PATH(Ogg_INCLUDE_DIR ogg/ogg.h
  HINTS
  $ENV{OGGDIR}
  PATH_SUFFIXES include
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

FIND_LIBRARY(Ogg_LIBRARY
  NAMES ogg ogg_static
  HINTS
  $ENV{OGGDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ogg
                                  REQUIRED_VARS Ogg_LIBRARY Ogg_INCLUDE_DIR)

