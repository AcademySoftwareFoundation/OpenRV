//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/LeaderFooterMovie.h>
#include <TwkMovie/MovieReader.h>
#include <iostream>

namespace TwkMovie
{
    using namespace std;
    using namespace TwkAudio;

    LeaderFooterMovie::LeaderFooterMovie(Movie* movie, int fs, int fe,
                                         Movie* leader, Movie* footer)
    {
        m_movie = movie;
        m_leader = leader;
        m_footer = footer;

        m_fs = fs;
        m_fe = fe;

        if (leader)
        {
            fs -= leader->info().end - leader->info().start + 1;
        }

        if (footer)
        {
            fe += footer->info().end - footer->info().end + 1;
        }

        m_info = movie->info();
        m_info.start = fs;
        m_info.end = fe;
        m_info.inc = 1;
        m_offset = 0;
    }

    LeaderFooterMovie::~LeaderFooterMovie() {}

    void LeaderFooterMovie::audioConfigure(const AudioConfiguration& conf)
    {
        if (m_leader)
            m_leader->audioConfigure(conf);
        m_movie->audioConfigure(conf);
        if (m_footer)
            m_footer->audioConfigure(conf);
    }

    size_t LeaderFooterMovie::audioFillBuffer(const AudioReadRequest& request,
                                              AudioBuffer& inbuffer)
    {
        Time t = request.startTime;
        Time d = request.duration;
        float fps = m_movie->info().fps;
        double rate = m_movie->info().audioSampleRate;
        size_t nc = m_movie->info().audioChannels.size();
        SampleTime osamps = timeToSamples(d, rate);
        SampleTime nsamps = timeToSamples(d, rate);
        SampleTime start = timeToSamples(t, rate);

        inbuffer.reconfigure(nsamps, m_movie->info().audioChannels, rate, t);
        inbuffer.zero();

        int leaderFrames =
            (!m_leader) ? 0 : m_leader->info().end - m_leader->info().start + 1;
        size_t nleader = timeToSamples(double(leaderFrames) / fps, rate);

        if (start < nleader)
        {
            if ((start + nsamps) > nleader)
            {
                size_t diff = nleader - start;

                //
                // Since we step backwards an m_offset amount each buffer fill,
                // adjust start to compensate for our first read
                //

                m_offset = nleader;
                start = m_offset;
                nsamps -= diff;
                d -= samplesToTime(diff, rate);
            }
            else
            {
                return inbuffer.size();
            }
        }

        start +=
            timeToSamples(double(m_fs - m_movie->info().start) / fps, rate);

        start -= m_offset;
        AudioReadRequest nrequest(samplesToTime(start, rate), d);

        m_movie->audioFillBuffer(nrequest, inbuffer);
        inbuffer.reconfigure(osamps, inbuffer.channels(), rate, t);

        if (osamps != nsamps)
        {
            memmove((void*)(inbuffer.pointer() + (osamps - nsamps) * nc),
                    (void*)inbuffer.pointer(), nsamps * sizeof(float) * nc);

            memset(inbuffer.pointer(), 0,
                   sizeof(float) * nc * (osamps - nsamps));
        }

        return inbuffer.size();
    }

    static void copyExtraData(TwkMovie::Movie::ReadRequest& dst,
                              const TwkMovie::Movie::ReadRequest& src)
    {
        dst.views = src.views;
        dst.layers = src.layers;
        dst.missing = src.missing;
    }

    void LeaderFooterMovie::imagesAtFrame(const ReadRequest& request,
                                          FrameBufferVector& fbs)
    {
        int frame = request.frame;
        int leaderFrames = 0;
        if (m_leader)
            leaderFrames = m_leader->info().end - m_leader->info().start + 1;

        if (frame >= m_fs && frame <= m_fe)
        {
            m_movie->imagesAtFrame(request, fbs);
        }
        else if (frame < m_fs)
        {
            if (m_leader)
            {
                ReadRequest r(frame - (m_fs - leaderFrames)
                                  + m_leader->info().start,
                              request.stereo);
                copyExtraData(r, request);
                m_leader->imagesAtFrame(r, fbs);
            }
            else
            {
                ReadRequest r(m_fs, request.stereo);
                copyExtraData(r, request);
                m_movie->imagesAtFrame(r, fbs);
            }
        }
        else
        {
            if (m_footer)
            {
                ReadRequest r(frame - m_fe + m_footer->info().start,
                              request.stereo);
                copyExtraData(r, request);
                m_footer->imagesAtFrame(r, fbs);
            }
            else
            {
                ReadRequest r(m_fe, request.stereo);
                copyExtraData(r, request);
                m_movie->imagesAtFrame(r, fbs);
            }
        }
    }

    void LeaderFooterMovie::identifiersAtFrame(const ReadRequest& request,
                                               IdentifierVector& ids)
    {
        int frame = request.frame;
        int leaderFrames = 0;
        if (m_leader)
            leaderFrames = m_leader->info().end - m_leader->info().start + 1;

        if (frame >= m_fs && frame <= m_fe)
        {
            m_movie->identifiersAtFrame(request, ids);
        }
        else if (frame < m_fs)
        {
            if (m_leader)
            {
                ReadRequest r(frame - (m_fs - leaderFrames)
                                  + m_leader->info().start,
                              request.stereo);
                m_leader->identifiersAtFrame(r, ids);
            }
            else
            {
                ReadRequest r(m_fs, request.stereo);
                m_movie->identifiersAtFrame(r, ids);
            }
        }
        else
        {
            if (m_footer)
            {
                ReadRequest r(frame - m_fe + m_footer->info().start,
                              request.stereo);
                m_footer->identifiersAtFrame(r, ids);
            }
            else
            {
                ReadRequest r(m_fe, request.stereo);
                m_movie->identifiersAtFrame(r, ids);
            }
        }
    }

} // namespace TwkMovie
