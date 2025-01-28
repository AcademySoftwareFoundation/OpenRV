//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOmray/IOmray.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOmray*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOmray(); }

    void destroy(TwkFB::IOmray* plug) { delete plug; }

} // extern  "C"
