//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/CrashHandler.h>
#include "CrashHandler_internal.h"
#include <TwkUtil/EnvVar.h>
#include <QtCore/QtCore>
#include <iostream>
#include <map>
#include <vector>

#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/annotation.h>

namespace TwkUtil
{

    // Environment variables for crash handler configuration
    //
    // NOTE: When RV_CRASH_DUMPS_ENABLED is set, RV automatically enables Mu debugging
    // (equivalent to -debug mu flag) to include Mu script source file information in
    // crash dumps. This helps identify which Mu script caused a crash.
    //
    ENVVAR_BOOL(evCrashDumpsEnabled, "RV_CRASH_DUMPS_ENABLED", true);
    ENVVAR_STRING(evCrashDumpsDir, "RV_CRASH_DUMPS_DIR", "");
    ENVVAR_INT(evCrashDumpsMaxCount, "RV_CRASH_DUMPS_MAX_COUNT", 10);
    ENVVAR_BOOL(evCrashDumpsAttachLogs, "RV_CRASH_DUMPS_ATTACH_LOGS", true);

    //
    // Helper functions accessible to platform implementations
    //

    std::string getDefaultCrashDumpDirectory()
    {
        std::string baseDir;

#if defined(PLATFORM_DARWIN)
        // macOS: ~/Library/Logs/[OrgName]/Crashes/
        baseDir = QDir::homePath().toStdString() + "/Library/Logs/" + QCoreApplication::organizationName().toStdString() + "/Crashes/";
#elif defined(PLATFORM_WINDOWS)
        // Windows: %APPDATA%/[OrgName]/Crashes/
        baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() + "/Crashes/";
#else
        // Linux: ~/.local/share/[OrgName]/Crashes/
        baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() + "/Crashes/";
#endif

        return baseDir;
    }

    void cleanupOldCrashDumps(const std::string& crashDumpDir)
    {
        if (crashDumpDir.empty() || evCrashDumpsMaxCount.getValue() <= 0)
        {
            return;
        }

        QDir crashDir(QString::fromStdString(crashDumpDir));
        if (!crashDir.exists())
        {
            return;
        }

        // Get all .dmp files sorted by modification time (oldest first)
        QFileInfoList dumpFiles = crashDir.entryInfoList(QStringList() << "*.dmp", QDir::Files, QDir::Time);

        int maxCount = evCrashDumpsMaxCount.getValue();
        int toDelete = dumpFiles.size() - maxCount;

        if (toDelete > 0)
        {
            std::cout << "INFO: Cleaning up " << toDelete << " old crash dump(s) to enforce max count of " << maxCount << std::endl;

            // Delete oldest files
            for (int i = dumpFiles.size() - 1; i >= dumpFiles.size() - toDelete; --i)
            {
                const QFileInfo& fileInfo = dumpFiles.at(i);
                if (QFile::remove(fileInfo.absoluteFilePath()))
                {
                    std::cout << "INFO: Deleted old crash dump: " << fileInfo.fileName().toStdString() << std::endl;
                }
            }
        }
    }

    // Helper function to convert string to FilePath for different platforms
    static base::FilePath StringToFilePath(const std::string& path)
    {
#if defined(PLATFORM_WINDOWS)
        // On Windows, FilePath expects wide strings (UTF-16)
        std::wstring wpath(path.begin(), path.end());
        return base::FilePath(wpath);
#else
        // On Unix platforms (macOS/Linux), FilePath takes regular strings (UTF-8)
        return base::FilePath(path);
#endif
    }

    //
    // CrashHandler - Public interface implementation
    //

    // Bridge the pimpl Impl to the internal CrashHandlerImpl
    class CrashHandler::Impl : public CrashHandlerImpl
    {
    public:
        using CrashHandlerImpl::CrashHandlerImpl;
    };

    CrashHandler::CrashHandler()
        : m_impl(static_cast<Impl*>(createPlatformCrashHandlerImpl()))
    {
    }

    CrashHandler::~CrashHandler() {}

    CrashHandler& CrashHandler::instance()
    {
        static CrashHandler s_instance;
        return s_instance;
    }

    bool CrashHandler::initialize(const std::string& appName, const std::string& appVersion, const std::string& handlerPath)
    {
        if (!evCrashDumpsEnabled.getValue())
        {
            std::cout << "INFO: Crash dumps disabled via RV_CRASH_DUMPS_ENABLED environment variable" << std::endl;
            return false;
        }

        if (m_impl->isInitialized())
        {
            std::cout << "WARNING: CrashHandler already initialized" << std::endl;
            return true;
        }

        bool success = m_impl->initialize(appName, appVersion, handlerPath);

        if (success)
        {
            std::cout << "INFO: Crash handler initialized successfully" << std::endl;
            std::cout << "INFO: Crash dumps will be saved to: " << m_impl->getCrashDumpDirectory() << std::endl;

            // Apply any annotations that were requested before init (e.g. GPU
            // info discovered during early GL setup) so they are not lost.
            for (const auto& [key, value] : m_pendingAnnotations)
            {
                m_impl->addAnnotation(key, value);
            }
            m_pendingAnnotations.clear();

            // Clean up old crash dumps if needed
            cleanupOldCrashDumps(m_impl->getCrashDumpDirectory());
        }
        else
        {
            std::cerr << "ERROR: Failed to initialize crash handler" << std::endl;
        }

        return success;
    }

    void CrashHandler::addAnnotation(const std::string& key, const std::string& value)
    {
        if (!m_impl->isInitialized())
        {
            // Buffer until init; the latest value per key wins. Applied in
            // initialize() so annotations set during early startup are not lost.
            m_pendingAnnotations[key] = value;
            return;
        }

        m_impl->addAnnotation(key, value);
    }

    void CrashHandler::attachLogFile(const std::string& logFilePath)
    {
        if (!m_impl->isInitialized())
        {
            std::cerr << "WARNING: Cannot attach log file - crash handler not initialized" << std::endl;
            return;
        }

        if (!evCrashDumpsAttachLogs.getValue())
        {
            return;
        }

        m_impl->attachLogFile(logFilePath);
    }

    bool CrashHandler::isInitialized() const { return m_impl->isInitialized(); }

    std::string CrashHandler::getCrashDumpDirectory() const { return m_impl->getCrashDumpDirectory(); }

    //
    // Platform-specific implementation using Crashpad
    //

    // Platform-specific configuration
#if defined(PLATFORM_DARWIN)
#define CRASH_HANDLER_IMPL_CLASS CrashHandlerDarwinImpl
#define PLATFORM_STRING "macOS"
#elif defined(PLATFORM_WINDOWS)
#define CRASH_HANDLER_IMPL_CLASS CrashHandlerWindowsImpl
#define PLATFORM_STRING "Windows"
#else
#define CRASH_HANDLER_IMPL_CLASS CrashHandlerLinuxImpl
#define PLATFORM_STRING "Linux"
#endif

    //
    // Runtime Crashpad annotations for dynamic crash metadata
    //
    namespace
    {
        // Buffer size for annotations (max 1024 chars each)
        constexpr size_t kAnnotationBufferSize = 1024;

        // Static annotation buffers - must have static storage duration
        // Mu script annotations (function-level: the Mu crash observer records
        // the function and its file, not a source line - see MuCrashObserver)
        char g_muScriptFileBuffer[kAnnotationBufferSize];
        char g_muFunctionBuffer[kAnnotationBufferSize];

        // Python script annotations
        char g_pyScriptFileBuffer[kAnnotationBufferSize];
        char g_pyScriptLineBuffer[64];
        char g_pyFunctionBuffer[kAnnotationBufferSize];

        // GPU annotations
        char g_gpuVendorBuffer[kAnnotationBufferSize];
        char g_gpuRendererBuffer[kAnnotationBufferSize];

        // Python interop annotations
        char g_pythonCallerBuffer[kAnnotationBufferSize];

        // Crashpad annotation objects for Mu
        crashpad::Annotation g_muScriptFile(crashpad::Annotation::Type::kString, "mu_script_file", g_muScriptFileBuffer);
        crashpad::Annotation g_muFunction(crashpad::Annotation::Type::kString, "mu_function", g_muFunctionBuffer);

        // Crashpad annotation objects for Python
        crashpad::Annotation g_pyScriptFile(crashpad::Annotation::Type::kString, "py_script_file", g_pyScriptFileBuffer);
        crashpad::Annotation g_pyScriptLine(crashpad::Annotation::Type::kString, "py_script_line", g_pyScriptLineBuffer);
        crashpad::Annotation g_pyFunction(crashpad::Annotation::Type::kString, "py_function", g_pyFunctionBuffer);

        // Crashpad annotation objects for GPU
        crashpad::Annotation g_gpuVendor(crashpad::Annotation::Type::kString, "gpu_vendor", g_gpuVendorBuffer);
        crashpad::Annotation g_gpuRenderer(crashpad::Annotation::Type::kString, "gpu_renderer", g_gpuRendererBuffer);

        // Crashpad annotation objects for Python interop
        crashpad::Annotation g_pythonCaller(crashpad::Annotation::Type::kString, "python_caller", g_pythonCallerBuffer);

        // Map of annotation names to their Crashpad annotation objects
        struct AnnotationMapping
        {
            const char* name;
            crashpad::Annotation* annotation;
            char* buffer;
            size_t buffer_size;
        };

        AnnotationMapping g_annotationMappings[] = {
            {"mu_script_file", &g_muScriptFile, g_muScriptFileBuffer, kAnnotationBufferSize},
            {"mu_function", &g_muFunction, g_muFunctionBuffer, kAnnotationBufferSize},
            {"py_script_file", &g_pyScriptFile, g_pyScriptFileBuffer, kAnnotationBufferSize},
            {"py_script_line", &g_pyScriptLine, g_pyScriptLineBuffer, 64},
            {"py_function", &g_pyFunction, g_pyFunctionBuffer, kAnnotationBufferSize},
            {"gpu_vendor", &g_gpuVendor, g_gpuVendorBuffer, kAnnotationBufferSize},
            {"gpu_renderer", &g_gpuRenderer, g_gpuRendererBuffer, kAnnotationBufferSize},
            {"python_caller", &g_pythonCaller, g_pythonCallerBuffer, kAnnotationBufferSize},
        };
    } // namespace

    //
    // Unified platform implementation (Darwin/Windows/Linux)
    //
    class CRASH_HANDLER_IMPL_CLASS : public CrashHandlerImpl
    {
    public:
        CRASH_HANDLER_IMPL_CLASS()
            : m_initialized(false)
        {
        }

        ~CRASH_HANDLER_IMPL_CLASS() override = default;

        bool initialize(const std::string& appName, const std::string& appVersion, const std::string& handlerPath) override;
        void addAnnotation(const std::string& key, const std::string& value) override;
        void attachLogFile(const std::string& logFilePath) override;

        bool isInitialized() const override { return m_initialized; }

        std::string getCrashDumpDirectory() const override { return m_crashDumpDir; }

    private:
        bool m_initialized;
        std::string m_crashDumpDir;
        std::string m_appName;
        std::string m_appVersion;

        std::unique_ptr<crashpad::CrashpadClient> m_client;
        std::unique_ptr<crashpad::CrashReportDatabase> m_database;
        std::map<std::string, std::string> m_annotations;
        std::vector<base::FilePath> m_attachments;
        std::string m_handlerPath;
        std::vector<std::string> m_arguments;

        bool ensureCrashDumpDirectoryExists();
    };

    bool CRASH_HANDLER_IMPL_CLASS::ensureCrashDumpDirectoryExists()
    {
        QDir dir(QString::fromStdString(m_crashDumpDir));
        if (!dir.exists())
        {
            if (!dir.mkpath("."))
            {
                std::cerr << "ERROR: Failed to create crash dump directory: " << m_crashDumpDir << std::endl;
                return false;
            }
        }
        return true;
    }

    bool CRASH_HANDLER_IMPL_CLASS::initialize(const std::string& appName, const std::string& appVersion, const std::string& handlerPath)
    {
        if (m_initialized)
        {
            return true;
        }

        m_appName = appName;
        m_appVersion = appVersion;
        m_client = std::make_unique<crashpad::CrashpadClient>();

        // Determine crash dump directory
        std::string customDir = evCrashDumpsDir.getValue();
        if (!customDir.empty())
        {
            m_crashDumpDir = customDir;
            if (m_crashDumpDir.back() != '/')
            {
                m_crashDumpDir += '/';
            }
            std::cout << "INFO: Using custom crash dump directory from RV_CRASH_DUMPS_DIR: " << m_crashDumpDir << std::endl;
        }
        else
        {
            m_crashDumpDir = getDefaultCrashDumpDirectory();
        }

        // Ensure directory exists
        if (!ensureCrashDumpDirectoryExists())
        {
            return false;
        }

        // Initialize crash report database
        base::FilePath dbPath = StringToFilePath(m_crashDumpDir);
        m_database = crashpad::CrashReportDatabase::Initialize(dbPath);
        if (!m_database)
        {
            std::cerr << "ERROR: Failed to initialize Crashpad crash report database at: " << m_crashDumpDir << std::endl;
            return false;
        }

        // Disable uploads (local storage only)
        crashpad::Settings* settings = m_database->GetSettings();
        if (settings)
        {
            settings->SetUploadsEnabled(false);
        }

        // Prepare handler path
        if (!QFile::exists(QString::fromStdString(handlerPath)))
        {
            std::cerr << "ERROR: Crashpad handler not found at: " << handlerPath << std::endl;
            return false;
        }
        m_handlerPath = handlerPath;

        // Prepare arguments for crashpad_handler
        m_arguments.push_back("--no-rate-limit"); // No upload rate limiting for local storage

        // Add initial annotations
        m_annotations["app_name"] = appName;
        m_annotations["app_version"] = appVersion;
        m_annotations["platform"] = PLATFORM_STRING;
        m_annotations["qt_version"] = QT_VERSION_STR;

        // Add system information
        m_annotations["cpu_count"] = QString::number(QThread::idealThreadCount()).toStdString();
        m_annotations["os_version"] = QSysInfo::productVersion().toStdString();
        m_annotations["os_kernel"] = QSysInfo::kernelVersion().toStdString();

        // Start the Crashpad handler
        bool success = m_client->StartHandler(StringToFilePath(m_handlerPath), // handler path
                                              dbPath,                          // database path
                                              dbPath,                          // metrics dir (same as db)
                                              "",                              // url (empty for local - no uploads)
                                              "",                              // http_proxy (empty - no uploads)
                                              m_annotations,                   // annotations
                                              m_arguments,                     // arguments
                                              true,                            // restartable
                                              false);                          // asynchronous_start

        if (!success)
        {
            std::cerr << "ERROR: Failed to start Crashpad handler" << std::endl;
            return false;
        }

        m_initialized = true;
        return true;
    }

    void CRASH_HANDLER_IMPL_CLASS::addAnnotation(const std::string& key, const std::string& value)
    {
        if (!m_initialized)
        {
            return;
        }

        // Update Crashpad runtime annotations (mapped/dynamic keys only). The
        // static process annotations live in m_annotations, set once in
        // initialize() and snapshotted by StartHandler; dynamic keys are carried
        // solely by their live crashpad::Annotation, so they must not also be
        // written to m_annotations (that duplicated them as stale, usually-empty
        // entries in the dump's "Simple Annotations").
        bool matched = false;
        for (const auto& mapping : g_annotationMappings)
        {
            if (key == mapping.name)
            {
                // Copy value to the annotation buffer
                size_t len = std::min(value.length(), mapping.buffer_size - 1);
                memcpy(mapping.buffer, value.c_str(), len);
                mapping.buffer[len] = '\0';

                // Update the Crashpad annotation size so it gets included in crash reports
                mapping.annotation->SetSize(len + 1); // +1 for NUL terminator
                matched = true;
                break;
            }
        }

#ifndef NDEBUG
        // An unmapped key is silently dropped from live crash reports (only mapped keys are
        // delivered to Crashpad). Surface it in debug builds so this never regresses.
        if (!matched)
        {
            std::cerr << "WARNING: CrashHandler::addAnnotation unmapped key '" << key << "' will not appear in crash reports" << std::endl;
        }
#endif
    }

    void CRASH_HANDLER_IMPL_CLASS::attachLogFile(const std::string& logFilePath)
    {
        if (!m_initialized)
        {
            return;
        }

        if (!QFile::exists(QString::fromStdString(logFilePath)))
        {
            std::cerr << "WARNING: Log file not found for attachment: " << logFilePath << std::endl;
            return;
        }

        base::FilePath logPath = StringToFilePath(logFilePath);
        m_attachments.push_back(logPath);

#if defined(PLATFORM_DARWIN)
        // On macOS AddAttachment() is not available; restart the handler with all attachments.
        // Do not replace m_client until the new handler is running.
        base::FilePath dbPath = StringToFilePath(m_crashDumpDir);
        auto start_handler = [&](const std::vector<base::FilePath>& attachments)
        {
            auto client = std::make_unique<crashpad::CrashpadClient>();
            const bool ok = client->StartHandler(StringToFilePath(m_handlerPath), // handler path
                                                 dbPath,                          // database path
                                                 dbPath,                          // metrics dir
                                                 "",                              // url (no uploads)
                                                 "",                              // http_proxy
                                                 m_annotations,                   // annotations
                                                 m_arguments,                     // arguments
                                                 true,                            // restartable
                                                 false,                           // asynchronous_start
                                                 attachments);                    // attachments
            if (ok)
            {
                m_client = std::move(client);
            }
            return ok;
        };

        if (!start_handler(m_attachments))
        {
            std::cerr << "WARNING: Failed to restart Crashpad handler with log attachment; keeping crash capture without log attachment: "
                      << logFilePath << std::endl;
            m_attachments.pop_back();
            return;
        }
#else
        m_client->AddAttachment(logPath);
#endif

        std::cout << "INFO: Registered log file for crash dump attachment: " << logFilePath << std::endl;
    }

    //
    // Factory function to create platform-specific implementation
    //
    CrashHandlerImpl* createPlatformCrashHandlerImpl() { return new CRASH_HANDLER_IMPL_CLASS(); }

} // namespace TwkUtil
