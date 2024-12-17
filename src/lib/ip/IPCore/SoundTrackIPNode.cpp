//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <TwkMath/Function.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <TwkUtil/Timer.h>
#include <IOtiff/IOtiff.h>
#include <TwkAudio/AudioCache.h>
#include <iostream>
#include <algorithm>

// Default based on 2 seconds of 48 kHz 2 channel audio
TwkAudio::SampleTime MAX_SAMPLES_PROCESSED_PER_EVAL = 192000;

namespace IPCore
{
    using namespace TwkAudio;
    using namespace TwkFB;
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkAudio;
    using namespace TwkMath;

    float SoundTrackIPNode::defaultVolume = 1.0;

    SoundTrackIPNode::SoundTrackIPNode(const std::string& name,
                                       const NodeDefinition* def, IPGraph* g,
                                       GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_sampleStart(0)
        , m_sampleEnd(0)
        , m_sampleCurrent(0)
        , m_fb(0)
        , m_serialNumber(0)
    {
        setMaxInputs(1);

        bool softClamp = def->intValue("defaults.softClamp", 0);

        m_volume =
            declareProperty<FloatProperty>("audio.volume", defaultVolume);
        m_balance = declareProperty<FloatProperty>("audio.balance", 0.0f);
        m_offset = declareProperty<FloatProperty>("audio.offset", 0.0f);
        m_internalOffset =
            declareProperty<FloatProperty>("audio.internalOffset", 0.0f);
        m_mute = declareProperty<IntProperty>("audio.mute", 0);
        m_softClamp =
            declareProperty<IntProperty>("audio.softClamp", softClamp ? 1 : 0);
        m_visualWidth = declareProperty<IntProperty>("visual.width", 0);
        m_visualHeight = declareProperty<IntProperty>("visual.height", 0);
        m_visualStartFrame =
            declareProperty<IntProperty>("visual.frameStart", 0);
        m_visualEndFrame = declareProperty<IntProperty>("visual.frameEnd", 0);
        pthread_mutex_init(&m_fblock, 0);
    }

    SoundTrackIPNode::~SoundTrackIPNode()
    {
        if (m_fb)
        {
            if (!m_fb->hasStaticRef() || m_fb->staticUnRef())
            {
                delete m_fb;
            }
        }
        pthread_mutex_destroy(&m_fblock);
    }

    bool SoundTrackIPNode::setInternalOffset(float t)
    {
        if (m_internalOffset->front() != t)
        {
            m_internalOffset->resize(1);
            m_internalOffset->front() = t;
            return true;
        }
        return false;
    }

    float SoundTrackIPNode::audioTextureComplete() const
    {
        return (graph()->audioCachingMode() == IPGraph::BufferCache)
                   ? 1.0
                   : double(m_sampleCurrent - m_sampleStart)
                         / double(m_sampleEnd - m_sampleStart);
    }

    IPImage* SoundTrackIPNode::evaluate(const Context& context)
    {
        if (inputs().empty())
            return IPImage::newNoImage(this, "No Input");
        IPImage* head = inputs().front()->evaluate(context);
        return head;
    }

    IPImage* SoundTrackIPNode::evaluateAudioTexture(const Context& context)
    {
        if (!inputs().empty() && m_fb)
        {
            if (m_sampleCurrent < m_sampleEnd && graph()->isAudioConfigured())
            {
                //
                //  Try and render
                //

                AudioCache& cache = graph()->audioCache();
                cache.lock();
                const size_t packetSize =
                    (cache.packetSize() > 0) ? cache.packetSize() : 512;
                const TwkAudio::Layout layout = cache.layout();
                const double rate = cache.rate();
                cache.unlock();

                int channels = TwkAudio::channelsCount(layout);
                size_t npackets =
                    MAX_SAMPLES_PROCESSED_PER_EVAL / (packetSize * channels);

                for (size_t n = 0;
                     m_sampleCurrent < m_sampleEnd && n < npackets;
                     m_sampleCurrent += packetSize, n++)
                {
                    AudioBuffer buffer(packetSize, layout, rate,
                                       samplesToTime(m_sampleCurrent, rate));

                    cache.lock();
                    const bool found = cache.fillBuffer(buffer);
                    cache.unlock();

                    if (found)
                    {
                        AudioContext acontext(buffer,
                                              graph()->cache().displayFPS());
                        renderAudio(acontext);
                    }
                }
            }

            IPImage* image =
                new IPImage(this, IPImage::BlendRenderType,
                            m_fb->referenceCopy(), IPImage::OutputTexture);

            image->tagMap[IPImage::textureIDTagName()] =
                IPImage::waveformTagValue();
            return image;
        }

        return 0;
    }

    void SoundTrackIPNode::propertyChanged(const Property* p)
    {
        IPNode::propertyChanged(p);

        const int w = m_visualWidth->front();
        const int h = m_visualHeight->front();
        const int fs = m_visualStartFrame->front();
        const int fe = m_visualEndFrame->front();

        if (w <= 0 || h <= 0 || (fs == 0 && fe == 0))
        {
            return;
        }

        if (p == m_visualHeight || p == m_visualWidth)
        {
            if (!m_fb)
            {
                m_fb = new FrameBuffer(w, h, 4, FrameBuffer::UCHAR);
                m_fb->staticRef();
                m_stats.clear();
            }

            if (m_fb && (m_fb->width() != w || m_fb->height() != h))
            {
                m_fb->restructure(w, h, 0, 4);
                m_stats.clear();
            }

            clearFB();
            m_stats.resize(h);
        }
        else if (p == m_visualStartFrame || p == m_visualEndFrame)
        {
            if (!m_fb)
            {
                m_fb = new FrameBuffer(w, h, 4, FrameBuffer::UCHAR);
                m_fb->staticRef();
                m_stats.clear();
            }
            m_stats.resize(h);

            updateRanges();
            clearFB();
        }
        else if (p == m_offset)
        {
            updateRanges();
        }
        else if (p == m_internalOffset)
        {
            updateRanges();
        }
    }

    void SoundTrackIPNode::clearFB()
    {
        if (m_fb)
        {
            memset(m_fb->pixels<unsigned char>(), 0, m_fb->planeSize());
            m_fb->idstream().str("");
            m_fb->idstream() << "EmptyAudioTexture";
            m_fb->attribute<string>("RVSource") = "soundtrack";
            fill(m_stats.begin(), m_stats.end(), RangeStats());
            m_sampleCurrent = m_sampleStart;
        }
    }

    void SoundTrackIPNode::updateRanges()
    {
        const int fs = m_visualStartFrame->front();
        const int fe = m_visualEndFrame->front();

        float offset = m_offset->front() + m_internalOffset->front();

        const SampleTime newStart = timeToSamples(
            ((fs - m_graphConfig.minFrame) / m_graphConfig.fps) - offset,
            m_audioConfig.rate);
        const SampleTime newEnd = timeToSamples(
            ((fe - m_graphConfig.minFrame + 1) / m_graphConfig.fps) - offset,
            m_audioConfig.rate);

        lockFB();
        m_sampleStart = newStart;
        m_sampleEnd = newEnd;
        unlockFB();

#if 0
    cout << "fs, fe = " << fs << ", " << fe << " -- "
         << "min, max = " << m_graphConfig.minFrame << ", " << m_graphConfig.maxFrame << " -- "
         << "range = " << newStart << " - " << newEnd << endl;
#endif
    }

    void SoundTrackIPNode::audioConfigure(const AudioConfiguration& config)
    {
        // cout << "audioConfigure" << endl;
        m_audioConfig = config;
        updateRanges();
        IPNode::audioConfigure(config);
    }

    void SoundTrackIPNode::graphConfigure(const GraphConfiguration& config)
    {
        // cout << "graphConfigure" << endl;
        m_graphConfig = config;
        updateRanges();
        IPNode::graphConfigure(config);
    }

    void SoundTrackIPNode::setInputs(const IPNodes& nodes)
    {
        IPNode::setInputs(nodes);
    }

    IPNode::ImageRangeInfo SoundTrackIPNode::imageRangeInfo() const
    {
        IPNodes nodes = inputs();

        if (!nodes.empty())
        {
            ImageRangeInfo i = nodes.front()->imageRangeInfo();

            if (i.inc == 0)
            {
                if (nodes.size() == 2)
                {
                    return nodes[1]->imageRangeInfo();
                }
            }

            return i;
        }

        return ImageRangeInfo();
    }

    void SoundTrackIPNode::renderAudio(const AudioContext& context)
    {
        if (!m_fb)
            return;

        AudioBuffer& buffer = context.buffer;
        const SampleTime s = buffer.startSample();
        if ((s + SampleTime(buffer.size()) < m_sampleStart)
            || (s > m_sampleEnd))
            return;

        lockFB();
        //
        //  Now that we have acquired the lock, check again that we want to
        //  render this buffer (in/out points  could have been reset in the
        //  meantime).
        //
        if ((s + SampleTime(buffer.size()) >= m_sampleStart)
            && (s <= m_sampleEnd))
        {
            renderAudioWaveform(context);
        }
        unlockFB();
    }

    void SoundTrackIPNode::renderAudioWaveform(const AudioContext& context)
    {
        if (m_sampleEnd == m_sampleStart)
            return;

        const int w = m_fb->width();
        const int h = m_fb->height();
        AudioBuffer& buffer = context.buffer;
        const SampleTime bufStart = buffer.startSample();

        static float totalRenderTime = 0.0;
        static float totalAudioTime = 0.0;

        const double l = double(m_sampleEnd - m_sampleStart);
        const double t0 = double(bufStart - m_sampleStart) / l;
        const double t1 =
            double((bufStart + buffer.size()) - m_sampleStart) / l;

        const int i0 = max(0, int(t0 * h));
        const int i1 = min(h - 1, int(t1 * h));
        const int in = i1 - i0 + 1;

        const AudioBuffer::BufferPointer p = buffer.pointer();
        const size_t channels = buffer.numChannels();
        const size_t nsamples = buffer.size();

        //  Keep track of last scanline, and clear statistics foreach
        //  new scanline;

        int lastSL = -1;
        RangeStats zero;

        //
        //  Make sure we only get stats for portion of buffer that overlaps with
        //  render window.
        //

        SampleTime firstSampleIndexInWindow =
            max(SampleTime(0), SampleTime(m_sampleStart - bufStart));
        SampleTime firstSampleIndexBeyondWindow =
            min(SampleTime(nsamples), SampleTime(m_sampleEnd - bufStart));

        const float normalize = 1.0f / channels;

        for (size_t i = firstSampleIndexInWindow * channels;
             i < firstSampleIndexBeyondWindow * channels; i += channels)
        {
            float v = 0.0f;
            for (int j = 0; j < channels; ++j)
                v += p[i + j];
            v *= normalize;

            v = ::pow(double(fabs(v)), 0.5) * (v < 0 ? -1.0 : 1.0);
            // float v = float(i)/float(nsamples*channels);  /* show position of
            // sample in buffer graphically */

            int sl = int(float(h)
                         * float(i / channels + bufStart - m_sampleStart) / l);
            if (sl >= i1)
                continue;
            if (sl < i0)
                continue;
            if (sl != lastSL)
            {
                m_stats[sl] = zero;
                lastSL = sl;
            }
            RangeStats& s = m_stats[sl];
            v = std::min(v, 1.0f);
            v = std::max(v, -1.0f);
            s.max = std::max(s.max, v);
            s.min = std::min(s.min, v);
            if (v >= 0.0f)
            {
                s.totalPos += v;
                s.nPos++;
            }
            else
            {
                s.totalNeg += v;
                s.nNeg++;
            }
        }

        int wn = w - 1;

        for (int sl = i0; sl <= i1 && sl < h; sl++)
        {
            RangeStats& s = m_stats[sl];
            if (s.min == std::numeric_limits<float>::max()
                || s.max == -std::numeric_limits<float>::max())
            {
                //
                //  We somehow messed up above and don't have stats for
                //  this scanline.  Might be a numerical problem.
                //
                continue;
            }
            unsigned char* scanline = m_fb->scanline<unsigned char>(sl);

            //
            //  Clear scanline before we draw on it.
            //
            for (int q = 0; q < wn; ++q)
            {
                unsigned char& b = scanline[q * 4 + 0];
                unsigned char& g = scanline[q * 4 + 1];
                unsigned char& r = scanline[q * 4 + 2];
                unsigned char& a = scanline[q * 4 + 3];

                r = 0;
                g = 0;
                b = 0;
                a = 255;
            }
            const int imedianPos =
                ((s.totalPos / float(s.nPos)) + 1.0) / 2.0 * wn;
            const int imedianNeg =
                ((s.totalNeg / float(s.nNeg)) + 1.0) / 2.0 * wn;
            const int imax = ((s.max + 1.0) / 2.0) * wn;
            const int imin = ((s.min + 1.0) / 2.0) * wn;

            for (int q = imin; q <= imax; q++)
            {
                unsigned char& b = scanline[q * 4 + 0];
                unsigned char& g = scanline[q * 4 + 1];
                unsigned char& r = scanline[q * 4 + 2];
                unsigned char& a = scanline[q * 4 + 3];

                int p = (q < imedianNeg || q > imedianPos) ? 150 : 255;
                r = p;
                g = p;
                b = p;
                a = 255;
            }
        }

        m_fb->idstream().str("");
        m_fb->idstream() << "AudioWaveform@" << m_fb << "/" << m_serialNumber++;
    }

    size_t SoundTrackIPNode::audioFillBuffer(const AudioContext& context)
    {
        //
        //  NOTE: the actual global audio offset, volume, and balance is
        //  stored on this node, but its *applied* in the IPGraph.
        //

        IPNodes nodes = inputs();
        if (nodes.empty())
            return 0;
        size_t rval = 0;

        AudioBuffer abuffer(context.buffer);
        AudioContext c(abuffer, context.fps);

        if (!nodes.empty() && nodes.size() <= 2)
        {
            rval = nodes.back()->audioFillBuffer(c);
        }
        else
        {
            rval = IPNode::audioFillBuffer(c);
        }

        if (m_fb)
            renderAudio(context);

        return rval;
    }

} // namespace IPCore
