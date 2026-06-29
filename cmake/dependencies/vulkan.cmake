#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Vulkan = Vulkan-Headers + Vulkan-Loader (libvulkan.so), fetched from hash-pinned Khronos GitHub tarballs. RV consumes only the headers (<vulkan/vulkan.h>) and
# the loader (linked via Vulkan::Vulkan; all API calls go through vkGetInstanceProcAddr/vkGetDeviceProcAddr) -- no validation layers or shader tools. Modeled on
# glew.cmake.
#

# FORCE_LIB so _lib_dir is install/lib (we also pass -DCMAKE_INSTALL_LIBDIR=lib to the loader), giving a deterministic location across RHEL (lib64) and
# non-RHEL.
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_VULKAN" "${RV_DEPS_VULKAN_VERSION}" "" "" FORCE_LIB)

# --- Try to find an installed/system package first (only when RV_DEPS_PREFER_INSTALLED=ON). ---
# Vulkan ships no CMake config; it is located via the built-in FindVulkan module (ALLOW_MODULE) or the `vulkan` pkg-config module. Omit VERSION (FindVulkan
# version reporting is unreliable).
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  Vulkan
  ALLOW_MODULE
  PKG_CONFIG_NAME
  vulkan
  DEPS_LIST_TARGETS
  Vulkan::Vulkan
)

# --- Library naming (shared between find and build paths), à la glew's _glew_lib_name. ---
# Linux: reference the SONAME symlink (libvulkan.so.1); the loader also installs the fully versioned real file (libvulkan.so.<version>) and the libvulkan.so dev
# symlink alongside it. Windows: loader produces vulkan-1.dll (in bin/) and the vulkan-1.lib import library (in lib/); the "-1" is RV_DEPS_VULKAN_VERSION_LIB.
IF(RV_TARGET_WINDOWS)
  SET(_vulkan_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}vulkan-${RV_DEPS_VULKAN_VERSION_LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_vulkan_lib
      ${_bin_dir}/${_vulkan_lib_name}
  )
  SET(_vulkan_implib_name
      ${CMAKE_IMPORT_LIBRARY_PREFIX}vulkan-${RV_DEPS_VULKAN_VERSION_LIB}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_vulkan_implib
      ${_lib_dir}/${_vulkan_implib_name}
  )
ELSE()
  SET(_vulkan_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}vulkan${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_VULKAN_VERSION_LIB}
  )
  SET(_vulkan_lib
      ${_lib_dir}/${_vulkan_lib_name}
  )
ENDIF()

IF(${_target}_FOUND)
  # Found via RV_FIND_DEPENDENCY (RV_DEPS_PREFER_INSTALLED=ON). FindVulkan/pkg-config usually provides the Vulkan::Vulkan target; create it only if the
  # pkg-config path did not.
  IF(NOT TARGET Vulkan::Vulkan)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      Vulkan::Vulkan
      TYPE
      SHARED
      LOCATION
      ${_vulkan_lib}
      IMPLIB
      "${_vulkan_implib}"
      SONAME
      ${_vulkan_lib_name}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ENDIF()

  # Found path: use TARGET_LIBS to resolve the actual library path at build time.
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS Vulkan::Vulkan)
ELSE()
  # --- Default: fetch headers + loader from Khronos and build from source. ---
  # We always build our own hash-pinned Vulkan (like every other managed dependency) unless RV_DEPS_PREFER_INSTALLED=ON. Note that a global Vulkan::Vulkan
  # target may already exist here: INCLUDE(qt6) runs before us and Qt's find_package(Qt6 Gui) -> WrapVulkanHeaders runs find_package(Vulkan), which (with
  # CMAKE_FIND_PACKAGE_TARGETS_GLOBAL) creates a global Vulkan::Vulkan from a discoverable *system* loader. build/vulkan.cmake takes ownership of that target
  # name (repointing it at our fetched build) rather than reusing the system loader.
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/vulkan.cmake)

  IF(RV_TARGET_WINDOWS)
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_bin_dir} OUTPUTS ${RV_STAGE_BIN_DIR}/${_vulkan_lib_name})
  ELSE()
    RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_vulkan_lib_name})
  ENDIF()
ENDIF()
