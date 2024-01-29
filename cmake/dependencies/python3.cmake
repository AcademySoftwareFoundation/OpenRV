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

SET(_pyside2_target
    "RV_DEPS_PYSIDE2"
)

SET(PYTHON_VERSION_MAJOR
    3
)

SET(PYTHON_VERSION_MINOR
    10
)

SET(PYTHON_VERSION_PATCH
    13
)

SET(_python3_version
    "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.${PYTHON_VERSION_PATCH}"
)

SET(RV_DEPS_PYTHON_VERSION_MAJOR
    ${PYTHON_VERSION_MAJOR}
)
SET(RV_DEPS_PYTHON_VERSION_SHORT
    "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}"
)

SET(_opentimelineio_version
    "0.15"
)

SET(_pyside2_version
    "5.15.10"
)

SET(_python3_download_url
    "https://github.com/python/cpython/archive/refs/tags/v${_python3_version}.zip"
)
SET(_python3_download_hash
    "21b32503f31386b37f0c42172dfe5637"
)

SET(_opentimelineio_download_url
    "https://github.com/AcademySoftwareFoundation/OpenTimelineIO"
)
SET(_opentimelineio_git_tag
    "v${_opentimelineio_version}"
)

SET(_pyside2_archive_url
    "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside2/PySide2-${_pyside2_version}-src/pyside-setup-opensource-src-${_pyside2_version}.zip"
)
SET(_pyside2_download_hash
    "87841aaced763b6b52e9b549e31a493f"
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

IF(RV_TARGET_WINDOWS)

  FETCHCONTENT_DECLARE(
    ${_opentimelineio_target}
    GIT_REPOSITORY ${_opentimelineio_download_url}
    GIT_TAG ${_opentimelineio_git_tag}
    SOURCE_SUBDIR "src" # Avoids the top level CMakeLists.txt
  )

  FETCHCONTENT_MAKEAVAILABLE(${_opentimelineio_target})

ENDIF()

FETCHCONTENT_DECLARE(
  ${_pyside2_target}
  URL ${_pyside2_archive_url}
  URL_HASH MD5=${_pyside2_download_hash}
  SOURCE_SUBDIR "sources" # Avoids the top level CMakeLists.txt
)

FETCHCONTENT_MAKEAVAILABLE(${_pyside2_target})

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
IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
  LIST(APPEND _python3_make_command "--openssl-dir")
  LIST(APPEND _python3_make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
ENDIF()
IF(RV_TARGET_WINDOWS)
  LIST(APPEND _python3_make_command "--opentimelineio-source-dir")
  LIST(APPEND _python3_make_command ${rv_deps_opentimelineio_SOURCE_DIR})
ENDIF()

IF(${RV_OSX_EMULATION})
  LIST(APPEND _python3_make_command --arch=${RV_OSX_EMULATION_ARCH})
ENDIF()

SET(_pyside2_make_command_script
    "${PROJECT_SOURCE_DIR}/src/build/make_pyside.py"
)
SET(_pyside2_make_command
    python3 "${_pyside2_make_command_script}"
)
LIST(APPEND _pyside2_make_command "--variant")
LIST(APPEND _pyside2_make_command ${CMAKE_BUILD_TYPE})
LIST(APPEND _pyside2_make_command "--source-dir")
LIST(APPEND _pyside2_make_command ${rv_deps_pyside2_SOURCE_DIR})
LIST(APPEND _pyside2_make_command "--output-dir")
LIST(APPEND _pyside2_make_command ${_install_dir})
LIST(APPEND _pyside2_make_command "--temp-dir")
LIST(APPEND _pyside2_make_command ${_build_dir})
IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
  LIST(APPEND _pyside2_make_command "--openssl-dir")
  LIST(APPEND _pyside2_make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
ENDIF()
LIST(APPEND _pyside2_make_command "--python-dir")
LIST(APPEND _pyside2_make_command ${_install_dir})
LIST(APPEND _pyside2_make_command "--qt-dir")
LIST(APPEND _pyside2_make_command ${RV_DEPS_QT5_LOCATION})

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

SET(_requirements_file
    "${PROJECT_SOURCE_DIR}/src/build/requirements.txt"
)
SET(_requirements_install_command
    "${_python3_executable}" -m pip install --upgrade -r "${_requirements_file}"
)

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

SET(${_python3_target}-requirements-flag
    ${_install_dir}/${_python3_target}-requirements-flag
)

ADD_CUSTOM_COMMAND(
  COMMENT "Installing requirements from ${_requirements_file}"
  OUTPUT ${${_python3_target}-requirements-flag}
  COMMAND ${_requirements_install_command}
  COMMAND ${CMAKE_COMMAND} -E touch ${${_python3_target}-requirements-flag}
  DEPENDS ${_python3_target} ${_requirements_file}
)

IF(RV_TARGET_WINDOWS
   AND CMAKE_BUILD_TYPE MATCHES "^Debug$"
)
  # OCIO v2.2's pybind11 doesn't find python<ver>.lib in Debug since the name is python<ver>_d.lib.
  ADD_CUSTOM_COMMAND(
    TARGET ${_python3_target}
    POST_BUILD
    COMMENT "Copying Debug Python lib as a unversionned file for Debug"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_python3_implib}" --destination
            "${_python_release_libpath}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_python3_implib}" --destination
            "${_python_release_in_bin_libpath}" DEPENDS ${_python3_target} ${_requirements_file}
  )
ENDIF()

SET(${_pyside2_target}-build-flag
    ${_install_dir}/${_pyside2_target}-build-flag
)

ADD_CUSTOM_COMMAND(
  COMMENT "Building PySide2 using ${_pyside2_make_command_script}"
  OUTPUT ${${_pyside2_target}-build-flag}
  # First PySide build script on Windows which doesn't respect '--debug' option
  COMMAND
    python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source
    "${PROJECT_SOURCE_DIR}/src/build/patch_PySide2/windows_desktop.py" --destination "${rv_deps_pyside2_SOURCE_DIR}/build_scripts/platforms/windows_desktop.py"
  COMMAND ${_pyside2_make_command} --prepare --build
  COMMAND cmake -E touch ${${_pyside2_target}-build-flag}
  DEPENDS ${_python3_target} ${_pyside2_make_command_script} ${${_python3_target}-requirements-flag}
  USES_TERMINAL
)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_python3_target}'s include and libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_BIN_DIR}/${_python3_lib_name}
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/lib" --destination
            "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/include" --destination
            "${RV_STAGE_INCLUDE_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/bin" --destination
            "${RV_STAGE_BIN_DIR}"
    COMMAND python3 "${PROJECT_SOURCE_DIR}/src/build/fix_pyside2_qt_copy.py" --python-executable "${RV_STAGE_BIN_DIR}/python3.exe" --libraries
            "${RV_STAGE_LIB_DIR}" --plugins "${RV_STAGE_PLUGINS_QT_DIR}"
    DEPENDS ${_python3_target} ${${_pyside2_target}-build-flag} ${${_python3_target}-requirements-flag}
  )
  ADD_CUSTOM_TARGET(
    ${_python3_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_python3_lib_name}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_python3_target}'s include and libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_python3_lib_name}
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/lib" --destination
            "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/include" --destination
            "${RV_STAGE_INCLUDE_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/bin" --destination
            "${RV_STAGE_BIN_DIR}"
    COMMAND python3 "${PROJECT_SOURCE_DIR}/src/build/fix_pyside2_qt_copy.py" --python-executable "${RV_STAGE_BIN_DIR}/python3" --libraries "${RV_STAGE_LIB_DIR}"
            --plugins "${RV_STAGE_PLUGINS_QT_DIR}"
    DEPENDS ${_python3_target} ${${_pyside2_target}-build-flag} ${${_python3_target}-requirements-flag}
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
SET(RV_DEPS_PYSIDE2_VERSION
    ${_pyside2_version}
    CACHE INTERNAL "" FORCE
)

SET(RV_DEPS_PYTHON3_EXECUTABLE
    ${_python3_executable}
    CACHE INTERNAL "" FORCE
)
