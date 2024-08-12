#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [libtiff 3.6 -- Old libtiff Webpage](http://www.libtiff.org/)
#
# [libtiff 3.9-4.5](https://download.osgeo.org/libtiff/)
#
# [libtiff 4.4](https://conan.io/center/libtiff)
#
# [libtiff 4.5](http://www.simplesystems.org/libtiff)
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

# OpenImageIO required >= 3.9, using latest 4.0
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_TIFF" "4.6.0" "" "")

SET(_download_url
    "https://gitlab.com/libtiff/libtiff/-/archive/v${_version}/libtiff-v${_version}.tar.gz"
)

SET(_download_hash
    "118a2e5fc9ed71653195b332b9715890"
)

IF(NOT RV_TARGET_WINDOWS)
  # Mac/Linux: Use the unversionned .dylib.so LINK which points to the proper file (which has a diff version number) Mac/Linux: TIFF doesn't use a Postfix only
  # 'MSVC'.
  IF(RV_TARGET_DARWIN)
    RV_MAKE_STANDARD_LIB_NAME("tiff" "6.0.2" "SHARED" "")
  ELSE()
    RV_MAKE_STANDARD_LIB_NAME("tiff" "" "SHARED" "")
  ENDIF()
ELSE()
  # The current CMake build code via NMake doesn't create a Debug lib named "libtiffd.lib"
  RV_MAKE_STANDARD_LIB_NAME("libtiff" "${_version}" "SHARED" "")
ENDIF()
# ByProducts note: Windows will only have the DLL in _byproducts, this is fine since both .lib and .dll will be updated together.

IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(zlib_library ZLIB::ZLIB IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(zlib_library ZLIB::ZLIB IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${zlib_include_dir}")
LIST(APPEND _configure_options "-DZLIB_LIBRARY=${zlib_library}")


IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(jpeg_library jpeg-turbo::jpeg IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(jpeg_library jpeg-turbo::jpeg IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(jpeg_include_dir jpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${jpeg_include_dir}")
LIST(APPEND _configure_options "-DJPEG_LIBRARY=${jpeg_library}")

LIST(APPEND _configure_options "-Djpeg=ON")
LIST(APPEND _configure_options "-Dlzma=OFF")
LIST(APPEND _configure_options "-Dzlib=ON")
LIST(APPEND _configure_options "-Dwebp=OFF")

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
  DEPENDS ZLIB::ZLIB jpeg-turbo::jpeg
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

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(Tiff::Tiff SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(Tiff::Tiff ${_target})
SET_PROPERTY(
  TARGET Tiff::Tiff
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
IF(RV_TARGET_WINDOWS)
  IF(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    SET(_tiff_lib_name "tiff.lib")
  ELSEIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(_tiff_lib_name "tiffd.lib")
  ENDIF()

  SET_PROPERTY(
    TARGET Tiff::Tiff
    PROPERTY IMPORTED_IMPLIB ${_lib_dir}/${_tiff_lib_name}
  )
ELSE()
  SET_PROPERTY(
    TARGET Tiff::Tiff
    PROPERTY IMPORTED_SONAME ${_libname}
  )
ENDIF()

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  Tiff::Tiff
  INTERFACE ${_include_dir}
)

TARGET_LINK_LIBRARIES(
  Tiff::Tiff
  INTERFACE ZLIB::ZLIB jpeg-turbo::jpeg
)

LIST(APPEND RV_DEPS_LIST Tiff::Tiff)
