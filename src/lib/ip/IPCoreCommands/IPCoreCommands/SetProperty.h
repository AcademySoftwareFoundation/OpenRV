//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__SetProperty__h__
#define __IPCoreCommands__SetProperty__h__
#include <TwkContainer/PropertyContainer.h>
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <IPCore/PropertyEditor.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  SetProperty
        //
        //  Create a new node from a type name
        //

        template <class T> class SetProperty : public TwkApp::Command
        {
        public:
            SetProperty(const TwkApp::CommandInfo* i)
                : TwkApp::Command(i)
            {
            }

            virtual ~SetProperty() {}

            typedef T Property;
            typedef typename T::container_type Container;
            typedef typename T::value_type ValueType;
            typedef std::vector<ValueType> ValueTypeVector;
            typedef typename Property::Layout Layout;
            typedef std::vector<Container> ContainerVector;
            typedef std::vector<std::string> StringVector;
            typedef PropertyEditor<T> Editor;

            void setArgs(IPGraph* graph, int frame, const std::string& propPath,
                         const ValueType& value);

            virtual void doit();
            virtual void undo();

        private:
            IPGraph* m_graph;
            int m_frame;
            std::string m_propPath;
            ValueType m_value;
            ValueTypeVector m_oldValues;
        };

        template <typename T>
        void SetProperty<T>::setArgs(IPGraph* graph, int frame,
                                     const std::string& propPath,
                                     const ValueType& value)
        {
            m_graph = graph;
            m_propPath = propPath;
            m_frame = frame;
            m_value = value;
        }

        template <typename T> void SetProperty<T>::doit()
        {
            Editor edit(*m_graph, m_frame, m_propPath);
            const typename Editor::PropertyVector& props =
                edit.propertyVector();

            m_oldValues.resize(props.size());

            for (size_t i = 0; i < props.size(); i++)
            {
                T* p = props[i];
                m_oldValues[i] = p->front();
            }
            edit.setValue(m_value);
        }

        template <typename T> void SetProperty<T>::undo()
        {
            Editor edit(*m_graph, m_frame, m_propPath);
            edit.setValue(m_oldValues);
        }

        template <typename T> class SetPropertyInfo : public TwkApp::CommandInfo
        {
        public:
            SetPropertyInfo(const std::string& name)
                : CommandInfo(name)
            {
            }

            virtual ~SetPropertyInfo() {}

            virtual TwkApp::Command* newCommand() const
            {
                return new SetProperty<T>(this);
            }
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__SetProperty__h__
