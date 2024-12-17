//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/DynamicArray.h>
#include <Mu/Exception.h>
#include <Mu/MachineRep.h>
#include <algorithm>
#include <stdarg.h>

namespace Mu
{
    using namespace std;

    DynamicArray::DynamicArray(const Class* c, size_t dimension)
        : ClassInstance(c)
        , _data(0)
        , _allocSize(0)
        , _dataSize(0)
    {
        assert(arrayType()->elementRep());
        for (int i = 0; i < dimension; i++)
            _sizes.push_back(0);
    }

    DynamicArray::DynamicArray(const Class* c, const SizeVector& dimensions)
        : ClassInstance(c)
        , _data(0)
        , _allocSize(0)
        , _dataSize(0)
    {
        assert(arrayType()->elementRep());
        _sizes = dimensions;
        size_t s = 1;
        for (int i = 0; i < _sizes.size(); i++)
            s *= _sizes[i];
        const MachineRep* rep = arrayType()->elementRep();
        resizeData(s * rep->size());
    }

    DynamicArray::DynamicArray(Thread& thread, const char* className)
        : ClassInstance(thread, className)
        , _data(0)
        , _allocSize(0)
        , _dataSize(0)
    {
        _sizes.resize(arrayType()->dimensions());
        fill(_sizes.begin(), _sizes.end(), 0);
    }

    DynamicArray::~DynamicArray() {}

    void DynamicArray::resizeData(size_t n)
    {
        if (n < _allocSize)
        {
            if (n > _dataSize)
            {
                memset(_data + _dataSize, 0, n - _dataSize);
            }

            _dataSize = n;
        }
        else if (n < _allocSize * 2)
        {
            _allocSize *= 2;
            byte* old = _data;
            bool atomic = elementType()->machineRep() != PointerRep::rep();
            _data =
                (byte*)(atomic ? MU_GC_ALLOC_ATOMIC_IGNORE_OFF_PAGE(_allocSize)
                               : MU_GC_ALLOC_IGNORE_OFF_PAGE(_allocSize));
            //_data = (byte*)MU_GC_MALLOC(_allocSize);

            if (old && _dataSize)
            {
                memcpy(_data, old, _dataSize);
                memset(_data + _dataSize, 0, _allocSize - _dataSize);
            }

            _dataSize = n;
        }
        else
        {
            _allocSize = n;
            byte* old = _data;
            bool atomic = elementType()->machineRep() != PointerRep::rep();
            _data =
                (byte*)(atomic ? MU_GC_ALLOC_ATOMIC_IGNORE_OFF_PAGE(_allocSize)
                               : MU_GC_ALLOC_IGNORE_OFF_PAGE(_allocSize));
            //_data = (byte*)MU_GC_MALLOC(_allocSize);

            if (old && _dataSize)
            {
                memcpy(_data, old, _dataSize);
                memset(_data + _dataSize, 0, _allocSize - _dataSize);
            }

            _dataSize = n;
        }
    }

    void DynamicArray::resize(size_t size0)
    {
        if (_sizes.size() != 1)
        {
            throw BadInternalArrayCallException(this);
        }

        const MachineRep* rep = arrayType()->elementRep();
        _sizes.front() = size0;
        resizeData(size0 * rep->size());
    }

    void DynamicArray::resize(size_t size0, size_t size1)
    {
        if (_sizes.size() != 2)
        {
            throw BadInternalArrayCallException(this);
        }

        const MachineRep* rep = arrayType()->elementRep();
        _sizes[0] = size0;
        _sizes[1] = size1;
        resizeData(size0 * size1 * rep->size());
    }

    void DynamicArray::resize(size_t size0, size_t size1, size_t size2)
    {
        if (_sizes.size() != 3)
        {
            throw BadInternalArrayCallException(this);
        }

        const MachineRep* rep = arrayType()->elementRep();
        _sizes[0] = size0;
        _sizes[1] = size1;
        _sizes[2] = size2;
        resizeData(size0 * size1 * size2 * rep->size());
    }

    void DynamicArray::resize(const SizeVector& sizes)
    {
        if (_sizes.size() != sizes.size())
        {
            throw BadInternalArrayCallException(this);
        }

        const MachineRep* rep = arrayType()->elementRep();
        size_t ns = 1;

        for (size_t i = 0; i < sizes.size(); i++)
            ns *= sizes[i];

        resizeData(ns * rep->size());
        _sizes = sizes;
    }

    void DynamicArray::clear()
    {
        // GC_free(_data);
        _data = 0;
        _dataSize = 0;
        _allocSize = 0;
        for (size_t i = 0; i < _sizes.size(); i++)
            _sizes[i] = 0;
    }

    void DynamicArray::erase(size_t start, size_t n)
    {
        size_t esize = elementType()->machineRep()->size();
        memmove(_data + start * esize, _data + (start + n) * esize,
                _dataSize - (start + n) * esize);
        _dataSize -= n * esize;
    }

} // namespace Mu
