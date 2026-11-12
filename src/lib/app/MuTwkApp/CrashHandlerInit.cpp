//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuTwkApp/CrashHandlerInit.h>
#include <TwkUtil/CrashHandler.h>
#include <sstream>

namespace TwkApp
{

    bool initializeCrashHandler(const std::string& appName, const std::string& executableDir)
    {
        // Version derived from the build macros, so all entry points report it
        // identically without duplicating the formatting.
        std::ostringstream versionStr;
        versionStr << MAJOR_VERSION << "." << MINOR_VERSION << "." << REVISION_NUMBER;
        const std::string appVersion = versionStr.str();

        // The handler is always the platform wrapper (never the bare
        // crashpad_handler) so the log redirection - and on Linux the ulimit
        // safety cap - always apply. Naming is fixed (contract C5).
#if defined(PLATFORM_WINDOWS)
        const std::string handlerName = "crashpad_handler.exe";
#elif defined(PLATFORM_LINUX)
        const std::string handlerName = "crashpad_handler_linux.sh";
#else
        const std::string handlerName = "crashpad_handler_macos.sh";
#endif
        const std::string handlerPath = executableDir + "/" + handlerName;

        return TwkUtil::CrashHandler::instance().initialize(appName, appVersion, handlerPath);
    }

} // namespace TwkApp
