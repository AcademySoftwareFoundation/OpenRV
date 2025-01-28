//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkContainer/Exception.h>
#include <TwkContainer/GTOReader.h>
#include <TwkContainer/ImageProperty.h>
#include <Gto/EXTProtocols.h>
#include <Gto/Protocols.h>
#include <algorithm>
#include <iostream>
#include <stl_ext/string_algo.h>

namespace TwkContainer
{
    using namespace std;

    GTOReader::GTOReader(bool readAsContainers)
        : Gto::Reader(0)
        , m_readAsContainers(readAsContainers)
        , m_useExisting(false)
    {
    }

    GTOReader::GTOReader(const Containers& objects)
        : Gto::Reader(0)
        , m_readAsContainers(false)
        , m_useExisting(true)
    {
        m_containers.resize(objects.size());
        copy(objects.begin(), objects.end(), m_containers.begin());
    }

    GTOReader::~GTOReader() {}

    GTOReader::Containers GTOReader::read(const char* filename)
    {
        if (open(filename))
        {
            return m_containers;
        }
        else
        {
            string msg = ", opening .gto file \"";
            if (filename)
                msg += string(filename);
            msg += "\", " + why();
            throw ReadFailedExc(msg.c_str());
        }
    }

    GTOReader::Containers GTOReader::read(std::istream& in, const char* name,
                                          unsigned int ormode)
    {
        if (open(in, name, ormode))
        {
            return m_containers;
        }
        else
        {
            string msg = ", opening \"";
            if (name)
                msg += string(name);
            msg += "\", " + why();
            throw ReadFailedExc(msg.c_str());
        }
    }

    PropertyContainer* GTOReader::newContainer(const string& protocol)
    {
        return new PropertyContainer;
    }

    Property* GTOReader::newProperty(const std::string& name,
                                     const std::string& interp,
                                     const PropertyInfo&)
    {
        return 0;
    }

    GTOReader::Request GTOReader::object(const string& name,
                                         const string& protocol,
                                         unsigned int protocolVersion,
                                         const ObjectInfo& info)
    {
        PropertyContainer* g = 0;
        bool found = false;

        if (m_useExisting)
        {
            for (int i = 0; i < m_containers.size(); i++)
            {
                PropertyContainer* pc = m_containers[i];

                if (pc->name() == name && pc->protocol() == protocol)
                {
                    found = true;
                    g = pc;
                }
            }
        }

        if (!g)
        {
            g = newContainer(protocol);
        }

        g->setName(name);
        g->setProtocol(protocol);
        g->setProtocolVersion(protocolVersion);

        if (!found)
            m_containers.push_back(g);

        return Request(true, g);
    }

    GTOReader::Request GTOReader::component(const string& name,
                                            const string& interp,
                                            const ComponentInfo& info)
    {
        PropertyContainer* g =
            reinterpret_cast<PropertyContainer*>(info.object->objectData);

        Component* c = g->createComponent(info.fullName);
        return Request(true, c);
    }

    GTOReader::Request GTOReader::property(const string& name,
                                           const string& ininterp,
                                           const PropertyInfo& info)
    {
        string interp;
        PropertyContainer* pc = reinterpret_cast<PropertyContainer*>(
            info.component->object->objectData);
        Component* c =
            reinterpret_cast<Component*>(info.component->componentData);
        Property* p = c->find(name.c_str());
        Property* np = 0;

        size_t sq = ininterp.find(';');

        if (sq != string::npos)
        {
            interp = ininterp.substr(0, sq);
        }
        else
        {
            interp = ininterp;
        }

        if (!p)
        {
            if (np = newProperty(name, interp, info))
            {
                c->add(np);
                np->resize(info.size);
                return Request(true, np);
            }
        }

        if (info.dims.y > 0)
        {
            //
            //  Multidimensional
            //

            Property::Layout layout = Property::IntLayout;

            switch (info.type)
            {
            default:
            case Gto::Int:
                layout = Property::IntLayout;
                break;
            case Gto::Float:
                layout = Property::FloatLayout;
                break;
            case Gto::Boolean:
                layout = Property::BoolLayout;
                break;
            case Gto::Double:
                layout = Property::DoubleLayout;
                break;
            case Gto::Half:
                layout = Property::HalfLayout;
                break;
            case Gto::Byte:
                layout = Property::ByteLayout;
                break;
            case Gto::Short:
                layout = Property::ShortLayout;
                break;
            }

            ImageProperty* ip = new ImageProperty(name.c_str(), layout);
            ip->restructure(info.dims.x, info.dims.y, info.dims.z, info.dims.w,
                            info.size);
            c->add(ip);
            return Request(true, ip);
        }

        //
        //  These are the defualt property types created when none is
        //  explicitly specified.
        //

        switch (info.type)
        {
        case Gto::Int:
            switch (info.dims.x)
            {
            case 1:
                if (p)
                {
                    if (!dynamic_cast<IntProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new IntProperty(name.c_str()));
                }
                break;

            case 2:
                if (p)
                {
                    if (!dynamic_cast<Vec2iProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec2iProperty(name.c_str()));
                }
                break;

            case 3:
                if (p)
                {
                    if (!dynamic_cast<Vec3iProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec3iProperty(name.c_str()));
                }
                break;

            case 4:
                if (p)
                {
                    if (!dynamic_cast<Vec4iProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec4iProperty(name.c_str()));
                }
                break;

            default:
                cerr << "WARNING: ignoring property \"" << name << "\"" << endl;
                return 0;
            }
            break;

        case Gto::Short:
            switch (info.dims.x)
            {
            case 1:
                if (p)
                {
                    if (!dynamic_cast<ShortProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new ShortProperty(name.c_str()));
                }
                break;

            case 2:
                if (p)
                {
                    if (!dynamic_cast<Vec2usProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec2usProperty(name.c_str()));
                }
                break;

            case 3:
                if (p)
                {
                    if (!dynamic_cast<Vec3usProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec3usProperty(name.c_str()));
                }
                break;

            case 4:
                if (p)
                {
                    if (!dynamic_cast<Vec4usProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec4usProperty(name.c_str()));
                }
                break;

            default:
                cerr << "WARNING: ignoring property \"" << name << "\"" << endl;
                return 0;
            }
            break;

        case Gto::Byte:
            switch (info.dims.x)
            {
            case 1:
                if (p)
                {
                    if (!dynamic_cast<ByteProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new ByteProperty(name.c_str()));
                }
                break;

            case 3:
                if (p)
                {
                    if (!dynamic_cast<Vec3ucProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec3ucProperty(name.c_str()));
                }
                break;

            case 4:
                if (p)
                {
                    if (!dynamic_cast<Vec4ucProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec4ucProperty(name.c_str()));
                }
                break;

            default:
                cerr << "WARNING: ignoring property \"" << name << "\"" << endl;
                return 0;
            }
            break;

        case Gto::Float:

            if (info.dims.y > 0)
            {
                if (info.dims.x == 4 && info.dims.y == 4)
                {
                    if (p)
                    {
                        if (!dynamic_cast<Mat44fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Mat44fProperty(name.c_str()));
                    }
                }
                else if (info.dims.x == 3 && info.dims.y == 3)
                {
                    if (p)
                    {
                        if (!dynamic_cast<Mat33fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Mat33fProperty(name.c_str()));
                    }
                    break;
                }
            }
            else
            {
                switch (info.dims.x)
                {
                case 1:
                    if (p)
                    {
                        if (!dynamic_cast<FloatProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new FloatProperty(name.c_str()));
                    }
                    break;

                case 2:
                    if (p)
                    {
                        if (!dynamic_cast<Vec2fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Vec2fProperty(name.c_str()));
                    }
                    break;

                case 3:
                    if (p)
                    {
                        if (!dynamic_cast<Vec3fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Vec3fProperty(name.c_str()));
                    }
                    break;

                case 4:
                    if (interp == GTO_INTERPRET_QUATERNION)
                    {
                        if (p)
                        {
                            if (!dynamic_cast<QuatfProperty*>(p))
                            {
                                std::string s(", ");
                                s += name;
                                throw BadPropertyTypeMatchExc(s.c_str());
                            }
                        }
                        else
                        {
                            c->add(p = new QuatfProperty(name.c_str()));
                        }
                    }
                    else
                    {
                        if (p)
                        {
                            if (!dynamic_cast<Vec4fProperty*>(p))
                            {
                                std::string s(", ");
                                s += name;
                                throw BadPropertyTypeMatchExc(s.c_str());
                            }
                        }
                        else
                        {
                            c->add(p = new Vec4fProperty(name.c_str()));
                        }
                    }
                    break;

                case 6:
                    if (p)
                    {
                        if (!dynamic_cast<Box3fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Box3fProperty(name.c_str()));
                    }
                    break;

                case 9:
                    if (p)
                    {
                        if (!dynamic_cast<Mat33fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Mat33fProperty(name.c_str()));
                    }
                    break;

                case 16:
                    if (p)
                    {
                        if (!dynamic_cast<Mat44fProperty*>(p))
                        {
                            std::string s(", ");
                            s += name;
                            throw BadPropertyTypeMatchExc(s.c_str());
                        }
                    }
                    else
                    {
                        c->add(p = new Mat44fProperty(name.c_str()));
                    }
                    break;

                default:
                    cerr << "WARNING: ignoring property \"" << name << "\""
                         << endl;
                    return 0;
                }
            }
            break;

        case Gto::Half:
            switch (info.dims.x)
            {
            case 1:
                if (p)
                {
                    if (!dynamic_cast<HalfProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new HalfProperty(name.c_str()));
                }
                break;

            case 2:
                if (p)
                {
                    if (!dynamic_cast<Vec2hProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec2hProperty(name.c_str()));
                }
                break;

            case 3:
                if (p)
                {
                    if (!dynamic_cast<Vec3hProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec3hProperty(name.c_str()));
                }
                break;

            case 4:
                if (p)
                {
                    if (!dynamic_cast<Vec4hProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new Vec4hProperty(name.c_str()));
                }
                break;

            default:
                cerr << "WARNING: ignoring property \"" << name << "\"" << endl;
                return 0;
            }
            break;

        case Gto::String:
            if (info.dims.x == 1)
            {
                if (p)
                {
                    if (!dynamic_cast<StringProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new StringProperty(name.c_str()));
                }
            }
            else if (info.dims.x == 2)
            {
                if (p)
                {
                    if (!dynamic_cast<StringPairProperty*>(p))
                    {
                        std::string s(", ");
                        s += name;
                        throw BadPropertyTypeMatchExc(s.c_str());
                    }
                }
                else
                {
                    c->add(p = new StringPairProperty(name.c_str()));
                }
            }
            else
            {
                cerr << "WARNING: ignoring property \"" << name << "\"" << endl;
                return 0;
            }
            break;

        case Gto::Double:
        case Gto::Boolean:
            cerr << "WARNING: ignoring property \"" << name << "\"" << endl;
            return 0;
        }

        p->resize(info.size);
        return Request(true, p);
    }

    void* GTOReader::data(const PropertyInfo& info, size_t bytes)
    {
        Property* p = reinterpret_cast<Property*>(info.propertyData);
        p->resize(info.size);

        /* AJG - no more dereferencing empty array's */
        if (p->size() == 0)
            return 0;

        if (StringProperty* sp = dynamic_cast<StringProperty*>(p))
        {
            m_tempstrings.resize(sp->size());
            return &m_tempstrings.front();
        }
        else if (StringPairProperty* sp = dynamic_cast<StringPairProperty*>(p))
        {
            m_tempstrings.resize(sp->size() * 2);
            return &m_tempstrings.front();
        }

        return p->rawData();
    }

    void GTOReader::dataRead(const PropertyInfo& info)
    {
        Property* p = reinterpret_cast<Property*>(info.propertyData);

        if (StringProperty* sp = dynamic_cast<StringProperty*>(p))
        {
            for (int i = 0; i < m_tempstrings.size(); i++)
            {
                (*sp)[i] = stringFromId(m_tempstrings[i]);
            }

            m_tempstrings.clear();
        }
        else if (StringPairProperty* sp = dynamic_cast<StringPairProperty*>(p))
        {
            for (int i = 0; i < m_tempstrings.size(); i += 2)
            {
                (*sp)[i / 2].first = stringFromId(m_tempstrings[i + 0]);
                (*sp)[i / 2].second = stringFromId(m_tempstrings[i + 1]);
            }

            m_tempstrings.clear();
        }
    }

    PropertyContainer* GTOReader::findByName(const string& name) const
    {
        for (size_t i = 0; i < m_containers.size(); i++)
        {
            PropertyContainer* pc = m_containers[i];
            if (pc->name() == name)
                return pc;
        }

        return 0;
    }

    PropertyContainer* GTOReader::findByProtocol(const string& protocol) const
    {
        for (size_t i = 0; i < m_containers.size(); i++)
        {
            PropertyContainer* pc = m_containers[i];
            if (pc->protocol() == protocol)
                return pc;
        }

        return 0;
    }

    void GTOReader::deleteAll()
    {
        for (size_t i = 0; i < m_containers.size(); i++)
        {
            delete m_containers[i];
        }
    }

} // namespace TwkContainer
