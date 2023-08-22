#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_ZLIB"
)

SET(_version
    "1.2.13"
)
SET(_download_url
    "https://github.com/madler/zlib/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    "fdedf0c8972a04a7c153dd73492d2d91"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)
SET(_include_dir
    ${_install_dir}/include
)

SET(_lib_dir
    ${_install_dir}/lib
)
SET(_bin_dir
    ${_install_dir}/bin
)

IF(RV_TARGET_WINDOWS)
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_zlibname
        "zlibd"
    )
  ELSE()
    SET(_zlibname
        "zlib"
    )
  ENDIF()
ELSE()
  SET(_zlibname
      "z"
  )
ENDIF()

SET(_libname
    ${CMAKE_SHARED_LIBRARY_PREFIX}${_zlibname}${CMAKE_SHARED_LIBRARY_SUFFIX}
)
SET(_libpath
    ${_lib_dir}/${_libname}
)

LIST(APPEND _zlib_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibname
      ${CMAKE_IMPORT_LIBRARY_PREFIX}${_zlibname}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_implibpath
      ${_lib_dir}/${_implibname}
  )
  LIST(APPEND _zlib_byproducts ${_implibpath})
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
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${_install_dir} -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
                    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${RV_DEPS_BASE_DIR}/${_target}/src
  BUILD_COMMAND ${CMAKE_COMMAND} --build ${RV_DEPS_BASE_DIR}/${_target}/src --parallel -v
  INSTALL_COMMAND ${CMAKE_COMMAND} --install ${RV_DEPS_BASE_DIR}/${_target}/src
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_zlib_byproducts}
  USES_TERMINAL_BUILD TRUE
)

FILE(MAKE_DIRECTORY "${_include_dir}")

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_bin_dir} ${RV_STAGE_BIN_DIR}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_libname}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_libname}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_libname}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(ZLIB::ZLIB SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(ZLIB::ZLIB ${_target})
SET_PROPERTY(
  TARGET ZLIB::ZLIB
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET ZLIB::ZLIB
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET ZLIB::ZLIB
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()
TARGET_INCLUDE_DIRECTORIES(
  ZLIB::ZLIB
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST ZLIB::ZLIB)

SET(RV_DEPS_ZLIB_INCLUDE_DIR
    ${_include_dir}
    CACHE STRING "Path to installed includes for ${_target}"
)

SET(RV_DEPS_ZLIB_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
