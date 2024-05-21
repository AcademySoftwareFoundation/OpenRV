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

# Common options
ADD_COMPILE_OPTIONS(
  ${_verbose_invocation}
  -Wall
  -Wnonportable-include-path
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
