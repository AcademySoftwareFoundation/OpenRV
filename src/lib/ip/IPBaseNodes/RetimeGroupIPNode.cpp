//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/PaintIPNode.h>
#include <IPBaseNodes/RetimeGroupIPNode.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <TwkContainer/Properties.h>
#include <TwkUtil/File.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;

    RetimeGroupIPNode::RetimeGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
        declareProperty<StringProperty>("ui.name", name);
        m_retimeNode =
            newMemberNodeOfType<RetimeIPNode>(retimeType(), "retime");
        setMaxInputs(m_retimeNode->maximumNumberOfInputs());
        setRoot(m_retimeNode);
    }

    RetimeGroupIPNode::~RetimeGroupIPNode()
    {
        //
        //  The GroupIPNode will delete the root
        //
    }

    string RetimeGroupIPNode::retimeType()
    {
        return definition()->stringValue("defaults.retimeType", "Retime");
    }

    string RetimeGroupIPNode::paintType()
    {
        return definition()->stringValue("defaults.paintType", "Paint");
    }

    IPNode* RetimeGroupIPNode::newSubGraphForInput(size_t index,
                                                   const IPNodes& newInputs)
    {
        IPNode* innode = newInputs[index];
        AdaptorIPNode* anode = newAdaptorForInput(innode);
        PaintIPNode* paintNode =
            newMemberNodeOfTypeForInput<PaintIPNode>(paintType(), innode, "p");

        paintNode->setInputs1(anode);
        return paintNode;
    }

    void RetimeGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        if (isDeleting())
        {
            IPNode::setInputs(newInputs);
        }
        else
        {
            setInputsWithReordering(newInputs, m_retimeNode);
        }
    }

} // namespace IPCore
