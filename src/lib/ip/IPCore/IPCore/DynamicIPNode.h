//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__DynamicIPNode__h__
#define __IPCore__DynamicIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{

    namespace Shader
    {
        class Function;
    }

    /// DynamicIPNode -- base class for nodes with definition that changes
    /// frequently at runtime.

    ///
    /// DynamicIPNode holds source code which is automatically made into a
    /// NodeDefinition on the fly. The NodeDefinition is not managed and
    /// is deleted and recreated each time source code changes. The
    /// DynamicIPNode is actually a group which manages a single
    /// IPInstance node of whatever the shader generates. This could be
    /// e.g ColorIPInstanceNode, FilterIPInstanceNode, or
    /// StackIPInstanceNode. The DynamicIPNode shields the graph from its
    /// internal managed node by sharing the internal node's parameters.
    ///
    /// Its possible to convert any IPInstanceNode into a DynamicIPNode as
    /// an editing operation. When this occurs, the parameters and source
    /// code for the IPInstanceNode is copied into a new DynamicIPNode and
    /// a new unmanaged NodeDefinition is created.
    ///
    /// At some point, the DynamicIPNode can convert its internal
    /// NodeDefinition into a properly named and signed NodeDefinition
    /// which can become managed allowing the node contents to be
    /// instantiated.
    ///

    class DynamicIPNode : public GroupIPNode
    {
    public:
        typedef Shader::Function Function;

        DynamicIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph* graph, GroupIPNode* group = 0);

        virtual ~DynamicIPNode();

        virtual void setInputs(const IPNodes&);
        virtual void propertyChanged(const Property*);
        virtual void readCompleted(const std::string&, unsigned int);

        std::string dynamicName() const;

        std::string stringValue(const std::string& name) const;
        StringProperty* stringProp(const std::string& name);
        void setStringValue(const std::string& name, const std::string& value,
                            bool withNotification = false);

        int intValue(const std::string& name) const;
        IntProperty* intProp(const std::string& name);
        void setIntValue(const std::string& name, int value,
                         bool withNotification = false);

        bool outputError() const;
        bool outputValid() const;
        std::string outputMessage() const;
        std::string outputFunctionName() const;
        std::string outputFunctionCallName() const;

        void publish(const std::string& filename);

        const NodeDefinition* nodeDefinition() const { return m_definition; }

    private:
        void rebuild();
        void setOutput(bool, bool, const std::string&);
        void setOutputNames(const std::string&, const std::string&);

    private:
        StringProperty* m_glslValid;
        StringProperty* m_glslCurrent;
        StringProperty* m_definitionName;
        IntProperty* m_outputError;
        IntProperty* m_outputValid;
        StringProperty* m_outputMessage;
        StringProperty* m_outputMain;
        StringProperty* m_outputCallMain;
        NodeDefinition* m_definition;
        IPNode* m_internalNode;
        Component* m_internalParameters;
        Component* m_parameters;
    };

} // namespace IPCore

#endif // __IPCore__DynamicIPNode__h__
