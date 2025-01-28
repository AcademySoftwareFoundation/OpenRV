//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__SoundTrackIPNode__h__
#define __IPCore__SoundTrackIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <TwkAudio/Audio.h>
#include <algorithm>
#include <limits>

namespace IPCore
{

    //
    //  class SoundTrackIPNode
    //
    //  Adds an audio stream to existing video and controls volume and
    //  other audio attributes. Similar in some respects to the Sequence
    //  since the Soundtrack node can use an edl.
    //

    class SoundTrackIPNode : public IPNode
    {
    public:
        typedef TwkMovie::Movie Movie;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::StringProperty StringProperty;
        typedef TwkContainer::Component Component;

        struct RangeStats
        {
            RangeStats()
                : min(std::numeric_limits<float>::max())
                , max(-std::numeric_limits<float>::max())
                , totalPos(0.f)
                , totalNeg(0.f)
                , nPos(0)
                , nNeg(0)
            {
            }

            float min;
            float max;
            float totalPos;
            float totalNeg;
            unsigned int nPos;
            unsigned int nNeg;
        };

        typedef std::vector<RangeStats> RangeStatsVector;

        SoundTrackIPNode(const std::string& name, const NodeDefinition* def,
                         IPGraph* graph, GroupIPNode* group = 0);

        virtual ~SoundTrackIPNode();
        virtual IPImage* evaluate(const Context&);
        IPImage* evaluateAudioTexture(const Context&);

        virtual void setInputs(const IPNodes&);
        virtual ImageRangeInfo imageRangeInfo() const;

        virtual size_t audioFillBuffer(const AudioContext&);
        virtual void propertyChanged(const Property*);

        //
        //  Return true only if the Offset actually changed.
        //
        bool setInternalOffset(float);

        float audioTextureComplete() const;

        static float defaultVolume;

        //
        //  Audio props
        //

    protected:
        virtual void audioConfigure(const AudioConfiguration&);
        virtual void graphConfigure(const GraphConfiguration&);

        void renderAudio(const AudioContext&);
        void renderAudioAC(const AudioContext&);
        void renderAudioWaveform(const AudioContext&);
        void updateRanges();
        void clearFB();

        void lockFB() { pthread_mutex_lock(&m_fblock); }

        void unlockFB() { pthread_mutex_unlock(&m_fblock); }

    private:
        FloatProperty* m_volume;
        FloatProperty* m_balance;
        FloatProperty* m_offset;
        FloatProperty* m_internalOffset;
        IntProperty* m_mute;
        IntProperty* m_softClamp;
        IntProperty* m_visualWidth;
        IntProperty* m_visualHeight;
        IntProperty* m_visualStartFrame;
        IntProperty* m_visualEndFrame;
        pthread_mutex_t m_fblock;

        AudioConfiguration m_audioConfig;
        GraphConfiguration m_graphConfig;
        TwkAudio::SampleTime m_sampleStart;
        TwkAudio::SampleTime m_sampleEnd;
        TwkAudio::SampleTime m_sampleCurrent;
        FrameBuffer* m_fb;
        RangeStatsVector m_stats;
        size_t m_serialNumber;
    };

} // namespace IPCore

#endif // __IPCore__SoundTrackIPNode__h__
