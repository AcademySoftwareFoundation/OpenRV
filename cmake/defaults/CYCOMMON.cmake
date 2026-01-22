# Common build dependencies for all CY20XX platforms aja https://github.com/aja-video/libajantv2
SET(RV_DEPS_AJA_VERSION
    "17.6.0.hotfix1"
)
SET(RV_DEPS_AJA_DOWNLOAD_HASH
    "dba447ddd1b0ee84cee8441c0adba06a"
)

# atomic_ops https://github.com/bdwgc/libatomic_ops
SET(RV_DEPS_ATOMIC_OPS_VERSION
    "7.10.0"
)
SET(RV_DEPS_ATOMIC_OPS_DOWNLOAD_HASH
    "35e417e4e49cd97976ef14c50e06db9b"
)

# dav1d https://github.com/videolan/dav1d
SET(RV_DEPS_DAV1D_VERSION
    "1.5.3"
)
SET(RV_DEPS_DAV1D_DOWNLOAD_HASH
    "6a195752588586acf13349a1cceedab8"
)

# doctest https://github.com/doctest/doctest
SET(RV_DEPS_DOCTEST_VERSION
    "2.4.12"
)
SET(RV_DEPS_DOCTEST_DOWNLOAD_HASH
    "92bcfd6352ebf6c741f9ffaa3cad8808"
)

# expat https://github.com/libexpat/libexpat
SET(RV_DEPS_EXPAT_VERSION
    "2.7.3"
)
SET(RV_DEPS_EXPAT_DOWNLOAD_HASH
    "e87e396c0062f9bb0f1b57c85f11dd0c"
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
    "8.2.10"
)
SET(RV_DEPS_GC_DOWNLOAD_HASH
    "d394a9dd165e742283fb82b20d1b688c"
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
    "2.5.4"
)
SET(RV_DEPS_OPENJPEG_DOWNLOAD_HASH
    "6160de075bb5191e482bc0f024b375e4"
)

# openjph https://github.com/aous72/OpenJPH
SET(RV_DEPS_OPENJPH_VERSION
    "0.26.0"
)
SET(RV_DEPS_OPENJPH_DOWNLOAD_HASH
    "469e12ba5e953ce7002d02f9486c8721"
)

# otio https://github.com/AcademySoftwareFoundation/OpenTimelineIO
SET(RV_DEPS_OTIO_VERSION
    "0.18.1"
)

# pcre2 https://github.com/PCRE2Project/pcre2
SET(RV_DEPS_PCRE2_VERSION
    "10.47"
)
SET(RV_DEPS_PCRE2_DOWNLOAD_HASH
    "2261078304c4e6904d1c94c9ffd04fd8"
)

# png https://github.com/glennrp/libpng
SET(RV_DEPS_PNG_VERSION
    "1.6.54"
)
SET(RV_DEPS_PNG_DOWNLOAD_HASH
    "fbad637cfd2eeef6b35e5ec3af97621c"
)

# raw https://github.com/LibRaw/LibRaw Please check the libraw_version.h file for your version number to get the LIBRAW_SHLIB_CURRENT value
# https://github.com/LibRaw/LibRaw/blob/0.21-stable/libraw/libraw_version.h
SET(RV_DEPS_RAW_VERSION
    "0.22.0"
)
SET(RV_DEPS_RAW_DOWNLOAD_HASH
    "1d2e307a1e6d7a34268fc421b17675fe"
)
SET(RV_DEPS_RAW_VERSION_LIB
    "24"
)

# spdlog https://github.com/gabime/spdlog
SET(RV_DEPS_SPDLOG_VERSION
    "1.17.0"
)
SET(RV_DEPS_SPDLOG_DOWNLOAD_HASH
    "f0d8dd02539fe609bdfd42c0549fe28d"
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
    "1.6.0"
)
SET(RV_DEPS_WEBP_DOWNLOAD_HASH
    "d498caf9323a24ce3ed40b84c22a32cd"
)

# zlib https://github.com/madler/zlib
SET(RV_DEPS_ZLIB_VERSION
    "1.3.1"
)
SET(RV_DEPS_ZLIB_DOWNLOAD_HASH
    "127b8a71a3fb8bebe89df1080f15fdf6"
)
