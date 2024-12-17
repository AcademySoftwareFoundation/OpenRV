//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <TwkApp/OutputPlugin.h>
#include <dlfcn.h>
#include <iostream>

namespace
{
    void* load_plugin_fct(const std::string& file, void* handle,
                          const std::string plugin_fct_name)
    {
        void* plugin_fct = dlsym(handle, plugin_fct_name.c_str());
        if (!plugin_fct)
        {
            std::cerr << "ERROR: ignoring Output plugin: " << file
                      << ": missing " << plugin_fct_name << std::endl;
        }

        return plugin_fct;
    }
} // namespace

namespace TwkApp
{

    OutputPlugin::~OutputPlugin() { unloadPlugin(); }

    std::shared_ptr<TwkApp::OutputPlugin>
    OutputPlugin::loadPlugin(const std::string& file)
    {
        if (void* handle = dlopen(file.c_str(), RTLD_LAZY))
        {
            output_module_create_t* plugCreate =
                reinterpret_cast<output_module_create_t*>(
                    load_plugin_fct(file, handle, "output_module_create"));
            output_module_destroy_t* plugDestroy =
                reinterpret_cast<output_module_destroy_t*>(
                    load_plugin_fct(file, handle, "output_module_destroy"));
            if (!plugCreate || !plugDestroy)
            {
                dlclose(handle);
                return nullptr;
            }

            return std::make_shared<TwkApp::OutputPlugin>(
                file, handle, plugCreate, plugDestroy);
        }
        else
        {
            std::cerr << "ERROR: cannot open Output plugin " << file << ": "
                      << dlerror() << std::endl;
        }

        return nullptr;
    }

    void OutputPlugin::unloadPlugin()
    {
        if (m_handle)
        {
            dlclose(m_handle);
            m_handle = nullptr;
        }
    }

    TwkApp::VideoModule*
    OutputPlugin::output_module_create(float output_plugin_version,
                                       int output_module_index)
    {
        TwkApp::VideoModule* outputModule = nullptr;

        if (m_output_module_create)
        {
            outputModule = m_output_module_create(output_plugin_version,
                                                  output_module_index);
        }

        return outputModule;
    }

    void OutputPlugin::output_module_destroy(TwkApp::VideoModule* output_module)
    {
        if (m_output_module_destroy)
        {
            m_output_module_destroy(output_module);
        }
    }

} // namespace TwkApp
