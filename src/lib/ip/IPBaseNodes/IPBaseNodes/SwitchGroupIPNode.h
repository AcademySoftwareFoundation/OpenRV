//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__SwitchGroupIPNode__h__
#define __IPGraph__SwitchGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class SwitchIPNode;
    class CompositeIPNode;
    class PaintIPNode;

    /// SwitchGroupIPNode manages a sub-graph that includes a switch node at the
    /// root

    ///
    /// The sub-graph contains one switch node as the root
    ///

    class SwitchGroupIPNode : public GroupIPNode
    {
    public:
        SwitchGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~SwitchGroupIPNode();
        virtual void setInputs(const IPNodes&);
        virtual IPNode* newSubGraphForInput(size_t, const IPNodes&);

        SwitchIPNode* switchNode() const { return m_switchNode; }

        void rebuild();

    private:
        SwitchIPNode* m_switchNode;

        IntProperty* m_markersIn;
        IntProperty* m_markersOut;
        Vec4fProperty* m_markersColor;
        StringProperty* m_markersName;
    };

} // namespace IPCore

#endif // __IPGraph__SwitchGroupIPNode__h__
