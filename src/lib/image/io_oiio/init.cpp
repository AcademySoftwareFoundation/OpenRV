//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOoiio/IOoiio.h>
#include <iostream>

using namespace TwkFB;
using namespace std;

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOoiio*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOoiio(); }

    void destroy(TwkFB::IOoiio* plug) { delete plug; }

} // extern  "C"
