#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Build instructions: https://opencolorio.readthedocs.io/en/latest/quick_start/installation.html#building-from-source
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OCIO" "2.2.1" "make" "")
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

SET(_download_hash
    "372d6982cf01818a21a12f9628701a91"
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
  SET(_ocio_win_sharedlibname
      OpenColorIO_2_2.dll
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
IF(RV_TARGET_LINUX)
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
LIST(APPEND _configure_options "-DOCIO_USE_SSE=ON") # SSE CPU performance optimizations

LIST(APPEND _configure_options "-DBOOST_ROOT=${RV_DEPS_BOOST_ROOT_DIR}")

# Ref.: https://cmake.org/cmake/help/latest/module/FindPython.html#hints
LIST(APPEND _configure_options "-DPython_ROOT_DIR=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install")
IF(NOT RV_TARGET_WINDOWS)
  SET(OCIO_PYTHON_PATH
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${RV_DEPS_PYTHON_VERSION_SHORT}
  )
  LIST(APPEND _configure_options "-DPython_EXECUTABLE=${OCIO_PYTHON_PATH}")
ENDIF()
LIST(APPEND _configure_options "-DOCIO_PYTHON_VERSION=${RV_DEPS_PYTHON_VERSION_SHORT}")

GET_TARGET_PROPERTY(_openexr_library OpenEXR::OpenEXR IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_ilmthread_library OpenEXR::IlmThread IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_openexr_include_dir OpenEXR::OpenEXR INTERFACE_INCLUDE_DIRECTORIES)

LIST(APPEND _configure_options "-DOpenEXR_VERSION=${RV_DEPS_OPENEXR_VERSION}")
LIST(APPEND _configure_options "-DOpenEXR_LIBRARY=${_openexr_library}")
LIST(APPEND _configure_options "-DOpenEXR_INCLUDE_DIR=${_openexr_include_dir}")
LIST(APPEND _configure_options "-DOpenEXR_ROOT=${RV_DEPS_OPENEXR_ROOT_DIR}")

GET_TARGET_PROPERTY(_imath_library Imath::Imath IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_imath_include_dir Imath::Imath INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DImath_LIBRARY=${_imath_library}")
LIST(APPEND _configure_options "-DImath_INCLUDE_DIR=${_imath_include_dir}/..")
LIST(APPEND _configure_options "-DImath_ROOT=${RV_DEPS_IMATH_ROOT_DIR}")

# OIIO target variables aren't defined yet as OCIO is included before OIIO Moreover, normally giving OCIO the OIIO lib won't work since we build OCIO before
# OIIO. OCIO is set to build missing dependencies so OCIO will use it's own OIIO lib.
SET(_oiio_install_dir
    ${RV_DEPS_BASE_DIR}/RV_DEPS_OIIO/install
)
IF(RV_TARGET_DARWIN)
  SET(_oiio_library
      "${_oiio_install_dir}/lib/libOpenImageIO.2.4.6.dylib"
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_oiio_library
      "${_oiio_install_dir}/lib64/libOpenImageIO.2.4.6.so"
  )
ELSE()
  SET(_oiio_library
      "${_oiio_install_dir}/lib/libOpenImageIO.2.4.6.dll"
  )
ENDIF()
SET(_oiio_include_dir
    "${_oiio_install_dir}/include"
)
SET(_depends_oiio
    ""
)

MESSAGE(DEBUG "OCIO: _oiio_library='${_oiio_library}'")
MESSAGE(DEBUG "OCIO: _oiio_include_dir='${_oiio_include_dir}'")
LIST(APPEND _configure_options "-DOPENIMAGEIO_LIBRARY=${_oiio_library}")
LIST(APPEND _configure_options "-DOPENIMAGEIO_INCLUDE_DIR=${_oiio_include_dir}")
LIST(APPEND _configure_options "-DOPENIMAGEIO_VERSION=${RV_DEPS_OIIO_VERSION}")
LIST(APPEND _configure_options "-DOPENIMAGEIO_ROOT_DIR=${_oiio_install_dir}")

LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")

# OCIO CMake Module FindOpenEXR.cmake file has issues with set_target_properties hence we set APPS OFF. If you need Apps for a Test: hardcode
# _OpenEXR_TARGET_CREATE to FALSE in FindOpenEXR.cmake in OCIO
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
    DEPENDS ${_depends_oiio} Boost::headers RV_DEPS_PYTHON3 Imath::Imath ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${_make_command} -j${_cpu_count}
    INSTALL_COMMAND ${_make_command} install
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ELSE()
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

  GET_TARGET_PROPERTY(_vcpkg_location VCPKG::VCPKG IMPORTED_LOCATION)
  GET_FILENAME_COMPONENT(_vcpkg_path ${_vcpkg_location} DIRECTORY)

  # Some options are not multi-platform so we start clean.
  SET(_configure_options
      ""
  )
  STRING(REPLACE "." "" PYTHON_VERSION_SHORT_NO_DOT ${RV_DEPS_PYTHON_VERSION_SHORT})
  LIST(
    APPEND
    _configure_options
    "-G ${CMAKE_GENERATOR}" # Not using Ninja: Ninja doesn't build due to Minizip wrong include style, VS uses search paths even for double-quotes includes.
    "-DCMAKE_INSTALL_PREFIX=${_install_dir}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DGLEW_ROOT=${_vcpkg_path}/packages/glew_x64-windows" # Unknown if GLEW_ROOT, GLUT_ROOT and Oiio_ROOT is needed but at least: *they are used by OCIO in
                                                           # CMake*
    "-DGLUT_ROOT=${_vcpkg_path}/packages/freeglut_x64-windows" # However, not passing them doesn't seem to cause the build to fail.
    "-DOpenImageIO_ROOT=${_oiio_install_dir}" # And the Dev (cfuoco) said that we need to pass them; they are most likely needed for OCIO APPS
    "-DOCIO_BUILD_PYTHON=ON"
    "-DOCIO_INSTALL_EXT_PACKAGES=ALL" # We should use the Default which is "MISSING" when OCIO removes OpenExr and find how to pass our own.
    "-DPython_ROOT=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install"
    "-DPython_LIBRARY=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${PYTHON_VERSION_SHORT_NO_DOT}.lib" # Mandatory param: OCIO CMake code finds Python
                                                                                                                # with this param
    # DRV_Python_LIBRARIES: A Patch RV created for PyOpenColorIO inside OCIO: Hardcode to Release since FindPython.cmake will find the Debug lib, which we don't
    # want and doesn't build.
    "-DRV_Python_LIBRARIES=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${PYTHON_VERSION_SHORT_NO_DOT}.lib"
    "-DPython_INCLUDE_DIR=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/include"
    "-DOCIO_PYTHON_VERSION=${RV_DEPS_PYTHON_VERSION_SHORT}"
    "-DBUILD_SHARED_LIBS=ON"
    "-DOCIO_BUILD_TESTS=OFF"
    "-DOCIO_BUILD_GPU_TESTS=OFF"
    "-DOCIO_BUILD_DOCS=OFF"
    # Note for OCIO v2.3.0 (future): OCIO_USE_SSE has been renamed to OCIO_USE_SIMD.
    "-DOCIO_USE_SSE=ON"
    # OCIO apps are not needed.
    "-DOCIO_BUILD_APPS=OFF"
    "-DOCIO_WARNING_AS_ERROR=OFF"
    "-DOCIO_BUILD_JAVA=OFF"
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
  LIST(
    APPEND
    _ocio_install_options
    "--install"
    "${_build_dir}"
    "--prefix"
    "${_install_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}" # for --config
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
    DEPENDS ${_depends_oiio} Boost::headers RV_DEPS_PYTHON3 Imath::Imath VCPKG::VCPKG ZLIB::ZLIB
    PATCH_COMMAND python3 ${_pyopencolorio_patch_script_path} ${_pyopencolorio_cmakelists_path}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${CMAKE_COMMAND} ${_ocio_build_options}
    INSTALL_COMMAND ${CMAKE_COMMAND} ${_ocio_install_options}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )

  SET(_vcpkg_manifest
      "${RV_PKGMANCONFIG_DIR}/ocio_vcpkg.json"
  )
  EXTERNALPROJECT_ADD_STEP(
    ${_target} add_vcpkg_manifest
    COMMENT "Copying the VCPKG manifest"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_vcpkg_manifest}" --destination
            "${_source_dir}/vcpkg.json"
    DEPENDERS configure
  )

  EXTERNALPROJECT_ADD_STEP(
    ${_target} install_vcpkg_manifest
    COMMENT "Installing OCIO dependencies via VCPKG"
    COMMAND ${_vcpkg_location} install --triplet x64-windows
    DEPENDEES add_vcpkg_manifest
    DEPENDERS configure
    WORKING_DIRECTORY "${_source_dir}"
  )
ENDIF()

IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Copying PyOpenColorIO lib into '${RV_STAGE_PLUGINS_PYTHON_DIR}'."
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_pyocio_lib}" --destination
            "${RV_STAGE_PLUGINS_PYTHON_DIR}/${_pyocio_libname}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination
            "${RV_STAGE_LIB_DIR}"
  )
ELSE()
  SET(_rv_stage_lib_site_package_dir
      "${RV_STAGE_LIB_DIR}/site-packages"
  )

  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Copying PyOpenColorIO lib into '${RV_STAGE_PLUGINS_PYTHON_DIR}'."
    # Copy PyOpenColorIO.pyd into RV_STAGE_PLUGINS_PYTHON_DIR as PyOpenColorIO_d.pyd in debug.
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_pyocio_lib}" --destination
            "$<$<CONFIG:Debug>:${RV_STAGE_PLUGINS_PYTHON_DIR}/PyOpenColorIO_d.pyd>$<$<CONFIG:Release>:${RV_STAGE_PLUGINS_PYTHON_DIR}/${_pyocio_libname}>"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_ocio_win_sharedlib_path}" --destination
            "${RV_STAGE_PLUGINS_PYTHON_DIR}/${_ocio_win_sharedlibname}"
  )

  # Debug Python on Windows search for modules with *_d suffix, but OCIO does not create a PyOpenColor_d.pyd.
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Rename PyOpenColorIO.py to PyOpenColorIO_d.py in '${_rv_stage_lib_site_package_dir}'."
      COMMAND ${CMAKE_COMMAND} -E rename ${_rv_stage_lib_site_package_dir}/PyOpenColorIO.pyd ${_rv_stage_lib_site_package_dir}/PyOpenColorIO_d.pyd
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
