#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)
INCLUDE(CPack)

SET(_target
    "RV"
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Gui
  REQUIRED
)

SET(_sources
    main.cpp
)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\" -DUSE_SIDECARS=1 -DMAJOR_VERSION=${RV_MAJOR_VERSION} -DMINOR_VERSION=${RV_MINOR_VERSION}
          -DREVISION_NUMBER=${RV_REVISION_NUMBER} -DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\"
)

#
# Platform common libraries
TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE DarwinBundle
          IOproxy
          MovieProxy
          MovieSideCar
          OpenEXR::OpenEXR
          MovieFB
          MovieProcedural
          Mu
          MuGL
          MuIO
          MuTwkApp
          PyTwkApp
          Qt5::Gui
          IPCore
          RvApp
          RvCommon
          RvPackage
          TwkQtCoreUtil
          TwkApp
          TwkDeploy
          TwkExc
          TwkFB
          TwkGLF
          TwkMovie
          TwkUtil
          arg
          stl_ext
          BDWGC::Gc
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

FILE(GLOB _icns_files *.icns)
FOREACH(
  _icns_file
  ${_icns_files}
)
  CONFIGURE_FILE(${_icns_file} ${RV_STAGE_RESOURCES_DIR} COPYONLY)
ENDFOREACH()
