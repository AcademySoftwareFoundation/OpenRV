#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official source repository: https://github.com/microsoft/vcpkg
#
# Vcpkg getting started page: https://vcpkg.io/en/getting-started.html
#
# Vcpkg manifest mode with cmake: https://learn.microsoft.com/en-us/vcpkg/examples/manifest-mode-cmake
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

IF(RV_TARGET_WINDOWS)
  RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_VCPKG" "2023.11.20" "" "")
  RV_SHOW_STANDARD_DEPS_VARIABLES()

  SET(_byproducts
      "${_source_dir}/vcpkg.exe"
  )
  EXTERNALPROJECT_ADD(
    ${_target}
    GIT_REPOSITORY https://github.com/Microsoft/vcpkg.git
    GIT_TAG ${_version}
    GIT_SHALLOW 1
    SOURCE_DIR ${_base_dir}/src
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )

  EXTERNALPROJECT_ADD_STEP(
    ${_target} bootstrap
    COMMENT "Calling bootstrap-vcpkg.bat"
    COMMAND ${_base_dir}/src/bootstrap-vcpkg.bat -disableMetrics
    DEPENDEES update
  )

  ADD_EXECUTABLE(VCPKG::VCPKG IMPORTED)
  ADD_DEPENDENCIES(VCPKG::VCPKG ${_target})
  SET_PROPERTY(
    TARGET VCPKG::VCPKG
    PROPERTY IMPORTED_LOCATION ${_source_dir}/vcpkg.exe
  )
ENDIF(RV_TARGET_WINDOWS)
