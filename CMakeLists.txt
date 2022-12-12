#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
CMAKE_MINIMUM_REQUIRED(VERSION 3.24)

SET(CMAKE_CONFIGURATION_TYPES
    "Debug;Release"
    CACHE STRING "" FORCE
)

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

SET(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE STRING "CMake Build Type"
)

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Specify our own CMake modules so we can include them right after.
SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/defaults ${CMAKE_SOURCE_DIR}/cmake/dependencies ${CMAKE_SOURCE_DIR}/cmake/globals
    ${CMAKE_SOURCE_DIR}/cmake/macros
)

SET(CMAKE_OSX_DEPLOYMENT_TARGET
    "10.14"
)
INCLUDE(rv_version) # RV's build system global variables

PROJECT(
  open-rv
  VERSION ${RV_VERSION_STRING}
  LANGUAGES CXX C
)

INCLUDE(rv_options) # RV's build options
INCLUDE(rv_targets) # RV's build platform definitions
INCLUDE(rv_globals) # RV's CMake-build global variables
INCLUDE(rv_stage) # RV's local appplication packaging
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
