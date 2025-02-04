//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOz/IOz.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOz*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOz(); }

    void destroy(TwkFB::IOz* plug) { delete plug; }

} // extern  "C"
