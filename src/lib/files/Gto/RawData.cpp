//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Gto/RawData.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>

namespace Gto
{
    using namespace std;

    Property::Property(const std::string& n, const std::string& i,
                       Gto::DataType t, size_t s, size_t w, bool allocate)
        : name(n)
        , fullName(n)
        , interp(i)
        , type(t)
        , size(s)
        , dims(w, 0, 0, 0)
        , voidData(0)
    {
        if (allocate)
        {
            if (t == String)
            {
                stringData = new string[w * s];
            }
            else
            {
                voidData = new char[dataSizeInBytes(t) * w * s];
            }
        }
    }

    Property::Property(const std::string& n, Gto::DataType t, size_t s,
                       size_t w, bool allocate)
        : name(n)
        , fullName(n)
        , interp("")
        , type(t)
        , size(s)
        , dims(w, 0, 0, 0)
        , voidData(0)
    {
        if (allocate)
        {
            if (t == String)
            {
                stringData = new string[w * s];
            }
            else
            {
                voidData = new char[dataSizeInBytes(t) * w * s];
            }
        }
    }

    Property::Property(const std::string& n, const std::string& fn,
                       const std::string& i, Gto::DataType t, size_t s,
                       const Dimensions& d, bool allocate)
        : name(n)
        , fullName(fn)
        , interp(i)
        , type(t)
        , size(s)
        , dims(d)
        , voidData(0)
    {
        if (allocate)
        {
            if (t == String)
            {
                stringData = new string[elementSize(dims) * s];
            }
            else
            {
                voidData = new char[dataSizeInBytes(t) * elementSize(dims) * s];
            }
        }
    }

    Property::Property(const std::string& n, const std::string& fn,
                       Gto::DataType t, size_t s, const Dimensions& d,
                       bool allocate)
        : name(n)
        , fullName(fn)
        , interp("")
        , type(t)
        , size(s)
        , dims(d)
        , voidData(0)
    {
        if (allocate)
        {
            if (t == String)
            {
                stringData = new string[elementSize(dims) * s];
            }
            else
            {
                voidData = new char[dataSizeInBytes(t) * elementSize(dims) * s];
            }
        }
    }

    Property::~Property()
    {
        if (type == Gto::String)
        {
            delete[] stringData;
        }
        else
        {
            delete[] (char*)voidData;
        }
    }

    //----------------------------------------------------------------------

    Component::~Component()
    {
        for (size_t i = 0; i < properties.size(); i++)
        {
            delete properties[i];
        }
    }

    //----------------------------------------------------------------------

    Object::~Object()
    {
        for (size_t i = 0; i < components.size(); i++)
        {
            delete components[i];
        }
    }

    //----------------------------------------------------------------------

    RawDataBase::~RawDataBase()
    {
        for (size_t i = 0; i < objects.size(); i++)
        {
            delete objects[i];
        }
    }

    //----------------------------------------------------------------------

    RawDataBaseReader::RawDataBaseReader(unsigned int mode)
        : Reader(mode)
    {
        m_dataBase = new RawDataBase;
    }

    RawDataBaseReader::~RawDataBaseReader() { delete m_dataBase; }

    bool RawDataBaseReader::open(const char* filename)
    {
        if (Reader::open(filename))
        {
            m_dataBase->strings = stringTable();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool RawDataBaseReader::open(std::istream& in, const char* name)
    {
        if (Reader::open(in, name))
        {
            m_dataBase->strings = stringTable();
            return true;
        }
        else
        {
            return false;
        }
    }

    Reader::Request RawDataBaseReader::object(const string& name,
                                              const string& protocol,
                                              unsigned int protocolVersion,
                                              const ObjectInfo& info)
    {
        Object* o = new Object(name, protocol, protocolVersion);
        m_dataBase->objects.push_back(o);
        return Request(true, o);
    }

    Reader::Request RawDataBaseReader::component(const string& name,
                                                 const string& interp,
                                                 const ComponentInfo& info)
    {
        Object* o = reinterpret_cast<Object*>(info.object->objectData);
        Component* c = new Component(name, interp, info.flags);

        while (info.childLevel < m_componentStack.size()
               && !m_componentStack.empty())
        {
            while (m_componentStack.size() > info.childLevel)
                m_componentStack.pop_back();
        }

        if (m_componentStack.empty())
        {
            c->fullName = name;

            m_componentStack.push_back(c);
            o->components.push_back(c);
        }
        else
        {
            c->fullName = m_componentStack.back()->fullName;
            c->fullName += ".";
            c->fullName += name;

            m_componentStack.back()->components.push_back(c);
            m_componentStack.push_back(c);
        }

        return Request(true, c);
    }

    Reader::Request RawDataBaseReader::property(const string& name,
                                                const string& interp,
                                                const PropertyInfo& info)
    {
        // Object *o    =
        // reinterpret_cast<Object*>(info.component->object->objectData);
        Component* c =
            reinterpret_cast<Component*>(info.component->componentData);

        Property* p = new Property(name, info.fullName, interp,
                                   (DataType)info.type, info.size, info.dims);

        c->properties.push_back(p);
        return Request(true, p);
    }

    void* RawDataBaseReader::data(const PropertyInfo& info, size_t bytes)
    {
        // Object *o    =
        // reinterpret_cast<Object*>(info.component->object->objectData);
        // Component *c =
        // reinterpret_cast<Component*>(info.component->componentData);
        Property* p = reinterpret_cast<Property*>(info.propertyData);

        if (bytes == 0)
        {
            p->voidData = NULL;
        }
        else
        {
            p->voidData = new char[bytes];
        }
        return p->voidData;
    }

    void RawDataBaseReader::dataRead(const PropertyInfo& info)
    {
        // Object *o    =
        // reinterpret_cast<Object*>(info.component->object->objectData);
        // Component *c =
        // reinterpret_cast<Component*>(info.component->componentData);
        Property* p = reinterpret_cast<Property*>(info.propertyData);

        p->size = info.size;

        if (p->type == Gto::String)
        {
            int* ints = p->int32Data;
            size_t numItems = p->size * elementSize(p->dims);
            p->stringData = new string[numItems];

            for (size_t i = 0; i < numItems; i++)
            {
                int index = ints[i];

                if (index >= 0 && index < stringTable().size())
                {
                    p->stringData[i] = stringTable()[ints[i]];
                }
                else
                {
                    cout << "ERROR: string index out of range in " << p->name
                         << " property: " << index
                         << " is larger than string table size of "
                         << stringTable().size() << endl;
                }
            }

            delete[] (char*)ints;
        }
    }

    //----------------------------------------------------------------------

    void RawDataBaseWriter::writeProperty(bool header, const Property* property)
    {
        if (header)
        {
            m_writer.property(property->name.c_str(), property->type,
                              property->size, property->dims,
                              property->interp.c_str());

            if (property->type == Gto::String)
            {
                int numItems = property->size * elementSize(property->dims);
                const string* data = property->stringData;

                for (int i = 0; i < numItems; i++)
                {
                    m_writer.intern(data[i]);
                }
            }
        }
        else
        {
            switch (property->type)
            {
            case Gto::Float:
                m_writer.propertyData(property->floatData);
                break;
            case Gto::Double:
                m_writer.propertyData(property->doubleData);
                break;
            case Gto::Short:
                m_writer.propertyData(property->uint16Data);
                break;
            case Gto::Int:
                m_writer.propertyData(property->int32Data);
                break;
            case Gto::Byte:
                m_writer.propertyData(property->uint8Data);
                break;
            case Gto::Half:
            case Gto::Boolean:
            default:
                abort(); // not implemented;
            case Gto::String:
            {
                size_t numItems = property->size * elementSize(property->dims);
                vector<int> data(numItems);

                for (size_t i = 0; i < numItems; i++)
                {
                    uint32 stringId = m_writer.lookup(property->stringData[i]);

                    if (stringId == uint32(-1))
                    {
                        cerr << "WARNING: writer detected bogus string id in "
                             << property->name << ", value is \""
                             << property->stringData[i] << "\"" << endl;

                        stringId = 0;
                    }

                    data[i] = stringId;
                }

                m_writer.propertyData(&data.front());
            }
            break;
            }
        }
    }

    void RawDataBaseWriter::writeComponent(bool header,
                                           const Component* component)
    {
        const Properties& props = component->properties;
        const Gto::Components& comps = component->components;

        if (header)
            m_writer.beginComponent(component->name.c_str(),
                                    component->interp.c_str());

        for (size_t i = 0; i < props.size(); i++)
        {
            if (const Property* p = props[i])
            {
                writeProperty(header, p);
            }
        }

        for (size_t i = 0; i < comps.size(); i++)
        {
            if (const Component* c = comps[i])
            {
                writeComponent(header, c);
            }
        }

        if (header)
            m_writer.endComponent();
    }

    bool RawDataBaseWriter::write(const char* filename, const RawDataBase& db,
                                  Writer::FileType type)
    {
        if (!m_writer.open(filename, type))
        {
            return false;
        }

        for (size_t i = 0; i < db.strings.size(); i++)
        {
            m_writer.intern(db.strings[i]);
        }

        for (size_t i = 0; i < db.objects.size(); i++)
        {
            if (db.objects[i])
            {
                const Object& o = *db.objects[i];

                m_writer.beginObject(o.name.c_str(), o.protocol.c_str(),
                                     o.protocolVersion);

                const Components& components = o.components;

                for (size_t q = 0; q < components.size(); q++)
                {
                    writeComponent(true, components[q]);
                }

                m_writer.endObject();
            }
        }

        m_writer.beginData();

        for (size_t i = 0; i < db.objects.size(); i++)
        {
            if (db.objects[i])
            {
                const Object& o = *db.objects[i];
                const Components& components = o.components;

                for (size_t q = 0; q < components.size(); q++)
                {
                    writeComponent(false, components[q]);
                }
            }
        }

        m_writer.endData();
        return true;
    }

} // namespace Gto
