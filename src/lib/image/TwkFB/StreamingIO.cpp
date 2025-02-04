//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkFB/StreamingIO.h>

namespace TwkFB
{
    using namespace std;

    StreamingFrameBufferIO::~StreamingFrameBufferIO() {}

    int StreamingFrameBufferIO::getIntAttribute(const std::string& name) const
    {
        if (name == "iosize")
            return m_iosize;
        else if (name == "iomaxAsync")
            return m_iomaxAsync;
        else if (name == "iotype")
            return m_iotype;
        return 0;
    }

    void StreamingFrameBufferIO::setIntAttribute(const std::string& name,
                                                 int value)
    {
        if (name == "iosize")
            m_iosize = value;
        else if (name == "iomaxAsync")
            m_iomaxAsync = value;
        else if (name == "iotype")
            m_iotype = (IOType)value;
    }

} // namespace TwkFB
