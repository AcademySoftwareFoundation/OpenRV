//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#define CMS_PLUGIN_VERSION 1.0

#include <TwkCMS/ColorManagementSystem.h>
#include <stl_ext/string_algo.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <stdlib.h>

#ifndef _MSC_VER
#include <dlfcn.h>
#endif

namespace TwkCMS
{
    using namespace stl_ext;
    using namespace std;
    using namespace TwkUtil;

    typedef ColorManagementSystem* create_t(float);
    typedef void destroy_t(ColorManagementSystem*);

    ColorManagementSystem::Transform::~Transform() {}

    void ColorManagementSystem::Transform::begin() {}

    void ColorManagementSystem::Transform::end() {}

    //----------------------------------------------------------------------

    ColorManagementSystem::CMSPlugins ColorManagementSystem::m_plugins;

    ColorManagementSystem::Transform::ColorSpace
    ColorManagementSystem::Transform::inputSpace() const
    {
        return RGB709;
    }

    ColorManagementSystem::Transform::ColorSpace
    ColorManagementSystem::Transform::outputSpace() const
    {
        return RGB709;
    }

    ColorManagementSystem::ColorManagementSystem(const char* name)
        : m_name(name)
    {
    }

    ColorManagementSystem::~ColorManagementSystem() {}

    void ColorManagementSystem::addTransform(Transform* t)
    {
        m_transforms.push_back(t);
    }

    ColorManagementSystem::Transform*
    ColorManagementSystem::findTransform(const string& name) const
    {
        for (int i = 0; i < m_transforms.size(); i++)
        {
            if (m_transforms[i]->name() == name)
            {
                return m_transforms[i];
            }
        }

        return 0;
    }

    void ColorManagementSystem::splitPath(vector<string>& tokens,
                                          const string& str)
    {
        tokenize(tokens, str, ":");
    }

    //
    //  Plugin management
    //

    static vector<string> getPluginFileList(const string& pathVar)
    {
        string pluginPath;

        if (getenv(pathVar.c_str()))
        {
            pluginPath = getenv(pathVar.c_str());
        }
        else if (getenv("BUILD_ROOT"))
        {
            pluginPath = getenv("BUILD_ROOT");
            pluginPath += "/plugins/TwkCMS";
        }
        else
        {
            pluginPath = "/usr/local/lib:/usr/lib:/lib";
        }

        return findInPath("TwkCMS.+so", pluginPath);
    }

    void ColorManagementSystem::addPlugin(ColorManagementSystem* cms)
    {
        m_plugins.push_back(cms);
    }

    ColorManagementSystem*
    ColorManagementSystem::findPlugin(const std::string& name)
    {
        for (int i = 0; i < m_plugins.size(); i++)
        {
            if (m_plugins[i]->name() == name)
            {
                return m_plugins[i];
            }
        }

        return 0;
    }

    void ColorManagementSystem::loadPlugins(const string& envvar)
    {
#ifndef _MSC_VER
        vector<string> pluginFiles = getPluginFileList(envvar);
        if (pluginFiles.empty())
            return;

        for (int i = 0; i < pluginFiles.size(); i++)
        {
            if (void* handle =
                    dlopen(pluginFiles[i].c_str(), RTLD_NOW | RTLD_GLOBAL))
            {
                create_t* plugCreate = (create_t*)dlsym(handle, "create");
                destroy_t* plugDestroy = (destroy_t*)dlsym(handle, "destroy");

                if (!plugCreate || !plugDestroy)
                {
                    dlclose(handle);

                    cerr << "ERROR: ignoring CMS plugin " << pluginFiles[i]
                         << ": missing create() or destroy(): " << dlerror()
                         << endl;

                    continue;
                }

                try
                {
                    if (ColorManagementSystem* plugin =
                            plugCreate(CMS_PLUGIN_VERSION))
                    {
                        m_plugins.push_back(plugin);
                        cout << "INFO: loaded CMS " << pluginFiles[i] << endl;
                    }
                }
                catch (...)
                {
                    cerr << "WARNING: exception thrown while creating CMS "
                            "plugin "
                         << pluginFiles[i].c_str() << ", ignoring" << endl;

                    dlclose(handle);
                }
            }
            else
            {
                cerr << "ERROR: cannot open CMS plugin " << pluginFiles[i]
                     << ": " << dlerror() << endl;
            }
        }
#endif
    }

} // namespace TwkCMS
