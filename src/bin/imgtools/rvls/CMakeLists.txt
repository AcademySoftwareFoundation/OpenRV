#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rvls"
)

LIST(APPEND _sources main.cpp utf8Main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
  REQUIRED
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC MuTwkApp
  PRIVATE IOproxy
          OpenEXR::OpenEXR
          MovieFB
          TwkFB
          MovieProcedural
          MovieProxy
          TwkDeploy
          TwkMovie
          TwkContainer
          TwkUtil
          arg
          stl_ext
          Boost::headers
          yaml_cpp
          Qt5::Core
          PyTwkApp
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE DarwinBundle
  )
ELSE()
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE QTBundle
  )
ENDIF()

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"" "-DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\""
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
