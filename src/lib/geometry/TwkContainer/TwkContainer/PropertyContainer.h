//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__PropertyContainer__h__
#define __TwkContainer__PropertyContainer__h__
#include <TwkContainer/Properties.h>
#include <TwkContainer/Component.h>
#include <TwkContainer/Exception.h>

namespace TwkContainer
{

    //
    //  class PropertyContainer
    //
    //  Holds a list of components which in turn hold a list of properites.
    //

    class PropertyContainer
    {
    public:
        typedef std::vector<Component*> Components;
        typedef std::vector<const Component*> ConstComponents;
        typedef std::vector<std::string> StringVector;
        typedef StringVector::const_iterator NameIterator;
        typedef std::map<std::string, Property*> NamedPropertyMap;

        PropertyContainer();
        virtual ~PropertyContainer();

        //
        //	Copying
        //

        virtual PropertyContainer* copy() const;
        virtual PropertyContainer* shallowCopy() const;

        virtual void setName(const std::string& t) { m_name = t; }

        virtual void setProtocol(const std::string& t) { m_protocol = t; }

        virtual void setProtocolVersion(unsigned int v)
        {
            m_protocolVersion = v;
        }

        const std::string& name() const { return m_name; }

        const std::string& protocol() const { return m_protocol; }

        const unsigned int protocolVersion() const { return m_protocolVersion; }

        virtual void
        copy(const PropertyContainer* other); // doesn't copy name+protocol
        void copyNameAndProtocol(const PropertyContainer* other);

        //
        //  Makes a new container which is has properties shallow copied
        //  from *this iff its property values differ from ref's
        //

        virtual PropertyContainer*
        shallowDiffCopy(const PropertyContainer* ref);

        //
        //	Component Management
        //

        Components& components() { return m_components; }

        const Components& components() const { return m_components; }

        virtual void add(Component*);
        virtual void remove(Component*);

        //
        //	Access to properties and components
        //

        Component* component(const std::string& fullname);
        const Component* component(const std::string& fullname) const;

        Component* component(NameIterator, NameIterator);
        const Component* component(NameIterator, NameIterator) const;

        Component* componentOf(const Property*);
        const Component* componentOf(const Property*) const;

        bool hasComponent(Component*) const;

        ConstComponents propertyPath(const Property*) const;
        std::string propertyFullName(const Property*,
                                     bool withContainer = true) const;

        //
        //	Will return an existing component.
        //

        Component* createComponent(const std::string& name,
                                   bool synchronized = false);

        Component* createComponent(NameIterator i, NameIterator end,
                                   bool synchronized = false);

        //
        //  Search for properties
        //

        Property* find(NameIterator, NameIterator);
        const Property* find(NameIterator, NameIterator) const;

        Property* find(const std::string& comp, const std::string& name);

        const Property* find(const std::string& comp,
                             const std::string& name) const;

        Property* find(const std::string& fullname);
        const Property* find(const std::string& fullname) const;

        template <class T>
        T* property(const std::string& comp, const std::string& name);

        template <class T>
        const T* property(const std::string& comp,
                          const std::string& name) const;

        template <class T> T* property(const std::string& fullname);

        template <class T> const T* property(const std::string& fullname) const;

        //
        //  All properties flattened into a map
        //

        void propertiesAsMap(NamedPropertyMap&) const;

        //
        //  Create or return existing property
        //

        template <class T>
        T* createProperty(const std::string& comp, const std::string& name);

        template <class T> T* createProperty(const std::string& fullname);

        template <class T> T* createProperty(NameIterator, NameIterator);

        //
        //  Declare creates or modifies the property and (optionally) sets
        //  the value. If forceValue is true, the value of the property is
        //  set from the incoming value regardless of the existing
        //  value. If it is false, the existing value is not changed
        //  unless the property is empty.
        //

        template <typename T>
        T* declareProperty(const std::string& fullname,
                           Property::Info* info = 0, bool forceValue = true);

        template <typename T>
        T* declareProperty(const std::string& fullname,
                           const typename T::value_type& value,
                           Property::Info* info = 0, bool forceValue = true);

        template <typename T>
        T* declareProperty(const std::string& fullname,
                           const typename T::container_type& value,
                           Property::Info* info = 0, bool forceValue = true);

        //
        //  This will get the property value if it exists, and isn't
        //  empty. Otherwise it will return the passed in defaultValue
        //

        template <typename T>
        typename T::value_type
        propertyValue(const std::string& fullname,
                      const typename T::value_type& defaultValue) const;

        template <typename T>
        typename T::container_type&
        propertyContainer(const std::string& fullname);

        template <typename T>
        const typename T::container_type&
        propertyContainer(const std::string& fullname) const;

        template <typename T>
        typename T::value_type
        propertyValue(const T*,
                      const typename T::value_type& defaultValue) const;

        //
        //  Set sets a single value. This requires that the property
        //  *already* has size 1 or the container size.
        //

        template <typename T>
        void setProperty(const std::string& fullname,
                         const typename T::value_type& value);

        template <typename T>
        void setProperty(T*, const typename T::value_type& value);

        //
        //  Sets the whole value container.
        //

        template <typename T>
        void setProperty(const std::string& fullname,
                         const typename T::container_type& value);

        //
        //  Returns the property -- if the property is shared with another
        //  container, a new copy will be created and returned.
        //

        template <class T>
        T* uniqueProperty(const std::string& comp, const std::string& name);

        //
        //  Remove property
        //

        template <class T>
        void removeProperty(const std::string& comp, const std::string& name);

        template <class T> void removeProperty(const std::string& fullname);

        void removeProperty(Property*);

        //
        //  This function will make sure that this container is "synchornized"
        //  with the passed in container. That means that this container has
        //  exactly the same properties as the passed in one. It does not mean
        //  that it has the same data or even that the property sizes are the
        //  same.
        //

        void synchronize(const PropertyContainer*);

        //
        //  Concatenate (intelligently) the passed in property container's
        //  data onto this one's. The default implementation is *not*
        //  intelligent about the concatenation, so you should certainly
        //  override it.
        //

        virtual void concatenate(const PropertyContainer*);

        //
        //	Must call if you replace properties
        //

        virtual void relinkPointers();

        //
        //  Should be archived?
        //

        bool isPersistent() const;

        //
        //  Parse a fullname
        //

        static void parseFullName(const std::string& fullname,
                                  StringVector& parts);

    protected:
        virtual PropertyContainer* emptyContainer() const;
        bool propertyPathInternal(const Property*, ConstComponents&) const;

    private:
        std::string m_name;
        std::string m_protocol;
        unsigned int m_protocolVersion;
        Components m_components;
    };

    // Forward declaration of some "utility" functions
    std::string name(const PropertyContainer& p);
    void setName(PropertyContainer& p, const std::string&);
    std::string protocol(const PropertyContainer& p);
    void setProtocol(PropertyContainer& p, const std::string&);
    void setProtocolVersion(PropertyContainer& p, int);
    int protocolVersion(const PropertyContainer& p);

    inline std::string name(const PropertyContainer* p) { return name(*p); }

    inline std::string protocol(const PropertyContainer* p)
    {
        return protocol(*p);
    }

    inline int protocolVersion(const PropertyContainer* p)
    {
        return protocolVersion(*p);
    }

    //----------------------------------------------------------------------

    template <class T>
    T* PropertyContainer::property(const std::string& comp,
                                   const std::string& name)
    {
        if (Component* c = component(comp))
        {
            return c->property<T>(name);
        }

        return 0;
    }

    template <class T>
    const T* PropertyContainer::property(const std::string& comp,
                                         const std::string& name) const
    {
        if (const Component* c = component(comp))
        {
            return c->property<const T>(name);
        }

        return 0;
    }

    template <class T>
    T* PropertyContainer::property(const std::string& fullname)
    {
        StringVector parts;
        parseFullName(fullname, parts);

        if (Component* c =
                component(parts.begin(), parts.begin() + (parts.size() - 1)))
        {
            return c->property<T>(parts.back());
        }

        return 0;
    }

    template <class T>
    const T* PropertyContainer::property(const std::string& fullname) const
    {
        StringVector parts;
        parseFullName(fullname, parts);

        if (const Component* c =
                component(parts.begin(), parts.begin() + (parts.size() - 1)))
        {
            return c->property<const T>(parts.back());
        }

        return 0;
    }

    template <class T>
    T* PropertyContainer::createProperty(const std::string& comp,
                                         const std::string& name)
    {
        StringVector parts;
        parseFullName(comp, parts);
        if (Component* c = createComponent(parts.begin(), parts.end(), false))
        {
            if (T* p = c->property<T>(name))
            {
                return p;
            }
            else
            {
                T* pp = new T(name);
                c->add(pp);
                return pp;
            }
        }

        throw UnexpectedExc(", PropertyContainer::createProperty() failed");
    }

    template <class T>
    T* PropertyContainer::createProperty(const std::string& fullname)
    {
        StringVector parts;
        parseFullName(fullname, parts);

        if (Component* c = createComponent(
                parts.begin(), parts.begin() + parts.size() - 1, false))
        {
            if (T* p = c->property<T>(parts.back()))
            {
                return p;
            }
            else
            {
                T* pp = new T(parts.back());
                c->add(pp);
                return pp;
            }
        }

        throw UnexpectedExc(", PropertyContainer::createProperty() failed");
    }

    template <class T>
    T* PropertyContainer::declareProperty(const std::string& fullname,
                                          Property::Info* info, bool forceValue)
    {
        T* p = createProperty<T>(fullname);
        if (forceValue)
            p->resize(0);
        if (info)
            p->setInfo(info);
        return p;
    }

    template <class T>
    T* PropertyContainer::declareProperty(const std::string& fullname,
                                          const typename T::value_type& value,
                                          Property::Info* info, bool forceValue)
    {
        T* p = createProperty<T>(fullname);

        if (forceValue || p->empty())
        {
            p->resize(1);
            p->front() = value;
        }

        if (info)
            p->setInfo(info);
        return p;
    }

    template <class T>
    T*
    PropertyContainer::declareProperty(const std::string& fullname,
                                       const typename T::container_type& value,
                                       Property::Info* info, bool forceValue)
    {
        T* p = createProperty<T>(fullname);

        if (forceValue || p->empty())
        {
            p->valueContainer() = value;
        }

        if (info)
            p->setInfo(info);
        return p;
    }

    template <typename T>
    void PropertyContainer::setProperty(const std::string& fullname,
                                        const typename T::value_type& value)
    {
        if (T* p = property<T>(fullname))
        {
            p->resize(1);
            p->front() = value;
        }
        else
        {
            throw NoPropertyExc(fullname.c_str());
        }
    }

    template <typename T>
    void PropertyContainer::setProperty(T* p,
                                        const typename T::value_type& value)
    {
        p->resize(1);
        p->front() = value;
    }

    template <typename T>
    void PropertyContainer::setProperty(const std::string& fullname,
                                        const typename T::container_type& value)
    {
        if (T* p = property<T>(fullname))
        {
            p->valueContainer() = value;
        }
        else
        {
            throw NoPropertyExc(fullname.c_str());
        }
    }

    template <class T>
    T* PropertyContainer::uniqueProperty(const std::string& comp,
                                         const std::string& name)
    {
        if (Component* c = createComponent(comp))
        {
            if (T* p = c->property<T>(name))
            {
                if (p->numRef() == 1)
                {
                    return p;
                }
                else
                {
                    c->remove(p);
                    T* n = dynamic_cast<T*>(p->copy());
                    c->add(n);
                    return n;
                }
            }
            else
            {
                return createProperty<T>(comp, name);
            }
        }

        throw UnexpectedExc(", PropertyContainer::createComponent() failed");
    }

    template <class T>
    void PropertyContainer::removeProperty(const std::string& comp,
                                           const std::string& name)
    {
        if (Component* c = component(comp))
        {
            if (T* p = c->property<T>(name))
            {
                c->remove(p);
                return;
            }
        }
    }

    template <class T>
    void PropertyContainer::removeProperty(const std::string& fullname)
    {
        if (T* p = property<T>(fullname))
        {
            removeProperty(p);
            return;
        }
    }

    template <typename T>
    typename T::value_type PropertyContainer::propertyValue(
        const std::string& fullname,
        const typename T::value_type& defaultValue) const
    {
        if (const T* p = property<T>(fullname))
        {
            if (!p->empty() && p->size() == 1)
                return p->front();
        }

        return defaultValue;
    }

    template <typename T>
    typename T::value_type PropertyContainer::propertyValue(
        const T* p, const typename T::value_type& defaultValue) const
    {
        if (p && !p->empty() && p->size() == 1)
            return p->front();
        return defaultValue;
    }

    template <typename T>
    const typename T::container_type&
    PropertyContainer::propertyContainer(const std::string& fullname) const
    {
        if (const T* p = property<T>(fullname))
        {
            return p->valueContainer();
        }

        std::string msg =
            fullname + " does not exist, propertyContainer failed.";
        throw NoPropertyExc(msg.c_str());
    }

    template <typename T>
    typename T::container_type&
    PropertyContainer::propertyContainer(const std::string& fullname)
    {
        if (T* p = property<T>(fullname))
        {
            return p->valueContainer();
        }

        std::string msg =
            fullname + " does not exist, propertyContainer failed.";
        throw NoPropertyExc(msg.c_str());
    }

} // namespace TwkContainer

#endif // __TwkContainer__PropertyContainer__h__
