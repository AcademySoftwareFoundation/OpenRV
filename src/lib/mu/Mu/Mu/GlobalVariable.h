#ifndef __Mu__GlobalVariable__h__
#define __Mu__GlobalVariable__h__
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
    //  class GlobalVariable
    //
    //  Variables which live in the global storage area of the Process. A
    //  GlobalVariable may include a chunck of initialization code.
    //

    class GlobalVariable : public Variable
    {
    public:
        GlobalVariable(Context* context, const char* name,
                       const Type* storageClass, int offset,
                       Attributes a = Readable, Node* initializer = 0);

        virtual ~GlobalVariable();

        //
        //	For this symbol, this can return either storageClass() or
        //	storageClass()->referenceType() depending on what the _node
        //	func is.
        //

        virtual const Type* nodeReturnType(const Node* n) const;

        virtual void output(std::ostream& o) const;
        virtual void outputNode(std::ostream&, const Node*) const;

    protected:
        Node* _initializer;
    };

} // namespace Mu

#endif // __Mu__GlobalVariable__h__
