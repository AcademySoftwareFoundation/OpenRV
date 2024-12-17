//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__NodeManager__h__
#define __IPCore__NodeManager__h__
#include <IPCore/NodeDefinition.h>
#include <iostream>
#include <map>
#include <string>

namespace IPCore
{
    class IPNode;
    class NodeDefinition;
    class GroupNodeDefinition;

    /// NodeManager manages NodeDefinition container creation and I/O

    ///
    /// NodeManager is responsible for creating IPNodes of any type. Some are
    /// hard coded, others are the result of loading GTO node definition data.
    ///

    class NodeManager
    {
    public:
        struct DefinitionSource
        {
            std::string filename;
            bool loaded;
        };

        typedef TwkContainer::PropertyContainer PropertyContainer;
        typedef std::map<std::string, NodeDefinition*> NodeDefinitionMap;
        typedef std::map<std::string, GroupNodeDefinition*>
            GroupNodeDefinitionMap;
        typedef std::vector<const NodeDefinition*> NodeDefinitionVector;

        NodeManager();
        ~NodeManager();

        //
        //  Loads definitions from a GTO file.
        //

        void loadDefinitions(const std::string& filename);
        void writeAllDefinitions(const std::string& filename,
                                 bool selfContained) const;
        void writeDefinitions(const std::string&, const NodeDefinitionVector&,
                              bool selfContained) const;

        void loadDefinitionsAlongPathVar(const std::string& envValue);

        //
        //  Add a definition (the definition is owned by the NodeManager)
        //

        void addDefinition(NodeDefinition*);

        //
        //  Get a definition
        //

        const NodeDefinition* definition(const std::string& typeName) const;

        //
        //  Create a node from a type name
        //

        IPNode* newNode(const std::string& typeName,
                        const std::string& nodeName, IPGraph* graph,
                        GroupIPNode* group) const;

        template <typename T>
        T* newNodeOfType(const std::string& typeName,
                         const std::string& nodeName, IPGraph* graph,
                         GroupIPNode* group) const
        {
            return dynamic_cast<T*>(newNode(typeName, nodeName, graph, group));
        }

        static void setDebug(bool b) { m_debug = b; }

        const NodeDefinitionMap& definitionMap() const
        {
            return m_definitionMap;
        }

        bool updateDefinition(const std::string& typeName);

    private:
    private:
        NodeDefinitionMap m_definitionMap;
        NodeDefinitionVector m_retiredDefinitions;
        static bool m_debug;
    };

} // namespace IPCore

#endif // __IPCore__NodeManager__h__
