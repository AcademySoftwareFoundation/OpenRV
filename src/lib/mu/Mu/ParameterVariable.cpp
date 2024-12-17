//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ParameterVariable.h>

namespace Mu
{
    using namespace std;

    ParameterVariable::ParameterVariable(Context* context, const char* name,
                                         const Type* storageClass, Attributes a)
        : StackVariable(context, name, storageClass, 0)
        , _hasDefaultValue(false)
        , _value()
    {
        init(a);
    }

    ParameterVariable::ParameterVariable(Context* context, const char* name,
                                         const char* storageClass, Attributes a)
        : StackVariable(context, name, storageClass, 0)
        , _hasDefaultValue(false)
        , _value()
    {
        init(a);
    }

    ParameterVariable::ParameterVariable(Context* context, const char* name,
                                         const Type* storageClass,
                                         const Value& defaultValue,
                                         Attributes a)
        : StackVariable(context, name, storageClass, 0)
        , _hasDefaultValue(true)
        , _value(defaultValue)
    {
        init(a);
    }

    ParameterVariable::ParameterVariable(Context* context, const char* name,
                                         const char* storageClass,
                                         const Value& defaultValue,
                                         Attributes a)
        : StackVariable(context, name, storageClass, 0)
        , _hasDefaultValue(true)
        , _value(defaultValue)
    {
        init(a);
    }

    ParameterVariable::~ParameterVariable() {}

    void ParameterVariable::init(Attributes a)
    {
        _inputOutput = a & InputOutput;
        _output = a & Output;
    }

    void ParameterVariable::output(ostream& o) const
    {
        Variable::output(o);
        o << " (parameter)";
    }

} // namespace Mu
