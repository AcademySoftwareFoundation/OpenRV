//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/FreeVariable.h>

namespace Mu
{
    using namespace std;

    FreeVariable::FreeVariable(Context* context, const char* name,
                               const Type* storageClass)
        : ParameterVariable(context, name, storageClass)
    {
    }

    FreeVariable::FreeVariable(Context* context, const char* name,
                               const char* storageClass)
        : ParameterVariable(context, name, storageClass)
    {
    }

    FreeVariable::FreeVariable(Context* context, const char* name,
                               const Type* storageClass,
                               const Value& defaultValue)
        : ParameterVariable(context, name, storageClass, defaultValue)
    {
    }

    FreeVariable::FreeVariable(Context* context, const char* name,
                               const char* storageClass,
                               const Value& defaultValue)
        : ParameterVariable(context, name, storageClass, defaultValue)
    {
    }

    FreeVariable::~FreeVariable() {}

    void FreeVariable::output(ostream& o) const
    {
        Variable::output(o);
        o << " (free parameter)";
    }

} // namespace Mu
