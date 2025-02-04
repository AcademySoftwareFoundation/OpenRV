//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/PaintIPNode.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPBaseNodes/SwitchGroupIPNode.h>
#include <IPBaseNodes/SwitchIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Transform2DIPNode.h>
#include <TwkContainer/Properties.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;

    SwitchGroupIPNode::SwitchGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
        declareProperty<StringProperty>("ui.name", name);
        string switchType =
            definition()->stringValue("defaults.switchType", "Switch");
        m_switchNode = newMemberNodeOfType<SwitchIPNode>(switchType, "switch");
        setRoot(m_switchNode);

        m_markersIn = declareProperty<IntProperty>("markers.in");
        m_markersOut = declareProperty<IntProperty>("markers.out");
        m_markersColor = declareProperty<Vec4fProperty>("markers.color");
        m_markersName = declareProperty<StringProperty>("markers.name");
    }

    SwitchGroupIPNode::~SwitchGroupIPNode()
    {
        //
        //  The GroupIPNode will delete m_root
        //
    }

    IPNode* SwitchGroupIPNode::newSubGraphForInput(size_t index,
                                                   const IPNodes& newInputs)
    {
        IPNode* innode = newInputs[index];
        return newAdaptorForInput(innode);
    }

    void SwitchGroupIPNode::rebuild()
    {
        if (!isDeleting())
        {
            graph()->beginGraphEdit();
            IPNodes cachedInputs = inputs();
            IPNodes emptyInputs;
            setInputs(emptyInputs);
            setInputs(cachedInputs);

            graph()->endGraphEdit();
        }
    }

    void SwitchGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        if (isDeleting())
        {
            IPNode::setInputs(newInputs);
        }
        else
        {
            setInputsWithReordering(newInputs, m_switchNode);
        }
    }

#if 0
void
SwitchGroupIPNode::propertyChanged(const Property* p)
{
    if (!isDeleting())
    {
        //
        //  None of this stuff should be happening here. If its needed
        //  than something else is wrong. See also FolderGroupIPNode
        //

        if (p == m_switchNode->outputFPSProperty()) 
        {
            rebuild(); // special behavior of switch
            return;
        }
        
        if (p == m_switchNode->outputSizeProperty() ||
            p == m_switchNode->autoSizeProperty())
        {
            return;
        }
    }

    GroupIPNode::propertyChanged(p);
}
#endif

} // namespace IPCore
