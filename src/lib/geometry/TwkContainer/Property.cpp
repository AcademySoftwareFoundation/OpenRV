//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkContainer/Property.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace TwkContainer
{
    using namespace std;

    Property::Info::~Info() { assert(m_ref == 0); }

    void Property::Info::ref() { m_ref++; }

    void Property::Info::unref()
    {
        assert(m_ref != 0);
        m_ref--;
        if (!m_ref)
            delete this;
    }

    string Property::layoutAsString(Layout l)
    {
        switch (l)
        {
        default:
        case IntLayout:
            return "int";
        case FloatLayout:
            return "float";
        case BoolLayout:
            return "bool";
        case DoubleLayout:
            return "double";
        case HalfLayout:
            return "half";
        case ByteLayout:
            return "byte";
        case ShortLayout:
            return "short";
        case StringLayout:
            return "string";
        }

        return "unknown layout";
    }

    size_t Property::sizeofLayout(Layout l)
    {
        switch (l)
        {
        default:
        case IntLayout:
            return sizeof(int);
        case FloatLayout:
            return sizeof(float);
        case BoolLayout:
            return sizeof(char);
        case DoubleLayout:
            return sizeof(double);
        case HalfLayout:
            return sizeof(short);
        case ByteLayout:
            return sizeof(char);
        case ShortLayout:
            return sizeof(short);
        case StringLayout:
            return sizeof(string);
        }
    }

    //----------------------------------------------------------------------

    Property::Property(const std::string& name)
        : m_name(name)
        , m_ref(0)
        , m_info(0)
        , m_propertyContainer(0)
        , m_component(0)
    {
    }

    Property::Property(Info* i, const std::string& name)
        : m_info(i)
        , m_name(name)
        , m_ref(0)
        , m_propertyContainer(0)
        , m_component(0)
    {
        if (m_info)
            m_info->ref();
    }

    Property::~Property()
    {
        assert(m_ref == 0);
        if (m_info)
            m_info->unref();
    }

    void Property::setInfo(Info* info)
    {
        if (m_info == info)
            return;
        if (m_info)
            m_info->unref();
        m_info = info;
        if (m_info)
            m_info->ref();
    }

    void Property::unref()
    {
        assert(m_ref != 0);
        m_ref--;

        if (!m_ref)
        {
            delete this;
        }
    }

    void Property::setInfo(Property* p) { setInfo(p->info()); }

} // namespace TwkContainer
