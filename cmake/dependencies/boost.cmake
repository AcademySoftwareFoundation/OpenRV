#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

# This should handle fetching or checking then compiling required 3rd party dependencies
SET(_target
    "RV_DEPS_BOOST"
)

# This version of boost resolves Python3 compatibilty issues on Big Sur and Monterey and is compatible with Python 2.7 through Python 3.10
SET(_version
    "1.80.0"
)

SET(_major_minor_version
    "1_80"
)

STRING(REPLACE "." "_" _version_with_underscore ${_version})
SET(_download_url
    "https://archives.boost.io/release/${_version}/source/boost_${_version_with_underscore}.tar.gz"
)

SET(_download_hash
    077f074743ea7b0cb49c6ed43953ae95
)

# Set _base_dir for Clean-<target>
SET(_base_dir
    ${RV_DEPS_BASE_DIR}/${_target}
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(${_target}_ROOT_DIR
    ${_install_dir}
)

SET(_boost_libs
    atomic
    chrono
    date_time
    filesystem
    graph
    iostreams
    locale
    program_options
    random
    regex
    serialization
    system
    thread
    timer
)

SET(_lib_dir
    ${_install_dir}/lib
)

# Note: Boost has a custom lib naming scheme on windows
IF(RV_TARGET_WINDOWS)
  SET(BOOST_SHARED_LIBRARY_PREFIX
      ""
  )
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(BOOST_LIBRARY_SUFFIX
        "-vc143-mt-gd-x64-${_major_minor_version}"
    )
  ELSE()
    SET(BOOST_LIBRARY_SUFFIX
        "-vc143-mt-x64-${_major_minor_version}"
    )
  ENDIF()
  SET(BOOST_SHARED_LIBRARY_SUFFIX
      ${BOOST_LIBRARY_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(BOOST_IMPORT_LIBRARY_PREFIX
      ""
  )
  SET(BOOST_IMPORT_LIBRARY_SUFFIX
      ${BOOST_LIBRARY_SUFFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
ELSE()
  SET(BOOST_SHARED_LIBRARY_PREFIX
      ${CMAKE_SHARED_LIBRARY_PREFIX}
  )
  SET(BOOST_SHARED_LIBRARY_SUFFIX
      ${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

FOREACH(
  _boost_lib
  ${_boost_libs}
)
  SET(_boost_${_boost_lib}_lib_name
      ${BOOST_SHARED_LIBRARY_PREFIX}boost_${_boost_lib}${BOOST_SHARED_LIBRARY_SUFFIX}
  )
  SET(_boost_${_boost_lib}_lib
      ${_lib_dir}/${_boost_${_boost_lib}_lib_name}
  )
  LIST(APPEND _boost_byproducts ${_boost_${_boost_lib}_lib})
  IF(RV_TARGET_WINDOWS)
    SET(_boost_${_boost_lib}_implib
        ${_lib_dir}/${BOOST_IMPORT_LIBRARY_PREFIX}boost_${_boost_lib}${BOOST_IMPORT_LIBRARY_SUFFIX}
    )
    LIST(APPEND _boost_byproducts ${_boost_${_boost_lib}_implib})
  ENDIF()
ENDFOREACH()

SET(_boost_b2_options
    "-s NO_LZMA=1"
)
IF(RV_VERBOSE_INVOCATION)
  SET(_boost_b2_options
      "${_boost_b2_options} -d+2"
  )
ELSE()
  SET(_boost_b2_options
      "${_boost_b2_options} -d+0"
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  SET(_toolset
      "clang"
  )

ELSEIF(RV_TARGET_LINUX)
  SET(_toolset
      "gcc"
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_toolset
      "msvc-14.3"
  )
ELSE()
  MESSAGE(FATAL_ERROR "Unsupported (yet) target for Boost")
ENDIF()

SET(_b2_command
    ./b2
)

IF(RV_TARGET_WINDOWS)
  SET(_bootstrap_command
      ./bootstrap.bat
  )
ELSE()
  SET(_bootstrap_command
      ./bootstrap.sh
  )
ENDIF()

IF(${RV_OSX_EMULATION})
  SET(_darwin_x86_64
      "arch" "${RV_OSX_EMULATION_ARCH}"
  )

  SET(_b2_command
      ${_darwin_x86_64} ${_b2_command}
  )
  SET(_bootstrap_command
      ${_darwin_x86_64} ${_bootstrap_command}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_boost_python_bin
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/python.exe
  )
ELSE()
  SET(_boost_python_bin
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python
  )
ENDIF()

STRING(TOLOWER ${CMAKE_BUILD_TYPE} _boost_variant)

LIST(
  TRANSFORM _boost_libs
  PREPEND "--with-"
          OUTPUT_VARIABLE _boost_with_list
)

EXTERNALPROJECT_ADD(
  ${_target}
  DEPENDS Python::Python
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${_bootstrap_command} --with-toolset=${_toolset} --with-python=${_boost_python_bin}
  BUILD_COMMAND
    # Ref.: https://www.boost.org/doc/libs/1_70_0/tools/build/doc/html/index.html#bbv2.builtin.features.cflags Ref.:
    # https://www.boost.org/doc/libs/1_76_0/tools/build/doc/html/index.html#bbv2.builtin.features.cflags
    ./b2 -a -q toolset=${_toolset} cxxstd=${RV_CPP_STANDARD} variant=${_boost_variant} link=shared threading=multi architecture=x86 address-model=64
    ${_boost_with_list} ${_boost_b2_options} -j${_cpu_count} install --prefix=${_install_dir} define=_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION
  INSTALL_COMMAND echo "Boost was both built and installed in the build stage"
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_boost_byproducts}
  USES_TERMINAL_BUILD TRUE
)

IF(RV_TARGET_WINDOWS)
  SET(_include_dir
      ${_install_dir}/include/boost-${_major_minor_version}
  )
ELSE()
  SET(_include_dir
      ${_install_dir}/include
  )
ENDIF()

FILE(MAKE_DIRECTORY ${_include_dir})

FOREACH(
  _boost_lib
  ${_boost_libs}
)
  ADD_LIBRARY(Boost::${_boost_lib} SHARED IMPORTED GLOBAL)
  ADD_DEPENDENCIES(Boost::${_boost_lib} ${_target})
  SET_PROPERTY(
    TARGET Boost::${_boost_lib}
    PROPERTY IMPORTED_LOCATION ${_boost_${_boost_lib}_lib}
  )
  SET_PROPERTY(
    TARGET Boost::${_boost_lib}
    PROPERTY IMPORTED_SONAME ${_boost_${_boost_lib}_lib_name}
  )

  IF(RV_TARGET_WINDOWS)
    SET_PROPERTY(
      TARGET Boost::${_boost_lib}
      PROPERTY IMPORTED_IMPLIB ${_boost_${_boost_lib}_implib}
    )
  ENDIF()
  TARGET_INCLUDE_DIRECTORIES(
    Boost::${_boost_lib}
    INTERFACE ${_include_dir}
  )

  LIST(APPEND RV_DEPS_LIST Boost::${_boost_lib})
  LIST(APPEND _boost_stage_output ${RV_STAGE_LIB_DIR}/${_boost_${_boost_lib}_lib_name})
ENDFOREACH()

ADD_LIBRARY(Boost::headers INTERFACE IMPORTED GLOBAL)
SET_TARGET_PROPERTIES(
  Boost::headers
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_include_dir}"
)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination
            "${RV_STAGE_BIN_DIR}"
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${_boost_stage_output}
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
    DEPENDS ${_target}
  )
ENDIF()

ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${_boost_stage_output}
)

ADD_CUSTOM_TARGET(
  clean-${_target}
  COMMENT "Cleaning '${_target}' ..."
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${_base_dir}
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${RV_DEPS_BASE_DIR}/cmake/dependencies/${_target}-prefix
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_BOOST_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
