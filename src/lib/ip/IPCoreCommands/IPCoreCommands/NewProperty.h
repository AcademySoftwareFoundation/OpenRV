//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__NewProperty__h__
#define __IPCoreCommands__NewProperty__h__
#include <TwkContainer/PropertyContainer.h>
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  NewProperty
        //
        //  Creates a new property on a node. The pointer will not survive
        //  undo. This command causes signals from the node both on creation
        //  and undo.
        //

        template <typename T> class NewProperty : public TwkApp::Command
        {
        public:
            NewProperty(const TwkApp::CommandInfo* info)
                : TwkApp::Command(info)
                , m_prop(0)
            {
            }

            virtual ~NewProperty()
            {
                if (m_prop)
                    m_prop->unref();
            }

            typedef T Property;
            typedef typename Property::Layout Layout;

            void setArgs(IPGraph* graph, const std::string& nodeName,
                         const std::string& propPath);

            virtual void doit();
            virtual void undo();

            Property* property() const { return m_prop; }

        private:
            IPGraph* m_graph;
            std::string m_nodeName;
            std::string m_propPath;
            Property* m_prop;
        };

        template <typename T>
        void NewProperty<T>::setArgs(IPGraph* graph, const std::string& node,
                                     const std::string& propPath)
        {
            m_graph = graph;
            m_nodeName = node;
            m_propPath = propPath;

            if (!m_graph->findNode(m_nodeName))
            {
                TWK_THROW_EXC_STREAM("NewProperty: unknown node "
                                     << m_nodeName);
            }
        }

        template <typename T> void NewProperty<T>::doit()
        {
            if (IPNode* node = m_graph->findNode(m_nodeName))
            {
                m_prop = node->createProperty<Property>(m_propPath);
                m_prop->ref();
                node->newPropertySignal()(m_prop);
            }
        }

        template <typename T> void NewProperty<T>::undo()
        {
            if (IPNode* node = m_graph->findNode(m_nodeName))
            {
                node->propertyWillBeDeletedSignal()(m_prop);
                node->removeProperty(m_prop);
                node->propertyDeletedSignal()(m_propPath);
                m_prop->unref();
                m_prop = 0;
            }
        }

        template <typename T> class NewPropertyInfo : public TwkApp::CommandInfo
        {
        public:
            NewPropertyInfo(const std::string& name)
                : CommandInfo(name)
            {
            }

            virtual ~NewPropertyInfo() {}

            virtual TwkApp::Command* newCommand() const
            {
                return new NewProperty<T>(this);
            }
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__NewProperty__h__
