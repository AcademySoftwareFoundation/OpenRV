#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [libtiff 3.6 -- Old libtiff Webpage](http://www.libtiff.org/)
#
# [libtiff 3.9-4.5](https://download.osgeo.org/libtiff/)
#
# [libtiff 4.4](https://conan.io/center/libtiff)
#
# [libtiff 4.5](http://www.simplesystems.org/libtiff)
#

# OpenImageIO required >= 3.9, using latest 4.0
FIND_PACKAGE(TIFF REQUIRED)
IF(TARGET TIFF::TIFF)
  SET_PROPERTY(
    TARGET TIFF::TIFF
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()

IF(DEFINED TIFF_VERSION)
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION}"
  )
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED TIFF_VERSION)
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION}"
  )
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED TIFF_VERSION_STRING)
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION_STRING}"
  )
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED TIFF_VERSION_STRING)
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION_STRING}"
  )
  SET(RV_DEPS_TIFF_VERSION
      "${TIFF_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
