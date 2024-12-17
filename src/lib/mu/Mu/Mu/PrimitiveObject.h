#ifndef __Mu__PrimitiveObject__h__
#define __Mu__PrimitiveObject__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Object.h>
#include <Mu/Value.h>

namespace Mu
{

    //
    //  class PrimitiveObject
    //
    //  An Object which holds a Value.
    //

    class PrimitiveObject : public Object
    {
    public:
        //
        //	If you use this constructor you must put the object in the
        //	correct memory segment yourself. (So if you want a constant or
        //	static object, this is the constructor you use.)
        //

        explicit PrimitiveObject(const Type* t)
            : Object(t)
            , _value(0)
        {
        }

        //
        //	Return a reference to the Value stored in this Object
        //

        Value& value() { return _value; }

    private:
        Value _value;
    };

} // namespace Mu

#endif // __Mu__PrimitiveObject__h__
