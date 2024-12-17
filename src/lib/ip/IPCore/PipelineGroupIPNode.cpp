//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPProperty.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/IPGraph.h>
#include <algorithm>

namespace IPCore
{
    using namespace std;

    namespace
    {
        PropertyInfo* nodesInfo = 0;

        bool equals(const IPNode::StringVector& a,
                    const IPNode::StringVector& b)
        {
            const size_t asize = a.size();
            const size_t bsize = b.size();

            if (asize != bsize)
                return false;

            for (size_t i = 0; i < asize; i++)
            {
                if (a[i] != b[i])
                    return false;
            }

            return true;
        }

    } // namespace

    PipelineGroupIPNode::PipelineGroupIPNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
        if (!nodesInfo)
        {
            //
            //  NOTE: we can't buid graph in more than one thread and
            //  because DLL static data doesn't seem to get initialized
            //  reliably we need to build these lazily
            //

            nodesInfo = new PropertyInfo(
                PropertyInfo::Persistent | PropertyInfo::RequiresGraphEdit, 1);
        }

        setMaxInputs(1);
        m_nodeTypeNames =
            declareProperty<StringProperty>("pipeline.nodes", nodesInfo);
        m_nodeTypeNames->valueContainer() =
            def->stringArrayValue("defaults.pipeline");
    }

    PipelineGroupIPNode::~PipelineGroupIPNode()
    {
        // base class will handle it
    }

    void PipelineGroupIPNode::setInputs(const IPNodes& inputs)
    {
        GroupIPNode::setInputs(inputs);

        if (inputs.empty())
        {
            clearPipeline();
        }
        else if (!m_nodes.empty())
        {
            if (AdaptorIPNode* anode =
                    dynamic_cast<AdaptorIPNode*>(m_nodes.back()))
            {
                anode->setGroupInputNode(inputs[0]);
            }
            else
            {
                buildPipeline();
            }
        }
        else
        {
            buildPipeline();
        }
    }

    void PipelineGroupIPNode::clearPipeline()
    {
        //
        //  deleting the root will delete everything including the adaptor
        //  node.
        //

        delete rootNode();
        setRoot(0);
        m_nodes.clear();
    }

    void PipelineGroupIPNode::buildPipeline()
    {
        if (inputs().empty())
        {
            clearPipeline();
            return;
        }

        //
        //  Try and reuse existing nodes -- this way appending/removing or
        //  reordering will not affect the existing node states. E.g. if
        //  you edited the nodes in the pipeline and then reordered it all
        //  of the edits would remain and only the ordering would change.
        //
        //  Remove the old nodes from the graph before creating new ones. This
        //  makes it highly probable that the node names will be similar
        //  (reused) and not cause a succession of names like x00001 x00002
        //  x00003 each time the pipeline is rebuilt
        //

        IPNodes oldNodes;

        for (size_t i = 0; i < m_nodes.size(); i++)
        {
            const size_t index = m_nodes.size() - 1 - i;
            IPNode* node = m_nodes[index];
            node->disconnectInputs();
            graph()->removeNode(node);
            remove(node);

            if (index > 0)
                oldNodes.push_back(node); // don't reuse the AdaptorIPNode
            else
                delete node;
        }

        m_nodes.clear();
        setRoot(0);

        //
        //  Build the pipeline from scratch, reuse the old nodes by
        //  copying their state instead of trying to reuse them
        //  directly. This means that the node names are allowed to change
        //  based on their positioning, but the state can move. (e.g. foo1
        //  of the BAR may become foo2 after this)
        //

        IPNode* innode = inputs().front();
        AdaptorIPNode* anode = newAdaptorForInput(innode);
        m_nodes.push_back(anode);

        for (size_t i = 0; i < m_nodeTypeNames->size(); i++)
        {
            const string& typeName = (*m_nodeTypeNames)[i];
            ostringstream suffix;
            suffix << i;

            if (IPNode* node = newMemberNode(typeName, suffix.str()))
            {
                node->setInputs1(m_nodes.back());
                m_nodes.push_back(node);

                //
                //  Search for a node of the same type in the oldNodes and
                //  if found copy its state to the new one
                //

                for (size_t q = 0; q < oldNodes.size(); q++)
                {
                    // Make sure to scan the old nodes in the correct order,
                    // otherwise nodes of the same type might end up in a
                    // different order.
                    // oldNodes are ordered from last node to first node
                    const size_t index = oldNodes.size() - 1 - q;
                    IPNode* oldNode = oldNodes[index];

                    if (oldNode && oldNode->protocol() == typeName)
                    {
                        node->copyNode(oldNode);
                        delete oldNode;
                        oldNodes[index] = 0;
                        break;
                    }
                }
            }
            else
            {
                cout << "ERROR: PipelineGroup " << name()
                     << " cannot create node of type \"" << typeName
                     << "\": ignoring" << endl;
            }
        }

        for (size_t q = 0; q < oldNodes.size(); q++)
        {
            delete oldNodes[q];
        }

        //
        //  Delete the remaining oldNodes
        //

        setRoot(m_nodes.back());
    }

    void PipelineGroupIPNode::propertyChanged(const Property* p)
    {
        if (p == m_nodeTypeNames)
            buildPipeline();
        GroupIPNode::propertyChanged(p);
    }

    void PipelineGroupIPNode::readCompleted(const string& type,
                                            unsigned int version)
    {
        buildPipeline();
        GroupIPNode::readCompleted(type, version);
    }

    IPNode::StringVector PipelineGroupIPNode::pipeline() const
    {
        return m_nodeTypeNames->valueContainer();
    }

    void PipelineGroupIPNode::setPipeline(const StringVector& array)
    {
        if (equals(array, m_nodeTypeNames->valueContainer()))
            return;

        m_nodeTypeNames->resize(array.size());
        m_nodeTypeNames->valueContainer() = array;
        buildPipeline();
    }

    void PipelineGroupIPNode::setPipeline1(const string& nodeType)
    {
        StringVector newPipeline(1);
        newPipeline[0] = nodeType;
        setPipeline(newPipeline);
    }

    void PipelineGroupIPNode::setPipeline2(const string& a, const string& b)
    {
        StringVector newPipeline(2);
        newPipeline[0] = a;
        newPipeline[1] = b;
        setPipeline(newPipeline);
    }

    void PipelineGroupIPNode::setPipeline3(const string& a, const string& b,
                                           const string& c)
    {
        StringVector newPipeline(3);
        newPipeline[0] = a;
        newPipeline[1] = b;
        newPipeline[2] = c;
        setPipeline(newPipeline);
    }

    void PipelineGroupIPNode::setPipeline4(const string& a, const string& b,
                                           const string& c, const string& d)
    {
        StringVector newPipeline(4);
        newPipeline[0] = a;
        newPipeline[1] = b;
        newPipeline[2] = c;
        newPipeline[3] = d;
        setPipeline(newPipeline);
    }

    IPNode* PipelineGroupIPNode::findNodeInPipelineByTypeName(
        const std::string& name) const
    {
        for (size_t i = 1; i < m_nodes.size(); i++)
        {
            if (m_nodes[i]->protocol() == name)
                return m_nodes[i];
        }

        return 0;
    }

    string PipelineGroupIPNode::profile() const
    {
        return propertyValue<StringProperty>("profile.name", "");
    }

    void PipelineGroupIPNode::copyNode(const IPNode* node)
    {
        if (const PipelineGroupIPNode* group =
                dynamic_cast<const PipelineGroupIPNode*>(node))
        {
            if (group->definition() == definition())
            {
                //
                //  Group props can determine their contents so we have to
                //  make sure to copy the props first then the underlying
                //  member structure.
                //

                copy(node);
                buildPipeline();
                if (group->rootNode())
                {
                    rootNode()->copyNode(group->rootNode());
                    copyInputs(rootNode(), group->rootNode());
                }
            }
        }
    }

} // namespace IPCore
