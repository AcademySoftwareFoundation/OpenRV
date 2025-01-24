//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MovieStdinFB/MovieStdinFB.h>
#include <TwkFB/IO.h>
#include <TwkMath/Function.h>
#include <TwkMovie/Exception.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/File.h>
#include <TwkAudio/Audio.h>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <string>

namespace TwkMovie
{

    using namespace std;
    using namespace TwkFB;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace TwkMovie;

    MovieStdinFB::MovieStdinFB()
        : MovieReader()
        , m_threadGroup(1)
        , m_stopCaching(false)
        , m_io(0)
    {
    }

    MovieStdinFB::~MovieStdinFB()
    {
        m_stopCaching = true;
        m_threadGroup.control_wait();

        for (int i = 0; i < m_cache.size(); i++)
        {
            delete m_cache[i];
        }
    }

    static void dispatch(void* data)
    {
        MovieStdinFB* mov = reinterpret_cast<MovieStdinFB*>(data);
        mov->cacheFrames();
    }

    void MovieStdinFB::open(const string& filename, const MovieInfo& info,
                            const Movie::ReadRequest& request)
    {
        size_t len = filename.size();

        if (extension(filename) != "stdinfb")
        {
            TWK_THROW_EXC_STREAM("StdFB: Unknown file type \"" << filename
                                                               << "\"" << endl);
        }

        m_filename = basename(filename.substr(0, filename.size() - 8));
        vector<string> tokens;
        stl_ext::tokenize(tokens, m_filename, ",");

        if (tokens.empty())
        {
            TWK_THROW_EXC_STREAM("StdFB: Badly formed filename \""
                                 << m_filename << "\"" << endl);
        }

        m_imageType = tokens[0];
        m_info.start = 1;
        m_info.end = 1;
        m_info.inc = 1;
        m_info.fps = 24;
        m_info.width = 720;
        m_info.height = 486;
        m_info.uncropWidth = 720;
        m_info.uncropHeight = 486;
        m_info.audio = false;
        m_info.audioSampleRate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
        m_info.audioChannels = TwkAudio::layoutChannels(TwkAudio::Stereo_2);
        m_info.pixelAspect = 1;
        m_info.orientation = TwkFB::FrameBuffer::NATURAL;

        m_io = TwkFB::GenericIO::findByExtension(m_imageType);

        if (!m_io)
        {
            TWK_THROW_EXC_STREAM("StdFB: Unknown file type \"" << m_imageType
                                                               << "\"" << endl);
        }

        FrameBuffer::DataType dataType = FrameBuffer::UCHAR;
        FrameBuffer::AttributeVector attrs;

        for (int i = 1; i < tokens.size(); i++)
        {
            vector<string> statement;
            stl_ext::tokenize(statement, tokens[i], "=");

            if (statement.size() != 2)
            {
                cerr << "ERROR: bad stdfb specification: " << statement[0]
                     << endl;
                continue;
            }

            const string& name = statement[0];
            const char* val = statement[1].c_str();

            attrs.push_back(new StringAttribute("stdin/" + name, statement[1]));

            if (name == "start")
                m_info.start = atoi(val);
            else if (name == "end")
                m_info.end = atoi(val);
            else if (name == "fps")
                m_info.fps = atof(val);
            else if (name == "inc")
                m_info.inc = atoi(val);
        }

        m_cache.resize(m_info.end - m_info.start + 1);
        fill(m_cache.begin(), m_cache.end(), (FrameBuffer*)0);

        m_threadGroup.dispatch(dispatch, this);
    }

    void MovieStdinFB::cacheFrames()
    {
        for (int i = m_info.start; i <= m_info.end && !m_stopCaching; i++)
        {
            try
            {
                FrameBuffer* fb = new FrameBuffer();
                m_cache[i - m_info.start] = fb;
                m_io->readImage(*fb, "-", FrameBufferIO::ReadRequest());
            }
            catch (std::exception& exc)
            {
                cerr << "ERROR: StdinFB: " << exc.what() << endl;
            }
        }
    }

    void MovieStdinFB::imagesAtFrame(const ReadRequest& request,
                                     FrameBufferVector& fbs)
    {
        int frame = request.frame;
        fbs.resize(1);
        if (!fbs.front())
            fbs.front() = new FrameBuffer();
        FrameBuffer& fb = *fbs.front();

        if (FrameBuffer* ifb = m_cache[frame - m_info.start])
        {
            if (ifb->width() != 0)
            {
                fb.restructure(ifb->width(), ifb->height(), ifb->depth(),
                               ifb->numChannels(), ifb->dataType(),
                               ifb->pixels<unsigned char>(),
                               &ifb->channelNames(), ifb->orientation(), false);

                fb.setIdentifier("");
                identifier(frame, fb.idstream());

                //
                //  By incorporating the progress into the id hash, the frame
                //  will automatically be reloaded as the progress changes.
                //

                if (ifb->hasAttribute("Progress"))
                {
                    fb.idstream() << ":" << ifb->attribute<int>("Progress");
                }

                ifb->copyAttributesTo(&fb);
                return;
            }
        }

        fb.restructure(2, 2, 0, 4, FrameBuffer::UCHAR);
        memset(fb.pixels<char>(), 0, fb.allocSize());
        fb.attribute<bool>("RequestedFrameLoading") = true;
    }

    void MovieStdinFB::identifiersAtFrame(const ReadRequest& request,
                                          IdentifierVector& ids)
    {
        int frame = request.frame;
        ids.resize(1);
        ostringstream str;
        identifier(frame, str);
        ids.front() = str.str();
    }

    void MovieStdinFB::identifier(int frame, std::ostream& o)
    {
        o << m_filename << ":" << frame;
    }

    //----------------------------------------------------------------------

    MovieStdinFBIO::MovieStdinFBIO()
        : MovieIO("MovieStdinFB", "z")
    {
        unsigned int capabilities = MovieIO::MovieRead | MovieIO::AttributeRead;
        addType("stdinfb", capabilities);
    }

    MovieStdinFBIO::~MovieStdinFBIO() {}

    std::string MovieStdinFBIO::about() const
    {
        return "Tweak Stdin Image I/O";
    }

    MovieReader* MovieStdinFBIO::movieReader() const
    {
        return new MovieStdinFB();
    }

    MovieWriter* MovieStdinFBIO::movieWriter() const { return 0; }

    void MovieStdinFBIO::getMovieInfo(const std::string& filename,
                                      MovieInfo&) const
    {
        if (extension(filename) != "stdinfb")
        {
            TWK_THROW_STREAM(IOException, "Not a stdinfb: " << filename);
        }
    }

} // namespace TwkMovie
