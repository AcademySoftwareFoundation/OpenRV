#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# python-build-standalone (PBS) variant of RV's embedded Python.
#
# This module is included from python3.cmake when RV_DEPS_PYTHON_USE_PBS=ON and the configuration is not Debug. Instead of compiling CPython + PySide6 + native
# packages from source, it:
#
# 1. Downloads a PBS "install_only" prebuilt CPython for the target platform.
# 2. Runs src/build/adapt_pbs_python.py to: - fix the Windows binary layout (python*.dll/exe -> bin/), - inject RV's sitecustomize.py (certifi CA bundle +
#    sys.path reorder), - repoint the PySide6 wheel's bundled Qt at RV's Qt (--qt), so a single Qt serves both RV's C++ app and PySide6 (required by
#    wrapInstance).
# 3. Installs requirements_pbs.txt (wheels, including PySide6/shiboken6).
#
# It defines the SAME outputs as the from-source python3.cmake so the rest of the build is unaffected: - imported target  Python::Python
# (IMPORTED_LOCATION/SONAME/INCLUDE/IMPLIB) - custom target    ${_python3_target}-stage-target  (stages lib/include/bin) - cache vars RV_DEPS_PYTHON3_VERSION,
# RV_DEPS_PYSIDE_VERSION, RV_DEPS_PYTHON3_EXECUTABLE
#
# The caller (python3.cmake) is expected to have already computed: _python3_target, _python3_version, _pyside_version, _install_dir, _source_dir, _bin_dir,
# _lib_dir, _include_dir, _python3_executable, _python3_lib, _python3_lib_name, PYTHON_VERSION_MAJOR/MINOR, and the RV_STAGE_* dirs.
#
# ---------------------------------------------------------------------------
# VALIDATION STATUS: the adapt mechanism (layout fix, sitecustomize, Qt repoint) is validated on macOS (see investigation/pbs-python). The per-platform PBS
# asset pins below and the CMake target graph require CI validation on each of macOS / Linux / Windows before this path is enabled by default.
# ---------------------------------------------------------------------------

# --- 1. Select the PBS asset for this platform -----------------------------
#
# The release tag and the per-triple sha256 pins are provided by the active CY*.cmake defaults file (next to RV_DEPS_PYTHON_VERSION), so they version together
# with the interpreter. This module only maps the current platform triple to the matching pin. A release job may still override the final URL/hash on the
# command line via -DRV_DEPS_PYTHON_PBS_URL / -DRV_DEPS_PYTHON_PBS_HASH.

IF(RV_TARGET_DARWIN)
  IF(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    SET(_pbs_triple
        "aarch64-apple-darwin"
    )
    SET(_pbs_triple_hash
        "${RV_DEPS_PYTHON_PBS_HASH_AARCH64_APPLE_DARWIN}"
    )
  ELSE()
    SET(_pbs_triple
        "x86_64-apple-darwin"
    )
    SET(_pbs_triple_hash
        "${RV_DEPS_PYTHON_PBS_HASH_X86_64_APPLE_DARWIN}"
    )
  ENDIF()
ELSEIF(RV_TARGET_LINUX)
  SET(_pbs_triple
      "x86_64-unknown-linux-gnu"
  )
  SET(_pbs_triple_hash
      "${RV_DEPS_PYTHON_PBS_HASH_X86_64_LINUX_GNU}"
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_pbs_triple
      "x86_64-pc-windows-msvc"
  )
  SET(_pbs_triple_hash
      "${RV_DEPS_PYTHON_PBS_HASH_X86_64_WINDOWS_MSVC}"
  )
ELSE()
  MESSAGE(FATAL_ERROR "RV_DEPS_PYTHON_USE_PBS: unsupported platform")
ENDIF()

# Command-line override wins; otherwise use the per-triple pin from CY*.cmake.
IF(NOT DEFINED RV_DEPS_PYTHON_PBS_HASH
   OR RV_DEPS_PYTHON_PBS_HASH STREQUAL ""
)
  SET(RV_DEPS_PYTHON_PBS_HASH
      "${_pbs_triple_hash}"
  )
ENDIF()

IF(NOT DEFINED RV_DEPS_PYTHON_PBS_URL
   OR RV_DEPS_PYTHON_PBS_URL STREQUAL ""
)
  IF(NOT DEFINED RV_DEPS_PYTHON_PBS_RELEASE_TAG
     OR RV_DEPS_PYTHON_PBS_RELEASE_TAG STREQUAL ""
  )
    MESSAGE(
      FATAL_ERROR
        "RV_DEPS_PYTHON_USE_PBS=ON but no PBS release tag is pinned. Set RV_DEPS_PYTHON_PBS_RELEASE_TAG in the active CY*.cmake (next to RV_DEPS_PYTHON_VERSION), "
        "or pass -DRV_DEPS_PYTHON_PBS_URL. See https://github.com/astral-sh/python-build-standalone/releases for the "
        "'cpython-${_python3_version}+<tag>-${_pbs_triple}-install_only.tar.gz' asset."
    )
  ENDIF()
  # install_only archives are named: cpython-<version>+<tag>-<triple>-install_only.tar.gz
  SET(RV_DEPS_PYTHON_PBS_URL
      "https://github.com/astral-sh/python-build-standalone/releases/download/${RV_DEPS_PYTHON_PBS_RELEASE_TAG}/cpython-${_python3_version}+${RV_DEPS_PYTHON_PBS_RELEASE_TAG}-${_pbs_triple}-install_only.tar.gz"
  )
ENDIF()

IF(RV_DEPS_PYTHON_PBS_HASH STREQUAL "")
  MESSAGE(FATAL_ERROR "RV_DEPS_PYTHON_USE_PBS=ON but no sha256 pin found for triple '${_pbs_triple}' at Python ${_python3_version}. "
                      "Add RV_DEPS_PYTHON_PBS_HASH_<TRIPLE> to the active CY*.cmake, or pass -DRV_DEPS_PYTHON_PBS_HASH."
  )
ENDIF()

MESSAGE(STATUS "RV_DEPS_PYTHON_USE_PBS: fetching ${RV_DEPS_PYTHON_PBS_URL} (sha256=${RV_DEPS_PYTHON_PBS_HASH})")

# --- 2. Locate RV's Qt to repoint PySide6 at -------------------------------
#
# RV_DEPS_QT_LOCATION (mandatory, set in qt6.cmake) points at RV's Qt install; QT_HOME is a defensive fallback. adapt_pbs_python.py resolves the
# framework/so/dll directory beneath it.
#
# This MUST resolve: with PBS we install a PySide6 wheel that bundles its own Qt. If we don't repoint it at RV's Qt, the process loads two different Qt copies
# and wrapInstance() hands RV's QMainWindow* to a foreign Qt with an incompatible object layout -> runtime crash. Fail loudly at configure time rather than ship
# a build that crashes when the UI touches PySide.
SET(_rv_qt_root
    "${RV_DEPS_QT_LOCATION}"
)
IF(_rv_qt_root STREQUAL "")
  SET(_rv_qt_root
      "$ENV{QT_HOME}"
  )
ENDIF()
IF(_rv_qt_root STREQUAL "")
  MESSAGE(FATAL_ERROR "RV_DEPS_PYTHON_USE_PBS=ON requires RV's Qt location so the PySide6 wheel's bundled Qt can be repointed at it. "
                      "Set -DRV_DEPS_QT_LOCATION=<path to RV's Qt root> (same Qt version as the PySide6 wheel)."
  )
ENDIF()

# --- 3. Download + extract + adapt + install wheels ------------------------

SET(_adapt_script
    "${PROJECT_SOURCE_DIR}/src/build/adapt_pbs_python.py"
)

SET(_pbs_url_hash_arg
    ""
)
IF(NOT RV_DEPS_PYTHON_PBS_HASH STREQUAL "")
  SET(_pbs_url_hash_arg
      URL_HASH "SHA256=${RV_DEPS_PYTHON_PBS_HASH}"
  )
ENDIF()

# _rv_qt_root is guaranteed non-empty by the FATAL_ERROR check above.
SET(_adapt_qt_arg
    --qt "${_rv_qt_root}"
)

# Interpreter that exists immediately after extraction, i.e. BEFORE the Windows layout fix moves python.exe into bin/. On Windows PBS install_only, python.exe
# sits at the install root; on Unix it is bin/python3. adapt_pbs_python.py only uses the standard library, so any of these can run it.
IF(RV_TARGET_WINDOWS)
  SET(_pbs_bootstrap_python
      "${_install_dir}/python.exe"
  )
ELSE()
  SET(_pbs_bootstrap_python
      "${_install_dir}/bin/python3"
  )
ENDIF()

# The PBS archive extracts to a top-level "python/" directory; we point the install dir at that so _install_dir/bin, _install_dir/lib line up with the paths
# python3.cmake already computed.
EXTERNALPROJECT_ADD(
  ${_python3_target}
  URL ${RV_DEPS_PYTHON_PBS_URL} ${_pbs_url_hash_arg}
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  # Order matters: 1. Stage the extracted "python/" tree into _install_dir. 2. pip install the wheels (PySide6, native extensions, certifi, ...). This MUST
  # happen before adapt, because adapt's Qt repoint needs PySide6 to already be present in site-packages. We run pip with the bootstrap interpreter (root
  # python.exe on Windows / bin/python3 on Unix) since the Windows layout fix that creates bin/python.exe is part of adapt and hasn't run yet. 3. adapt: Windows
  # layout fix, sitecustomize injection, and PySide6 Qt repoint (PySide6 now exists).
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${_source_dir} ${_install_dir}
  COMMAND "${_pbs_bootstrap_python}" -s -E -m pip install --upgrade --no-cache-dir -r "${CMAKE_BINARY_DIR}/requirements_pbs.txt"
  COMMAND "${_pbs_bootstrap_python}" "${_adapt_script}" --install "${_install_dir}" ${_adapt_qt_arg}
  BUILD_BYPRODUCTS ${_python3_lib} ${_python3_executable}
  USES_TERMINAL_DOWNLOAD TRUE
  USES_TERMINAL_INSTALL TRUE
)

# --- 4. Staging (mirror python3.cmake) -------------------------------------

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    COMMENT "Staging ${_python3_target} (PBS) into ${RV_STAGE_BIN_DIR}"
    OUTPUT ${RV_STAGE_BIN_DIR}/${_python3_lib_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/include ${RV_STAGE_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
    DEPENDS ${_python3_target}
  )
  ADD_CUSTOM_TARGET(
    ${_python3_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_python3_lib_name}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Staging ${_python3_target} (PBS) into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_python3_lib_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/include ${RV_STAGE_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
    DEPENDS ${_python3_target}
  )
  ADD_CUSTOM_TARGET(
    ${_python3_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_python3_lib_name}
  )
ENDIF()

# --- 5. Imported target (mirror python3.cmake) -----------------------------

ADD_LIBRARY(Python::Python SHARED IMPORTED GLOBAL)
SET_TARGET_PROPERTIES(
  Python::Python
  PROPERTIES SYSTEM FALSE
)
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
  SET(_python_release_implib
      ${_lib_dir}/python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  # CMAKE_CONFIGURATION_TYPES always includes Debug on MSVC (see root CMakeLists.txt), so the multi-config generator validates Python::Python against a Debug
  # configuration too, even when only Release is being built. PBS is gated to non-Debug (python3.cmake), so there is no Debug import lib to point at; set the
  # generic (config-unsuffixed) property as the fallback CMake uses for any configuration without its own IMPORTED_IMPLIB_<CONFIG>.
  SET_PROPERTY(
    TARGET Python::Python
    PROPERTY IMPORTED_IMPLIB ${_python_release_implib}
  )
  SET_PROPERTY(
    TARGET Python::Python
    PROPERTY IMPORTED_IMPLIB_RELEASE ${_python_release_implib}
  )
  SET_PROPERTY(
    TARGET Python::Python
    PROPERTY IMPORTED_IMPLIB_RELWITHDEBINFO ${_python_release_implib}
  )
  SET_PROPERTY(
    TARGET Python::Python
    PROPERTY IMPORTED_IMPLIB_MINSIZEREL ${_python_release_implib}
  )
  TARGET_LINK_DIRECTORIES(Python::Python INTERFACE ${_lib_dir})
ENDIF()
FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  Python::Python
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST Python::Python)

ADD_DEPENDENCIES(dependencies ${_python3_target}-stage-target)

# --- 6. Output cache vars (mirror python3.cmake) ---------------------------

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
