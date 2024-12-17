//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <TwkApp/OutputPlugins.h>
#include <TwkApp/OutputPlugin.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <vector>

namespace
{
    std::vector<std::string> getPluginFileList(const std::string& pathVar)
    {
        std::string pluginPath;

        if (getenv(pathVar.c_str()))
        {
            pluginPath = getenv(pathVar.c_str());
        }
        else
        {
            pluginPath = ".";
        }

#ifdef PLATFORM_LINUX
        return TwkUtil::findInPath(".*\\.so", pluginPath);
#endif

#ifdef PLATFORM_WINDOWS
        return TwkUtil::findInPath(".*\\.dll", pluginPath);
#endif

#ifdef PLATFORM_APPLE_MACH_BSD
        return TwkUtil::findInPath(".*\\.dylib", pluginPath);
#endif
    }
} // namespace

namespace TwkApp
{

    bool OutputPlugins::m_loadedAll = false;
    OutputPlugins::Plugins* OutputPlugins::m_plugins = nullptr;

    OutputPlugins::OutputPlugins() {}

    OutputPlugins::~OutputPlugins() { unloadPlugins(); }

    void OutputPlugins::loadPlugins(const std::string& envVar)
    {
        if (!m_loadedAll)
        {
            std::vector<std::string> pluginFiles = getPluginFileList(envVar);

            if (pluginFiles.empty())
            {
                std::cout << "INFO: no output plugins found in "
                          << getenv(envVar.c_str()) << std::endl;
            }

            for (int i = 0; i < pluginFiles.size(); i++)
            {
                std::cout << "INFO: loading plugin " << pluginFiles[i]
                          << std::endl;

                if (TwkApp::OutputPluginPtr plugin =
                        TwkApp::OutputPlugin::loadPlugin(pluginFiles[i]))
                {
                    addPlugin(plugin);
                }
            }

            m_loadedAll = true;
        }
    }

    void OutputPlugins::unloadPlugins()
    {
        if (m_plugins)
        {
            m_plugins->clear();
            delete m_plugins;
            m_plugins = nullptr;
        }
    }

    OutputPlugins::Plugins& OutputPlugins::plugins()
    {
        if (!m_plugins)
            m_plugins = new Plugins;
        return *m_plugins;
    }

    void OutputPlugins::addPlugin(const TwkApp::OutputPluginPtr& plugin)
    {
        plugins().insert(plugin);
    }

} // namespace TwkApp
