#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_python3_target
    "RV_DEPS_PYTHON3"
)

SET(_opentimelineio_target
    "RV_DEPS_OPENTIMELINEIO"
)

SET(_pyside_target
    "${RV_DEPS_PYSIDE_TARGET}"
)

SET(_python3_version
    "${RV_DEPS_PYTHON_VERSION}"
)
STRING(REPLACE "." ";" _python_version_list "${_python3_version}")

LIST(GET _python_version_list 0 PYTHON_VERSION_MAJOR)
LIST(GET _python_version_list 1 PYTHON_VERSION_MINOR)
LIST(GET _python_version_list 2 PYTHON_VERSION_PATCH)

SET(RV_DEPS_PYTHON_VERSION_SHORT
    "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}"
)

# This version is used for generating src/build/requirements.txt from requirements.txt.in template
SET(_opentimelineio_version
    "${RV_DEPS_OTIO_VERSION}"
)

SET(_pyside_version
    "${RV_DEPS_PYSIDE_VERSION}"
)

SET(_numpy_version
    "${RV_DEPS_NUMPY_VERSION}"
)

SET(_python3_download_url
    "https://github.com/python/cpython/archive/refs/tags/v${_python3_version}.zip"
)

SET(_python3_download_hash
    "${RV_DEPS_PYTHON_DOWNLOAD_HASH}"
)

SET(_opentimelineio_download_url
    "https://github.com/AcademySoftwareFoundation/OpenTimelineIO"
)

SET(_opentimelineio_git_tag
    "v${_opentimelineio_version}"
)

SET(_pyside_archive_url
    "${RV_DEPS_PYSIDE_ARCHIVE_URL}"
)

SET(_pyside_download_hash
    "${RV_DEPS_PYSIDE_DOWNLOAD_HASH}"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_python3_target}/install
)
SET(_source_dir
    ${RV_DEPS_BASE_DIR}/${_python3_target}/src
)
SET(_build_dir
    ${RV_DEPS_BASE_DIR}/${_python3_target}/build
)

FETCHCONTENT_DECLARE(
  ${_pyside_target}
  URL ${_pyside_archive_url}
  URL_HASH MD5=${_pyside_download_hash}
  SOURCE_SUBDIR "sources" # Avoids the top level CMakeLists.txt
)

FETCHCONTENT_MAKEAVAILABLE(${_pyside_target})

SET(_python3_make_command_script
    "${PROJECT_SOURCE_DIR}/src/build/make_python.py"
)
SET(_python3_make_command
    python3 "${_python3_make_command_script}"
)
LIST(APPEND _python3_make_command "--variant")
LIST(APPEND _python3_make_command ${CMAKE_BUILD_TYPE})
LIST(APPEND _python3_make_command "--source-dir")
LIST(APPEND _python3_make_command ${_source_dir})
LIST(APPEND _python3_make_command "--output-dir")
LIST(APPEND _python3_make_command ${_install_dir})
LIST(APPEND _python3_make_command "--temp-dir")
LIST(APPEND _python3_make_command ${_build_dir})

LIST(APPEND _python3_make_command "--vfx_platform")
LIST(APPEND _python3_make_command ${RV_VFX_CY_YEAR})

IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
  LIST(APPEND _python3_make_command "--openssl-dir")
  LIST(APPEND _python3_make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
ENDIF()
IF(RV_TARGET_WINDOWS)
  LIST(APPEND _python3_make_command "--python-version")
  LIST(APPEND _python3_make_command "${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
ENDIF()

IF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(_pyside_make_command_script
      "${PROJECT_SOURCE_DIR}/src/build/make_pyside.py"
  )
  SET(_pyside_make_command
      python3 "${_pyside_make_command_script}"
  )

  LIST(APPEND _pyside_make_command "--variant")
  LIST(APPEND _pyside_make_command ${CMAKE_BUILD_TYPE})
  LIST(APPEND _pyside_make_command "--source-dir")
  LIST(APPEND _pyside_make_command ${rv_deps_pyside2_SOURCE_DIR})
  LIST(APPEND _pyside_make_command "--output-dir")
  LIST(APPEND _pyside_make_command ${_install_dir})
  LIST(APPEND _pyside_make_command "--temp-dir")
  LIST(APPEND _pyside_make_command ${_build_dir})

  IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
    LIST(APPEND _pyside_make_command "--openssl-dir")
    LIST(APPEND _pyside_make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
  ENDIF()

  LIST(APPEND _pyside_make_command "--python-dir")
  LIST(APPEND _pyside_make_command ${_install_dir})
  LIST(APPEND _pyside_make_command "--qt-dir")
  LIST(APPEND _pyside_make_command ${RV_DEPS_QT_LOCATION})
  LIST(APPEND _pyside_make_command "--python-version")
  LIST(APPEND _pyside_make_command "${RV_DEPS_PYTHON_VERSION_SHORT}")
ELSEIF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
  SET(_pyside_make_command_script
      "${PROJECT_SOURCE_DIR}/src/build/make_pyside6.py"
  )
  SET(_pyside_make_command
      python3 "${_pyside_make_command_script}"
  )

  LIST(APPEND _pyside_make_command "--variant")
  LIST(APPEND _pyside_make_command ${CMAKE_BUILD_TYPE})
  LIST(APPEND _pyside_make_command "--source-dir")
  LIST(APPEND _pyside_make_command ${rv_deps_pyside6_SOURCE_DIR})
  LIST(APPEND _pyside_make_command "--output-dir")
  LIST(APPEND _pyside_make_command ${_install_dir})
  LIST(APPEND _pyside_make_command "--temp-dir")
  LIST(APPEND _pyside_make_command ${_build_dir})

  IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
    LIST(APPEND _pyside_make_command "--openssl-dir")
    LIST(APPEND _pyside_make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
  ENDIF()

  LIST(APPEND _pyside_make_command "--python-dir")
  LIST(APPEND _pyside_make_command ${_install_dir})
  LIST(APPEND _pyside_make_command "--qt-dir")
  LIST(APPEND _pyside_make_command ${RV_DEPS_QT_LOCATION})
  LIST(APPEND _pyside_make_command "--python-version")
  LIST(APPEND _pyside_make_command "${RV_DEPS_PYTHON_VERSION_SHORT}")
ENDIF()

IF(RV_TARGET_WINDOWS)
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(PYTHON3_EXTRA_WIN_LIBRARY_SUFFIX_IF_DEBUG
        "_d"
    )
  ELSE()
    SET(PYTHON3_EXTRA_WIN_LIBRARY_SUFFIX_IF_DEBUG
        ""
    )
  ENDIF()
  SET(_python_name
      python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}${PYTHON3_EXTRA_WIN_LIBRARY_SUFFIX_IF_DEBUG}
  )
  SET(_include_dir
      ${_install_dir}/include
  )
  SET(_bin_dir
      ${_install_dir}/bin
  )
  SET(_lib_dir
      ${_install_dir}/libs
  )
  SET(_python3_lib_name
      ${_python_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_python3_lib
      ${_bin_dir}/${_python3_lib_name}
  )
  SET(_python3_implib
      ${_lib_dir}/${_python_name}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  SET(_python3_executable
      ${_bin_dir}/python${PYTHON3_EXTRA_WIN_LIBRARY_SUFFIX_IF_DEBUG}.exe
  )

  # When building in Debug, we need the Release name also: see below for add_custom_command.
  SET(_python_release_libname
      python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
  SET(_python_release_libpath
      ${_lib_dir}/${_python_release_libname}
  )

  SET(_python_release_in_bin_libpath
      ${_bin_dir}/${_python_release_libname}
  )
ELSE() # Not WINDOWS
  SET(_python_name
      python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}
  )
  SET(_include_dir
      ${_install_dir}/include/${_python_name}
  )
  SET(_lib_dir
      ${_install_dir}/lib
  )
  SET(_python3_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}${_python_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_python3_lib
      ${_lib_dir}/${_python3_lib_name}
  )
  SET(_python3_executable
      ${_install_dir}/bin/python3
  )
ENDIF()

# Set the appropriate library for CMAKE_ARGS based on platform Windows needs the import library (.lib), Unix needs the shared library (.so/.dylib)
IF(RV_TARGET_WINDOWS)
  SET(_python3_cmake_library
      ${_python3_implib}
  )
ELSE()
  SET(_python3_cmake_library
      ${_python3_lib}
  )
ENDIF()

# Generate requirements.txt from template with the OpenTimelineIO and NumPy versions substituted
SET(_requirements_input_file
    "${PROJECT_SOURCE_DIR}/src/build/requirements.txt.in"
)
SET(_requirements_output_file
    "${CMAKE_BINARY_DIR}/requirements.txt"
)

CONFIGURE_FILE(${_requirements_input_file} ${_requirements_output_file} @ONLY)

# OpenTimelineIO needs to be built from source with CMAKE_ARGS to ensure it uses the correct custom-built Python libraries. This is required for both old and
# new versions of pybind11, especially pybind11 v2.13.6+ which has stricter detection. Note: pybind11's FindPythonLibsNew.cmake uses PYTHON_LIBRARY (all caps),
# PYTHON_INCLUDE_DIR, and PYTHON_EXECUTABLE variables. --no-cache-dir: Don't use pip's wheel cache (prevents using wheels built for wrong Python version)
# --force-reinstall: Reinstall packages even if already installed (ensures fresh build)

# Set OTIO_CXX_DEBUG_BUILD for all Debug builds to ensure OTIO's C++ extensions are built with debug symbols and proper optimization levels matching RV's build
# type. On Windows, this also ensures OTIO links against the debug Python library (python311_d.lib).
IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
  SET(_otio_debug_env
      "OTIO_CXX_DEBUG_BUILD=1"
  )
ELSE()
  SET(_otio_debug_env
      ""
  )
ENDIF()

# Build dependencies that must be installed FIRST before building other packages. These are installed from wheels in a separate pip command to ensure they're
# available when packages like PyOpenGL_accelerate need Cython to compile .pyx files.
SET(RV_PYTHON_BUILD_DEPS
    "pip" # Package installer (pure Python)
    "setuptools" # Build system (pure Python)
    "wheel" # Wheel format support (pure Python)
    "Cython" # Build tool - MUST be installed before packages with .pyx files
    "meson-python" # Build backend (pure Python)
    "ninja" # Build tool (native but doesn't link to Python)
    CACHE STRING "Build dependencies to install first (from wheels)"
)

# List of packages that are safe to install from pre-built wheels. All other packages (those with C/C++/Rust extensions) will be built from source to ensure
# proper linking against our custom Python build. Packages are built from source unless explicitly listed here. This includes: pure Python packages, build tools
# that don't need ABI compatibility, and packages with data files only.
SET(RV_PYTHON_WHEEL_SAFE
    ${RV_PYTHON_BUILD_DEPS} # Include build deps in wheel-safe list
    "PyOpenGL" # OpenGL bindings without acceleration (pure Python)
    "certifi" # SSL certificate bundle (just data files)
    "six" # Python 2/3 compatibility (pure Python)
    "packaging" # Version parsing (pure Python)
    "requests" # HTTP library (pure Python)
    CACHE STRING "Packages safe to install from wheels (pure Python or build tools)"
)

# Convert lists to space/comma-separated strings for pip commands
STRING(REPLACE ";" "," _wheel_safe_packages "${RV_PYTHON_WHEEL_SAFE}")

# On Mac, Ensure we build the python packages with the SDK that matches the version of XCode we use to build python
IF(RV_TARGET_DARWIN
   AND CMAKE_OSX_SYSROOT
)
  MESSAGE(STATUS "Using macOS SDK for Python packages: ${CMAKE_OSX_SYSROOT}")
  SET(_sdkroot_env
      "SDKROOT=${CMAKE_OSX_SYSROOT}"
  )
ENDIF()

# Phase 1: Install build dependencies for phase 2. Note: RV_PYTHON_BUILD_DEPS is kept as a CMake list (semicolon-separated) so it expands to separate arguments.
SET(_build_deps_install_command
    ${CMAKE_COMMAND} -E env ${_sdkroot_env} "${_python3_executable}" -s -E -I -m pip install --upgrade --no-cache-dir ${RV_PYTHON_BUILD_DEPS}
)

# Phase 2: Install main requirements (with build-from-source for native extensions)
SET(_requirements_install_command
    ${CMAKE_COMMAND} -E env ${_otio_debug_env} ${_sdkroot_env}
)

# Only set OPENSSL_DIR if we built OpenSSL ourselves (not for Rocky Linux 8 CY2023 which uses system OpenSSL)
IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
  LIST(APPEND _requirements_install_command "OPENSSL_DIR=${RV_DEPS_OPENSSL_INSTALL_DIR}")
ENDIF()

# Build all packages from source except those in RV_PYTHON_WHEEL_SAFE. Packages with native extensions (opentimelineio, numpy, PyOpenGL-accelerate,
# cryptography, pydantic, cffi, etc.) will be built from source for proper ABI compatibility.
LIST(
  APPEND
  _requirements_install_command
  "CMAKE_ARGS=-DPYTHON_LIBRARY=${_python3_cmake_library} -DPYTHON_INCLUDE_DIR=${_include_dir} -DPYTHON_EXECUTABLE=${_python3_executable}"
  "${_python3_executable}"
  -s
  -E
  -I
  -m
  pip
  install
  --upgrade
  --no-cache-dir
  --force-reinstall
  --no-binary
  :all:
  --only-binary
  ${_wheel_safe_packages}
  -r
  "${_requirements_output_file}"
)

IF(RV_TARGET_WINDOWS)
  SET(_patch_python_command
      "patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/python-${RV_DEPS_PYTHON_VERSION}/python.${RV_DEPS_PYTHON_VERSION}.openssl.props.patch &&\
       patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/python-${RV_DEPS_PYTHON_VERSION}/python.${RV_DEPS_PYTHON_VERSION}.python.props.patch &&\
       patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patch/python-${RV_DEPS_PYTHON_VERSION}/python.${RV_DEPS_PYTHON_VERSION}.get_externals.bat.patch"
  )

  # TODO: Above patches are for Python 3.11.9, need to add other versions.
  RV_VFX_SET_VARIABLE(
    _patch_command
    CY2023
    ""
    CY2024
    "${_patch_python_command}"
    CY2025
    "${_patch_python_command}"
    CY2026
    ""
  )
  # Split the command into a semi-colon separated list.
  SEPARATE_ARGUMENTS(_patch_command)
  STRING(
    REGEX
    REPLACE ";+" ";" _patch_command "${_patch_command}"
  )
ENDIF()

EXTERNALPROJECT_ADD(
  ${_python3_target}
  DOWNLOAD_NAME ${_python3_target}_${_python3_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  URL ${_python3_download_url}
  URL_MD5 ${_python3_download_hash}
  DEPENDS OpenSSL::Crypto OpenSSL::SSL
  CONFIGURE_COMMAND ${_python3_make_command} --configure
  BUILD_COMMAND ${_python3_make_command} --build
  INSTALL_COMMAND ${_python3_make_command} --install
  BUILD_BYPRODUCTS ${_python3_executable} ${_python3_lib} ${_python3_implib}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  USES_TERMINAL_BUILD TRUE
)

# Phase 1 flag: Build dependencies (Cython, etc.)
SET(${_python3_target}-build-deps-flag
    ${_install_dir}/${_python3_target}-build-deps-flag
)

ADD_CUSTOM_COMMAND(
  COMMENT "Installing Python build dependencies (pip, setuptools, wheel, Cython, etc.)"
  OUTPUT ${${_python3_target}-build-deps-flag}
  COMMAND ${_build_deps_install_command}
  COMMAND cmake -E touch ${${_python3_target}-build-deps-flag}
  DEPENDS ${_python3_target}
)

# Phase 2 flag: Main requirements (depends on build deps being installed first)
SET(${_python3_target}-requirements-flag
    ${_install_dir}/${_python3_target}-requirements-flag
)

ADD_CUSTOM_COMMAND(
  COMMENT "Installing requirements from ${_requirements_output_file} pyside and other dependencies"
  OUTPUT ${${_python3_target}-requirements-flag}
  COMMAND ${_requirements_install_command}
  COMMAND cmake -E touch ${${_python3_target}-requirements-flag}
  DEPENDS ${_python3_target} ${${_python3_target}-build-deps-flag} ${_requirements_output_file} ${_requirements_input_file}
)

# Test the Python distribution after requirements are installed
SET(${_python3_target}-test-flag
    ${_install_dir}/${_python3_target}-test-flag
)

SET(_test_python_script
    "${PROJECT_SOURCE_DIR}/src/build/test_python.py"
)

ADD_CUSTOM_COMMAND(
  COMMENT "Testing Python distribution"
  OUTPUT ${${_python3_target}-test-flag}
  COMMAND python3 "${_test_python_script}" --python-home "${_install_dir}" --variant "${CMAKE_BUILD_TYPE}"
  COMMAND cmake -E touch ${${_python3_target}-test-flag}
  DEPENDS ${${_python3_target}-requirements-flag} ${_test_python_script}
)

IF(RV_TARGET_WINDOWS
   AND CMAKE_BUILD_TYPE MATCHES "^Debug$"
)
  # OCIO v2.2's pybind11 doesn't find python<ver>.lib in Debug since the name is python<ver>_d.lib. Also, Rust libraries (like cryptography via pyo3) look for
  # python3.lib.
  ADD_CUSTOM_COMMAND(
    TARGET ${_python3_target}
    POST_BUILD
    COMMENT "Copying Debug Python lib as a unversionned file for Debug"
    COMMAND cmake -E copy_if_different ${_python3_implib} ${_python_release_libpath}
    COMMAND cmake -E copy_if_different ${_python3_implib} ${_python_release_in_bin_libpath}
    COMMAND cmake -E copy_if_different ${_lib_dir}/python${PYTHON_VERSION_MAJOR}_d.lib ${_lib_dir}/python${PYTHON_VERSION_MAJOR}.lib
    COMMAND cmake -E copy_if_different ${_bin_dir}/python${PYTHON_VERSION_MAJOR}_d.lib ${_bin_dir}/python${PYTHON_VERSION_MAJOR}.lib DEPENDS ${_python3_target}
            ${_requirements_output_file} ${_requirements_input_file}
  )
ENDIF()

SET(${_pyside_target}-build-flag
    ${_install_dir}/${_pyside_target}-build-flag
)

IF(RV_VFX_PLATFORM STREQUAL CY2023)
  ADD_CUSTOM_COMMAND(
    COMMENT "Building PySide2 using ${_pyside_make_command_script}"
    OUTPUT ${${_pyside_target}-build-flag}
    # First PySide build script on Windows which doesn't respect '--debug' option
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/build/patch_PySide2/windows_desktop.py
            ${rv_deps_pyside2_SOURCE_DIR}/build_scripts/platforms/windows_desktop.py
    COMMAND ${_pyside_make_command} --prepare --build
    COMMAND cmake -E touch ${${_pyside_target}-build-flag}
    DEPENDS ${_python3_target} ${_pyside_make_command_script} ${${_python3_target}-requirements-flag} ${${_python3_target}-test-flag}
    USES_TERMINAL
  )

  SET(_build_flag_depends
      ${${_pyside_target}-build-flag}
  )
ELSEIF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
  ADD_CUSTOM_COMMAND(
    COMMENT "Building PySide6 using ${_pyside_make_command_script}"
    OUTPUT ${${_pyside_target}-build-flag}
    COMMAND ${_pyside_make_command} --prepare --build
    COMMAND cmake -E touch ${${_pyside_target}-build-flag}
    DEPENDS ${_python3_target} ${_pyside_make_command_script} ${${_python3_target}-requirements-flag} ${${_python3_target}-test-flag}
    USES_TERMINAL
  )

  SET(_build_flag_depends
      ${${_pyside_target}-build-flag}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_copy_commands
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR} COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/include
      ${RV_STAGE_INCLUDE_DIR} COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
  )

  IF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
    LIST(
      APPEND
      _copy_commands
      COMMAND
      ${CMAKE_COMMAND}
      -E
      copy_directory
      ${_install_dir}/DLLs
      ${RV_STAGE_ROOT_DIR}/DLLs
    )
  ENDIF()

  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_python3_target}'s include and libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_BIN_DIR}/${_python3_lib_name} ${_copy_commands}
    DEPENDS ${_python3_target} ${${_python3_target}-requirements-flag} ${${_python3_target}-test-flag} ${_build_flag_depends}
  )

  ADD_CUSTOM_TARGET(
    ${_python3_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_python3_lib_name}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_python3_target}'s include and libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_python3_lib_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/include ${RV_STAGE_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
    DEPENDS ${_python3_target} ${${_python3_target}-requirements-flag} ${${_python3_target}-test-flag} ${_build_flag_depends}
  )
  ADD_CUSTOM_TARGET(
    ${_python3_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_python3_lib_name}
  )
ENDIF()

ADD_LIBRARY(Python::Python SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(Python::Python ${_python3_target})
SET_PROPERTY(
  TARGET Python::Python
  PROPERTY IMPORTED_LOCATION ${_python3_lib}
)
SET_PROPERTY(
  TARGET Python::Python
  PROPERTY IMPORTED_SONAME ${_python3_lib_name}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET Python::Python
    PROPERTY IMPORTED_IMPLIB ${_python3_implib}
  )
ENDIF()
FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  Python::Python
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST Python::Python)

ADD_DEPENDENCIES(dependencies ${_python3_target}-stage-target)

SET(RV_DEPS_PYTHON3_VERSION
    ${_python3_version}
    CACHE INTERNAL "" FORCE
)
SET(RV_DEPS_PYSIDE_VERSION
    ${_pyside_version}
    CACHE INTERNAL "" FORCE
)

SET(RV_DEPS_PYTHON3_EXECUTABLE
    ${_python3_executable}
    CACHE INTERNAL "" FORCE
)
