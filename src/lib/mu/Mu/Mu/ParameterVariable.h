#ifndef __Mu__ParameterVariable__h__
#define __Mu__ParameterVariable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/StackVariable.h>
#include <Mu/Value.h>

namespace Mu
{
    namespace Archive
    {
        class Reader;
    }

    class ParameterVariable : public StackVariable
    {
    public:
        enum ParameterAttribute
        {
            Output = 1 << 10,
            InputOutput = 1 << 11,
        };

        ParameterVariable(Context* context, const char* name,
                          const Type* storageClass, Attributes a = ReadWrite);

        ParameterVariable(Context* context, const char* name,
                          const char* storageClass, Attributes a = ReadWrite);

        ParameterVariable(Context* context, const char* name,
                          const Type* storageClass, const Value& defaultValue,
                          Attributes a = ReadWrite);

        ParameterVariable(Context* context, const char* name,
                          const char* storageClass, const Value& defaultValue,
                          Attributes a = ReadWrite);

        virtual ~ParameterVariable();

        void init(Attributes);

        virtual void output(std::ostream& o) const;

        bool hasDefaultValue() const { return _hasDefaultValue; }

        Value defaultValue() const { return _value; }

    private:
        void setDefaultValue(const Value& v) { _value = v; }

    private:
        Value _value;
        bool _hasDefaultValue : 1;
        bool _output : 1;
        bool _inputOutput : 1;

        friend class Archive::Reader;
    };

} // namespace Mu

#endif // __Mu__ParameterVariable__h__
