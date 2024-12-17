//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__StackIPNode__h__
#define __IPGraph__StackIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <algorithm>

namespace IPCore
{

    //
    //  class StackIPNode
    //
    //  All image inputs are output together in a "stack". All or one of
    //  the audio inputs can be selected. The stack node includes a
    //  compositing operation applied to the image stack.
    //

    class StackIPNode : public IPNode
    {
    public:
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkContainer::StringProperty StringProperty;
        typedef std::pair<int, int> FrameRange;
        typedef std::vector<FrameRange> FrameRangeVector;

        StackIPNode(const std::string& name, const NodeDefinition* def,
                    IPGraph* graph, GroupIPNode* group = 0);
        virtual ~StackIPNode();
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

        static void setDefaultCompType(const std::string& type)
        {
            m_defaultCompType = type;
        }

        FloatProperty* outputFPSProperty() const { return m_outputFPS; }

        IntProperty* outputSizeProperty() const { return m_outputSize; }

        IntProperty* autoSizeProperty() const { return m_autoSize; }

        virtual void propagateFlushToInputs(const FlushContext&);

        void invalidate();

    protected:
        void computeRanges();
        int inputFrame(size_t, int, bool unconstrained = false);
        void mapInputToEvalFramesInternal(size_t inputIndex, const FrameVector&,
                                          FrameVector&) const;

        void lock() const { pthread_mutex_lock(&m_lock); }

        void unlock() const { pthread_mutex_unlock(&m_lock); }

        void lazyUpdateRanges() const;
        void updateChosenAudioInput();

        IPImage* collapseInputs(const Context&, IPImage::BlendMode,
                                IPImageVector&, IPImageSet&);
        bool interactiveSize(const Context&) const;

    protected:
        mutable bool m_rangeInfoDirty;
        mutable bool m_structureInfoDirty;
        bool m_fit;
        std::vector<ImageRangeInfo> m_rangeInfos;
        FrameRangeVector m_globalRanges;
        ImageRangeInfo m_info;
        ImageStructureInfo m_structureInfo;
        mutable pthread_mutex_t m_lock;
        IntProperty* m_alignStartFrames;
        IntProperty* m_strictFrameRanges;
        IntProperty* m_useCutInfo;
        StringProperty* m_activeAudioInput;
        int m_activeAudioInputIndex;
        StringProperty* m_compMode;
        int m_offset;
        FloatProperty* m_outputFPS;
        IntProperty* m_outputSize;
        IntProperty* m_autoSize;
        IntProperty* m_interactiveSize;
        IntProperty* m_supportReversedOrderBlending;

    private:
        static std::string m_defaultCompType;
    };

} // namespace IPCore

#endif // __IPGraph__StackIPNode__h__
