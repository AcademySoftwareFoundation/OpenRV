#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

ADD_SUBDIRECTORY(FastMemcpyTest)
ADD_SUBDIRECTORY(QFontTest)
ADD_SUBDIRECTORY(CrashHandlerTest)

# End-to-end crash-dump smoke test: launches the app, triggers the test-only crash() command and asserts a minidump is produced (plus, where minidump_dump is
# available, the expected annotations). Enabled by default on every platform that builds the Crashpad handler. On Windows (no Breakpad/minidump_dump) it only
# verifies that a dump was produced.
OPTION(RV_BUILD_CRASH_DUMP_SMOKE_TEST "Register the crash-dump end-to-end smoke test" ON)
IF(RV_BUILD_CRASH_DUMP_SMOKE_TEST
   AND RV_DEPS_CRASHPAD_VERSION
)
  ADD_SUBDIRECTORY(CrashDumpSmokeTest)
ENDIF()

IF(RV_TARGET_LINUX)
  # These are simple tests replicating part of what the AudioRender is doing on Linux, uses `dlopen`
  ADD_SUBDIRECTORY(LoadingSharedLibrariesTest)
  # Won't run (yet) on macOS
  ADD_SUBDIRECTORY(IPCore)
ENDIF()
