//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/CrashHandler.h>
// Private, non-installed header: it lives only in this source directory and is
// not exposed on any TwkUtil/ include path, so it is included with quotes.
#include "CrashHandler_internal.h"
#include <TwkUtil/EnvVar.h>
#include <QtCore/QtCore>
#include <array>
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

    // Display name recorded in the "platform" crash annotation. The single
    // unified implementation below works on all platforms, so this only selects
    // the label - there is no need for per-platform class names.
#if defined(PLATFORM_DARWIN)
    constexpr const char* kPlatformName = "macOS";
#elif defined(PLATFORM_WINDOWS)
    constexpr const char* kPlatformName = "Windows";
#else
    constexpr const char* kPlatformName = "Linux";
#endif

    //
    // Runtime Crashpad annotations for dynamic crash metadata
    //
    namespace
    {
        // Buffer size for annotations (max 1024 chars each)
        constexpr size_t kAnnotationBufferSize = 1024;

        // Runtime Crashpad string annotations. crashpad::StringAnnotation owns its
        // own fixed-size storage and clamps overlong values in Set(), so there is
        // no need to manage raw char buffers or copy into them by hand.
        //
        // Mu script annotations (function-level: the Mu crash observer records the
        // function and its file, not a source line - see MuCrashObserver)
        crashpad::StringAnnotation<kAnnotationBufferSize> g_muScriptFile("mu_script_file");
        crashpad::StringAnnotation<kAnnotationBufferSize> g_muFunction("mu_function");

        // Python script annotations
        crashpad::StringAnnotation<kAnnotationBufferSize> g_pyScriptFile("py_script_file");
        crashpad::StringAnnotation<kAnnotationBufferSize> g_pyScriptLine("py_script_line");
        crashpad::StringAnnotation<kAnnotationBufferSize> g_pyFunction("py_function");

        // GPU annotations
        crashpad::StringAnnotation<kAnnotationBufferSize> g_gpuVendor("gpu_vendor");
        crashpad::StringAnnotation<kAnnotationBufferSize> g_gpuRenderer("gpu_renderer");

        // Python interop annotations
        crashpad::StringAnnotation<kAnnotationBufferSize> g_pythonCaller("python_caller");

        // Map of annotation names to their Crashpad annotation objects
        struct AnnotationMapping
        {
            const char* name;
            crashpad::StringAnnotation<kAnnotationBufferSize>* annotation;
        };

        const std::array<AnnotationMapping, 8> g_annotationMappings = {{
            {"mu_script_file", &g_muScriptFile},
            {"mu_function", &g_muFunction},
            {"py_script_file", &g_pyScriptFile},
            {"py_script_line", &g_pyScriptLine},
            {"py_function", &g_pyFunction},
            {"gpu_vendor", &g_gpuVendor},
            {"gpu_renderer", &g_gpuRenderer},
            {"python_caller", &g_pythonCaller},
        }};
    } // namespace

    //
    // Unified platform implementation (Darwin/Windows/Linux)
    //
    class CrashHandlerPlatformImpl : public CrashHandlerImpl
    {
    public:
        CrashHandlerPlatformImpl()
            : m_initialized(false)
        {
        }

        ~CrashHandlerPlatformImpl() override = default;

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

    bool CrashHandlerPlatformImpl::ensureCrashDumpDirectoryExists()
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

    bool CrashHandlerPlatformImpl::initialize(const std::string& appName, const std::string& appVersion, const std::string& handlerPath)
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
        m_annotations["platform"] = kPlatformName;
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

    void CrashHandlerPlatformImpl::addAnnotation(const std::string& key, const std::string& value)
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
                // StringAnnotation owns its storage and clamps overlong values,
                // so we hand it the string directly.
                mapping.annotation->Set(value);
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

    void CrashHandlerPlatformImpl::attachLogFile(const std::string& logFilePath)
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
    CrashHandlerImpl* createPlatformCrashHandlerImpl() { return new CrashHandlerPlatformImpl(); }

} // namespace TwkUtil
