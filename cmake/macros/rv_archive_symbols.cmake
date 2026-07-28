#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# RV_CREATE_SYMBOLS_ARCHIVE_TARGET: define the 'symbols_archive' build target.
#
# Crash-reporting symbols are stripped from the customer-facing package (see the install pre_install*.cmake filters) and archived separately so that a customer
# crash dump can be symbolicated offline. This target collects the build's symbols into a single versioned zip under the packages directory:
#
# ${RV_PACKAGES_DIR}/RV-<version>-<os>-<arch>-symbols.zip
#
# OpenRV only PRODUCES this artifact. Uploading it to a symbol store (for example Artifactory) is the responsibility of the external build pipeline, so that no
# commercial-only infrastructure is assumed in the open-source base.
#
# Run it after a full build: cmake --build <build-dir> --target symbols_archive
#
# Platform contents: - macOS/Linux: the Breakpad symbols/ tree plus the symbolication tools (minidump_stackwalk, minidump_dump) and symbolicate_crash.sh, so the
# zip is self-contained. After unpacking, symbolicate with: ./symbolicate_crash.sh --symbols ./symbols <crash.dmp> - Windows: the PDB files (native WinDbg/CDB
# symbolication).
#
FUNCTION(RV_CREATE_SYMBOLS_ARCHIVE_TARGET)
  IF(NOT
     (RV_TARGET_DARWIN
      OR RV_TARGET_LINUX
      OR RV_TARGET_WINDOWS)
  )
    RETURN()
  ENDIF()

  # macOS/Linux symbols come from Breakpad; with no Breakpad there is nothing to archive.
  IF((RV_TARGET_DARWIN
      OR RV_TARGET_LINUX)
     AND NOT RV_DEPS_BREAKPAD_VERSION
  )
    RETURN()
  ENDIF()

  SET(_version
      "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
  )
  STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" _os)
  STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _arch)
  IF(NOT _arch)
    SET(_arch
        "unknown"
    )
  ENDIF()
  SET(_name
      "RV-${_version}-${_os}-${_arch}-symbols"
  )
  SET(_dir
      "${RV_PACKAGES_DIR}/${_name}"
  )
  SET(_zip
      "${RV_PACKAGES_DIR}/${_name}.zip"
  )

  IF(RV_TARGET_WINDOWS)
    # PDBs are scattered across the staged tree, so the glob must run at build time via a -P helper. Native symbolication uses WinDbg/CDB against them.
    ADD_CUSTOM_TARGET(
      symbols_archive
      COMMENT "Archiving Windows PDB symbols into ${_zip}"
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${_dir}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${_dir}
      COMMAND ${CMAKE_COMMAND} -DRV_APP_ROOT=${RV_APP_ROOT} -DRV_ARCHIVE_DIR=${_dir} -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/rv_collect_pdbs.cmake
      COMMAND ${CMAKE_COMMAND} -E tar cf ${_zip} --format=zip ${_name}
      WORKING_DIRECTORY ${RV_PACKAGES_DIR}
      VERBATIM
    )
  ELSE()
    ADD_CUSTOM_TARGET(
      symbols_archive
      COMMENT "Archiving crash-reporting symbols into ${_zip}"
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${_dir}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${RV_APP_ROOT}/symbols ${_dir}/symbols
      COMMAND ${CMAKE_COMMAND} -E copy ${RV_STAGE_BIN_DIR}/minidump_stackwalk ${RV_STAGE_BIN_DIR}/minidump_dump ${RV_STAGE_BIN_DIR}/symbolicate_crash.sh
              ${_dir}/
      COMMAND ${CMAKE_COMMAND} -E tar cf ${_zip} --format=zip ${_name}
      WORKING_DIRECTORY ${RV_PACKAGES_DIR}
      VERBATIM
    )
  ENDIF()

  # Build the main executable (and its POST_BUILD symbol generation) before archiving, so running the target on a fresh tree does not archive a missing or stale
  # symbols directory. A full build populates the rest of the tree.
  ADD_DEPENDENCIES(symbols_archive main_executable)
ENDFUNCTION()
