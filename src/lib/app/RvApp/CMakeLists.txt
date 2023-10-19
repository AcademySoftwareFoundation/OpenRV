#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "RvApp"
)

SET(_sources
    RvSession.cpp
    RvGraph.cpp
    RvNodeDefinitions.cpp
    FormatIPNode.cpp
    FileSpaceLinearizeIPNode.cpp
    RvViewGroupIPNode.cpp
    Options.cpp
    CommandsModule.cpp
    PyCommandsModule.cpp
)

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
  PUBLIC IPCore Mu MuLang TwkMovie TwkFBAux
  PRIVATE Python::Python
          IPBaseNodes
          IPMu
          MuAutoDoc
          MuEncoding
          MuGL
          MuGLText
          MuGLU
          MuIO
          MuImage
          MuMathLinear
          MuSystem
          MuTwkApp
          OCIONodes
          PyTwkApp
          TwkApp
          TwkContainer
          TwkDeploy
          TwkFB
          TwkMath
          TwkUtil
          stl_ext
          ICCNodes
          CDL
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE glew nedmalloc win_posix gc
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-wd4355"
  )
ELSE()
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE BDWGC::Gc
  )
ENDIF()

GET_PROPERTY(
  RV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION GLOBAL
  PROPERTY "RV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION"
)
IF(RV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION=1"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE ${RV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION}
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
