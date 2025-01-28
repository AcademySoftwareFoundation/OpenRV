//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IONSImage/IONSImage.h>
#include <iostream>

extern "C"
{

    TwkFB::FrameBufferIO* create()
    {
        TwkFB::IONSImage::useLocalMemoryPool();
        return new TwkFB::IONSImage();
    }

    void destroy(TwkFB::IONSImage* plug) { delete plug; }

} // extern  "C"
