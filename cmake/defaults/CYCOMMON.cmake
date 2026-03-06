# Common build dependencies for all CY20XX platforms aja https://github.com/aja-video/libajantv2
SET(RV_DEPS_AJA_VERSION
    "17.1.0"
)
SET(RV_DEPS_AJA_DOWNLOAD_HASH
    "b9d189f77e18dbdff7c39a339b1a5dd4"
)

# atomic_ops https://github.com/ivmai/libatomic_ops
SET(RV_DEPS_ATOMIC_OPS_VERSION
    "7.7.0"
)
SET(RV_DEPS_ATOMIC_OPS_DOWNLOAD_HASH
    "cc7fad1e71b3064abe1ea821ae9a9a6e"
)

# dav1d https://github.com/videolan/dav1d
SET(RV_DEPS_DAV1D_VERSION
    "1.4.3"
)
SET(RV_DEPS_DAV1D_DOWNLOAD_HASH
    "2c62106fda87a69122dc8709243a34e8"
)

# doctest https://github.com/doctest/doctest
SET(RV_DEPS_DOCTEST_VERSION
    "2.4.9"
)
SET(RV_DEPS_DOCTEST_DOWNLOAD_HASH
    "a7948b5ec1f69de6f84c7d7487aaf79b"
)

# expat https://github.com/libexpat/libexpat
SET(RV_DEPS_EXPAT_VERSION
    "2.6.3"
)
SET(RV_DEPS_EXPAT_DOWNLOAD_HASH
    "985086e206a01e652ca460eb069e4780"
)

# ffmpeg https://github.com/FFmpeg/FFmpeg
IF(RV_FFMPEG STREQUAL 8)
  SET(RV_FFMPEG_8
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_FFMPEG_8)
  SET(RV_DEPS_FFMPEG_VERSION
      "n8.0"
  )
  SET(RV_DEPS_FFMPEG_DOWNLOAD_HASH
      "fcf93d5855f654b82d4aa8aae62d64d3"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avutil
      "60"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_swresample
      "6"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_swscale
      "9"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avcodec
      "62"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avformat
      "62"
  )
ELSEIF(RV_FFMPEG STREQUAL 7)
  SET(RV_FFMPEG_7
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_FFMPEG_7)
  SET(RV_DEPS_FFMPEG_VERSION
      "n7.1"
  )
  SET(RV_DEPS_FFMPEG_DOWNLOAD_HASH
      "a7a85ec05c9bc3aeefee12743899d8ab"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avutil
      "59"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_swresample
      "5"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_swscale
      "8"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avcodec
      "61"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avformat
      "61"
  )
ELSEIF(RV_FFMPEG STREQUAL 6)
  SET(RV_FFMPEG_6
      ON
  )
  ADD_COMPILE_DEFINITIONS(RV_FFMPEG_6)
  SET(RV_DEPS_FFMPEG_VERSION
      "n6.1.2"
  )
  SET(RV_DEPS_FFMPEG_DOWNLOAD_HASH
      "953b858e5be3ab66232bdbb90e42f50d"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avutil
      "58"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_swresample
      "4"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_swscale
      "7"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avcodec
      "60"
  )
  SET(RV_DEPS_FFMPEG_VERSION_LIB_avformat
      "60"
  )
ELSE()
  # This will happene if a valid RV_FFMPEG is not defined
  MESSAGE(FATAL_ERROR "FFMPEG_8, RV_FFMPEG_7, or RV_FFMPEG_6 must be defined to select the FFMPEG version to use")
ENDIF()

# gc https://github.com/ivmai/bdwgc
SET(RV_DEPS_GC_VERSION
    "8.2.2"
)
SET(RV_DEPS_GC_DOWNLOAD_HASH
    "2ca38d05e1026b3426cf6c24ca3a7787"
)

# glew https://github.com/nigels-com/glew
SET(RV_DEPS_GLEW_VERSION
    "e1a80a9f12d7def202d394f46e44cfced1104bfb"
)
SET(RV_DEPS_GLEW_DOWNLOAD_HASH
    "9bfc689dabeb4e305ce80b5b6f28bcf9"
)
SET(RV_DEPS_GLEW_VERSION_LIB
    "2.2.0"
)

# imgui https://github.com/pthom/imgui
#
# Note this also depends on the following repositories: https://github.com/pthom/implot.git https://github.com/dpaulat/imgui-backend-qt.git
# https://github.com/pthom/imgui-node-editor.git

SET(RV_DEPS_IMGUI_VERSION
    "bundle_20250323"
)
SET(RV_DEPS_IMGUI_DOWNLOAD_HASH
    "1ea3f48e9c6ae8230dac6e8a54f6e74b"
)
SET(RV_DEPS_IMPLOT_TAG
    "61af48ee1369083a3da391a849867af6d1b811a6"
)
SET(RV_DEPS_IMGUI_BACKEND_QT_TAG
    "023345ca8abf731fc50568c0197ceebe76bb4324"
)
SET(RV_DEPS_IMGUI_NODE_EDITOR_TAG
    "dae8edccf15d19e995599ecd505e7fa1d3264a4c"
)

# jpegturbo https://github.com/libjpeg-turbo/libjpeg-turbo TODO: Needs additional work in cmake/dependencies/jpegturbo.cmake to fully integrate new versioning
# scheme with libs
SET(RV_DEPS_JPEGTURBO_VERSION
    "2.1.4"
)
SET(RV_DEPS_JPEGTURBO_DOWNLOAD_HASH
    "357dc26a802c34387512a42697846d16"
)
SET(RV_DEPS_JPEGTURBO_VERSION_LIB
    "62.3.0"
)

# oiio https://github.com/AcademySoftwareFoundation/OpenImageIO
SET(RV_DEPS_OIIO_VERSION
    "2.5.19.1"
)
SET(RV_DEPS_OIIO_DOWNLOAD_HASH
    "5af6de5a73c6d234eed8e2874a5aed62"
)

# openjpeg https://github.com/uclouvain/openjpeg
SET(RV_DEPS_OPENJPEG_VERSION
    "2.5.0"
)
SET(RV_DEPS_OPENJPEG_DOWNLOAD_HASH
    "5cbb822a1203dd75b85639da4f4ecaab"
)

# openjph https://github.com/aous72/OpenJPH
SET(RV_DEPS_OPENJPH_VERSION
    "0.21.3"
)
SET(RV_DEPS_OPENJPH_DOWNLOAD_HASH
    "d0a3fb5f643a8948d5874624ff94a229"
)

# otio https://github.com/AcademySoftwareFoundation/OpenTimelineIO
SET(RV_DEPS_OTIO_VERSION
    "0.18.1"
)

# pcre2 https://github.com/PCRE2Project/pcre2
SET(RV_DEPS_PCRE2_VERSION
    "10.43"
)
SET(RV_DEPS_PCRE2_DOWNLOAD_HASH
    "e4c3f2a24eb5c15bec8360e50b3f0137"
)

# png https://github.com/glennrp/libpng
SET(RV_DEPS_PNG_VERSION
    "1.6.55"
)
SET(RV_DEPS_PNG_DOWNLOAD_HASH
    "933118ac208387d0727e3d5e36558022"
)

# raw https://github.com/LibRaw/LibRaw Please check the libraw_version.h file for your version number to get the LIBRAW_SHLIB_CURRENT value
# https://github.com/LibRaw/LibRaw/blob/0.21-stable/libraw/libraw_version.h
SET(RV_DEPS_RAW_VERSION
    "0.21.1"
)
SET(RV_DEPS_RAW_DOWNLOAD_HASH
    "3ad334296a7a2c8ee841f353cc1b450b"
)
SET(RV_DEPS_RAW_VERSION_LIB
    "23"
)

# spdlog https://github.com/gabime/spdlog
SET(RV_DEPS_SPDLOG_VERSION
    "1.11.0"
)
SET(RV_DEPS_SPDLOG_DOWNLOAD_HASH
    "cd620e0f103737a122a3b6539bd0a57a"
)

# tiff https://gitlab.com/libtiff/libtiff
SET(RV_DEPS_TIFF_VERSION
    "4.6.0"
)
SET(RV_DEPS_TIFF_DOWNLOAD_HASH
    "118a2e5fc9ed71653195b332b9715890"
)
SET(RV_DEPS_TIFF_VERSION_LIB
    "6.0.2"
)

# webp https://github.com/webmproject/libwebp
SET(RV_DEPS_WEBP_VERSION
    "1.2.1"
)
SET(RV_DEPS_WEBP_DOWNLOAD_HASH
    "ef5ac6de4b800afaebeb10df9ef189b2"
)

# zlib https://github.com/madler/zlib
SET(RV_DEPS_ZLIB_VERSION
    "1.3.1"
)
SET(RV_DEPS_ZLIB_DOWNLOAD_HASH
    "127b8a71a3fb8bebe89df1080f15fdf6"
)
