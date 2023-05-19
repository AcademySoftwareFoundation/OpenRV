#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

ADD_CUSTOM_TARGET(dependencies)
SET(RV_DEPS_LIST
    ""
    CACHE INTERNAL ""
)

FIND_PACKAGE(Git QUIET)
IF(GIT_FOUND
   AND EXISTS "${PROJECT_SOURCE_DIR}/.git"
)
  # Update submodules as needed
  OPTION(GIT_SUBMODULE "Check submodules during build" ON)
  IF(GIT_SUBMODULE)
    MESSAGE(STATUS "Updating submodules")
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    IF(NOT GIT_SUBMOD_RESULT EQUAL "0")
      MESSAGE(WARNING "Unable to init or update git submodules (${GIT_SUBMOD_RESULT})")
    ENDIF()
  ENDIF()
ENDIF()

IF(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/pub/CMakeLists.txt")
  MESSAGE(FATAL_ERROR "Unable to find required git submodules. GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
ENDIF()

INCLUDE(qt5)
INCLUDE(openssl) # Python3 requirement
INCLUDE(python3)
INCLUDE(boost)
INCLUDE(dav1d)
IF(RV_TARGET_WINDOWS)
  INCLUDE(atomic_ops)
ENDIF()
IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  INCLUDE(gc)
  INCLUDE(glew)
ENDIF()
INCLUDE(imath)
INCLUDE(zlib)
INCLUDE(openexr)
INCLUDE(ffmpeg)
INCLUDE(doctest)
# INCLUDE(mesagl)

LIST(REMOVE_DUPLICATES RV_DEPS_LIST)
SET(RV_DEPS_LIST
    ${RV_DEPS_LIST}
    CACHE INTERNAL ""
)

MESSAGE(STATUS "Using Boost:        ${RV_DEPS_BOOST_VERSION}")
MESSAGE(STATUS "Using Dav1d:        ${RV_DEPS_DAV1D_VERSION}")
MESSAGE(STATUS "Using FFMPEG:       ${RV_DEPS_FFMPEG_VERSION}")
IF(RV_TARGET_WINDOWS)
  MESSAGE(STATUS "Using atomic_ops:   ${RV_DEPS_ATOMIC_OPS_VERSION}")
ENDIF()
IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  MESSAGE(STATUS "Using GC:           ${RV_DEPS_GC_VERSION}")
  MESSAGE(STATUS "Using GLEW:         ${RV_DEPS_GLEW_VERSION}")
ENDIF()
MESSAGE(STATUS "Using Imath:        ${RV_DEPS_IMATH_VERSION}")
MESSAGE(STATUS "Using OpenEXR:      ${RV_DEPS_OPENEXR_VERSION}")
MESSAGE(STATUS "Using Python3:      ${RV_DEPS_PYTHON3_VERSION}")
MESSAGE(STATUS "Using PySide2:      ${RV_DEPS_PYSIDE2_VERSION}")
MESSAGE(STATUS "Using Qt5:          ${RV_DEPS_QT5_VERSION}")
MESSAGE(STATUS "Using zlib:         ${RV_DEPS_ZLIB_VERSION}")
# MESSAGE(STATUS "Using MesaGL:       ${RV_DEPS_MESAGL_VERSION}")
