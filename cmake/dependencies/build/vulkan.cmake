#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build Vulkan-Headers (header-only install) + Vulkan-Loader (libvulkan.so) from hash-pinned Khronos GitHub tarballs. Both install into the shared
# ${_install_dir}. Included by cmake/dependencies/vulkan.cmake when no system Vulkan package is found.
#
# Expects these variables from the caller (set by RV_CREATE_STANDARD_DEPS_VARIABLES): _target, _version, _install_dir, _include_dir, _lib_dir. And these
# dep-specific variables: _vulkan_lib, _vulkan_lib_name.
#
# The loader ships pre-generated sources and gates its Python codegen behind LOADER_CODEGEN (default OFF), so no Python is required at build time. It does
# require the Vulkan-Headers CMake package (find_package(VulkanHeaders CONFIG)), provided via CMAKE_PREFIX_PATH below.

SET(_headers_url
    "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/vulkan-sdk-${_version}.tar.gz"
)
SET(_loader_url
    "https://github.com/KhronosGroup/Vulkan-Loader/archive/refs/tags/vulkan-sdk-${_version}.tar.gz"
)

# --- Vulkan-Headers: installs headers + the VulkanHeaders CMake config + the registry. ---
EXTERNALPROJECT_ADD(
  ${_target}_headers
  URL ${_headers_url}
  URL_MD5 ${RV_DEPS_VULKAN_HEADERS_DOWNLOAD_HASH}
  DOWNLOAD_NAME vulkan-headers-${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/headers-src
  BINARY_DIR ${RV_DEPS_BASE_DIR}/${_target}/headers-build
  INSTALL_DIR ${_install_dir}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${_install_dir} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DVULKAN_HEADERS_ENABLE_MODULE=OFF -DVULKAN_HEADERS_ENABLE_TESTS=OFF
  BUILD_ALWAYS FALSE
  USES_TERMINAL_BUILD TRUE
)

# --- Vulkan-Loader: builds libvulkan.so against the installed headers. ---
# WSI: enable Xlib/XCB (RV is an X11 client); leave Wayland off to avoid a wayland-client build dependency.
EXTERNALPROJECT_ADD(
  ${_target}
  DEPENDS ${_target}_headers
  URL ${_loader_url}
  URL_MD5 ${RV_DEPS_VULKAN_LOADER_DOWNLOAD_HASH}
  DOWNLOAD_NAME vulkan-loader-${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/loader-src
  BINARY_DIR ${RV_DEPS_BASE_DIR}/${_target}/loader-build
  INSTALL_DIR ${_install_dir}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${_install_dir}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_LIBDIR=lib
             -DCMAKE_PREFIX_PATH=${_install_dir}
             -DVULKAN_HEADERS_INSTALL_DIR=${_install_dir}
             -DBUILD_TESTS=OFF
             -DBUILD_WSI_XLIB_SUPPORT=ON
             -DBUILD_WSI_XCB_SUPPORT=ON
             -DBUILD_WSI_WAYLAND_SUPPORT=OFF
             -DBUILD_WSI_XLIB_XRANDR_SUPPORT=OFF
  BUILD_BYPRODUCTS ${_vulkan_lib}
  BUILD_ALWAYS FALSE
  USES_TERMINAL_BUILD TRUE
)

IF(TARGET Vulkan::Vulkan)
  # Qt's find_package(Qt6 Gui) -> WrapVulkanHeaders ran find_package(Vulkan) before us and created a global Vulkan::Vulkan pointing at the system loader. We
  # can't redefine it (ADD_LIBRARY would error), so repoint it at our fetched build instead. This is invisible to Qt: it consumes only the Vulkan headers (via
  # WrapVulkanHeaders) and dlopen()s the loader at runtime -- it never links Vulkan::Vulkan's IMPORTED_LOCATION.
  MESSAGE(STATUS "Repointing existing Vulkan::Vulkan target at managed Vulkan ${_version}")
  ADD_DEPENDENCIES(Vulkan::Vulkan ${_target})
  FILE(MAKE_DIRECTORY ${_include_dir})
  SET_TARGET_PROPERTIES(
    Vulkan::Vulkan
    PROPERTIES IMPORTED_LOCATION ${_vulkan_lib}
               IMPORTED_SONAME ${_vulkan_lib_name}
               INTERFACE_INCLUDE_DIRECTORIES ${_include_dir}
  )
  SET(RV_DEPS_LIST
      ${RV_DEPS_LIST} Vulkan::Vulkan
  )
ELSE()
  RV_ADD_IMPORTED_LIBRARY(
    NAME
    Vulkan::Vulkan
    TYPE
    SHARED
    LOCATION
    ${_vulkan_lib}
    SONAME
    ${_vulkan_lib_name}
    INCLUDE_DIRS
    ${_include_dir}
    DEPENDS
    ${_target}
    ADD_TO_DEPS_LIST
  )
ENDIF()
