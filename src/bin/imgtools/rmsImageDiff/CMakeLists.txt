#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rmsImageDiff"
)

LIST(APPEND _sources main.cpp utf8Main.cpp)

IF(RV_TARGET_DARWIN)
  LIST(APPEND _sources pool.mm)
ENDIF()

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE IOproxy
          MovieProxy
          MovieProcedural
          MovieFB
          TwkMovie
          TwkExc
          TwkFB
          TwkMath
          TwkUtil
          arg
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE DarwinBundle "-framework Foundation"
  )
ELSE()
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE QTBundle
  )
ENDIF()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
