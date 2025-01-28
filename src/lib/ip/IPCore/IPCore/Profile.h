//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__Profile__h__
#define __IPCore__Profile__h__
#include <iostream>
#include <vector>

namespace TwkContainer
{
    class GTOReader;
    class PropertyContainer;
} // namespace TwkContainer

namespace IPCore
{
    class GroupIPNode;
    class IPNode;
    class IPGraph;

    class Profile
    {
    public:
        Profile(const std::string& filename, IPGraph* graph);
        ~Profile();

        const std::string& name() const { return m_name; }

        const std::string& fileName() { return m_filename; }

        const std::string& tag() { return m_tag; }

        bool isLoaded() { return m_reader != 0; }

        void load(); // can throw if somethings wrong

        TwkContainer::GTOReader* reader() const { return m_reader; }

        TwkContainer::PropertyContainer* root() const { return m_root; }

        TwkContainer::PropertyContainer* header() const { return m_header; }

        void apply(IPNode*);

        std::string comment() const;
        std::string structureDescription() const;

    private:
        std::string m_name;
        std::string m_filename;
        std::string m_tag;
        TwkContainer::GTOReader* m_reader;
        TwkContainer::PropertyContainer* m_root;
        TwkContainer::PropertyContainer* m_header;
        IPGraph* m_graph;
    };

    typedef std::vector<Profile*> ProfileVector;

    void profilesInPath(ProfileVector& profiles, const std::string& tag,
                        IPGraph*);
    std::string profileMatchingNameInPath(const std::string& name,
                                          const std::string& tag,
                                          IPGraph* graph);

} // namespace IPCore

#endif // __IPCore__Profile__h__
