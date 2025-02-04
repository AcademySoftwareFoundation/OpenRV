//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__GTOWriter__h__
#define __TwkContainer__GTOWriter__h__
#include <Gto/Writer.h>
#include <string>

namespace TwkContainer
{
    class Geometry;
    class Component;
    class Property;
    class PropertyContainer;

    class GTOWriter
    {
    public:
        struct Object
        {
            Object()
                : name("")
                , protocol("")
                , protocolVersion(1)
                , container(0)
            {
            }

            Object(PropertyContainer* c, const std::string& n = "",
                   const std::string& p = "", unsigned int v = 0)
                : container(c)
                , protocol(p)
                , protocolVersion(v)
                , name(n)
            {
            }

            std::string name;
            std::string protocol;
            unsigned int protocolVersion;
            PropertyContainer* container;
        };

        typedef std::vector<Object> ObjectVector;
        typedef Gto::Writer::FileType FileType;

        GTOWriter(const char* stamp = 0);
        virtual ~GTOWriter();

        static Gto::DataType gtoType(const Property*);

        bool write(const char* filename, const ObjectVector&, FileType type);

        bool write(std::ostream& out, const ObjectVector&, FileType type);

        //
        //  Override to add interpretation string even when not available from
        //  Property::Info
        //

        virtual std::string interpretationString(const Property*);

        //
        //  Override to prevent property from being written or to override
        //  Property::Info persistent flag.
        //

        virtual bool shouldWriteProperty(const Property*) const;

    private:
        void writeComponent(bool header, const Component*);
        void writeProperty(bool header, const Property*);
        bool writeInternal(const ObjectVector&, FileType type);

    private:
        std::string m_stamp;
        Gto::Writer m_writer;
    };

} // namespace TwkContainer

#endif // __TwkContainer__GTOWriter__h__
