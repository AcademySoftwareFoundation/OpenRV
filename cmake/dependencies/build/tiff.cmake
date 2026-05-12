#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(zlib_library ZLIB::ZLIB IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(zlib_library ZLIB::ZLIB IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${zlib_include_dir}")
LIST(APPEND _configure_options "-DZLIB_LIBRARY=${zlib_library}")

IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(jpeg_library libjpeg-turbo::jpeg IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(jpeg_library libjpeg-turbo::jpeg IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(jpeg_include_dir libjpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${jpeg_include_dir}")
LIST(APPEND _configure_options "-DJPEG_LIBRARY=${jpeg_library}")

LIST(APPEND _configure_options "-Djpeg=ON")
LIST(APPEND _configure_options "-Dlzma=OFF")
LIST(APPEND _configure_options "-Dwebp=OFF")
LIST(APPEND _configure_options "-Dzlib=ON")
LIST(APPEND _configure_options "-Dzstd=OFF")

# Do not need TIFF tools.
LIST(APPEND _configure_options "-Dtiff-tools=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  DEPENDS ZLIB::ZLIB libjpeg-turbo::jpeg
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_libpath}
  USES_TERMINAL_BUILD TRUE
)

ADD_CUSTOM_COMMAND(
  TARGET ${_target}
  POST_BUILD
  COMMENT "Installing ${_target}'s missing headers"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_base_dir}/build/libtiff/tif_config.h ${_base_dir}/src/libtiff/tiffiop.h ${_base_dir}/src/libtiff/tif_dir.h
          ${_base_dir}/src/libtiff/tif_hash_set.h ${_include_dir}
)
