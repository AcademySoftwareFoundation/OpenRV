#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#


IF(NOT RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH)
  # Warning message if the Blackmagic Decklink SDK path is not specified should already have been reported in bmd.cmake
  RETURN()
ENDIF()

SET(_target
    "BlackMagicDevices"
)

SET(_sources
    BlackMagicModule.cpp
    DeckLinkVideoDevice.cpp
    BlackMagicOutput.cpp
    HDRVideoFrame.cpp
    StereoVideoFrame.cpp
    DeckLinkSDKAPIDispatch.cpp
    DeckLinkProfileCallback.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DMSWindows"
  )
ENDIF()

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE BlackmagicDeckLinkSDK Boost::program_options TwkApp TwkGLF TwkUtil
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE "-framework Foundation" "-framework IOKit"
  )
ENDIF()

RV_STAGE(TYPE "OUTPUT_PLUGIN" TARGET ${_target})
