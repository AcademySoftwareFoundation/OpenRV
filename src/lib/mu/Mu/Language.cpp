//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Class.h>
#include <Mu/Function.h>
#include <Mu/Interface.h>
#include <Mu/Language.h>
#include <Mu/Symbol.h>
#include <Mu/Type.h>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <vector>

namespace Mu
{

    using namespace std;

    Language::Language(const String& name, const String& nameSpaceSeparator)
        : _name(name)
        , _nsSeparator(nameSpaceSeparator)
        , _verbose(false)
    {
    }

    Language::~Language() {}

} // namespace Mu
