#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "ApplicationTest"
)

LIST(APPEND _sources main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

TARGET_LINK_LIBRARIES(${_target} TwkUtil doctest::doctest RvApp)

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

RV_STAGE(TYPE "EXECUTABLE" TARGET ${_target})
