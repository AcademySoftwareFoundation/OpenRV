#ifndef __Mu__PrimitiveType__h__
#define __Mu__PrimitiveType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Type.h>

namespace Mu
{

    //
    //  class PrimitiveType
    //
    //  A Primitive type is different from other types in that its value
    //  can be stored directly in a Value -- i.e. it does not require a
    //  Object in order to store the value. (As opposed to Class which is
    //  also a type and which must store its value in a ClassInstance).
    //

    class PrimitiveType : public Type
    {
    public:
        PrimitiveType(Context* context, const char* name, const MachineRep*);
        virtual ~PrimitiveType();

        //
        //	Return a new Object which will hold a primitive type (this
        //	will actually return a PrimitiveObject).
        //

        virtual Object* newObject() const = 0;

        //
        //  The object size is the size of the primitive.
        //

        virtual size_t objectSize() const;

        //
        //  Sets the memory to 0
        //

        virtual void constructInstance(Pointer) const;
        virtual void copyInstance(Pointer, Pointer) const;
    };

} // namespace Mu

#endif // __Mu__PrimitiveType__h__
