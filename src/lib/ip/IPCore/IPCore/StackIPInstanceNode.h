//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__StackIPInstanceNode__h__
#define __IPCore__StackIPInstanceNode__h__
#include <iostream>
#include <IPCore/IPInstanceNode.h>

namespace IPCore
{
    class GroupNode;

    /// StackIPInstanceNode is a base class for IPInstanceNode types which need
    /// management of multiple overlapping inputs

    ///
    /// StackIPInstanceNode: all image inputs are output together in a
    /// "stack". All or one of the audio inputs can be selected.
    ///

    class StackIPInstanceNode : public IPInstanceNode
    {
    public:
        typedef std::pair<int, int> FrameRange;
        typedef std::vector<FrameRange> FrameRangeVector;
        typedef std::vector<ImageRangeInfo> ImageRangeInfoVector;
        typedef std::vector<ImageStructureInfo> ImageStructureInfoVector;

        enum OutOfRangePolicy
        {
            NoImageOutOfRange,
            BlackOutOfRange,
            HoldOutOfRange
        };

        StackIPInstanceNode(const std::string& name, const NodeDefinition* def,
                            IPGraph* graph, GroupIPNode* group = 0);

        virtual ~StackIPInstanceNode();

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
        virtual void readCompleted(const std::string&, unsigned int);

        void setFitInputsToOutputAspect(bool b)
        {
            m_fit = b;
            setHasLinearTransform(b);
        }

        FloatProperty* outputFPSProperty() const { return m_outputFPS; }

        IntProperty* outputSizeProperty() const { return m_outputSize; }

        IntProperty* autoSizeProperty() const { return m_autoSize; }

        virtual void propagateFlushToInputs(const FlushContext&);

    protected:
        void computeRanges();
        void updateOutOfRangePolicy();
        int inputFrame(size_t, int, bool unconstrained = false);
        void mapInputToEvalFramesInternal(size_t inputIndex, const FrameVector&,
                                          FrameVector&) const;

        void lock() const { pthread_mutex_lock(&m_lock); }

        void unlock() const { pthread_mutex_unlock(&m_lock); }

        void lazyUpdateRanges() const;
        void updateActiveAudioInput();

    protected:
        mutable bool m_rangeInfoDirty;
        mutable bool m_structureInfoDirty;
        bool m_fit;
        OutOfRangePolicy m_outOfRangePolicy;
        ImageRangeInfoVector m_rangeInfos;
        ImageStructureInfoVector m_structInfos;
        FrameRangeVector m_globalRanges;
        ImageRangeInfo m_info;
        ImageStructureInfo m_structureInfo;
        mutable pthread_mutex_t m_lock;
        IntProperty* m_alignStartFrames;
        StringProperty* m_outOfRangePolicyProp;
        IntProperty* m_useCutInfo;
        StringProperty* m_activeAudioInput;
        int m_activeAudioInputIndex;
        int m_offset;
        FloatProperty* m_outputFPS;
        IntProperty* m_outputSize;
        IntProperty* m_autoSize;
    };

} // namespace IPCore

#endif // __IPCore__StackIPInstanceNode__h__
