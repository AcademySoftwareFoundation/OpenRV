//
//  Copyright (c) 2009 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __IOhtj2k__IOhtj2k__h__
#define __IOhtj2k__IOhtj2k__h__
#include <iostream>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/StreamingIO.h>
#include <fstream>
#include <pthread.h>

namespace TwkFB {

class IOhtj2k : public StreamingFrameBufferIO
{
public:

    IOhtj2k(IOType ioMethod = StandardIO,
            size_t chunkSize = 61440,
            int maxAsync = 16);

    virtual ~IOhtj2k();
    

    virtual void readImage(FrameBuffer& fb,
                           const std::string& filename,
                           const ReadRequest& request) const;

    virtual std::string about() const;
    virtual void getImageInfo(const std::string& filename, FBInfo&) const;
};



} // TwkFB

#endif // __IOhtj2k__IOhtj2k__h__
