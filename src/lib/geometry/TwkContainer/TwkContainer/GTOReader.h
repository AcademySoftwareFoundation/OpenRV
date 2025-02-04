//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__GTOReader__h__
#define __TwkContainer__GTOReader__h__
#include <string>
#include <vector>
#include <TwkContainer/PropertyContainer.h>
#include <Gto/Reader.h>

namespace TwkContainer
{

    class GTOReader : protected Gto::Reader
    {
    public:
        //
        //  Types
        //

        typedef std::vector<PropertyContainer*> Containers;
        typedef Reader::Request Request;

        //
        //  Constructors
        //

        GTOReader(bool readAsContainers = false);
        GTOReader(const Containers& existingContainers);
        virtual ~GTOReader();

        //
        //	There's only one call to this class that does anything
        //	interesting. read will throw one of the TwkContainer exceptions
        // if 	something goes wrong.
        //

        Containers read(const char* filename);
        Containers read(std::istream& in, const char* name,
                        unsigned int ormode = 0);

        //
        //  Override this function if you want to do non-default
        //  container creation based on the protocol.
        //

        virtual PropertyContainer* newContainer(const std::string& protocol);

        //
        //  Override this function if you want to make a specific type of
        //  property given the name of a property, a protocol, and
        //  component name.
        //
        //  If this function returns 0, the reader will use the default
        //  property type.
        //

        virtual Property* newProperty(const std::string& name,
                                      const std::string& interp,
                                      const PropertyInfo&);

        virtual Request object(const std::string& name,
                               const std::string& protocol,
                               unsigned int protocolVersion,
                               const ObjectInfo& header);

        virtual Request component(const std::string& name,
                                  const std::string& interp,
                                  const ComponentInfo& header);

        virtual Request property(const std::string& name,
                                 const std::string& interp,
                                 const PropertyInfo& header);

        virtual void* data(const PropertyInfo&, size_t bytes);
        virtual void dataRead(const PropertyInfo&);

        //
        //  For more interesting access to the containers
        //

        PropertyContainer* findByProtocol(const std::string&) const;
        PropertyContainer* findByName(const std::string&) const;

        const Containers& containersRead() const { return m_containers; }

        void deleteAll();

    private:
        bool m_readAsContainers;
        bool m_useExisting;
        Containers m_containers;
        std::vector<int> m_tempstrings;
    };

} // namespace TwkContainer

#endif // __TwkContainer__GTOReader__h__
