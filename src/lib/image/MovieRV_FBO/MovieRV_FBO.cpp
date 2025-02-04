//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/AudioRenderer.h>
#include <RvApp/RvSession.h>
#include <IPCore/ImageRenderer.h>
#include <MovieRV_FBO/MovieRV_FBO.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/OutputGroupIPNode.h>
#include <TwkAudio/Audio.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkContainer/GTOReader.h>
#include <TwkFB/Exception.h>
#include <TwkFB/IO.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLFFBO/FBOVideoDevice.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLState.h>
#include <TwkGLText/TwkGLText.h>
#include <iostream>

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#endif

namespace TwkFB
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkGLF;
    using namespace TwkMath;
    using namespace TwkAudio;
    using namespace IPCore;

    struct AttrCollector
    {
        AttrCollector(vector<string>& k, vector<string>& v)
            : keys(k)
            , values(v)
        {
        }

        vector<string>& keys;
        vector<string>& values;

        void operator()(IPImage* i)
        {
            if (i->fb)
            {
                string prefix = "";

                if (i->fb->hasAttribute("RVSource"))
                {
                    prefix = i->fb->attribute<string>("RVSource");
                }

                const TwkFB::FrameBuffer::AttributeVector& attrs =
                    i->fb->attributes();

                if (prefix != "")
                    prefix = "/" + prefix + "/./";

                for (size_t i = 0; i < attrs.size(); i++)
                {
                    keys.push_back(prefix + attrs[i]->name());
                    values.push_back(attrs[i]->valueAsString());
                }
            }
        }
    };

    TwkGLF::GLVideoDevice* MovieRV::m_device = 0;

    MovieRV::MovieRV()
        : MovieReader()
        , EventNode("MovieRV")
        , m_session(0)
        , m_audioChannels(TwkAudio::layoutChannels(TwkAudio::Stereo_2))
        , m_audioRate(TWEAK_AUDIO_DEFAULT_SAMPLE_RATE)
        , m_audioPacketSize(512)
        , m_audioInit(true)
    // m_device(0)
    {
        //
        //  Default to 44.1khz 2 channels audio
        //
    }

    MovieRV::~MovieRV() { delete m_session; }

    void MovieRV::uninit()
    {
        m_device->makeCurrent();
        TwkGLF::UninitPBOPools();
    }

    void MovieRV::open(const string& filename, const TwkMovie::MovieInfo& info,
                       const Movie::ReadRequest& request)
    {
        // nothing works for this
    }

    void MovieRV::open(Rv::RvSession* session, const TwkMovie::MovieInfo& info,
                       TwkAudio::ChannelsVector audioChannels, double audioRate,
                       size_t audioPacketSize)
    {
        m_info = info;
        ostringstream idstream;
        if (audioRate)
            m_audioRate = audioRate;
        m_audioPacketSize = audioPacketSize;
        if (audioChannels.size())
            m_audioChannels = audioChannels;

        AudioRenderer::setNoAudio(true);

        m_session = session;
        m_session->listenTo(this);
        m_session->setOpaquePointer(this);

        TwkMath::Vec2i size = m_session->maxSize();

        if (info.width > 0 && info.height > 0)
        {
            size = Vec2i(info.width, info.height);
        }

        // cout << "size = " << size.x << ", " << size.y << endl;

        m_info.start = m_session->rangeStart();
        m_info.end = m_session->rangeEnd() - 1;
        m_info.inc = m_session->inc();
        m_info.fps = m_session->fps();
        m_info.pixelAspect = 1.0;
        m_info.audio = m_session->hasAudio();
        m_info.audioSampleRate = m_audioRate;
        m_info.audioChannels = m_audioChannels;
        m_info.orientation = FrameBuffer::NATURAL;

        Session::setUsePreEval(false);
        m_session->setCaching(Session::NeverCache);
        m_session->graph().setAudioCacheExtents(3.0, 5.0);
        m_session->graph().setAudioThreading(false);

        idstream << m_filename << ":RV"
                 << ":w" << m_info.width << ":h" << m_info.height;

        m_idstring = idstream.str();
        m_thread = pthread_self();
    }

    void MovieRV::imagesAtFrame(const ReadRequest& request,
                                FrameBufferVector& fbs)
    {
        int frame = request.frame;
        bool stereo = request.stereo;

        if (!m_device)
        {
            m_device =
                new FBOVideoDevice(0, m_info.uncropWidth, m_info.uncropHeight,
                                   m_info.numChannels == 4, (stereo) ? 2 : 1);
            TWK_GLDEBUG;
            m_device->makeCurrent();
            TWK_GLDEBUG;
            ImageRenderer::queryGL();
            TWK_GLDEBUG;
        }

        if (m_session->controlVideoDevice() != m_device)
        {
            m_session->setControlVideoDevice(m_device);
            m_session->setOutputVideoDevice(m_device);
        }

        m_device->makeCurrent();
        m_thread = pthread_self();

        //
        //  Find the stereo node and make sure its in an acceptable
        //  state. For example, don't let it be in hardware mode.
        //

        DisplayStereoIPNode* stereoNode = 0;

        IPGraph::NodeVector nodes;
        m_session->graph().findNodesByTypeName(frame, nodes, "RVDisplayStereo");
        m_session->makeCurrentSession();

        if (!nodes.empty()
            && (stereoNode = dynamic_cast<DisplayStereoIPNode*>(nodes.front())))
        {
            if (stereoNode->stereoType() == "hardware")
            {
                stereoNode->setStereoType("off");
            }
        }
        else
        {
            stereo = false;
        }

        fbs.resize(stereo ? 2 : 1);
        TWK_GLDEBUG;

        for (size_t i = 0; i < fbs.size(); i++)
        {
            if (!fbs[i])
                fbs[i] = new FrameBuffer();
            FrameBuffer* fb = fbs[i];

            m_device->setDefaultFBOIndex(i);

            fb->restructure(m_device->width(), m_device->height(), 1, 4,
                            TwkFB::FrameBuffer::HALF, 0, 0,
                            TwkFB::FrameBuffer::NATURAL, true);

            m_device->makeCurrent();

            if (stereo && stereoNode)
            {
                stereoNode->setStereoType(i == 0 ? "left" : "right");
            }

            m_session->setRealtime(false);
            m_session->setFrame(frame);
            m_session->render();
            m_device->makeCurrent();

            GLenum dtype = GL_FLOAT;
            GLenum ctype = fb->numChannels() == 3 ? GL_RGB : GL_RGBA;

            switch (fb->dataType())
            {
            case FrameBuffer::FLOAT:
                dtype = GL_FLOAT;
                break;
            case FrameBuffer::HALF:
                dtype = GL_HALF_FLOAT_ARB;
                break;
            case FrameBuffer::UCHAR:
                dtype = GL_UNSIGNED_INT_8_8_8_8;
                break;
            case FrameBuffer::USHORT:
                dtype = GL_UNSIGNED_SHORT;
                break;
            default:
                break;
            }

            glReadPixels(0, 0, fb->width(), fb->height(), ctype, dtype,
                         fb->pixels<GLvoid>());
            glFinish();

            if (const IPImage* ipimage = m_session->displayImage())
            {
                vector<string> keys;
                vector<string> values;
                AttrCollector collector(keys, values);
                foreach_ip(const_cast<IPImage*>(ipimage), collector);

                for (size_t i = 0; i < keys.size(); i++)
                {
                    fb->attribute<string>(keys[i]) = values[i];
                }
            }

            fb->setIdentifier("");
            fb->attribute<TwkGLF::GLVideoDevice*>("videoDevice") = m_device;
            fb->attribute<TwkGLF::GLState*>("glState") =
                m_session->renderer()->getGLState();
            fb->attribute<string>("renderer") = "hw";
            identifier(frame, fb->idstream());

            const vector<string>& missing = m_session->missingFrameInfo();
            ostringstream str;

            for (size_t i = 0; i < missing.size(); i++)
            {
                if (i)
                    str << endl;
                str << missing[i];
            }

            if (!missing.empty())
                fb->attribute<string>("missing-image") = str.str();
        }
    }

    void MovieRV::identifiersAtFrame(const ReadRequest& request,
                                     IdentifierVector& ids)
    {
        int frame = request.frame;
        ostringstream str;
        identifier(frame, str);
        ids.resize(1);
        ids.front() = str.str();
    }

    void MovieRV::identifier(int frame, ostream& o)
    {
        o << m_idstring << ":" << frame;
    }

    size_t MovieRV::audioFillBuffer(const AudioReadRequest& request,
                                    AudioBuffer& buffer)
    {
        size_t nsamps = timeToSamples(request.duration, m_audioRate);
        buffer.reconfigure(nsamps, m_audioChannels, m_audioRate,
                           request.startTime);
        IPNode::AudioContext context(buffer, m_session->fps());

        if (m_audioInit)
        {
            //
            //  Initialize as late as possible (which is right before we
            //  actually need the audio).
            //

            IPGraph::AudioConfiguration config(
                m_audioRate,
                TwkAudio::channelLayouts(m_audioChannels.size()).front(),
                m_audioPacketSize, m_session->fps(), m_session->inc() < 0,
                buffer.startSample(),
                size_t((m_session->rangeEnd() - m_session->rangeStart())
                           / m_session->fps() * m_audioRate
                       + 0.49));

            m_session->graph().audioConfigure(config);
            m_audioInit = false;
            // m_session->graph().audioFillBuffer(context);
        }

        return m_session->graph().audioFillBuffer(context);
    }

    bool MovieRV::audioCacheSlice() { return false; }

    void MovieRV::blockingCacheAudio()
    {
        const Rv::RvSession::Sources& sources = m_session->sources();

        for (int i = 0; i < sources.size(); i++)
        {
            if (FileSourceIPNode* node =
                    dynamic_cast<FileSourceIPNode*>(sources[i]))
            {
                // for (int q=0; q < node->numLayers(); q++)
                //{
                // Movie* mov = node->layer(q);
                //}
            }
        }
    }

    void MovieRV::flush() {}

    //----------------------------------------------------------------------

    MovieRVIO::MovieRVIO()
        : TwkMovie::MovieIO()
    {
        StringPairVector codecs;
        StringPairVector acodecs;
        addType("rv", "Tweak RV file", MovieRead | MovieReadAudio, codecs,
                acodecs);
    }

    MovieRVIO::~MovieRVIO() {}

    std::string MovieRVIO::about() const { return "MovieRV_FBO"; }

    TwkMovie::MovieReader* MovieRVIO::movieReader() const
    {
        return new MovieRV();
    }

    TwkMovie::MovieWriter* MovieRVIO::movieWriter() const { return 0; }

    TwkMovie::MovieInfo
    MovieRVIO::getMovieInfo(const std::string& filename) const
    {
        TwkMovie::MovieInfo minfo;
        GTOReader reader;
        GTOReader::Containers containers = reader.read(filename.c_str());

        minfo.width = 0;
        minfo.height = 0;
        minfo.numChannels = 4;
        minfo.dataType = TwkFB::FrameBuffer::UCHAR;
        minfo.orientation = TwkFB::FrameBuffer::NATURAL;

        for (int i = 0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            string p = protocol(pc);

            if (p == "RVSession")
            {
                StringProperty* sp =
                    pc->property<StringProperty>("session", "sources");
                Vec2iProperty* rp =
                    pc->property<Vec2iProperty>("session", "range");
                FloatProperty* fpsp =
                    pc->property<FloatProperty>("session", "fps");
                IntProperty* incp = pc->property<IntProperty>("session", "inc");

                if (sp->empty() || rp->empty() || fpsp->empty()
                    || incp->empty())
                {
                    for (int i = 0; i < containers.size(); i++)
                        delete containers[i];
                    TWK_THROW_STREAM(
                        TwkFB::IOException,
                        "Unable to read session object: " << filename);
                }

                minfo.start = rp->front().x;
                minfo.end = rp->front().y;
                minfo.inc = incp->front();
                minfo.fps = fpsp->front();
                minfo.quality = 1.0;
                minfo.pixelAspect = 1.0;
                minfo.audio = true;
                minfo.audioSampleRate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
                minfo.audioChannels =
                    TwkAudio::layoutChannels(TwkAudio::Stereo_2);
            }

            if (p == "RVSource")
            {
                //
                //  This is approximate. If there's a scale on one of these
                //  the result will be incorrect.
                //

                if (Vec2iProperty* rp =
                        pc->property<Vec2iProperty>("proxy", "size"))
                {
                    minfo.width = std::max(rp->front().x, minfo.width);
                    minfo.height = std::max(rp->front().y, minfo.height);
                }
            }
        }

        TWK_THROW_STREAM(TwkFB::IOException,
                         "Unable to read session object: " << filename);

        return minfo;
    }

} // namespace TwkFB
