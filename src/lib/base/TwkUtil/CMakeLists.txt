#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkUtil"
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
  REQUIRED
)

SET(_sources
    TwkRegEx.cpp
    Timer.cpp
    FileSequence.cpp
    Str.cpp
    RegexGlob.cpp
    Notifier.cpp
    PathConform.cpp
    EnvVar.cpp
    File.cpp
    Daemon.cpp
    FrameUtils.cpp
    SystemInfo.cpp
    StdioBuf.cpp
    FileMMap.cpp
    FileStream.cpp
    User.cpp
    ProcessInfo.cpp
    ThreadName.cpp
    FourCC.cpp
    Timecode.cpp
    Base64.cpp
    MemPool.cpp
    FNV1a.cpp
    Log.cpp
    Clock.cpp
    sgcHopImplementation.cpp
    sgcHopTools.cpp
    sgcJobDispatcher.cpp
    sgcRefCounted.cpp
    FileLogger.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Boost::thread TwkExc TwkMath spdlog::spdlog
  PRIVATE Boost::system Qt5::Core stl_ext
)

IF(RV_TARGET_LINUX)
  SET(THREADS_PREFER_PTHREAD_FLAG
      TRUE
  )
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC Threads::Threads
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC win_pthreads win_posix
  )
ENDIF()

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE aio
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE nedmalloc win_posix win_pthreads TwkQtCoreUtil
    PUBLIC pcre2-8 pcre2-posix
  )
ENDIF()

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-D_GNU_SOURCE"
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DTWKUTIL_BUILD"
  )
ENDIF()

RV_STAGE(TYPE "SHARED_LIBRARY" TARGET ${_target})
