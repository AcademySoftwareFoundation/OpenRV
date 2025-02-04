//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/ReformattingMovie.h>
#include <TwkFB/Operations.h>
#include <TwkFBAux/FBAux.h>
#include <math.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Iostream.h>
#include <TwkAudio/Audio.h>
#include <TwkAudio/Resampler.h>
#include <TwkAudio/Mix.h>
#include <limits>
#include <sstream>

#define AUDIO_READPOSITIONOFFSET_THRESHOLD \
    0.01 // In secs; this is the max amount of slip we will allow

namespace TwkMovie
{
    using namespace TwkFB;
    using namespace TwkFBAux;
    using namespace std;
    using namespace TwkMath;
    using namespace TwkAudio;

    ReformattingMovie::ReformattingMovie(Movie* mov)
        : Movie()
        , m_movie(mov)
        , m_ysamples(0)
        , m_asamples(0)
        , m_rysamples(0)
        , m_bysamples(0)
        , m_usamples(0)
        , m_vsamples(0)
        , m_ingamma(1.0f)
        , m_inlog(false)
        , m_inlogc(false)
        , m_inRedLog(false)
        , m_inRedLogFilm(false)
        , m_outlog(false)
        , m_outLogC(false)
        , m_outLogCEI(0.0f)
        , m_outRedLog(false)
        , m_outRedLogFilm(false)
        , m_outgamma(1.0f)
        , m_flip(false)
        , m_flop(false)
        , m_exposure(0.0f)
        , m_verbose(false)
        , m_useFloat(false)
        , m_scale(1.0f)
        , m_xsize(0)
        , m_ysize(0)
        , m_inpremult(false)
        , m_inunpremult(false)
        , m_inSRGB(false)
        , m_outSRGB(false)
        , m_outRec709(false)
        , m_outpremult(false)
        , m_outunpremult(false)
        , m_output709toACES(false)
        , m_startFrame(numeric_limits<int>::min())
        , m_endFrame(numeric_limits<int>::min())
        , m_outtype(FrameBuffer::__NUM_TYPES__)
        , m_audioRate(0.0)
        , m_audioChannels(layoutChannels(UnknownLayout))
        , m_orientation(FrameBuffer::__NUM_ORIENTATION__)
        , m_outWhiteX(999)
        , m_outWhiteY(999)
    {
        m_info = mov->info();
        m_astate = new ResamplingMovie(mov);
    }

    ReformattingMovie::~ReformattingMovie() { delete m_astate; }

    Movie* ReformattingMovie::clone() const
    {
        ReformattingMovie* m = new ReformattingMovie(m_movie->clone());
        m->setFPS(m_fps);
        m->setAudio(m_audioRate, m_audioSamples, m_audioChannels);
        m->setTimeRange(m_startFrame, m_endFrame);
        m->setUseFloatingPoint(m_useFloat);
        m->setVerbose(m_verbose);
        m->setInputLogSpace(m_inlog);
        m->setInputLogCSpace(m_inlogc);
        m->setInputRedLogSpace(m_inRedLog);
        m->setInputRedLogFilmSpace(m_inRedLogFilm);
        m->setInputSRGB(m_inSRGB);
        m->setInputGamma(m_ingamma);
        if (m_inpremult)
            m->setInputPremultiply();
        if (m_inunpremult)
            m->setInputUnpremultiply();
        m->setRelativeExposure(m_exposure);

        m->setOutputResolution(m_xsize, m_ysize);
        m->setFBScaling(m_scale);

        m->setChannelMap(m_channelMap);
        m->setFlip(m_flip);
        m->setFlop(m_flop);
        m->setOutputOrientation(m_orientation);
        m->convertToYRYBY(m_ysamples, m_rysamples, m_bysamples, m_asamples);

        m->convertToYUV(m_ysamples, m_usamples, m_vsamples);

        if (m_outpremult)
            m->setOutputPremultiply();
        if (m_outunpremult)
            m->setOutputUnpremultiply();
        m->setOutputLogSpace(m_outlog);
        m->setOutputSRGB(m_outSRGB);
        m->setOutputRec709(m_outRec709);
        m->setOutputLogC(m_outLogC);
        m->setOutputLogCEI(m_outLogCEI);
        m->setOutputRedLog(m_outRedLog);
        m->setOutputRedLogFilm(m_outRedLogFilm);
        m->setOutputGamma(m_outgamma);
        m->setOutputFormat(m_outtype);

        return m;
    }

    //----------------------------------------------------------------------
    //
    //  Audio
    //

    bool ReformattingMovie::hasAudio() const { return m_movie->hasAudio(); }

    void ReformattingMovie::setFPS(float fps) { m_fps = fps; }

    void ReformattingMovie::setAudio(float rate, size_t samples,
                                     ChannelsVector channels)
    {
        m_audioRate = rate;
        m_audioChannels = channels;
        m_audioSamples = samples;

        if (!m_audioRate)
            m_audioRate = m_movie->info().audioSampleRate;
        if (!m_audioChannels.size())
            m_audioChannels = m_movie->info().audioChannels;

        if (m_audioRate > 0)
        {
            double factor =
                m_audioRate / double(m_movie->info().audioSampleRate);
            m_astate->reset(m_audioChannels.size(), factor, m_audioSamples);
        }
    }

    void ReformattingMovie::audioConfigure(const AudioConfiguration& conf)
    {
        setAudio(conf.rate, conf.bufferSize, layoutChannels(conf.layout));
        m_movie->audioConfigure(conf);
    }

    size_t
    ReformattingMovie::audioFillBuffer(const AudioReadRequest& outRequest,
                                       AudioBuffer& inbuffer)
    {
        AudioBuffer rsBuffer(inbuffer.size(), inbuffer.channels(),
                             inbuffer.rate(), inbuffer.startTime());

        size_t nread = m_astate->audioFillBuffer(rsBuffer);

        mixChannels(rsBuffer, inbuffer, 1.0, 1.0, false);

        return nread;
    }

    //----------------------------------------------------------------------
    //
    //  Video
    //

    void ReformattingMovie::flush() { m_movie->flush(); }

    void ReformattingMovie::setTimeRange(int start, int end)
    {
        m_startFrame = start;
        m_endFrame = end;
        m_info.start = start;
        m_info.end = end;
    }

    void ReformattingMovie::setFBScaling(float f)
    {
        m_scale = f;

        if (m_xsize && m_ysize)
        {
            m_info.width = int(m_xsize * m_scale + 0.49f);
            m_info.height = int(m_ysize * m_scale + 0.49f);
        }
        else
        {
            m_info.width = int(m_movie->info().width * m_scale + 0.49f);
            m_info.height = int(m_movie->info().height * m_scale + 0.49f);
        }
    }

    void ReformattingMovie::setOutputResolution(int w, int h)
    {
        m_xsize = w;
        m_ysize = h;

        if (w && h)
        {
            m_info.width = int(m_xsize * m_scale + 0.49);
            m_info.height = int(m_ysize * m_scale + 0.49);
        }
        else
        {
            m_info.width = int(m_movie->info().width * m_scale + 0.49);
            m_info.height = int(m_movie->info().height * m_scale + 0.49);
        }
    }

    //
    //  This is the meat of this class.
    //

    void ReformattingMovie::imagesAtFrame(const ReadRequest& request,
                                          FrameBufferVector& fbs)
    {
        int frame = request.frame;

        //
        //  Get the frame from the upstream movie object
        //

        if (m_startFrame != numeric_limits<int>::min())
        {
            if (frame < m_startFrame)
                frame = m_startFrame;
            if (frame > m_endFrame)
                frame = m_endFrame;
        }

        m_movie->imagesAtFrame(request, fbs);
        ostringstream idstr;
        identifier(idstr);

        for (unsigned int q = 0; q < fbs.size(); q++)
        {
            FrameBuffer& fb = *fbs[q];
            FrameBuffer* outfb = &fb;
            FrameBuffer attrHolder;

            fb.copyAttributesTo(&attrHolder);

#if 0 //  ALAN_UNCROP
        cerr << "ReformattingMovie::imagesAtFrame " << outfb->identifier() << 
                " w " << outfb->width() << " h " << outfb->height() <<
                " uncrop w " << outfb->uncropWidth() << " h " << outfb->uncropHeight() <<
                " x " << outfb->uncropX() << " y " << outfb->uncropY() <<
                " on " << outfb->uncrop() <<
                endl;
#endif

            bool useFloat = m_useFloat || m_rysamples;

            //
            //  If its planar or non-REC 709 convert it to
            //  what we expect.
            //

            if (outfb->dataType() == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8
                || outfb->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8)
            {
                FrameBuffer* infb = outfb;

                if (useFloat)
                {
                    outfb = copyConvert(infb, FrameBuffer::FLOAT);
                }
                else
                {
                    outfb = convertToLinearRGB709(infb);
                }

                if (m_verbose)
                    cout << "INFO: converted to RGB from UYVY" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (outfb->isPlanar())
            {
                FrameBuffer* infb = outfb;
                outfb = mergePlanes(infb);
                if (m_verbose)
                    cout << "INFO: converted to packed pixels" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if ((outfb->numChannels() == 3 || outfb->numChannels() == 4)
                && !outfb->isYUV() && !outfb->isYRYBY())
            {
                if (outfb->channelName(0) != "R" || outfb->channelName(1) != "G"
                    || outfb->channelName(2) != "B")
                {
                    FrameBuffer* infb = outfb;
                    vector<string> chmap;
                    chmap.push_back("R");
                    chmap.push_back("G");
                    chmap.push_back("B");
                    if (outfb->numChannels() == 4)
                        chmap.push_back("A");

                    outfb = channelMap(infb, chmap);
                    if (infb != &fb)
                        delete infb;

                    if (m_verbose)
                    {
                        cout << "INFO: remapped channels to R G B" << endl;
                    }
                }
            }

            if (useFloat && outfb->dataType() != FrameBuffer::FLOAT)
            {
                FrameBuffer* infb = outfb;
                outfb = copyConvert(infb, FrameBuffer::FLOAT);
                // if (m_verbose) cout << "INFO: processing at best quality (32
                // bit float)"
                //<< endl;
                if (infb != &fb)
                    delete infb;
            }

            if (outfb->hasPrimaries() || outfb->isYUV() || outfb->isYRYBY())
            {
                FrameBuffer* infb = outfb;
                outfb = convertToLinearRGB709(infb);
                if (m_verbose)
                    cout << "INFO: converted to REC 709" << endl;
                if (infb != &fb)
                    delete infb;
            }

            //
            //  Convert to ACES gamut
            //

            if (m_output709toACES)
            {
                FrameBuffer* infb = outfb;
                Mat44f M;

                if (m_outWhiteX != 999 && m_outWhiteY != 999)
                {
                    infb->setAdoptedNeutral(m_outWhiteX, m_outWhiteY);
                }

                acesMatrix(infb, (float*)&M, true);
                applyTransform(infb, outfb, linearColorTransform, &M);
                if (m_verbose)
                    cout << "INFO: ACES gamut" << endl;
                if (infb != &fb)
                    delete infb;
            }
            else if (m_outWhiteX != 999 && m_outWhiteY != 999)
            {
                float chr[8] = {0.6400f, 0.3300f, 0.3000f, 0.6000f,
                                0.1500f, 0.0600f, 0.3127f, 0.3290f};

                FrameBuffer* infb = outfb;
                Mat44f M;

                float neutral[2] = {chr[6], chr[7]};
                float newNeutral[2] = {m_outWhiteX, m_outWhiteY};

                TwkFB::colorSpaceConversionMatrix(
                    (const float*)&chr, (const float*)&chr,
                    (const float*)&neutral, (const float*)&newNeutral, true,
                    (float*)&M);

                applyTransform(infb, outfb, linearColorTransform, &M);
                if (m_verbose)
                    cout << "INFO: White Point" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_channelMap.size())
            {
                FrameBuffer* infb = outfb;
                outfb = channelMap(infb, m_channelMap);
                if (m_verbose)
                {
                    cout << "INFO: remapped channels to";
                    for (int i = 0; i < m_channelMap.size(); i++)
                        cout << " " << m_channelMap[i];
                    cout << endl;
                }

                if (infb != &fb)
                    delete infb;
            }

            //
            //  If its a Cineon or DPX and it needs log->lin do it,
            //  otherwise if it is in some gamma space bring it back to
            //  linear.
            //

            if (m_inlog)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLogToLinear(infb, outfb);
                if (m_verbose)
                    cout << "INFO: log -> lin" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_inRedLog)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertRedLogToLinear(infb, outfb);
                if (m_verbose)
                    cout << "INFO: redlog -> lin" << endl;
                if (infb != &fb)
                    delete infb;
            }

            // RedLogFilm is identical to CineonLog;
            // see aces v1.0.1 distribution.
            if (m_inRedLogFilm)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLogToLinear(infb, outfb);
                if (m_verbose)
                    cout << "INFO: redlogfilm -> lin" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_inSRGB)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertSRGBToLinear(infb, outfb);
                if (m_verbose)
                    cout << "INFO: sRGB -> lin" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_ingamma != 1.0)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                applyGamma(infb, outfb, 1.0 / m_ingamma);
                if (m_verbose)
                    cout << "INFO: applied input gamma correction of "
                         << m_ingamma << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_inpremult)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                premult(infb, outfb);
                if (m_verbose)
                    cout << "INFO: applied input premult" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_inunpremult)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                unpremult(infb, outfb);
                if (m_verbose)
                    cout << "INFO: applied input unpremult" << endl;
                if (infb != &fb)
                    delete infb;
            }

            //
            //  Apply any linear transforms (the image is linear now)
            //

            if (m_scale != 1.0)
            {
                FrameBuffer* infb = outfb;

                outfb = new FrameBuffer(
                    infb->coordinateType(), int(infb->width() * m_scale),
                    int(infb->height() * m_scale), infb->depth(),
                    infb->numChannels(), infb->dataType(), 0,
                    &infb->channelNames(), infb->orientation(), true);

                resize(infb, outfb);

                if (m_verbose)
                {
                    cout << "INFO: new image geometry: ";
                    outfb->outputInfo(cout);
                    cout << endl;
                }

                if (infb != &fb)
                    delete infb;
            }

            if (m_xsize
                && (outfb->width() != m_xsize || outfb->height() != m_ysize))
            {
                FrameBuffer* infb = outfb;

                outfb = new FrameBuffer(
                    infb->coordinateType(), m_xsize, m_ysize, infb->depth(),
                    infb->numChannels(), infb->dataType(), 0,
                    &infb->channelNames(), infb->orientation(), true);

                resize(infb, outfb);

                if (m_verbose)
                {
                    cout << "INFO: new image geometry: ";
                    outfb->outputInfo(cout);
                    cout << endl;
                }

                if (infb != &fb)
                    delete infb;
            }

            if (m_exposure != 0.0)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                float s = pow(2.0, (double)m_exposure);
                Mat44f M;
                M.makeScale(Vec3f(s, s, s));
                applyTransform(infb, outfb, linearColorTransform, &M);
                if (m_verbose)
                    cout << "INFO: applied relative exposure of " << m_exposure
                         << " stops" << endl;
                if (infb != &fb)
                    delete infb;
            }

            //
            //  Flip / Flop
            //

            if (m_flip)
            {
                if (m_verbose)
                    cout << "INFO: flipping" << endl;
                const FrameBuffer* infb = outfb;
                outfb = infb->copy();
                flip(outfb);
                if (infb != &fb)
                    delete infb;
            }

            if (m_flop)
            {
                if (m_verbose)
                    cout << "INFO: flopping" << endl;
                const FrameBuffer* infb = outfb;
                outfb = infb->copy();
                flop(outfb);
                if (infb != &fb)
                    delete infb;
            }

            if (m_orientation != FrameBuffer::__NUM_ORIENTATION__
                && outfb->orientation() != m_orientation)
            {
                if (m_verbose)
                    cout << "INFO: reorienting" << endl;
                const FrameBuffer* infb = outfb;
                outfb = infb->copy();

                switch (m_orientation)
                {
                case FrameBuffer::NATURAL:
                    switch (outfb->orientation())
                    {
                    case FrameBuffer::TOPLEFT:
                        flip(outfb);
                        break;
                    case FrameBuffer::TOPRIGHT:
                        flop(outfb);
                        flip(outfb);
                        break;
                    case FrameBuffer::BOTTOMRIGHT:
                        flop(outfb);
                        break;
                    default:
                        break;
                    }
                    break;
                case FrameBuffer::TOPLEFT:
                    switch (outfb->orientation())
                    {
                    case FrameBuffer::NATURAL:
                        flip(outfb);
                        break;
                    case FrameBuffer::TOPRIGHT:
                        flop(outfb);
                        break;
                    case FrameBuffer::BOTTOMRIGHT:
                        flip(outfb);
                        flop(outfb);
                        break;
                    default:
                        break;
                    }
                    break;
                case FrameBuffer::TOPRIGHT:
                    switch (outfb->orientation())
                    {
                    case FrameBuffer::NATURAL:
                        flip(outfb);
                        flop(outfb);
                        break;
                    case FrameBuffer::TOPLEFT:
                        flop(outfb);
                        break;
                    case FrameBuffer::BOTTOMRIGHT:
                        flip(outfb);
                        break;
                    default:
                        break;
                    }
                    break;
                case FrameBuffer::BOTTOMRIGHT:
                    switch (outfb->orientation())
                    {
                    case FrameBuffer::TOPLEFT:
                        flip(outfb);
                        flop(outfb);
                        break;
                    case FrameBuffer::TOPRIGHT:
                        flip(outfb);
                        break;
                    case FrameBuffer::BOTTOMRIGHT:
                        flop(outfb);
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }

                outfb->setOrientation(m_orientation);
                if (infb != &fb)
                    delete infb;
            }

            //
            //  Output lin->log or gamma the image to the output space
            //

            if (m_outgamma != 1.0)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                applyGamma(infb, outfb, m_outgamma);
                if (m_verbose)
                    cout << "INFO: output gamma of " << m_outgamma << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outlog)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLinearToLog(infb, outfb);
                if (m_verbose)
                    cout << "INFO: lin -> log" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outLogC)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLinearToLogC(infb, outfb, m_outLogCEI);
                if (m_verbose)
                    cout << "INFO: lin -> LogC (EI="
                         << (m_outLogCEI == 0.0f ? 800 : m_outLogCEI) << ")"
                         << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outRedLog)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLinearToRedLog(infb, outfb);
                if (m_verbose)
                    cout << "INFO: lin -> RedLog" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outRedLogFilm)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLinearToLog(infb, outfb);
                if (m_verbose)
                    cout << "INFO: lin -> RedLogFilm" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outSRGB)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLinearToSRGB(infb, outfb);
                if (m_verbose)
                    cout << "INFO: lin -> sRGB" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outRec709)
            {
                FrameBuffer* infb = outfb;

                outfb = infb->copy();
                convertLinearToRec709(infb, outfb);
                if (m_verbose)
                    cout << "INFO: lin -> Rec709" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outpremult)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                premult(infb, outfb);
                if (m_verbose)
                    cout << "INFO: applied output premult" << endl;
                if (infb != &fb)
                    delete infb;
            }

            if (m_outunpremult)
            {
                FrameBuffer* infb = outfb;
                outfb = infb->copy();
                unpremult(infb, outfb);
                if (m_verbose)
                    cout << "INFO: applied output unpremult" << endl;
                if (infb != &fb)
                    delete infb;
            }

            //
            //  If planar output then convert to planar and subsample
            //

            if (m_ysamples)
            {
                FrameBuffer* infb = outfb;

                if (outfb->numChannels() == 1)
                {
                    //
                    //  Make a B+W RGB image
                    //

                    infb->setChannelName(0, "R");

                    vector<string> chmap;
                    chmap.push_back("R");
                    chmap.push_back("G");
                    chmap.push_back("B");

                    outfb = channelMap(infb, chmap);
                    if (infb != &fb)
                        delete infb;

                    Mat44f M(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);

                    applyTransform(outfb, outfb, linearColorTransform, &M);
                }

                if (m_usamples)
                {
                }
                else if (m_rysamples)
                {
                    if (m_verbose)
                    {
                        cout << "INFO: converting to Y RY BY";
                        if (outfb->numChannels() == 4 && m_asamples)
                            cout << " A";
                        cout << endl;
                    }

                    convertRGBtoYRYBY(outfb, outfb);
                }
                else
                {
                    //
                    //  Should not get here
                    //

                    abort();
                }

                //
                //  Split into planes
                //

                if (m_verbose)
                    cout << "INFO: splitting planes" << endl;
                infb = outfb;
                FrameBufferVector vfb = split(infb);
                if (infb != &fb)
                    delete infb;

                int samples[4];
                samples[0] = m_ysamples;
                samples[1] = m_usamples ? m_usamples : m_rysamples;
                samples[2] = m_vsamples ? m_vsamples : m_bysamples;
                samples[3] = m_asamples;

                for (int i = 0; i < vfb.size(); i++)
                {
                    FrameBuffer* fb = vfb[i];
                    FrameBuffer* nfb = fb;

                    if (samples[i] > 1)
                    {
                        if (m_verbose)
                            cout << "INFO: resampling plane "
                                 << fb->channelName(0) << " factor "
                                 << samples[i] << endl;

                        nfb = new FrameBuffer(
                            fb->coordinateType(), fb->width() / samples[i],
                            fb->height() / samples[i], 1, 1, outfb->dataType(),
                            0, &fb->channelNames(), fb->orientation(), true);

                        resize(fb, nfb);

                        delete vfb[i];
                    }

                    if (samples[i] != 0)
                    {
                        if (i)
                            outfb->appendPlane(nfb);
                        else
                            outfb = nfb;
                    }
                    else
                        delete vfb[i];
                }
            }

            if (m_outtype != FrameBuffer::__NUM_TYPES__)
            {
                FrameBuffer* infb = outfb;
                outfb = copyConvert(infb, m_outtype);

                int outbits = 0;
                bool fp = m_outtype >= FrameBuffer::HALF;

                switch (m_outtype)
                {
                case FrameBuffer::UCHAR:
                    outbits = 8;
                    break;
                case FrameBuffer::HALF:
                case FrameBuffer::USHORT:
                    outbits = 16;
                    break;
                case FrameBuffer::FLOAT:
                    outbits = 32;
                    break;
                case FrameBuffer::DOUBLE:
                    outbits = 64;
                    break;
                default:
                    break;
                }

                if (m_verbose)
                    cout << "INFO: requested output " << outbits << " bit "
                         << (fp ? "float" : "int") << endl;

                if (infb != &fb)
                    delete infb;
            }

            if (outfb != &fb)
            {
                if (fb.isPlanar())
                    fb.deleteAllPlanes();

                fb.restructure(outfb->width(), outfb->height(), outfb->depth(),
                               outfb->numChannels(), outfb->dataType(), 0,
                               &outfb->channelNames(), outfb->orientation(),
                               true);

                if (outfb->isPlanar())
                {
                    for (FrameBuffer* plane = outfb->nextPlane(); plane;
                         plane = plane->nextPlane())
                    {
                        fb.appendPlane(new FrameBuffer(
                            plane->coordinateType(), plane->width(),
                            plane->height(), plane->depth(),
                            plane->numChannels(), plane->dataType(), 0,
                            &plane->channelNames(), plane->orientation(),
                            true));
                    }
                }

                attrHolder.copyAttributesTo(outfb);

                copy(outfb, &fb);
#if 0 //  ALAN_UNCROP

            If uncrop data was properly set on these frame buffers,
            pass it on here.

            fb.setUncrop (outfb);
#endif

                delete outfb;
            }

            fb.setIdentifier(fb.identifier() + idstr.str());
        }
    }

    void ReformattingMovie::identifiersAtFrame(const ReadRequest& request,
                                               IdentifierVector& ids)
    {
        int frame = request.frame;

        if (m_startFrame != numeric_limits<int>::min())
        {
            if (frame < m_startFrame)
                frame = m_startFrame;
            if (frame > m_endFrame)
                frame = m_endFrame;
        }

        m_movie->identifiersAtFrame(request, ids);
        ostringstream idstream;
        identifier(idstream);

        for (unsigned int i = 0; i < ids.size(); i++)
        {
            ids[i] += idstream.str();
        }
    }

    void ReformattingMovie::identifier(ostream& idstream)
    {
        idstream << ":reformat"; // catch all

        if (m_useFloat)
            idstream << ":f";
        if (m_channelMap.size())
        {
            idstream << ":chmap2_";
            for (int i = 0; i < m_channelMap.size(); i++)
                idstream << m_channelMap[i];
        }

        if (m_inlog)
            idstream << ":loglin";
        if (m_inlogc)
            idstream << ":loglinc";
        if (m_inRedLog)
            idstream << ":redloglin";
        if (m_inRedLogFilm)
            idstream << ":redlogfilmlin";
        if (m_ingamma != 1.0)
            idstream << ":ig" << m_ingamma;
        if (m_scale != 1.0)
            idstream << ":scl" << m_scale;
        if (m_xsize)
            idstream << ":w" << m_xsize << ":h" << m_ysize;
        if (m_exposure != 0.0)
            idstream << ":e" << m_exposure;
        if (m_flip)
            idstream << ":fx";
        if (m_flop)
            idstream << ":fy";
        if (m_outgamma != 1.0)
            idstream << ":og" << m_outgamma;
        if (m_outlog)
            idstream << ":linlog";
        if (m_outLogC)
            idstream << ":linlogc:ei" << m_outLogCEI;
        if (m_outRedLog)
            idstream << ":linredlog";
        if (m_outRedLogFilm)
            idstream << ":linredlogfilm";
        if (m_ysamples)
            idstream << ":s"
                     << ":" << m_ysamples << ":" << m_usamples << ":"
                     << m_vsamples << ":" << m_rysamples << ":" << m_bysamples
                     << ":" << m_asamples;

        idstream << ":b" << m_outtype;
    }

} // namespace TwkMovie
