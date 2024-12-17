//******************************************************************************
// Copyright (c) 2010 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__RetimeIPNode__h__
#define __IPGraph__RetimeIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <boost/thread.hpp>
#include <vector>
#include <algorithm>

namespace IPCore
{

    //
    //  class RetimeIPNode
    //
    //  Retime input -- nothing fancy, just do a pull-down/up
    //

    class RetimeIPNode : public IPNode
    {
    public:
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkAudio::Time Time;
        typedef boost::mutex Mutex;
        typedef boost::mutex::scoped_lock ScopedLock;

        RetimeIPNode(const std::string& name, const NodeDefinition* def,
                     IPGraph* graph, GroupIPNode* group = 0);

        virtual ~RetimeIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;

        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual void testEvaluate(const Context&, TestEvaluationResult&);
        virtual void flushAllCaches(const FlushContext&);
        virtual void propagateFlushToInputs(const FlushContext&);
        virtual void propagateFlushToOutputs(const FlushContext&);
        virtual ImageRangeInfo imageRangeInfo() const;
        virtual size_t audioFillBuffer(const AudioContext&);
        virtual void mapInputToEvalFrames(size_t inputIndex, const FrameVector&,
                                          FrameVector&) const;

        virtual void setInputs(const IPNodes&);
        virtual bool testInputs(const IPNodes&, std::ostringstream&) const;
        virtual void propertyChanged(const Property*);

        FloatProperty* outputFPSProperty() const { return m_fps; }

        void resetFPS();

    protected:
        virtual void inputChanged(int index);
        virtual void
        inputRangeChanged(int index,
                          PropagateTarget target = LegacyPropagateTarget);

    private:
        int retimedFrame(int frame) const;
        int invRetimedFrame0(int frame) const;
        int invRetimedFrame(int frame) const;

        void updateWarpData() const;
        void updateExplicitData() const;

        bool explicitPropertiesOK() const;

    private:
        FloatProperty* m_vscale;
        FloatProperty* m_voffset;
        FloatProperty* m_ascale;
        FloatProperty* m_aoffset;
        FloatProperty* m_fps;
        FloatProperty* m_warpKeyRates;
        IntProperty* m_outputFrame;
        IntProperty* m_inputFrame;
        IntProperty* m_warpKeyFrames;
        IntProperty* m_warpActive;
        IntProperty* m_warpStyle;
        IntProperty* m_explicitActive;
        IntProperty* m_explicitFirstOutputFrame;
        IntProperty* m_explicitInputFrames;
        Time m_grainDuration;
        Time m_grainEnvelope;
        mutable ImageRangeInfo m_inputInfo;
        mutable bool m_warpDataValid;
        mutable bool m_explicitDataValid;
        mutable Mutex m_nodeMutex;
        mutable FrameVector m_warpedInToOut;
        mutable FrameVector m_warpedOutToIn;
        mutable FrameVector m_explicitInToOut;
        mutable int m_explicitFirstInputFrame;
        mutable bool m_fpsDetected{false};
    };

} // namespace IPCore

#endif // __IPGraph__RetimeIPNode__h__
