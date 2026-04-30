#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the UTV project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
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

# Prefer ffmpeg-full from Homebrew if it exists, as it is keg-only and contains all the required non-free codecs (like ProRes).
IF(APPLE)
  IF(EXISTS "/opt/homebrew/opt/ffmpeg-full/lib/pkgconfig")
    SET(ENV{PKG_CONFIG_PATH}
        "/opt/homebrew/opt/ffmpeg-full/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
    )
  ELSEIF(EXISTS "/usr/local/opt/ffmpeg-full/lib/pkgconfig")
    SET(ENV{PKG_CONFIG_PATH}
        "/usr/local/opt/ffmpeg-full/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
    )
  ENDIF()
ENDIF()

FIND_PACKAGE(PkgConfig REQUIRED)

# Individual check for each module to get specific variables
FOREACH(
  _lib
  avcodec avformat avutil swresample swscale
)
  PKG_CHECK_MODULES(FFMPEG_lib${_lib} REQUIRED lib${_lib})

  IF(NOT TARGET ffmpeg::${_lib})
    ADD_LIBRARY(ffmpeg::${_lib} INTERFACE IMPORTED GLOBAL)

    # Try to find the actual library file to avoid "library not found" errors
    FIND_LIBRARY(
      FFMPEG_lib${_lib}_PATH
      NAMES ${_lib}
      PATHS ${FFMPEG_lib${_lib}_LIBRARY_DIRS} ${FFMPEG_lib${_lib}_LIBDIR}
    )

    IF(FFMPEG_lib${_lib}_PATH)
      SET_PROPERTY(
        TARGET ffmpeg::${_lib}
        PROPERTY INTERFACE_LINK_LIBRARIES "${FFMPEG_lib${_lib}_PATH}"
      )
    ELSE()
      # Fallback to the names if path not found
      SET_PROPERTY(
        TARGET ffmpeg::${_lib}
        PROPERTY INTERFACE_LINK_LIBRARIES "${FFMPEG_lib${_lib}_LIBRARIES}"
      )
    ENDIF()

    SET_PROPERTY(
      TARGET ffmpeg::${_lib}
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_lib${_lib}_INCLUDE_DIRS}"
    )
    SET_PROPERTY(
      TARGET ffmpeg::${_lib}
      PROPERTY INTERFACE_COMPILE_OPTIONS "${FFMPEG_lib${_lib}_CFLAGS_OTHER}"
    )

    # Promote to global if it came from PkgConfig target
    IF(TARGET PkgConfig::FFMPEG_lib${_lib})
      SET_PROPERTY(
        TARGET PkgConfig::FFMPEG_lib${_lib}
        PROPERTY IMPORTED_GLOBAL TRUE
      )
      # Prefer the PkgConfig target as it handles internal dependencies better
      SET_PROPERTY(
        TARGET ffmpeg::${_lib}
        PROPERTY INTERFACE_LINK_LIBRARIES PkgConfig::FFMPEG_lib${_lib}
      )
    ENDIF()
  ENDIF()
ENDFOREACH()

# Ensure cross-linking between components
TARGET_LINK_LIBRARIES(
  ffmpeg::avcodec
  INTERFACE ffmpeg::avutil ffmpeg::swresample
)
TARGET_LINK_LIBRARIES(
  ffmpeg::avformat
  INTERFACE ffmpeg::avcodec
)
TARGET_LINK_LIBRARIES(
  ffmpeg::swscale
  INTERFACE ffmpeg::avutil
)
TARGET_LINK_LIBRARIES(
  ffmpeg::swresample
  INTERFACE ffmpeg::avutil
)

# Add OpenSSL dependency to avutil as many FFmpeg builds require it
FIND_PACKAGE(OpenSSL QUIET)
IF(TARGET OpenSSL::Crypto)
  TARGET_LINK_LIBRARIES(
    ffmpeg::avutil
    INTERFACE OpenSSL::Crypto
  )
ENDIF()
