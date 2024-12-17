//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/PaintIPNode.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/StackIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Transform2DIPNode.h>
#include <TwkContainer/Properties.h>
#include <TwkUtil/sgcHop.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;

    bool StackGroupIPNode::m_defaultAutoRetime = true;

    StackGroupIPNode::StackGroupIPNode(const std::string& name,
                                       const NodeDefinition* def,
                                       IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
        declareProperty<StringProperty>("ui.name", name);

        m_retimeToOutput = declareProperty<IntProperty>(
            "timing.retimeInputs", m_defaultAutoRetime ? 1 : 0);

        m_stackNode = newMemberNodeOfType<StackIPNode>(stackType(), "stack");
        m_paintNode = newMemberNodeOfType<PaintIPNode>(paintType(), "paint");

        m_paintNode->setInputs1(m_stackNode);
        m_stackNode->setFitInputsToOutputAspect(true);
        setRoot(m_paintNode);

        m_markersIn = declareProperty<IntProperty>("markers.in");
        m_markersOut = declareProperty<IntProperty>("markers.out");
        m_markersColor = declareProperty<Vec4fProperty>("markers.color");
        m_markersName = declareProperty<StringProperty>("markers.name");
    }

    StackGroupIPNode::~StackGroupIPNode()
    {
        //
        //  The GroupIPNode will delete m_root
        //
    }

    string StackGroupIPNode::retimeType()
    {
        return definition()->stringValue("defaults.retimeType", "Retime");
    }

    string StackGroupIPNode::transformType()
    {
        return definition()->stringValue("defaults.transformType",
                                         "Transform2D");
    }

    string StackGroupIPNode::stackType()
    {
        return definition()->stringValue("defaults.stackType", "Stack");
    }

    string StackGroupIPNode::paintType()
    {
        return definition()->stringValue("defaults.paintType", "Paint");
    }

    IPNode* StackGroupIPNode::newSubGraphForInput(size_t index,
                                                  const IPNodes& newInputs)
    {
        float fps = m_stackNode->imageRangeInfo().fps;
        if (fps == 0.0 && !newInputs.empty())
        {
            auto const imageRangeInfo = newInputs[index]->imageRangeInfo();
            if (!imageRangeInfo.isUndiscovered)
                fps = imageRangeInfo.fps;
        }

        bool retimeInputs = m_retimeToOutput->front() ? true : false;
        IPNode* innode = newInputs[index];
        AdaptorIPNode* anode = newAdaptorForInput(innode);
        Transform2DIPNode* tnode =
            newMemberNodeOfTypeForInput<Transform2DIPNode>(transformType(),
                                                           innode, "t");

        tnode->setAdaptiveResampling(true);
        tnode->setInputs1(anode);
        IPNode* node = tnode;

        if (fps != 0.0 && retimeInputs)
        {
            IPNode* retimer = newMemberNodeForInput(retimeType(), innode, "rt");
            retimer->setInputs1(node);
            retimer->setProperty<FloatProperty>("output.fps", fps);
            node = retimer;
        }

        return node;
    }

    IPNode* StackGroupIPNode::modifySubGraphForInput(size_t index,
                                                     const IPNodes& newInputs,
                                                     IPNode* subgraph)
    {
        bool retimeInputs = m_retimeToOutput->front() ? true : false;

        if (retimeInputs && subgraph->protocol() != retimeType())
        {
            auto const inputRangeInfo = newInputs[index]->imageRangeInfo();
            if (!inputRangeInfo.isUndiscovered)
            {
                IPNode* innode = newInputs[index];
                IPNode* retimer =
                    newMemberNodeForInput(retimeType(), innode, "rt");
                retimer->setInputs1(subgraph);
                retimer->setProperty<FloatProperty>("output.fps",
                                                    inputRangeInfo.fps);
                subgraph = retimer;
            }
        }

        if (subgraph->protocol() == retimeType())
        {
            float retimeFPS = 0.0f;
            if (retimeInputs)
                retimeFPS = m_stackNode->outputFPSProperty()->front();
            else
                retimeFPS = newInputs[index]->imageRangeInfo().fps;

            subgraph->setProperty<FloatProperty>("output.fps", retimeFPS);
            m_stackNode->invalidate();
        }

        return subgraph;
    }

    void StackGroupIPNode::rebuild(int inputIndex)
    {
        if (!isDeleting())
        {
            graph()->beginGraphEdit();
            IPNodes cachedInputs = inputs();
            if (inputIndex >= 0)
                setInputsWithReordering(cachedInputs, m_stackNode, inputIndex);
            else
                setInputs(cachedInputs);

            graph()->endGraphEdit();
        }
    }

    void StackGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        HOP_PROF_FUNC();
        if (isDeleting())
        {
            IPNode::setInputs(newInputs);
        }
        else
        {
            setInputsWithReordering(newInputs, m_stackNode);
        }
    }

    void StackGroupIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_retimeToOutput || p == m_stackNode->outputFPSProperty())
            {
                rebuild();
                propagateRangeChange();
                if (p == m_stackNode->outputFPSProperty())
                    return;
            }
        }

        GroupIPNode::propertyChanged(p);
    }

    void StackGroupIPNode::inputMediaChanged(IPNode* srcNode, int srcOutIndex,
                                             PropagateTarget target)
    {
        IPNode::inputMediaChanged(srcNode, srcOutIndex, target);

        auto inputIndex = mapToInputIndex(srcNode, srcOutIndex);

        if (inputIndex < 0)
        {
            return;
        }

        auto input = m_stackNode->inputs()[inputIndex];

        if (input->protocol() == "RVRetime")
        {
            if (auto retime = dynamic_cast<RetimeIPNode*>(input))
                retime->resetFPS();
        }

        rebuild(inputIndex);
    }

} // namespace IPCore
