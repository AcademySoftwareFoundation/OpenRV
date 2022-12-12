#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
IF(RV_VERBOSE_INVOCATION)
  SET(_verbose_invocation
      "-v"
  )
ELSE()
  SET(_verbose_invocation
      ""
  )
ENDIF()

#
# Ref.: https://clang.llvm.org/docs/CommandGuide/clang.html
IF(${RV_CPP_STANDARD} STREQUAL "14")
  # Why not CMake's default gnu++14 ???
  SET(_clang_cxx_standard
      "c++14"
  )
ELSEIF(${RV_CPP_STANDARD} STREQUAL "17")
  # Why not CMake's default gnu++17 ???
  SET(_clang_cxx_standard
      "c++17"
  )
ELSE()
  MESSAGE(FATAL_ERROR "Unexpected RV_CPP_STANDARD: '${RV_CPP_STANDARD}'")
ENDIF()

# Common options
ADD_COMPILE_OPTIONS(
  ${_verbose_invocation}
  -Wall
  -Wnonportable-include-path
  -stdlib=libc++
  # not sure we need to actually set '-std' I would assume CMake does it for us.
  -std=${_clang_cxx_standard}
  -msse
  -msse2
  -msse3
  -mmmx
)

IF(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  # Release build specific options
  ADD_COMPILE_OPTIONS(-DNDEBUG -O3 # Maximum optimization
  )
ELSEIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  # Debug build specific options
  ADD_COMPILE_OPTIONS(
    -DDEBUG -g # Generate debugging information
    -O0 # No optimization
  )
ENDIF()
