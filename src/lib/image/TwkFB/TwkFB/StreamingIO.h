//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkFB__StreamingIO__h__
#define __TwkFB__StreamingIO__h__
#include <TwkFB/dll_defs.h>
#include <TwkFB/IO.h>
#include <iostream>

namespace TwkFB
{

    //
    //  This class provides an interface to RV in order to talk to FBIO
    //  objects which use TwkUtil::FileStream
    //

    class TWKFB_EXPORT StreamingFrameBufferIO : public FrameBufferIO
    {
    public:
        enum IOType
        {
            StandardIO,
            BufferedIO,
            UnbufferedIO,
            MemoryMappedIO,
            AsyncBufferedIO,
            AsyncUnbufferedIO
        };

        StreamingFrameBufferIO(const std::string& identifier,
                               const std::string& sortKey,
                               IOType type = StandardIO,
                               size_t chunkSize = 61440, int maxAsync = 16)
            : FrameBufferIO(identifier, sortKey)
            , m_iotype(type)
            , m_iosize(chunkSize)
            , m_iomaxAsync(maxAsync)
        {
        }

        virtual ~StreamingFrameBufferIO();

        //
        //  Handles the IOType, size and maxAsync
        //

        virtual int getIntAttribute(const std::string& name) const;
        virtual void setIntAttribute(const std::string& name, int value);

        void iomethod(IOType t) { m_iotype = t; }

        void iosize(size_t t) { m_iosize = t; }

        void iomaxAsync(size_t t) { m_iomaxAsync = t; }

    protected:
        IOType m_iotype;
        size_t m_iosize;
        size_t m_iomaxAsync;
    };

} // namespace TwkFB

#endif // __TwkFB__StreamingIO__h__
