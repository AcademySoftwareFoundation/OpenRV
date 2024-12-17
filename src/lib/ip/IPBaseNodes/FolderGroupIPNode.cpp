//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/FolderGroupIPNode.h>
#include <IPBaseNodes/SwitchIPNode.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/LayoutGroupIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/NodeDefinition.h>
#include <TwkUtil/File.h>
#include <TwkContainer/Properties.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;

    FolderGroupIPNode::FolderGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
        declareProperty<StringProperty>("ui.name", name);
        m_viewType = declareProperty<StringProperty>("mode.viewType", "switch");

        //
        //  This is wastefull, but simple: the types and viewType values
        //  used by the folder are taken from the defaults in the node
        //  definition. This way RV can use RVStack, RVSwitch, etc, and
        //  non-RV programs can use their own versions of these nodes or
        //  something else entirely.
        //

        const Component* defaults = def->component("folder");
        assert(defaults);
        const Component::Container& props = defaults->properties();

        for (size_t i = 0; i < props.size(); i++)
        {
            if (const StringProperty* sp =
                    dynamic_cast<const StringProperty*>(props[i]))
            {
                if (sp->size() == 1)
                {
                    m_internalNodes.push_back(
                        InternalNodeEntry(sp->front(), sp->name()));
                }
            }
        }

        updateViewType();
    }

    FolderGroupIPNode::~FolderGroupIPNode()
    {
        //
        //  The GroupIPNode will delete m_root
        //
    }

    IPNode* FolderGroupIPNode::newSubGraphForInput(size_t index,
                                                   const IPNodes& newInputs)
    {
        IPNode* innode = newInputs[index];
        return newAdaptorForInput(innode);
    }

    void FolderGroupIPNode::rebuild()
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

    void FolderGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        if (isDeleting())
        {
            IPNode::setInputs(newInputs);
        }
        else
        {
            updateViewType();
            setInputsWithReordering(newInputs, rootNode());
        }
    }

    void FolderGroupIPNode::updateViewType()
    {
        const string& view = m_viewType->front();

        for (size_t i = 0; i < m_internalNodes.size(); i++)
        {
            InternalNodeEntry& entry = m_internalNodes[i];

            if (view == entry.value
                && (!rootNode() || rootNode() != entry.node))
            {
                if (!entry.node)
                    entry.node = newMemberNode(entry.type, entry.value);
                if (rootNode())
                    entry.node->setInputs(rootNode()->inputs());
                setRoot(entry.node);
            }
        }
    }

    void FolderGroupIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
#if 0
        //
        //  None of this stuff should be happening here. If its needed
        //  than something else is wrong.
        //

        if (m_switchNode)
        {
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
#endif

            if (p == m_viewType)
            {
                updateViewType();
            }
        }

        GroupIPNode::propertyChanged(p);
    }

} // namespace IPCore
