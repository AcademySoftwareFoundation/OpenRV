//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkContainer/GTOWriter.h>
#include <Gto/Protocols.h>
#include <Gto/EXTProtocols.h>
#include <TwkContainer/PropertyContainer.h>
#include <iostream>

namespace TwkContainer
{
    using namespace std;

    GTOWriter::GTOWriter(const char* stamp)
        : m_writer()
    {
        if (stamp)
        {
            m_stamp = stamp;
        }
    }

    GTOWriter::~GTOWriter() {}

    Gto::DataType GTOWriter::gtoType(const Property* p)
    {
        switch (p->layoutTrait())
        {
        case Property::FloatLayout:
            return Gto::Float;
        case Property::HalfLayout:
            return Gto::Half;
        case Property::DoubleLayout:
            return Gto::Double;
        case Property::ShortLayout:
            return Gto::Short;
        case Property::IntLayout:
            return Gto::Int;
        case Property::ByteLayout:
            return Gto::Byte;
        case Property::StringLayout:
            return Gto::String;
        default:
            break;
        }

        return Gto::Byte;
    }

    string GTOWriter::interpretationString(const Property* p)
    {
        if (p->info())
        {
            if (p->info()->interp() != "")
                return p->info()->interp();
        }

        if (dynamic_cast<const QuatfProperty*>(p))
        {
            return "quaternion";
        }
        else
        {
            return "";
        }
    }

    bool GTOWriter::shouldWriteProperty(const Property* property) const
    {
        return !property->info() || property->info()->isPersistent();
    }

    void GTOWriter::writeProperty(bool header, const Property* property)
    {
        if (!shouldWriteProperty(property))
            return;

        if (property->layoutTrait() == Property::CompoundLayout)
        {
            cerr << "WARNING: GTOWriter does not understand type of property "
                 << property->name() << endl;
            return;
        }

        Gto::DataType type = gtoType(property);
        string interp = interpretationString(property);
        int xsize = property->xsizeTrait();
        int ysize = property->ysizeTrait();
        int zsize = property->zsizeTrait();
        int wsize = property->wsizeTrait();

        if (const StringProperty* sp =
                dynamic_cast<const StringProperty*>(property))
        {
            if (header)
            {
                for (int i = 0; i < sp->size(); i++)
                {
                    const std::string& s = (*sp)[i];
                    m_writer.intern(s);
                }

                m_writer.property(property->name().c_str(), type,
                                  property->size(),
                                  Gto::Dimensions(xsize, ysize, zsize, wsize),
                                  interp.c_str());
            }
            else
            {
                int* data = new int[sp->size()];

                for (int i = 0; i < sp->size(); i++)
                {
                    const std::string& s = (*sp)[i];
                    data[i] = m_writer.lookup(s);
                }

                m_writer.propertyData(data);
            }
        }
        else if (const StringPairProperty* sp =
                     dynamic_cast<const StringPairProperty*>(property))
        {
            if (header)
            {
                for (int i = 0; i < sp->size(); i++)
                {
                    const std::string& s0 = (*sp)[i].first;
                    const std::string& s1 = (*sp)[i].second;
                    m_writer.intern(s0);
                    m_writer.intern(s1);
                }

                m_writer.property(property->name().c_str(), type,
                                  property->size(),
                                  Gto::Dimensions(xsize, ysize, zsize, wsize),
                                  interp.c_str());
            }
            else
            {
                int* data = new int[sp->size() * 2];

                for (int i = 0; i < sp->size(); i++)
                {
                    const std::string& s0 = (*sp)[i].first;
                    const std::string& s1 = (*sp)[i].second;
                    data[i * 2 + 0] = m_writer.lookup(s0);
                    data[i * 2 + 1] = m_writer.lookup(s1);
                }

                m_writer.propertyData(data);
            }
        }
        else
        {
            if (header)
            {
                m_writer.property(property->name().c_str(), type,
                                  property->size(),
                                  Gto::Dimensions(xsize, ysize, zsize, wsize),
                                  interp.c_str());
            }
            else
            {
                if (!property->empty())
                {
                    m_writer.propertyDataRaw(property->rawData());
                }
                else
                {
                    m_writer.propertyDataRaw(0);
                }
            }
        }
    }

    void GTOWriter::writeComponent(bool header, const Component* component)
    {
        if (component->isPersistent())
        {
            const Component::Container& props = component->properties();
            const Component::Components& comps = component->components();
            const unsigned int flags =
                component->isTransposable() ? Gto::Matrix : 0;
            const bool declare = header && (!props.empty() || !comps.empty());

            if (declare)
                m_writer.beginComponent(component->name().c_str(), flags);
            for (size_t i = 0; i < props.size(); i++)
                writeProperty(header, props[i]);
            for (size_t i = 0; i < comps.size(); i++)
                writeComponent(header, comps[i]);
            if (declare)
                m_writer.endComponent();
        }
    }

    bool GTOWriter::write(ostream& out, const GTOWriter::ObjectVector& objects,
                          FileType type)
    {
        if (!m_writer.open(out, type))
            return false;
        return writeInternal(objects, type);
    }

    bool GTOWriter::write(const char* filename,
                          const GTOWriter::ObjectVector& objects, FileType type)
    {
        if (!m_writer.open(filename, type))
            return false;
        return writeInternal(objects, type);
    }

    bool GTOWriter::writeInternal(const GTOWriter::ObjectVector& objects,
                                  FileType type)
    {
        if (m_stamp != "")
        {
            m_writer.intern(m_stamp);
        }

        for (int i = 0; i < objects.size(); i++)
        {
            const Object& o = objects[i];
            string protocol = o.protocol;
            string name = o.name;
            unsigned int version = o.protocolVersion;
            PropertyContainer* container = o.container;

            if (!container->isPersistent())
            {
                continue;
            }

            if (name == "")
                name = container->name();
            if (protocol == "")
                protocol = container->protocol();
            if (version == 0)
                version = container->protocolVersion();

            //
            //  Since we know the proper version numbers here (the TwkContainer
            //  library is being used) just set the ones we know.
            //

            if (protocol == GTO_PROTOCOL_POLYGON
                || protocol == GTO_PROTOCOL_TRANSFORM
                || protocol == GTO_PROTOCOL_CATMULL_CLARK
                || protocol == GTO_PROTOCOL_LOOP)
            {
                version = 2;
            }
            else if (protocol == GTO_PROTOCOL_NURBS
                     || protocol == GTO_PROTOCOL_PARTICLE
                     || protocol == GTO_PROTOCOL_TEXCHANNEL
                     || protocol == GTO_PROTOCOL_STRAND
                     || protocol == GTO_PROTOCOL_IMAGE
                     || protocol == GTO_EXT_PROTOCOL_SECONDARY_STRAND
                     || protocol == GTO_EXT_PROTOCOL_RIGID_BODY)
            {
                version = 1;
            }
            else
            {
                // its a user thang.
            }

            m_writer.beginObject(name.c_str(), protocol.c_str(), version);

            const PropertyContainer* g = container;
            const PropertyContainer::Components& comps = g->components();

            for (int q = 0; q < comps.size(); q++)
            {
                writeComponent(true, comps[q]);
            }

            m_writer.endObject();
        }

        m_writer.beginData();

        for (int i = 0; i < objects.size(); i++)
        {
            const Object& o = objects[i];

            const PropertyContainer* g = o.container;
            const PropertyContainer::Components& comps = g->components();

            for (int q = 0; q < comps.size(); q++)
            {
                writeComponent(false, comps[q]);
            }
        }

        m_writer.endData();
        m_writer.close();
        return true;
    }

} // namespace TwkContainer
