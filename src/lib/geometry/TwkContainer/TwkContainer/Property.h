//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__Property__h__
#define __TwkContainer__Property__h__
#include <string>
#include <TwkContainer/DefaultValues.h>
#include <TwkContainer/Exception.h>
#include <algorithm>
#include <assert.h>
#include <sstream>

namespace TwkContainer
{
    class PropertyContainer;

    //
    //  class Property
    //
    //  Property is an array of some type with a name. The base class
    //  holds nothing but the name. Properties are typically created by
    //  typedefing the template class TypedProperty<> defined below.
    //

    class Property
    {
    public:
        //
        //  Types
        //

        //
        //  Info is an object which can be attached to each property. Its
        //  reference counted.
        //

        class Info
        {
        public:
            Info(bool persistent = true, bool copyable = true,
                 size_t initialRefCount = 0, const std::string& interp = "")
                : m_ref(initialRefCount)
                , m_copyable(copyable)
                , m_persistent(persistent)
                , m_interp(interp) {};

            virtual void ref();
            virtual void unref();

            size_t numRef() const { return m_ref; }

            void setPersistent(bool b) { m_persistent = b; }

            bool isPersistent() const { return m_persistent; }

            void setCopyable(bool b) { m_copyable = b; }

            bool isCopyable() const { return m_copyable; }

            void setInterp(const std::string& i) { m_interp = i; }

            const std::string& interp() const { return m_interp; }

        protected:
            virtual ~Info();

        private:
            size_t m_ref;
            std::string m_interp;
            bool m_persistent : 1;
            bool m_copyable : 1;
        };

        //
        //  Used for generic memory manipulation on POD types.
        //

        enum Layout
        {
            CompoundLayout,
            FloatLayout,
            IntLayout,
            BoolLayout,
            DoubleLayout,
            HalfLayout,
            ByteLayout,
            ShortLayout,
            StringLayout
        };

        static size_t sizeofLayout(Layout);
        static std::string layoutAsString(Layout);

        //
        //  Constructors
        //

        Property(const std::string& name);
        Property(Info* i, const std::string& name);

        //
        //  Member access
        //

        const std::string& name() const { return m_name; }

        //
        //  Info
        //

        Info* info() { return m_info; }

        const Info* info() const { return m_info; }

        void setInfo(Info*);
        void setInfo(Property*); // set to argument's info

        template <class T> const T* infoOfType() const
        {
            return dynamic_cast<const T*>(m_info);
        }

        //
        //  Reference counting
        //

        void ref() { m_ref++; }

        void unref();

        size_t numRef() const { return m_ref; }

        bool isShared() const { return m_ref > 1; }

        //
        //  Container that owns this prop. Note: this really busts the idea
        //  that a prop can be shared by two containers. The original container
        //  must be the last to unref().
        //

        const PropertyContainer* container() const
        {
            return m_propertyContainer;
        }

        PropertyContainer* container() { return m_propertyContainer; }

        //
        //  Generic Traits
        //

        virtual Layout layoutTrait() const = 0;
        virtual size_t xsizeTrait() const = 0;
        virtual size_t ysizeTrait() const = 0;
        virtual size_t zsizeTrait() const = 0;
        virtual size_t wsizeTrait() const = 0;

        bool isCompoundLayout() { return layoutTrait() == CompoundLayout; }

        //
        //  Reordering
        //

        virtual void swap(size_t a, size_t b) = 0;

        //
        //	Size
        //

        virtual size_t size() const = 0;
        virtual bool empty() const = 0;

        //
        //	Resize
        //

        virtual void resize(size_t) = 0;

        //
        //  Delete range
        //

        virtual void erase(size_t start, size_t num) = 0;
        virtual void eraseUnsorted(size_t start, size_t num) = 0;

        //
        //	This returns the size of an element as calculated by the
        //	sizeof() operator.
        //

        virtual size_t sizeofElement() const = 0;

        //
        //	Inserting a default value
        //

        virtual void insertDefaultValue(size_t index, size_t len = 1) = 0;

        void appendDefaultValue(size_t len = 1)
        {
            insertDefaultValue(size(), len);
        }

        virtual void clearToDefaultValue() = 0;

        //
        //	copy() copies the property completely. copyNoData() copies the
        //	type and name, but without any data. copy() with an argument
        //	will copy the contents of the argument to this.
        //

        virtual Property* copy(const char* newName = 0) const = 0;
        virtual Property* copyNoData() const = 0;
        virtual void copy(const Property*) = 0;
        virtual void copyRange(const Property*, size_t begin, size_t end) = 0;

        //
        //  Concatenate data
        //

        virtual void concatenate(const Property*) = 0;

        //
        //  Continuguous anonymous raw data
        //

        virtual void* rawData() = 0;
        virtual const void* rawData() const = 0;

        virtual std::string valueAsString() const { return ""; }

        //
        //  Compare
        //

        virtual bool equalityCompare(const Property*) const = 0;
        virtual bool structureCompare(const Property*) const = 0;

        //
        //  Optimized access to containing component.  Can't work if property is
        //  shared.
        //

        void* component() const { return (isShared() ? 0 : m_component); }

    protected:
        virtual ~Property();

        Info* mutableInfo() const { return m_info; }

        void setComponent(void* c) { m_component = c; }

    private:
        std::string m_name;
        size_t m_ref;
        Info* m_info;
        PropertyContainer* m_propertyContainer;
        mutable void* m_component;

        friend class Component;
    };

    //
    //  template TypedProperty
    //
    //  TypedProperty<> extends property to add a container of some
    //  type. This holds the data. (See Properties.h). The additional
    //  template arguments are property traits.
    //

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout type>
    class TypedProperty : public Property
    {
    public:
        typedef Container container_type;
        typedef typename Container::value_type T;
        typedef typename Container::value_type value_type;
        typedef typename Container::value_type* value_pointer;
        typedef const typename Container::value_type* const_value_pointer;
        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;
        typedef typename Container::reverse_iterator reverse_iterator;
        typedef
            typename Container::const_reverse_iterator const_reverse_iterator;
        typedef typename Container::reference reference;
        typedef typename Container::const_reference const_reference;
        typedef TypedProperty<Container, xsize, ysize, zsize, wsize, type>
            this_type;

        explicit TypedProperty(const std::string& name)
            : Property(name)
            , m_container()
            , m_default(defaultValue<T>())
        {
        }

        explicit TypedProperty(Info* i, const std::string& name)
            : Property(i, name)
            , m_container()
            , m_default(defaultValue<T>())
        {
        }

        virtual ~TypedProperty();

        //
        //  Traits
        //

        virtual Layout layoutTrait() const;
        virtual size_t xsizeTrait() const;
        virtual size_t ysizeTrait() const;
        virtual size_t zsizeTrait() const;
        virtual size_t wsizeTrait() const;

        //
        //	Access
        //

        reference operator[](size_t i)
        {
            assert(i < m_container.size());
            return m_container[i];
        }

        const_reference operator[](size_t i) const
        {
            assert(i < m_container.size());
            return m_container[i];
        }

        reference front() { return m_container.front(); }

        value_type front() const { return m_container.front(); }

        reference back() { return m_container.back(); }

        value_type back() const { return m_container.back(); }

        //
        //	Iterators
        //

        iterator begin() { return m_container.begin(); }

        const_iterator begin() const { return m_container.begin(); }

        iterator end() { return m_container.end(); }

        const_iterator end() const { return m_container.end(); }

        reverse_iterator rbegin() { return m_container.begin(); }

        const_reverse_iterator rbegin() const { return m_container.begin(); }

        reverse_iterator rend() { return m_container.end(); }

        const_reverse_iterator rend() const { return m_container.end(); }

        void erase(iterator i) { m_container.erase(i); }

        //
        //	Adding/Removing elements
        //

        void push_back(const_reference v) { m_container.push_back(v); }

        void push_front(const_reference v)
        {
            m_container.insert(m_container.begin(), v);
        }

        void pop_back() { m_container.pop_back(); }

        void pop_front() { m_container.erase(m_container.begin()); }

        //
        //  Reordering
        //

        virtual void swap(size_t a, size_t b);

        //
        //	Size
        //

        virtual size_t size() const;
        virtual bool empty() const;

        virtual size_t sizeofElement() const;

        virtual void resize(size_t s);

        virtual void erase(size_t start, size_t num);
        virtual void eraseUnsorted(size_t start, size_t num);

        //
        //	copy() copies the property completely. copyNoData() copies the
        //	type and name, but without any data. copy() with an argument
        //	will copy the contents of the argument to this.
        //

        virtual Property* copy(const char* newName = 0) const;
        virtual Property* copyNoData() const;
        virtual void copy(const Property*);
        virtual void copyRange(const Property*, size_t begin, size_t end);

        //
        //  Concat
        //

        virtual void concatenate(const Property*);
        void concatenateWithOffset(const this_type*, T);

        //
        //	Default value handling
        //

        virtual void insertDefaultValue(size_t, size_t len = 1);
        virtual void clearToDefaultValue();

        //
        //	Raw data pointer
        //

        const_value_pointer data() const;
        value_pointer data();

        // Return container directly
        const Container& valueContainer() const { return m_container; }

        Container& valueContainer() { return m_container; }

        virtual void* rawData();
        virtual const void* rawData() const;

        virtual std::string valueAsString() const;

        virtual bool equalityCompare(const Property*) const;
        virtual bool structureCompare(const Property*) const;

    private:
        Container m_container;
        T m_default;
    };

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    TypedProperty<Container, xsize, ysize, zsize, wsize,
                  layout>::~TypedProperty()
    {
        // nothing
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    size_t
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::size() const
    {
        return m_container.size();
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    bool
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::empty() const
    {
        return m_container.empty();
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    size_t TypedProperty<Container, xsize, ysize, zsize, wsize,
                         layout>::sizeofElement() const
    {
        return sizeof(typename Container::value_type);
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void TypedProperty<Container, xsize, ysize, zsize, wsize,
                       layout>::insertDefaultValue(size_t index, size_t len)
    {
        m_container.insert(m_container.begin() + index, len, m_default);
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void TypedProperty<Container, xsize, ysize, zsize, wsize,
                       layout>::clearToDefaultValue()
    {
        std::fill(m_container.begin(), m_container.end(), m_default);
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::swap(size_t a,
                                                                       size_t b)
    {
        std::swap(m_container[a], m_container[b]);
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::resize(
        size_t s)
    {
        size_t osize = m_container.size();

        if (s < osize)
        {
            m_container.resize(s);
        }
        else if (s > osize)
        {
            insertDefaultValue(m_container.size(), s - m_container.size());
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::erase(
        size_t s, size_t n)
    {
        if (m_container.size())
        {
            m_container.erase(m_container.begin() + s,
                              m_container.begin() + (s + n));
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::eraseUnsorted(
        size_t s, size_t n)
    {
        if (m_container.size())
        {
            if (n == 1)
            {
                *(m_container.begin() + s) = m_container.back();
            }
            else
            {
                std::copy(m_container.begin() + (m_container.size() - n - 1),
                          m_container.end(), m_container.begin() + s);
            }

            m_container.resize(m_container.size() - n);
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    typename TypedProperty<Container, xsize, ysize, zsize, wsize,
                           layout>::const_value_pointer
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::data() const
    {
        return &(m_container.front());
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    typename TypedProperty<Container, xsize, ysize, zsize, wsize,
                           layout>::value_pointer
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::data()
    {
        return &(m_container.front());
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    Property*
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::copyNoData()
        const
    {
        return new this_type(mutableInfo(), name().c_str());
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    Property*
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::copy(
        const char* newName) const
    {
        this_type* p =
            new this_type(mutableInfo(), newName ? newName : name().c_str());
        p->resize(size());
        std::copy(begin(), end(), p->begin());
        return p;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::copy(
        const Property* p)
    {
        if (const this_type* tp = dynamic_cast<const this_type*>(p))
        {
            m_container.resize(tp->size());
            std::copy(tp->begin(), tp->end(), begin());
        }
        else
        {
            throw TypeMismatchExc();
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::copyRange(
        const Property* p, size_t i0, size_t i1)
    {
        if (const this_type* tp = dynamic_cast<const this_type*>(p))
        {
            m_container.resize(i1 - i0);
            std::copy(tp->begin() + i0, tp->begin() + i1, begin());
        }
        else
        {
            throw TypeMismatchExc();
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::concatenate(
        const Property* p)
    {
        if (const this_type* tp = dynamic_cast<const this_type*>(p))
        {
            for (int i = 0; i < tp->size(); i++)
            {
                push_back((*tp)[i]);
            }
        }
        else
        {
            throw TypeMismatchExc();
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void TypedProperty<Container, xsize, ysize, zsize, wsize,
                       layout>::concatenateWithOffset(const this_type* p, T v)
    {
        for (int i = 0, s = p->size(); i < s; i++)
        {
            push_back((*p)[i] + v);
        }
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    Property::Layout
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::layoutTrait()
        const
    {
        return layout;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    size_t
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::xsizeTrait()
        const
    {
        return xsize;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    size_t
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::ysizeTrait()
        const
    {
        return ysize;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    size_t
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::zsizeTrait()
        const
    {
        return zsize;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    size_t
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::wsizeTrait()
        const
    {
        return wsize;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    void*
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::rawData()
    {
        return data();
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    const void*
    TypedProperty<Container, xsize, ysize, zsize, wsize, layout>::rawData()
        const
    {
        return data();
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    bool TypedProperty<Container, xsize, ysize, zsize, wsize,
                       layout>::equalityCompare(const Property* p) const
    {
        if (const this_type* tp = dynamic_cast<const this_type*>(p))
        {
            if (m_container.size() == tp->valueContainer().size())
            {
                for (size_t i = 0, s = m_container.size(); i < s; i++)
                {
                    if ((*this)[i] != (*tp)[i])
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    bool TypedProperty<Container, xsize, ysize, zsize, wsize,
                       layout>::structureCompare(const Property* p) const
    {
        if (const this_type* tp = dynamic_cast<const this_type*>(p))
        {
            if (m_container.size() == tp->valueContainer().size())
            {
                return true;
            }
        }

        return false;
    }

    template <class Container, size_t xsize, size_t ysize, size_t zsize,
              size_t wsize, Property::Layout layout>
    std::string TypedProperty<Container, xsize, ysize, zsize, wsize,
                              layout>::valueAsString() const
    {
        std::ostringstream str;

        for (size_t i = 0; i < m_container.size(); i++)
        {
            if (i)
                str << " ";
            str << m_container[i];
        }

        return str.str();
    }

} // namespace TwkContainer

#endif // __TwkContainer__Property__h__
