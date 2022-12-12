//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#include <IODevIL/IODevIL.h>
#include <iostream>

extern "C" {

#ifdef PLATFORM_WINDOWS
__declspec(dllexport) TwkFB::FrameBufferIO* create();
__declspec(dllexport) void destroy(TwkFB::IODevIL*);
#endif

TwkFB::FrameBufferIO* 
create()
{
    return new TwkFB::IODevIL();
}

void 
destroy(TwkFB::IODevIL* plug)
{
    delete plug;
}

} // extern  "C"
