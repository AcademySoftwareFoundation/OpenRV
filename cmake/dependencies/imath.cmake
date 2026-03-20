#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMATH" "${RV_DEPS_IMATH_VERSION}" "" "")

IF(RV_USE_BREW_DEPS)
  FIND_PACKAGE(Imath CONFIG)
  IF(Imath_FOUND)
    MESSAGE(STATUS "Using Homebrew Imath: ${Imath_VERSION}")

    # Helper macro to promote target to global
    MACRO(PROMOTE_TO_GLOBAL target)
      IF(TARGET ${target})
        GET_TARGET_PROPERTY(_is_imported ${target} IMPORTED)
        IF(_is_imported)
          SET_TARGET_PROPERTIES(
            ${target}
            PROPERTIES IMPORTED_GLOBAL TRUE
          )
        ENDIF()
      ENDIF()
    ENDMACRO()

    IF(TARGET Imath::Imath)
      PROMOTE_TO_GLOBAL(Imath::Imath)
      LIST(APPEND RV_DEPS_LIST Imath::Imath)
    ELSEIF(TARGET Imath::imath)
      PROMOTE_TO_GLOBAL(Imath::imath)
      ADD_LIBRARY(Imath::Imath ALIAS Imath::imath)
      LIST(APPEND RV_DEPS_LIST Imath::Imath)
    ELSE()
      # Fallback using variables
      ADD_LIBRARY(Imath::Imath UNKNOWN IMPORTED GLOBAL)
      SET_PROPERTY(
        TARGET Imath::Imath
        PROPERTY IMPORTED_LOCATION "${Imath_LIBRARY}"
      )
      TARGET_INCLUDE_DIRECTORIES(
        Imath::Imath
        INTERFACE "${Imath_INCLUDE_DIR}"
      )
      LIST(APPEND RV_DEPS_LIST Imath::Imath)
    ENDIF()

    SET(RV_DEPS_IMATH_VERSION
        "${Imath_VERSION}"
        CACHE INTERNAL "" FORCE
    )
    SET(RV_DEPS_IMATH_ROOT_DIR
        "${Imath_DIR}/../.."
        CACHE INTERNAL "" FORCE
    )
    SET(RV_DEPS_IMATH_CMAKE_DIR
        "${Imath_DIR}"
        CACHE STRING "" FORCE
    )
    RETURN()
  ENDIF()
ENDIF()

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    "${RV_DEPS_IMATH_DOWNLOAD_HASH}"
)

SET(_include_dir
    ${_install_dir}/include/Imath
)

IF(RV_TARGET_DARWIN)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}.${RV_DEPS_IMATH_LIB_VER}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_IMATH_LIB_VER}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(RV_DEPS_IMATH_CMAKE_DIR
    ${_lib_dir}/cmake/Imath
    CACHE STRING "Path to Imath CMake files ${_target}"
)

SET(RV_DEPS_IMATH_CMAKE_DIR
    ${_lib_dir}/cmake/Imath
)

SET(_libpath
    ${_lib_dir}/${_libname}
)

LIST(APPEND _imath_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibpath
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  LIST(APPEND _imath_byproducts ${_implibpath})
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_imath_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

RV_ADD_IMPORTED_LIBRARY(
  NAME
  Imath::Imath
  TYPE
  SHARED
  LOCATION
  ${_libpath}
  SONAME
  ${_libname}
  IMPLIB
  ${_implibpath}
  INCLUDE_DIRS
  ${_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)
