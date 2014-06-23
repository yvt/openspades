# find ODE (Open Dynamics Engine) includes and library
#
# ODE_INCLUDE_DIR - where the directory containing the ODE headers can be
#                   found
# ODE_LIBRARY     - full path to the ODE library
# ODE_FOUND       - TRUE if ODE was found

IF (NOT ODE_FOUND)

  FIND_PATH(ODE_INCLUDE_DIR ode/ode.h
    /usr/include
    /usr/local/include
    $ENV{OGRE_HOME}/include # OGRE SDK on WIN32
    $ENV{INCLUDE}
  )
  FIND_LIBRARY(ODE_LIBRARY
    NAMES ode
    PATHS
    /usr/lib
    /usr/local/lib
    $ENV{OGRE_HOME}/lib # OGRE SDK on WIN32
  )

  IF(ODE_INCLUDE_DIR)
    MESSAGE(STATUS "Found ODE include dir: ${ODE_INCLUDE_DIR}")
  ELSE(ODE_INCLUDE_DIR)
    MESSAGE(STATUS "Could NOT find ODE headers.")
  ENDIF(ODE_INCLUDE_DIR)

  IF(ODE_LIBRARY)
    MESSAGE(STATUS "Found ODE library: ${ODE_LIBRARY}")
  ELSE(ODE_LIBRARY)
    MESSAGE(STATUS "Could NOT find ODE library.")
  ENDIF(ODE_LIBRARY)

  IF(ODE_INCLUDE_DIR AND ODE_LIBRARY)
     SET(ODE_FOUND TRUE CACHE STRING "Whether ODE was found or not")
   ELSE(ODE_INCLUDE_DIR AND ODE_LIBRARY)
     SET(ODE_FOUND FALSE)
     IF(ODE_FIND_REQUIRED)
       MESSAGE(FATAL_ERROR "Could not find ODE. Please install ODE (http://www.ode.org)")
     ENDIF(ODE_FIND_REQUIRED)
   ENDIF(ODE_INCLUDE_DIR AND ODE_LIBRARY)
ENDIF (NOT ODE_FOUND)

# vim: et sw=4 ts=4