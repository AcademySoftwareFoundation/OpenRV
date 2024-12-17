//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__SequenceIPNode__h__
#define __IPGraph__SequenceIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <algorithm>
#include <mutex>

namespace IPCore
{

    //
    //  class SequenceIPNode
    //
    //  Converts inputs into formats that can be used by the ImageRenderer
    //

    class SequenceIPNode : public IPNode
    {
    public:
        struct EvalPoint
        {
            EvalPoint(int s, int f, int i)
                : sourceIndex(s)
                , sourceFrame(f)
                , inputIndex(i)
            {
            }

            int sourceIndex;
            int sourceFrame;
            int inputIndex;
        };

        typedef TwkMovie::Movie Movie;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::StringProperty StringProperty;
        typedef TwkContainer::Component Component;
        typedef std::vector<ImageRangeInfo> RangeInfos;
        typedef std::vector<ImageStructureInfo> StructureInfos;
        typedef std::recursive_mutex Mutex;
        typedef std::lock_guard<Mutex> LockGuard;

        SequenceIPNode(const std::string& name, const NodeDefinition* def,
                       IPGraph* graph, GroupIPNode* group = 0);

        virtual ~SequenceIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void testEvaluate(const Context&, TestEvaluationResult&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);

        virtual void setInputs(const IPNodes&);
        virtual ImageRangeInfo imageRangeInfo() const;
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void mediaInfo(const Context&, MediaInfoVector&) const;
        virtual void mapInputToEvalFrames(size_t inputIndex,
                                          const FrameVector& in,
                                          FrameVector& out) const;

        void init();
        void createDefaultEDL(int append = 0) const;
        virtual void propertyChanged(const Property*);

        int indexAtFrame(int) const;
        int indexAtSample(TwkAudio::SampleTime seekSample, double sampleRate,
                          double fps);
        EvalPoint evaluationPoint(int frame, int forceIndex = -1) const;

        virtual size_t audioFillBuffer(const AudioContext&);

        void updateInputData() const;

        FloatProperty* outputFPSProperty() const { return m_outputFPS; }

        IntProperty* outputSizeProperty() const { return m_outputSize; }

        IntProperty* autoSizeProperty() const { return m_autoSize; }

        virtual void propagateFlushToInputs(const FlushContext&);

        bool clipCaching() { return m_clipCaching; }

        void setClipCaching(bool b) { m_clipCaching = b; }

        void setVolatileInputs(bool b) { m_volatileInputs = b; }

        /// this signal is called before all state change
        VoidSignal& changingSignal() { return m_changingSignal; }

        /// this signal is called after all state change
        VoidSignal& changedSignal() { return m_changedSignal; }

        /// return the source offset and the source range info of a specific
        /// source
        bool getSourceRange(int sourceIndex, ImageRangeInfo& rangeInfo,
                            int& sourceOffset);

        void invalidate();

    protected:
        virtual void inputChanged(int inputIndex);
        virtual void
        inputRangeChanged(int inputIndex,
                          PropagateTarget target = LegacyPropagateTarget);
        virtual void inputImageStructureChanged(int inputIndex,
                                                PropagateTarget target);
        void lazyBuildState() const;
        bool interactiveSize(const Context&) const;
        void createDefaultEDLInternal(int append, const IPNodes& inputs) const;
        void updateInputDataInternal(const IPNodes& inputs) const;

    private:
        // minimum discovered source before to distribute averange range to
        // other undiscovered sources.
        static const int kMinSourceDiscovered = 1;

        mutable bool m_updateEDL;
        mutable bool m_updateHiddenData;
        mutable RangeInfos m_rangeInfos;
        mutable ImageStructureInfo m_structInfo;
        mutable Mutex m_mutex;

        bool m_clipCaching;
        bool m_volatileInputs;

        Component* m_edl;
        IntProperty* m_edlSource;
        IntProperty* m_edlGlobalIn;
        IntProperty* m_edlSourceIn;
        IntProperty* m_edlSourceOut;
        IntProperty* m_autoEDL;
        IntProperty* m_useCutInfo;
        IntProperty* m_autoSize;
        FloatProperty* m_outputFPS;
        IntProperty* m_outputSize;
        IntProperty* m_interactiveSize;
        StringProperty* m_inputsBlendingModes;
        IntProperty* m_supportReversedOrderBlending;

        /// this signal is called before all state change
        VoidSignal m_changingSignal;

        /// this signal is called after all state change
        VoidSignal m_changedSignal;

        // m_changing is true when the state is changing.
        mutable bool m_changing{false};
    };

} // namespace IPCore

#endif // __IPGraph__SequenceIPNode__h__
