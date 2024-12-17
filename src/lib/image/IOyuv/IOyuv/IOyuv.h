//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __YUVPLUGIN_H__
#define __YUVPLUGIN_H__

#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <stdio.h>

namespace TwkFB
{

    class IOyuv : public FrameBufferIO
    {
    public:
        IOyuv();
        virtual ~IOyuv();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        //     virtual void writeImage(const FrameBuffer& img,
        //                             const std::string& filename,
        //                             const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;
    };

} // namespace TwkFB

#endif // End #ifdef __YUVPLUGIN_H__
