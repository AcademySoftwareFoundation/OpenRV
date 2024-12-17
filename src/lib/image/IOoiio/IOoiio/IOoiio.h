//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOoiio__IOoiio__h__
#define __IOoiio__IOoiio__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <fstream>

namespace TwkFB
{

    /// OpenImageIO reader

    class IOoiio : public FrameBufferIO
    {
    public:
        IOoiio();
        virtual ~IOoiio();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;

        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;
    };

} // namespace TwkFB

#endif // __IOoiio__IOoiio__h__
