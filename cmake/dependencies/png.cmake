#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official source repository https://libpng.sourceforge.io/
#
# Some clone on GitHub https://github.com/glennrp/libpng
#

FIND_PACKAGE(PNG REQUIRED)
IF(TARGET PNG::PNG)
  SET_PROPERTY(
    TARGET PNG::PNG
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()

IF(DEFINED PNG_VERSION)
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION}"
  )
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED PNG_VERSION)
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION}"
  )
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED PNG_VERSION_STRING)
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION_STRING}"
  )
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED PNG_VERSION_STRING)
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION_STRING}"
  )
  SET(RV_DEPS_PNG_VERSION
      "${PNG_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
