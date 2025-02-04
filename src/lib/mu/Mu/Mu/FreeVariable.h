#ifndef __Mu__FreeVariable__h__
#define __Mu__FreeVariable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/ParameterVariable.h>
#include <Mu/Value.h>

namespace Mu
{

    class FreeVariable : public ParameterVariable
    {
    public:
        FreeVariable(Context*, const char* name, const Type* storageClass);

        FreeVariable(Context*, const char* name, const char* storageClass);

        FreeVariable(Context*, const char* name, const Type* storageClass,
                     const Value& defaultValue);

        FreeVariable(Context*, const char* name, const char* storageClass,
                     const Value& defaultValue);

        virtual ~FreeVariable();

        virtual void output(std::ostream& o) const;
    };

} // namespace Mu

#endif // __Mu__FreeVariable__h__
