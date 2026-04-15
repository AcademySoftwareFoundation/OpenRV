#
# SPDX-License-Identifier: Apache-2.0
#

# Do not build the executables.
LIST(APPEND _configure_options "-DBUILD_CODEC=OFF")
# OpenJPH >= 0.23 sets CMAKE_DEBUG_POSTFIX=_d, producing libopenjph_d. Override to keep consistent naming across build types (matches what
# RV_MAKE_STANDARD_LIB_NAME expects).
LIST(APPEND _configure_options "-DCMAKE_DEBUG_POSTFIX=")

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
  # DEPENDS ZLIB::ZLIB TIFF::TIFF PNG::PNG
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)
