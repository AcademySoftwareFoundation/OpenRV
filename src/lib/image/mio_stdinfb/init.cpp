//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MovieStdinFB/MovieStdinFB.h>
#include <iostream>

using namespace TwkFB;

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkMovie::MovieIO* create();
    __declspec(dllexport) void destroy(TwkMovie::MovieStdinFBIO*);
#endif

    TwkMovie::MovieIO* create() { return new TwkMovie::MovieStdinFBIO(); }

    void destroy(TwkMovie::MovieStdinFBIO* plug) { delete plug; }

} // extern  "C"
