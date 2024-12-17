#ifndef __Mu__UTF8__h__
#define __Mu__UTF8__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdlib.h>
#include <Mu/config.h>
#include <Mu/GarbageCollector.h>

namespace Mu
{

    //
    //  Return the length (num characters) of a UTF-8 string
    //  Assumes a proper UTF-8 string
    //

    size_t UTF8len(const char* s);

    //
    //  Convert the UTF-8 possibly multibyte character at c
    //  into a UTF-32 encoding
    //
    //  Assumes a proper UTF-8 string
    //

    UTF32Char UTF8convert(const char* c, int& n);

    //
    //  Tokenize a string into a buffer
    //

    void UTF8tokenize(STLVector<Mu::String>::Type& tokens,
                      const Mu::String& str, Mu::String delimiters = " ");

    void UTF8tokenize(APIAllocatable::STLVector<Mu::String>::Type& tokens,
                      const Mu::String& str, Mu::String delimiters = " ");

    //
    //  Convert a UTF-8 string in to a UTF32 string
    //

    Mu::UTF16String UTF16convert(const Mu::String& s);

} // namespace Mu

#endif // __Mu__UTF8__h__
