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

RV_VFX_SET_VARIABLE(_pyside_target CY2023 "RV_DEPS_PYSIDE2" CY2024 "RV_DEPS_PYSIDE6")

SET(PYTHON_VERSION_MAJOR
    3
)

RV_VFX_SET_VARIABLE(PYTHON_VERSION_MINOR CY2023 "10" CY2024 "11")

RV_VFX_SET_VARIABLE(PYTHON_VERSION_PATCH CY2023 "13" CY2024 "9")

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

RV_VFX_SET_VARIABLE(_pyside_version CY2023 "5.15.10" CY2024 "6.5.3")

SET(_python3_download_url
    "https://github.com/python/cpython/archive/refs/tags/v${_python3_version}.zip"
)
RV_VFX_SET_VARIABLE(_python3_download_hash CY2023 "21b32503f31386b37f0c42172dfe5637" CY2024 "392eccd4386936ffcc46ed08057db3e7")

SET(_opentimelineio_download_url
    "https://github.com/AcademySoftwareFoundation/OpenTimelineIO"
)
SET(_opentimelineio_git_tag
    "v${_opentimelineio_version}"
)

RV_VFX_SET_VARIABLE(
  _pyside_archive_url
  CY2023
  "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside2/PySide2-${_pyside_version}-src/pyside-setup-opensource-src-${_pyside_version}.zip"
  CY2024
  "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside6/PySide6-${_pyside_version}-src/pyside-setup-everywhere-src-${_pyside_version}.zip"
)

RV_VFX_SET_VARIABLE(_pyside_download_hash CY2023 "87841aaced763b6b52e9b549e31a493f" CY2024 "515d3249c6e743219ff0d7dd25b8c8d8")

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
RV_VFX_SET_VARIABLE(_vfx_platform_ CY2023 "2023" CY2024 "2024")
LIST(APPEND _python3_make_command ${_vfx_platform_})

IF(DEFINED RV_DEPS_OPENSSL_INSTALL_DIR)
  LIST(APPEND _python3_make_command "--openssl-dir")
  LIST(APPEND _python3_make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
ENDIF()
IF(RV_TARGET_WINDOWS)
  LIST(APPEND _python3_make_command "--opentimelineio-source-dir")
  LIST(APPEND _python3_make_command ${rv_deps_opentimelineio_SOURCE_DIR})
  LIST(APPEND _python3_make_command "--python-version")
  LIST(APPEND _python3_make_command "${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
ENDIF()

# TODO_QT: Maybe we could use something like NOT CY2023 since after 2023, it is Qt6 TODO_QT: Below code could be simplified, but for now it is faster to test.
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
  LIST(APPEND _pyside_make_command ${RV_DEPS_QT5_LOCATION})
  LIST(APPEND _pyside_make_command "--python-version")
  LIST(APPEND _pyside_make_command "${RV_DEPS_PYTHON_VERSION_SHORT}")
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2024)
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
  LIST(APPEND _pyside_make_command ${RV_DEPS_QT6_LOCATION})
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
  COMMAND cmake -E touch ${${_python3_target}-requirements-flag}
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
    COMMAND cmake -E copy_if_different ${_python3_implib} ${_python_release_libpath}
    COMMAND cmake -E copy_if_different ${_python3_implib} ${_python_release_in_bin_libpath} DEPENDS ${_python3_target} ${_requirements_file}
  )
ENDIF()

SET(${_pyside_target}-build-flag
    ${_install_dir}/${_pyside_target}-build-flag
)

# TODO_QT: Maybe we could use something like NOT CY2023 since after 2023, it is Qt6 TODO_QT: Below code could be simplified, but for now it is faster to test.
IF(RV_VFX_PLATFORM STREQUAL CY2023)
  ADD_CUSTOM_COMMAND(
    COMMENT "Building PySide2 using ${_pyside_make_command_script}"
    OUTPUT ${${_pyside_target}-build-flag}
    # First PySide build script on Windows which doesn't respect '--debug' option
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/build/patch_PySide2/windows_desktop.py
            ${rv_deps_pyside2_SOURCE_DIR}/build_scripts/platforms/windows_desktop.py
    COMMAND ${_pyside_make_command} --prepare --build
    COMMAND cmake -E touch ${${_pyside_target}-build-flag}
    DEPENDS ${_python3_target} ${_pyside_make_command_script} ${${_python3_target}-requirements-flag}
    USES_TERMINAL
  )

  SET(_build_flag_depends
      ${${_pyside_target}-build-flag}
  )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2024)
  ADD_CUSTOM_COMMAND(
    COMMENT "Building PySide6 using ${_pyside_make_command_script}"
    OUTPUT ${${_pyside_target}-build-flag}
    COMMAND ${_pyside_make_command} --prepare --build
    COMMAND cmake -E touch ${${_pyside_target}-build-flag}
    DEPENDS ${_python3_target} ${_pyside_make_command_script} ${${_python3_target}-requirements-flag}
    USES_TERMINAL
  )

  SET(_build_flag_depends
      ${${_pyside_target}-build-flag}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_copy_commands
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/include ${RV_STAGE_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
  )

  IF(RV_VFX_CY2024)
    LIST(APPEND _copy_commands COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/DLLs ${RV_STAGE_ROOT_DIR}/DLLs)
  ENDIF()

  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_python3_target}'s include and libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_BIN_DIR}/${_python3_lib_name}
    ${_copy_commands}
    DEPENDS ${_python3_target} ${${_python3_target}-requirements-flag} ${_build_flag_depends}
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
    DEPENDS ${_python3_target} ${${_python3_target}-requirements-flag} ${_build_flag_depends}
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
