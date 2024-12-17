//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOz__IOz__h__
#define __IOz__IOz__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <stdio.h>

namespace TwkFB
{

    class IOz : public FrameBufferIO
    {
    public:
        IOz(bool normalizeOnInput = true);
        virtual ~IOz();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

    private:
        bool m_normalizeOnInput;
    };

} // namespace TwkFB

#endif // __IOz__IOz__h__
