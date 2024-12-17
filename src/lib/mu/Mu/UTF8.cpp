//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/utf8_v2/checked.h>
#include <Mu/utf8_v2/unchecked.h>
#include <Mu/UTF8.h>

namespace Mu
{
    using namespace std;
    using namespace utf8;

    size_t UTF8len(const char* s)
    {
        size_t size = 0;
        for (; *s; unchecked::next(s))
            size++;
        return size;
    }

    UTF32Char UTF8convert(const char* s, int& nbytes)
    {
        const size_t l = internal::sequence_length(s);
        nbytes = l;
        UTF32Char c;
        utf8to32(s, s + l, &c);
        return c;
    }

    void UTF8tokenize(STLVector<String>::Type& tokens, const String& str,
                      String delimiters)
    {
        String::size_type lastPos = str.find_first_not_of(delimiters, 0);
        String::size_type pos = str.find_first_of(delimiters, lastPos);

        while (pos != String::npos || lastPos != String::npos)
        {
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

    void UTF8tokenize(APIAllocatable::STLVector<String>::Type& tokens,
                      const String& str, String delimiters)
    {
        String::size_type lastPos = str.find_first_not_of(delimiters, 0);
        String::size_type pos = str.find_first_of(delimiters, lastPos);

        while (pos != String::npos || lastPos != String::npos)
        {
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

    UTF16String UTF16convert(const Mu::String& s)
    {
        const char* c = s.c_str();
        int n = s.size();
        int i = 0;
        UTF16String u;

        while (i < n)
        {
            int nc;
            u.push_back(UTF16String::value_type(UTF8convert(c + i, nc)));
            i += nc;
        }

        return u;
    }

} // namespace Mu
