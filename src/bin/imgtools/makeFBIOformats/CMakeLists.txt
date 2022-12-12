#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "makeFBIOformats"
)

LIST(APPEND _sources main.cpp utf8Main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE TwkFB Gto TwkUtil OpenEXR::OpenEXR
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})

ADD_CUSTOM_COMMAND(
  COMMENT "Generating ${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}/formats.gto"
  OUTPUT ${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}/formats.gto
  COMMAND $<TARGET_FILE:${_target}> ${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}
  WORKING_DIRECTORY ${RV_STAGE_BIN_DIR}
  DEPENDS ${_target} image_formats
)

ADD_CUSTOM_TARGET(
  formats.gto ALL
  DEPENDS ${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}/formats.gto
)

ADD_DEPENDENCIES(installed_image_formats formats.gto)
