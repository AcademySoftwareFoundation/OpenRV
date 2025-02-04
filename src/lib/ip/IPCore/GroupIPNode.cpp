//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/GroupIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <TwkUtil/sgcHop.h>

namespace IPCore
{
    using namespace std;

    GroupIPNode::GroupIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
        , m_root(0)
    {
    }

    GroupIPNode::~GroupIPNode()
    {
        deleteInputs();

        //
        //  Delete by disconnecting all inputs first and then
        //  deleting. This prevents the need for recursive delete (of
        //  course that should *still* work, but its more dangerous and
        //  less efficient).
        //

        for (IPNodeSet::iterator i = m_members.begin(); i != m_members.end();
             ++i)
        {
            IPNode* n = *i;
            n->willDelete();
            n->disconnectInputs();
        }

        while (!m_members.empty())
        {
            IPNode* n = *m_members.begin();
            delete n;
        }

        m_root = 0;
    }

    void GroupIPNode::add(IPNode* n) { m_members.insert(n); }

    void GroupIPNode::remove(IPNode* n) { m_members.erase(n); }

    void GroupIPNode::setInputs(const IPNodes& newInputs)
    {
        IPNode::setInputs(newInputs);
    }

    string GroupIPNode::internalNodeNameForInput(IPNode* input,
                                                 const string& smallIdString)
    {
        ostringstream nout;
        nout << name() << "_" << smallIdString << "_" << input->name();
        return nout.str();
    }

    string GroupIPNode::internalNodeName(const string& smallIdString)
    {
        ostringstream nout;
        nout << name() << "_" << smallIdString;
        return nout.str();
    }

    void GroupIPNode::internalOutputNodesFor(IPNode* node, IPNodes& nodes) const
    {
        nodes.clear();
        GroupIPNode* nodeGroup = node->group();

        for (IPNodeSet::const_iterator i = m_members.begin();
             i != m_members.end(); ++i)
        {
            if (AdaptorIPNode* anode = dynamic_cast<AdaptorIPNode*>(*i))
            {
                if (anode->groupInputNode() == node
                    || (nodeGroup && nodeGroup == anode->groupInputNode()))
                {
                    // cout << "found anode " << anode->name() << " in " <<
                    // name() << endl;
                    const IPNodes& outputs = anode->outputs();
                    for (size_t q = 0; q < outputs.size(); q++)
                        nodes.push_back(outputs[q]);
                }
            }
        }
    }

    IPImage* GroupIPNode::evaluate(const Context& context)
    {
        //
        //  By doing this here, the NoImage will have the most useful UI
        //  name. Otherwise it may get one of the member node names
        //

        if (minimumNumberOfInputs() > 0 && inputs().size() == 0 || !m_root)
        {
            return IPImage::newNoImage(this, "Empty");
        }
        else
        {
            return m_root->evaluate(context);
        }
    }

    void GroupIPNode::metaEvaluate(const Context& c, MetaEvalVisitor& visitor)
    {
        if (m_root)
        {
            visitor.enter(c, this);
            if (visitor.traverseChild(c, 0, this, m_root))
                m_root->metaEvaluate(c, visitor);
            visitor.leave(c, this);
        }
    }

    void GroupIPNode::visitRecursive(NodeVisitor& visitor)
    {
        if (m_root)
        {
            visitor.enter(this);
            if (visitor.traverseChild(0, this, m_root))
                m_root->visitRecursive(visitor);
            visitor.leave(this);
        }
    }

    void GroupIPNode::testEvaluate(const Context& context,
                                   TestEvaluationResult& results)
    {
        if (m_root)
            m_root->testEvaluate(context, results);
    }

    IPImageID* GroupIPNode::evaluateIdentifier(const Context& context)
    {
        return m_root ? m_root->evaluateIdentifier(context) : 0;
    }

    IPNode::ImageRangeInfo GroupIPNode::imageRangeInfo() const
    {
        return m_root ? m_root->imageRangeInfo() : ImageRangeInfo();
    }

    IPNode::ImageStructureInfo
    GroupIPNode::imageStructureInfo(const Context& context) const
    {
        return m_root ? m_root->imageStructureInfo(context)
                      : ImageStructureInfo();
    }

    void GroupIPNode::mediaInfo(const Context& context,
                                MediaInfoVector& infos) const
    {
        if (m_root)
            m_root->mediaInfo(context, infos);
    }

    bool GroupIPNode::isMediaActive() const
    {
        if (m_root)
        {
            return m_root->isMediaActive();
        }

        return IPNode::isMediaActive();
    }

    void GroupIPNode::setMediaActive(bool state)
    {
        if (m_root)
        {
            m_root->setMediaActive(state);
        }
    }

    void GroupIPNode::inputChanged(int index)
    {
        for (IPNodeSet::iterator i = m_members.begin(); i != m_members.end();
             ++i)
        {
            if (AdaptorIPNode* anode = dynamic_cast<AdaptorIPNode*>(*i))
            {
                anode->propagateInputChangeInternal();
            }
        }
    }

    void GroupIPNode::inputRangeChanged(int index, PropagateTarget target)
    {
        for (IPNodeSet::iterator i = m_members.begin(); i != m_members.end();
             ++i)
        {
            if (AdaptorIPNode* anode = dynamic_cast<AdaptorIPNode*>(*i))
            {
                anode->propagateRangeChangeInternal(target);
            }
        }
    }

    void GroupIPNode::inputImageStructureChanged(int index,
                                                 PropagateTarget target)
    {
        if (target & MemberPropagateTarget)
        {
            for (IPNodeSet::iterator i = m_members.begin();
                 i != m_members.end(); ++i)
            {
                if (AdaptorIPNode* anode = dynamic_cast<AdaptorIPNode*>(*i))
                {
                    anode->propagateImageStructureChangeInternal(target);
                }
            }
        }
    }

    void GroupIPNode::propagateFlushToInputs(const FlushContext& c)
    {
        if (m_root)
            m_root->propagateFlushToInputs(c);

        //
        //  The below would propagate through the inputs, but some of the
        //  nodes in the group may decide to not propagate to all inputs,
        //  so let the propagation get to inputs through the adaptor nodes
        //  via the call to root above.
        //
        // IPNode::propagateFlushToInputs(c);
    }

    size_t GroupIPNode::audioFillBuffer(const AudioContext& c)
    {
        return m_root ? m_root->audioFillBuffer(c) : 0;
    }

    void GroupIPNode::propagateAudioConfigToInputs(const AudioConfiguration& c)
    {
        if (m_root)
            m_root->propagateAudioConfigToInputs(c);
        IPNode::propagateAudioConfigToInputs(c);
    }

    void
    GroupIPNode::propagateGraphConfigToInputs(const GraphConfiguration& config)
    {
        if (m_root)
            m_root->propagateGraphConfigToInputs(config);
        IPNode::propagateGraphConfigToInputs(config);
    }

    IPNode* GroupIPNode::newSubGraphForInput(size_t index, const IPNodes& nodes)
    {
        return nodes[index];
    }

    IPNode* GroupIPNode::modifySubGraphForInput(size_t index,
                                                const IPNodes& newInputs,
                                                IPNode* subgraph)
    {
        return subgraph;
    }

    void GroupIPNode::setInputsWithReordering(const IPNodes& newInputs,
                                              IPNode* fanInNode,
                                              int inputIndexOnly)
    {
        HOP_PROF_FUNC();
        if (isDeleting())
        {
            IPNode::setInputs(newInputs);
            return;
        }

        IPNodes inputs(newInputs.size());
        IndexArray indices;
        IndexArray removeIndices;
        IPNodes removeNodes;

        {
            HOP_PROF("inputPartialReordering");
            inputPartialReordering(newInputs, removeIndices, indices);
        }

        {
            HOP_PROF("fanInNode->findInputs");
            fanInNode->findInputs(removeIndices, removeNodes);
        }

        {
            HOP_PROF("*SubGraphForInput loop");
            for (size_t q = 0; q < /*indices.size()*/ newInputs.size(); q++)
            {
                int i = indices[q];

                if ((inputIndexOnly >= 0))
                {
                    if ((i >= 0) && (inputIndexOnly == i))
                        inputs[q] = modifySubGraphForInput(
                            q, newInputs, fanInNode->inputs()[i]);
                    else
                        inputs[q] = fanInNode->inputs()[i];
                }
                else
                {
                    if (i >= 0)
                    {
                        HOP_PROF("modifySubGraphForInput");
                        inputs[q] = modifySubGraphForInput(
                            q, newInputs, fanInNode->inputs()[i]);
                    }
                    else
                    {
                        HOP_PROF("newSubGraphForInput");
                        inputs[q] = newSubGraphForInput(q, newInputs);
                    }
                }
            }
        }

        {
            HOP_PROF("fanInNode->setInputs");
            fanInNode->setInputs(inputs);
        }

        //
        //  This will cause notification and we need that to happen
        //  *after* we've built the internal graph for everything to
        //  update properly
        //

        {
            HOP_PROF("IPNode::setInputs");
            IPNode::setInputs(newInputs);
        }

        //
        //  Delete the drit
        //

        {
            HOP_PROF("Delete removed nodes");
            for (size_t q = 0; q < removeNodes.size(); q++)
                delete removeNodes[q];
        }
    }

    void GroupIPNode::flushIDsOfGroup()
    {
        FBCache::IDSet substr;
        substr.insert(name());

        FBCache& cache = graph()->cache();

        cache.lock();
        cache.flushIDSetSubstr(substr);
        cache.freeAllTrash();
        cache.unlock();
    }

    void GroupIPNode::readCompleted(const string& t, unsigned int v)
    {
        setInputs(inputs());

        IPNode::readCompleted(t, v);
    }

    void GroupIPNode::swapNodes(IPNode* A, IPNode* B)
    {
        IPNodes outputs = A->outputs();

        B->setInputs(A->inputs());

        for (size_t i = 0; i < outputs.size(); i++)
        {
            IPNode* node = outputs[i];
            IPNodes inputs = node->inputs();
            replace(inputs.begin(), inputs.end(), A, B);
            node->setInputs(inputs);
        }
    }

    void GroupIPNode::addAuxillaryOutput(IPNode* node)
    {
        m_auxOutputs.push_back(node);
    }

    IPNode* GroupIPNode::newMemberNode(const std::string& typeName,
                                       const std::string& nameStem)
    {
        return graph()->newNode(typeName, internalNodeName(nameStem), this);
    }

    IPNode* GroupIPNode::newMemberNodeForInput(const std::string& typeName,
                                               IPNode* input,
                                               const std::string& nameStem)
    {
        return graph()->newNode(
            typeName, internalNodeNameForInput(input, nameStem), this);
    }

    AdaptorIPNode* GroupIPNode::newAdaptorForInput(IPNode* input)
    {
        AdaptorIPNode* n = dynamic_cast<AdaptorIPNode*>(graph()->newNode(
            "Adaptor", internalNodeNameForInput(input, "a"), this));

        n->setGroupInputNode(input);
        return n;
    }

    IPNode* GroupIPNode::memberByType(const std::string& typeName)
    {
        for (IPNodeSet::iterator i = m_members.begin(); i != m_members.end();
             ++i)
        {
            IPNode* n = *i;
            if (n->protocol() == typeName)
                return n;
        }
        return 0;
    }

    void GroupIPNode::collectMemberNodes(IPNodeSet& nodeSet, size_t depth)
    {
        for (IPNodeSet::iterator i = m_members.begin(); i != m_members.end();
             ++i)
        {
            IPNode* n = *i;
            n->collectMemberNodes(nodeSet, depth + 1);
        }

        if (depth != 0)
            nodeSet.insert(const_cast<GroupIPNode*>(this));
    }

    namespace
    {
        struct TypeFilter
        {
            TypeFilter(const string& n)
                : name(n)
            {
            }

            string name;

            bool operator()(const IPNode* node)
            {
                return node->protocol() != name;
            }
        };
    } // namespace

    void GroupIPNode::collectMemberNodesByTypeName(const string& typeName,
                                                   IPNodeSet& nodeSet,
                                                   size_t depth)
    {
        IPNodeSet nodes;
        collectMemberNodes(nodes, depth);
        remove_copy_if(nodes.begin(), nodes.end(),
                       inserter(nodeSet, nodeSet.end()), TypeFilter(typeName));
    }

    void GroupIPNode::isolate()
    {
        //
        //  To isolate a group we remove all of the members (and their
        //  members) from the graph and only isolate the top level group.
        //  This is OK because there shouldn't be any external connections
        //  to group members that can't be recreated.
        //

        //
        //  NOTE: until we have proper connections working this is a work
        //  around to prevent accidently reestablishing inputs/outputs which
        //  were removed on purpose when originally isolating. In other words:
        //  if this node was already isolated (deleted) and then the user
        //  undoes the deletion (restored) and then deletes again via redo the
        //  original code which initiated the DeleteNode command doesn't get a
        //  chance to remove any inputs/outputs which were purely temporary
        //  (like UI preview nodes). Because of that we can accidently store
        //  one of the temporary inputs/ouputs and then try and restore it
        //  again resulting in a possibly hosed graph.
        //
        //  The workaround here is to not remove the __graph component which
        //  originally connected the inputs and outputs for this node so that
        //  doit + undo + redo + undo + redo won't add back in a record of a
        //  temporary connection. In the future with connection objects, we'll
        //  know that the connection is temporary and just ignore it
        //  automatically instead of the UI mucking with it.
        //

        bool hasGraphComp = component("__graph") != 0;

        IPNodeSet members;
        collectMemberNodes(members);

        StringPairProperty::container_type externalOutputs;
        IntProperty::container_type externalIndex;

        for (IPNodeSet::iterator i = members.begin(); i != members.end(); ++i)
        {
            IPNode* mnode = *i;
            if (mnode == this)
                continue;
            graph()->removeNode(mnode);

            const IPNodes& memberOutputs = mnode->outputs();
            IPNodes outputNodesToRemove;

            for (size_t i = 0; i < memberOutputs.size(); i++)
            {
                if (members.count(memberOutputs[i]) == 0)
                {
                    //
                    //  This is an external output. Save the connection
                    //  for restoration.
                    //

                    IPNode* externalNode = memberOutputs[i];
                    size_t index = externalNode->indexOfChild(mnode);

                    externalOutputs.push_back(
                        StringPair(mnode->name(), externalNode->name()));
                    externalIndex.push_back(int(index));
                    outputNodesToRemove.push_back(externalNode);
                }
            }

            for (size_t i = 0; i < outputNodesToRemove.size(); i++)
            {
                outputNodesToRemove[i]->removeInput(mnode);
            }
        }

        IPNode::isolate();

        if (!externalOutputs.empty() && !hasGraphComp)
        {
            StringPairProperty* sp =
                createProperty<StringPairProperty>("__graph.externalOutputs");
            IntProperty* ip =
                createProperty<IntProperty>("__graph.externalIndex");

            sp->valueContainer() = externalOutputs;
            ip->valueContainer() = externalIndex;
        }
    }

    void GroupIPNode::restore()
    {
        IPNodeSet members;
        collectMemberNodes(members);

        for (IPNodeSet::iterator i = members.begin(); i != members.end(); ++i)
        {
            IPNode* mnode = *i;
            graph()->addNode(mnode);
        }

        StringPairProperty* sp =
            property<StringPairProperty>("__graph.externalOutputs");
        IntProperty* ip = property<IntProperty>("__graph.externalIndex");

        if (sp && ip)
        {
            //
            //  Prevent restore from deleting these props.
            //

            ip->ref();
            sp->ref();
        }

        IPNode::restore();

        if (sp && ip)
        {
            for (size_t i = 0; i < sp->size(); i++)
            {
                const StringPair& con = (*sp)[i];
                size_t index = (*ip)[i];

                IPNode* mnode = graph()->findNodePossiblyIsolated(con.first);
                IPNode* externalNode =
                    graph()->findNodePossiblyIsolated(con.second);

                if (mnode && externalNode)
                    externalNode->insertInput(mnode, index);
            }

            //
            //  This will cause the props to be deleted
            //

            sp->unref();
            ip->unref();
        }
    }

    void GroupIPNode::copyInputs(IPNode* A, const IPNode* B)
    {
        if (A->definition() == B->definition())
        {
            const IPNode::IPNodes& aInputs = A->inputs();
            const IPNode::IPNodes& bInputs = B->inputs();

            for (size_t i = 0; i < aInputs.size() && i < bInputs.size(); i++)
            {
                IPNode* aNode = aInputs[i];
                const IPNode* bNode = bInputs[i];

                if (aNode->definition() == bNode->definition())
                {
                    //
                    //  Only follow the graph if the nodes match
                    //  definitions
                    //
                    //  Group props can determine their contents so we have to
                    //  make sure to copy the props first then the underlying
                    //  member structure.
                    //

                    aNode->copyNode(bNode);
                    copyInputs(aNode, bNode);
                }
            }
        }
    }

    void GroupIPNode::copyNode(const IPNode* node)
    {
        if (const GroupIPNode* group = dynamic_cast<const GroupIPNode*>(node))
        {
            if (group->definition() == definition())
            {
                //
                //  Group props can determine their contents so we have to
                //  make sure to copy the props first then the underlying
                //  member structure.
                //

                copy(node);
                rootNode()->copyNode(group->rootNode());
                copyInputs(rootNode(), group->rootNode());
            }
        }
    }

} // namespace IPCore
