#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "IPBaseNodes"
)

SET(_nodes
    CacheLUT
    Retime
    StackGroup
    ChannelMap
    Look
    SequenceGroup
    Stack
    ImageSource
    Overlay
    Sequence
    SwitchGroup
    Color
    Paint
    SourceGroup
    Switch
    FileSource
    LayoutGroup
    Source
    FolderGroup
    Linearize
    RetimeGroup
    SourceStereo
    LensWarp
    Crop
    RotateCanvas
    PrimaryConvert
    FileOutputGroup
    UnsharpMask
    FilterGaussian
    Clarity
    NoiseReduction
    ColorTemperature
    ColorExposure
    ColorSaturation
    ColorCurve
    ColorHighlight
    ColorShadow
    ColorVibrance
    ColorLinearToSRGB
    ColorSRGBToLinear
    ColorGrayScale
    ColorCDL
    YCToRGB
    AudioAdd
)
# Append the 'IPNode' suffix to all of the elements
LIST(
  TRANSFORM _nodes
  APPEND IPNode.cpp
)

SET(_sources
    ${_nodes} BaseDefinitions.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkApp TwkFB TwkMovie Qt5::Core
  PRIVATE CDL
          IPCore
          OpenEXR::OpenEXR
          TwkContainer
          TwkFBAux
          TwkMath
          TwkMediaLibrary
          TwkUtil
          stl_ext
          Boost::filesystem
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC glew nedmalloc win_posix
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-wd4355"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
