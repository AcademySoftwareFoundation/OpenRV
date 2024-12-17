//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Environment.h>
#include <Mu/UTF8.h>

namespace Mu
{
    namespace Environment
    {
        using namespace std;

        struct EnvData
        {
            MU_GC_NEW_DELETE
            String _home;
            SearchPath _modulePath;
            ProgramArguments _programArgs;
        };

        static EnvData* env = 0;

        static void init()
        {
            if (env)
                return;
            env = new EnvData;

            const char* mp = getenv("MU_MODULE_PATH");
            const char* mh = getenv("MU_HOME");

#ifdef WIN32
            String modulePath = mp ? mp : ".";
            env->_home = mh ? mh : "";
            modulePath += ";" + env->_home;

            UTF8tokenize(env->_modulePath, modulePath, ";");
#else
            String modulePath = mp ? mp : ".";
            env->_home = mh ? mh : "";
            modulePath += ":" + env->_home;

            UTF8tokenize(env->_modulePath, modulePath, ":");
#endif
        }

        void pathComponents(const String& path, PathComponents& components)
        {
            UTF8tokenize(components, path, "/");
        }

        void setModulePath(const SearchPath& paths)
        {
            if (!env)
                init();
            env->_modulePath = paths;
        }

        void setProgramArguments(const ProgramArguments& args)
        {
            if (!env)
                init();
            env->_programArgs = args;
        }

        const String& muHomeLocation()
        {
            if (!env)
                init();
            return env->_home;
        }

        const SearchPath& modulePath()
        {
            if (!env)
                init();
            return env->_modulePath;
        }

        const ProgramArguments& programArguments()
        {
            if (!env)
                init();
            return env->_programArgs;
        }

    } // namespace Environment
} // namespace Mu
