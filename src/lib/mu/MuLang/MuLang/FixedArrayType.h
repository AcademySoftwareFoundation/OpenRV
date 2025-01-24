#ifndef __MuLang__FixedArrayType__h__
#define __MuLang__FixedArrayType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <vector>
#include <Mu/Type.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/MachineRep.h>

namespace Mu
{

    //
    //  class FixedArrayType
    //
    //  An array class parameterized by type. The array object handles memory
    //  using the std::vector<> template. The memory is guaranteed to be
    //  contiguous (this is in an upcoming ANSI C++ specification). This class
    //  uses the stardard ClassInstance allocation scheme.
    //

    class FixedArrayType : public Class
    {
    public:
        typedef STLVector<size_t>::Type SizeVector;

        FixedArrayType(Context*, const char* name, Class* superClass,
                       const Type* elementType, const SizeVector& dimensions);

        FixedArrayType(Context*, const char* name, Class* superClass,
                       const Type* elementType, const size_t* dimensions,
                       size_t nDimensions);

        ~FixedArrayType();

        virtual MatchResult match(const Type*, Bindings&) const;

        const Type* elementType() const { return _elementType; }

        const MachineRep* elementRep() const
        {
            return elementType()->machineRep();
        }

        const SizeVector& dimensions() const { return _dimensions; }

        //
        //	Fixed size arrays or dynamic
        //

        size_t fixedSize() const { return _fixedSize; }

        //
        //	Symbol API
        //

        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();

        //
        //	Type API
        //

        virtual const Type* fieldType(size_t) const;
        virtual ValuePointer fieldPointer(Object*, size_t) const;
        virtual const ValuePointer fieldPointer(const Object*, size_t) const;

        //
        //  Class API
        //

        virtual void freeze();

        //
        //	Fixed array functions
        //

        static NODE_DECLARATION(fixed_construct, Pointer);
        static NODE_DECLARATION(fixed_construct_aggregate, Pointer);
        static NODE_DECLARATION(fixed_copyconstruct, Pointer);
        static NODE_DECLARATION(fixed_print, void);
        static NODE_DECLARATION(fixed_size, int);
        static NODE_DECLARATION(fixed_index1, Pointer);
        static NODE_DECLARATION(fixed_indexN, Pointer);
        static NODE_DECLARATION(fixed_equals, bool);

    private:
        const Type* _elementType;
        SizeVector _dimensions;
        size_t _fixedSize;
    };

} // namespace Mu

#endif // __MuLang__FixedArrayType__h__
