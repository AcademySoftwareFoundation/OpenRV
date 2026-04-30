#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(imgui QUIET)
IF(imgui_FOUND)
  MESSAGE(STATUS "Using system imgui")

  IF(DEFINED imgui_VERSION)
    SET(RV_DEPS_IMGUI_VERSION
        "${imgui_VERSION}"
    )
    SET(RV_DEPS_IMGUI_VERSION
        "${imgui_VERSION}"
        CACHE STRING "" FORCE
    )
  ELSEIF(DEFINED IMGUI_VERSION)
    SET(RV_DEPS_IMGUI_VERSION
        "${IMGUI_VERSION}"
    )
    SET(RV_DEPS_IMGUI_VERSION
        "${IMGUI_VERSION}"
        CACHE STRING "" FORCE
    )
  ELSEIF(DEFINED imgui_VERSION_STRING)
    SET(RV_DEPS_IMGUI_VERSION
        "${imgui_VERSION_STRING}"
    )
    SET(RV_DEPS_IMGUI_VERSION
        "${imgui_VERSION_STRING}"
        CACHE STRING "" FORCE
    )
  ELSEIF(DEFINED IMGUI_VERSION_STRING)
    SET(RV_DEPS_IMGUI_VERSION
        "${IMGUI_VERSION_STRING}"
    )
    SET(RV_DEPS_IMGUI_VERSION
        "${IMGUI_VERSION_STRING}"
        CACHE STRING "" FORCE
    )
  ENDIF()
ENDIF()
