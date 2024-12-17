//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__InsertProperty__h__
#define __IPCoreCommands__InsertProperty__h__
#include <TwkContainer/PropertyContainer.h>
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  InsertProperty
        //
        //  Create a new node from a type name
        //

        template <class T> class InsertProperty : public TwkApp::Command
        {
        public:
            InsertProperty(const TwkApp::CommandInfo* i)
                : TwkApp::Command(i)
            {
            }

            virtual ~InsertProperty() {}

            typedef TwkContainer::Property Property;
            typedef typename T::container_type Container;
            typedef typename Property::Layout Layout;

            void setArgs(IPGraph* graph, const std::string& nodeName,
                         const std::string& propPath, Container& value,
                         size_t index);

            virtual void doit();
            virtual void undo();

        private:
            IPGraph* m_graph;
            std::string m_nodeName;
            std::string m_propPath;
            Container m_value;
            size_t m_index;
        };

        template <typename T>
        void InsertProperty<T>::setArgs(IPGraph* graph,
                                        const std::string& nodeName,
                                        const std::string& propPath,
                                        Container& value, size_t index)
        {
            m_graph = graph;
            m_value = value;
            m_nodeName = nodeName;
            m_propPath = propPath;
            m_index = index;

            if (!m_graph->findNode(m_nodeName))
            {
                TWK_THROW_EXC_STREAM("NewProperty: unknown node "
                                     << m_nodeName);
            }
        }

        template <typename T> void InsertProperty<T>::doit()
        {
            if (IPNode* node = m_graph->findNode(m_nodeName))
            {
                if (T* prop = node->property<T>(m_propPath))
                {
                    node->propertyWillChangeSignal()(prop);
                    node->propertyWillInsertSignal()(prop, m_index,
                                                     m_value.size());
                    Container& c = prop->valueContainer();
                    c.insert(c.begin() + m_index, m_value.begin(),
                             m_value.end());
                    node->propertyDidInsertSignal()(prop, m_index,
                                                    m_value.size());
                    node->propertyChangedSignal()(prop);
                }
            }
        }

        template <typename T> void InsertProperty<T>::undo()
        {
            if (IPNode* node = m_graph->findNode(m_nodeName))
            {
                if (T* prop = node->property<T>(m_propPath))
                {
                    node->propertyWillChangeSignal()(prop);
                    Container& c = prop->valueContainer();
                    c.erase(c.begin() + m_index,
                            c.begin() + m_index + m_value.size());
                    node->propertyChangedSignal()(prop);
                }
            }
        }

        template <typename T>
        class InsertPropertyInfo : public TwkApp::CommandInfo
        {
        public:
            InsertPropertyInfo(const std::string& name)
                : CommandInfo(name)
            {
            }

            virtual ~InsertPropertyInfo() {}

            virtual TwkApp::Command* newCommand() const
            {
                return new InsertProperty<T>(this);
            }
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__InsertProperty__h__
