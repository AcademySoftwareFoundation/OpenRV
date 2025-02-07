#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Need 3.27 for Boost 1.82 (FindBoost.cmake).
CMAKE_MINIMUM_REQUIRED(VERSION 3.27)

# These variables are parsed by sphinx for the documentation. One-line formatting facilitates parsing and readability.
# cmake-format: off
SET(RV_MAJOR_VERSION "2" CACHE STRING "RV's version major")
SET(RV_MINOR_VERSION "2" CACHE STRING "RV's version minor")
SET(RV_REVISION_NUMBER "0" CACHE STRING "RV's revision number")
SET(RV_VERSION_YEAR "2025" CACHE STRING "RV's year of release.")
# cmake-format: on

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE
      "Debug"
  )
ENDIF()

IF(CMAKE_BUILD_TYPE
   AND NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$"
)
  MESSAGE(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
ENDIF()

IF(NOT RV_RELEASE_DESCRIPTION)
  IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(RV_RELEASE_DESCRIPTION
        "DEVELOPMENT"
    )
  ELSE()
    # as checked 'src/lib/app/RvCommon/RvApplication.cpp'
    SET(RV_RELEASE_DESCRIPTION
        "RELEASE"
    )
  ENDIF()
ENDIF()

SET(RV_UI_APPLICATION_NAME
    "Open RV"
    CACHE STRING "RV's UI application name"
)
SET(RV_INTERNAL_APPLICATION_NAME
    "OpenRV"
    CACHE STRING "RV's internal application name"
)
SET(RV_INTERNAL_ORGANIZATION_NAME
    "ASWF"
    CACHE STRING "RV's internal organization name"
)
SET(RV_INTERNAL_ORGANIZATION_DOMAIN
    "aswf.com"
    CACHE STRING "RV's internal organization domain"
)

IF(NOT RV_APPLICATION_TYPE)
  SET(RV_APPLICATION_TYPE
      ${RV_INTERNAL_APPLICATION_NAME}
  )
ENDIF()

PROJECT(
  open-rv
  VERSION "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
  LANGUAGES CXX C
)

SET(CMAKE_CONFIGURATION_TYPES
    "Debug;Release"
    CACHE STRING "" FORCE
)

SET(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE STRING "CMake Build Type"
)

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

MESSAGE("PROJECT DIR ${PROJECT_SOURCE_DIR}")

# Specify our own CMake modules so we can include them right after.
SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/defaults ${PROJECT_SOURCE_DIR}/cmake/dependencies ${PROJECT_SOURCE_DIR}/cmake/globals
    ${PROJECT_SOURCE_DIR}/cmake/macros
)

INCLUDE(rv_options) # RV's build options
INCLUDE(rv_targets) # RV's build platform definitions
INCLUDE(rv_globals) # RV's CMake-build global variables
INCLUDE(rv_stage) # RV's local appplication packaging
INCLUDE(rv_vfx)
INCLUDE(CTest)

# The 'cxx_defaults' module will sets global C/C++ compiler defaults Note that variable such as 'CMAKE_CXX_COMPILER_ID' is only available after the 'project'
# statement just above.
INCLUDE(cxx_defaults)

# This should handle fetching or checking then compiling required 3rd party dependencies One can simple disable processing of a given dependency by simply
# commenting out the relevant line. (macOS) /usr/local/Cellar/cmake/3.20.3/share/cmake/Modules/FetchContent.cmake
INCLUDE(FetchContent) # once in the project to include the module
INCLUDE(ExternalProject) # once in the project to include the module

# Force verbosity of the FETCHCONTENT function, thus outputting some progress rather than just stalling CMake. Fetching and expanding an archive takes some
# time, as an indicator that something is actually happening, we'll force verbosity of the FETCHCONTENT function thus printing some minimal progress.
SET(FETCHCONTENT_QUIET
    OFF
)

ADD_SUBDIRECTORY(cmake/dependencies)

# RV's main code
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(packages)

ADD_SUBDIRECTORY(cmake/install)
