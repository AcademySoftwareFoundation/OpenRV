//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkContainer/PropertyContainer.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>
#include <algorithm>
#include <TwkContainer/Exception.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/join.hpp>

namespace TwkContainer
{
    using namespace std;
    using namespace stl_ext;
    using namespace boost;

    PropertyContainer::PropertyContainer()
        : m_protocolVersion(1)
    {
    }

    PropertyContainer::~PropertyContainer()

    {
        delete_contents(m_components);
        m_components.clear();
    }

    void PropertyContainer::parseFullName(const string& fullname,
                                          StringVector& parts)
    {
        algorithm::split(parts, fullname, is_any_of(string(".")));
    }

    void PropertyContainer::copyNameAndProtocol(const PropertyContainer* other)
    {
        setName(other->name());
        setProtocol(other->protocol());
        setProtocolVersion(other->protocolVersion());
    }

    PropertyContainer* PropertyContainer::copy() const
    {
        PropertyContainer* c = emptyContainer();
        c->copyNameAndProtocol(this);

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isCopyable())
            {
                if (Component* o =
                        c->component(m_components[i]->name().c_str()))
                {
                    c->remove(o);
                    delete o;
                }

                c->add(m_components[i]->copy());
            }
        }

        c->relinkPointers();
        return c;
    }

    PropertyContainer* PropertyContainer::shallowCopy() const
    {
        PropertyContainer* c = emptyContainer();
        c->copyNameAndProtocol(this);

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isCopyable())
            {
                if (Component* o =
                        c->component(m_components[i]->name().c_str()))
                {
                    c->remove(o);
                    delete o;
                }

                c->add(m_components[i]->shallowCopy());
            }
        }

        c->relinkPointers();
        return c;
    }

    void PropertyContainer::copy(const PropertyContainer* container)
    {
        const Components& components = container->components();
        ostringstream errors;

        for (int i = 0; i < container->components().size(); i++)
        {
            const Component* other = container->components()[i];

            if (other->isCopyable())
            {
                Component* c = component(other->name());

                if (!c)
                {
                    c = new Component(other->name());
                    add(c);
                }

                try
                {
                    c->copy(other);
                }
                catch (TypeMismatchExc&)
                {
                    errors << endl
                           << "ERROR: copying " << container->name() << "."
                           << c->name() << endl;
                }
            }
        }

        if (!errors.str().empty())
            throw TypeMismatchExc(errors.str().c_str());
    }

    void PropertyContainer::relinkPointers()
    {
        // nothing
    }

    PropertyContainer* PropertyContainer::emptyContainer() const
    {
        return new PropertyContainer;
    }

    void PropertyContainer::add(Component* c)
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->name() == c->name())
            {
                throw UnexpectedExc(
                    ", component with same name already exists");
            }
        }

        m_components.push_back(c);
        c->m_container = this;
    }

    void PropertyContainer::remove(Component* c)
    {
        Components::iterator i =
            std::find(m_components.begin(), m_components.end(), c);

        if (i != m_components.end())
        {
            m_components.erase(i);
        }
    }

    Component* PropertyContainer::createComponent(NameIterator i,
                                                  NameIterator end,
                                                  bool synchronized)
    {
        if (Component* c = component(*i))
        {
            return c->createComponent(i + 1, end, synchronized);
        }
        else
        {
            Component* cc = new Component(*i, synchronized);
            add(cc);
            return cc->createComponent(i + 1, end, synchronized);
        }
    }

    Component* PropertyContainer::createComponent(const string& name,
                                                  bool synchronized)
    {
        StringVector parts;
        parseFullName(name, parts);
        return createComponent(parts.begin(), parts.end(), synchronized);
    }

    Component* PropertyContainer::component(NameIterator begin,
                                            NameIterator end)
    {
        if (begin == end)
            return 0;

        for (size_t i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->name() == *begin)
            {
                return m_components[i]->component(begin + 1, end);
            }
        }

        return 0;
    }

    const Component* PropertyContainer::component(NameIterator begin,
                                                  NameIterator end) const
    {
        if (begin == end)
            return 0;

        for (size_t i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->name() == *begin)
            {
                return m_components[i]->component(begin + 1, end);
            }
        }

        return 0;
    }

    Component* PropertyContainer::component(const std::string& name)
    {
        StringVector parts;
        parseFullName(name, parts);
        return component(parts.begin(), parts.end());
    }

    const Component* PropertyContainer::component(const std::string& name) const
    {
        StringVector parts;
        parseFullName(name, parts);
        return component(parts.begin(), parts.end());
    }

    void PropertyContainer::synchronize(const PropertyContainer* container)
    {
        const Components& components = container->components();

        for (int i = 0; i < components.size(); i++)
        {
            const Component* oc = components[i];
            Component* c = createComponent(oc->name().c_str());

            const Component::Container& properties = oc->properties();

            for (int q = 0; q < properties.size(); q++)
            {
                const Property* op = properties[q];

                if (!c->find(op->name().c_str()))
                {
                    c->add(op->copyNoData());
                }
            }
        }
    }

    void PropertyContainer::concatenate(const PropertyContainer* pc)
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            Component* c = m_components[i];

            if (const Component* oc = pc->component(c->name().c_str()))
            {
                c->concatenate(oc);
            }
        }
    }

    const Property* PropertyContainer::find(NameIterator begin,
                                            NameIterator end) const
    {
        if (begin == end)
            return 0;

        if (const Component* c = component(*begin))
        {
            return c->find(begin + 1, end);
        }

        return 0;
    }

    Property* PropertyContainer::find(NameIterator begin, NameIterator end)
    {
        if (begin == end)
            return 0;

        if (Component* c = component(*begin))
        {
            return c->find(begin + 1, end);
        }

        return 0;
    }

    Property* PropertyContainer::find(const std::string& fullname)
    {
        StringVector parts;
        parseFullName(fullname, parts);
        return find(parts.begin(), parts.end());
    }

    const Property* PropertyContainer::find(const std::string& fullname) const
    {
        StringVector parts;
        parseFullName(fullname, parts);
        return find(parts.begin(), parts.end());
    }

    Property* PropertyContainer::find(const std::string& comp,
                                      const std::string& name)
    {
        if (Component* c = component(comp))
        {
            return c->find(name);
        }

        return 0;
    }

    const Property* PropertyContainer::find(const std::string& comp,
                                            const std::string& name) const
    {
        if (const Component* c = component(comp))
        {
            return c->find(name);
        }

        return 0;
    }

    Component* PropertyContainer::componentOf(const Property* p)
    {
        //
        //  Try quick access via property, fallback to slower lookup if that
        //  fails.
        //

        if (Component* c = static_cast<Component*>(p->component()))
        {
            if (c->container() == p->container())
                return c;
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            Component* c = m_components[i];

            if (Property* cp = c->find(p->name().c_str()))
            {
                if (cp == p)
                    return c;
            }
        }

        return 0;
    }

    const Component* PropertyContainer::componentOf(const Property* p) const
    {
        //
        //  Try quick access via property, fallback to slower lookup if that
        //  fails.
        //
        if (const Component* c = static_cast<Component*>(p->component()))
        {
            if (c->container() == p->container())
                return c;
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            const Component* c = m_components[i];

            if (const Property* cp = c->find(p->name().c_str()))
            {
                if (cp == p)
                    return c;
            }
        }

        return 0;
    }

    void PropertyContainer::removeProperty(Property* p)
    {
        if (Component* c = componentOf(p))
        {
            c->remove(p);
            return;
        }
    }

    bool PropertyContainer::hasComponent(Component* c) const
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i] == c
                || m_components[i]->hasComponentRecursive(c))
                return true;
        }

        return false;
    }

    bool PropertyContainer::isPersistent() const
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isPersistent())
                return true;
        }

        return false;
    }

    void PropertyContainer::propertiesAsMap(NamedPropertyMap& pmap) const
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            m_components[i]->propertiesAsMap(pmap, "");
        }
    }

    bool PropertyContainer::propertyPathInternal(const Property* p,
                                                 ConstComponents& comps) const
    {
        for (size_t i = 0; i < m_components.size(); i++)
        {
            const Component* c = m_components[i];
            if (c->propertyPath(comps, p))
                return true;
        }

        return false;
    }

    PropertyContainer::ConstComponents
    PropertyContainer::propertyPath(const Property* p) const
    {
        ConstComponents comps;
        propertyPathInternal(p, comps);
        return comps;
    }

    string PropertyContainer::propertyFullName(const Property* p,
                                               bool withContainer) const
    {
        ostringstream str;
        ConstComponents comps;

        if (propertyPathInternal(p, comps))
        {
            if (withContainer)
                str << name() << ".";

            for (ConstComponents::const_reverse_iterator i = comps.rbegin();
                 i != comps.rend(); ++i)
            {
                str << (*i)->name() << ".";
            }

            str << p->name();
        }

        return str.str();
    }

    //-*****************************************************************************
    // Utility functions
    //******************************************************************************

    std::string name(const PropertyContainer& g) { return g.name(); }

    void setName(PropertyContainer& g, const std::string& name)
    {
        g.setName(name);
    }

    void setProtocol(PropertyContainer& g, const std::string& name)
    {
        g.setProtocol(name);
    }

    void setProtocolVersion(PropertyContainer& g, int version)
    {
        g.setProtocolVersion((unsigned int)version);
    }

    int protocolVersion(const PropertyContainer& g)
    {
        return (int)g.protocolVersion();
    }

    std::string protocol(const PropertyContainer& g) { return g.protocol(); }

    PropertyContainer*
    PropertyContainer::shallowDiffCopy(const PropertyContainer* ref)
    {
        PropertyContainer* pc = new PropertyContainer();
        Components& comps = components();

        for (size_t i = 0; i < comps.size(); i++)
        {
            Component* c = comps[i];

            if (c->isCopyable())
            {
                if (const Component* refcomp = ref->component(c->name()))
                {
                    Component* newc = c->name() == "object"
                                          ? c->shallowCopy()
                                          : c->shallowDiffCopy(refcomp);

                    if (newc->properties().empty())
                    {
                        delete newc;
                    }
                    else
                    {
                        pc->add(newc);
                    }
                }
                else
                {
                    pc->add(c->shallowCopy());
                }
            }
        }

        return pc;
    }

} // namespace TwkContainer
