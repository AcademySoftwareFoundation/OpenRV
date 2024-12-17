//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__AdaptorIPNode__h__
#define __IPCore__AdaptorIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{

    /// AdaptorIPNode is used inside a GroupIPNode as the last output or first
    /// input

    ///
    /// This is a flow control node. It will either propagate to
    /// evaluation-like information which normally flows up the graph to
    /// its enclosing group or notification-like information (state
    /// change) which normally flows down the graph to its enclosing
    /// group.
    //
    /// If AdaptorIPNode has no inputs and does not have an inputNode
    /// passed into its constructor, it will evaluate the inputs of the
    /// group node its connected to (passed into the constructor). If the
    /// AdaptorIPNode has no output node it will call the GroupIPNode's
    /// outputs when propagating from leaves to root.
    ///

    class AdaptorIPNode : public IPNode
    {
    public:
        AdaptorIPNode(const std::string& name, const NodeDefinition* definition,
                      IPGraph* graph, GroupIPNode* group);

        AdaptorIPNode(const std::string& name, const NodeDefinition* definition,
                      IPNode* groupInputNode, GroupIPNode* group,
                      IPGraph* graph);

        virtual ~AdaptorIPNode();

        //
        //  AdaptorIPNode API
        //

        void setGroupInputNode(IPNode* node) { m_groupInputNode = node; }

        IPNode* groupInputNode() const { return m_groupInputNode; }

        //
        //  IPNode API
        //

        virtual ImageRangeInfo imageRangeInfo() const;
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void mediaInfo(const Context&, MediaInfoVector&) const;
        virtual bool isMediaActive() const;
        virtual void setMediaActive(bool state);
        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual void visitRecursive(NodeVisitor&);
        virtual void testEvaluate(const Context&, TestEvaluationResult&);
        virtual size_t audioFillBuffer(const AudioContext&);
        virtual void propagateFlushToOutputs(const FlushContext&);
        virtual void propagateFlushToInputs(const FlushContext&);
        virtual void prepareForWrite();
        virtual void writeCompleted();

    protected:
        IPNode* m_groupInputNode;
    };

} // namespace IPCore

#endif // __IPCore__AdaptorIPNode__h__
