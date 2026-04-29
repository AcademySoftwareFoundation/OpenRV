#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

IF(RV_USE_SYSTEM_DEPS)
  FIND_PACKAGE(imgui QUIET)
  IF(imgui_FOUND)
    MESSAGE(STATUS "Using system imgui")
    
    IF(DEFINED imgui_VERSION)
      SET(RV_DEPS_IMGUI_VERSION "${imgui_VERSION}")
      SET(RV_DEPS_IMGUI_VERSION "${imgui_VERSION}" CACHE STRING "" FORCE)
    ELSEIF(DEFINED IMGUI_VERSION)
      SET(RV_DEPS_IMGUI_VERSION "${IMGUI_VERSION}")
      SET(RV_DEPS_IMGUI_VERSION "${IMGUI_VERSION}" CACHE STRING "" FORCE)
    ELSEIF(DEFINED imgui_VERSION_STRING)
      SET(RV_DEPS_IMGUI_VERSION "${imgui_VERSION_STRING}")
      SET(RV_DEPS_IMGUI_VERSION "${imgui_VERSION_STRING}" CACHE STRING "" FORCE)
    ELSEIF(DEFINED IMGUI_VERSION_STRING)
      SET(RV_DEPS_IMGUI_VERSION "${IMGUI_VERSION_STRING}")
      SET(RV_DEPS_IMGUI_VERSION "${IMGUI_VERSION_STRING}" CACHE STRING "" FORCE)
    ENDIF()
    RETURN()
  ENDIF()
ENDIF()

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMGUI" "${RV_DEPS_IMGUI_VERSION}" "" "")

# There is no version suffix for imgui library name.
RV_MAKE_STANDARD_LIB_NAME("imgui" "" "SHARED" "")

SET(_lib_dir
    ${_install_dir}/lib
)

IF(RV_TARGET_WINDOWS)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}imgui${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}imgui${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_libpath
      ${_bin_dir}/${_libname}
  )
ELSE()
  SET(_libpath
      ${_lib_dir}/${_libname}
  )
ENDIF()

# Download implot from official repo
EXTERNALPROJECT_ADD(
  implot_download
  GIT_REPOSITORY "https://github.com/epezent/implot.git"
  GIT_TAG ${RV_DEPS_IMPLOT_TAG}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/implot
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  UPDATE_DISCONNECTED 1
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

# Download imgui_backend_qt (still using dpaulat as it's the standard for Qt integration)
EXTERNALPROJECT_ADD(
  imgui_backend_qt_download
  GIT_REPOSITORY "https://github.com/dpaulat/imgui-backend-qt.git"
  GIT_TAG ${RV_DEPS_IMGUI_BACKEND_QT_TAG}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-backend-qt
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  UPDATE_DISCONNECTED 1
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

# Download imgui-node-editor from official repo
EXTERNALPROJECT_ADD(
  imgui_node_editor_download
  GIT_REPOSITORY "https://github.com/thedmd/imgui-node-editor.git"
  GIT_TAG ${RV_DEPS_IMGUI_NODE_EDITOR_TAG}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-node-editor
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  UPDATE_DISCONNECTED 1
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

SET(_qt_location
    ${RV_DEPS_QT_LOCATION}
)
SET(_find_qt_version
    "Qt6"
)

EXTERNALPROJECT_ADD(
  ${_target}
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG ${RV_DEPS_IMGUI_VERSION}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  INSTALL_DIR ${_install_dir}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/src
  PATCH_COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/imgui/CMakeLists.txt ${CMAKE_BINARY_DIR}/${_target}/src/CMakeLists.txt && ${CMAKE_COMMAND} -E
    copy_directory ${CMAKE_BINARY_DIR}/${_target}/deps/implot ${CMAKE_BINARY_DIR}/${_target}/src/implot && ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-backend-qt/backends ${CMAKE_BINARY_DIR}/${_target}/src/backends && ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-node-editor ${CMAKE_BINARY_DIR}/${_target}/src/imgui-node-editor
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options} -DFIND_QT_VERSION=${_find_qt_version} -DCMAKE_PREFIX_PATH=${_qt_location}/lib/cmake
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  UPDATE_DISCONNECTED 1
  BUILD_BYPRODUCTS ${_libpath}
  BUILD_ALWAYS FALSE
  USES_TERMINAL_BUILD TRUE
  DEPENDS implot_download imgui_backend_qt_download imgui_node_editor_download
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

RV_ADD_IMPORTED_LIBRARY(
  NAME
  imgui::imgui
  TYPE
  SHARED
  LOCATION
  ${_libpath}
  SONAME
  ${_libname}
  IMPLIB
  ${_implibpath}
  INCLUDE_DIRS
  ${_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)
