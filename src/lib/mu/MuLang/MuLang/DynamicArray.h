#ifndef __MuLang__DynamicArray__h__
#define __MuLang__DynamicArray__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/ClassInstance.h>
#include <MuLang/DynamicArrayType.h>
#include <vector>

namespace Mu
{

    //
    //  class DynamicArray
    //
    //  Used by ArrayType and VaryingTypeModifier to implement their
    //  objects.  The array is implemented in a fashion not unlike a
    //  C-style dynamic array -- by keeping track of type sizes. This is
    //  the only dependance the array has on its element type.
    //

    class DynamicArray : public ClassInstance
    {
    public:
        typedef unsigned char byte;
        typedef APIAllocatable::STLVector<size_t>::Type SizeVector;

        DynamicArray(const Class*, size_t dimension);
        DynamicArray(const Class*, const SizeVector& dimensions);
        DynamicArray(Thread&, const char*);
        ~DynamicArray();

        //
        //	The array type object which "owns" this dynamic array
        //

        const DynamicArrayType* arrayType() const
        {
            return static_cast<const DynamicArrayType*>(type());
        }

        const Type* elementType() const { return arrayType()->elementType(); }

        //
        //	Dimensions are in number of elements
        //

        const SizeVector& dimensions() const { return _sizes; }

        //
        //	These return the number of elements, not the size in bytes
        //

        size_t size() const
        {
            return _dataSize / elementType()->machineRep()->size();
        }

        size_t size(size_t dimension) const { return _sizes[dimension]; }

        //
        //	The first three resize functions are for native use. The last
        //	is the one that should be called by code that is using the
        //	DynamicArray as its object type.
        //
        //	Note: these functions will throw if the array is incompatable
        //	with the number of dimensions given.
        //

        void resize(size_t size);
        void resize(size_t size0, size_t size1);
        void resize(size_t size0, size_t size1, size_t size2);
        void resize(const SizeVector& sizes);

        //
        //  Erasing from the interior of the array
        //

        void erase(size_t index, size_t n = 1);

        //
        //	Clears all data in the array. Same as std::vector::clear();
        //

        void clear();

        //
        //	Pointer to the data in specified type
        //

        template <typename T> T* data() { return (T*)(_data); }

        template <typename T> const T* data() const
        {
            return (const T*)(_data);
        }

        //
        //  STL-like access (using C pointers as iterators)
        //

        template <typename T> T* begin() { return data<T>(); }

        template <typename T> T* end() { return begin<T>() + size(); }

        template <typename T> const T* begin() const { return data<T>(); }

        template <typename T> const T* end() const
        {
            return begin<T>() + size();
        }

        //
        //	Access to individual elements. You should access them in
        //	accordance with the proper type.
        //

        template <typename T> T& element(size_t i) { return data<T>()[i]; }

        template <typename T> T& element(size_t i, size_t j)
        {
            return data<T>()[i * size(0) + j];
        }

        template <typename T> T& element(size_t i, size_t j, size_t k)
        {
            return data<T>()[i * size(0) * size(1) + j * size(1) + k];
        }

        template <typename T> const T& element(size_t i) const
        {
            return data<T>()[i];
        }

        template <typename T> const T& element(size_t i, size_t j) const
        {
            return data<T>()[i * size(0) + j];
        }

        template <typename T>
        const T& element(size_t i, size_t j, size_t k) const
        {
            return data<T>()[i * size(0) * size(1) + j * size(1) + k];
        }

        //
        //	For references to elements, you can use these
        //

        byte* elementPointer(int i)
        {
            return &_data[i * arrayType()->elementRep()->size()];
        }

        byte* elementPointer(int i, int j)
        {
            size_t s = arrayType()->elementRep()->size();
            return &_data[s * (i * size(0) + j)];
        }

        byte* elementPointer(int i, int j, int k)
        {
            size_t s = arrayType()->elementRep()->size();
            return &_data[s * (i * size(0) * size(1) + j * size(1) + k)];
        }

        const byte* elementPointer(int i) const
        {
            return &_data[i * arrayType()->elementRep()->size()];
        }

        const byte* elementPointer(int i, int j) const
        {
            size_t s = arrayType()->elementRep()->size();
            return &_data[s * (i * size(0) + j)];
        }

        const byte* elementPointer(int i, int j, int k) const
        {
            size_t s = arrayType()->elementRep()->size();
            return &_data[s * (i * size(0) * size(1) + j * size(1) + k)];
        }

    protected:
        void resizeData(size_t);

    protected:
        DynamicArray();
        SizeVector _sizes;
        size_t _allocSize;
        size_t _dataSize;
        byte* _data;
    };

} // namespace Mu

#endif // __MuLang__DynamicArray__h__
