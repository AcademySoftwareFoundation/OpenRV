//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Bundle__h__
#define __TwkApp__Bundle__h__
#include <string>
#include <vector>

namespace TwkApp
{

    //
    //  class Bundle
    //
    //  This is an interface to Application files outside of the binary.
    //  An instance of this must be passed to the Application class. This
    //  is usually instantiated as either a DarwinBundle, UnixBundle, or
    //  WindowsBundle depending on the OS.
    //
    //  For our purposes, search paths to plugins, or paths to files are
    //  placed in environment variables. These are then searched by
    //  individual components (like image plugins for example) that know
    //  where and how to look for what they want.
    //

    class Bundle
    {
    public:
        //
        //  These could change to wstring or ustring
        //

        typedef std::string Path;
        typedef std::string FileName;
        typedef std::string CacheItemName;
        typedef std::string DirName;
        typedef std::string EnvVar;
        typedef std::vector<Path> PathVector;
        typedef unsigned char Byte;
        typedef std::string FileAccessPermission;
        typedef void* AccessObject;

        class CacheArena
        {
        public:
            CacheArena(const std::string& name)
                : m_name(name)
            {
            }

            virtual ~CacheArena();

            const std::string& name() const { return m_name; }

        private:
            std::string m_name;
        };

        //
        //
        //

        Bundle(const std::string& appName, size_t major_version,
               size_t minor_version, size_t revision_number,
               bool sandboxed = false, bool inheritedSandbox = false);

        virtual ~Bundle();

        static Bundle* mainBundle() { return m_mainBundle; }

        const std::string& applicationName() const { return m_applicationName; }

        bool isSandboxed() const { return m_sandboxed; }

        bool isInheritedSandbox() const { return m_inheritedSandbox; }

        //
        //  Application root. This is not where the binary lives: its the
        //  topmost directory that holds the whole app.
        //

        virtual Path top() = 0;

        //
        //

        const std::string& majorVersionDir() const { return m_majorVersionDir; }

        const std::string& versionDir() const { return m_versionDir; }

        //
        //  Find a particular binary, or application
        //

        virtual Path executableFile(const FileName& name) = 0;
        virtual Path application(const FileName& name) = 0;

        //
        //  The user support root path (~/Library/Application Support/APP)
        //

        virtual PathVector supportPath() = 0;

        //
        //  Looks for:
        //      rcenv
        //      .rcfileName in home dir
        //      rcfileName in bundle
        //
        //  type is the extension, so .rvrc.mu which could be overriden by
        //  RV_INIT env var would look like:
        //
        //      rcfile("rvrc", "mu", "RV_INIT")
        //

        virtual Path rcfile(const FileName& rcfileName, const FileName& type,
                            const EnvVar& rcenv) = 0;

        //
        //  Find a seach path for types of plugins, or scripts. The app
        //  versions will find the path associated with the application
        //  directory only.
        //

        virtual PathVector pluginPath(const DirName& pluginType) = 0;
        virtual PathVector scriptPath(const DirName& scriptType) = 0;
        virtual Path appPluginPath(const DirName& pluginType) = 0;
        virtual Path appScriptPath(const DirName& pluginType) = 0;
        virtual Path userPluginPath(const DirName& pluginType) = 0;
        virtual Path userCacheDir() = 0;

        //
        //  For example: resource("rv_manual", "pdf");
        //

        virtual Path resource(const FileName& name, const FileName& type) = 0;

        //
        //  Find all the license files in the usual system locations
        //

        virtual PathVector licenseFiles(const FileName& licFileName,
                                        const FileName& type) = 0;

        //
        //  Location of default license (may not exist)
        //

        virtual Path defaultLicense(const FileName& licFileName,
                                    const FileName& type) = 0;

        //
        //  Get/Set environment variables
        //  The version that takes a vector creates a path value
        //  if force is false, and the env var exists, it will not be touched.
        //

        virtual std::string getEnvVar(const EnvVar& name,
                                      const std::string& defaultValue = "");

        virtual void setEnvVar(const EnvVar& name, const Path& value,
                               bool force = false) = 0;

        virtual void addPathToEnvVar(const EnvVar& name,
                                     const PathVector& value) = 0;

        //
        //  User directories. These don't necessarily exist.
        //

        virtual Path userHome();
        virtual Path userMovies();
        virtual Path userMusic();
        virtual Path userPictures();

        //
        //  Crash Log
        //

        virtual FileName crashLogFile() = 0;
        virtual Path crashLogDirectory() = 0;

        //
        //  Cache File Management
        //
        //  This API manages cache files which are allowed to be removed
        //  at any time.
        //
        //  userCacheDir()  is the location of the main cache dir.
        //  rescanCache()   will update internal cache lookups (cache cache)
        //  cacheItemPath() will return a path to the item name IFF it
        //                  already exists in the cache. If not it returns
        //                  a blank path
        //
        //  addCacheItem()  will cause a stat on the file unless you give it
        //                  the size in bytes of the cache item you
        //                  created
        //

        virtual void rescanCache(const std::string& cacheName);
        virtual bool hasCacheItem(const std::string& cacheName,
                                  const CacheItemName&);
        virtual Path cacheItemPath(const std::string& cacheName,
                                   const CacheItemName&);
        virtual void addCacheItem(const std::string& cacheName,
                                  const CacheItemName&, size_t size = 0);

        //
        //  Security API for external filesystem resources
        //  This is chiefly here to support sandboxing
        //

        virtual FileAccessPermission
        permissionForFileAccess(const Path&, bool readonly) const;
        virtual AccessObject
        beginFileAccessWithPermission(const FileAccessPermission&) const;
        virtual void endFileAccessWithPermission(AccessObject) const;

    private:
        std::string m_applicationName;
        size_t m_majorVersion;
        size_t m_minorVersion;
        size_t m_revisionNumber;
        std::string m_majorVersionDir;
        std::string m_versionDir;
        bool m_sandboxed;
        bool m_inheritedSandbox;
        static Bundle* m_mainBundle;
    };

} // namespace TwkApp

#endif // __TwkApp__Bundle__h__
