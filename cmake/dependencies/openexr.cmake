#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(Imath REQUIRED)
FIND_PACKAGE(OpenEXR REQUIRED)

# Promote targets to GLOBAL so they are visible everywhere
FOREACH(
  _tgt
  OpenEXR::OpenEXR OpenEXR::Iex OpenEXR::IlmThread OpenEXR::OpenEXRCore
)
  IF(TARGET ${_tgt})
    SET_PROPERTY(
      TARGET ${_tgt}
      PROPERTY IMPORTED_GLOBAL TRUE
    )
    LIST(APPEND RV_DEPS_LIST ${_tgt})
  ENDIF()
ENDFOREACH()

# Make sure RV_DEPS_LIST is updated in parent scope if needed, though CACHE INTERNAL is used at the end of CMakeLists.txt
SET(RV_DEPS_LIST
    ${RV_DEPS_LIST}
    CACHE INTERNAL ""
)

IF(DEFINED Imath_VERSION)
  SET(RV_DEPS_OPENEXR_VERSION
      "${Imath_VERSION}"
  )
  SET(RV_DEPS_OPENEXR_VERSION
      "${Imath_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED IMATH_VERSION)
  SET(RV_DEPS_OPENEXR_VERSION
      "${IMATH_VERSION}"
  )
  SET(RV_DEPS_OPENEXR_VERSION
      "${IMATH_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED Imath_VERSION_STRING)
  SET(RV_DEPS_OPENEXR_VERSION
      "${Imath_VERSION_STRING}"
  )
  SET(RV_DEPS_OPENEXR_VERSION
      "${Imath_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED IMATH_VERSION_STRING)
  SET(RV_DEPS_OPENEXR_VERSION
      "${IMATH_VERSION_STRING}"
  )
  SET(RV_DEPS_OPENEXR_VERSION
      "${IMATH_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
