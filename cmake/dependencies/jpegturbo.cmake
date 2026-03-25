#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_JPEGTURBO" "${RV_DEPS_JPEGTURBO_VERSION}" "" "")

# libjpeg-turbo ships CMake CONFIG files that create libjpeg-turbo::jpeg and libjpeg-turbo::turbojpeg.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  libjpeg-turbo
  VERSION
  ${_version}
  DEPS_LIST_TARGETS
  libjpeg-turbo::jpeg
  libjpeg-turbo::turbojpeg
)

SET(_download_url
    "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_JPEGTURBO_DOWNLOAD_HASH}
)

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/jpegturbo.cmake)

  #
  # --- libjpeg-turbo::jpeg (build path)
  #
  ADD_LIBRARY(libjpeg-turbo::jpeg SHARED IMPORTED GLOBAL)
  ADD_DEPENDENCIES(libjpeg-turbo::jpeg ${_target})

  IF(NOT RV_TARGET_WINDOWS)
    SET_PROPERTY(
      TARGET libjpeg-turbo::jpeg
      PROPERTY IMPORTED_LOCATION ${_libjpegpath}
    )
    SET_PROPERTY(
      TARGET libjpeg-turbo::jpeg
      PROPERTY IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} ${_libjpegpath}
    )
  ELSE()
    SET_PROPERTY(
      TARGET libjpeg-turbo::jpeg
      PROPERTY IMPORTED_LOCATION ${_winlibjpegpath}
    )
    SET_PROPERTY(
      TARGET libjpeg-turbo::jpeg
      PROPERTY IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} ${_winlibjpegpath}
    )
    SET_PROPERTY(
      TARGET libjpeg-turbo::jpeg
      PROPERTY IMPORTED_IMPLIB ${_libjpegimplibpath}
    )
  ENDIF()

  FILE(MAKE_DIRECTORY "${_include_dir}")
  TARGET_INCLUDE_DIRECTORIES(
    libjpeg-turbo::jpeg
    INTERFACE ${_include_dir}
  )

  #
  # --- libjpeg-turbo::turbojpeg (build path)
  #
  ADD_LIBRARY(libjpeg-turbo::turbojpeg SHARED IMPORTED GLOBAL)
  ADD_DEPENDENCIES(libjpeg-turbo::turbojpeg ${_target})

  IF(NOT RV_TARGET_WINDOWS)
    SET_PROPERTY(
      TARGET libjpeg-turbo::turbojpeg
      PROPERTY IMPORTED_LOCATION ${_libturbojpegpath}
    )
    SET_PROPERTY(
      TARGET libjpeg-turbo::turbojpeg
      PROPERTY IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} ${_libturbojpegpath}
    )
  ELSE()
    SET_PROPERTY(
      TARGET libjpeg-turbo::turbojpeg
      PROPERTY IMPORTED_LOCATION ${_winlibturbojpegpath}
    )
    SET_PROPERTY(
      TARGET libjpeg-turbo::turbojpeg
      PROPERTY IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} ${_winlibturbojpegpath}
    )
    SET_PROPERTY(
      TARGET libjpeg-turbo::turbojpeg
      PROPERTY IMPORTED_IMPLIB ${_libturbojpegimplibpath}
    )
  ENDIF()

  TARGET_INCLUDE_DIRECTORIES(
    libjpeg-turbo::turbojpeg
    INTERFACE ${_include_dir}
  )

  LIST(APPEND RV_DEPS_LIST libjpeg-turbo::jpeg)
  LIST(APPEND RV_DEPS_LIST libjpeg-turbo::turbojpeg)
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS libjpeg-turbo::jpeg libjpeg-turbo::turbojpeg)
ENDIF()
