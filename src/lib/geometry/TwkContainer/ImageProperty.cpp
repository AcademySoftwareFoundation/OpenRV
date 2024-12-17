//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkContainer/ImageProperty.h>
#include <algorithm>

namespace TwkContainer
{
    using namespace std;

    ImageProperty::ImageProperty(const string& name, Layout l)
        : Property(name)
        , m_num(0)
        , m_data(0)
        , m_layout(l)
    {
        clear();
    }

    ImageProperty::ImageProperty(Info* i, const string& name, Layout l)
        : Property(i, name)
        , m_num(0)
        , m_data(0)
        , m_layout(l)
    {
        clear();
    }

    ImageProperty::~ImageProperty() { clear(); }

    size_t ImageProperty::sizeOfDimension(size_t dim) const
    {
        return m_sizes[dim];
    }

    void ImageProperty::restructure(size_t s0, size_t s1, size_t s2, size_t s3,
                                    size_t num)
    {
        delete[] m_data;
        m_data = 0;
        m_sizes[0] = s0;
        m_sizes[1] = s1;
        m_sizes[2] = s2;
        m_sizes[3] = s3;
        m_num = num;

        size_t nbytes = sizeInBytes();
        if (nbytes > 0)
            m_data = new unsigned char[nbytes];
    }

    size_t ImageProperty::sizeInBytes() const
    {
        size_t total = m_num;
        for (size_t i = 0; i < 4; i++)
            if (m_sizes[i] > 0)
                total *= m_sizes[i];
        return total * sizeofLayout(m_layout);
    }

    void ImageProperty::clear() { restructure(0, 0, 0, 0, 0); }

    Property::Layout ImageProperty::layoutTrait() const { return m_layout; }

    size_t ImageProperty::xsizeTrait() const { return m_sizes[0]; }

    size_t ImageProperty::ysizeTrait() const { return m_sizes[1]; }

    size_t ImageProperty::zsizeTrait() const { return m_sizes[2]; }

    size_t ImageProperty::wsizeTrait() const { return m_sizes[3]; }

    void ImageProperty::swap(size_t a, size_t b)
    {
        size_t esize = sizeofElement();
        byte* p0 = m_data + esize * a;
        byte* p1 = m_data + esize * b;
        byte* e = p0 + esize;
        for (byte* p = p0; p != e; p++, p1++)
            std::swap(*p, *p1);
    }

    size_t ImageProperty::size() const { return m_num; }

    bool ImageProperty::empty() const { return m_num == 0; }

    void ImageProperty::resize(size_t n)
    {
        if (n != m_num)
            restructure(m_sizes[0], m_sizes[1], m_sizes[2], m_sizes[3], n);
    }

    void ImageProperty::erase(size_t start, size_t num) { abort(); }

    void ImageProperty::eraseUnsorted(size_t start, size_t num) { abort(); }

    size_t ImageProperty::sizeofElement() const
    {
        size_t total = 1;
        for (size_t i = 0; i < 4; i++)
            if (m_sizes[i] > 0)
                total *= m_sizes[i];
        return total * sizeofLayout(m_layout);
    }

    void ImageProperty::insertDefaultValue(size_t index, size_t len)
    {
        abort();
    }

    void ImageProperty::clearToDefaultValue() { abort(); }

    Property* ImageProperty::copy(const char* newName) const
    {
        ImageProperty* p =
            new ImageProperty(newName ? newName : name().c_str(), m_layout);
        p->restructure(m_sizes[0], m_sizes[1], m_sizes[2], m_sizes[3], m_num);
        memcpy(p->data<unsigned char>(), m_data, sizeInBytes());
        return p;
    }

    Property* ImageProperty::copyNoData() const
    {
        return new ImageProperty(name().c_str(), m_layout);
    }

    void ImageProperty::copy(const Property* prop)
    {
        if (const ImageProperty* p = dynamic_cast<const ImageProperty*>(prop))
        {
            assert(p->layoutTrait() == m_layout);

            restructure(p->sizeOfDimension(0), p->sizeOfDimension(1),
                        p->sizeOfDimension(2), p->sizeOfDimension(3),
                        p->size());

            memcpy(m_data, p->data<unsigned char>(), sizeInBytes());
        }
    }

    void ImageProperty::copyRange(const Property*, size_t begin, size_t end)
    {
        abort();
    }

    void ImageProperty::concatenate(const Property*) { abort(); }

    void* ImageProperty::rawData() { return m_data; }

    const void* ImageProperty::rawData() const { return m_data; }

    bool ImageProperty::equalityCompare(const Property* prop) const
    {
        if (const ImageProperty* p = dynamic_cast<const ImageProperty*>(prop))
        {
            if (p->layoutTrait() == m_layout
                && p->sizeOfDimension(0) == m_sizes[0]
                && p->sizeOfDimension(1) == m_sizes[1]
                && p->sizeOfDimension(2) == m_sizes[2]
                && p->sizeOfDimension(3) == m_sizes[3] && p->size() == m_num)
            {
                return memcmp(p->data<byte>(), m_data, sizeInBytes()) == 0;
            }
        }

        return false;
    }

    bool ImageProperty::structureCompare(const Property* prop) const
    {
        if (const ImageProperty* p = dynamic_cast<const ImageProperty*>(prop))
        {
            if (p->layoutTrait() == m_layout
                && p->sizeOfDimension(0) == m_sizes[0]
                && p->sizeOfDimension(1) == m_sizes[1]
                && p->sizeOfDimension(2) == m_sizes[2]
                && p->sizeOfDimension(3) == m_sizes[3] && p->size() == m_num)
            {
                return true;
            }
        }

        return false;
    }

    string ImageProperty::valueAsString() const
    {
        ostringstream str;
        str << sizeOfDimension(0) << "x" << sizeOfDimension(1) << "x"
            << sizeOfDimension(2) << "x" << sizeOfDimension(3) << " "
            << layoutAsString(m_layout);

        return str.str();
    }

} // namespace TwkContainer
