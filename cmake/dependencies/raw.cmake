#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# LibRaw official Web page: https://www.libraw.org/about LibRaw official sources:  https://www.libraw.org/data/LibRaw-0.21.1.tar.gz LibRaw build from sources:
# https://www.libraw.org/docs/Install-LibRaw-eng.html
#

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(LIBRAW REQUIRED libraw)
IF(TARGET PkgConfig::LIBRAW)
  SET_PROPERTY(
    TARGET PkgConfig::LIBRAW
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
IF(NOT TARGET LibRaw::raw)
  ADD_LIBRARY(LibRaw::raw INTERFACE IMPORTED GLOBAL)
  FIND_LIBRARY(
    LIBRAW_LIB_PATH
    NAMES raw libraw
    PATHS ${LIBRAW_LIBRARY_DIRS} ${LIBRAW_LIBDIR}
  )
  TARGET_LINK_LIBRARIES(
    LibRaw::raw
    INTERFACE ${LIBRAW_LIB_PATH}
  )
  TARGET_INCLUDE_DIRECTORIES(
    LibRaw::raw
    INTERFACE ${LIBRAW_INCLUDE_DIRS}
  )
ENDIF()

IF(DEFINED PkgConfig_VERSION)
  SET(RV_DEPS_RAW_VERSION
      "${PkgConfig_VERSION}"
  )
  SET(RV_DEPS_RAW_VERSION
      "${PkgConfig_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED PKGCONFIG_VERSION)
  SET(RV_DEPS_RAW_VERSION
      "${PKGCONFIG_VERSION}"
  )
  SET(RV_DEPS_RAW_VERSION
      "${PKGCONFIG_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED PkgConfig_VERSION_STRING)
  SET(RV_DEPS_RAW_VERSION
      "${PkgConfig_VERSION_STRING}"
  )
  SET(RV_DEPS_RAW_VERSION
      "${PkgConfig_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED PKGCONFIG_VERSION_STRING)
  SET(RV_DEPS_RAW_VERSION
      "${PKGCONFIG_VERSION_STRING}"
  )
  SET(RV_DEPS_RAW_VERSION
      "${PKGCONFIG_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
