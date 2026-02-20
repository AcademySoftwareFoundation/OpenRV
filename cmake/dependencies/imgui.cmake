#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMGUI" "${RV_DEPS_IMGUI_VERSION}" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_imgui_download_url
    "https://github.com/pthom/imgui/archive/refs/tags/${_version}.zip"
)

# Hashes for verification (replace with actual hash values)
SET(_imgui_download_hash
    ${RV_DEPS_IMGUI_DOWNLOAD_HASH}
)

# There is no version suffix for imgui library name.
RV_MAKE_STANDARD_LIB_NAME("imgui" "" "SHARED" "")

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

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

# Download implot into a separate directory
EXTERNALPROJECT_ADD(
  implot_download
  GIT_REPOSITORY "https://github.com/pthom/implot.git"
  GIT_TAG ${RV_DEPS_IMPLOT_TAG}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/implot
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

SET(_patch_command_for_imgui_backend_qt
    # This patch is needed to make the backend compatible with Qt 5 and Qt 6.
    patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/imgui_impl_qt.cpp.patch
)

# Download imgui_backend_qt into a separate directory
EXTERNALPROJECT_ADD(
  imgui_backend_qt_download
  GIT_REPOSITORY "https://github.com/dpaulat/imgui-backend-qt.git"
  GIT_TAG ${RV_DEPS_IMGUI_BACKEND_QT_TAG}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-backend-qt
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

# Download imgui-node-editor into a separate directory Using imgui-node-editor fork from imgui-bundle repository.
EXTERNALPROJECT_ADD(
  imgui_node_editor_download
  GIT_REPOSITORY "https://github.com/pthom/imgui-node-editor.git"
  GIT_TAG ${RV_DEPS_IMGUI_NODE_EDITOR_TAG}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-node-editor
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

SET(_qt_location
    ${RV_DEPS_QT_LOCATION}
)
SET(_find_qt_version
    "Qt${RV_DEPS_QT_MAJOR}"
)
IF(NOT _qt_location)
  SET(_qt_major
      ${RV_DEPS_QT_MAJOR}
  )
  MESSAGE(FATAL_ERROR "Qt is not found in path \"${_qt_location}\". Please provide -DRV_DEPS_QT_LOCATION=<path> to CMake.")
ENDIF()

SET(_patch_command_for_imgui
    patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/imgui_cpp_h.patch
)

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_imgui_download_url}
  URL_MD5 ${_imgui_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/src
  PATCH_COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/imgui/CMakeLists.txt ${CMAKE_BINARY_DIR}/${_target}/src/CMakeLists.txt && ${CMAKE_COMMAND} -E
    copy_directory ${CMAKE_BINARY_DIR}/${_target}/deps/implot ${CMAKE_BINARY_DIR}/${_target}/src/implot && ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-backend-qt/backends ${CMAKE_BINARY_DIR}/${_target}/src/backends && ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-node-editor ${CMAKE_BINARY_DIR}/${_target}/src/imgui-node-editor && ${_patch_command_for_imgui_backend_qt} &&
    ${_patch_command_for_imgui}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options} -DFIND_QT_VERSION=${_find_qt_version} -DCMAKE_PREFIX_PATH=${_qt_location}/lib/cmake
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_BYPRODUCTS ${_libpath}
  BUILD_ALWAYS FALSE
  USES_TERMINAL_BUILD TRUE
  DEPENDS implot_download imgui_backend_qt_download imgui_node_editor_download
)

IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} BIN_DIR ${_bin_dir} OUTPUTS ${RV_STAGE_BIN_DIR}/${_libname})
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname})
ENDIF()

ADD_LIBRARY(imgui::imgui SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(imgui::imgui ${_target})

SET_PROPERTY(
  TARGET imgui::imgui
  PROPERTY IMPORTED_LOCATION ${_libpath}
)

SET_PROPERTY(
  TARGET imgui::imgui
  PROPERTY IMPORTED_SONAME ${_libname}
)

IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET imgui::imgui
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  imgui::imgui
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST imgui::imgui)

# Set version for about dialog
SET(RV_DEPS_IMGUI_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
