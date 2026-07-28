#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# End-to-end crash-dump smoke test. Drives the staged main executable through crash_dump_smoke_test.py. Enabled by default; toggle with
# RV_BUILD_CRASH_DUMP_SMOKE_TEST.

# The main executable's staged name differs per platform (nsapps/RV is macOS-only).
IF(RV_TARGET_DARWIN)
  SET(_rv_binary
      "${RV_STAGE_BIN_DIR}/RV"
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_rv_binary
      "${RV_STAGE_BIN_DIR}/rv.exe"
  )
ELSE()
  SET(_rv_binary
      "${RV_STAGE_BIN_DIR}/rv"
  )
ENDIF()

FIND_PACKAGE(
  Python3
  COMPONENTS Interpreter
  REQUIRED
)

SET(_smoke_command
    ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/crash_dump_smoke_test.py --rv-binary ${_rv_binary}
)

# minidump_dump is a Breakpad tool (macOS/Linux only). On Windows it is not built, so omit it; the script then skips annotation verification and only checks
# that a dump was produced (Windows symbolicates the native PDBs separately).
IF(RV_DEPS_BREAKPAD_VERSION)
  LIST(APPEND _smoke_command --minidump-dump ${RV_STAGE_BIN_DIR}/minidump_dump)
ENDIF()

ADD_TEST(
  NAME "CrashDumpSmokeTest"
  COMMAND ${_smoke_command}
)
