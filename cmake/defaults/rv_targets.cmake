#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(CMAKE_SKIP_RPATH
    ON
)

IF(APPLE)
  # TODO: Need to check to value of CMAKE_HOST_SYSTEM_PROCESSOR on M2 and M3.
  #       It might returns only "arm".
  SET(RV_DISABLE_APPLE_ARM64
      OFF
      CACHE BOOL "Prevent RV to be build for Apple ARM64 native build"
  )

  IF(RV_DISABLE_APPLE_ARM64)
    # Force build to x86_64 even on Apple ARM because RV_DISABLE_APPLE_ARM64 is enabled.
    SET(CMAKE_OSX_ARCHITECTURES
        "x86_64"
        CACHE STRING "Force compilation to x86_64" FORCE
    )
  ENDIF()

  # If CMAKE_OSX_ARCHITECTURES is not set, set it to the host native build. (x86_64 or arm64)
  IF(NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
    SET(CMAKE_OSX_ARCHITECTURES
        "${CMAKE_HOST_SYSTEM_PROCESSOR}"
        CACHE STRING "Force compilation to the native architecture (${CMAKE_HOST_SYSTEM_PROCESSOR})" FORCE
    )
  ENDIF()

  # Universal build (x86_64;arm64) are not supported and not tested.
  # Set a variable to easily check if the current build is for x86_64 or arm64. Used in cmake/dependencies/*.cmake.
  # Set a compile option to be used as define in the code.
  IF(DEFINED CMAKE_OSX_ARCHITECTURES OR NOT CMAKE_OSX_ARCHITECTURES STREQUAL "")
    IF("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64")
      SET(RV_TARGET_APPLE_X86_64
          ON
          CACHE INTERNAL "Compile for x86_64 on Apple MacOS" FORCE
      )
      SET(__target_arch__ -DRV_ARCH_X86_64)
    ELSEIF("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
      SET(RV_TARGET_APPLE_ARM64
          ON
          CACHE INTERNAL "Compile for arm64 on Apple MacOS" FORCE
      )
      SET(__target_arch__ -DRV_ARCH_ARM64)
    ELSEIF("${CMAKE_OSX_ARCHITECTURES}" MATCHES "(arm64;x86_64|x86_64;arm64)")
      MESSAGE(FATAL "Universal build on MacOS is not supported.")
    ENDIF()
  ENDIF()

  MESSAGE(STATUS "Building for ${CMAKE_OSX_ARCHITECTURES} (CMAKE_HOST_SYSTEM_PROCESSOR = ${CMAKE_HOST_SYSTEM_PROCESSOR})")
  ADD_COMPILE_OPTIONS(${__target_arch__})

  # Darwin is the name of the mach BSD-base kernel :-)
  SET(RV_TARGET_DARWIN
      BOOL TRUE "Detected target is Apple's macOS"
      CACHE INTERNAL ""
  )
  SET(RV_TARGET_STRING
      "Darwin"
      CACHE INTERNAL ""
  )

  # The code makes extensive use of the 'PLATFORM_DARWIN' definition
  SET(PLATFORM
      "DARWIN"
      CACHE STRING "Platform identifier used in Tweak Makefiles"
  )

  SET(CMAKE_MACOSX_RPATH
      ON
  )

  # Get macOS version
  SET(_macos_version_string
      ""
  )
  FIND_PROGRAM(_sw_vers sw_vers)
  EXECUTE_PROCESS(
    COMMAND ${_sw_vers} -productVersion
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _macos_version_string
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  IF(_macos_version_string STREQUAL "")
    MESSAGE(FATAL_ERROR "Failed to get macOS version.")
  ELSE()
    MESSAGE(STATUS "Building using: macOS ${_macos_version_string}")
  ENDIF()

  ADD_COMPILE_OPTIONS(
    -DARCH_IA32_64
    -DPLATFORM_DARWIN
    -DTWK_LITTLE_ENDIAN
    -D__LITTLE_ENDIAN__
    -DPLATFORM_APPLE_MACH_BSD
    -DPLATFORM_OPENGL
    -DGL_SILENCE_DEPRECATION
    # _XOPEN_SOURCE, Required on macOS to resolve such compiling error:
    # /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/include/ucontext.h:51:2: error: The deprecated
    # ucontext routines require _XOPEN_SOURCE to be defined error The deprecated ucontext routines require _XOPEN_SOURCE to be defined
    # https://pubs.opengroup.org/onlinepubs/009695399/
    -D_XOPEN_SOURCE=600
    -D_DARWIN_C_SOURCE # was at least required to compile 'pub/cxvore' on macOS
    -DOSX_VERSION=${_macos_version_string}
  )

  #
  # CXXFLAGS_DARWIN   += -DOSX_VERSION=\"$(shell sw_vers -productVersion)\"
  #

ELSEIF(UNIX)
  MESSAGE(STATUS "Building RV for generic Linux OS")
  SET(RV_TARGET_LINUX
      BOOL TRUE "Detected a generic Linux OS"
      CACHE INTERNAL ""
  )
  SET(RV_TARGET_STRING
      "Linux"
      CACHE INTERNAL ""
  )
  # Linux target is not enough to be able to set endianess We should use Boost endian.hpp in files casing on either TWK_LITTLE_ENDIAN or TWK_BIG_ENDIAN
  ADD_COMPILE_OPTIONS(
    -DARCH_IA32_64
    -DPLATFORM_LINUX
    -DTWK_LITTLE_ENDIAN
    -D__LITTLE_ENDIAN__
    -DTWK_NO_SGI_BYTE_ORDER
    -DGL_GLEXT_PROTOTYPES
    -DPLATFORM_OPENGL=1
  )

  EXECUTE_PROCESS(
    COMMAND cat /etc/redhat-release
    OUTPUT_VARIABLE RHEL_VERBOSE
    ERROR_QUIET
  )

  IF(RHEL_VERBOSE)
    MESSAGE(STATUS "Redhat Entreprise Linux version: ${RHEL_VERBOSE}")
    STRING(REGEX MATCH "([0-9]+)" RHEL_VERSION_MAJOR "${RHEL_VERBOSE}")
    SET(RV_TARGET_IS_RHEL${RHEL_VERSION_MAJOR}
        BOOL TRUE "Detected a Redhat Entreprise Linux OS"
    )
  ELSE()
    MESSAGE(FATAL_ERROR "Unknown or unsupported Linux distribution version; stopping configuration!")
  ENDIF()
ELSEIF(WIN32)
  MESSAGE(STATUS "Building RV for Microsoft Windows")
  SET(RV_TARGET_WINDOWS
      BOOL TRUE "Detected target is Microsoft's Windows (64bit)"
      CACHE INTERNAL ""
  )
  SET(RV_TARGET_STRING
      "Windows"
      CACHE INTERNAL ""
  )
  ADD_COMPILE_OPTIONS(
    -DARCH_IA32_64
    -DPLATFORM_WINDOWS
    -DTWK_LITTLE_ENDIAN
    -D__LITTLE_ENDIAN__
    -DTWK_NO_SGI_BYTE_ORDER
    -DGL_GLEXT_PROTOTYPES
    -DPLATFORM_OPENGL=1
  )
  SET(PLATFORM
      "WINDOWS"
      CACHE STRING "Platform identifier"
  )
  SET(ARCH
      "IA32_64"
      CACHE STRING "CPU Architecture identifier"
  )
ELSE()
  MESSAGE(FATAL_ERROR "Unsupported platform")

ENDIF()