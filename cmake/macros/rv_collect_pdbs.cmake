#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Helper for the 'symbols_archive' target on Windows (run via `cmake -P`).
#
# Copies every staged .pdb file into ${RV_ARCHIVE_DIR}/symbols, preserving each file's path relative to RV_APP_ROOT so that identically-named PDBs from
# different modules do not collide. The glob runs here (build time) because the PDBs are build artifacts that do not exist at configure time.
#
# Required -D arguments: RV_APP_ROOT, RV_ARCHIVE_DIR.
#
FILE(
  GLOB_RECURSE _pdbs
  RELATIVE "${RV_APP_ROOT}"
  "${RV_APP_ROOT}/*.pdb"
)

FOREACH(
  _pdb
  ${_pdbs}
)
  GET_FILENAME_COMPONENT(_subdir "${_pdb}" DIRECTORY)
  FILE(
    COPY "${RV_APP_ROOT}/${_pdb}"
    DESTINATION "${RV_ARCHIVE_DIR}/symbols/${_subdir}"
  )
ENDFOREACH()

LIST(LENGTH _pdbs _count)
MESSAGE(STATUS "rv_collect_pdbs: copied ${_count} PDB file(s) into ${RV_ARCHIVE_DIR}/symbols")
