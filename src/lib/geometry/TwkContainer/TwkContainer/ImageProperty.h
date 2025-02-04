//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkContainer__ImageProperty__h__
#define __TwkContainer__ImageProperty__h__
#include <TwkContainer/Property.h>
#include <iostream>

namespace TwkContainer
{

    //
    //  ImageProperty holds a multi-dimensional image buffer. The buffer
    //  can be resize and a pointer to the buffer can be returned. There
    //  is no typed interface to the image.
    //

    class ImageProperty : public Property
    {
    public:
        typedef unsigned char byte;

        ImageProperty(const std::string& name, Layout);
        ImageProperty(Info* i, const std::string& name, Layout);
        virtual ~ImageProperty();

        size_t sizeOfDimension(size_t dim) const;
        void restructure(size_t s0, size_t s1, size_t s2, size_t s3,
                         size_t num = 1);
        void clear();

        size_t sizeInBytes() const;

        template <typename T> T* data() { return reinterpret_cast<T*>(m_data); }

        template <typename T> const T* data() const
        {
            return reinterpret_cast<T*>(m_data);
        }

        virtual Layout layoutTrait() const;
        virtual size_t xsizeTrait() const;
        virtual size_t ysizeTrait() const;
        virtual size_t zsizeTrait() const;
        virtual size_t wsizeTrait() const;
        virtual void swap(size_t a, size_t b);
        virtual size_t size() const;
        virtual bool empty() const;
        virtual void resize(size_t);
        virtual void erase(size_t start, size_t num);
        virtual void eraseUnsorted(size_t start, size_t num);
        virtual size_t sizeofElement() const;
        virtual void insertDefaultValue(size_t index, size_t len = 1);
        virtual void clearToDefaultValue();
        virtual Property* copy(const char* newName = 0) const;
        virtual Property* copyNoData() const;
        virtual void copy(const Property*);
        virtual void copyRange(const Property*, size_t begin, size_t end);
        virtual void concatenate(const Property*);
        virtual void* rawData();
        virtual const void* rawData() const;
        virtual bool equalityCompare(const Property*) const;
        virtual bool structureCompare(const Property*) const;
        virtual std::string valueAsString() const;

    private:
        Layout m_layout;
        size_t m_sizes[4];
        size_t m_num;
        byte* m_data;
    };

} // namespace TwkContainer

#endif // __TwkContainer__ImageProperty__h__
