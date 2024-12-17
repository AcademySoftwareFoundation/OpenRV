//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__TransitionIPInstanceNode__h__
#define __IPCore__TransitionIPInstanceNode__h__
#include <iostream>
#include <IPCore/IPInstanceNode.h>

namespace IPCore
{
    class NodeDefinition;
    class IPGraph;
    class GroupIPNode;

    /// TransitionIPInstanceNode is an IPInstanceNode for transitions

    ///
    /// TransitionIPInstanceNode is an instance of a NodeDefinition. The
    /// NodeDefinition completely determines the behavior of the node
    ///

    class TransitionIPInstanceNode : public IPInstanceNode
    {
    public:
        typedef std::pair<int, int> FrameRange;
        typedef std::vector<FrameRange> FrameRangeVector;
        typedef std::vector<ImageRangeInfo> ImageRangeInfoVector;

        TransitionIPInstanceNode(const std::string& name,
                                 const NodeDefinition* definition,
                                 IPGraph* graph, GroupIPNode* group = 0);

        virtual ~TransitionIPInstanceNode();

        virtual IPImage* evaluate(const Context&);

        virtual void testEvaluate(const Context&, TestEvaluationResult&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;

        virtual size_t audioFillBuffer(const AudioContext&);

        virtual void setInputs(const IPNodes&);
        virtual ImageRangeInfo imageRangeInfo() const;
        virtual void mapInputToEvalFrames(size_t inputIndex, const FrameVector&,
                                          FrameVector&) const;

        virtual void propertyChanged(const Property*);
        virtual void inputRangeChanged(int inputIndex, PropagateTarget target);
        virtual void inputImageStructureChanged(int inputIndex,
                                                PropagateTarget target);

        void setFitInputsToOutputAspect(bool b)
        {
            m_fit = b;
            setHasLinearTransform(b);
        }

        FloatProperty* outputFPSProperty() const { return m_outputFPS; }

        IntProperty* outputSizeProperty() const { return m_outputSize; }

        IntProperty* autoSizeProperty() const { return m_autoSize; }

        virtual void propagateFlushToInputs(const FlushContext&);
        virtual void readCompleted(const std::string&, unsigned int);

    protected:
        void computeRanges();
        int inputFrame(size_t, int, bool unconstrained = false);
        void mapInputToEvalFramesInternal(size_t inputIndex, const FrameVector&,
                                          FrameVector&) const;

        void lazyUpdateRanges() const;
        void updateChosenAudioInput();

    private:
        mutable bool m_rangeInfoDirty;
        mutable bool m_structureInfoDirty;
        bool m_fit;
        ImageRangeInfoVector m_rangeInfos;
        FrameRangeVector m_globalRanges;
        ImageRangeInfo m_info;
        ImageStructureInfo m_structureInfo;
        IntProperty* m_useCutInfo;
        FloatProperty* m_outputFPS;
        IntProperty* m_outputSize;
        IntProperty* m_autoSize;
        FloatProperty* m_startFrame;
        FloatProperty* m_duration;
    };

} // namespace IPCore

#endif // __IPCore__TransitionIPInstanceNode__h__
