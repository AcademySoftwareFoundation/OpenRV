//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__PropertyEdit__h__
#define __IPCore__PropertyEdit__h__
#include <iostream>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>

namespace IPCore
{

    //
    //  This should be used by external code when editing property values
    //  (e.g. interface code). The property change is notification occurs
    //  when the PropertyEdit is destroyed.
    //
    //  For an undoable SetProperty see IPCoreCommands/SetProperty.h which
    //  uses this class.
    //
    //  For example:
    //
    //  PropertyEditor<FloatProperty> editor(graph(),
    //  "#RVSoundTrack.audio.volume");
    //      -or-
    //  FloatPropertyEditor editor(graph(), "#RVSoundTrack.audio.volume");
    //      -or-
    //  FloatPropertyEditor editor(graph(), floatPropPointer)
    //  edit.setValue( edit.value() * 2.0 );
    //
    //  NOTE: the frame parameter in the constructor is used when
    //  identifying nodes by type. Nodes are searched for in evaluation
    //  order.
    //

    template <class T> class PropertyEditor
    {
    public:
        typedef T PropertyType;
        typedef typename PropertyType::value_type ValueType;
        typedef typename PropertyType::container_type ContainerType;
        typedef std::vector<ContainerType> ContainerVector;
        typedef std::vector<PropertyType*> PropertyVector;
        typedef IPGraph::PropertyVector UntypedPropertyVector;

        PropertyEditor(IPGraph&, int frame, const std::string& propertyName);
        PropertyEditor(IPNode*, const std::string& propertyName);
        PropertyEditor(T*, IPNode* node = 0);
        PropertyEditor(IPGraph&, const UntypedPropertyVector&);
        ~PropertyEditor();

        ValueType value();
        ContainerType& container();

        void setValue(const ValueType& v);
        void setValue(const ContainerType&);
        void setValue(const ContainerVector&);

        const PropertyVector& propertyVector() const { return m_props; }

    private:
        IPGraph* m_graph;
        PropertyVector m_props;
    };

    template <class T>
    PropertyEditor<T>::PropertyEditor(IPNode* node, const std::string& propName)
    {
        T* prop = node->property<T>(propName);
        if (prop)
            m_props.push_back(prop);
        m_graph = node->graph();
    }

    template <class T> PropertyEditor<T>::PropertyEditor(T* prop, IPNode* node)
    {
        IPNode* n = node ? node : dynamic_cast<IPNode*>(prop->container());
        if (prop)
            m_props.push_back(prop);
        m_graph = n ? n->graph() : 0;
    }

    template <class T>
    PropertyEditor<T>::PropertyEditor(IPGraph& graph, int frame,
                                      const std::string& name)
    {
        graph.findPropertyOfType<T>(frame, m_props, name);
        if (m_props.empty())
            TWK_THROW_STREAM(BadPropertyExc, "Unknown property " << name);
        m_graph = &graph;
    }

    template <class T>
    PropertyEditor<T>::PropertyEditor(IPGraph& graph,
                                      const UntypedPropertyVector& propVector)
    {
        size_t s = propVector.size();
        m_props.resize(s);

        for (size_t i = 0; i < s; i++)
        {
            T* p = dynamic_cast<T*>(propVector[i]);
            if (!p)
                TWK_THROW_STREAM(BadPropertyExc, "Bad property type");
            m_props[i] = p;
        }

        if (m_props.empty())
            TWK_THROW_STREAM(BadPropertyExc, "No properties");
        m_graph = &graph;
    }

    template <class T> PropertyEditor<T>::~PropertyEditor() {}

    template <class T>
    typename PropertyEditor<T>::ValueType PropertyEditor<T>::value()
    {
        for (size_t i = 0; i < m_props.size(); i++)
        {
            if (m_props[i]->size() == 0)
                m_props[i]->resize(1);
        }

        // Returns the value of the first prop since its ambiguous
        return m_props[0]->front();
    }

    template <class T>
    typename PropertyEditor<T>::ContainerType& PropertyEditor<T>::container()
    {
        // can only return the first prop's container
        return m_props[0]->valueContainer();
    }

    template <class T>
    void
    PropertyEditor<T>::setValue(const typename PropertyEditor<T>::ValueType& v)
    {
        IPGraph::GraphEdit edit(*m_graph, m_props);

        for (size_t i = 0; i < m_props.size(); i++)
        {
            IPNode* node = dynamic_cast<IPNode*>(m_props[i]->container());
            if (m_props[i]->size() == 1 && m_props[i]->front() == v)
                continue;

            if (node)
                node->propertyWillChange(m_props[i]);

            m_props[i]->resize(1);
            m_props[i]->front() = v;

            if (node)
                node->propertyChanged(m_props[i]);
        }
    }

    template <class T>
    void PropertyEditor<T>::setValue(const ContainerVector& container)
    {
        IPGraph::GraphEdit edit(*m_graph, m_props);
        assert(container.size() == m_props.size());

        for (size_t i = 0; i < m_props.size(); i++)
        {
            IPNode* node = dynamic_cast<IPNode*>(m_props[i]->container());
            if (node)
                node->propertyWillChange(m_props[i]);
            m_props[i]->valueContainer() = container[i];
            if (node)
                node->propertyChanged(m_props[i]);
        }
    }

    template <class T>
    void PropertyEditor<T>::setValue(
        const typename PropertyEditor<T>::ContainerType& v)
    {
        IPGraph::GraphEdit edit(*m_graph, m_props);

        for (size_t i = 0; i < m_props.size(); i++)
        {
            IPNode* node = dynamic_cast<IPNode*>(m_props[i]->container());

            if (node)
                node->propertyWillChange(m_props[i]);

            m_props[i]->resize(v.size());
            std::copy(v.begin(), v.end(), m_props[i]->valueContainer().begin());

            if (node)
                node->propertyChanged(m_props[i]);
        }
    }

    typedef PropertyEditor<TwkContainer::StringProperty> StringPropertyEditor;
    typedef PropertyEditor<TwkContainer::IntProperty> IntPropertyEditor;
    typedef PropertyEditor<TwkContainer::FloatProperty> FloatPropertyEditor;
    typedef PropertyEditor<TwkContainer::HalfProperty> HalfPropertyEditor;
    typedef PropertyEditor<TwkContainer::Vec4fProperty> Vec4fPropertyEditor;
    typedef PropertyEditor<TwkContainer::Vec3fProperty> Vec3fPropertyEditor;
    typedef PropertyEditor<TwkContainer::Vec2fProperty> Vec2fPropertyEditor;
    typedef PropertyEditor<TwkContainer::Mat44fProperty> Mat44fPropertyEditor;

} // namespace IPCore

#endif // __IPCore__PropertyEdit__h__
