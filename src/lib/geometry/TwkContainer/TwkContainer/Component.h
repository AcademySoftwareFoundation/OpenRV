//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__Component__h__
#define __TwkContainer__Component__h__
#include <string>
#include <vector>
#include <map>

namespace TwkContainer
{
    class Property;
    class PropertyContainer;

    //
    //  class Component
    //
    //  A Component is a group of Properties. An example would be "points"
    //  which might contain the properties "position", "velocity", "mass",
    //  etc. Typically, a component will have properties whose sizes are
    //  the same. However, there is no enforcement of this.
    //

    class Component
    {
    public:
        typedef std::vector<std::string> StringVector;
        typedef StringVector::const_iterator NameIterator;
        typedef std::vector<Property*> Container;
        typedef Container Properties;
        typedef std::vector<Component*> Components;
        typedef std::vector<const Component*> ConstComponents;
        typedef std::map<std::string, Property*> NamedPropertyMap;

        //
        //	Transposable components should have all properties of the same
        //	size.
        //

        explicit Component(const std::string& name, bool transposable = false);
        ~Component();

        //
        //	Returns the values passed into the constructor.
        //

        bool isSynchronized() const { return m_transposable; }

        bool isTransposable() const { return m_transposable; }

        const std::string& name() const { return m_name; }

        PropertyContainer* container() { return m_container; }

        const PropertyContainer* container() const { return m_container; }

        //
        //	Component Management
        //

        Components& components() { return m_components; }

        const Components& components() const { return m_components; }

        Component* component(NameIterator, NameIterator);
        const Component* component(NameIterator, NameIterator) const;

        void add(Component*);
        void remove(Component*);

        //
        //	Access to properties and components
        //

        Component* component(const std::string& fullname);
        const Component* component(const std::string& fullname) const;

        bool hasComponent(Component*) const;
        bool hasComponentRecursive(Component*) const;

        //
        //	Will return an existing component.
        //

        Component* createComponent(const std::string& fullname,
                                   bool synchronized = false);

        Component* createComponent(NameIterator i, NameIterator end,
                                   bool synchronized = false);

        //
        //	Property management
        //

        const Container& properties() const { return m_properties; }

        Container& properties() { return m_properties; }

        void propertiesAsMap(NamedPropertyMap&, std::string prefix) const;

        void add(Property* p);
        void remove(Property*);
        void remove(const std::string&);

        const Property* find(const std::string&) const;
        Property* find(const std::string&);

        const Property* find(NameIterator, NameIterator) const;
        Property* find(NameIterator, NameIterator);

        bool hasProperty(const Property*) const;
        bool propertyPath(ConstComponents& path, const Property*) const;

        template <class T> T* property(const std::string& name);

        template <class T> const T* property(const std::string& name) const;

        //
        //	resizing all properties in a component
        //

        void resize(size_t s);

        //
        //  range deletion
        //

        void erase(size_t start, size_t num);
        void eraseUnsorted(size_t start, size_t num);

        //
        //  resize to the first non-zero property size
        //

        void resizeNonZero();

        //
        //	Copy everthing
        //

        Component* copy() const;

        //
        //  Copy other to this
        //

        void copy(const Component*);

        //
        //  Shallow copy copies the component, but the properties are
        //  shared between this component and the new one
        //

        Component* shallowCopy();

        //
        //  Similar to PropertyContainer::shallowDiffCopy() but for this
        //  component only.
        //

        Component* shallowDiffCopy(const Component* ref);

        //
        //  Contatenate. Presumably the passed in component has all the
        //  same properties as this one. However, its not an error to pass
        //  in somethign that doesn't have all the same props.
        //

        void concatenate(const Component*);

        //
        //  Archive
        //

        bool isPersistent() const;
        bool isCopyable() const;

    private:
        std::string m_name;
        Container m_properties;
        bool m_transposable;
        PropertyContainer* m_container;
        Components m_components;

        friend class PropertyContainer;
    };

    //----------------------------------------------------------------------

    template <class T> inline T* Component::property(const std::string& name)
    {
        return dynamic_cast<T*>(find(name));
    }

    template <class T>
    inline const T* Component::property(const std::string& name) const
    {
        return dynamic_cast<const T*>(find(name));
    }

} // namespace TwkContainer

#endif // __TwkContainer__Component__h__
