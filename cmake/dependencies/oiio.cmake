#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [OIIO -- Sources](https://github.com/OpenImageIO/oiio)
#
# [OIIO -- Documentation](https://openimageio.readthedocs.io/en/v${VERSION)NUMBER}/)
#
# [OIIO -- Build instructions](https://github.com/OpenImageIO/oiio/blob/master/INSTALL.md)
#

FIND_PACKAGE(OpenImageIO REQUIRED)
IF(TARGET OpenImageIO::OpenImageIO)
  SET_PROPERTY(
    TARGET OpenImageIO::OpenImageIO
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
IF(TARGET OpenImageIO::OpenImageIO_Util)
  SET_PROPERTY(
    TARGET OpenImageIO::OpenImageIO_Util
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ELSEIF(TARGET OpenImageIO::OpenImageIO)
  # Map _Util to main target if missing
  ADD_LIBRARY(OpenImageIO::OpenImageIO_Util INTERFACE IMPORTED GLOBAL)
  TARGET_LINK_LIBRARIES(
    OpenImageIO::OpenImageIO_Util
    INTERFACE OpenImageIO::OpenImageIO
  )
ENDIF()

IF(DEFINED OpenImageIO_VERSION)
  SET(RV_DEPS_OIIO_VERSION
      "${OpenImageIO_VERSION}"
  )
  SET(RV_DEPS_OIIO_VERSION
      "${OpenImageIO_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED OPENIMAGEIO_VERSION)
  SET(RV_DEPS_OIIO_VERSION
      "${OPENIMAGEIO_VERSION}"
  )
  SET(RV_DEPS_OIIO_VERSION
      "${OPENIMAGEIO_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED OpenImageIO_VERSION_STRING)
  SET(RV_DEPS_OIIO_VERSION
      "${OpenImageIO_VERSION_STRING}"
  )
  SET(RV_DEPS_OIIO_VERSION
      "${OpenImageIO_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED OPENIMAGEIO_VERSION_STRING)
  SET(RV_DEPS_OIIO_VERSION
      "${OPENIMAGEIO_VERSION_STRING}"
  )
  SET(RV_DEPS_OIIO_VERSION
      "${OPENIMAGEIO_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
