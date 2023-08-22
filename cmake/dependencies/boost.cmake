#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
INCLUDE(cxx_defaults)

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

# This should handle fetching or checking then compiling required 3rd party dependencies
SET(_target
    "RV_DEPS_BOOST"
)

# This version of boost resolves Python3 compatibilty issues on Big Sur and Monterey and is compatible with Python 2.7 through Python 3.10
SET(_version
    "1.76.0"
)

STRING(REPLACE "." ";" _version_list ${_version})
LIST(GET VERSION_LIST 0 _major_version)
LIST(GET VERSION_LIST 1 _minor_version)

SET(_major_minor_version
    "${_major_version}_${_minor_version}"
)

STRING(REPLACE "." "_" _version_with_underscore ${_version})
SET(_download_url
    "https://boostorg.jfrog.io/artifactory/main/release/${_version}/source/boost_${_version_with_underscore}.tar.gz"
)

SET(_download_hash
    e425bf1f1d8c36a3cd464884e74f007a
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(_lib_dir
    ${_install_dir}/lib
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

FILE(MAKE_DIRECTORY ${_install_dir})
FILE(MAKE_DIRECTORY ${_lib_dir})
FILE(MAKE_DIRECTORY ${_include_dir})

SET(_boost_libs
    chrono
    date_time
    filesystem
    graph
    iostreams
    program_options
    random
    regex
    serialization
    system
    thread
    timer
)

LIST(
  TRANSFORM _boost_libs
  PREPEND "--with-"
          OUTPUT_VARIABLE _boost_with_list
)

STRING(TOLOWER ${CMAKE_BUILD_TYPE} _boost_variant)

# Note: Boost has a custom lib naming scheme on windows
IF(RV_TARGET_WINDOWS)
  SET(BOOST_SHARED_LIBRARY_PREFIX
      ""
  )
  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(BOOST_LIBRARY_SUFFIX
        "-vc142-mt-gd-x64-${_major_minor_version}"
    )
  ELSE()
    SET(BOOST_LIBRARY_SUFFIX
        "-vc142-mt-x64-${_major_minor_version}"
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

LIST(APPEND _boost_cxx_flags "-stdlib=libc++")
LIST(APPEND _boost_link_flags "-stdlib=libc++")

IF(RV_TARGET_DARWIN)
  SET(_toolset
      "clang"
  )

  LIST(APPEND _boost_cxx_flags "-fPIC")

  FOREACH(
    _cmake_arch
    ${CMAKE_OSX_ARCHITECTURES}
  )
    LIST(APPEND _boost_cxx_flags "-arch" "${_cmake_arch}")
    LIST(APPEND _boost_link_flags "-arch" "${_cmake_arch}")
  ENDFOREACH()

  LIST(APPEND _boost_cxx_flags "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
  LIST(APPEND _boost_link_flags "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
ELSEIF(RV_TARGET_LINUX)
  SET(_toolset
      "gcc"
  )

  LIST(APPEND _boost_cxx_flags "-fPIC")
ELSEIF(RV_TARGET_WINDOWS)
  SET(_toolset
      "msvc-14.2"
  )
ELSE()
  MESSAGE(FATAL_ERROR "Unsupported (yet) target for Boost")
ENDIF()

SET(_boost_b2_command
    ./b2
)

IF(RV_TARGET_WINDOWS)
  SET(_boost_bootstrap_command
      ./bootstrap.bat
  )
ELSE()
  SET(_boost_bootstrap_command
      ./bootstrap.sh
  )
ENDIF()

LIST(APPEND _boost_bootstrap_options "--prefix=${_install_dir}")
LIST(APPEND _boost_bootstrap_options "--with-toolset=${_toolset}")
LIST(APPEND _boost_bootstrap_options "--with-python=${RV_DEPS_PYTHON3_EXECUTABLE}")

LIST(APPEND _boost_b2_options "toolset=${_toolset}")
LIST(APPEND _boost_b2_options "${_boost_with_list}")
LIST(APPEND _boost_b2_options "-d+0")
LIST(APPEND _boost_b2_options "-q")
LIST(APPEND _boost_b2_options "cxxstd=${RV_CPP_STANDARD}")
LIST(APPEND _boost_b2_options "visibility=global")
LIST(APPEND _boost_b2_options "variant=${_boost_variant}")
LIST(APPEND _boost_b2_options "link=shared")
LIST(APPEND _boost_b2_options "address-model=64")
LIST(APPEND _boost_b2_options "threading=multi")

STRING(REPLACE ";" " " _boost_cxx_flags_joined "${_boost_cxx_flags}")
STRING(REPLACE ";" " " _boost_link_flags_joined "${_boost_link_flags}")

LIST(APPEND _boost_b2_options "cxxflags=${_boost_cxx_flags_joined}")
LIST(APPEND _boost_b2_options "linkflags=${_boost_link_flags_joined}")

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
  CONFIGURE_COMMAND ${_boost_bootstrap_command} ${_boost_boostrap_options}
  BUILD_COMMAND ${_boost_b2_command} ${_boost_b2_options} -j ${_cpu_count} install --prefix=${_install_dir}
  INSTALL_COMMAND echo "Boost was both built and installed in the build stage"
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_boost_byproducts}
  USES_TERMINAL_BUILD TRUE
)

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
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_BIN_DIR}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${_boost_stage_output}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    DEPENDS ${_target}
  )
ENDIF()

ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${_boost_stage_output}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_BOOST_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
