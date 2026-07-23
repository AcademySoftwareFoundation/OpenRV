//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilCrashHandlerInternal_h_
#define _TwkUtilCrashHandlerInternal_h_

#include <string>

namespace TwkUtil
{

    //
    // Internal interface for platform-specific implementations
    // This header is not part of the public API
    //

    class CrashHandlerImpl
    {
    public:
        virtual ~CrashHandlerImpl() = default;

        virtual bool initialize(const std::string& appName, const std::string& appVersion, const std::string& handlerPath) = 0;
        virtual void addAnnotation(const std::string& key, const std::string& value) = 0;
        virtual void attachLogFile(const std::string& logFilePath) = 0;
        virtual bool isInitialized() const = 0;
        virtual std::string getCrashDumpDirectory() const = 0;
    };

    // Helper functions accessible to platform implementations
    std::string getDefaultCrashDumpDirectory();
    void cleanupOldCrashDumps(const std::string& crashDumpDir);

    // Factory function implemented by each platform file
    CrashHandlerImpl* createPlatformCrashHandlerImpl();

} // namespace TwkUtil

#endif // _TwkUtilCrashHandlerInternal_h_
