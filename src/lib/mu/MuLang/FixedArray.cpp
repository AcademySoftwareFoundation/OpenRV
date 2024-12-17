//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/FixedArray.h>
#include <algorithm>
#include <numeric>

namespace Mu
{
    using namespace std;

    FixedArray::FixedArray(const FixedArrayType* c, size_t s)
        : ClassInstance(c)
    {
        memset(data<unsigned char>(), 0, c->instanceSize());
    }

    FixedArray::FixedArray(Thread& thread, const char* className)
        : ClassInstance(thread, className)
    {
        const FixedArrayType* a = arrayType();
        memset(data<unsigned char>(), 0, a->instanceSize());
    }

    FixedArray::~FixedArray() {}

    size_t FixedArray::size() const
    {
        const FixedArrayType::SizeVector& sizes = arrayType()->dimensions();
        return accumulate(sizes.begin(), sizes.end(), 1, multiplies<size_t>());
    }

} // namespace Mu
