//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOtiff__IOtiff__h__
#define __IOtiff__IOtiff__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/StreamingIO.h>

namespace TwkFB
{

    class IOtiff : public StreamingFrameBufferIO
    {
    public:
        IOtiff(bool addAlphaTo3Channel = false, bool useRGBPlanar = false,
               IOType ioMethod = StandardIO, size_t chunkSize = 61440,
               int maxAsync = 16);

        virtual ~IOtiff();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;

        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        virtual bool getBoolAttribute(const std::string& name) const;
        virtual void setBoolAttribute(const std::string& name, bool value);

    private:
        bool m_addAlphaTo3Channel;
        bool m_rgbPlanar;
    };

} // namespace TwkFB

#endif // __IOtiff__IOtiff__h__
