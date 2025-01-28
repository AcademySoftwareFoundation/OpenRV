//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOrla__IOrla__h__
#define __IOrla__IOrla__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <stdio.h>

namespace TwkFB
{

    class IOrla : public FrameBufferIO
    {
    public:
        IOrla();
        virtual ~IOrla();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        //     virtual void writeImage(const FrameBuffer& img,
        //                             const std::string& filename,
        //                             const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        void throwError(const std::string& filename, FILE*) const;

        mutable bool m_error;
    };

} // namespace TwkFB

#endif // __IOrla__IOrla__h__
