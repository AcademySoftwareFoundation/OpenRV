#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "stl_ext"
)

SET(_sources
    hash_table.cpp
    string_hash_table.cpp
    string_algo.cpp
    thread_group.cpp
    fixed_block_allocator.cpp
    block_alloc_arena.cpp
    fixed_block_arena.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC nedmalloc rt
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC nedmalloc win_pthreads
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DSTL_EXT_BUILD=1"
  )
ENDIF()

RV_STAGE(TYPE "SHARED_LIBRARY" TARGET ${_target})
