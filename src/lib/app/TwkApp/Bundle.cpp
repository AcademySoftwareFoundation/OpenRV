//******************************************************************************
// Copyright (c) 2007 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <TwkApp/Bundle.h>
#include <sstream>
#include <boost/filesystem.hpp>
#include <TwkUtil/File.h>

namespace TwkApp {
using namespace std;
using namespace boost;

Bundle* Bundle::m_mainBundle = 0;

Bundle::Bundle(const std::string& appName,
               size_t major_version,
               size_t minor_version,
               size_t revision_number,
               bool sandboxed,
               bool inheritedSandbox)
    : m_applicationName(appName),
      m_majorVersion(major_version),
      m_minorVersion(minor_version),
      m_revisionNumber(revision_number),
      m_sandboxed(sandboxed),
      m_inheritedSandbox(inheritedSandbox)
{
    ostringstream str;
    str << m_majorVersion;
    m_majorVersionDir = str.str();
    str << "." << m_minorVersion;
    m_versionDir = str.str();

    if (!m_mainBundle) m_mainBundle = this;
}

Bundle::~Bundle() 
{
    if (m_mainBundle == this) m_mainBundle = 0;
}

namespace {

}

void 
Bundle::rescanCache(const string& cacheName)
{
}

bool 
Bundle::hasCacheItem(const string& cacheName, const CacheItemName& item)
{
    Path cachePath = cacheItemPath(cacheName, item);
    return cachePath != "" && TwkUtil::fileExists(cachePath.c_str());
}

Bundle::Path 
Bundle::cacheItemPath(const string& cacheName, const CacheItemName& item)
{
    boost::filesystem::path cachePath(userCacheDir());
    cachePath /= cacheName;
    cachePath /= item;
    return cachePath.string();
}

void 
Bundle::addCacheItem(const string& cacheName, const CacheItemName& item, size_t size)
{
    Path cachePath = cacheItemPath(cacheName, item);
}

Bundle::FileAccessPermission
Bundle::permissionForFileAccess(const Path&, bool readonly) const
{
    return FileAccessPermission();
}

Bundle::AccessObject
Bundle::beginFileAccessWithPermission(const FileAccessPermission&) const
{
    return AccessObject(0);
}

void
Bundle::endFileAccessWithPermission(AccessObject) const
{
}

string
Bundle::getEnvVar(const EnvVar& name, const std::string& defaultValue)
{
    if (const char* v = getenv(name.c_str()))
    {
        return v;
    }
    else
    {
        return defaultValue;
    }
}

Bundle::Path
Bundle::userHome()
{
#if defined(PLATFORM_APPLE_MACH_BSD) || defined(PLATFORM_LINUX)
    return getEnvVar("HOME", "/");
#endif

#if defined(PLATFORM_WINDOWS)
    string udrive = getEnvVar("HOMEDRIVE", "c:");
    string upath  = getEnvVar("HOMEPATH", "/");
    return udrive + upath;
#endif
}

Bundle::Path
Bundle::userMovies()
{
    return userHome() + "/Movies";
}

Bundle::Path
Bundle::userMusic()
{
    return userHome() + "/Music";
}

Bundle::Path
Bundle::userPictures()
{
    return userHome() + "/Pictures";
}

} // TwkApp
