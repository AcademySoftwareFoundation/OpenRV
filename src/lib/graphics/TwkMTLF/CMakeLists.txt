#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# TwkMTLF is macOS-only — the parent CMakeLists guards this with IF(RV_TARGET_DARWIN)

INCLUDE(cxx_defaults)

SET(_target
    "TwkMTLF"
)

SET(_sources
    MetalVideoDevice.mm MetalFBO.mm MetalBuffer.mm MetalSyncObject.mm MetalProgram.mm
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

# Enable Automatic Reference Counting for all Objective-C++ sources.
TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-fobjc-arc"
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkApp TwkFB TwkUtil
  PRIVATE "-framework Metal" "-framework MetalKit" "-framework QuartzCore" "-framework Foundation"
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
