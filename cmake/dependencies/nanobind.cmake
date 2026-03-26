#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_NANOBIND" "2.7.0" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/wjakob/nanobind.git"
)

SET(_git_commit
    "44ad9a9e5729abda24ef8dc9d76233d801e651e9"
)

SET(_patch_command_nanobind_windows_debug
    ""
)
IF(RV_TARGET_WINDOWS
   AND CMAKE_BUILD_TYPE MATCHES "^Debug$"
)
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

# Set up dependencies - start with Python, add extra packages for CY2023.
SET(_nanobind_dependencies
    Python::Python
)

IF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(_nanobind_python_extra_packages
      "${_nanobind_python_executable}" -m pip install typing_extensions
  )

  # Create a stamp file to track nanobind installation
  SET(_nanobind_stamp
      ${CMAKE_CURRENT_BINARY_DIR}/${_target}-extra-packages-stamp
  )

  ADD_CUSTOM_COMMAND(
    OUTPUT ${_nanobind_stamp}
    COMMAND ${_nanobind_python_extra_packages}
    COMMAND ${CMAKE_COMMAND} -E touch ${_nanobind_stamp}
    COMMENT "Installing extra Python package for Python <3.11 and creating stamp file"
    DEPENDS Python::Python
  )

  # Add a custom target that depends on the stamp file
  ADD_CUSTOM_TARGET(
    ${_target}-extra-packages
    DEPENDS ${_nanobind_stamp}
    COMMENT "Ensuring Python packages are installed for ${_target}"
  )

  # Add extra packages to dependencies
  LIST(APPEND _nanobind_dependencies ${_target}-extra-packages)
ENDIF()

LIST(APPEND _configure_options "-DNB_TEST=OFF")
LIST(APPEND _configure_options "-DPython_ROOT=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install")
LIST(APPEND _configure_options "-DPython_EXECUTABLE=${_nanobind_python_executable}")

EXTERNALPROJECT_ADD(
  ${_target}
  GIT_REPOSITORY "${_download_url}"
  GIT_TAG "${_git_commit}"
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  UPDATE_COMMAND ""
  PATCH_COMMAND ${_patch_command_nanobind_windows_debug}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_install_dir}/nanobind/cmake/nanobind-config.cmake
  USES_TERMINAL_BUILD TRUE
  DEPENDS ${_nanobind_dependencies}
)

ADD_DEPENDENCIES(dependencies RV_DEPS_NANOBIND)
