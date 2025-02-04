//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    using namespace std;

    AdaptorIPNode::AdaptorIPNode(const string& name, const NodeDefinition* def,
                                 IPNode* groupInputNode, GroupIPNode* group,
                                 IPGraph* graph)
        : IPNode(name, def, graph)
        , m_groupInputNode(groupInputNode)
    {
        setWritable(false);
        if (group)
            setGroup(group);
        setMaxInputs(0);
    }

    AdaptorIPNode::AdaptorIPNode(const string& name, const NodeDefinition* def,
                                 IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph)
        , m_groupInputNode(0)
    {
        setWritable(false);
        if (group)
            setGroup(group);
        setMaxInputs(0);
    }

    AdaptorIPNode::~AdaptorIPNode() {}

    IPNode::ImageRangeInfo AdaptorIPNode::imageRangeInfo() const
    {
        if (m_groupInputNode)
            return m_groupInputNode->imageRangeInfo();
        else
            return IPNode::imageRangeInfo();
    }

    IPNode::ImageStructureInfo
    AdaptorIPNode::imageStructureInfo(const Context& context) const
    {
        if (m_groupInputNode)
            return m_groupInputNode->imageStructureInfo(context);
        return IPNode::imageStructureInfo(context);
    }

    void AdaptorIPNode::mediaInfo(const Context& context,
                                  MediaInfoVector& infos) const
    {
        if (m_groupInputNode)
            m_groupInputNode->mediaInfo(context, infos);
    }

    bool AdaptorIPNode::isMediaActive() const
    {
        if (m_groupInputNode)
        {
            return m_groupInputNode->isMediaActive();
        }

        return IPNode::isMediaActive();
    }

    void AdaptorIPNode::setMediaActive(bool state)
    {
        if (m_groupInputNode)
        {
            return m_groupInputNode->setMediaActive(state);
        }
    }

    IPImage* AdaptorIPNode::evaluate(const Context& context)
    {
        return m_groupInputNode ? m_groupInputNode->evaluate(context)
                                : IPNode::evaluate(context);
    }

    IPImageID* AdaptorIPNode::evaluateIdentifier(const Context& context)
    {
        if (m_groupInputNode)
            return m_groupInputNode->evaluateIdentifier(context);
        else
            return IPNode::evaluateIdentifier(context);
    }

    void AdaptorIPNode::metaEvaluate(const Context& c, MetaEvalVisitor& visitor)
    {
        if (m_groupInputNode)
            m_groupInputNode->metaEvaluate(c, visitor);
        else
            IPNode::metaEvaluate(c, visitor);
    }

    void AdaptorIPNode::visitRecursive(NodeVisitor& visitor)
    {
        visitor.enter(this);
        if (m_groupInputNode
            && visitor.traverseChild(0, this, m_groupInputNode))
        {
            m_groupInputNode->visitRecursive(visitor);
        }
        visitor.leave(this);
    }

    void AdaptorIPNode::testEvaluate(const Context& context,
                                     TestEvaluationResult& results)
    {
        if (m_groupInputNode)
            m_groupInputNode->testEvaluate(context, results);
        else
            IPNode::testEvaluate(context, results);
    }

    void AdaptorIPNode::propagateFlushToOutputs(const FlushContext& c)
    {
        if (!m_groupInputNode)
            IPNode::propagateFlushToOutputs(c);
        else
            group()->IPNode::propagateFlushToOutputs(c);
    }

    void AdaptorIPNode::propagateFlushToInputs(const FlushContext& c)
    {
        if (m_groupInputNode)
            m_groupInputNode->propagateFlushToInputs(c);
        else
            IPNode::propagateFlushToInputs(c);
    }

    size_t AdaptorIPNode::audioFillBuffer(const AudioContext& c)
    {
        if (m_groupInputNode)
            return m_groupInputNode->audioFillBuffer(c);
        else
            return IPNode::audioFillBuffer(c);
    }

    void AdaptorIPNode::prepareForWrite()
    {
        if (group())
        {
            //
            //  NOTE: the adaptor will report the first input index which
            //  matches its m_groupInputNode. If the group node has a node as
            //  its input more than once you'll get multiple occurances of the
            //  same input. This isn't actually a problem: by definition the
            //  first input is identical to the Nth input if its the same node
            //  at least outside of the group node.
            //
            //  Ideally if the user would want a node to have the same input
            //  occuring multiple times that node would implement some facility
            //  so that the input the user wants to duplicate could be used
            //  internally multiple times (e.g. the combine node does this)
            //

            const IPNodes& inputs = group()->inputs();

            IPNodes::const_iterator i =
                std::find(inputs.begin(), inputs.end(), m_groupInputNode);

            if (i != inputs.end())
            {
                declareProperty<IntProperty>("input.index",
                                             int(i - inputs.begin()));
            }
            else
            {
                // what do we do here?
            }
        }
    }

    void AdaptorIPNode::writeCompleted()
    {
        if (group())
        {
            try
            {
                removeProperty<IntProperty>("input.index");
            }
            catch (...)
            {
                // nothing
            }
        }
    }

} // namespace IPCore
