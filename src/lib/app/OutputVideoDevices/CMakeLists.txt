#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "OutputVideoDevices"
)

SET(_sources
    OutputVideoDevice.cpp OutputVideoDeviceModule.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkApp TwkGLF TwkUtil TwkMovie stl_ext Boost::thread
  PRIVATE TwkAudio IPCore TwkExc
)

IF(RV_TARGET_LINUX)
  FIND_PACKAGE(OpenGL REQUIRED)

  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC OpenGL::GL OpenGL::GLU OpenGL::GLX
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC win_pthreads glew
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
