#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_BMD"
)

SET(RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH
    ""
    CACHE STRING "Path to Blackmagic Decklink SDK (zip)"
)

IF(NOT RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH)
  MESSAGE(
    WARNING
      "Blackmagic Decklink SDK path not specified, disabling Blackmagic output plugin.\nDownload the Blackmagic Desktop Video SDK to add Blackmagic output capability to Open RV (optional): https://www.blackmagicdesign.com/desktopvideo_sdk. Then set RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH to the path of the downloaded zip file on the rvcfg line.\nExample:\nrvcfg -DRV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH='<downloads_path>/Blackmagic_DeckLink_SDK_14.1.zip'"
  )
  RETURN()
ENDIF()

STRING(
  REGEX
  REPLACE ".*_([0-9]+\\.[0-9]+(\\.[0-9]+)?).*" "\\1" _version ${RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH}
)

IF(RV_TARGET_DARWIN)
  SET(_bmd_platform_dir
      "Mac"
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_bmd_platform_dir
      "Linux"
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_bmd_platform_dir
      "Win"
  )
ENDIF()
SET(_include_dir
    ${RV_DEPS_BASE_DIR}/${_target}/src/${_bmd_platform_dir}/include
)

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${RV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

# Generate the DeckLinkAPI.h file from DeckLinkAPI.idl provided with the DeckLink SDK
IF(RV_TARGET_WINDOWS)
  EXTERNALPROJECT_ADD_STEP(
    ${_target} post_install_step
    COMMAND midl.exe ARGS /header DeckLinkAPI.h /iid DeckLinkAPIDispatch.cpp DeckLinkAPI.idl
    WORKING_DIRECTORY ${_include_dir}
    DEPENDEES install
  )
ENDIF()

ADD_LIBRARY(BlackmagicDeckLinkSDK INTERFACE)
ADD_DEPENDENCIES(BlackmagicDeckLinkSDK ${_target})
TARGET_INCLUDE_DIRECTORIES(
  BlackmagicDeckLinkSDK
  INTERFACE ${_include_dir}
)

SET(RV_DEPS_BMD_VERSION_INCLUDE_DIR
    ${_include_dir}
    CACHE STRING "Path to installed includes for ${_target}"
)

SET(RV_DEPS_BMD_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
