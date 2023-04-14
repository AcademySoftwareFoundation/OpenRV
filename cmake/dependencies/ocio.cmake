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

# 
# Also add OpenExr libraries to _byproduct
#
SET(_iex_name "Iex-3_1")
SET(_ilmthread_name "IlmThread-3_1")
SET(_openexrcore_name "OpenEXR-3_1")
SET(_lib_type STATIC)
SET(RV_DEPS_OPENEXR_ROOT_DIR ${RV_DEPS_OCIO_DIST_DIR})
IF(RV_TARGET_WINDOWS)
  MESSAGE(FATAL_ERROR "TODO: define name")
  SET(_iex_implib "")
  SET(_openexr_implib "")
  SET(_ilmthread_implib "")
ENDIF()
IF(RV_TARGET_LINUX)
  SET(_ociolib_dir "${RV_DEPS_OCIO_DIST_DIR}/lib64")  
ELSE()
  SET(_ociolib_dir "${RV_DEPS_OCIO_DIST_DIR}/lib")
ENDIF()

SET(_ilmthread_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_ilmthread_name}${RV_DEBUG_POSTFIX}.a")
SET( _iex_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_iex_name}${RV_DEBUG_POSTFIX}.a")
SET(_openexr_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}${_openexrcore_name}${RV_DEBUG_POSTFIX}.a")
LIST(APPEND _byproducts "${_ilmthread_lib}")
LIST(APPEND _byproducts "${_iex_lib}")
LIST(APPEND _byproducts "${_openexr_lib}")

#
# Now, also add yaml-cpp as by-product
SET(_yaml_cpp_lib "${_ociolib_dir}/${CMAKE_${_lib_type}_LIBRARY_PREFIX}yaml-cpp${_ocio_debug_postfix}.a")
LIST(APPEND _byproducts "${_yaml_cpp_lib}")

#
# and finally the PyOpenColorIO library
SET(_pyocio_lib_dir "${_lib_dir}/python${RV_DEPS_PYTHON_VERSION_SHORT}/site-packages")
SET(_pyocio_libname PyOpenColorIO.so)
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
LIST(APPEND _configure_options "-DPython_EXECUTABLE=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${RV_DEPS_PYTHON_VERSION_SHORT}")
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

#MESSAGE(FATAL_ERROR "RV_DEPS_IMATH_ROOT_DIR='${RV_DEPS_IMATH_ROOT_DIR}'")
#GET_TARGET_PROPERTY(_imath_library Imath::Imath IMPORTED_LOCATION)
#GET_TARGET_PROPERTY(_imath_include_dir Imath::Imath INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DImath_ROOT=${RV_DEPS_IMATH_ROOT_DIR}")
#LIST(APPEND _configure_options "-DImath_DIR=${RV_DEPS_IMATH_ROOT_DIR}")
#LIST(APPEND _configure_options "-DIMATH_LIBRARY=${_imath_library}")
#LIST(APPEND _configure_options "-DIMATH_INCLUDE_DIR=${_imath_include_dir}")

MESSAGE(WARNING "OCIO: We would like to compile against own OIIO, but OIIO uses OCIO's own OpenEXR")
IF(FALSE)
  GET_TARGET_PROPERTY(_oiio_library oiio::oiio IMPORTED_LOCATION)
  GET_TARGET_PROPERTY(_oiio_include_dir oiio::oiio INTERFACE_INCLUDE_DIRECTORIES)
  SET(_depends_oiio "oiio::oiio")
ELSE()
  # The 'RV_DEPS_OIIO_ROOT_DIR' doesn't exist yet
  SET(_oiio_base_dir ${RV_DEPS_BASE_DIR}/RV_DEPS_OIIO)
  SET(_oiio_install_dir ${_oiio_base_dir}/install)
  SET(_oiio_library "${_oiio_install_dir}/lib/libOpenImageIO.2.4.6.dylib")
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

MESSAGE(WARNING "TODO: ${_target}: figure out how to run tests???")
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

ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Copying PyOpenColorIO lib into '${_pyocio_dest_dir}'."
    COMMAND ${CMAKE_COMMAND} -E copy ${_pyocio_lib} ${_pyocio_dest_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
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

# It is required to force directory creation at configure time
# otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY ${_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  ocio::ocio
  INTERFACE ${_include_dir}
)
