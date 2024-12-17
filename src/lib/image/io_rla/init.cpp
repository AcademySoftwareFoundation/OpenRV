//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOrla/IOrla.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOrla*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOrla(); }

    void destroy(TwkFB::IOrla* plug) { delete plug; }

} // extern  "C"
