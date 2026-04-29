#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project.
# Copyright (C) 2026  Makai Systems. All Rights Reserved.
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

IF(RV_USE_SYSTEM_DEPS)
  FIND_PACKAGE(PkgConfig REQUIRED)
  PKG_CHECK_MODULES(FFMPEG_LIBS REQUIRED
    libavcodec
    libavformat
    libavutil
    libswresample
    libswscale
  )

  FOREACH(_lib avcodec avformat avutil swresample swscale)
    IF(NOT TARGET ffmpeg::${_lib})
      ADD_LIBRARY(ffmpeg::${_lib} INTERFACE IMPORTED GLOBAL)
      SET_PROPERTY(TARGET ffmpeg::${_lib} PROPERTY INTERFACE_LINK_LIBRARIES "${FFMPEG_lib${_lib}_LINK_LIBRARIES}")
      SET_PROPERTY(TARGET ffmpeg::${_lib} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_lib${_lib}_INCLUDE_DIRS}")
      SET_PROPERTY(TARGET ffmpeg::${_lib} PROPERTY INTERFACE_COMPILE_OPTIONS "${FFMPEG_lib${_lib}_CFLAGS_OTHER}")
      
      # Map pkg-config names to our internal target names
      IF(TARGET PkgConfig::FFMPEG_lib${_lib})
         # Use the pkg-config generated target if available for better dependency handling
         SET_PROPERTY(TARGET ffmpeg::${_lib} PROPERTY INTERFACE_LINK_LIBRARIES PkgConfig::FFMPEG_lib${_lib})
      ENDIF()
    ENDIF()
  ENDFOREACH()
  
  RETURN()
ENDIF()

SET(_target
    "RV_DEPS_FFMPEG"
)

SET(_version
    ${RV_DEPS_FFMPEG_VERSION}
)

SET(_download_hash
    ${RV_DEPS_FFMPEG_DOWNLOAD_HASH}
)

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

IF(RV_TARGET_DARWIN)
  SET(_ffmpeg_avutil_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avutil.${RV_DEPS_FFMPEG_VERSION_LIB_avutil}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swresample_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swresample.${RV_DEPS_FFMPEG_VERSION_LIB_swresample}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swscale_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swscale.${RV_DEPS_FFMPEG_VERSION_LIB_swscale}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avcodec_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec.${RV_DEPS_FFMPEG_VERSION_LIB_avcodec}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avformat_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avformat.${RV_DEPS_FFMPEG_VERSION_LIB_avformat}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_ffmpeg_avutil_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_FFMPEG_VERSION_LIB_avutil}
  )
  SET(_ffmpeg_swresample_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_FFMPEG_VERSION_LIB_swresample}
  )
  SET(_ffmpeg_swscale_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_FFMPEG_VERSION_LIB_swscale}
  )
  SET(_ffmpeg_avcodec_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_FFMPEG_VERSION_LIB_avcodec}
  )
  SET(_ffmpeg_avformat_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_FFMPEG_VERSION_LIB_avformat}
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_ffmpeg_avutil_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avutil-${RV_DEPS_FFMPEG_VERSION_LIB_avutil}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swresample_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swresample-${RV_DEPS_FFMPEG_VERSION_LIB_swresample}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_swscale_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}swscale-${RV_DEPS_FFMPEG_VERSION_LIB_swscale}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avcodec_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avcodec-${RV_DEPS_FFMPEG_VERSION_LIB_avcodec}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ffmpeg_avformat_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}avformat-${RV_DEPS_FFMPEG_VERSION_LIB_avformat}${CMAKE_SHARED_LIBRARY_SUFFIX}
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

# Disable x11 on macOS to avoid linking against Homebrew's X11 libraries, ensuring binary portability
IF(RV_TARGET_DARWIN
   OR RV_TARGET_APPLE_ARM64
)
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-xlib")
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-libxcb")
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-libxcb-shm")
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-libxcb-shape")
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-libxcb-xfixes")
  # For older FFmpeg versions, you might also need: LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-x11grab")
ENDIF()

# Change the condition to TRUE to be able to debug into FFmpeg.
IF(FALSE)
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-optimizations")
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--enable-debug=3")
  LIST(APPEND RV_FFMPEG_COMMON_CONFIG_OPTIONS "--disable-stripping")
ENDIF()

# Controls the EXTERNALPROJECT_ADD/BUILD_ALWAYS option
SET(${_force_rebuild}
    FALSE
)

IF(RV_TARGET_APPLE_ARM64)
  SET(RV_FFMPEG_USE_VIDEOTOOLBOX_DEFAULT_VALUE
      ON
  )
ELSE()
  SET(RV_FFMPEG_USE_VIDEOTOOLBOX_DEFAULT_VALUE
      OFF
  )
ENDIF()

OPTION(RV_FFMPEG_USE_VIDEOTOOLBOX "FFmpeg laveraging the VideoToolbox framework" ${RV_FFMPEG_USE_VIDEOTOOLBOX_DEFAULT_VALUE})

# Make a list of the Open RV's FFmpeg config options unless already customized. Note that a super project, a project consuming Open RV as a submodule, can
# customize the FFmpeg config options via the RV_FFMPEG_CONFIG_OPTIONS cmake property.
IF(NOT RV_FFMPEG_CONFIG_OPTIONS)
  SET(RV_FFMPEG_CONFIG_OPTIONS "")
  
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

ADD_CUSTOM_TARGET(
  clean-${_target}
  COMMENT "Cleaning '${_target}' ..."
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${_base_dir}
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${RV_DEPS_BASE_DIR}/cmake/dependencies/${_target}-prefix
)

# Note: On Windows, FFmpeg stores both import libs and DLLs in the install bin directory, so we copy _lib_dir (which is install/bin on Windows) to both stage
# dirs.
IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} EXTRA_LIB_DIRS ${RV_STAGE_BIN_DIR} USE_FLAG_FILE)
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} USE_FLAG_FILE)
ENDIF()

SET(RV_DEPS_FFMPEG_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
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

ADD_CUSTOM_TARGET(
  clean-${_target}
  COMMENT "Cleaning '${_target}' ..."
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${_base_dir}
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${RV_DEPS_BASE_DIR}/cmake/dependencies/${_target}-prefix
)

# Note: On Windows, FFmpeg stores both import libs and DLLs in the install bin directory, so we copy _lib_dir (which is install/bin on Windows) to both stage
# dirs.
IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} EXTRA_LIB_DIRS ${RV_STAGE_BIN_DIR} USE_FLAG_FILE)
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} USE_FLAG_FILE)
ENDIF()

SET(RV_DEPS_FFMPEG_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
