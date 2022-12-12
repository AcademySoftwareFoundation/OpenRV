#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "FastMemcpyTest"
)

LIST(APPEND _sources TestFastMemcpy.cpp main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE TwkFB TwkUtil
)

ADD_TEST(
  NAME ${_target}
  COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${RV_STAGE_LIB_DIR} "$<TARGET_FILE:${_target}>"
)

RV_STAGE(TYPE "EXECUTABLE" TARGET ${_target})
