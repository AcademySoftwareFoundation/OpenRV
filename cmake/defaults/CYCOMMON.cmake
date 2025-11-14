# Common build dependencies for all CY20XX platforms
# aja
# https://github.com/aja-video/libajantv2
SET(RV_DEPS_AJA_VERSION "17.1.0")
SET(RV_DEPS_AJA_DOWNLOAD_HASH "b9d189f77e18dbdff7c39a339b1a5dd4")

# atomic_ops
# https://github.com/ivmai/libatomic_ops
SET(RV_DEPS_ATOMIC_OPS_VERSION "7.7.0")
SET(RV_DEPS_ATOMIC_OPS_DOWNLOAD_HASH "cc7fad1e71b3064abe1ea821ae9a9a6e")

# dav1d
# https://github.com/videolan/dav1d
SET(RV_DEPS_DAV1D_VERSION "1.4.3")
SET(RV_DEPS_DAV1D_DOWNLOAD_HASH "2c62106fda87a69122dc8709243a34e8")

# doctest
# https://github.com/doctest/doctest
SET(RV_DEPS_DOCTEST_VERSION "2.4.9")
SET(RV_DEPS_DOCTEST_DOWNLOAD_HASH "a7948b5ec1f69de6f84c7d7487aaf79b")   

# expat
# https://github.com/libexpat/libexpat
SET(RV_DEPS_EXPAT_VERSION "2.6.3")
SET(RV_DEPS_EXPAT_DOWNLOAD_HASH "985086e206a01e652ca460eb069e4780")

# ffmpeg
# https://github.com/FFmpeg/FFmpeg
# TODO: cmake/dependencies/ffmpeg.cmake defines library names and 
# will need to updated for new versions and/or
# the logic moved here
IF(RV_FFMPEG_8)
    SET(RV_DEPS_FFMPEG_VERSION "n8.0")
    SET(RV_DEPS_FFMPEG_DOWNLOAD_HASH "fcf93d5855f654b82d4aa8aae62d64d3")
ELSEIF(RV_FFMPEG_8)
    SET(RV_DEPS_FFMPEG_VERSION "n7.1")
    SET(RV_DEPS_FFMPEG_DOWNLOAD_HASH "a7a85ec05c9bc3aeefee12743899d8ab")
ELSEIF(RV_FFMPEG_8)
    SET(RV_DEPS_FFMPEG_VERSION "n6.1.2")
    SET(RV_DEPS_FFMPEG_DOWNLOAD_HASH "953b858e5be3ab66232bdbb90e42f50d")
ELSE()
  # This will happene if a valid RV_FFMPEG is not defined
    MESSAGE(FATAL_ERROR "The requested version of FFmpeg is not supported.")
ENDIF()

# gc
# https://github.com/ivmai/bdwgc
SET(RV_DEPS_GC_VERSION "8.2.2")
SET(RV_DEPS_GC_DOWNLOAD_HASH "2ca38d05e1026b3426cf6c24ca3a7787")

# oiio
# https://github.com/AcademySoftwareFoundation/OpenImageIO
SET(RV_DEPS_OIIO_VERSION "2.5.19.1")
SET(RV_DEPS_OIIO_DOWNLOAD_HASH "5af6de5a73c6d234eed8e2874a5aed62")
# otio
# https://github.com/AcademySoftwareFoundation/OpenTimelineIO
SET(RV_DEPS_OTIO_VERSION "0.18.1")
