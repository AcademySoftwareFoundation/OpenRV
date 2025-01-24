#ifndef __Mu__FunctionObject__h__
#define __Mu__FunctionObject__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <Mu/ClassInstance.h>

namespace Mu
{
    class Signature;
    class Function;
    class FunctionType;

    //
    //  class FunctionObject
    //
    //  This object holds either a function or a lambda object.
    //

    class FunctionObject : public ClassInstance
    {
    public:
        FunctionObject(const FunctionType*);
        FunctionObject(const Function*);
        FunctionObject(Thread&, const char*);

        void setFunction(const Function* f) { _function = f; }

        const Function* function() const { return _function; }

        void setDependent(FunctionObject* o) { _dependent = o; }

        FunctionObject* dependent() const { return _dependent; }

    protected:
        FunctionObject();
        ~FunctionObject();

    protected:
        const Function* _function;
        FunctionObject* _dependent;

        friend class FunctionType;
    };

} // namespace Mu

#endif // __Mu__FunctionObject__h__
