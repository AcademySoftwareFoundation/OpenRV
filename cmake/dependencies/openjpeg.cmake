
#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Official sources: https://github.com/uclouvain/openjpeg
# Build instructions: https://github.com/uclouvain/openjpeg/blob/master/INSTALL.md

include(ProcessorCount)  # require CMake 3.15+
ProcessorCount(_cpu_count)

# version 2+ requires changes to IOjp2 project
RV_CREATE_STANDARD_DEPS_VARIABLES( "RV_DEPS_OPENJPEG" "2.5.0" "make" "")
IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' 
  # for some reason, this CMake-based project isn't using lib64
  SET(_lib_dir ${_install_dir}/lib)
ENDIF()
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url "https://github.com/uclouvain/openjpeg/archive/refs/tags/v${_version}.tar.gz")
SET(_download_hash "5cbb822a1203dd75b85639da4f4ecaab")

RV_MAKE_STANDARD_LIB_NAME("openjp2" "2.5.0" "SHARED" "")

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
LIST(APPEND _configure_options "-DBUILD_THIRDPARTY=1")  # to build LCMS2

GET_TARGET_PROPERTY(_zlib_library ZLIB::ZLIB IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DZLIB_LIBRARY=${_zlib_library}")
LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${_zlib_include_dir}")

GET_TARGET_PROPERTY(_png_library PNG::PNG IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_png_include_dir PNG::PNG INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
LIST(APPEND _configure_options "-DPNG_INCLUDE_DIR=${_png_include_dir}")

GET_TARGET_PROPERTY(_tiff_library Tiff::Tiff IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_tiff_include_dir Tiff::Tiff INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DTIFF_LIBRARY=${_tiff_library}")
LIST(APPEND _configure_options "-DTIFF_INCLUDE_DIR=${_tiff_include_dir}")

EXTERNALPROJECT_ADD( ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  DEPENDS ZLIB::ZLIB Tiff::Tiff PNG::PNG
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND
        ${_make_command} -j${_cpu_count} -v
    INSTALL_COMMAND
        ${_make_command} install        
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(OpenJpeg::OpenJpeg SHARED IMPORTED GLOBAL)
SET_PROPERTY(
    TARGET OpenJpeg::OpenJpeg
    PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET OpenJpeg::OpenJpeg
  PROPERTY IMPORTED_SONAME ${_libname}
)

# It is required to force directory creation at configure time
# otherwise CMake complains about importing a non-existing path
SET(_openjpeg_include_dir "${_include_dir}/openjpeg-2.5")
FILE(MAKE_DIRECTORY "${_openjpeg_include_dir}")
TARGET_INCLUDE_DIRECTORIES(OpenJpeg::OpenJpeg 
    INTERFACE ${_openjpeg_include_dir}
)

LIST(APPEND RV_DEPS_LIST OpenJpeg::OpenJpeg)
