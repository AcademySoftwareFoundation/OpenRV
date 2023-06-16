#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(rv_git)
SET(RV_PATCHES_DIR "${CMAKE_SOURCE_DIR}/cmake/patches")
SET(RV_PKGMANCONFIG_DIR "${CMAKE_SOURCE_DIR}/cmake/packman")

SET(RV_DEPS_DOWNLOAD_DIR
    "${RV_DEPS_BASE_DIR}/downloads"
    CACHE STRING "RV's 3rd party download cache location."
)
FILE(MAKE_DIRECTORY ${RV_DEPS_DOWNLOAD_DIR})

SET(RV_BUILD_ROOT
    ${CMAKE_BINARY_DIR}/stage
    CACHE STRING "RV's build output directory."
)
FILE(MAKE_DIRECTORY ${RV_BUILD_ROOT})

SET(RV_APP_ROOT
    ${RV_BUILD_ROOT}/app
    CACHE STRING "RV's build output app directory."
)
FILE(MAKE_DIRECTORY ${RV_APP_ROOT})

SET(RV_PACKAGES_DIR
    ${RV_BUILD_ROOT}/packages
    CACHE STRING "RV's build output packages output directory."
)
FILE(MAKE_DIRECTORY ${RV_PACKAGES_DIR})

IF(RV_TARGET_DARWIN)
  SET(RV_STAGE_ROOT_DIR
      ${RV_APP_ROOT}/RV.app/Contents
      CACHE STRING "RV's build install root directory."
  )
  SET(RV_STAGE_BIN_DIR
      ${RV_STAGE_ROOT_DIR}/MacOS
  )
ELSE()
  SET(RV_STAGE_ROOT_DIR
      ${RV_APP_ROOT}
      CACHE STRING "RV's build output directory."
  )
  SET(RV_STAGE_BIN_DIR
      ${RV_STAGE_ROOT_DIR}/bin
  )
ENDIF()

FILE(MAKE_DIRECTORY ${RV_STAGE_ROOT_DIR})
MESSAGE(STATUS "RV_STAGE_ROOT_DIR: ${RV_STAGE_ROOT_DIR}")

IF(RV_TARGET_DARWIN)
  SET(RV_STAGE_FRAMEWORKS_DIR
      ${RV_STAGE_ROOT_DIR}/Frameworks
  )
  FILE(MAKE_DIRECTORY ${RV_STAGE_FRAMEWORKS_DIR})
  MESSAGE(STATUS "RV_STAGE_FRAMEWORKS_DIR: ${RV_STAGE_FRAMEWORKS_DIR}")
ENDIF()

IF(RV_TARGET_LINUX
   OR RV_TARGET_WINDOWS
)
  SET(RV_STAGE_SCRIPTS_DIR
      ${RV_STAGE_ROOT_DIR}/scripts
  )
  FILE(MAKE_DIRECTORY ${RV_STAGE_SCRIPTS_DIR})
ENDIF()

FILE(MAKE_DIRECTORY ${RV_STAGE_BIN_DIR})
MESSAGE(STATUS "RV_STAGE_BIN_DIR: ${RV_STAGE_BIN_DIR}")

SET(RV_STAGE_LIB_DIR
    ${RV_STAGE_ROOT_DIR}/lib
)
FILE(MAKE_DIRECTORY ${RV_STAGE_LIB_DIR})
MESSAGE(STATUS "RV_STAGE_LIB_DIR: ${RV_STAGE_LIB_DIR}")

SET(RV_STAGE_INCLUDE_DIR
    ${RV_STAGE_ROOT_DIR}/include
)
FILE(MAKE_DIRECTORY ${RV_STAGE_INCLUDE_DIR})
MESSAGE(STATUS "RV_STAGE_INCLUDE_DIR: ${RV_STAGE_INCLUDE_DIR}")

SET(RV_STAGE_SRC_DIR
    ${RV_STAGE_ROOT_DIR}/src
)
FILE(MAKE_DIRECTORY ${RV_STAGE_SRC_DIR})
MESSAGE(STATUS "RV_STAGE_SRC_DIR: ${RV_STAGE_SRC_DIR}")

IF(RV_TARGET_DARWIN)
  SET(RV_STAGE_RESOURCES_DIR
      ${RV_STAGE_ROOT_DIR}/Resources
  )
  SET(RV_STAGE_RESOURCES_ENGLISH_DIR
      ${RV_STAGE_RESOURCES_DIR}/English.lproj
  )
  FILE(MAKE_DIRECTORY ${RV_STAGE_RESOURCES_ENGLISH_DIR})
  MESSAGE(STATUS "RV_STAGE_RESOURCES_ENGLISH_DIR: ${RV_STAGE_RESOURCES_ENGLISH_DIR}")
ELSE()
  SET(RV_STAGE_RESOURCES_DIR
      ${RV_STAGE_ROOT_DIR}/resources
  )
ENDIF()

FILE(MAKE_DIRECTORY ${RV_STAGE_RESOURCES_DIR})
MESSAGE(STATUS "RV_STAGE_RESOURCES_DIR: ${RV_STAGE_RESOURCES_DIR}")

IF(RV_TARGET_LINUX)
  # For legacy reason, we keep the original lowercase name
  SET(RV_STAGE_PLUGINS_DIR
      ${RV_STAGE_ROOT_DIR}/plugins
  )
ELSE()
  SET(RV_STAGE_PLUGINS_DIR
      ${RV_STAGE_ROOT_DIR}/PlugIns
  )
ENDIF()
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_DIR: ${RV_STAGE_PLUGINS_DIR}")

SET(RV_STAGE_PLUGINS_CONFIGFILES_DIR
    ${RV_STAGE_PLUGINS_DIR}/ConfigFiles
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_CONFIGFILES_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_CONFIGFILES_DIR: ${RV_STAGE_PLUGINS_CONFIGFILES_DIR}")

SET(RV_STAGE_PLUGINS_IMAGEFORMATS_DIR
    ${RV_STAGE_PLUGINS_DIR}/ImageFormats
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_IMAGEFORMATS_DIR: ${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}")

SET(RV_STAGE_PLUGINS_MOVIEFORMATS_DIR
    ${RV_STAGE_PLUGINS_DIR}/MovieFormats
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_MOVIEFORMATS_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_MOVIEFORMATS_DIR: ${RV_STAGE_PLUGINS_MOVIEFORMATS_DIR}")

SET(RV_STAGE_PLUGINS_MU_DIR
    ${RV_STAGE_PLUGINS_DIR}/Mu
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_MU_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_MU_DIR: ${RV_STAGE_PLUGINS_MU_DIR}")

SET(RV_STAGE_PLUGINS_NODES_DIR
    ${RV_STAGE_PLUGINS_DIR}/Nodes
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_NODES_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_NODES_DIR: ${RV_STAGE_PLUGINS_NODES_DIR}")

SET(RV_STAGE_PLUGINS_OIIO_DIR
    ${RV_STAGE_PLUGINS_DIR}/OIIO
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_OIIO_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_OIIO_DIR: ${RV_STAGE_PLUGINS_OIIO_DIR}")

SET(RV_STAGE_PLUGINS_PACKAGES_DIR
    ${RV_STAGE_PLUGINS_DIR}/Packages
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_PACKAGES_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_PACKAGES_DIR: ${RV_STAGE_PLUGINS_PACKAGES_DIR}")

SET(RV_STAGE_PLUGINS_PROFILES_DIR
    ${RV_STAGE_PLUGINS_DIR}/Profiles
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_PROFILES_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_PROFILES_DIR: ${RV_STAGE_PLUGINS_PROFILES_DIR}")

SET(RV_STAGE_PLUGINS_PYTHON_DIR
    ${RV_STAGE_PLUGINS_DIR}/Python
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_PYTHON_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_PYTHON_DIR: ${RV_STAGE_PLUGINS_PYTHON_DIR}")

SET(RV_STAGE_PLUGINS_QT_DIR
    ${RV_STAGE_PLUGINS_DIR}/Qt
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_QT_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_QT_DIR: ${RV_STAGE_PLUGINS_QT_DIR}")

SET(RV_STAGE_PLUGINS_SUPPORTFILES_DIR
    ${RV_STAGE_PLUGINS_DIR}/SupportFiles
)
FILE(MAKE_DIRECTORY ${RV_STAGE_PLUGINS_SUPPORTFILES_DIR})
MESSAGE(STATUS "RV_STAGE_PLUGINS_SUPPORTFILES_DIR: ${RV_STAGE_PLUGINS_SUPPORTFILES_DIR}")

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    "${RV_STAGE_LIB_DIR}"
)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${RV_STAGE_LIB_DIR}"
)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${RV_STAGE_BIN_DIR}"
)
IF(RV_TARGET_WINDOWS)
  FOREACH(
    OUTPUTCONFIG
    ${CMAKE_CONFIGURATION_TYPES}
  )
    STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
        "${RV_STAGE_LIB_DIR}"
    )
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
        "${RV_STAGE_LIB_DIR}"
    )
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
        "${RV_STAGE_BIN_DIR}"
    )
  ENDFOREACH()
ENDIF()

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(RV_DEBUG_POSTFIX
      "_d"
  )
ENDIF()

SET(RV_COPYRIGHT_TEXT
    "Copyright Contributors to the Open RV Project"
    CACHE STRING "RV's copyright text."
)
