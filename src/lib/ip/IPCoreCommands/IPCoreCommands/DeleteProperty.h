//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__DeleteProperty__h__
#define __IPCoreCommands__DeleteProperty__h__
#include <TwkContainer/PropertyContainer.h>
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  DeleteProperty
        //
        //  Deletes a property on a node. The pointer will not survive
        //  undo. This command causes signals from the node both on
        //  destruction and undo (reinsertion).
        //

        template <typename T> class DeleteProperty : public TwkApp::Command
        {
        public:
            DeleteProperty(const TwkApp::CommandInfo* i)
                : TwkApp::Command(i)
                , m_prop(0)
            {
            }

            virtual ~DeleteProperty()
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

        private:
            IPGraph* m_graph;
            std::string m_nodeName;
            std::string m_propPath;
            Property* m_prop;
        };

        template <typename T>
        void DeleteProperty<T>::setArgs(IPGraph* graph, const std::string& node,
                                        const std::string& propPath)
        {
            m_graph = graph;
            m_nodeName = node;
            m_propPath = propPath;

            if (!m_graph->findNode(m_nodeName))
            {
                TWK_THROW_EXC_STREAM("DeleteProperty: unknown node "
                                     << m_nodeName);
            }
        }

        template <typename T> void DeleteProperty<T>::doit()
        {
            if (IPNode* node = m_graph->findNode(m_nodeName))
            {
                m_prop = node->createProperty<Property>(m_propPath);
                m_prop->ref();
                node->propertyWillBeDeletedSignal()(m_prop);
                node->removeProperty(m_prop);
                node->propertyDeletedSignal()(m_propPath);
            }
        }

        template <typename T> void DeleteProperty<T>::undo()
        {
            if (IPNode* node = m_graph->findNode(m_nodeName))
            {
                Property* newProp = node->createProperty<Property>(m_propPath);
                node->newPropertySignal()(newProp);
                newProp->copy(m_prop);
                m_prop->unref();
                m_prop = 0;
            }
        }

        template <typename T>
        class DeletePropertyInfo : public TwkApp::CommandInfo
        {
        public:
            DeletePropertyInfo(const std::string& name)
                : CommandInfo(name)
            {
            }

            virtual ~DeletePropertyInfo() {}

            virtual TwkApp::Command* newCommand() const
            {
                return new DeleteProperty<T>(this);
            }
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__DeleteProperty__h__
