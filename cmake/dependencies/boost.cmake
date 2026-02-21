#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# IMPORTANT: CMake minimum version need to be increased everytime Boost version is increased. e.g. CMake 3.27 is needed for Boost 1.82 to be found by
# FindBoost.cmake.
#
# Starting from CMake 3.30, FindBoost.cmake has been removed in favor of BoostConfig.cmake (Boost 1.70+). This behavior is covered by CMake policy CMP0167.

SET(_ext_boost_version
    ${RV_DEPS_BOOST_VERSION}
)
SET(_major_minor_version
    ${RV_DEPS_BOOST_MAJOR_MINOR_VERSION}
)
SET(_download_hash
    ${RV_DEPS_BOOST_DOWNLOAD_HASH}
)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_BOOST" "${_ext_boost_version}" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

STRING(REPLACE "." "_" _version_with_underscore ${_version})
SET(_download_url
    "https://archives.boost.io/release/${_version}/source/boost_${_version_with_underscore}.tar.gz"
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

LIST(APPEND _boost_b2_options "-s")
LIST(APPEND _boost_b2_options "NO_LZMA=1")

IF(RV_VERBOSE_INVOCATION)
  LIST(APPEND _boost_b2_options "-d+2")
ELSE()
  LIST(APPEND _boost_b2_options "-d+0")
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

SET(__boost_arch__
    x86
)
IF(APPLE)
  IF(RV_TARGET_APPLE_ARM64)
    SET(__boost_arch__
        arm
    )
  ENDIF()
ENDIF()

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
    ./b2 -a -q toolset=${_toolset} cxxstd=${RV_CPP_STANDARD} variant=${_boost_variant} link=shared threading=multi architecture=${__boost_arch__}
    address-model=64 ${_boost_with_list} ${_boost_b2_options} -j${_cpu_count} install --prefix=${_install_dir}
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

# Note: On Windows, Boost's b2 puts both .lib and .dll in lib/, so we copy _lib_dir to both RV_STAGE_LIB_DIR and RV_STAGE_BIN_DIR.
IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} EXTRA_LIB_DIRS ${RV_STAGE_BIN_DIR} OUTPUTS ${_boost_stage_output})
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${_boost_stage_output})
ENDIF()

