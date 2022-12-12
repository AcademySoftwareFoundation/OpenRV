//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#include <IOjp2/IOjp2.h>
#include <iostream>

using namespace TwkFB;

extern "C" {

#ifdef PLATFORM_WINDOWS
__declspec(dllexport) TwkFB::FrameBufferIO* create();
__declspec(dllexport) void destroy(IOjp2*);
#endif

TwkFB::FrameBufferIO* 
create()
{
    return new TwkFB::IOjp2();
}

void 
destroy(TwkFB::IOjp2* plug)
{
    delete plug;
}

} // extern  "C"
