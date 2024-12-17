//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__LayoutGroupIPNode__h__
#define __IPGraph__LayoutGroupIPNode__h__
#include <iostream>
#include <atomic>
#include <mutex>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class StackIPNode;
    class PaintIPNode;

    /// LayoutGroupIPNode manages a sub-graph that includes a stack at the root

    ///
    /// The sub-graph contains one stack node as the root and a transform
    /// at each input.
    ///

    class LayoutGroupIPNode : public GroupIPNode
    {
    public:
        LayoutGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~LayoutGroupIPNode();

        virtual void setInputs(const IPNodes&) override;
        virtual IPNode* newSubGraphForInput(size_t, const IPNodes&) override;
        virtual IPNode* modifySubGraphForInput(size_t, const IPNodes&,
                                               IPNode*) override;
        virtual void propertyChanged(const Property*) override;
        virtual void
        inputImageStructureChanged(int inputIndex,
                                   PropagateTarget target) override;

        StackIPNode* stackNode() const { return m_stackNode; }

        static void setDefaultMode(const std::string& mode)
        {
            m_defaultMode = mode;
        }

        void readCompleted(const std::string&, unsigned int) override;
        IPImage* evaluate(const Context& context) override;

        void inputMediaChanged(IPNode* srcNode, int srcOutIndex,
                               PropagateTarget target) override;

        static void setDefaultAutoRetime(bool b) { m_defaultAutoRetime = b; }

    protected:
        void rebuild(int inputIndex = -1);

        inline void requestLayout()
        {
            std::lock_guard<std::mutex> guard(m_layoutMutex);

            m_layoutRequested = true;
        }

        inline void layoutIfRequested()
        {
            std::lock_guard<std::mutex> guard(m_layoutMutex);
            if (m_layoutRequested)
            {
                layout();
                m_layoutRequested = false;
            }
        }

    private:
        std::string transformType();
        std::string stackType();
        std::string retimeType();
        std::string paintType();
        void layout();

    private:
        StackIPNode* m_stackNode;
        PaintIPNode* m_paintNode;
        StringProperty* m_mode;
        IntProperty* m_retimeToOutput;
        IntProperty* m_gridColumns;
        IntProperty* m_gridRows;
        FloatProperty* m_spacing;
        std::string m_lastMode;
        std::mutex m_layoutMutex;
        bool m_layoutRequested;

        static std::string m_defaultMode;
        static bool m_defaultAutoRetime;
    };

} // namespace IPCore

#endif // __IPGraph__LayoutGroupIPNode__h__
