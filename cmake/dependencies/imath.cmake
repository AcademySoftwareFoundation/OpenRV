#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_IMATH"
)

SET(_version
    "3.1.5"
)

SET(_download_url
    "https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v${_version}.zip"
)

SET(_download_hash
    "921ac54505ab076a95b33a16b61956f4"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)
SET(_include_dir
    ${_install_dir}/include/Imath
)
SET(RV_DEPS_IMATH_ROOT_DIR
    ${_install_dir}
)

SET(_make_command
    make
)
SET(_configure_command
    cmake
)

IF(${RV_OSX_EMULATION})
  SET(_darwin_x86_64
      "arch" "${RV_OSX_EMULATION_ARCH}"
  )

  SET(_make_command
      ${_darwin_x86_64} ${_make_command}
  )
  SET(_configure_command
      ${_darwin_x86_64} ${_configure_command}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  # MSYS2/CMake defaults to Ninja
  SET(_make_command
      ninja
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}.29.4.0${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}.29.4.0
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_libname
      ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

IF(RV_TARGET_LINUX)
  SET(_lib_dir
      ${_install_dir}/lib64
  )
ELSE()
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

SET(RV_DEPS_IMATH_CMAKE_DIR
    ${_lib_dir}/cmake/Imath
    CACHE STRING "Path to Imath CMake files ${_target}"
)

SET(RV_DEPS_IMATH_CMAKE_DIR
    ${_lib_dir}/cmake/Imath
)

SET(_libpath
    ${_lib_dir}/${_libname}
)

LIST(APPEND _imath_byproducts ${_libpath})

IF(RV_TARGET_WINDOWS)
  SET(_implibpath
      ${_install_dir}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}Imath-3_1${RV_DEBUG_POSTFIX}${CMAKE_IMPORT_LIBRARY_SUFFIX}
  )
  LIST(APPEND _imath_byproducts ${_implibpath})
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${_install_dir} -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${RV_DEPS_BASE_DIR}/${_target}/src
  BUILD_COMMAND ${_make_command} -j${_cpu_count}
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_imath_byproducts}
  USES_TERMINAL_BUILD TRUE
)

FILE(MAKE_DIRECTORY "${_include_dir}")

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND python3 "${PROJECT_SOURCE_DIR}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/lib" --destination
            "${RV_STAGE_LIB_DIR}"
    COMMAND python3 "${PROJECT_SOURCE_DIR}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/bin" --destination
            "${RV_STAGE_BIN_DIR}"
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_libname}
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_libname}
    COMMAND python3 "${PROJECT_SOURCE_DIR}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination
            "${RV_STAGE_LIB_DIR}"
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_libname}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(Imath::Imath SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(Imath::Imath ${_target})
SET_PROPERTY(
  TARGET Imath::Imath
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET Imath::Imath
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET Imath::Imath
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()
TARGET_INCLUDE_DIRECTORIES(
  Imath::Imath
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST Imath::Imath)

SET(RV_DEPS_IMATH_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
