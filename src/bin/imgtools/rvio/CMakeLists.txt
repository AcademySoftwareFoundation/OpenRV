#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rvio"
)

LIST(APPEND _sources UICommands.cpp main.cpp utf8Main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core Gui Widgets
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Mu MuLang MuTwkApp TwkMovie TwkAudio
  PRIVATE RvCommon
          IOproxy
          MovieProxy
          OpenEXR::OpenEXR
          PyTwkApp
          RvApp
          IPCore
          IPBaseNodes
          MovieRV_FBO
          MovieFB
          MovieMuDraw
          MovieProcedural
          TwkAudio
          TwkMovie
          TwkCMS
          TwkMath
          TwkDeploy
          TwkExc
          TwkFB
          TwkUtil
          TwkQtBase
          arg
          stl_ext
          TwkMediaLibrary
          Qt5::Core
          TwkGLFFBO
)

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Qt5::Gui Qt5::Widgets GLEW::GLEW BDWGC::Gc QTBundle
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Qt5::Gui Qt5::Widgets glew gc QTBundle
  )
ELSEIF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE BDWGC::Gc DarwinBundle
  )
ENDIF()

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-DRVIO_HW=1" "-DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"" "-DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\""
)

GET_PROPERTY(
  RVIO_THIRD_PARTY_CUSTOMIZATION GLOBAL
  PROPERTY "RVIO_THIRD_PARTY_CUSTOMIZATION"
)
IF(RVIO_THIRD_PARTY_CUSTOMIZATION)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRVIO_THIRD_PARTY_CUSTOMIZATION=1"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE ${RVIO_THIRD_PARTY_CUSTOMIZATION}
  )
ENDIF()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
