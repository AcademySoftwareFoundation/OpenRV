#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_NANOBIND" "2.7.0" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_patch_command_nanobind_windows_debug "")
IF(RV_TARGET_WINDOWS)
  SET(_patch_command_nanobind_windows_debug
      patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/nanobind.windows.debug.patch
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_nanobind_python_executable
        ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python_d.exe
    )
  ELSE()
    SET(_nanobind_python_executable
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python3.exe
    )
  ENDIF()
ELSE()
  SET(_nanobind_python_executable
    ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python3
  )
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  GIT_REPOSITORY "https://github.com/wjakob/nanobind.git"
  GIT_TAG "44ad9a9e5729abda24ef8dc9d76233d801e651e9"
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  PATCH_COMMAND ${_patch_command_nanobind_windows_debug}
  CONFIGURE_COMMAND 
    ${CMAKE_COMMAND} ${_configure_options} -DPython_ROOT=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install -DPython_EXECUTABLE=${_nanobind_python_executable}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_install_dir}/nanobind/cmake/nanobind-config.cmake
  USES_TERMINAL_BUILD TRUE
  DEPENDS Python::Python
)

ADD_DEPENDENCIES(dependencies RV_DEPS_NANOBIND)