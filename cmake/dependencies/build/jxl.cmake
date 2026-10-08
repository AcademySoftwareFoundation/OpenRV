#
# SPDX-License-Identifier: Apache-2.0
#

# Build only the shared libraries; skip tools, tests, examples, docs, and optional codecs so nothing outside libjxl's own sources is required.
LIST(APPEND _configure_options "-DBUILD_SHARED_LIBS=ON")
LIST(APPEND _configure_options "-DBUILD_TESTING=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_TOOLS=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_EXAMPLES=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_BENCHMARK=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_DOXYGEN=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_MANPAGES=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_JNI=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_PLUGINS=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_DEVTOOLS=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_SJPEG=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_OPENEXR=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_TCMALLOC=OFF")
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_FUZZERS=OFF")

# Use libjxl's own pinned third_party submodules rather than system copies: These are compiled into the libjxl libraries, so no separate RV_DEPS builds for
# highway/brotli/skcms are needed.
LIST(APPEND _configure_options "-DJPEGXL_ENABLE_SKCMS=ON")
LIST(APPEND _configure_options "-DJPEGXL_FORCE_SYSTEM_HWY=OFF")
LIST(APPEND _configure_options "-DJPEGXL_FORCE_SYSTEM_BROTLI=OFF")
LIST(APPEND _configure_options "-DJPEGXL_FORCE_SYSTEM_LCMS2=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  GIT_REPOSITORY "https://github.com/libjxl/libjxl.git"
  # RV_DEPS_JXL_VERSION tracks the ABI/soname series (e.g. 0.12); the exact release tag (e.g. v0.12.0) is kept separately since libjxl only tags full x.y.z
  # releases.
  GIT_TAG "${RV_DEPS_JXL_GIT_TAG}"
  GIT_SHALLOW ON
  GIT_PROGRESS ON
  # Fetch only the submodules required to build the libraries.
  GIT_SUBMODULES "third_party/highway;third_party/brotli;third_party/skcms" GIT_SUBMODULES_RECURSE ON
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)
