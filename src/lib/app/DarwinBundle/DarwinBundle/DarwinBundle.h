//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __DarwinBundle__DarwinBundle__h__
#define __DarwinBundle__DarwinBundle__h__
#include <TwkApp/Bundle.h>

namespace TwkApp
{

    //
    //  DarwinBundle implements the TwkApp::Bundle API for an NSApp. It
    //  basically calls the underlying NSBundle functions.
    //
    //  This class will also create and AutoRelease pool. So if you make
    //  this on the stack in main it will provide ObjC memory management
    //  for you.
    //
    //  If the bundle is created as sandboxed the security bookmark
    //  functions will return actual values.
    //

    struct BundlePaths;

    class DarwinBundle : public Bundle
    {
    public:
        DarwinBundle(const FileName& AppName, size_t majorVersion,
                     size_t minorVersion, size_t revisionNumber,
                     bool protocolHandler = false, bool sandboxed = false,
                     bool inheritedSandbox = false);

        virtual ~DarwinBundle();

        //
        //  DarwinBundle API
        //

        PathVector fileInSupportPath(const FileName& name);

        //
        //  Resource path, as in NSBundle call
        //

        Path resourcePath();

        Path builtInPluginPath();

        //
        //  This is a helper function which gets rid of the -psn argument
        //  that the open command adds to the argument list. nargv
        //  contains the filtered arguments.
        //

        static void removePSN(int& argc, char* argv[],
                              std::vector<char*>& nargv);

        //
        //  Bundle API
        //

        virtual Path userHome();
        virtual Path userMovies();
        virtual Path userMusic();
        virtual Path userPictures();
        virtual Path top();
        virtual Path executableFile(const FileName& name);
        virtual Path application(const FileName& name);
        virtual PathVector supportPath();
        virtual Path rcfile(const FileName& rcfileName, const FileName& type,
                            const EnvVar& rcenv);
        virtual Path userPluginPath(const DirName& pluginType);
        virtual PathVector pluginPath(const DirName& pluginType);
        virtual PathVector scriptPath(const DirName& scriptType);
        virtual Path appPluginPath(const DirName& pluginType);
        virtual Path appScriptPath(const DirName& scriptType);
        virtual Path userCacheDir();
        virtual Path resource(const FileName& name, const FileName& type);
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

        //
        //  Register this executable as the default rvlink protocol
        //  handler.
        //

        void registerHandler();

    private:
        BundlePaths* m_bundle;
        std::string m_userSupportPath;
    };

} // namespace TwkApp

#endif // __DarwinBundle__DarwinBundle__h__
