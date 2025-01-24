//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IOmray__IOmray__h__
#define __IOmray__IOmray__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <stdio.h>

namespace TwkFB
{

    class IOmray : public FrameBufferIO
    {
    public:
        IOmray();
        virtual ~IOmray();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;
    };

} // namespace TwkFB

#endif // __IOmray__IOmray__h__
