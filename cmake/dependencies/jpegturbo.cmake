#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
include(ProcessorCount)  # require CMake 3.15+
ProcessorCount(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_JPEGTURBO" "2.1.4" "make" "")
# RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/${_version}.tar.gz")

SET(_download_hash
    "357dc26a802c34387512a42697846d16"
)

# CMake is not generating debug postfix for JpegTurbo
RV_MAKE_STANDARD_LIB_NAME("jpeg" "62.3.0" "SHARED" "")
SET(_libjpegname ${_libname})
SET(_libjpegpath ${_libpath})

# CMake is not generating debug postfix for JpegTurbo
RV_MAKE_STANDARD_LIB_NAME("turbojpeg" "0.2.0" "SHARED" "")
SET(_libturbojpegname ${_libname})
SET(_libturbojpegpath ${_libpath})

SET(_byproducts "")
LIST(APPEND _byproducts ${_libjpegpath})
LIST(APPEND _byproducts ${_libturbojpegpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibjpegpath
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}jpeg${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_implibturbojpegpath
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}turbojpeg${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  LIST(APPEND _byproducts ${_implibjpegpath})
  LIST(APPEND _byproducts ${_implibturbojpegpath})
ENDIF()

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
#GET_TARGET_PROPERTY(_zlib_library ZLIB::ZLIB IMPORTED_LOCATION)
#GET_TARGET_PROPERTY(_zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
#LIST(APPEND _configure_options "-DZLIB_LIBRARY=${_zlib_library}")
#LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${_zlib_include_dir}")

#GET_TARGET_PROPERTY(_png_library PNG::PNG IMPORTED_LOCATION)
#GET_TARGET_PROPERTY(_png_include_dir PNG::PNG INTERFACE_INCLUDE_DIRECTORIES)
#LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
#LIST(APPEND _configure_options "-DPNG_INCLUDE_DIR=${_png_include_dir}")

#GET_TARGET_PROPERTY(_tiff_library Tiff::Tiff IMPORTED_LOCATION)
#GET_TARGET_PROPERTY(_tiff_include_dir Tiff::Tiff INTERFACE_INCLUDE_DIRECTORIES)
#LIST(APPEND _configure_options "-DTIFF_LIBRARY=${_tiff_library}")
#LIST(APPEND _configure_options "-DTIFF_INCLUDE_DIR=${_tiff_include_dir}")

EXTERNALPROJECT_ADD( ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND
        ${_make_command} -j${_cpu_count} -v
    INSTALL_COMMAND
        ${_make_command} install        
    BUILD_IN_SOURCE
        FALSE
    BUILD_ALWAYS
        FALSE
    BUILD_BYPRODUCTS
       ${_byproducts}
)

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

#
# --- JpegTurbo::Jpeg Library
#
add_library(jpeg-turbo::jpeg SHARED IMPORTED GLOBAL)
add_dependencies(jpeg-turbo::jpeg ${_target})
set_property(
    TARGET jpeg-turbo::jpeg
    PROPERTY IMPORTED_LOCATION ${_libjpegpath}
)
set_property(
    TARGET jpeg-turbo::jpeg
    PROPERTY IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} ${_libjpegpath}
)

FILE(MAKE_DIRECTORY "${_include_dir}")  # required at configure time for importing include path.
target_include_directories(jpeg-turbo::jpeg
    INTERFACE ${_include_dir}
)

#
# --- JpegTurbo::JpegTurbo Library
#
add_library(jpeg-turbo::turbojpeg SHARED IMPORTED GLOBAL)
add_dependencies(jpeg-turbo::turbojpeg ${_target})
set_property(
    TARGET jpeg-turbo::turbojpeg
    PROPERTY IMPORTED_LOCATION ${_libturbojpegpath}
)
set_property(
    TARGET jpeg-turbo::turbojpeg
    PROPERTY IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} ${_libturbojpegpath}
)
target_include_directories(jpeg-turbo::turbojpeg
    INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST jpeg-turbo::jpeg)
LIST(APPEND RV_DEPS_LIST jpeg-turbo::turbojpeg)
