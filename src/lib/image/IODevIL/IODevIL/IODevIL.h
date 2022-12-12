//******************************************************************************
// Copyright (c) 2008 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __IODevIL__IODevIL__h__
#define __IODevIL__IODevIL__h__
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <string>
#include <map>
#include <mutex>

namespace TwkFB {

/// Read/Write images using the DevIL (actually just the IL part) library

///
/// DevIL is an open source (LGPL) library which reads and writes a
/// number of image file formats. This reader/writer uses only the
/// portion of the library called "IL" which does the basic I/O and
/// nothing else. (The rest of the library is for GL texture binding,
/// or basic image processing, or other stuff we don't need).
///

class IODevIL : public FrameBufferIO
{
public:
    //
    //  Types
    //

    typedef std::map<unsigned int, const char*> ErrorMap;

    struct TargaHeader
    {
	unsigned char  IDLen;
        unsigned char  pad[1 + 1 + 5];
	unsigned short width;
	unsigned short height;
	unsigned char  bpp;
    };

    //
    //  Constructors
    //

    IODevIL();
    virtual ~IODevIL();
    

    virtual void readImage(FrameBuffer& fb,
                           const std::string& filename,
                           const ReadRequest& request) const;
    virtual void writeImage(const FrameBuffer& img, 
                            const std::string& filename,
                            const WriteRequest& request) const;
    virtual std::string about() const;
    virtual void getImageInfo(const std::string& filename, FBInfo&) const;

protected:
    typedef std::mutex                                  Mutex;
    typedef std::lock_guard<Mutex>                      ScopedLock;

private:
    const char* errorString(unsigned int) const;

private:
    ErrorMap m_errorMap;
    static Mutex m_devILmutex;
};

} // TwkFB

#endif // __IODevIL__IODevIL__h__
