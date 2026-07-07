//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuTwkApp__CrashHandlerInit__h__
#define __MuTwkApp__CrashHandlerInit__h__

#include <string>

namespace TwkApp
{
    //
    //  initializeCrashHandler
    //
    //  Single shared crash-reporting init path for every RV executable (rv, the
    //  macOS RV bundle app, rvio, ...). Resolves the platform-specific crashpad
    //  handler *wrapper* sitting next to the executable, initializes the crash
    //  handler, and - on success - enables Mu debugging so dumps carry script
    //  source context.
    //
    //  Per-executable copies of this sequence are how the entry points drifted
    //  apart (bare handler vs wrapper, missing setDebugging); all binaries MUST
    //  call this instead. See docs/crash-reporting.md (contracts C4/C5).
    //
    //  The handler path is resolved as executableDir + "/" + the fixed wrapper
    //  name for the platform:
    //    - macOS:   crashpad_handler_macos.sh
    //    - Linux:   crashpad_handler_linux.sh
    //    - Windows: crashpad_handler.exe
    //
    //  executableDir is normally QCoreApplication::applicationDirPath() (passed
    //  as a string so this helper stays free of any Qt dependency). Must be
    //  called after the Qt application object exists.
    //
    //  The application version is derived from the build's MAJOR/MINOR/REVISION
    //  macros, so callers need not (and must not) format it themselves.
    //
    //  Returns true if the crash handler initialized.
    //
    bool initializeCrashHandler(const std::string& appName, const std::string& executableDir);
} // namespace TwkApp

#endif // __MuTwkApp__CrashHandlerInit__h__
