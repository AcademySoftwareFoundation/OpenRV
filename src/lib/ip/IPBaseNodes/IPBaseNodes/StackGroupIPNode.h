//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__StackGroupIPNode__h__
#define __IPGraph__StackGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class StackIPNode;
    class PaintIPNode;

    /// StackGroupIPNode manages a sub-graph that includes a stack at the root

    ///
    /// The sub-graph contains one compositing node as the root and
    /// possibly a wipe at each input.
    ///

    class StackGroupIPNode : public GroupIPNode
    {
    public:
        StackGroupIPNode(const std::string& name, const NodeDefinition* def,
                         IPGraph* graph, GroupIPNode* group = 0);

        virtual ~StackGroupIPNode();

        virtual void setInputs(const IPNodes&) override;
        virtual IPNode* newSubGraphForInput(size_t, const IPNodes&) override;
        virtual IPNode* modifySubGraphForInput(size_t index,
                                               const IPNodes& newInputs,
                                               IPNode* subgraph) override;
        virtual void propertyChanged(const Property*) override;

        StackIPNode* stackNode() const { return m_stackNode; }

        static void setDefaultAutoRetime(bool b) { m_defaultAutoRetime = b; }

    protected:
        void inputMediaChanged(IPNode* srcNode, int srcOutIndex,
                               PropagateTarget target) override;

    private:
        std::string transformType();
        std::string stackType();
        std::string retimeType();
        std::string paintType();
        void rebuild(int inputIndex = -1);

    private:
        StackIPNode* m_stackNode;
        PaintIPNode* m_paintNode;
        IntProperty* m_retimeToOutput;

        IntProperty* m_markersIn;
        IntProperty* m_markersOut;
        Vec4fProperty* m_markersColor;
        StringProperty* m_markersName;

        static bool m_defaultAutoRetime;
    };

} // namespace IPCore

#endif // __IPGraph__StackGroupIPNode__h__
