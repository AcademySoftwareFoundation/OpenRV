//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IONSImage__IONSImage__h__
#define __IONSImage__IONSImage__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>

namespace TwkFB
{

    class IONSImage : public FrameBufferIO
    {
    public:
        IONSImage();
        virtual ~IONSImage();

        static void useLocalMemoryPool();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;
    };

} // namespace TwkFB

#endif // __IONSImage__IONSImage__h__
