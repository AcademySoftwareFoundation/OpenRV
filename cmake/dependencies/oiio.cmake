#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# [OIIO -- Sources](https://github.com/OpenImageIO/oiio)
#
# [OIIO -- Documentation](https://openimageio.readthedocs.io/en/v2.4.7.1/)
#
# [OIIO -- Build instructions](https://github.com/OpenImageIO/oiio/blob/master/INSTALL.md)
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OIIO" "2.4.6.0" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/OpenImageIO/archive/refs/tags/v${_version}.tar.gz"
)
SET(_download_hash
    "628c588112ce8e08f31ec3417eb6828d"
)

RV_MAKE_STANDARD_LIB_NAME("OpenImageIO_Util" "2.4.6" "SHARED" "${RV_DEBUG_POSTFIX}")
SET(_byprojects_copy
    ${_byproducts}
)
SET(_oiio_utils_libname
    ${_libname}
)
SET(_oiio_utils_libpath
    ${_libpath}
)
SET(_oiio_utils_implibpath
    ${_implibpath}
)
RV_MAKE_STANDARD_LIB_NAME("OpenImageIO" "2.4.6" "SHARED" "${RV_DEBUG_POSTFIX}")
LIST(APPEND _byproducts ${_byprojects_copy})

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
LIST(APPEND _configure_options "-DBUILD_TESTING=OFF")
LIST(APPEND _configure_options "-DUSE_PYTHON=0") # this on would requireextra pybind11 package
LIST(APPEND _configure_options "-DUSE_OCIO=0")
LIST(APPEND _configure_options "-DUSE_FREETYPE=0")
LIST(APPEND _configure_options "-DUSE_GIF=OFF")

LIST(APPEND _configure_options "-DBoost_ROOT=${RV_DEPS_BOOST_ROOT_DIR}")
LIST(APPEND _configure_options "-DOpenEXR_ROOT=${RV_DEPS_OPENEXR_ROOT_DIR}")

IF(NOT RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(_imath_library Imath::Imath IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_imath_include_dir Imath::Imath INTERFACE_INCLUDE_DIRECTORIES)
  LIST(APPEND _configure_options "-DImath_LIBRARY=${_imath_library}")
  LIST(APPEND _configure_options "-DImath_INCLUDE_DIR=${_imath_include_dir}/..")
  GET_FILENAME_COMPONENT(_imath_library_path ${_imath_library} DIRECTORY)
  LIST(APPEND _configure_options "-DImath_DIR=${_imath_library_path}/cmake/Imath")
ELSE()
  # Must point to the IMath-config.cmake file which is a 'FindIMath.cmake' type of file.
  LIST(APPEND _configure_options "-DImath_DIR=${RV_DEPS_IMATH_ROOT_DIR}/lib/cmake/Imath")
ENDIF()

GET_TARGET_PROPERTY(_png_library PNG::PNG IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_png_include_dir PNG::PNG INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DPNG_LIBRARY=${_png_library}")
LIST(APPEND _configure_options "-DPNG_PNG_INCLUDE_DIR=${_png_include_dir}")

IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(_jpeg_library jpeg-turbo::jpeg IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(_jpeg_library jpeg-turbo::jpeg IMPORTED_LOCATION)
ENDIF()
GET_TARGET_PROPERTY(_jpeg_include_dir jpeg-turbo::jpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEG_LIBRARY=${_jpeg_library}")
LIST(APPEND _configure_options "-DJPEG_INCLUDE_DIR=${_jpeg_include_dir}")

GET_TARGET_PROPERTY(_jpegturbo_library jpeg-turbo::turbojpeg IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_jpegturbo_include_dir jpeg-turbo::turbojpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DJPEGTURBO_LIBRARY=${_jpegturbo_library}")
LIST(APPEND _configure_options "-DJPEGTURBO_INCLUDE_DIR=${_jpegturbo_include_dir}")

LIST(APPEND _configure_options "-DOpenJPEG_ROOT=${RV_DEPS_OPENJPEG_ROOT_DIR}")
GET_TARGET_PROPERTY(_openjpeg_library OpenJpeg::OpenJpeg IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_openjpeg_include_dir OpenJpeg::OpenJpeg INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DOPENJPEG_OPENJP2_LIBRARY=${_openjpeg_library}")
LIST(APPEND _configure_options "-DOPENJPEG_INCLUDE_DIR=${_openjpeg_include_dir}")

LIST(APPEND _configure_options "-DTIFF_ROOT=${RV_DEPS_TIFF_ROOT_DIR}")

GET_TARGET_PROPERTY(_ffmpeg_include_dir ffmpeg::avcodec INTERFACE_INCLUDE_DIRECTORIES)
IF(RV_TARGET_WINDOWS)
  GET_TARGET_PROPERTY(_ffmpeg_libavcodec ffmpeg::avcodec IMPORTED_IMPLIB)
  GET_TARGET_PROPERTY(_ffmpeg_libavformat ffmpeg::avformat IMPORTED_IMPLIB)
  GET_TARGET_PROPERTY(_ffmpeg_libavutil ffmpeg::avutil IMPORTED_IMPLIB)
  GET_TARGET_PROPERTY(_ffmpeg_libswscale ffmpeg::swscale IMPORTED_IMPLIB)
ELSE()
  GET_TARGET_PROPERTY(_ffmpeg_libavcodec ffmpeg::avcodec IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_ffmpeg_libavformat ffmpeg::avformat IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_ffmpeg_libavutil ffmpeg::avutil IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_ffmpeg_libswscale ffmpeg::swscale IMPORTED_LOCATION)
ENDIF()
LIST(APPEND _configure_options "-DFFMPEG_INCLUDE_DIR=${_ffmpeg_include_dir}")
LIST(APPEND _configure_options "-DFFMPEG_INCLUDES=${_ffmpeg_include_dir}")
LIST(APPEND _configure_options "-DFFMPEG_AVCODEC_INCLUDE_DIR=${_ffmpeg_include_dir}")
LIST(APPEND _configure_options "-DFFMPEG_LIBRARIES=${_ffmpeg_libavcodec} ${_ffmpeg_libavformat} ${_ffmpeg_libavutil} ${_ffmpeg_libswscale}")
LIST(APPEND _configure_options "-DFFMPEG_LIBAVCODEC=${_ffmpeg_libavcodec}")
LIST(APPEND _configure_options "-DFFMPEG_LIBAVFORMAT=${_ffmpeg_libavformat}")
LIST(APPEND _configure_options "-DFFMPEG_LIBAVUTIL=${_ffmpeg_libavutil}")
LIST(APPEND _configure_options "-DFFMPEG_LIBSWSCALE=${_ffmpeg_libswscale}")

IF(RV_TARGET_LINUX)
  MESSAGE(STATUS "Building OpenImageIO using system's freetype library.")
  SET(_depends_freetype
      ""
  )
ELSE()
  SET(_depends_freetype
      freetype
  )
  GET_TARGET_PROPERTY(_freetype_library freetype IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_freetype_include_dir freetype INTERFACE_INCLUDE_DIRECTORIES)
  LIST(APPEND _configure_options "-DFREETYPE_LIBRARY=${_freetype_library}")
  LIST(APPEND _configure_options "-DFREETYPE_INCLUDE_DIR=${_freetype_include_dir}")
  MESSAGE(DEBUG "OIIO: _freetype_library='${_freetype_library}'")
  MESSAGE(DEBUG "OIIO: _freetype_include_dir='${_freetype_include_dir}'")
ENDIF()

LIST(APPEND _configure_options "-DLibRaw_ROOT=${RV_DEPS_RAW_ROOT_DIR}")

LIST(APPEND _configure_options "-DQt5_ROOT=${RV_DEPS_QT5_LOCATION}")

IF(NOT RV_TARGET_LINUX)
  LIST(APPEND _configure_options "-DWebP_ROOT=${RV_DEPS_WEBP_ROOT_DIR}")
  # Linux has a Link error related to relocation; WebP appears not built with -fPIC. Hence OIIO will build WebP itself.
ENDIF()
LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")

# OIIO tools are not needed.
LIST(APPEND _configure_options "-DOIIO_BUILD_TOOLS=OFF" "-DOIIO_BUILD_TESTS=OFF")

IF(RV_TARGET_WINDOWS)
  LIST(PREPEND _configure_options "-G ${CMAKE_GENERATOR}")
ENDIF()

IF(NOT RV_TARGET_WINDOWS)
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
    DEPENDS ${_depends_freetype}
            jpeg-turbo::jpeg
            Tiff::Tiff
            OpenEXR::OpenEXR
            OpenJpeg::OpenJpeg
            jpeg-turbo::turbojpeg
            PNG::PNG
            Boost::headers
            Boost::thread
            Boost::filesystem
            Imath::Imath
            Webp::Webp
            Raw::Raw
            ffmpeg::swresample
            ffmpeg::swscale
            ffmpeg::avcodec
            ffmpeg::swresample
            ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${_cmake_build_command}
    INSTALL_COMMAND ${_cmake_install_command}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ELSE()
  LIST(
    APPEND
    _oiio_build_options
    "--build"
    "${_build_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}"
    "--parallel"
    "${_cpu_count}"
  )
  LIST(
    APPEND
    _oiio_install_options
    "--install"
    "${_build_dir}"
    "--prefix"
    "${_install_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}" # for --config
  )

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
    DEPENDS ${_depends_freetype}
            jpeg-turbo::jpeg
            Tiff::Tiff
            OpenEXR::OpenEXR
            OpenJpeg::OpenJpeg
            jpeg-turbo::turbojpeg
            PNG::PNG
            Boost::headers
            Boost::thread
            Boost::filesystem
            Imath::Imath
            Webp::Webp
            Raw::Raw
            ffmpeg::swresample
            ffmpeg::swscale
            ffmpeg::avcodec
            ffmpeg::swresample
            ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${CMAKE_COMMAND} ${_oiio_build_options}
    INSTALL_COMMAND ${CMAKE_COMMAND} ${_oiio_install_options}
    BUILD_IN_SOURCE FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ENDIF()

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(oiio::oiio SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(oiio::oiio ${_target})
SET_PROPERTY(
  TARGET oiio::oiio
  PROPERTY IMPORTED_LOCATION ${_libpath}
)

SET_PROPERTY(
  TARGET oiio::oiio
  PROPERTY IMPORTED_SONAME ${_libname}
)

IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET oiio::oiio
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  oiio::oiio
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST oiio::oiio)

ADD_LIBRARY(oiio::utils SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(oiio::utils ${_target})
SET_PROPERTY(
  TARGET oiio::utils
  PROPERTY IMPORTED_LOCATION ${_oiio_utils_libpath}
)

SET_PROPERTY(
  TARGET oiio::utils
  PROPERTY IMPORTED_SONAME ${_oiio_utils_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET oiio::utils
    PROPERTY IMPORTED_IMPLIB ${_oiio_utils_implibpath}
  )
ENDIF()

LIST(APPEND RV_DEPS_LIST oiio::utils)
