#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# LibRaw official Web page  -- https://www.libraw.org/about
# LibRaw official sources   -- https://www.libraw.org/data/LibRaw-0.21.1.tar.gz
# LibRaw build from sources -- https://www.libraw.org/docs/Install-LibRaw-eng.html
#
INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES( "RV_DEPS_RAW" "0.21.1" "make" "../src/configure")
IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' 
  # for some reason, this CMake-based project isn't using lib64
  SET(_lib_dir ${_install_dir}/lib)
ENDIF()
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://www.libraw.org/data/LibRaw-${_version}.tar.gz"
)

SET(_download_hash
    "2942732de752f46baccd9c6d57823b7b"
)

RV_MAKE_STANDARD_LIB_NAME("raw" "23" "SHARED" "")

# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
SET(_configure_options "")  # Overrides defaults set in 'RV_CREATE_STANDARD_DEPS_VARIABLES'
LIST(APPEND _configure_options "--prefix=${_install_dir}")
LIST(APPEND _configure_options "--enable-lcms")  # The lcms library was linked against in our legacy build system.

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  #DEPENDS ZLIB::ZLIB lcms 
  DEPENDS ZLIB::ZLIB
  CONFIGURE_COMMAND ${_configure_command} ${_configure_options}
  BUILD_COMMAND ${_make_command} -j${_cpu_count} -v
  INSTALL_COMMAND ${_make_command} install
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

# The macro is using existing _target, _libname, _lib_dir and _bin_dir variabless
RV_COPY_LIB_BIN_FOLDERS()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(Raw::Raw SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(Raw::Raw ${_target})
SET_PROPERTY(
  TARGET Raw::Raw
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET Raw::Raw
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET Raw::Raw
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

# It is required to force directory creation at configure time
# otherwise CMake complains about importing a non-existing path
FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  Raw::Raw
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST Raw::Raw)

