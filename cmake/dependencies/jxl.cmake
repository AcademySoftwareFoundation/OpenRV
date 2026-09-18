#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [libjxl -- Sources](https://github.com/libjxl/libjxl)
#
# [libjxl -- Documentation](https://libjxl.readthedocs.io/en/latest/)
#
# [libjxl -- Build instructions](https://github.com/libjxl/libjxl/blob/main/BUILDING.md)
#

IF(NOT JXL_ROOT)
  RETURN()
ENDIF()

# Find out the libjxl version from the header file
IF(RV_DEPS_JXL_INCLUDE_DIR)
  FILE(
    STRINGS "${RV_DEPS_JXL_INCLUDE_DIR}/jxl/version.h" TMP
    REGEX "^#define JPEGXL_MAJOR_VERSION .*$"
  )
  STRING(REGEX MATCHALL "[0-9]+" JPEGXL_MAJOR_VERSION ${TMP})
  FILE(
    STRINGS "${RV_DEPS_JXL_INCLUDE_DIR}/jxl/version.h" TMP
    REGEX "^#define JPEGXL_MINOR_VERSION .*$"
  )
  STRING(REGEX MATCHALL "[0-9]+" JPEGXL_MINOR_VERSION ${TMP})
  FILE(
    STRINGS "${RV_DEPS_JXL_INCLUDE_DIR}/jxl/version.h" TMP
    REGEX "^#define JPEGXL_PATCH_VERSION .*$"
  )
  STRING(REGEX MATCHALL "[0-9]+" JPEGXL_PATCH_VERSION ${TMP})
  SET(RV_DEPS_JXL_VERSION
      "${JPEGXL_MAJOR_VERSION}.${JPEGXL_MINOR_VERSION}.${JPEGXL_PATCH_VERSION}"
  )
ENDIF()

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_JXL" "${RV_DEPS_JXL_VERSION}" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

# Resolve libjxl and libjxl_threads under the caller-provided JXL_ROOT.
FIND_LIBRARY(
  RV_DEPS_JXL_LIBRARY
  NAMES jxl
  PATHS "${JXL_ROOT}/lib" "${JXL_ROOT}/lib64"
  NO_DEFAULT_PATH
)
FIND_LIBRARY(
  RV_DEPS_JXL_THREADS_LIBRARY
  NAMES jxl_threads
  PATHS "${JXL_ROOT}/lib" "${JXL_ROOT}/lib64"
  NO_DEFAULT_PATH
)

IF(NOT RV_DEPS_JXL_LIBRARY
   OR NOT RV_DEPS_JXL_THREADS_LIBRARY
)
  MESSAGE(WARNING "JXL_ROOT=${JXL_ROOT} but libjxl or libjxl_threads were not found under ${JXL_ROOT}/lib[64]; JPEG XL will be disabled in OpenImageIO.")
  # Leave RV_DEPS_JXL_ROOT_DIR unset so oiio.cmake disables the format.
  UNSET(RV_DEPS_JXL_ROOT_DIR)
  RETURN()
ENDIF()

SET(RV_DEPS_JXL_ROOT_DIR
    "${JXL_ROOT}"
)

# Validate the resolved root actually exposes the headers OIIO compiles against.
FIND_PATH(
  RV_DEPS_JXL_INCLUDE_DIR
  NAMES jxl/encode.h jxl/decode.h
  PATHS "${RV_DEPS_JXL_ROOT_DIR}/include"
  NO_DEFAULT_PATH
)
IF(NOT RV_DEPS_JXL_INCLUDE_DIR)
  MESSAGE(
    WARNING
      "libjxl libraries were found (${RV_DEPS_JXL_LIBRARY}) but jxl/encode.h was not found under ${RV_DEPS_JXL_ROOT_DIR}/include; JPEG XL will be disabled in OpenImageIO."
  )
  UNSET(RV_DEPS_JXL_ROOT_DIR)
  RETURN()
ENDIF()

MESSAGE(STATUS "Found libjxl:         ${RV_DEPS_JXL_LIBRARY}")
MESSAGE(STATUS "Found libjxl_threads: ${RV_DEPS_JXL_THREADS_LIBRARY}")
MESSAGE(STATUS "Using libjxl root:    ${RV_DEPS_JXL_ROOT_DIR}")
