//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOsgi/IOsgi.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOsgi*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOsgi(); }

    void destroy(TwkFB::IOsgi* plug) { delete plug; }

} // extern  "C"
