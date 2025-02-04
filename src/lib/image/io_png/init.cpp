//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOpng/IOpng.h>
#include <iostream>

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOpng*);
#endif

    TwkFB::FrameBufferIO* create() { return new TwkFB::IOpng(); }

    void destroy(TwkFB::IOpng* plug) { delete plug; }

} // extern  "C"
