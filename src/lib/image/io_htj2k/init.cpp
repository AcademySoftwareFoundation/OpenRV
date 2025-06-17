//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

#include <IOhtj2k/IOhtj2k.h>
#include <iostream>


using namespace TwkFB;
using namespace std;

extern "C" {

#ifdef PLATFORM_WINDOWS
__declspec(dllexport) TwkFB::FrameBufferIO* create();
__declspec(dllexport) void destroy(TwkFB::IOhtj2k*);
#endif

FrameBufferIO* 
create()
{

    return new IOhtj2k(); 
}

void 
destroy(IOhtj2k* plug)
{
    delete plug;
}

} // extern  "C"
