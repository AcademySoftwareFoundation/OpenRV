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

IF(NOT RV_TARGET_APPLE_ARM64)
  SET(__compiler_options_x86_64 -msse -msse2 -msse3 -mmmx)
ENDIF()

SET(__compiler_options__
  -Wall ${__compiler_options_x86_64}
)

# Common options
ADD_COMPILE_OPTIONS(
  ${_verbose_invocation}
  ${_compiler_options__}
  -Wnonportable-include-path
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
