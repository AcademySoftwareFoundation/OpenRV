#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# ---------------------- FFmpeg Build Customization ----------------------------
# Note: The FFmpeg build can be customized by super projects via the following cmake global properties (typically by appending values to them):
# cmake-format: off
# RV_FFMPEG_DEPENDS - Additional FFmpeg's dependencies can be appended to this property 
# RV_FFMPEG_PATCH_COMMAND_STEP - Commands to be executed as part of FFmpeg's patch step 
# RV_FFMPEG_POST_CONFIGURE_STEP - Commands to be executed after FFMpeg's configure step 
# RV_FFMPEG_CONFIG_OPTIONS - Custom FFmpeg configure options to enable/disable decoders and encoders 
# RV_FFMPEG_EXTRA_C_OPTIONS - Extra cflags options 
# RV_FFMPEG_EXTRA_LIBPATH_OPTIONS - Extra libpath options
# cmake-format: on
# ------------------------------------------------------------------------------

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_FFMPEG"
)

IF(RV_FFMPEG_7)
  SET(_version
      "n7.1"
  )

  SET(_download_hash
      "a7a85ec05c9bc3aeefee12743899d8ab"
  )
ELSEIF(RV_FFMPEG_6)
  SET(_version
      "n6.1.2"
  )

  SET(_download_hash
      "953b858e5be3ab66232bdbb90e42f50d"
  )
ELSE()
  # This shouldn't happen, but it could if there are changes the acceptable versions of FFmpeg in ffmpeg.cmake and this section is not updated accordingly.
  MESSAGE(FATAL_ERROR "The requested version of FFmpeg is not supported.")
ENDIF()

SET(_download_url
    "https://github.com/FFmpeg/FFmpeg/archive/refs/tags/${_version}.zip"
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

IF(RV_FFMPEG_7)
  IF(RV_TARGET_DARWIN)
    SET(_ffmpeg_avutil_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avutil.59${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swresample_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swresample.5${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swscale_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swscale.8${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avcodec_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec.61${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avformat_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avformat.61${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ELSEIF(RV_TARGET_LINUX)
    SET(_ffmpeg_avutil_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}.59
    )
    SET(_ffmpeg_swresample_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}.5
    )
    SET(_ffmpeg_swscale_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX}.8
    )
    SET(_ffmpeg_avcodec_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}.61
    )
    SET(_ffmpeg_avformat_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}.61
    )
  ELSEIF(RV_TARGET_WINDOWS)
    SET(_ffmpeg_avutil_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avutil-59${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swresample_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swresample-5${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swscale_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swscale-8${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avcodec_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec-61${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avformat_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avformat-61${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ENDIF()
ELSEIF(RV_FFMPEG_6)
  IF(RV_TARGET_DARWIN)
    SET(_ffmpeg_avutil_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avutil.58${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swresample_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swresample.4${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swscale_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swscale.7${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avcodec_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec.60${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avformat_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avformat.60${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ELSEIF(RV_TARGET_LINUX)
    SET(_ffmpeg_avutil_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}.58
    )
    SET(_ffmpeg_swresample_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}.4
    )
    SET(_ffmpeg_swscale_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX}.7
    )
    SET(_ffmpeg_avcodec_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}.60
    )
    SET(_ffmpeg_avformat_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}.60
    )
  ELSEIF(RV_TARGET_WINDOWS)
    SET(_ffmpeg_avutil_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avutil-58${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swresample_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swresample-4${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_swscale_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}swscale-7${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avcodec_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec-60${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ffmpeg_avformat_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}avformat-60${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ENDIF()
ELSE()
  # This shouldn't happen. But it could if there are changes the acceptable versions of FFmpeg in ffmpeg.cmake and this section is not updated accordingly.
  MESSAGE(FATAL_ERROR "The requested version of FFmpeg is not supported.")
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

# Fetch customizable FFmpeg build properties
GET_PROPERTY(
  RV_FFMPEG_DEPENDS GLOBAL
  PROPERTY "RV_FFMPEG_DEPENDS"
)
GET_PROPERTY(
  RV_FFMPEG_PATCH_COMMAND_STEP GLOBAL
  PROPERTY "RV_FFMPEG_PATCH_COMMAND_STEP"
)
GET_PROPERTY(
  RV_FFMPEG_POST_CONFIGURE_STEP GLOBAL
  PROPERTY "RV_FFMPEG_POST_CONFIGURE_STEP"
)
GET_PROPERTY(
  RV_FFMPEG_CONFIG_OPTIONS GLOBAL
  PROPERTY "RV_FFMPEG_CONFIG_OPTIONS"
)
GET_PROPERTY(
  RV_FFMPEG_EXTRA_C_OPTIONS GLOBAL
  PROPERTY "RV_FFMPEG_EXTRA_C_OPTIONS"
)
GET_PROPERTY(
  RV_FFMPEG_EXTRA_LIBPATH_OPTIONS GLOBAL
  PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS"
)
GET_PROPERTY(
  RV_FFMPEG_EXTERNAL_LIBS GLOBAL
  PROPERTY "RV_FFMPEG_EXTERNAL_LIBS"
)

# Make a list of common FFmpeg config options
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--enable-shared")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-static")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-iconv")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-outdevs")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-programs")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-large-tests")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-vaapi")
LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-doc")
IF(RV_TARGET_WINDOWS)
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--toolchain=msvc")
ENDIF()

# Controls the EXTERNALPROJECT_ADD/BUILD_ALWAYS option
SET(${_force_rebuild}
    FALSE
)

# Make a list of the Open RV's FFmpeg config options unless already customized. Note that a super project, a project consuming Open RV as a submodule, can
# customize the FFmpeg config options via the RV_FFMPEG_CONFIG_OPTIONS cmake property.
IF(NOT RV_FFMPEG_CONFIG_OPTIONS)
  SET(NON_FREE_DECODERS_TO_DISABLE
      "aac"
      "aac_at"
      "aac_fixed"
      "aac_latm"
      "bink"
      "binkaudio_dct"
      "binkaudio_rdft"
      "dnxhd"
      "dvvideo"
      "prores"
      "qtrle"
      "vp9"
      "vp9_cuvid"
      "vp9_mediacodec"
      "vp9_qsv"
      "vp9_rkmpp"
      "vp9_v4l2m2m"
  )

  FOREACH(
    NON_FREE_DECODER_TO_DISABLE
    ${NON_FREE_DECODERS_TO_DISABLE}
  )
    IF(NOT NON_FREE_DECODER_TO_DISABLE IN_LIST RV_FFMPEG_NON_FREE_DECODERS_TO_ENABLE)
      LIST(APPEND _disabled_decoders "--disable-decoder=${NON_FREE_DECODER_TO_DISABLE}")
    ELSE()
      MESSAGE(STATUS "FFmpeg decoder ${NON_FREE_DECODER_TO_DISABLE} enabled")
    ENDIF()
  ENDFOREACH()

  SET(NON_FREE_ENCODERS_TO_DISABLE
      "aac"
      "aac_mf"
      "dnxhd"
      "dvvideo"
      "prores"
      "qtrle"
      "vp9_qsv"
      "vp9_vaapi"
  )
  FOREACH(
    NON_FREE_ENCODER_TO_DISABLE
    ${NON_FREE_ENCODERS_TO_DISABLE}
  )
    IF(NOT NON_FREE_ENCODER_TO_DISABLE IN_LIST RV_FFMPEG_NON_FREE_ENCODERS_TO_ENABLE)
      LIST(APPEND _disabled_encoders "--disable-encoder=${NON_FREE_ENCODER_TO_DISABLE}")
    ELSE()
      MESSAGE(STATUS "FFmpeg encoder ${NON_FREE_ENCODER_TO_DISABLE} enabled")
    ENDIF()
  ENDFOREACH()

  LIST(APPEND _disabled_parsers "--disable-parser=vp9")

  LIST(APPEND _disabled_filters "--disable-filter=geq")

  LIST(APPEND _disabled_protocols "--disable-protocol=ffrtmpcrypt")
  LIST(APPEND _disabled_protocols "--disable-protocol=rtmpe")
  LIST(APPEND _disabled_protocols "--disable-protocol=rtmpte")

  SET(RV_FFMPEG_CONFIG_OPTIONS
      ${_disabled_decoders} ${_disabled_encoders} ${_disabled_filters} ${_disabled_parsers} ${_disabled_protocols}
  )

  IF(NOT RV_FFMPEG_CONFIG_OPTIONS STREQUAL RV_FFMPEG_CONFIG_OPTIONS_CACHE)
    SET(${_force_rebuild}
        TRUE
    )
    SET(RV_FFMPEG_CONFIG_OPTIONS_CACHE
        ${RV_FFMPEG_CONFIG_OPTIONS}
        CACHE STRING "FFmpeg config options" FORCE
    )
  ENDIF()
ENDIF()

LIST(REMOVE_DUPLICATES RV_FFMPEG_DEPENDS)
LIST(REMOVE_DUPLICATES RV_FFMPEG_CONFIG_OPTIONS)
LIST(REMOVE_DUPLICATES RV_FFMPEG_EXTRA_C_OPTIONS)
LIST(REMOVE_DUPLICATES RV_FFMPEG_EXTRA_LIBPATH_OPTIONS)
LIST(REMOVE_DUPLICATES RV_FFMPEG_EXTERNAL_LIBS)

SET(_ffmpeg_preprocess_pkg_config_path
    $ENV{PKG_CONFIG_PATH}
)
LIST(APPEND _ffmpeg_preprocess_pkg_config_path "${RV_DEPS_DAVID_LIB_DIR}/pkgconfig")
IF(RV_TARGET_WINDOWS)
  FOREACH(
    _ffmpeg_pkg_config_path_element IN
    LISTS _ffmpeg_preprocess_pkg_config_path
  )
    # Changing path start from "c:/..." to "/c/..." and replacing all backslashes with slashes since PkgConfig wants a linux path
    STRING(REPLACE "\\" "/" _ffmpeg_pkg_config_path_element "${_ffmpeg_pkg_config_path_element}")
    STRING(REPLACE ":" "" _ffmpeg_pkg_config_path_element "${_ffmpeg_pkg_config_path_element}")
    STRING(FIND ${_ffmpeg_pkg_config_path_element} / _ffmpeg_first_slash_index)
    IF(_ffmpeg_first_slash_index GREATER 0)
      STRING(PREPEND _ffmpeg_pkg_config_path_element "/")
    ENDIF()
    LIST(APPEND _ffmpeg_pkg_config_path ${_ffmpeg_pkg_config_path_element})
  ENDFOREACH()
ELSE()
  SET(_ffmpeg_pkg_config_path
      ${_ffmpeg_preprocess_pkg_config_path}
  )
ENDIF()
LIST(JOIN _ffmpeg_pkg_config_path ":" _ffmpeg_pkg_config_path)

SEPARATE_ARGUMENTS(RV_FFMPEG_PATCH_COMMAND_STEP)

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
  PATCH_COMMAND ${RV_FFMPEG_PATCH_COMMAND_STEP}
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E env "PKG_CONFIG_PATH=${_ffmpeg_pkg_config_path}" ${_configure_command} --prefix=${_install_dir} ${RV_FFMPEG_COMMON_CONFIG_OPTIONS}
    ${RV_FFMPEG_CONFIG_OPTIONS} ${RV_FFMPEG_EXTRA_C_OPTIONS} ${RV_FFMPEG_EXTRA_LIBPATH_OPTIONS} ${RV_FFMPEG_EXTERNAL_LIBS}
  BUILD_COMMAND ${_make_command} -j${_cpu_count}
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS ${_force_rebuild}
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
  ffmpeg::avutil
  INTERFACE OpenSSL::Crypto
)

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
    BYPRODUCTS ${${_target}-stage-flag}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    COMMAND cmake -E touch ${${_target}-stage-flag}
    BYPRODUCTS ${${_target}-stage-flag}
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
