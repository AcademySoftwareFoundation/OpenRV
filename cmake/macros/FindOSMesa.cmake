# Try to find Mesa off-screen library and include dir. Once done this will define
#
# OSMesa_FOUND        - true if OSMesa has been found OSMesa_INCLUDE_DIRS - where the GL/osmesa.h can be found OSMesa_LIBRARIES    - Link this to use OSMesa
# OSMesa_VERSION      - Version of OSMesa found OSMesa::OSMesa      - Imported target

FIND_PATH(
  OSMESA_INCLUDE_DIR
  NAMES GL/osmesa.h
  PATHS "${OSMESA_ROOT}/include" "$ENV{OSMESA_ROOT}/include" /usr/openwin/share/include /opt/graphics/OpenGL/include
  DOC "OSMesa include directory"
)
MARK_AS_ADVANCED(OSMESA_INCLUDE_DIR)

FIND_LIBRARY(
  OSMESA_LIBRARY
  NAMES OSMesa OSMesa16 OSMesa32
  PATHS "${OSMESA_ROOT}/lib" "$ENV{OSMESA_ROOT}/lib" /opt/graphics/OpenGL/lib /usr/openwin/lib
  DOC "OSMesa library"
)
MARK_AS_ADVANCED(OSMESA_LIBRARY)

IF(OSMESA_INCLUDE_DIR
   AND EXISTS "${OSMESA_INCLUDE_DIR}/GL/osmesa.h"
)
  FILE(
    STRINGS "${OSMESA_INCLUDE_DIR}/GL/osmesa.h" _OSMesa_version_lines
    REGEX "OSMESA_[A-Z]+_VERSION"
  )
  STRING(
    REGEX
    REPLACE ".*# *define +OSMESA_MAJOR_VERSION +([0-9]+).*" "\\1" _OSMesa_version_major "${_OSMesa_version_lines}"
  )
  STRING(
    REGEX
    REPLACE ".*# *define +OSMESA_MINOR_VERSION +([0-9]+).*" "\\1" _OSMesa_version_minor "${_OSMesa_version_lines}"
  )
  STRING(
    REGEX
    REPLACE ".*# *define +OSMESA_PATCH_VERSION +([0-9]+).*" "\\1" _OSMesa_version_patch "${_OSMesa_version_lines}"
  )
  SET(OSMesa_VERSION
      "${_OSMesa_version_major}.${_OSMesa_version_minor}.${_OSMesa_version_patch}"
  )
  UNSET(_OSMesa_version_major)
  UNSET(_OSMesa_version_minor)
  UNSET(_OSMesa_version_patch)
  UNSET(_OSMesa_version_lines)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  OSMesa
  REQUIRED_VARS OSMESA_INCLUDE_DIR OSMESA_LIBRARY
  VERSION_VAR OSMesa_VERSION
)

IF(OSMesa_FOUND)
  SET(OSMesa_INCLUDE_DIRS
      "${OSMESA_INCLUDE_DIR}"
  )
  SET(OSMesa_LIBRARIES
      "${OSMESA_LIBRARY}"
  )

  IF(NOT TARGET OSMesa::OSMesa)
    ADD_LIBRARY(OSMesa::OSMesa UNKNOWN IMPORTED)
    SET_TARGET_PROPERTIES(
      OSMesa::OSMesa
      PROPERTIES IMPORTED_LOCATION "${OSMESA_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${OSMESA_INCLUDE_DIR}"
    )
  ENDIF()
ENDIF()