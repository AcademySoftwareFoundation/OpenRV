//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/StdioBuf.h>

namespace TwkUtil
{

    StdioBuf::StdioBuf(FILE* f)
        : std::streambuf()
        , m_file(f)
    {
    }

    int StdioBuf::overflow(int c) { return c != EOF ? fputc(c, m_file) : EOF; }

    int StdioBuf::underflow()
    {
        int c = getc(m_file);
        if (c != EOF)
            ungetc(c, m_file);
        return c;
    }

    int StdioBuf::uflow() { return getc(m_file); }

    int StdioBuf::pbackfail(int c)
    {
        return c != EOF ? ungetc(c, m_file) : EOF;
    }

    int StdioBuf::sync() { return fflush(m_file); }

    std::streampos StdioBuf::seekpos(std::streampos sp,
                                     std::ios_base::openmode which)
    {
        return seekoff(sp, std::ios_base::beg, which);
    }

    std::streampos StdioBuf::seekoff(std::streamoff off,
                                     std::ios_base::seekdir way,
                                     std::ios_base::openmode which)
    {
        int whence;

        switch (way)
        {
        default:
        case std::ios_base::beg:
            whence = SEEK_SET;
            break;
        case std::ios_base::cur:
            whence = SEEK_CUR;
            break;
        case std::ios_base::end:
            whence = SEEK_END;
            break;
        }

        if (fseek(m_file, off, whence))
        {
            return -1;
        }
        else
        {
            return ftell(m_file);
        }
    }

} // namespace TwkUtil
