#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

IF(NOT RV_CPP_STANDARD
   OR RV_CPP_STANDARD STREQUAL ""
)
  MESSAGE(FATAL_ERROR "The RV_CPP_STANDARD variable is not defined")
ENDIF()

IF(NOT RV_C_STANDARD
   OR RV_C_STANDARD STREQUAL ""
)
  MESSAGE(FATAL_ERROR "The RV_C_STANDARD variable is not defined")
ENDIF()

# specify the C/C++ standard
SET(CMAKE_CXX_STANDARD
    ${RV_CPP_STANDARD}
)
SET(CMAKE_C_STANDARD
    ${RV_C_STANDARD}
)
SET(CMAKE_CXX_STANDARD_REQUIRED
    TRUE
)
SET(CMAKE_C_STANDARD_REQUIRED
    TRUE
)

#
# Ref.: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html#variable:CMAKE_%3CLANG%3E_COMPILER_ID
#
# Also consider this one: https://stackoverflow.com/a/10055571
#

IF(NOT DEFINED RV_MAJOR_VERSION)
  MESSAGE(FATAL_ERROR "The 'RV_MAJOR_VERSION' CMake variable is not set!")
ENDIF()
IF(NOT DEFINED RV_MINOR_VERSION)
  MESSAGE(FATAL_ERROR "The 'RV_MINOR_VERSION' CMake variable is not set!")
ENDIF()
IF(NOT DEFINED RV_REVISION_NUMBER)
  MESSAGE(FATAL_ERROR "The 'RV_REVISION_NUMBER' CMake variable is not set!")
ENDIF()

ADD_COMPILE_OPTIONS(-DMAJOR_VERSION=${RV_MAJOR_VERSION} -DMINOR_VERSION=${RV_MINOR_VERSION} -DREVISION_NUMBER=${RV_REVISION_NUMBER})

IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  INCLUDE(cxx_gcc_defaults)
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  INCLUDE(cxx_clang_defaults)
ELSEIF(MSVC)
  INCLUDE(cxx_msvc_defaults)
ELSE()
  MESSAGE(FATAL_ERROR "Couldn't determine compiler identity")
ENDIF()

#
# VFX Platform option
#

# Add preprocessor variable for use in the code and cmake variable to decide the version of an external dependencies based on the VFX platform. Note that the
# macro in rv_vfx.cmake are dependant on those RV_VFX_CY20XX.
IF(RV_VFX_PLATFORM STREQUAL CY2024)
  SET(RV_VFX_CY2024
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_VFX_CY2024)
  SET(RV_QT_PACKAGE_NAME
      "Qt6"
  )
  SET(RV_QT_MU_TARGET
      "MuQt6"
  )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(RV_VFX_CY2023
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_VFX_CY2023)
  SET(RV_QT_PACKAGE_NAME
      "Qt5"
  )
  SET(RV_QT_MU_TARGET
      "MuQt5"
  )
ENDIF()

#
# FFmpeg option
#

# Add preprocessor variable for use in code and cmake to determine the current version of FFmpeg. Current version must be one of the supported versions, as
# defined in ffmpeg.cmake.
IF(RV_FFMPEG STREQUAL 6)
  SET(RV_FFMPEG_6
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_FFMPEG_6)
ELSEIF(RV_FFMPEG STREQUAL 7)
  SET(RV_FFMPEG_7
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_FFMPEG_7)
ELSEIF(RV_FFMPEG STREQUAL 8)
  SET(RV_FFMPEG_8
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_FFMPEG_8)
ENDIF()
