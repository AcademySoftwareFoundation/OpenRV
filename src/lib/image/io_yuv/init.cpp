//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOyuv/IOyuv.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOyuv*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOyuv(); }

    void destroy(TwkFB::IOyuv* plug) { delete plug; }

} // extern  "C"
