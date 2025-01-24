//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <AJADevices/AJAModule.h>
#include <TwkUtil/FourCC.h>

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
                return new AJADevices::AJAModule(
                    0, TwkUtil::FourCC<'R', 'V', 'S', '0'>::value,
                    AJADevices::AJAModule::OperationMode::ProMode);
            }
            else if (output_module_index == 1)
            {
                return new AJADevices::AJAModule(
                    0, TwkUtil::FourCC<'R', 'V', 'S', '1'>::value,
                    AJADevices::AJAModule::OperationMode::SimpleMode);
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
