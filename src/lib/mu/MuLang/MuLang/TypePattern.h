#ifndef __MuLang__TypePattern__h__
#define __MuLang__TypePattern__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/TypePattern.h>

namespace Mu
{

    class MatchDynamicArray : public TypePattern
    {
    public:
        MatchDynamicArray(Context* c)
            : TypePattern(c, "?dyn_array")
        {
        }

        virtual MatchResult match(const Type*, Bindings&) const;
    };

    class MatchFixedArray : public TypePattern
    {
    public:
        MatchFixedArray(Context* c)
            : TypePattern(c, "?fixed_array")
        {
        }

        virtual MatchResult match(const Type*, Bindings&) const;
    };

} // namespace Mu

#endif // __MuLang__TypePattern__h__
