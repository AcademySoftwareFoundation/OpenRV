#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "LoadingSharedLibrariesTest"
)

LIST(APPEND _sources main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/lib/base ${PROJECT_SOURCE_DIR}/src/lib/image
)

TARGET_LINK_LIBRARIES(
  ${_target}
  RvApp
  IPCore
  IPBaseNodes
  TwkUtil
  dl
  doctest::doctest
)

# Simply assert that the test executable actually works.
ADD_TEST(
  NAME "${_target} - ${_shared_library}"
  COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${RV_STAGE_LIB_DIR}:${RV_STAGE_LIB_DIR}/OpenSSL "$<TARGET_FILE:${_target}>"
)

FOREACH(
  _shared_library
  ${_shared_libraries}
)
  ADD_TEST(
    NAME "${_target} - ${_shared_library}"
    COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${RV_STAGE_LIB_DIR}:${RV_STAGE_LIB_DIR}/OpenSSL $<TARGET_FILE:${_target}> --library
            $<TARGET_FILE:${_shared_library}>
  )
ENDFOREACH()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
