#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Debugging options
OPTION(RV_VERBOSE_INVOCATION "Show the compiler/link command invocation." OFF)
OPTION(RV_SHOW_ALL_VARIABLES "Displays all build variables." ON)

#
# General build options
SET(RV_DEPS_BASE_DIR
    "${CMAKE_BINARY_DIR}"
    CACHE STRING "RV's 3rd party cache location."
)
SET(RV_DEPS_DOWNLOAD_DIR
    "${RV_DEPS_BASE_DIR}/RV_DEPS_DOWNLOAD"
    CACHE STRING "RV's 3rd party download cache location."
)

IF(NOT EXISTS (${RV_DEPS_BASE_DIR}))
  FILE(MAKE_DIRECTORY ${RV_DEPS_BASE_DIR})
ENDIF()

IF(NOT EXISTS (${RV_DEPS_DOWNLOAD_DIR}))
  FILE(MAKE_DIRECTORY ${RV_DEPS_DOWNLOAD_DIR})
ENDIF()

#
# VFX Platform option
#
# That option is to control the versions of the external dependencies that OpenRV downloads and install based on the VFX platform.
#
# e.g. For CY2023, OCIO 2.2.x should be supported. For CY2024, OCIO 2.3.x should be supported.
#
# Supported VFX platform.
SET(RV_VFX_SUPPORTED_OPTIONS
    CY2023 CY2024 CY2025 CY2026
)
# Default option
SET(_RV_VFX_PLATFORM
    "CY2025"
)

IF(DEFINED RV_VFX_PLATFORM)
  # Match lowercase and uppercase.
  STRING(TOUPPER "${RV_VFX_PLATFORM}" _RV_VFX_PLATFORM)
  IF(NOT "${_RV_VFX_PLATFORM}" IN_LIST RV_VFX_SUPPORTED_OPTIONS)
    MESSAGE(FATAL_ERROR "RV_VFX_PLATFORM=${RV_VFX_PLATFORM} is unsupported. Supported values are: ${RV_VFX_SUPPORTED_OPTIONS}")
  ENDIF()
ENDIF()

# Overwrite the cache variable with the normalized (upper)case.
SET(RV_VFX_PLATFORM
    "${_RV_VFX_PLATFORM}"
    CACHE STRING "Set the VFX platform for installaing external dependencies" FORCE
)

SET_PROPERTY(
  CACHE RV_VFX_PLATFORM
  PROPERTY STRINGS ${_RV_VFX_PLATFORM}
)

#
# C/C++ standard options
#
# The C++ standard follows the VFX Reference Platform: CY2026 and later require C++20, earlier years stay on C++17. It is derived from RV_VFX_PLATFORM rather
# than set by the user, so that the standard RV is built with always matches the one its dependencies are built with.
#
IF(RV_VFX_PLATFORM STRGREATER_EQUAL "CY2026")
  SET(_RV_CPP_STANDARD
      "20"
  )
ELSE()
  SET(_RV_CPP_STANDARD
      "17"
  )
ENDIF()

# Re-derived on every configure (FORCE), like RV_VFX_PLATFORM above. Without it an existing build tree reconfigured to a newer VFX platform would keep the
# standard cached by its first configure and silently build CY2026 with C++17.
SET(RV_CPP_STANDARD
    "${_RV_CPP_STANDARD}"
    CACHE STRING "RV's general C++ coding standard" FORCE
)
SET_PROPERTY(
  CACHE RV_CPP_STANDARD
  PROPERTY STRINGS ${_RV_CPP_STANDARD}
)

# The C standard stays at C17 on every VFX platform. It is deliberately not tied to RV_CPP_STANDARD: there is no C20, and CMake silently ignores an invalid
# C_STANDARD value (emitting no -std flag at all, which would un-pin the C standard). C17 is the newest C standard with universal MSVC/GCC/Clang support, and
# the VFX Reference Platform pins only the C++ standard.
SET(RV_C_STANDARD
    "17"
    CACHE STRING "RV's general C coding standard"
)
SET_PROPERTY(
  CACHE RV_C_STANDARD
  PROPERTY STRINGS C99 11 17
)

#
# FFmpeg option
#
# This option is to control the version of FFmpeg that OpenRV downloads and installs.
#
# There will be one version used by default per major release. Any other version will require RV_FFMPEG env var to be set with the desired major version.
#

# Supported FFmpeg versions.
SET(RV_FFMPEG_SUPPORTED_OPTIONS
    6 7 8
)
# Default option
SET(_RV_FFMPEG
    "8"
)

IF(DEFINED RV_FFMPEG)
  SET(_RV_FFMPEG
      ${RV_FFMPEG}
  )
  IF(NOT "${_RV_FFMPEG}" IN_LIST RV_FFMPEG_SUPPORTED_OPTIONS)
    MESSAGE(FATAL_ERROR "RV_FFMPEG=${RV_FFMPEG} is unsupported. Supported values are: ${RV_FFMPEG_SUPPORTED_OPTIONS}")
  ENDIF()
ENDIF()

# Overwrite the cache variable
SET(RV_FFMPEG
    "${_RV_FFMPEG}"
    CACHE STRING "Set FFmpeg version for installing external depdendency" FORCE
)
SET_PROPERTY(
  CACHE RV_FFMPEG
  PROPERTY STRINGS ${_RV_FFMPEG}
)

#
# Dependency resolution option
#
# When ON, try find_package() for each dependency before building from source. When OFF (default), always build dependencies from source (current behavior).
#
# Per-dependency override: set RV_DEPS_<NAME>_FORCE_BUILD=ON to force building a specific dependency from source even when RV_DEPS_PREFER_INSTALLED=ON.
#
OPTION(RV_DEPS_PREFER_INSTALLED "Try find_package() for dependencies before building from source" OFF)

#
# Version matching mode for dependency resolution.
#
# Controls how RV_FIND_DEPENDENCY matches versions when RV_DEPS_PREFER_INSTALLED=ON. EXACT requires the exact version specified in CY*.cmake (default,
# recommended). MINIMUM accepts the specified version or newer (standard find_package behavior).
#
# Per-dependency override: set RV_DEPS_<NAME>_VERSION_MATCH=EXACT or MINIMUM to override the global setting for a specific dependency.
#
SET(RV_DEPS_VERSION_MATCH
    "EXACT"
    CACHE STRING "Version matching mode for find_package: EXACT or MINIMUM"
)
SET_PROPERTY(
  CACHE RV_DEPS_VERSION_MATCH
  PROPERTY STRINGS EXACT MINIMUM
)
