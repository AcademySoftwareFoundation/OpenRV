#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMGUI" "bundle_20250323" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_imgui_download_url
    "https://github.com/pthom/imgui/archive/refs/tags/${_version}.zip"
)

# Hashes for verification (replace with actual hash values)
SET(_imgui_download_hash
    "1ea3f48e9c6ae8230dac6e8a54f6e74b"
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
  GIT_TAG "61af48ee1369083a3da391a849867af6d1b811a6"
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
  GIT_TAG "023345ca8abf731fc50568c0197ceebe76bb4324"
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
  GIT_TAG "dae8edccf15d19e995599ecd505e7fa1d3264a4c"
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

RV_VFX_SET_VARIABLE(_qt_location CY2023 "${RV_DEPS_QT5_LOCATION}" CY2024 "${RV_DEPS_QT6_LOCATION}")
RV_VFX_SET_VARIABLE(_find_qt_version CY2023 "Qt5" CY2024 "Qt6")
IF(NOT _qt_location)
  RV_VFX_SET_VARIABLE(_qt_major CY2023 "5" CY2024 "6")
  MESSAGE(FATAL_ERROR "Qt is not found in path \"${_qt_location}\". Please provide -DRV_DEPS_QT${_qt_major}_LOCATION=<path> to CMake.")
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
    ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-node-editor ${CMAKE_BINARY_DIR}/${_target}/src/imgui-node-editor 
    && ${_patch_command_for_imgui_backend_qt} && ${_patch_command_for_imgui}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options} -DFIND_QT_VERSION=${_find_qt_version} -DCMAKE_PREFIX_PATH=${_qt_location}/lib/cmake
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_BYPRODUCTS ${_libpath}
  BUILD_ALWAYS FALSE
  USES_TERMINAL_BUILD TRUE
  DEPENDS implot_download imgui_backend_qt_download imgui_node_editor_download
)

RV_COPY_LIB_BIN_FOLDERS()

ADD_LIBRARY(imgui::imgui SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(dependencies ${_target}-stage-target)
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
