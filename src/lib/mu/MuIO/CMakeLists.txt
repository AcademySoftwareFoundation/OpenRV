#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MuIO"
)

SET(_sources
    IOModule.cpp
    StreamType.cpp
    OStreamType.cpp
    OFStreamType.cpp
    IStreamType.cpp
    IFStreamType.cpp
    OSStreamType.cpp
    ISStreamType.cpp
    exec-stream.cpp
    ProcessType.cpp
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
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DUNICODE" "-D_POSIX_" "-D_UNICODE"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE win_posix
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
