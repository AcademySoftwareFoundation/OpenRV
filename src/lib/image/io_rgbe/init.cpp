//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOrgbe/IOrgbe.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOrgbe*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOrgbe(); }

    void destroy(TwkFB::IOrgbe* plug) { delete plug; }

} // extern  "C"
