//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilCrashHandler_h_
#define _TwkUtilCrashHandler_h_

#include <TwkUtil/dll_defs.h>
#include <map>
#include <memory>
#include <string>

namespace TwkUtil
{

    /// @class CrashHandler
    /// @brief Singleton class for managing crash dump generation via Google Crashpad
    ///
    /// This class provides cross-platform crash dump generation using Google Crashpad.
    /// It captures crashes (SIGSEGV, SIGABRT, etc.) and generates minidumps with stack
    /// traces, registers, loaded modules, and custom annotations.
    ///
    /// Environment Variables:
    ///   RV_CRASH_DUMPS_ENABLED=1       - Enable/disable crash dumps (default: true)
    ///   RV_CRASH_DUMPS_DIR=""          - Override default crash dump directory
    ///   RV_CRASH_DUMPS_MAX_COUNT=10    - Maximum number of crash dumps to retain (default: 10)
    ///   RV_CRASH_DUMPS_ATTACH_LOGS=1   - Attach log files to crash dumps (default: true)
    ///
    /// Default Storage Locations:
    ///   macOS:   ~/Library/Logs/[OrgName]/Crashes/
    ///   Windows: %APPDATA%/[OrgName]/Crashes/
    ///   Linux:   ~/.local/share/[OrgName]/Crashes/
    ///
    /// Usage:
    /// @code
    ///   TwkUtil::CrashHandler& handler = TwkUtil::CrashHandler::instance();
    ///   bool success = handler.initialize("AppName", "1.0.0", "/path/to/crashpad_handler");
    ///   if (success) {
    ///       handler.addAnnotation("platform", "macOS");
    ///       handler.attachLogFile("/path/to/app.log");
    ///   }
    /// @endcode
    class TWKUTIL_EXPORT CrashHandler
    {
    public:
        /// @brief Get the singleton instance
        /// @return Reference to the singleton CrashHandler instance
        static CrashHandler& instance();

        /// @brief Initialize the crash handler
        /// @param appName Application name (used in crash reports)
        /// @param appVersion Application version string (e.g., "2025.1.0")
        /// @param handlerPath Absolute path to the crashpad_handler executable
        /// @return true if initialization succeeded, false otherwise
        ///
        /// This method must be called early in the application lifecycle, preferably
        /// before Qt or other frameworks are initialized, to ensure crashes during
        /// startup are captured.
        ///
        /// The handler path should point to:
        ///   macOS:   AppBundle.app/Contents/MacOS/crashpad_handler
        ///   Windows: [InstallDir]/crashpad_handler.exe
        ///   Linux:   [InstallDir]/crashpad_handler
        bool initialize(const std::string& appName, const std::string& appVersion, const std::string& handlerPath);

        /// @brief Add a custom annotation to crash reports
        /// @param key Annotation key (e.g., "platform", "gpu_vendor")
        /// @param value Annotation value
        ///
        /// Annotations are key-value pairs attached to crash reports. They provide
        /// additional context for debugging (e.g., OS version, GPU model, settings).
        /// Annotations added before initialize() are buffered (last value per key
        /// wins) and applied when initialize() succeeds, so callers need not worry
        /// about init ordering (e.g. GPU info discovered during early GL setup).
        void addAnnotation(const std::string& key, const std::string& value);

        /// @brief Attach a log file to crash reports
        /// @param logFilePath Absolute path to the log file
        ///
        /// The log file will be included with crash dumps if RV_CRASH_DUMPS_ATTACH_LOGS
        /// is enabled (default). This allows correlating crashes with recent log events.
        void attachLogFile(const std::string& logFilePath);

        /// @brief Check if the crash handler is initialized
        /// @return true if initialize() was successful, false otherwise
        bool isInitialized() const;

        /// @brief Get the crash dump storage directory
        /// @return Absolute path to the crash dump directory, or empty string if not initialized
        std::string getCrashDumpDirectory() const;

        /// @brief Destructor
        ~CrashHandler();

    private:
        /// @brief Private constructor (singleton pattern)
        CrashHandler();

        /// @brief Disable copy constructor
        CrashHandler(const CrashHandler&) = delete;

        /// @brief Disable assignment operator
        CrashHandler& operator=(const CrashHandler&) = delete;

        /// @brief Private implementation (pimpl idiom for platform-specific code)
        class Impl;
        std::unique_ptr<Impl> m_impl;

        /// @brief Annotations requested before initialize(); applied on success.
        std::map<std::string, std::string> m_pendingAnnotations;
    };

} // namespace TwkUtil

#endif // _TwkUtilCrashHandler_h_
