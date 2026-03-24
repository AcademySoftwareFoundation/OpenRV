#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "AudioRendererTest"
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

TARGET_LINK_LIBRARIES(${_target} TwkUtil doctest::doctest IPCore RvApp Mu)

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(${_target} pthread dl)
ENDIF()

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(${_target} "-framework OpenCL" "-framework OpenGL" # "-framework IOKit" "-framework QuartzCore" "-framework AppKit"
  )
ENDIF()

# Simply assert that the test executable actually works.
ADD_TEST(
  NAME "${_target} - ${_shared_library}"
  COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${RV_STAGE_LIB_DIR}:${RV_STAGE_LIB_DIR}/OpenSSL "$<TARGET_FILE:${_target}>"
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
