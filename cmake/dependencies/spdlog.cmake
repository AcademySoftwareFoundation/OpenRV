#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(spdlog REQUIRED)
IF(TARGET spdlog::spdlog)
  SET_PROPERTY(
    TARGET spdlog::spdlog
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
IF(TARGET spdlog::spdlog_header_only)
  SET_PROPERTY(
    TARGET spdlog::spdlog_header_only
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()

IF(DEFINED spdlog_VERSION)
  SET(RV_DEPS_SPDLOG_VERSION
      "${spdlog_VERSION}"
  )
  SET(RV_DEPS_SPDLOG_VERSION
      "${spdlog_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED SPDLOG_VERSION)
  SET(RV_DEPS_SPDLOG_VERSION
      "${SPDLOG_VERSION}"
  )
  SET(RV_DEPS_SPDLOG_VERSION
      "${SPDLOG_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED spdlog_VERSION_STRING)
  SET(RV_DEPS_SPDLOG_VERSION
      "${spdlog_VERSION_STRING}"
  )
  SET(RV_DEPS_SPDLOG_VERSION
      "${spdlog_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED SPDLOG_VERSION_STRING)
  SET(RV_DEPS_SPDLOG_VERSION
      "${SPDLOG_VERSION_STRING}"
  )
  SET(RV_DEPS_SPDLOG_VERSION
      "${SPDLOG_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
