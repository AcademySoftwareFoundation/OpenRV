#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_APPLE"
)

SET(RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH
    ""
    CACHE STRING "Path to ProRes SDK from Apple"
)

IF(NOT RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH)
  MESSAGE(
    WARNING
      "ProRes SDK path not specified, disabling ProRes support.\nContact Apple at prores@apple.com to obtain the free SDK. Then set RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH to the path of the received zip file on the rvcfg line.\nExample:\nrvcfg -DRV_DEPS_APPLE_PRORES_SDK_ZIP_PATH='<downloads_path>/ProResDecoder_Linux_x86_64-15B54.zip'"
  )
  RETURN()
ENDIF()

STRING(
  REGEX
  REPLACE ".*[-_]([0-9A-Z]+)\\.(zip|tgz)" "\\1" _version ${RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH}
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/prores
)
SET(_include_dir
    ${_install_dir}
)
SET(_lib_dir
    ${_install_dir}
)

SET(_prores_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}ProResDecoder${CMAKE_STATIC_LIBRARY_SUFFIX}
)

SET(_install_command
    ""
)

SET(_download_name
    ${_target}_prores_${_version}.zip
)
SET(_source_dir
    ${RV_DEPS_BASE_DIR}/${_target}/prores
)

SET(_prores_lib
    ${_lib_dir}/${_prores_lib_name}
)

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${RV_DEPS_APPLE_PRORES_SDK_ZIP_PATH}
  DOWNLOAD_NAME ${_download_name}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS ${_prores_lib}
)

ADD_LIBRARY(Apple::ProRes STATIC IMPORTED GLOBAL)
TARGET_INCLUDE_DIRECTORIES(
  Apple::ProRes
  INTERFACE ${_include_dir}
)
SET_PROPERTY(
  TARGET Apple::ProRes
  PROPERTY IMPORTED_LOCATION ${_prores_lib}
)

SET(RV_DEPS_APPLE_PRORES_VERSION_INCLUDE_DIR
    ${_include_dir}
    CACHE STRING "Path to installed includes for ${_target}"
)
ADD_DEPENDENCIES(Apple::ProRes ${_prores_lib})
IF(RV_TARGET_WINDOWS)
  ADD_DEPENDENCIES(Apple::ProRes ${_target})
ENDIF()

SET(RV_DEPS_APPLE_PRORES_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
