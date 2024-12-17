//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/TypePattern.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/FixedArrayType.h>
#include <Mu/OpaqueType.h>

namespace Mu
{
    using namespace std;

    Type::MatchResult MatchDynamicArray::match(const Type* other,
                                               Bindings&) const
    {
        return dynamic_cast<const DynamicArrayType*>(other) != 0 ? Match
                                                                 : NoMatch;
    }

    Type::MatchResult MatchFixedArray::match(const Type* other, Bindings&) const
    {
        return dynamic_cast<const FixedArrayType*>(other) != 0 ? Match
                                                               : NoMatch;
    }

} // namespace Mu
