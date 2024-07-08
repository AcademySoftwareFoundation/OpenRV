#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Find the NDI SDK headers and libraries.
#
# Once done this will define: NDI_SDK_FOUND    - true if NDI SDK has been found NDI_SDK::NDI_SDK - Imported target
#
# Note: Set NDI_SDK_ROOT to specify an additional directory to search.

FIND_PATH(
  NDI_SDK_INCLUDE_DIR
  NAMES Processing.NDI.Lib.h
  PATHS "${NDI_SDK_ROOT}/include" "${NDI_SDK_ROOT}/Include" "$ENV{NDI_SDK_ROOT}/include" "$ENV{NDI_SDK_ROOT}/Include" "/Library/NDI SDK for Apple/include"
        "$ENV{PROGRAMFILES}/NDI/NDI 6 SDK/Include"
  DOC "NDI_SDK include directory"
)
MARK_AS_ADVANCED(NDI_SDK_INCLUDE_DIR)

FIND_LIBRARY(
  NDI_SDK_LIBRARY
  NAMES ndi Processing.NDI.Lib.x64
  PATHS "${NDI_SDK_ROOT}/lib" "$ENV{NDI_SDK_ROOT}/lib" "${NDI_SDK_ROOT}/lib/x86_64-linux-gnu" "$ENV{NDI_SDK_ROOT}/lib/x86_64-linux-gnu"
        "/Library/NDI SDK for Apple/lib/macOS" "$ENV{PROGRAMFILES}/NDI/NDI 6 SDK/Lib/x64"
  DOC "NDI_SDK library"
)
MARK_AS_ADVANCED(NDI_SDK_LIBRARY)

IF(RV_TARGET_WINDOWS)
  FIND_FILE(
    NDI_SDK_BIN
    NAMES Processing.NDI.Lib.x64.dll
    PATHS "$ENV{PROGRAMFILES}/NDI/NDI 6 SDK/Bin/x64"
    DOC "NDI_SDK DLL"
  )
  MARK_AS_ADVANCED(NDI_SDK_BIN)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  NDI_SDK
  REQUIRED_VARS NDI_SDK_INCLUDE_DIR NDI_SDK_LIBRARY
  VERSION_VAR NDI_SDK_VERSION
)

IF(NDI_SDK_FOUND)
  IF(NOT TARGET NDI_SDK::NDI_SDK)
    ADD_LIBRARY(NDI_SDK::NDI_SDK UNKNOWN IMPORTED)
    SET_TARGET_PROPERTIES(
      NDI_SDK::NDI_SDK
      PROPERTIES IMPORTED_LOCATION "${NDI_SDK_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${NDI_SDK_INCLUDE_DIR}"
    )
  ENDIF()
ENDIF()
