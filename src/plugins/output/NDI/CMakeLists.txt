#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "NDI"
)

SET(_sources
    NDIModule.cpp NDIVideoDevice.cpp NDIOutput.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

# Note that a super project, a project consuming Open RV as a submodule, could decide to fetch the NDI_SDK::NDI_SDK from artifactory and thus have already
# created the NDI_SDK::NDI_SDK target. In that case, we should not try to find.
IF(NOT TARGET NDI_SDK::NDI_SDK)
  FIND_PACKAGE(NDI_SDK)
  IF(NOT NDI_SDK_FOUND)
    MESSAGE(
      WARNING
        "NDI SDK not found, disabling NDI output plugin.\nDownload the NDI SDK to add NDI output capability to Open RV (optional): https://ndi.video/. Set the NDI_SDK_ROOT environment variable to the root of the NDI SDK installation if the NDI SDK fails to be automatically located."
    )
    RETURN()
  ENDIF()

  IF(RV_TARGET_WINDOWS)
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Installing NDI SDK libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy ${NDI_SDK_LIBRARY} ${RV_STAGE_LIB_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy ${NDI_SDK_BIN} ${RV_STAGE_BIN_DIR}
    )
  ELSEIF(RV_TARGET_LINUX)
    GET_FILENAME_COMPONENT(_ndi_sdk_lib_dir ${NDI_SDK_LIBRARY} DIRECTORY)
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Installing NDI SDK libs into ${RV_STAGE_LIB_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_ndi_sdk_lib_dir} ${RV_STAGE_LIB_DIR}
    )
  ELSE()
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Installing NDI SDK libs into ${RV_STAGE_LIB_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy ${NDI_SDK_LIBRARY} ${RV_STAGE_LIB_DIR}
    )
  ENDIF()
ENDIF()

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE NDI_SDK::NDI_SDK Boost::program_options Boost::thread TwkApp TwkGLF TwkUtil
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE "-framework Foundation" "-framework IOKit"
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE winmm Setupapi Netapi32 Shlwapi
  )
ENDIF()

RV_STAGE(TYPE "OUTPUT_PLUGIN" TARGET ${_target})
