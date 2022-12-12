#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkGLText"
)

SET(_sources
    GLT.cpp TwkGLText.cpp defaultFont.cpp
)

FIND_PACKAGE(OpenGL REQUIRED)

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
  PUBLIC FTGL TwkGLF TwkExc TwkMath
  PRIVATE stl_ext OpenGL::GL OpenGL::GLU TwkUtil utf8
)

IF(RV_TARGET_LINUX)
  SET(THREADS_PREFER_PTHREAD_FLAG
      TRUE
  )
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Threads::Threads
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE win_pthreads win_posix
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE glew
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DFTGL_LIBRARY_STATIC"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
