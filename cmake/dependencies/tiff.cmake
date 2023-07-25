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
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_TIFF" "4.0.9" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://download.osgeo.org/libtiff/tiff-${_version}.tar.gz"
)

SET(_download_hash
    "54bad211279cc93eb4fca31ba9bfdc79"
)

IF(NOT RV_TARGET_WINDOWS)
  # Mac/Linux: Use the unversionned .dylib.so LINK which points to the proper file (which has a diff version number) Mac/Linux: TIFF doesn't use a Postfix only
  # 'MSVC'.
  IF(RV_TARGET_DARWIN)
    RV_MAKE_STANDARD_LIB_NAME("tiff" "5.3.0" "SHARED" "")
  ELSE()
    RV_MAKE_STANDARD_LIB_NAME("tiff" "" "SHARED" "")
  ENDIF()
ELSE()
  # The current CMake build code via NMake doesn't create a Debug lib named "libtiffd.lib"
  RV_MAKE_STANDARD_LIB_NAME("libtiff" "${_version}" "SHARED" "")
ENDIF()
# ByProducts note: Windows will only have the DLL in _byproducts, this is fine since both .lib and .dll will be updated together.

IF(RV_TARGET_WINDOWS)
  #
  # On Windows build, the options are in the cmake/patches/nmake.opt' file. Hence to toggle the options in nmake.opt, we have a patch in cmake/patches to change
  # it. UPDATE NOTE: When upating TIFF, update the patch in cmake/patches.

  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_patch_path "${RV_PATCHES_DIR}/tiff_nmake_dbg.opt")
  ELSE()
    SET(_patch_path "${RV_PATCHES_DIR}/tiff_nmake_rel.opt")
  ENDIF()

  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS ZLIB::ZLIB jpeg-turbo::jpeg
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${_patch_path} ${_source_dir}/nmake.opt
    CONFIGURE_COMMAND ""
    BUILD_COMMAND nmake /f Makefile.vc
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )

  # prepare directories for copy. <cmake> -E copy expects the directories to exist otherwise copy won't work correctly. WARNING: CMAKE 3.26: if directories do
  # not exist, <cmake> -E copy will copy the FILE as a FILE that looks like a directory; no real directory will be created
  FILE(MAKE_DIRECTORY ${_lib_dir})
  FILE(MAKE_DIRECTORY ${_bin_dir})
  # Building with nmake isn't providing a nice install target, we need to do it manually
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/libtiff.lib ${_lib_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/libtiff.dll ${_bin_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tiff.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tiffio.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tiffiop.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tiffio.hxx ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tiffvers.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tiffconf.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tif_config.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/libtiff/tif_dir.h ${_include_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_base_dir}/src/tools ${_bin_dir}
    COMMAND ${CMAKE_COMMAND} -E rm ${_bin_dir}/Makefile.am ${_bin_dir}/Makefile.vc ${_bin_dir}/Makefile.in ${_bin_dir}/CMakeLists.txt
  )

ELSE()

  # Linux & macOS Explicitely disabling LZMA support until we have updated build system and documentation
  LIST(APPEND _configure_options "-Dlzma=OFF")

  LIST(APPEND _configure_options "-Dzlib=ON")
  GET_TARGET_PROPERTY(_root_library ZLIB::ZLIB IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_root_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
  LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${_root_include_dir}")
  LIST(APPEND _configure_options "-DZLIB_LIBRARY=${_root_library}")

  LIST(APPEND _configure_options "-Djpeg=ON")
  SET(_jpeg_library
      ""
  )
  SET(_jpeg_include_dir
      ""
  )
  GET_TARGET_PROPERTY(_jpeg_library jpeg-turbo::jpeg IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_jpeg_include_dir jpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
  LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${_jpeg_include_dir}")
  LIST(APPEND _configure_options "-DJPEG_LIBRARY=${_jpeg_library}")

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
    DEPENDS ZLIB::ZLIB RV_DEPS_JPEGTURBO
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${_make_command} -j${_cpu_count}
    INSTALL_COMMAND ${_make_command} install
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_libpath}
    USES_TERMINAL_BUILD TRUE
  )

  # Adding missing files that was there in src/pub/tiff and that IOTiff needs
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s missing headers"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_base_dir}/build/libtiff/tif_config.h ${_base_dir}/src/libtiff/tiffiop.h ${_base_dir}/src/libtiff/tif_dir.h
            ${_include_dir}
  )
ENDIF()

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
  SET_PROPERTY(
    TARGET Tiff::Tiff
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
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
