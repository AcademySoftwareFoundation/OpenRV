#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_BREAKPAD" "${RV_DEPS_BREAKPAD_VERSION}" "" "")

# Breakpad - for crash dump symbolication tools (dump_syms, minidump_stackwalk)
SET(_download_url
    "https://github.com/google/breakpad/archive/${_version}.tar.gz"
)

SET(_download_hash
    ${RV_DEPS_BREAKPAD_DOWNLOAD_HASH}
)

SET(_source_dir
    ${RV_DEPS_BASE_DIR}/${_target}/src/breakpad-${_version}
)

SET(_build_dir
    ${RV_DEPS_BASE_DIR}/${_target}/build
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(_bin_dir
    ${_install_dir}/bin
)

# Tool names
SET(_dump_syms_name
    dump_syms
)
SET(_minidump_stackwalk_name
    minidump_stackwalk
)
SET(_minidump_dump_name
    minidump_dump
)
SET(_dump_syms_tool
    ${_bin_dir}/${_dump_syms_name}
)
SET(_minidump_stackwalk_tool
    ${_bin_dir}/${_minidump_stackwalk_name}
)
SET(_minidump_dump_tool
    ${_bin_dir}/${_minidump_dump_name}
)
IF(RV_TARGET_DARWIN)
  SET(_dump_syms_xcode_path
      ${_source_dir}/src/tools/mac/dump_syms/build/Release/dump_syms
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  EXTERNALPROJECT_ADD(
    ${_target}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    BINARY_DIR ${_build_dir}
    INSTALL_DIR ${_install_dir}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    PATCH_COMMAND git clone https://github.com/google/googletest.git ${_source_dir}/src/testing || true
    CONFIGURE_COMMAND ${_source_dir}/configure --prefix=${_install_dir}
    BUILD_COMMAND make -j${_cpu_count}
    COMMAND xcodebuild -project ${_source_dir}/src/tools/mac/dump_syms/dump_syms.xcodeproj -configuration Release
    INSTALL_COMMAND make install
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_minidump_stackwalk_tool} ${_minidump_dump_tool} ${_dump_syms_xcode_path}
    USES_TERMINAL_BUILD TRUE
  )
ELSE()
  # Linux - needs LSS header in addition to googletest
  EXTERNALPROJECT_ADD(
    ${_target}
    DOWNLOAD_NAME ${_target}_${_version}.tar.gz
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    BINARY_DIR ${_build_dir}
    INSTALL_DIR ${_install_dir}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    PATCH_COMMAND git clone https://github.com/google/googletest.git ${_source_dir}/src/testing || true
    COMMAND git clone https://chromium.googlesource.com/linux-syscall-support ${_source_dir}/src/third_party/lss || true
    CONFIGURE_COMMAND ${_source_dir}/configure --prefix=${_install_dir}
    BUILD_COMMAND make -j${_cpu_count}
    INSTALL_COMMAND make install
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_dump_syms_tool} ${_minidump_stackwalk_tool} ${_minidump_dump_tool}
    USES_TERMINAL_BUILD TRUE
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  ADD_CUSTOM_COMMAND(
    COMMENT "Staging ${_target} tools into ${RV_STAGE_BIN_DIR}"
    OUTPUT ${RV_STAGE_BIN_DIR}/${_minidump_stackwalk_name} ${RV_STAGE_BIN_DIR}/${_minidump_dump_name} ${RV_STAGE_BIN_DIR}/${_dump_syms_name}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_minidump_stackwalk_tool} ${RV_STAGE_BIN_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_minidump_dump_tool} ${RV_STAGE_BIN_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_dump_syms_xcode_path} ${RV_STAGE_BIN_DIR}/
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_minidump_stackwalk_name}
    DEPENDS ${RV_STAGE_BIN_DIR}/${_minidump_dump_name}
    DEPENDS ${RV_STAGE_BIN_DIR}/${_dump_syms_name}
  )
ELSE()
  # Linux
  ADD_CUSTOM_COMMAND(
    COMMENT "Staging ${_target} tools into ${RV_STAGE_BIN_DIR}"
    OUTPUT ${RV_STAGE_BIN_DIR}/${_dump_syms_name} ${RV_STAGE_BIN_DIR}/${_minidump_stackwalk_name} ${RV_STAGE_BIN_DIR}/${_minidump_dump_name}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_dump_syms_tool} ${RV_STAGE_BIN_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_minidump_stackwalk_tool} ${RV_STAGE_BIN_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_minidump_dump_tool} ${RV_STAGE_BIN_DIR}/
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_dump_syms_name}
    DEPENDS ${RV_STAGE_BIN_DIR}/${_minidump_stackwalk_name}
    DEPENDS ${RV_STAGE_BIN_DIR}/${_minidump_dump_name}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_BREAKPAD_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
