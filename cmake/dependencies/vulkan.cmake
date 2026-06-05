#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Vulkan = Vulkan-Headers + Vulkan-Loader (libvulkan.so), fetched from hash-pinned Khronos
# GitHub tarballs. RV consumes only the headers (<vulkan/vulkan.h>) and the loader (linked via
# Vulkan::Vulkan; all API calls go through vkGetInstanceProcAddr/vkGetDeviceProcAddr) -- no
# validation layers or shader tools. Modeled on glew.cmake.
#
# Linux-only; this file is only INCLUDE()d when RV_ENABLE_LINUX_VULKAN_SDK=ON.

# FORCE_LIB so _lib_dir is install/lib (we also pass -DCMAKE_INSTALL_LIBDIR=lib to the loader),
# giving a deterministic location across RHEL (lib64) and non-RHEL.
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_VULKAN" "${RV_DEPS_VULKAN_VERSION}" "" "" FORCE_LIB)

# --- Try to find an installed/system package first (only when RV_DEPS_PREFER_INSTALLED=ON). ---
# Vulkan ships no CMake config; it is located via the built-in FindVulkan module (ALLOW_MODULE)
# or the `vulkan` pkg-config module. Omit VERSION (FindVulkan version reporting is unreliable).
RV_FIND_DEPENDENCY(
  TARGET ${_target}
  PACKAGE Vulkan
  ALLOW_MODULE
  PKG_CONFIG_NAME vulkan
  DEPS_LIST_TARGETS Vulkan::Vulkan
)

# --- Library naming (shared between find and build paths), à la glew's _glew_lib_name. ---
# We reference the SONAME symlink (libvulkan.so.1); the loader also installs the fully
# versioned real file (libvulkan.so.<version>) and the libvulkan.so dev symlink alongside it.
SET(_vulkan_lib_name
    ${CMAKE_SHARED_LIBRARY_PREFIX}vulkan${CMAKE_SHARED_LIBRARY_SUFFIX}.${RV_DEPS_VULKAN_VERSION_LIB}
)
SET(_vulkan_lib
    ${_lib_dir}/${_vulkan_lib_name}
)

IF(${_target}_FOUND)
  # Found via RV_FIND_DEPENDENCY (RV_DEPS_PREFER_INSTALLED=ON). FindVulkan/pkg-config usually
  # provides the Vulkan::Vulkan target; create it only if the pkg-config path did not.
  IF(NOT TARGET Vulkan::Vulkan)
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

  # Found path: use TARGET_LIBS to resolve the actual library path at build time.
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS Vulkan::Vulkan)
ELSEIF(TARGET Vulkan::Vulkan)
  # Another consumer (e.g. Qt's find_package(Vulkan)) already created a global Vulkan::Vulkan
  # from a discoverable system Vulkan. Reuse it rather than fetching/redefining (which would
  # collide). A system Vulkan being discoverable implies it is present at runtime too.
  MESSAGE(STATUS "Vulkan::Vulkan already defined by another package; reusing it (not fetching).")
ELSE()
  # --- Nothing available: fetch headers + loader from Khronos and build from source. ---
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/vulkan.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_vulkan_lib_name})
ENDIF()
