//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOpng__IOpng__h__
#define __IOpng__IOpng__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <stdio.h>

namespace TwkFB
{

    class IOpng : public FrameBufferIO
    {
    public:
        IOpng();
        virtual ~IOpng();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        bool m_error;
    };

} // namespace TwkFB

#endif // __IOpng__IOpng__h__
