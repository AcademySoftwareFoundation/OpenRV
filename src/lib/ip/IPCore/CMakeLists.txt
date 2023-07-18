#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)
INCLUDE(rv_quote_file)

FIND_PACKAGE(
  Qt5
  COMPONENTS Widgets
  REQUIRED
)

SET(CMAKE_AUTOUIC
    ON
)
SET(CMAKE_AUTOMOC
    ON
)
SET(CMAKE_AUTORCC
    ON
)

SET(_target
    "IPCore"
)

SET(_sources
    Session.cpp
    Exception.cpp
    IPNode.cpp
    Application.cpp
    DefaultMode.cpp
    ShaderSymbol.cpp
    ShaderFunction.cpp
    ShaderExpression.cpp
    ShaderProgram.cpp
    ShaderCommon.cpp
    ShaderUtil.cpp
    IPImage.cpp
    ImageFBO.cpp
    FBCache.cpp
    ShaderValues.cpp
    IPGraph.cpp
    PaintCommand.cpp
    NodeDefinition.cpp
    NodeManager.cpp
    IPInstanceNode.cpp
    IPInstanceGroupNode.cpp
    Profile.cpp
    CoreDefinitions.cpp
    RenderQuery.cpp
)

# TODO: Find out whether ALL files are used and replace with a *.glsl glob ???
SET(_shaders
    SourceRGBA
    SourceBGRA
    SourcePlanarRGB
    SourcePlanarRGBA
    SourceBGRAUncrop
    SourceRGBAUncrop
    SourceYVYU
    SourcePlanarRGBUncrop
    SourcePlanarRGBAUncrop
    SourcePlanarYUV
    SourcePlanarYUVUncrop
    SourcePlanarYUVA
    SourcePlanarYUVAUncrop
    SourcePlanarYRYBY
    SourcePlanar2YUV
    SourcePlanarYRYBYUncrop
    SourcePlanarYRYBYA
    SourcePlanarYRYBYAUncrop
    SourceY
    SourceYA
    SourceYUncrop
    SourceYAUncrop
    SourcePlanarYAC2
    SourcePlanarYAC2Uncrop
    SourcePlanarYA
    SourceYUVA
    SourceAYUV
    SourcePlanarYAUncrop
    ColorTemperatureOffset
    ColorSRGBLinear
    ColorRec709Linear
    ColorQuantize
    ColorBlendWithConstant
    ColorPremult
    ColorGamma
    ColorMatrix
    ColorLinearCineonLog
    ColorCineonLogLinear
    ColorCineonSoftClipLogLinear
    ColorLogCLinear
    ColorLinearLogC
    ColorViperLogLinear
    ColorLinearRedLog
    ColorRedLogLinear
    ColorUnpremult
    ColorLinearSRGB
    ColorLinearRec709
    Color3DLUT
    Color3DLUTGLSampling
    ColorACESLogLinear
    ColorLinearACESLog
    ColorChannelLUT
    ColorCDL
    ColorCDL_SAT
    ColorCDLForACESLinear
    ColorCDLForACESLog
    ColorSMPTE240MLinear
    ColorLinearSMPTE240M
    ColorLuminanceLUT
    ColorOutOfRange
    InlineBoxResize
    Dither
    InlineAdaptiveBoxResize
    StencilBox
    StencilBoxNegAlpha
    BoxFilter
    StereoScanline
    StereoChecker
    StereoAnaglyph
    StereoLumAnaglyph
    Over2
    Over3
    Over4
    Add2
    Add3
    Add4
    Replace2
    Replace3
    Replace4
    Difference2
    Difference3
    ResizeDownSample
    ResizeDownSampleFast
    ResizeDownSampleDerivative
    ResizeUpSampleVertical
    ResizeUpSampleHorizontal
    ResizeUpSampleVerticalMitchell
    ResizeUpSampleHorizontalMitchell
    Difference4
    ReverseDifference2
    ReverseDifference3
    ReverseDifference4
    InlineDissolve2
    ColorCDL_SAT_noClamp
    ColorPremultLight
    SourceARGB
    SourceARGBUncrop
    SourceABGR
    SourceABGRUncrop
    SourceARGBfromBGRA
    SourceARGBfromBGRAUncrop
    SourceABGRfromBGRA
    SourceABGRfromBGRAUncrop
    LensWarpRadial
    LensWarpTangential
    LensWarpRadialAndTangential
    LensWarp3DE4AnamorphicDegree6
    ColorClamp
    ColorMatrix4D
    FilterGaussianHorizontal
    FilterGaussianVertical
    FilterGaussianHorizontalFast
    FilterGaussianVerticalFast
    FilterUnsharpMask
    FilterClarity
    FilterNoiseReduction
    ColorSRGBYCbCr
    ColorYCbCrSRGB
    ColorLinearGray
    ColorCurve
    ColorHighlight
    ColorShadow
    ColorVibrance
    Histogram
    ICCLinearSRGB
)

# Create shaders and add them to sources
QUOTE_FILES(_cooked_shaders INPUT_LIST ${_shaders} SUFFIX "glsl")
LIST(APPEND _sources ${_cooked_shaders})

# Create CL Kernels
SET(_cl_kernels
    Histogram48k Histogram32k Histogram16k
)
QUOTE_FILES(_cooked_cl_kernels INPUT_LIST ${_cl_kernels} SUFFIX "cl")
LIST(APPEND _sources ${_cooked_cl_kernels})

FILE(GLOB _nodes *IPNode.cpp)
# message(STATUS "_nodes: '${_nodes}'")
LIST(APPEND _sources ${_nodes})

FILE(GLOB _instance_nodes *IPInstanceNode.cpp)
# message(STATUS "_instance_nodes: '${_instance_nodes}'")
LIST(APPEND _sources ${_instance_nodes})

SET(_renderers
    ImageRenderer.cpp AudioRenderer.cpp PerFrameAudioRenderer.cpp
)
# message(STATUS "_renderers: '${_renderers}'")
LIST(APPEND _sources ${_renderers})

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
  PUBLIC LUT
         TwkApp
         TwkAudio
         TwkContainer
         TwkFB
         TwkMath
         TwkMovie
         TwkPaint
         TwkUtil
         stl_ext
         TwkGLF
  PRIVATE CDL
          OpenEXR::OpenEXR
          TwkDeploy
          TwkGLText
          TwkCMS
          IOtiff
          Mu
          Boost::filesystem
          ${CMAKE_DL_LIBS}
          IPBaseNodes
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC glew nedmalloc win_posix IPBaseNodes bcrypt
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-wd4355"
  )
ELSEIF(RV_TARGET_DARWIN)
  FIND_PACKAGE(OpenCL REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC OpenCL::OpenCL
  )
ELSE()
  TARGET_LINK_LIBRARIES(
    ${_target}
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
