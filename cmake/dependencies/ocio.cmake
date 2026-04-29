#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Build instructions: https://opencolorio.readthedocs.io/en/latest/quick_start/installation.html#building-from-source
#

FIND_PACKAGE(OpenColorIO REQUIRED)
IF(TARGET OpenColorIO::OpenColorIO)
  SET_PROPERTY(
    TARGET OpenColorIO::OpenColorIO
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
# Map to internal target name
IF(NOT TARGET OpenColorIO::OpenColorIO)
  ADD_LIBRARY(OpenColorIO::OpenColorIO INTERFACE IMPORTED GLOBAL)
  TARGET_LINK_LIBRARIES(
    OpenColorIO::OpenColorIO
    INTERFACE OpenColorIO::OpenColorIO
  )
ENDIF()

IF(DEFINED OpenColorIO_VERSION)
  SET(RV_DEPS_OCIO_VERSION
      "${OpenColorIO_VERSION}"
  )
  SET(RV_DEPS_OCIO_VERSION
      "${OpenColorIO_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED OPENCOLORIO_VERSION)
  SET(RV_DEPS_OCIO_VERSION
      "${OPENCOLORIO_VERSION}"
  )
  SET(RV_DEPS_OCIO_VERSION
      "${OPENCOLORIO_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED OpenColorIO_VERSION_STRING)
  SET(RV_DEPS_OCIO_VERSION
      "${OpenColorIO_VERSION_STRING}"
  )
  SET(RV_DEPS_OCIO_VERSION
      "${OpenColorIO_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED OPENCOLORIO_VERSION_STRING)
  SET(RV_DEPS_OCIO_VERSION
      "${OPENCOLORIO_VERSION_STRING}"
  )
  SET(RV_DEPS_OCIO_VERSION
      "${OPENCOLORIO_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
