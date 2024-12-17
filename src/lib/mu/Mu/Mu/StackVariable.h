#ifndef __Mu__StackVariable__h__
#define __Mu__StackVariable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Variable.h>

namespace Mu
{

    //
    //  class StackVariable
    //
    //  Variables which live on the stack and are dynamically allocated
    //  during evaluation.
    //

    class StackVariable : public Variable
    {
    public:
        StackVariable(Context* context, const char* name,
                      const Type* storageClass, int stackPos,
                      Attributes a = ReadWrite);

        StackVariable(Context* context, const char* name,
                      const char* storageClass, int stackPos,
                      Attributes a = ReadWrite);

        virtual ~StackVariable();

        //
        //	For this symbol, this can return either storageClass() or
        //	storageClass()->referenceType() depending on what the _node
        //	func is.
        //

        virtual const Type* nodeReturnType(const Node* n) const;

        //
        //	More Symbol API
        //

        virtual void output(std::ostream& o) const;
        virtual void outputNode(std::ostream&, const Node*) const;
        virtual String mangledName() const;
    };

} // namespace Mu

#endif // __Mu__StackVariable__h__
