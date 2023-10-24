#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rv"
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

FIND_PACKAGE(
  Qt5
  COMPONENTS Core Gui Widgets OpenGL
  REQUIRED
)

SET(SOURCES
    main.cpp utf8WinMain.cpp
)

IF(RV_TARGET_WINDOWS)
  IF(DEFINED RV_RC_ICON)
    LIST(APPEND SOURCES ${RV_RC_ICON})
  ELSE()
    LIST(APPEND SOURCES RV.RC) # Use Default Open RV logo
  ENDIF()
ENDIF()

QT5_ADD_RESOURCES(_rv_qrc rv.qrc)

ADD_EXECUTABLE(
  ${_target}
  ${SOURCES} ${_rv_qrc}
)

IF(RV_TARGET_WINDOWS)
  IF(CMAKE_BUILD_TYPE STREQUAL "Release")
    SET_PROPERTY(
      TARGET ${_target}
      PROPERTY WIN32_EXECUTABLE True
    )
  ENDIF()
ENDIF()

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

IF(RV_TARGET_LINUX)
  SET(THREADS_PREFER_PTHREAD_FLAG
      TRUE
  )
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Threads::Threads
  )
ENDIF()

IF(NOT RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Qt::OpenGL
  )
ENDIF()

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE RvCommon
          IOproxy
          MovieProxy
          MovieSideCar
          OpenEXR::OpenEXR
          MovieFB
          MovieProcedural
          MuGL
          MuIO
          MuTwkApp
          PyTwkApp
          IPCore
          RvApp
          RvPackage
          TwkApp
          TwkDeploy
          QTBundle
          TwkQtCoreUtil
          TwkExc
          TwkFB
          TwkGLFFBO
          TwkMovie
          TwkUtil
          TwkGLF
          arg
          stl_ext
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE glew win_pthreads Shlwapi
  )
ENDIF()

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -D__STDC_FORMAT_MACROS
          -DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"
          -DUSE_SIDECARS=1
          -DMAJOR_VERSION=${RV_MAJOR_VERSION}
          -DMINOR_VERSION=${RV_MINOR_VERSION}
          -DREVISION_NUMBER=${RV_REVISION_NUMBER}
          -DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\"
)

GET_PROPERTY(
  RV_THIRD_PARTY_CUSTOMIZATION GLOBAL
  PROPERTY "RV_THIRD_PARTY_CUSTOMIZATION"
)
IF(RV_THIRD_PARTY_CUSTOMIZATION)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRV_THIRD_PARTY_CUSTOMIZATION=1"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE ${RV_THIRD_PARTY_CUSTOMIZATION}
  )
ENDIF()

RV_STAGE(TYPE "MAIN_EXECUTABLE" TARGET ${_target})
