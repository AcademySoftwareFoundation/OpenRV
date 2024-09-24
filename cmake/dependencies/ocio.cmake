#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Build instructions: https://opencolorio.readthedocs.io/en/latest/quick_start/installation.html#building-from-source
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_VFX_SET_VARIABLE(
  _ext_dep_version
  CY2023 "2.2.1"
  CY2024 "2.3.2"
)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OCIO" "${_ext_dep_version}" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

# The folder OCIO is building its own dependencies
SET(RV_DEPS_OCIO_DIST_DIR
    ${_build_dir}/ext/dist
)

IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
  # Here the postfix is "d" and not "_d": the postfix inside OCIO is: "d".
  SET(_ocio_debug_postfix
      "d"
  )
  MESSAGE(DEBUG "Using debug postfix: '${_ocio_debug_postfix}'")
ELSE()
  SET(_ocio_debug_postfix
      ""
  )
ENDIF()

RV_VFX_SET_VARIABLE(
  _download_hash
  CY2023 "372d6982cf01818a21a12f9628701a91"
  CY2024 "8af74fcb8c4820ab21204463a06ba490"
)

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/OpenColorIO/archive/refs/tags/v${_version}.tar.gz"
)

# Another project that isn't adding a debug postfix
RV_MAKE_STANDARD_LIB_NAME("OpenColorIO" "${_version}" "SHARED" "")

IF(RV_TARGET_WINDOWS)
  SET(_byproducts
      ""
  ) # Empty out the List as it has the wrong DLL name: it doesn't have the version suffix

  # OpenColorIO shared library has the same name on Release and Debug.
  RV_VFX_SET_VARIABLE(
    _ocio_win_sharedlibname
    CY2023 "OpenColorIO_2_2.dll"
    CY2024 "OpenColorIO_2_3.dll"
  )

  SET(_ocio_win_sharedlib_path
      ${_bin_dir}/${_ocio_win_sharedlibname}
  )
  LIST(APPEND _byproducts ${_ocio_win_sharedlib_path})
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_ociolib_dir
      "${RV_DEPS_OCIO_DIST_DIR}/lib"
  )
ENDIF()
IF(RHEL_VERBOSE)
  SET(_ociolib_dir
      "${RV_DEPS_OCIO_DIST_DIR}/lib64"
  )
ELSE()
  SET(_ociolib_dir
      "${RV_DEPS_OCIO_DIST_DIR}/lib"
  )
ENDIF()

#
# Now, also add yaml-cpp as by-product
SET(_yaml_cpp_lib
    "${_ociolib_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${_ocio_debug_postfix}${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
LIST(APPEND _byproducts "${_yaml_cpp_lib}")

#
# and finally the PyOpenColorIO library
IF(RV_TARGET_WINDOWS)
  SET(_pyocio_lib_dir
      "${_lib_dir}/site-packages"
  )
  SET(_pyocio_libname
      PyOpenColorIO.pyd
  )
ELSE()
  SET(_pyocio_lib_dir
      "${_lib_dir}/python${RV_DEPS_PYTHON_VERSION_SHORT}/site-packages"
  )
  SET(_pyocio_libname
      PyOpenColorIO.so
  )
ENDIF()

RV_VFX_SET_VARIABLE(
  _pyocio_lib_dir
  CY2024 "${_pyocio_lib_dir}/PyOpenColorIO"
)

SET(_pyocio_lib
    "${_pyocio_lib_dir}/${_pyocio_libname}"
)
SET(_pyocio_dest_dir
    ${RV_STAGE_LIB_DIR}/python${RV_DEPS_PYTHON_VERSION_SHORT}
)
LIST(APPEND _byproducts "${_pyocio_lib}")

#
# Assemble CMake configure options
#
# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
LIST(APPEND _configure_options "-DOCIO_BUILD_TESTS=ON")
LIST(APPEND _configure_options "-DOCIO_BUILD_GPU_TESTS=OFF")
LIST(APPEND _configure_options "-DOCIO_BUILD_PYTHON=ON") # This build PyOpenColorIO

# SIMD CPU performance optimizations
# OCIO 2.3.X can utilize SSE/AVX (Intel) and ARM NEON (Apple M chips) SIMD instructions.
RV_VFX_SET_VARIABLE(
  _ocio_simd_options_str
  CY2023 "-DOCIO_USE_SSE=ON"
  CY2024 "-DOCIO_USE_SIMD=ON"
)
LIST(APPEND _configure_options "${_ocio_simd_options_str}")

# Ref.: https://cmake.org/cmake/help/latest/module/FindPython.html#hints
LIST(APPEND _configure_options "-DPython_ROOT_DIR=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install")
IF(NOT RV_TARGET_WINDOWS)
  SET(OCIO_PYTHON_PATH
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${RV_DEPS_PYTHON_VERSION_SHORT}
  )
  LIST(APPEND _configure_options "-DPython_EXECUTABLE=${OCIO_PYTHON_PATH}")
ENDIF()
LIST(APPEND _configure_options "-DOCIO_PYTHON_VERSION=${RV_DEPS_PYTHON_VERSION_SHORT}")

# Using Imath_ROOT because Imath_DIR does not seems to be enough on UNIX-based platform (at least Rocky linux).
LIST(APPEND _configure_options "-DImath_ROOT=${RV_DEPS_IMATH_ROOT_DIR}")

LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")

# OCIO apps are not needed.
LIST(APPEND _configure_options "-DOCIO_BUILD_APPS=OFF")

IF(NOT RV_TARGET_WINDOWS)
  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.zip
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    BINARY_DIR ${_build_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS Boost::headers RV_DEPS_PYTHON3 Imath::Imath ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${_cmake_build_command}
    INSTALL_COMMAND ${_cmake_install_command}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ELSE() # Windows
  SET(_pyopencolorio_patch_script_path
      ${PROJECT_SOURCE_DIR}/src/build/patch_OCIO/ocio_pyopencolorio_patch.py
  )
  SET(_pyopencolorio_cmakelists_path
      ${_source_dir}/src/bindings/python/CMakeLists.txt
  )
  SET(_backup_pyopencolorio_cmakelists_path
      ${_pyopencolorio_cmakelists_path}.bk
  )
  IF(EXISTS ${_backup_pyopencolorio_cmakelists_path})
    FILE(REMOVE ${_backup_pyopencolorio_cmakelists_path})
  ENDIF()

  # Some options are not multi-platform so we start clean.
  SET(_configure_options
      ""
  )
  STRING(REPLACE "." "" PYTHON_VERSION_SHORT_NO_DOT ${RV_DEPS_PYTHON_VERSION_SHORT})
  
  # Windows only.
  # Because of an issue in Debug with minizip-ng finding ZLIB at two locations,
  # ZLIB_LIBRARY and ZLIB_INCLUDE_DIR is used for both Release and Debug. 
  # ZLIB_ROOT is not enough to fix the issue.
  GET_TARGET_PROPERTY(_zlib_library ZLIB::ZLIB IMPORTED_IMPLIB)
  GET_TARGET_PROPERTY(_zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)

  LIST(
    APPEND
    _configure_options
    # Not using Ninja: Ninja doesn't build due to Minizip wrong include style, VS uses search paths even for double-quotes includes.
    "-G ${CMAKE_GENERATOR}"
    "-DOCIO_VERBOSE=ON"
    "-DCMAKE_INSTALL_PREFIX=${_install_dir}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DZLIB_LIBRARY=${_zlib_library}"
    "-DZLIB_INCLUDE_DIR=${_zlib_include_dir}"
    "-Dexpat_ROOT=${RV_DEPS_EXPAT_ROOT_DIR}"
    "-DImath_DIR=${RV_DEPS_IMATH_ROOT_DIR}/lib/cmake/Imath"
    "-DPython_ROOT=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install"
    # Mandatory param: OCIO CMake code finds Python.
    "-DPython_LIBRARY=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${PYTHON_VERSION_SHORT_NO_DOT}.lib"                                                                                                # with this param
    # DRV_Python_LIBRARIES: 
    # A Patch RV created for PyOpenColorIO inside OCIO: Hardcode to Release since FindPython.cmake will find the Debug lib, 
    # which we don't want and doesn't build.
    "-DRV_Python_LIBRARIES=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${PYTHON_VERSION_SHORT_NO_DOT}.lib"
    "-DPython_INCLUDE_DIR=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/include"
    "-DOCIO_PYTHON_VERSION=${RV_DEPS_PYTHON_VERSION_SHORT}"
    "-DBUILD_SHARED_LIBS=ON"
    "-DOCIO_BUILD_PYTHON=ON"
    "-DOCIO_INSTALL_EXT_PACKAGES=MISSING"
    "-DOCIO_BUILD_TESTS=OFF"
    "-DOCIO_BUILD_GPU_TESTS=OFF"
    "-DOCIO_BUILD_DOCS=OFF"
    # OCIO apps are not needed.
    "-DOCIO_BUILD_APPS=OFF"
    "-DOCIO_WARNING_AS_ERROR=OFF"
    "-DOCIO_BUILD_JAVA=OFF"
    "${_ocio_simd_options_str}"
    "-S ${_source_dir}"
    "-B ${_build_dir}"
  )

  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    # Use debug Python executable.
    LIST(APPEND _configure_options "-DPython_EXECUTABLE=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python_d.exe")
  ELSE()
    LIST(APPEND _configure_options "-DPython_EXECUTABLE=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python.exe")
  ENDIF()

  LIST(APPEND _ocio_build_options "--build" "${_build_dir}" "--config" "${CMAKE_BUILD_TYPE}"
       # "--parallel"    # parallel breaks minizip because Zlib is built before minizip and minizip depends on Zlib. "${_cpu_count}"   # Moreover, our Zlib
       # isn't compatible with OCIO: tons of STD C++ missing symbols errors.
  )

  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.zip
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    BINARY_DIR ${_build_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS Boost::headers RV_DEPS_PYTHON3 Imath::Imath ZLIB::ZLIB EXPAT::EXPAT
    PATCH_COMMAND 
      python3 ${_pyopencolorio_patch_script_path} ${_pyopencolorio_cmakelists_path}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${CMAKE_COMMAND} ${_ocio_build_options}
    INSTALL_COMMAND ${_cmake_install_command}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ENDIF()

SET(_ocio_stage_plugins_python_dir "${RV_STAGE_PLUGINS_PYTHON_DIR}")
IF(RV_VFX_CY2024)
  SET(_ocio_stage_plugins_python_dir "${_ocio_stage_plugins_python_dir}/PyOpenColorIO")
ENDIF()

IF(RV_VFX_CY2023)
  # All platform
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Copying PyOpenColorIO lib into '${_ocio_stage_plugins_python_dir}'."
    COMMAND ${CMAKE_COMMAND} -E copy ${_pyocio_lib} ${_ocio_stage_plugins_python_dir}
  )
ELSEIF(RV_VFX_CY2024)
  # All platform
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Copying PyOpenColorIO directory into '${_ocio_stage_plugins_python_dir}'."
    # Copy PyOpenColorIO directory to the stage python plugins directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_pyocio_lib_dir} ${_ocio_stage_plugins_python_dir}
  )
ENDIF()

# All platform
ADD_CUSTOM_COMMAND(
  TARGET ${_target}
  POST_BUILD
  COMMENT "Copying OpenColorIO lib into '${RV_STAGE_LIB_DIR}'."
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
)

IF(RV_TARGET_WINDOWS)
  SET(_rv_stage_lib_site_package_dir "${RV_STAGE_LIB_DIR}/site-packages")
  IF(RV_VFX_CY2024)
    SET(_rv_stage_lib_site_package_dir "${_rv_stage_lib_site_package_dir}/PyOpenColorIO")
  ENDIF()

  # Windows only.
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    # Copy OCIO shared library to the stage python plugins directory.
    COMMAND ${CMAKE_COMMAND} -E copy ${_ocio_win_sharedlib_path} ${_ocio_stage_plugins_python_dir}
  ) 

  # Windows only.
  # Debug Python on Windows search for modules with *_d suffix, but OCIO does not create a PyOpenColor_d.pyd.
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Rename PyOpenColorIO.py to PyOpenColorIO_d.py in '${_rv_stage_lib_site_package_dir}' and '${_ocio_stage_plugins_python_dir}."
      COMMAND ${CMAKE_COMMAND} -E copy ${_rv_stage_lib_site_package_dir}/PyOpenColorIO.pyd ${_rv_stage_lib_site_package_dir}/PyOpenColorIO_d.pyd
      COMMAND ${CMAKE_COMMAND} -E copy ${_pyocio_lib} ${_ocio_stage_plugins_python_dir}/PyOpenColorIO_d.pyd
    )
  ENDIF()
ENDIF()

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_LIBRARY(ocio::ocio SHARED IMPORTED GLOBAL)
LIST(APPEND RV_DEPS_LIST ocio::ocio)
ADD_DEPENDENCIES(ocio::ocio ${_target})
SET_PROPERTY(
  TARGET ocio::ocio
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET ocio::ocio
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

# It is required to force directory creation at configure time otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  ocio::ocio
  INTERFACE ${_include_dir}
)
