//******************************************************************************
// Copyright (c) 2005-2013 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOraw__IOraw__h__
#define __IOraw__IOraw__h__

#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>

namespace TwkFB
{

    class IOraw : public FrameBufferIO
    {
    public:
        IOraw(int threads, double scale, std::string primaries,
              bool bruteForce);
        virtual ~IOraw();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;

        //    virtual void writeImage(const FrameBuffer& img,
        //                            const std::string& filename,
        //                            const WriteRequest& request) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename,
                                  FBInfo& info) const;

    private:
        int m_threadCount;
        double m_scaleFactor;
        std::string m_primaries;
    };

} // namespace TwkFB

#endif // __IOraw__IOraw__h__
