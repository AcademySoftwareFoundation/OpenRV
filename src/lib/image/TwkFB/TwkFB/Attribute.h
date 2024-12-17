//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__Attribute__h__
#define __TwkFB__Attribute__h__
#include <TwkFB/dll_defs.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Iostream.h>
#include <sstream>
#include <string>
#include <vector>
#include <string.h>

namespace TwkFB
{

    typedef std::vector<unsigned char> DataContainer;

    //
    //  FBAttribute
    //
    //  An untyped base class for attributes
    //

    class TWKFB_EXPORT FBAttribute
    {
    public:
        FBAttribute(const std::string& name)
            : m_name(name)
        {
        }

        virtual ~FBAttribute() {}

        static void* operator new(size_t s);
        static void operator delete(void* p, size_t s);

        const std::string& name() const { return m_name; }

        virtual FBAttribute* copy() const = 0;
        virtual FBAttribute* copyWithPrefix(const std::string&) const = 0;
        virtual std::string valueAsString() const = 0;
        virtual void* data() = 0;

    protected:
        std::string m_name;
    };

    //----------------------------------------------------------------------
    //
    //  Specialized for DataContainer
    //

    class TWKFB_EXPORT DataContainerAttribute : public FBAttribute
    {
    public:
        DataContainerAttribute(std::string name, const void* memory,
                               size_t size)
            : FBAttribute(name)
            , m_container(size)
        {
            memcpy(&m_container.front(), memory, size);
        }

        virtual FBAttribute* copy() const;
        virtual FBAttribute* copyWithPrefix(const std::string&) const;
        virtual std::string valueAsString() const;

        virtual void* data() { return &m_container.front(); }

        void set(const void* data, size_t s)
        {
            m_container.resize(s);
            memcpy(&m_container.front(), data, s);
        }

        const DataContainer* dataContainer() const { return &m_container; }

        const void* data() const { return &m_container.front(); }

        size_t size() const { return m_container.size(); }

    protected:
        DataContainer m_container;
    };

    //----------------------------------------------------------------------
    //
    //  TypedFBAttribute
    //  An attribute with a value
    //

    template <typename T> class TypedFBAttribute : public FBAttribute
    {
    public:
        TypedFBAttribute(const std::string& name, T value);

        virtual ~TypedFBAttribute();

        const T& value() const { return m_value; }

        T& value() { return m_value; }

        virtual FBAttribute* copy() const;
        virtual FBAttribute* copyWithPrefix(const std::string&) const;

        virtual void* data() { return &m_value; }

        virtual std::string valueAsString() const;

    private:
        T m_value;
    };

    template <typename T>
    TypedFBAttribute<T>::TypedFBAttribute(const std::string& name, T value)
        : FBAttribute(name)
        , m_value(value)
    {
    }

    template <>
    TypedFBAttribute<const char*>::TypedFBAttribute(const std::string& name,
                                                    const char* value);
    template <>
    TypedFBAttribute<char*>::TypedFBAttribute(const std::string& name,
                                              char* value);

    template <typename T> TypedFBAttribute<T>::~TypedFBAttribute() {}

    template <typename T> FBAttribute* TypedFBAttribute<T>::copy() const
    {
        return new TypedFBAttribute<T>(name(), m_value);
    }

    template <typename T>
    FBAttribute*
    TypedFBAttribute<T>::copyWithPrefix(const std::string& prefix) const
    {
        return new TypedFBAttribute<T>(prefix + name(), m_value);
    }

    typedef TypedFBAttribute<std::string> StringAttribute;
    typedef TypedFBAttribute<float> FloatAttribute;

    template <typename T> std::string TypedFBAttribute<T>::valueAsString() const
    {
        std::ostringstream str;
        str << value();
        return str.str();
    }

    //----------------------------------------------------------------------
    //
    //  TypedFBVectorAttribute
    //  An attribute with a vector of values
    //

    template <typename T> class TypedFBVectorAttribute : public FBAttribute
    {
    public:
        typedef std::vector<T> TVector;

        TypedFBVectorAttribute(const std::string& name, const TVector& value)
            : FBAttribute(name)
            , m_value(value)
        {
        }

        virtual ~TypedFBVectorAttribute();

        const TVector& value() const { return m_value; }

        TVector& value() { return m_value; }

        virtual FBAttribute* copy() const;
        virtual FBAttribute* copyWithPrefix(const std::string&) const;

        virtual void* data() { return &m_value; }

        virtual std::string valueAsString() const;

    private:
        TVector m_value;
    };

    template <typename T> TypedFBVectorAttribute<T>::~TypedFBVectorAttribute()
    {
    }

    template <typename T> FBAttribute* TypedFBVectorAttribute<T>::copy() const
    {
        return new TypedFBVectorAttribute<T>(name(), m_value);
    }

    template <typename T>
    FBAttribute*
    TypedFBVectorAttribute<T>::copyWithPrefix(const std::string& prefix) const
    {
        return new TypedFBVectorAttribute<T>(prefix + name(), m_value);
    }

    template <typename T>
    std::string TypedFBVectorAttribute<T>::valueAsString() const
    {
        std::ostringstream str;

        for (size_t i = 0; i < value().size(); i++)
        {
            if (i)
                str << ", ";
            str << value()[i];
        }

        return str.str();
    }

    //----------------------------------------------------------------------

    typedef TypedFBAttribute<std::string> StringAttribute;
    typedef TypedFBAttribute<float> FloatAttribute;
    typedef TypedFBAttribute<TwkMath::Vec2f> Vec2fAttribute;
    typedef TypedFBAttribute<TwkMath::Mat44f> Mat44fAttribute;
    typedef TypedFBAttribute<TwkMath::Mat33f> Mat33fAttribute;
    typedef TypedFBAttribute<int> IntAttribute;
    typedef TypedFBVectorAttribute<std::string> StringVectorAttribute;

} // namespace TwkFB

#endif // __TwkFB__Attribute__h__
