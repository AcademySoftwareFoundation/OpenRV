#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "RvPackage"
)

SET(_sources
    PackageManager.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Qt5::Core
  PRIVATE TwkDeploy
          TwkUtil
          TwkQtCoreUtil
          yaml
          stl_ext
          TwkApp
          minizip
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DINTERNAL_APPLICATION_NAME=\"${RV_INTERNAL_APPLICATION_NAME}\" -DINTERNAL_ORGANIZATION_NAME=\"${RV_INTERNAL_ORGANIZATION_NAME}\"
          -DINTERNAL_ORGANIZATION_DOMAIN=\"${RV_INTERNAL_ORGANIZATION_DOMAIN}\"
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
