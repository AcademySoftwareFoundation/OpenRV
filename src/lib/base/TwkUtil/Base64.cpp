//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkUtil/Base64.h>
#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <sstream>

namespace TwkUtil
{
    using namespace std;
    using namespace boost;

    string base64Encode(const char* data, size_t size)
    {
        namespace bai = boost::archive::iterators;
        stringstream os;

        typedef bai::base64_from_binary<bai::transform_width<const char*, 6, 8>>
            base64_enc;

        copy(base64_enc(data), base64_enc(data + size),
             ostream_iterator<char>(os));

        return os.str();
    }

    string id64Encode(const char* data, size_t size, char t62, char t63,
                      char pad)
    {
        string x = base64Encode(data, size);

        for (size_t i = 0, s = x.size(); i < s; i++)
        {
            if (x[i] == '/')
                x[i] = t63;
            else if (x[i] == '+')
                x[i] = t62;
            else if (x[i] == '=')
                x[i] = pad;
        }

        return x;
    }

    void base64Decode(const char* c, size_t size, vector<char>& data)
    {
        data.clear();
        namespace bai = boost::archive::iterators;
        typedef bai::transform_width<bai::binary_from_base64<const char*>, 8, 6>
            base64_dec;

        copy(base64_dec(c), base64_dec(c + size), back_inserter(data));
    }

    void id64Decode(const char* c, size_t size, vector<char>& data, char t62,
                    char t63, char pad)
    {
        vector<char> buffer(size);
        copy(c, c + size, buffer.begin());

        for (size_t i = 0; i < size; i++)
        {
            if (buffer[i] == t63)
                buffer[i] = '/';
            else if (buffer[i] == t62)
                buffer[i] = '+';
            else if (buffer[i] == pad)
                buffer[i] = '=';
        }

        base64Decode(&buffer[0], size, data);
    }

} // namespace TwkUtil
