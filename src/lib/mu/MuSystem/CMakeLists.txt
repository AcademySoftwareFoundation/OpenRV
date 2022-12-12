#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MuSystem"
)

SET(_sources
    SystemModule.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DFTGL_LIBRARY_STATIC"
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE # NICOLAS: Not really sure why I need to manually set this one should I set '_XOPEN_SOURCE_EXTENDED' instead?
            "-D_POSIX_C_SOURCE=199309"
  )
ENDIF()

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Mu
  PRIVATE MuLang
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

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
