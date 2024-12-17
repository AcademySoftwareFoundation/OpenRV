//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <memory>
#include <string>

namespace TwkApp
{

    //
    //  Prototypes for the output plugin interface
    //  ------------------------------------------
    //
    //  If you implement an output plugin, you will need to have a function
    //  called "output_module_create" which is extern "C" and has the signature:
    //
    //      extern "C" TwkApp::VideoModule* output_module_create(float
    //      output_plugin_version, int output_module_index);
    //
    //  You will also need a function "output_module_destroy" of type:
    //
    //      extern "C" void output_module_destroy(TwkApp::VideoModule*);
    //
    //

#define OUTPUT_PLUGIN_VERSION 1.0

    class VideoModule;
    typedef TwkApp::VideoModule* output_module_create_t(float, int);
    typedef void output_module_destroy_t(TwkApp::VideoModule*);

    //
    //  OutputPlugin
    //
    //  Output plugin file and functions' entries info
    //

    class OutputPlugin
    {
    public:
        OutputPlugin(const std::string& file, void* handle,
                     output_module_create_t* create_fct,
                     output_module_destroy_t* destroy_fct)
            : m_file(file)
            , m_handle(handle)
            , m_output_module_create(create_fct)
            , m_output_module_destroy(destroy_fct) {};
        virtual ~OutputPlugin();

        static std::shared_ptr<TwkApp::OutputPlugin>
        loadPlugin(const std::string& pathToPlugin);

        void unloadPlugin();

        const std::string& filename() const { return m_file; }

        TwkApp::VideoModule* output_module_create(float output_plugin_version,
                                                  int output_module_index);
        void output_module_destroy(TwkApp::VideoModule* output_module);

    private:
        std::string m_file;
        void* m_handle = nullptr;
        output_module_create_t* m_output_module_create = nullptr;
        output_module_destroy_t* m_output_module_destroy = nullptr;
    };

} // namespace TwkApp
