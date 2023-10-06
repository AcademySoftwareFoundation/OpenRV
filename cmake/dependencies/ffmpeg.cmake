#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_FFMPEG"
)

SET(_version
    "n4.4.3"
)
SET(_download_url
    "https://github.com/FFmpeg/FFmpeg/archive/refs/tags/${_version}.zip"
)

SET(_download_hash
    "51ffa9de9e5b0c17cbabc0d8b780beb2"
)

SET(_base_dir
    ${RV_DEPS_BASE_DIR}/${_target}
)
SET(_install_dir
    ${_base_dir}/install
)

SET(${_target}_ROOT_DIR
    ${_install_dir}
)

SET(_make_command
    make
)
SET(_configure_command
    sh ./configure
)

IF(${RV_OSX_EMULATION})
  SET(_darwin_x86_64
      "arch" "${RV_OSX_EMULATION_ARCH}"
  )

  SET(_make_command
      ${_darwin_x86_64} ${_make_command}
  )
  SET(_configure_command
      ${_darwin_x86_64} ${_configure_command}
  )
ENDIF()

SET(_include_dir
    ${_install_dir}/include
)
IF(RV_TARGET_WINDOWS)
  SET(_lib_dir
      ${_install_dir}/bin
  )
ELSE()
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  SET(_ffmpeg_avutil_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avutil.56${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swresample_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swresample.3${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swscale_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swscale.5${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avcodec_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec.58${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avformat_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avformat.58${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_ffmpeg_avutil_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}.56
  )
  SET(_ffmpeg_swresample_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}.3
  )
  SET(_ffmpeg_swscale_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX}.5
  )
  SET(_ffmpeg_avcodec_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}.58
  )
  SET(_ffmpeg_avformat_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}.58
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_ffmpeg_avutil_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avutil-56${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swresample_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swresample-3${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swscale_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swscale-5${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avcodec_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec-58${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avformat_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avformat-58${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_ffmpeg_libs
    avutil swresample swscale avcodec avformat
)

FOREACH(
  _ffmpeg_lib
  ${_ffmpeg_libs}
)
  SET(_ffmpeg_${_ffmpeg_lib}_lib
      ${_lib_dir}/${_ffmpeg_${_ffmpeg_lib}_lib_name}
  )
  LIST(APPEND _build_byproducts ${_ffmpeg_${_ffmpeg_lib}_lib})
  IF(RV_TARGET_WINDOWS)
    SET(_ffmpeg_${_ffmpeg_lib}_implib
        ${_lib_dir}/${CMAKE_IMPORT_LIBRARY_PREFIX}${_ffmpeg_lib}${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    LIST(APPEND _build_byproducts ${_ffmpeg_${_ffmpeg_lib}_implib})
  ENDIF()
ENDFOREACH()

# Make a list of common FFmpeg config options
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--enable-shared")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-static")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-iconv")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-outdevs")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-programs")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-large-tests")
IF(RV_TARGET_WINDOWS)
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--toolchain=msvc")
ENDIF()
SET(RV_FFMPEG_COMMON_CONFIG_OPTIONS
    ${RV_FFMPEG_COMMON_CONFIG_OPTIONS}
    CACHE INTERNAL ""
)

# Make a list of the Open RV's FFmpeg config options unless already customized. Note that a super project, a project consuming Open RV as a submodule, can
# customize the FFmpeg config options via the RV_FFMPEG_CONFIG_OPTIONS cmake variable.
IF(NOT DEFINED RV_FFMPEG_CONFIG_OPTIONS)
  LIST(APPEND _disabled_decoders "--disable-decoder=bink")
  LIST(APPEND _disabled_decoders "--disable-decoder=binkaudio_dct")
  LIST(APPEND _disabled_decoders "--disable-decoder=binkaudio_rdft")
  LIST(APPEND _disabled_decoders "--disable-decoder=vp9")
  LIST(APPEND _disabled_decoders "--disable-decoder=vp9_cuvid")
  LIST(APPEND _disabled_decoders "--disable-decoder=vp9_mediacodec")
  LIST(APPEND _disabled_decoders "--disable-decoder=vp9_qsv")
  LIST(APPEND _disabled_decoders "--disable-decoder=vp9_rkmpp")
  LIST(APPEND _disabled_decoders "--disable-decoder=vp9_v4l2m2m")
  LIST(APPEND _disabled_decoders "--disable-decoder=dnxhd")
  LIST(APPEND _disabled_decoders "--disable-decoder=prores")
  LIST(APPEND _disabled_decoders "--disable-decoder=qtrle")
  LIST(APPEND _disabled_decoders "--disable-decoder=aac")
  LIST(APPEND _disabled_decoders "--disable-decoder=aac_fixed")
  LIST(APPEND _disabled_decoders "--disable-decoder=aac_latm")
  LIST(APPEND _disabled_decoders "--disable-decoder=dvvideo")

  LIST(APPEND _disabled_encoders "--disable-encoder=dnxhd")
  LIST(APPEND _disabled_encoders "--disable-encoder=prores")
  LIST(APPEND _disabled_encoders "--disable-encoder=qtrle")
  LIST(APPEND _disabled_encoders "--disable-encoder=aac")
  LIST(APPEND _disabled_encoders "--disable-encoder=aac_mf")
  LIST(APPEND _disabled_encoders "--disable-encoder=vp9_qsv")
  LIST(APPEND _disabled_encoders "--disable-encoder=vp9_vaapi")
  LIST(APPEND _disabled_encoders "--disable-encoder=dvvideo")

  LIST(APPEND _disabled_parsers "--disable-parser=vp9")

  LIST(APPEND _disabled_filters "--disable-filter=geq")

  LIST(APPEND _disabled_protocols "--disable-protocol=ffrtmpcrypt")
  LIST(APPEND _disabled_protocols "--disable-protocol=rtmpe")
  LIST(APPEND _disabled_protocols "--disable-protocol=rtmpte")

  SET(RV_FFMPEG_CONFIG_OPTIONS
      "${_disabled_decoders} ${_disabled_encoders} ${_disabled_filters} ${_disabled_parsers} ${_disabled_protocols}"
      CACHE INTERNAL ""
  )
ENDIF()

LIST(REMOVE_DUPLICATES RV_FFMPEG_DEPENDS)
SET(RV_FFMPEG_DEPENDS
    ${RV_FFMPEG_DEPENDS}
    CACHE INTERNAL ""
)

LIST(REMOVE_DUPLICATES RV_FFMPEG_EXTRA_C_OPTIONS)
SET(RV_FFMPEG_EXTRA_C_OPTIONS
    ${RV_FFMPEG_EXTRA_C_OPTIONS}
    CACHE INTERNAL ""
)

LIST(REMOVE_DUPLICATES RV_FFMPEG_EXTRA_LIBPATH_OPTIONS)
SET(RV_FFMPEG_EXTRA_LIBPATH_OPTIONS
    ${RV_FFMPEG_EXTRA_LIBPATH_OPTIONS}
    CACHE INTERNAL ""
)

LIST(REMOVE_DUPLICATES RV_FFMPEG_EXTERNAL_LIBS)
SET(RV_FFMPEG_EXTERNAL_LIBS
    ${RV_FFMPEG_EXTERNAL_LIBS}
    CACHE INTERNAL ""
)

SET(_ffmpeg_david_cmake_lib_dir_path
    "${RV_DEPS_DAVID_LIB_DIR}"
)
IF(RV_TARGET_WINDOWS)
  # Changing path start from "c:/..." to "/c/..." and replacing all backslashes with slashes since PkgConfig wants a linux path
  STRING(REPLACE "\\" "/" _ffmpeg_david_cmake_lib_dir_path "${_ffmpeg_david_cmake_lib_dir_path}")
  STRING(REPLACE ":" "" _ffmpeg_david_cmake_lib_dir_path "${_ffmpeg_david_cmake_lib_dir_path}")
  STRING(FIND ${_ffmpeg_david_cmake_lib_dir_path} / _ffmpeg_first_slash_index)
  IF(_ffmpeg_first_slash_index GREATER 0)
    STRING(PREPEND _ffmpeg_david_cmake_lib_dir_path "/")
  ENDIF()
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  DEPENDS ${RV_FFMPEG_DEPENDS}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  ${RV_FFMPEG_PATCH_COMMAND_STEP}
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E env "PKG_CONFIG_PATH=$PKG_CONFIG_PATH:${_ffmpeg_david_cmake_lib_dir_path}/pkgconfig" ${_configure_command} --prefix=${_install_dir}
    ${RV_FFMPEG_COMMON_CONFIG_OPTIONS} ${RV_FFMPEG_CONFIG_OPTIONS} ${RV_FFMPEG_EXTRA_C_OPTIONS} ${RV_FFMPEG_EXTRA_LIBPATH_OPTIONS} ${RV_FFMPEG_EXTERNAL_LIBS}
  BUILD_COMMAND ${_make_command} -j${_cpu_count}
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_build_byproducts}
  USES_TERMINAL_BUILD TRUE
)

IF(RV_FFMPEG_POST_CONFIGURE_STEP)
  EXTERNALPROJECT_ADD_STEP(
    ${_target} post_configure_step
    ${RV_FFMPEG_POST_CONFIGURE_STEP}
    DEPENDEES configure
    DEPENDERS build
  )
ENDIF()

FILE(MAKE_DIRECTORY ${_include_dir})

FOREACH(
  _ffmpeg_lib
  ${_ffmpeg_libs}
)
  ADD_LIBRARY(ffmpeg::${_ffmpeg_lib} SHARED IMPORTED GLOBAL)
  ADD_DEPENDENCIES(ffmpeg::${_ffmpeg_lib} ${_target})
  SET_PROPERTY(
    TARGET ffmpeg::${_ffmpeg_lib}
    PROPERTY IMPORTED_LOCATION ${_ffmpeg_${_ffmpeg_lib}_lib}
  )
  IF(RV_TARGET_WINDOWS)
    SET_PROPERTY(
      TARGET ffmpeg::${_ffmpeg_lib}
      PROPERTY IMPORTED_IMPLIB ${_ffmpeg_${_ffmpeg_lib}_implib}
    )
  ENDIF()
  TARGET_INCLUDE_DIRECTORIES(
    ffmpeg::${_ffmpeg_lib}
    INTERFACE ${_include_dir}
  )

  LIST(APPEND RV_DEPS_LIST ffmpeg::${_ffmpeg_lib})
ENDFOREACH()

TARGET_LINK_LIBRARIES(
  ffmpeg::swresample
  INTERFACE ffmpeg::avutil
)
TARGET_LINK_LIBRARIES(
  ffmpeg::swscale
  INTERFACE ffmpeg::avutil
)
TARGET_LINK_LIBRARIES(
  ffmpeg::avcodec
  INTERFACE ffmpeg::swresample
)
TARGET_LINK_LIBRARIES(
  ffmpeg::avformat
  INTERFACE ffmpeg::avcodec
)

SET(${_target}-stage-flag
    ${RV_STAGE_LIB_DIR}/${_target}-stage-flag
)

ADD_CUSTOM_TARGET(
  clean-${_target}
  COMMENT "Cleaning '${_target}' ..."
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${_base_dir}
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${RV_DEPS_BASE_DIR}/cmake/dependencies/${_target}-prefix
)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    # Note: The FFmpeg build stores both the import lib and the dll in the install bin directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
    COMMAND cmake -E touch ${${_target}-stage-flag}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${${_target}-stage-flag}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    COMMAND cmake -E touch ${${_target}-stage-flag}
    DEPENDS ${_target}
  )
ENDIF()

ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${${_target}-stage-flag}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_FFMPEG_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
