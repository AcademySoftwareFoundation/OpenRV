#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

IF(RV_TARGET_WINDOWS)
  FIND_PROGRAM(_jom_executable jom)
  IF(_jom_executable)
    SET(_libraw_build_command
        ${_jom_executable} /J${_cpu_count} /f Makefile.msvc
    )
  ELSE()
    SET(_libraw_build_command
        nmake /f Makefile.msvc
    )
  ENDIF()

  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_base_dir}/src
    INSTALL_DIR ${_install_dir}
    DEPENDS ZLIB::ZLIB
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${_libraw_build_command}
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )

  # Building with nmake for Windows doesn't provide a nice install target, we need to do it manually. We remove unneeded files after copying the required
  # directories
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs & files into ${_install_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_base_dir}/src/lib ${_lib_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_base_dir}/src/libraw ${_include_dir}/libraw
    # Copy only the DLL on Windows because there is no option to disable the "examples/samples" with Makefile.msvc.
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_bin_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${_base_dir}/src/bin/${_libname} ${_bin_dir}
    COMMAND ${CMAKE_COMMAND} -E rm ${_lib_dir}/Makefile
  )

ELSE()
  # The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
  SET(_configure_options
      ""
  ) # Overrides defaults set in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
  LIST(APPEND _configure_options "--prefix=${_install_dir}")
  LIST(APPEND _configure_options "--enable-lcms")

  GET_TARGET_PROPERTY(_lcms_include_dir lcms INTERFACE_INCLUDE_DIRECTORIES)
  SET(_lcms2_flags
      "-I${_lcms_include_dir}"
  )
  SET(_lcms2_libs
      "-L${RV_STAGE_LIB_DIR} -llcms"
  )

  SET(_configure_command
      ${CMAKE_COMMAND} -E env LCMS2_CFLAGS='${_lcms2_flags}' ${CMAKE_COMMAND} -E env LCMS2_LIBS='${_lcms2_libs}' ${_configure_command}
  )

  EXTERNALPROJECT_ADD(
    ${_target}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    INSTALL_DIR ${_install_dir}
    DEPENDS ZLIB::ZLIB lcms
    CONFIGURE_COMMAND aclocal
    COMMAND autoreconf --install
    COMMAND ${_configure_command} ${_configure_options}
    BUILD_COMMAND ${_make_command}
    INSTALL_COMMAND ${_make_command} install
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
ENDIF()
