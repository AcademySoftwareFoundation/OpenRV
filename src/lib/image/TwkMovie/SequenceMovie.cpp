//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/SequenceMovie.h>
#include <TwkMovie/MovieReader.h>
#include <iostream>

namespace TwkMovie
{
    using namespace std;
    using namespace TwkAudio;

    SequenceMovie::SequenceMovie(int fs)
        : m_fs(fs)
    {
    }

    SequenceMovie::~SequenceMovie() {}

    Movie* SequenceMovie::clone() const
    {
        SequenceMovie* m = new SequenceMovie(m_fs);

        for (int i = 0; i < m_movies.size(); i++)
        {
            m->addMovie(m_movies[i].movie->clone(), m_movies[i].inStart,
                        m_movies[i].inEnd);
        }

        return m;
    }

    void SequenceMovie::addMovie(Movie* m, int fs, int fe)
    {
        InputMovie im;
        im.movie = m;
        im.frames = fe - fs + 1;

        if (m_movies.empty())
        {
            im.inStart = fs;
        }
        else
        {
            im.inStart = m_movies.back().inEnd;
        }

        im.inEnd = im.inStart + im.frames;

        m_movies.push_back(im);

        if (m_movies.size() == 1)
        {
            m_info = m->info();
            m_info.start = m_movies.front().inStart;
            m_info.end = m_movies.back().inEnd;
        }
        else
        {
            m_info.end = m_movies.back().inEnd;
        }
    }

    void SequenceMovie::audioConfigure(const AudioConfiguration& conf)
    {
        for (size_t i = 0; i < m_movies.size(); i++)
        {
            m_movies[i].movie->audioConfigure(conf);
        }
    }

    size_t SequenceMovie::audioFillBuffer(const AudioReadRequest& request,
                                          AudioBuffer& inbuffer)
    {
        Time t = request.startTime;
        Time d = request.duration;
        size_t nc = inbuffer.numChannels();
        Time t0 = 0;
        Time t1 = 0;
        Time rate = inbuffer.rate();
        size_t nsamps = timeToSamples(d, rate);

        inbuffer.reconfigure(nsamps, inbuffer.channels(), rate, t);

        AudioBuffer::BufferPointer p = inbuffer.pointer();

        for (size_t i = 0; nsamps && i < m_movies.size(); i++)
        {
            InputMovie& im = m_movies[i];
            Movie* m = im.movie;

            Time t0 = Time(im.inStart) / double(m->info().fps);
            Time t1 = Time(im.inEnd) / double(m->info().fps);

            if (t >= t0 && t < t1)
            {
                Time tstart = t - t0;
                size_t remaining = timeToSamples(t1 - t, rate);
                size_t lsamps = nsamps < remaining ? nsamps : remaining;
                Time tlen = samplesToTime(lsamps, rate);

                AudioBuffer outbuffer(p, inbuffer.channels(), lsamps, tstart,
                                      rate);

                if (m->hasAudio())
                {
                    m->audioFillBuffer(AudioReadRequest(tstart, tlen),
                                       outbuffer);
                }
                else
                {
                    outbuffer.zero();
                }

                p += (lsamps * nc);
                nsamps -= lsamps;
            }

            t0 = t1;
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

    void SequenceMovie::imagesAtFrame(const ReadRequest& request,
                                      FrameBufferVector& fbs)
    {
        int frame = request.frame;
        int fstart = m_fs;
        int fend = m_fs;

        for (size_t i = 0; i < m_movies.size(); i++)
        {
            InputMovie& im = m_movies[i];
            Movie* m = im.movie;

            if (frame >= im.inStart && frame < im.inEnd)
            {
                int f = frame - im.inStart + m->info().start;

                Movie::ReadRequest r(f, request.stereo);
                copyExtraData(r, request);

                m->imagesAtFrame(r, fbs);
                return;
            }
        }
    }

    void SequenceMovie::identifiersAtFrame(const ReadRequest& request,
                                           IdentifierVector& ids)
    {
        int frame = request.frame;
        int fstart = m_fs;

        for (size_t i = 0; i < m_movies.size(); i++)
        {
            InputMovie& im = m_movies[i];
            Movie* m = im.movie;

            if (frame >= im.inStart && frame < im.inEnd)
            {
                int f = frame - im.inStart + m->info().start;

                Movie::ReadRequest r(f, request.stereo);
                copyExtraData(r, request);

                m->identifiersAtFrame(r, ids);
                return;
            }
        }
    }

} // namespace TwkMovie
