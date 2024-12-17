//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkContainer/Component.h>
#include <TwkContainer/Property.h>
#include <TwkContainer/PropertyContainer.h>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>
#include <TwkContainer/Exception.h>
#include <algorithm>
#include <functional>

namespace TwkContainer
{
    using namespace stl_ext;
    using namespace std;

    Component::Component(const std::string& name, bool transposable)
        : m_name(name)
        , m_transposable(transposable)
        , m_container(0)
    {
        // nothing
    }

    Component::~Component()
    {
        for_each(m_properties.begin(), m_properties.end(),
                 std::mem_fn(&Property::unref));

        m_properties.clear();

        for (size_t i = 0; i < m_components.size(); i++)
        {
            delete m_components[i];
        }

        m_components.clear();
    }

    void Component::remove(Property* p)
    {
        stl_ext::remove(m_properties, p);
        p->setComponent(0);
        p->unref();
    }

    void Component::remove(const std::string& name)
    {
        if (Property* p = find(name))
            remove(p);
    }

    void Component::add(Property* p)
    {
        if (component(p->name()))
        {
            throw UnexpectedExc(", component with same name already exists");
        }

        if (find(p->name()))
        {
            throw UnexpectedExc(", property with same name already exists");
        }

        p->ref();
        if (!p->m_propertyContainer)
            p->m_propertyContainer = m_container;
        m_properties.push_back(p);

        p->setComponent(this);
    }

    bool Component::hasProperty(const Property* p) const
    {
        for (int i = 0; i < m_properties.size(); i++)
        {
            if (m_properties[i] == p)
                return true;
        }

        return false;
    }

    bool Component::propertyPath(ConstComponents& path, const Property* p) const
    {
        if (hasProperty(p))
        {
            path.push_back(this);
            return true;
        }

        for (size_t i = 0; i < m_components.size(); i++)
        {
            const Component* c = m_components[i];
            if (c->propertyPath(path, p))
                return true;
        }

        return false;
    }

    const Property* Component::find(NameIterator begin, NameIterator end) const
    {
        if (begin == end)
            return 0;

        for (size_t i = 0; i < m_properties.size(); i++)
        {
            if (m_properties[i]->name() == *begin)
                return m_properties[i];
        }

        for (size_t i = 0; i < m_components.size(); i++)
        {
            if (const Property* p = m_components[i]->find(begin + 1, end))
            {
                return p;
            }
        }

        return 0;
    }

    Property* Component::find(NameIterator begin, NameIterator end)
    {
        if (begin == end)
            return 0;

        for (size_t i = 0; i < m_properties.size(); i++)
        {
            if (m_properties[i]->name() == *begin)
                return m_properties[i];
        }

        for (size_t i = 0; i < m_components.size(); i++)
        {
            if (Property* p = m_components[i]->find(begin + 1, end))
            {
                return p;
            }
        }

        return 0;
    }

    const Property* Component::find(const std::string& name) const
    {
        StringVector parts;
        PropertyContainer::parseFullName(name, parts);
        return find(parts.begin(), parts.end());
    }

    Property* Component::find(const std::string& name)
    {
        StringVector parts;
        PropertyContainer::parseFullName(name, parts);
        return find(parts.begin(), parts.end());
    }

    void Component::resize(size_t s)
    {
        for (int i = 0, n = m_properties.size(); i < n; i++)
        {
            if (m_properties[i]->size() != s)
                m_properties[i]->resize(s);
        }
    }

    void Component::resizeNonZero()
    {
        for (int i = 0, s = m_properties.size(); i < s; i++)
        {
            if (m_properties[i]->size() != 0)
            {
                resize(m_properties[i]->size());
                return;
            }
        }
    }

    Component* Component::copy() const
    {
        Component* c = new Component(name(), isSynchronized());

        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            if (!p->info() || p->info()->isCopyable())
                c->add(p->copy());
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isCopyable())
                c->add(m_components[i]->copy());
        }

        return c;
    }

    Component* Component::shallowCopy()
    {
        Component* c = new Component(name(), isSynchronized());

        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            if (!p->info() || p->info()->isCopyable())
                c->add(p);
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isCopyable())
                c->add(m_components[i]->shallowCopy());
        }

        return c;
    }

    void Component::copy(const Component* other)
    {
        const Properties& props = other->properties();
        const Components& comps = other->components();

        for (int i = 0; i < props.size(); i++)
        {
            const Property* p0 = props[i];

            if (!p0->info() || p0->info()->isCopyable())
            {
                if (Property* p1 = find(p0->name()))
                {
                    p1->copy(p0);
                }
                else
                {
                    add(p0->copy());
                }
            }
        }

        for (int i = 0; i < comps.size(); i++)
        {
            const Component* c0 = comps[i];

            if (c0->isCopyable())
            {
                if (Component* c = component(c0->name()))
                {
                    c->copy(c0);
                }
                else
                {
                    add(c0->copy());
                }
            }
        }
    }

    void Component::concatenate(const Component* c)
    {
        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p0 = m_properties[i];

            if (const Property* p1 = c->find(p0->name().c_str()))
            {
                p0->concatenate(p1);
            }
        }
    }

    void Component::erase(size_t start, size_t num)
    {
        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            p->erase(start, num);
        }
    }

    void Component::eraseUnsorted(size_t start, size_t num)
    {
        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            p->eraseUnsorted(start, num);
        }
    }

    bool Component::isPersistent() const
    {
        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            if (!p->info() || p->info()->isPersistent())
                return true;
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isPersistent())
                return true;
        }

        return false;
    }

    bool Component::isCopyable() const
    {
        if (m_properties.empty())
            return true;

        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            if (!p->info() || p->info()->isCopyable())
                return true;
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->isCopyable())
                return true;
        }

        return false;
    }

    Component* Component::shallowDiffCopy(const Component* ref)
    {
        Component* newc = new Component(name(), isSynchronized());

        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];

            if (!p->info() || p->info()->isCopyable())
            {
                if (const Property* refp = ref->find(p->name()))
                {
                    if (!p->equalityCompare(refp))
                    {
                        newc->add(p);
                    }
                }
                else
                {
                    newc->add(p);
                }
            }
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            Component* c = m_components[i];

            if (c->isCopyable())
            {
                if (const Component* refc = ref->component(c->name()))
                {
                    newc->add(c->shallowDiffCopy(refc));
                }
                else
                {
                    newc->add(c->shallowCopy());
                }
            }
        }

        return newc;
    }

    void Component::add(Component* c)
    {
        if (find(c->name()))
        {
            throw UnexpectedExc(", property with same name already exists");
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->name() == c->name())
            {
                throw UnexpectedExc(
                    ", component with same name already exists");
            }
        }

        m_components.push_back(c);
        c->m_container = m_container;
    }

    void Component::remove(Component* c)
    {
        Components::iterator i =
            std::find(m_components.begin(), m_components.end(), c);

        if (i != m_components.end())
        {
            m_components.erase(i);
        }
    }

    Component* Component::createComponent(NameIterator i, NameIterator end,
                                          bool synchronized)
    {
        if (i == end)
            return this;

        if (Component* nc = component(*i))
        {
            return nc->createComponent(i + 1, end, synchronized);
        }
        else
        {
            Component* newComp = new Component(*i, synchronized);
            add(newComp);
            return newComp->createComponent(i + 1, end, synchronized);
        }
    }

    Component* Component::createComponent(const std::string& name,
                                          bool synchronized)
    {
        if (Component* c = component(name))
        {
            return c;
        }
        else
        {
            StringVector parts;
            PropertyContainer::parseFullName(name, parts);
            return createComponent(parts.begin(), parts.end(), synchronized);
        }
    }

    const Component* Component::component(NameIterator begin,
                                          NameIterator end) const
    {
        if (begin == end)
            return this;

        for (size_t i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->name() == *begin)
            {
                return m_components[i]->component(begin + 1, end);
            }
        }

        return 0;
    }

    Component* Component::component(NameIterator begin, NameIterator end)
    {
        if (begin == end)
            return this;

        for (size_t i = 0; i < m_components.size(); i++)
        {
            if (m_components[i]->name() == *begin)
            {
                return m_components[i]->component(begin + 1, end);
            }
        }

        return 0;
    }

    Component* Component::component(const std::string& name)
    {
        StringVector parts;
        PropertyContainer::parseFullName(name, parts);
        return component(parts.begin(), parts.end());
    }

    const Component* Component::component(const std::string& name) const
    {
        StringVector parts;
        PropertyContainer::parseFullName(name, parts);
        return component(parts.begin(), parts.end());
    }

    bool Component::hasComponent(Component* c) const
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i] == c)
                return true;
        }

        return false;
    }

    bool Component::hasComponentRecursive(Component* c) const
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            if (m_components[i] == c
                || m_components[i]->hasComponentRecursive(c))
                return true;
        }

        return false;
    }

    void Component::propertiesAsMap(NamedPropertyMap& pmap,
                                    std::string prefix) const
    {
        for (int i = 0; i < m_properties.size(); i++)
        {
            Property* p = m_properties[i];
            string key = prefix;
            if (key != "")
                key += ".";
            key += name();
            key += ".";
            key += p->name();

            pmap[key] = p;
        }

        for (int i = 0; i < m_components.size(); i++)
        {
            string newPrefix = prefix;
            newPrefix += ".";
            newPrefix += name();
            m_components[i]->propertiesAsMap(pmap, newPrefix);
        }
    }

} // namespace TwkContainer
