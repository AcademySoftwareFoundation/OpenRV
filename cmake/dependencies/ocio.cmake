#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Build instructions:
#   https://opencolorio.readthedocs.io/en/latest/quick_start/installation.html#building-from-source
#
INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES( "RV_DEPS_OCIO" "2.2.1" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

# The folder OCIO is building it's own dependencies
SET(RV_DEPS_OCIO_DIST_DIR ${_build_dir}/ext/dist)

IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    # Here the postfix is "d" and not "_d": the postfix inside OCIO is: "d".
    SET(_ocio_debug_postfix "d")
    MESSAGE(DEBUG "Using debug postfix: '${_ocio_debug_postfix}'")
ELSE()
    SET(_ocio_debug_postfix "")
ENDIF()

SET(_download_hash "372d6982cf01818a21a12f9628701a91")

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/OpenColorIO/archive/refs/tags/v${_version}.tar.gz"
)

# Another project that isn't adding a debug postfix
RV_MAKE_STANDARD_LIB_NAME("OpenColorIO" "${_version}" "SHARED" "")

IF(RV_TARGET_WINDOWS)
  SET(_byproducts "")   # Empty out the List as it has the wrong DLL name: it doesn't have the version suffix
  SET(_ocio_win_sharedlibname OpenColorIO_2_2.dll)
  SET(_ocio_win_sharedlib_path ${_bin_dir}/${_ocio_win_sharedlibname})
  LIST(APPEND _byproducts ${_ocio_win_sharedlib_path})
ENDIF()
# 
# Also add OpenExr libraries to _byproduct
#
SET(_iex_name "Iex-3_1")
SET(_ilmthread_name "IlmThread-3_1")
SET(_openexrcore_name "OpenEXR-3_1")
SET(_lib_type STATIC)
SET(RV_DEPS_OPENEXR_ROOT_DIR ${RV_DEPS_OCIO_DIST_DIR})
IF(RV_TARGET_WINDOWS)
  SET(_ociolib_dir "${RV_DEPS_OCIO_DIST_DIR}/lib")
  SET(_iex_implib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_ilmthread_name}${RV_DEBUG_POSTFIX}.lib")
  SET(_openexr_implib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_iex_name}${RV_DEBUG_POSTFIX}.lib")
  SET(_ilmthread_implib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_openexrcore_name}${RV_DEBUG_POSTFIX}.lib")
ENDIF()
IF(RV_TARGET_LINUX)
  SET(_ociolib_dir "${RV_DEPS_OCIO_DIST_DIR}/lib64")
ELSE()
  SET(_ociolib_dir "${RV_DEPS_OCIO_DIST_DIR}/lib")
ENDIF()

SET(_ilmthread_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_ilmthread_name}${RV_DEBUG_POSTFIX}${CMAKE_${_lib_type}_LIBRARY_SUFFIX}")
SET( _iex_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_iex_name}${RV_DEBUG_POSTFIX}${CMAKE_${_lib_type}_LIBRARY_SUFFIX}")
SET(_openexr_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_openexrcore_name}${RV_DEBUG_POSTFIX}${CMAKE_${_lib_type}_LIBRARY_SUFFIX}")
LIST(APPEND _byproducts "${_ilmthread_lib}")
LIST(APPEND _byproducts "${_iex_lib}")
LIST(APPEND _byproducts "${_openexr_lib}")

#
# Now, also add yaml-cpp as by-product
SET(_yaml_cpp_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}yaml-cpp${_ocio_debug_postfix}${CMAKE_${_lib_type}_LIBRARY_SUFFIX}")
LIST(APPEND _byproducts "${_yaml_cpp_lib}")

#
# and finally the PyOpenColorIO library
IF(RV_TARGET_WINDOWS)
  SET(_pyocio_lib_dir "${_lib_dir}/site-packages")
  SET(_pyocio_libname PyOpenColorIO.pyd)
ELSE()
  SET(_pyocio_lib_dir "${_lib_dir}/python${RV_DEPS_PYTHON_VERSION_SHORT}/site-packages")
  SET(_pyocio_libname PyOpenColorIO.so)
ENDIF()
SET(_pyocio_lib "${_pyocio_lib_dir}/${_pyocio_libname}")
SET(_pyocio_dest_dir ${RV_STAGE_LIB_DIR}/python${RV_DEPS_PYTHON_VERSION_SHORT})
LIST(APPEND _byproducts "${_pyocio_lib}")

#
# Assemble CMake configure options
#
# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
LIST(APPEND _configure_options "-DOCIO_BUILD_TESTS=ON")
LIST(APPEND _configure_options "-DOCIO_BUILD_GPU_TESTS=OFF")
LIST(APPEND _configure_options "-DOCIO_BUILD_PYTHON=ON")  # This build PyOpenColorIO
LIST(APPEND _configure_options "-DOCIO_USE_SSE=ON")  # SSE CPU performance optimizations

LIST(APPEND _configure_options "-DBOOST_ROOT=${RV_DEPS_BOOST_ROOT_DIR}")

# Ref.: https://cmake.org/cmake/help/latest/module/FindPython.html#hints
LIST(APPEND _configure_options "-DPython_ROOT_DIR=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install")
IF(RV_TARGET_WINDOWS)
  # Python path on Windows: python3.9.exe doesn't exist. Moreover, the executable needs the EXE extension otherwise find_package fails
  SET(OCIO_PYTHON_PATH ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${RV_DEPS_PYTHON_VERSION_MAJOR}.exe)
  LIST(APPEND _configure_options "-DPython_EXECUTABLE=${OCIO_PYTHON_PATH}")
ELSE()
  SET(OCIO_PYTHON_PATH ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${RV_DEPS_PYTHON_VERSION_SHORT})
  LIST(APPEND _configure_options "-DPython_EXECUTABLE=${OCIO_PYTHON_PATH}")
ENDIF()
LIST(APPEND _configure_options "-DOCIO_PYTHON_VERSION=${RV_DEPS_PYTHON_VERSION_SHORT}")

IF(NOT RV_USE_OCIO_OPENEXR)
  #
  # Why isn't OCIO finding our own copy of OpenEXR?
  #
  #GET_TARGET_PROPERTY(_openexr_library OpenEXR::OpenEXR IMPORTED_LOCATION)
  #GET_TARGET_PROPERTY(_ilmthread_library OpenEXR::IlmThread IMPORTED_LOCATION)
  #GET_TARGET_PROPERTY(_iex_library OpenEXR::Iex IMPORTED_LOCATION)
  #GET_TARGET_PROPERTY(_openexr_include_dir OpenEXR::OpenEXR INTERFACE_INCLUDE_DIRECTORIES)
  #SET(_openexr_libraries "")
  #LIST(APPEND _openexr_libraries ${_openexr_library})
  #LIST(APPEND _openexr_libraries ${_ilmthread_library})
  #LIST(APPEND _openexr_libraries ${_iex_library})
  #MESSAGE(DEBUG "_openexr_libraries='${_openexr_library}'")
  #MESSAGE(DEBUG "_openexr_include_dir='${_openexr_include_dir}'")
  LIST(APPEND _configure_options "-DOpenEXR_VERSION=${RV_DEPS_OPENEXR_VERSION}")
  LIST(APPEND _configure_options "-DOpenEXR_LIBRARY=${_openexr_library}")
  LIST(APPEND _configure_options "-DOpenEXR_INCLUDE_DIR=${_openexr_include_dir}")
  #LIST(APPEND _configure_options "-DOpenEXR_ROOT=${RV_DEPS_OPENEXR_ROOT_DIR}")
ENDIF()

GET_TARGET_PROPERTY(_imath_library Imath::Imath IMPORTED_LOCATION)
GET_TARGET_PROPERTY(_imath_include_dir Imath::Imath INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DImath_LIBRARY=${_imath_library}")
LIST(APPEND _configure_options "-DImath_INCLUDE_DIR=${_imath_include_dir}/..")
LIST(APPEND _configure_options "-DImath_ROOT=${RV_DEPS_IMATH_ROOT_DIR}")

MESSAGE(WARNING "OCIO: We would like to compile against own OIIO, but OIIO uses OCIO's own OpenEXR")
IF(FALSE)
  GET_TARGET_PROPERTY(_oiio_library oiio::oiio IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_oiio_include_dir oiio::oiio INTERFACE_INCLUDE_DIRECTORIES)
  SET(_depends_oiio "oiio::oiio")
ELSE()
  # The 'RV_DEPS_OIIO_ROOT_DIR' doesn't exist yet
  SET(_oiio_base_dir ${RV_DEPS_BASE_DIR}/RV_DEPS_OIIO)
  SET(_oiio_install_dir ${_oiio_base_dir}/install)
  # TODO: _oiio_library: What to do for Windows and Linux?
  IF(RV_TARGET_DARWIN)
    SET(_oiio_library "${_oiio_install_dir}/lib/libOpenImageIO.2.4.6.dylib")
  ENDIF()
  SET(_oiio_include_dir "${_oiio_install_dir}/include")
  SET(_depends_oiio "")
ENDIF()
MESSAGE(DEBUG "OCIO: _oiio_library='${_oiio_library}'")
MESSAGE(DEBUG "OCIO: _oiio_include_dir='${_oiio_include_dir}'")
LIST(APPEND _configure_options "-DOPENIMAGEIO_LIBRARY=${_oiio_library}")
LIST(APPEND _configure_options "-DOPENIMAGEIO_INCLUDE_DIR=${_oiio_include_dir}")
LIST(APPEND _configure_options "-DOPENIMAGEIO_VERSION=${RV_DEPS_OIIO_VERSION}")
LIST(APPEND _configure_options "-DOPENIMAGEIO_ROOT_DIR=${_oiio_install_dir}")

LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")

LIST(APPEND _configure_options "-DOCIO_BUILD_APPS=OFF")

MESSAGE(WARNING "TODO: ${_target}: figure out how to run tests???")

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
    DEPENDS ${_depends_oiio} Boost::headers Python::Python Imath::Imath ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${_make_command} -j${_cpu_count} -v
    INSTALL_COMMAND ${_make_command} install
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ELSE()
  GET_TARGET_PROPERTY(_vcpkg_location VCPKG::VCPKG IMPORTED_LOCATION)
  GET_FILENAME_COMPONENT(_vcpkg_path ${_vcpkg_location} DIRECTORY)

  # Some options are not multi-platform so we start clean.
  SET(_configure_options "")
  LIST(APPEND _configure_options
    "-G ${CMAKE_GENERATOR}"                   # Ninja doesn't build due to Minizip include bad style, VS uses search paths even for dbl-quotes includes.
    "-DCMAKE_INSTALL_PREFIX=${_install_dir}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DGLEW_ROOT=${_vcpkg_path}/packages/glew_x64-windows"   # Still unknown if GLEW_ROOT, GLUT_ROOT and Oiio_ROOT is needed but at least: *they are used by OCIO in CMake*
    "-DGLUT_ROOT=${_vcpkg_path}/packages/freeglut_x64-windows"   # However, not passing them doesn't seem to cause the build to fail.
    "-DOpenImageIO_ROOT=${_oiio_install_dir}"         # And the Dev (cfuoco) said that we need to pass them.
    "-DOCIO_BUILD_PYTHON=ON"
    "-DOCIO_INSTALL_EXT_PACKAGES=ALL"
    "-DPython_ROOT=${OCIO_PYTHON_PATH}"
    "-DBUILD_SHARED_LIBS=ON"
    "-DOCIO_BUILD_APPS=ON"
    "-DOCIO_BUILD_TESTS=OFF"
    "-DOCIO_BUILD_GPU_TESTS=OFF"
    "-DOCIO_BUILD_DOCS=OFF"
    "-DOCIO_USE_SSE=ON"
    "-DOCIO_WARNING_AS_ERROR=OFF"
    "-DOCIO_BUILD_JAVA=OFF"
    "-S ${_source_dir}"
    "-B ${_build_dir}"
  )

  # TODO_IBR: Both Build & Install & possibly Configure do not Honor CMAKE_BUILD_TYPE
  LIST(APPEND _ocio_build_options
    "--build"
    "${_build_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}"
    # "--parallel"    # parallel breaks minizip because Zlib is built before minizip and minizip depends on Zlib.
    # "${_cpu_count}"   # Moreover, our Zlib isn't compatible with OCIO: tons of STD C++ missing symbols errors.
  )
  LIST(APPEND _ocio_install_options
    "--install"
    "${_build_dir}"
    "--prefix"
    "${_install_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}"   # for --config
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
    DEPENDS ${_depends_oiio} Boost::headers Python::Python Imath::Imath VCPKG::VCPKG ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
    BUILD_COMMAND ${CMAKE_COMMAND} ${_ocio_build_options}
    INSTALL_COMMAND ${CMAKE_COMMAND} ${_ocio_install_options}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )

  SET(_vcpkg_manifest "${RV_PKGMANCONFIG_DIR}/ocio_vcpkg.json")
  EXTERNALPROJECT_ADD_STEP(
    ${_target} add_vcpkg_manifest
    COMMENT "Copying the VCPKG manifest"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_vcpkg_manifest} "${_source_dir}/vcpkg.json"
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

  # TODO_IBR: Add a step to copy build/src/bindings here but exclure the .pyd file (total 3 files, 1 excluded.)
  # TODO_IBR: Then make sure to review all _byproducts and ensure they are on the EXTERNALPROJECT_ADD call.

ENDIF()

IF(RV_TARGET_DARWIN OR
    RV_TARGET_LINUX)
  ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
    COMMENT "Copying PyOpenColorIO lib into '${RV_STAGE_PLUGINS_PYTHON_DIR}'."
    COMMAND ${CMAKE_COMMAND} -E copy ${_pyocio_lib} ${RV_STAGE_PLUGINS_PYTHON_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
)
ELSE()
  ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
    COMMENT "Copying PyOpenColorIO lib into '${RV_STAGE_PLUGINS_PYTHON_DIR}'."
    COMMAND ${CMAKE_COMMAND} -E copy ${_pyocio_lib} ${RV_STAGE_PLUGINS_PYTHON_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy ${_ocio_win_sharedlib_path} ${RV_STAGE_PLUGINS_PYTHON_DIR}
  )

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

# It is required to force directory creation at configure time
# otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  ocio::ocio
  INTERFACE ${_include_dir}
)
