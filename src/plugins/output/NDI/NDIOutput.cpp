//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <NDI/NDIModule.h>

extern "C"
{
#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkApp::VideoModule*
    output_module_create(float output_plugin_version, int output_module_index);
    __declspec(dllexport) void output_module_destroy(TwkApp::VideoModule*);
#endif

    TwkApp::VideoModule* output_module_create(float output_plugin_version,
                                              int output_module_index)
    {
        try
        {
            if (output_module_index == 0)
            {
                return new NDI::NDIModule();
            }
        }
        catch (...)
        {
        }

        return nullptr;
    }

    void output_module_destroy(TwkApp::VideoModule* output_module)
    {
        delete output_module;
    }

} // extern  "C"
