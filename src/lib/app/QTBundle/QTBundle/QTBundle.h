//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __QTBundle__QTBundle__h__
#define __QTBundle__QTBundle__h__
#include <TwkApp/Bundle.h>
#include <QtCore/QtCore>

namespace TwkApp
{

    //
    //  QTBundle implements the TwkApp::Bundle API using QT functions.
    //  The Bundle layout on disk should look like this:
    //
    //  root/
    //      bin/
    //          the_app_binary
    //          auxillary_binary
    //      lib/
    //          libs_for_plugins.a
    //          some_dso.so             (could be .dylib)
    //      plugins/
    //      plugins/
    //          TwkFB/                  (for example)
    //          TwkMovie/               (for example)
    //      scripts/
    //          rv/                     (for example)
    //      doc/
    //          manual.pdf              (for example)
    //      html/
    //          index.html              (for example)
    //      etc/
    //          license.gto
    //      ui/
    //          preferences.ui
    //

    class QTBundle : public Bundle
    {
    public:
        QTBundle(const FileName& AppName, size_t major_version,
                 size_t minor_version, size_t revision_number,
                 bool unused = false, bool sandboxed = false,
                 bool inheritedSandbox = false);
        virtual ~QTBundle();

        //
        //  Need to call this if no other functions are called.
        //

        void initializeAfterQApplication();

        //
        //  Bundle API
        //

        virtual Path top();
        virtual Path executableFile(const FileName& name);
        virtual Path application(const FileName& name);
        virtual Path rcfile(const FileName& rcfileName, const FileName& type,
                            const EnvVar& rcenv);
        virtual Path userPluginPath(const DirName& pluginType);
        virtual PathVector pluginPath(const DirName& pluginType);
        virtual PathVector scriptPath(const DirName& scriptType);
        virtual Path appPluginPath(const DirName& pluginType);
        virtual Path appScriptPath(const DirName& pluginType);
        virtual Path userCacheDir();
        virtual Path resource(const FileName& name, const FileName& type);
        virtual PathVector supportPath();
        virtual PathVector licenseFiles(const FileName& licFileName,
                                        const FileName& type);
        virtual Path defaultLicense(const FileName& licFileName,
                                    const FileName& type);
        virtual void setEnvVar(const EnvVar& name, const Path& value,
                               bool force = false);
        virtual void addPathToEnvVar(const EnvVar& name,
                                     const PathVector& value);
        virtual void addPathToEnvVar(const EnvVar& name, const Path& value,
                                     bool toHead = false);
        virtual FileName crashLogFile();
        virtual Path crashLogDirectory();

        virtual FileAccessPermission
        permissionForFileAccess(const Path&, bool readonly) const;
        virtual AccessObject
        beginFileAccessWithPermission(const FileAccessPermission&) const;
        virtual void endFileAccessWithPermission(AccessObject) const;

    private:
        void init();

    private:
        bool m_initialized;
        QString m_appName;
        QDir m_bin;
        QDir m_home;
        QDir m_root;
        QDir m_scripts;
        QDir m_plugins;
        QDir m_doc;
        QDir m_html;
        QDir m_etc;
        QDir m_ui;
        QDir m_profiles;
        QDir m_homeSupport;
        QDir m_pyhome;
        QList<QDir> m_supportPath;
    };

} // namespace TwkApp

#endif // __QTBundle__QTBundle__h__
