//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IOexr/FileStreamIStream.h>
#include <TwkExc/Exception.h>
#include <string.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkExc;

    FileStreamIStream::FileStreamIStream(const std::string& filename, Type type,
                                         size_t chunk, int maxAsync)
        : IStream(filename.c_str())
        , m_stream(filename, type, chunk, maxAsync)
        , m_index(0)
    {
    }

    FileStreamIStream::~FileStreamIStream() {}

    bool FileStreamIStream::isMemoryMapped() const { return true; }

    bool FileStreamIStream::read(char c[/*n*/], int n)
    {
        const char* p = ((char*)m_stream.data()) + m_index;
        const size_t size = m_stream.size();

        if (m_index + n >= size)
            n = size - m_index;

        if (n <= 0)
            TWK_THROW_STREAM(Exception,
                             "Incomplete EXR file: " << size << " bytes");

        memcpy(c, p, size_t(n));
        m_index += n;

        return m_index < m_stream.size();
    }

    char* FileStreamIStream::readMemoryMapped(int n)
    {
        char* p = ((char*)m_stream.data()) + m_index;
        const size_t size = m_stream.size();

        if (m_index + n > size)
        {
            TWK_THROW_STREAM(Exception, "Past end of FileStreamIStream");
        }

        m_index += n;
        return p;
    }

    uint64_t FileStreamIStream::tellg() { return uint64_t(m_index); }

    void FileStreamIStream::seekg(uint64_t pos) { m_index = size_t(pos); }

    void FileStreamIStream::clear()
    {
        // m_index = 0;
    }

} // namespace TwkFB
