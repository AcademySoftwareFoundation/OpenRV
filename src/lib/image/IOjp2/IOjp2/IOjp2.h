//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __IOjp2__IOjp2__h__
#define __IOjp2__IOjp2__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <stdio.h>
extern "C" {
#include <openjpeg.h>
}

namespace TwkFB {

class IOjp2 : public FrameBufferIO
{
public:
    IOjp2();
    virtual ~IOjp2();
    
    virtual void readImage(FrameBuffer& fb,
                           const std::string& filename,
                           const ReadRequest& request) const;
//     virtual void writeImage(const FrameBuffer& img, 
//                             const std::string& filename,
//                             const WriteRequest& request) const;
    virtual std::string about() const;
    virtual void getImageInfo(const std::string& filename, FBInfo&) const;

    void throwIfError(const std::string& filename) const;

    mutable bool m_error;

private:
    
    // mutable opj_event_mgr_t m_eventMgr;
};

} // TwkFB

#endif // __IOjp2__IOjp2__h__

