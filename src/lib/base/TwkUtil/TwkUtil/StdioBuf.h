//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__StdioBuf__h__
#define __TwkUtil__StdioBuf__h__
#include <iostream>
#include <stdio.h>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    class TWKUTIL_EXPORT StdioBuf : public std::streambuf
    {
    public:
        StdioBuf(FILE*);

    protected:
        virtual int overflow(int c = EOF);
        virtual int underflow();
        virtual int uflow();
        virtual int pbackfail(int c = EOF);
        virtual int sync();
        virtual std::streampos
        seekoff(std::streamoff off, std::ios_base::seekdir way,
                std::ios_base::openmode which = std::ios_base::in
                                                | std::ios_base::out);
        virtual std::streampos
        seekpos(std::streampos sp,
                std::ios_base::openmode which = std::ios_base::in
                                                | std::ios_base::out);

    private:
        FILE* m_file;
    };

} // namespace TwkUtil

#endif // __TwkUtil__StdioBuf__h__
