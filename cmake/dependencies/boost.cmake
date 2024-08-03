#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# IMPORTANT: CMake minimum version need to be increased everytime Boost version is increased.
#            e.g. CMake 3.27 is needed for Boost 1.82 to be found by FindBoost.cmake.
#
#            Starting from CMake 3.30, FindBoost.cmake has been removed in favor of BoostConfig.cmake (Boost 1.70+).
#            This behavior is covered by CMake policy CMP0167.

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

# Note: Boost 1.80 cannot be built with XCode 15 which is now the only XCode version available on macOS Sonoma without a hack. 
# Boost 1.81+ has all the fixes required to be able to be built with XCode 15, however it is not VFX Platform CY2023 compliant which specifies Boost version 1.80. 
# With the aim of making the OpenRV build on macOS smoother by default, OpenRV will use Boost 1.81 if XCode 15 or more recent.
IF(RV_TARGET_DARWIN)
  EXECUTE_PROCESS(
    COMMAND xcrun clang --version
    OUTPUT_VARIABLE CLANG_FULL_VERSION_STRING
  )
  STRING(
    REGEX
    REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${CLANG_FULL_VERSION_STRING}
  )
  IF(CLANG_VERSION_STRING VERSION_GREATER_EQUAL 15.0)
    MESSAGE(STATUS "Clang version ${CLANG_VERSION_STRING} is not compatible with Boost 1.80, using Boost 1.81 instead. "
                   "Install XCode 14.3.1 if you absolutely want to use Boost version 1.80 as per VFX reference platform CY2023"
    )
    SET(_BOOST_DETECTED_XCODE_15_ ON)
  ENDIF()
ENDIF()

# Set some variables for VFX2024 since those value are used at two locations.
SET(_BOOST_VFX2024_VERSION_ "1.82.0")
SET(_BOOST_VFX2024_MAJOR_MINOR_VERSION_ "1_82")
SET(_BOOST_VFX2024_DOWNLOAD_HASH_ "f7050f554a65f6a42ece221eaeec1660")

IF (NOT _BOOST_DETECTED_XCODE_15_)
  # XCode 14 and below.
  RV_VFX_SET_VARIABLE(
    _ext_boost_version
    CY2023 "1.80.0"
    CY2024 "${_BOOST_VFX2024_VERSION_}"
  )

  RV_VFX_SET_VARIABLE(
    _major_minor_version
    CY2023 "1_80"
    CY2024 "${_BOOST_VFX2024_MAJOR_MINOR_VERSION_}"
  )

  RV_VFX_SET_VARIABLE(
    _download_hash
    CY2023 "077f074743ea7b0cb49c6ed43953ae95"
    CY2024 "${_BOOST_VFX2024_DOWNLOAD_HASH_}"
  )
ELSE()
  # XCode 15 and above. (Need Boost 1.81+)
  RV_VFX_SET_VARIABLE(
    _ext_boost_version
    # Use Boost 1.81.0 for VFX2023 (Boost 1.80.0 does not work with XCode 15)
    CY2023 "1.81.0"
    CY2024 "${_BOOST_VFX2024_VERSION_}"
  )

  RV_VFX_SET_VARIABLE(
    _major_minor_version
    CY2023 "1_81"
    CY2024 "${_BOOST_VFX2024_MAJOR_MINOR_VERSION_}"
  )

  RV_VFX_SET_VARIABLE(
    _download_hash
    CY2023 "4bf02e84afb56dfdccd1e6aec9911f4b"
    CY2024 "${_BOOST_VFX2024_DOWNLOAD_HASH_}"
  )
ENDIF()

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_BOOST" "${_ext_boost_version}" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

STRING(REPLACE "." "_" _version_with_underscore ${_version})
SET(_download_url
    "https://archives.boost.io/release/${_version}/source/boost_${_version_with_underscore}.tar.gz"
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

SET(__boost_arch__ x86)
IF(APPLE)
  IF(RV_TARGET_APPLE_ARM64)
    SET(__boost_arch__ arm)
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
    ./b2 -a -q toolset=${_toolset} cxxstd=${RV_CPP_STANDARD} variant=${_boost_variant} link=shared threading=multi architecture=${__boost_arch__} address-model=64
    ${_boost_with_list} ${_boost_b2_options} -j${_cpu_count} install --prefix=${_install_dir}
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