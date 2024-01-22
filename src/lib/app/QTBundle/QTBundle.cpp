//******************************************************************************
// Copyright (c) 2007 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <QTBundle/QTBundle.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stl_ext/string_algo.h>

#ifdef WIN32
#define SEP ";"
#else
#define SEP ":"
#endif

namespace TwkApp {
using namespace std;

QTBundle::QTBundle(const FileName& appName,
                   size_t majv,
                   size_t minv,
                   size_t revn,
                   bool unused,
                   bool sandboxed, 
                   bool inheritedSandbox) 
    : Bundle(appName, majv, minv, revn, sandboxed, inheritedSandbox),
      m_appName(appName.c_str()),
      m_initialized(false)
{
    ostringstream str;
    str << majv << "." << minv << "." << revn;
    setEnvVar("TWK_APP_VERSION", str.str().c_str(), true);
}

QTBundle::~QTBundle()
{
}

void
QTBundle::initializeAfterQApplication()
{
    if (!m_initialized) init();
}

void
QTBundle::init()
{
    m_initialized = true;
    m_bin = QCoreApplication::applicationDirPath();
    m_home = QDir::homePath();
    m_root = m_bin;
    m_root.cdUp();
    m_scripts = m_root;
    m_plugins = m_root;
    m_doc = m_root;
    m_html = m_root;
    m_etc = m_root;
    m_ui = m_root;
    if (!m_scripts.cd("scripts")) cerr << "WARNING: missing scripts dir in bundle" << endl;
    if (!m_plugins.cd("plugins")) cerr << "WARNING: missing plugin dir in bundle" << endl;

    // Note: we do not issue a warning here for a potentially missing 'etc' dir since 
    // it is optional and is not populated on all platforms like on Linux for example.
    m_etc.cd("etc");

#ifdef PLATFORM_LINUX
    m_homeSupport = m_home;

    if (!m_homeSupport.cd(".rv"))
    {
        m_homeSupport = m_home;
        m_homeSupport.mkpath(".rv");
        m_homeSupport.cd(".rv");
    }
#endif

#ifdef PLATFORM_WINDOWS
    m_homeSupport = getenv("APPDATA");

    if (!m_homeSupport.cd("RV"))
    {
        m_homeSupport = getenv("APPDATA");
        m_homeSupport.mkpath("RV");
        m_homeSupport.cd("RV");
    }
#endif

    m_pyhome = m_root;
    m_pyhome.cd("lib");
    m_pyhome.cd("python" PYTHON_VERSION);
    bool setPythonHome = !(getenv("PYTHONHOME") && getenv("RV_ALLOW_SITE_PYTHONHOME"));
    if (setPythonHome) setEnvVar("PYTHONHOME", m_root.absolutePath().toUtf8().constData(), true);

    bool forceToFront = (!getenv("RV_PYTHONPATH_APPEND_ONLY"));

    addPathToEnvVar("PYTHONPATH", m_pyhome.absoluteFilePath("lib-dynload").toUtf8().constData(), forceToFront);
    addPathToEnvVar("PYTHONPATH", m_pyhome.absolutePath().toUtf8().constData(), forceToFront);

    if (const char* c = getenv("RV_SUPPORT_PATH"))
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, c, SEP);

        for (size_t i=0; i < tokens.size(); i++)
        {
            m_supportPath.push_back(QDir(tokens[i].c_str()));
        }

        //
        //  Add the plugins area if its not in there. Otherwise the app
        //  might not start.
        //

        if (!m_supportPath.contains(m_plugins))
        {
            m_supportPath.push_back(m_plugins);
        }
    }
    else
    {
        m_supportPath.push_back(m_homeSupport);
        m_supportPath.push_back(m_plugins);
    }

    for (size_t i = 0; i < m_supportPath.size(); i++)
    {
        QDir& dir = m_supportPath[i];
        //  cerr << "supp path " << i << ": '" << dir.absolutePath().toUtf8().constData() << "'" << endl;

        if (dir.exists())
        {
            //  cerr << "    exists" << endl;
            QDir profiles(dir.absoluteFilePath("Profiles"));

            if (!profiles.exists())
            {
                static const char* dirs[] = {"Packages", "ImageFormats", "MovieFormats",
                                             "Mu", "SupportFiles", "ConfigFiles", "lib", 
                                             "Python", "OIIO", "Nodes", "Profiles",
                                             "Output", "MediaLibrary",
                                             NULL};

                for (size_t q = 0; dirs[q]; q++)
                {
                    QDir d(dir.absoluteFilePath(dirs[q]));
                    if (!d.exists()) dir.mkdir(dirs[q]);
                }
            }

            if (dir.exists("Profiles")) 
            {
                addPathToEnvVar("TWK_PROFILE_PLUGIN_PATH", 
                                dir.absoluteFilePath("Profiles").toUtf8().constData());
            }

            if (dir.exists("ImageFormats")) 
            {
                addPathToEnvVar("TWK_FB_PLUGIN_PATH", 
                                dir.absoluteFilePath("ImageFormats").toUtf8().constData());
            }

            if (dir.exists("Nodes")) 
            {
                addPathToEnvVar("TWK_NODE_PLUGIN_PATH", 
                                dir.absoluteFilePath("Nodes").toUtf8().constData());
            }

            if (dir.exists("OIIO")) 
            {
                addPathToEnvVar("OIIO_LIBRARY_PATH", 
                                dir.absoluteFilePath("OIIO").toUtf8().constData());
            }

            if (dir.exists("Output"))
            {
                addPathToEnvVar("TWK_OUTPUT_PLUGIN_PATH", 
                                dir.absoluteFilePath("Output").toUtf8().constData());
            }

            if (dir.exists("MovieFormats"))
            {
                addPathToEnvVar("TWK_MOVIE_PLUGIN_PATH", 
                                dir.absoluteFilePath("MovieFormats").toUtf8().constData());
            }

            if (dir.exists("Mu"))
            {
                //  cerr << "    Mu subdir exists" << endl;
                addPathToEnvVar("MU_MODULE_PATH",
                                dir.absoluteFilePath("Mu").toUtf8().constData());
            }

            if (dir.exists("Python"))
            {
                //  cerr << "    Mu subdir exists" << endl;
                addPathToEnvVar("PYTHONPATH",
                                dir.absoluteFilePath("Python").toUtf8().constData());
            }

            if (dir.exists("MediaLibrary"))
            {
                addPathToEnvVar("TWK_MEDIA_LIBRARY_PLUGIN_PATH",
                                 dir.absoluteFilePath("MediaLibrary").toUtf8().constData());
                addPathToEnvVar("PYTHONPATH",
                                 dir.absoluteFilePath("MediaLibrary").toUtf8().constData());
            }
        }
        else
        {
            m_supportPath.erase(m_supportPath.begin() + i); 
            i--;
        }
    }

    addPathToEnvVar("TWK_APP_SUPPORT_PATH", supportPath());

    //
    //  On windows, need to add the bin directory to the path so that io/mio
    //  plugins, which load from subdirectories of the plugins dir, will find
    //  additional dlls (like jpeg.dll) in the bin directory.
    //
    #ifdef PLATFORM_WINDOWS
	addPathToEnvVar("PATH", m_bin.absolutePath().toUtf8().constData(), true);
    #endif
}

Bundle::Path 
QTBundle::top()
{
    if (!m_initialized) init();
    return Path(m_root.absolutePath().toUtf8().data());
}

Bundle::Path 
QTBundle::resource(const FileName& name, const FileName& type)
{
    if (!m_initialized) init();
    QString fname(name.c_str());
    fname += ".";
    fname += QString(type.c_str());

    if (type == "pdf")
    {
        if (m_doc.exists())
        {

            QFileInfo file(m_doc.absoluteFilePath(fname));

            if (file.exists() && file.isReadable())
            {
                return Path(file.absoluteFilePath().toUtf8().data());
            }
        }
    }
    else if (type == "html" || type == "xhtml")
    {
        if (m_html.exists())
        {
            QFileInfo file(m_html.absoluteFilePath(fname));

            if (file.exists() && file.isReadable())
            {
                return Path(file.absoluteFilePath().toUtf8().data());
            }
        }
    }
    else if (type == "ui")
    {
        if (m_ui.exists())
        {
            QFileInfo file(m_ui.absoluteFilePath(fname));

            if (file.exists() && file.isReadable())
            {
                return Path(file.absoluteFilePath().toUtf8().data());
            }
        }
    }

    //
    //  Should add images, etc
    //

    return Path();
}

Bundle::Path
QTBundle::executableFile(const FileName& name)
{
    if (!m_initialized) init();
    string n = name;
#ifdef WIN32
    n += ".exe";
#endif
    QFileInfo file(m_bin.absoluteFilePath(n.c_str()));

    if (file.exists() && file.isExecutable()) 
    {
        return file.absoluteFilePath().toUtf8().data();
    }
    else
    {
        return Path();
    }
}

Bundle::Path
QTBundle::application(const FileName& name)
{
    if (!m_initialized) init();
    //
    //  With tweak bundle, there are no other applications, just
    //  executables.
    //

    return executableFile(name);
}

Bundle::Path 
QTBundle::defaultLicense(const FileName& licFileName,
                         const FileName& type)
{
    if (!m_initialized) init();
    QString lic(licFileName.c_str());
    lic += ".";
    lic += QString(type.c_str());
    return string(m_etc.absoluteFilePath(lic).toUtf8().data());
}

Bundle::PathVector
QTBundle::licenseFiles(const FileName& licFileName,
                       const FileName& type)
{
    if (!m_initialized) init();
    PathVector paths;

    QList<QDir> searchPath = m_supportPath;
    searchPath.push_back(m_etc);

    for (size_t i = 0; i < searchPath.size(); i++)
    {
        if (searchPath[i].exists())
        {
            QString lic(licFileName.c_str());
            lic += ".";
            lic += QString(type.c_str());

            QFileInfo file(searchPath[i].absoluteFilePath(lic));

            if (file.exists() && file.isReadable())
            {
                paths.push_back(file.absoluteFilePath().toUtf8().data());
            }
        }
    }

    Path dpath = defaultLicense(licFileName, type);

    if (find(paths.begin(), paths.end(), dpath) == paths.end())
    {
        paths.push_back(dpath);
    }

    if (paths.empty())
    {
        cerr << "WARNING: no license files found" << endl;
    }

    return paths;
}

Bundle::Path 
QTBundle::rcfile(const FileName& rcfileName, 
                 const FileName& type,
                 const EnvVar& rcenv)
{
    if (!m_initialized) init();

    if (const char* e = getenv(rcenv.c_str()))
    {
        return Path(e);
    }
    else
    {
        QString base = rcfileName.c_str();
        base += ".";
        base += type.c_str();
        QString dot = ".";
        dot += base;

        QFileInfo homeInit(m_home.absoluteFilePath(dot));

        if (homeInit.exists() && homeInit.isReadable())
        {
            return Path(homeInit.absoluteFilePath().toUtf8().data());
        }
        else
        {
            QDir scriptsDir(m_scripts);

            if (scriptsDir.cd(m_appName))
            {
                QFileInfo initInfo(scriptsDir.absoluteFilePath(base));
                
                if (initInfo.exists() && initInfo.isReadable())
                {
                    return initInfo.absoluteFilePath().toUtf8().data();
                }
            }
        }
    }

    return Path();
}

Bundle::PathVector 
QTBundle::supportPath()
{
    PathVector paths;
    //paths.push_back(m_homeSupport.absolutePath().toUtf8().data());
    for (size_t i = 0; i < m_supportPath.size(); i++)
    {
        paths.push_back(m_supportPath[i].absolutePath().toUtf8().constData());
    }

    return paths;
}

Bundle::PathVector 
QTBundle::pluginPath(const DirName& pluginType)
{
    if (!m_initialized) init();

    PathVector paths;

    for (size_t i = 0; i < m_supportPath.size(); i++)
    {
        paths.push_back(m_supportPath[i].absoluteFilePath(pluginType.c_str()).toUtf8().constData());
    }

    return paths;
}

Bundle::PathVector 
QTBundle::scriptPath(const DirName& scriptType)
{
    if (!m_initialized) init();
    PathVector paths;
    QDir dir(m_scripts);
    dir.cd(scriptType.c_str());

    if (dir.exists())
    {
        paths.push_back(dir.absolutePath().toUtf8().data());
    }

    QDir homeScripts(m_homeSupport);
    bool homeOK = homeScripts.cd(scriptType.c_str());
    
    if (homeOK && homeScripts.exists())
        paths.push_back(homeScripts.absolutePath().toUtf8().data());

    return paths;
}

QTBundle::Path
QTBundle::appPluginPath(const DirName& pluginType)
{
    if (!m_initialized) init();
    Path p = m_plugins.absolutePath().toUtf8().constData();
    if (p[p.size()-1] != '/') p += "/";
    p += pluginType;
    return p;
}

QTBundle::Path 
QTBundle::appScriptPath(const DirName& pluginType)
{
    if (!m_initialized) init();
    Path p = m_scripts.absolutePath().toUtf8().constData();
    if (p[p.size()-1] != '/') p += "/";
    p += pluginType;
    return p;
}

QTBundle::Path 
QTBundle::userCacheDir()
{
    QString s = m_homeSupport.absoluteFilePath("Caches");
    return s.toUtf8().constData();
}

void
QTBundle::setEnvVar(const EnvVar& var, const Path& value, bool force)
{
    if (!getenv(var.c_str()) || force)
    {
#ifdef WIN32
        ostringstream es;
        es << var << "=" << value;
        //
        //  "::putenv" stopped working on windows, no idea why it
        //  used to work and now doesn't.  Changing to "::_putenv"
        //  makes it work again.
        //
        int ret = ::_putenv(es.str().c_str());
        //  cerr << "putenv " << es.str() << ", returns " << ret << endl;
        //  const char *charVar = getenv(var.c_str());
        //  string strVar = (charVar) ? charVar : "";
        //  cerr << "val is now '" << strVar << "'" << endl;
#else
        setenv(var.c_str(), value.c_str(), 1);
#endif
    }
}

void
QTBundle::addPathToEnvVar(const EnvVar& var, const Path& value, bool toHead)
{
    ostringstream str;
    const char* envvar = getenv(var.c_str());

    if (toHead)
    {
        str << value;
        if (envvar) str << SEP << envvar;
    }
    else
    {
        if (envvar) str << envvar << SEP;
        str << value;
    }
    setEnvVar(var, str.str(), true);
}

void
QTBundle::addPathToEnvVar(const EnvVar& var, const PathVector& value)
{
    ostringstream str;
    const char* envvar = getenv(var.c_str());
    if (envvar) str << envvar;

    for (int i=0; i < value.size(); i++)
    {
        if (i || envvar) str << SEP;
        str << value[i];
    }

    setEnvVar(var, str.str(), true);
}

Bundle::FileName
QTBundle::crashLogFile()
{
    return FileName(".tweak_crash_log");
}

Bundle::Path
QTBundle::crashLogDirectory()
{
    return getenv("HOME");
}

Bundle::Path
QTBundle::userPluginPath(const DirName& pluginType)
{
    QString s = m_homeSupport.absoluteFilePath(pluginType.c_str());
    return s.toUtf8().constData();
}

TwkApp::Bundle::FileAccessPermission 
QTBundle::permissionForFileAccess(const Path&, bool readonly) const
{
    FileAccessPermission permission;
    return permission;
}

TwkApp::Bundle::AccessObject 
QTBundle::beginFileAccessWithPermission(const FileAccessPermission&) const
{
    return AccessObject(0x1);
}

void 
QTBundle::endFileAccessWithPermission(AccessObject) const
{
    //
    //  Nothing yet
    //
}

} // TwkApp
