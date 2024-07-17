#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "AJADevices"
)

SET(_sources
    AJAModule.cpp KonaVideoDevice.cpp AJAOutput.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE ${RV_DEPS_AJA_COMPILE_OPTIONS}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE aja::ntv2 Boost::program_options Boost::thread TwkApp TwkGLF TwkUtil
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE "-framework Foundation" "-framework IOKit"
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE winmm Setupapi Netapi32 Shlwapi
  )
ENDIF()

RV_STAGE(TYPE "OUTPUT_PLUGIN" TARGET ${_target})
