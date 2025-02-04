//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MovieProcedural/MovieProcedural.h>
#include <TwkFB/IO.h>
#include <TwkMath/Function.h>
#include <TwkMath/Noise.h>
#include <TwkMovie/Exception.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMath/Color.h>
#include <TwkUtil/File.h>
#include <TwkUtil/Base64.h>
#include <algorithm>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <string>

namespace TwkMovie
{

    using namespace std;
    using namespace TwkFB;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace TwkMovie;
    using namespace TwkAudio;
    using namespace boost;

    MovieProcedural::MovieProcedural()
        : MovieReader()
        , m_red(0.0f)
        , m_green(0.0f)
        , m_blue(0.0f)
        , m_alpha(1.0f)
        , m_audioFreq(1000.0f)
        , m_audioAmp(1.0f)
        , m_hpan(0)
        , m_flash(false)
        , m_interval(1.0f)
    {
        m_threadSafe = true;
    }

    MovieProcedural::~MovieProcedural() {}

    void MovieProcedural::open(const string& filename, const MovieInfo& info,
                               const Movie::ReadRequest& request)
    {
        m_filename = filename;
        string description = basename(filename.substr(0, filename.size() - 10));

        vector<string> tokens;
        algorithm::split(tokens, description, is_any_of(string(",")),
                         token_compress_on);

        m_imageType = tokens[0];
        m_info = info;
        m_info.start = 1;
        m_info.end = 1;
        m_info.inc = 1;
        m_info.fps = 24;
        m_info.width = 720;
        m_info.height = 486;
        m_info.uncropWidth = 720;
        m_info.uncropHeight = 486;
        m_info.uncropX = 0;
        m_info.uncropY = 0;
        m_info.audio = false;
        m_info.audioSampleRate = 48000.0;
        m_info.audioChannels = TwkAudio::layoutChannels(TwkAudio::Stereo_2);
        m_info.pixelAspect = 1;
        m_info.video = true;
        m_info.numChannels = 4;
        m_info.dataType = FrameBuffer::UCHAR;
        m_info.orientation = FrameBuffer::NATURAL;

        FrameBuffer::DataType dataType = FrameBuffer::UCHAR;

        FrameBuffer::AttributeVector attrs;

        regex hdSpecRE("(\\d{3,4})p(\\d+(\\.\\d*)?)?");
        smatch m;

        int numAudioChans = m_info.audioChannels.size();
        for (int i = 1; i < tokens.size(); i++)
        {
            vector<string> statement;
            string::size_type p = tokens[i].find('=');

            if (p != string::npos)
            {
                statement.push_back(tokens[i].substr(0, p));
                statement.push_back(tokens[i].substr(p + 1));
            }
            else
            {
                statement.push_back(tokens[i]);
            }

            if (statement.size() == 0)
                continue;

            if (statement.size() > 2)
            {
                cerr << "ERROR: bad movieproc specification: " << statement[0]
                     << endl;
                continue;
            }

            const string& name = statement[0];
            const char* val = statement.size() == 1 ? "" : statement[1].c_str();

            bool addAttr = true;

            if (name == "start")
                m_info.start = atoi(val);
            else if (name == "end")
                m_info.end = atoi(val);
            else if (name == "fps")
                m_info.fps = atof(val);
            else if (name == "inc")
                m_info.inc = atoi(val);
            else if (name == "red")
                m_red = atof(val);
            else if (name == "gray")
                m_red = atof(val), m_green = m_red, m_blue = m_red;
            else if (name == "grey")
                m_red = atof(val), m_green = m_red, m_blue = m_red;
            else if (name == "green")
                m_green = atof(val);
            else if (name == "blue")
                m_blue = atof(val);
            else if (name == "alpha")
                m_alpha = atof(val);
            else if (name == "width")
                m_info.uncropWidth = m_info.width = atoi(val);
            else if (name == "height")
                m_info.uncropHeight = m_info.height = atoi(val);
            else if (name == "hpan")
                m_hpan = atoi(val);
            else if (name == "flash")
                m_flash = true;
            else if (name == "interval")
                m_interval = atof(val);
            else if (name == "freq")
                m_audioFreq = atof(val);
            else if (name == "amp")
                m_audioAmp = atof(val);
            else if (name == "rate")
                m_info.audioSampleRate = atof(val);
            else if (name == "audioChannels")
                numAudioChans = atoi(val);
            else if (name == "errorString")
            {
                vector<char> errorStrings;
                TwkUtil::id64Decode(val, errorStrings);
                errorStrings.push_back(0);
                m_errorMessage = &errorStrings[0];
                addAttr = false;
            }
            else if (name == "filename")
            {
                vector<char> filename;
                TwkUtil::id64Decode(val, filename);
                filename.push_back(0);
                m_filename = &filename[0];
                addAttr = false;
            }
            else if (statement.size() == 2 && name.size() > 4
                     && name.substr(0, 5) == "attr:")
            {
                attrs.push_back(new StringAttribute(name.substr(5), val));
                addAttr = false;
            }
            else if (name == "audio")
            {
                m_audioType = statement[1];
                m_info.audio = true;
            }
            else if (name == "depth")
            {
                char back = statement[1][statement[1].size() - 1];

                switch (atoi(val))
                {
                default:
                case 8:
                    dataType = FrameBuffer::UCHAR;
                    break;

                case 16:
                    dataType =
                        back == 'f' ? FrameBuffer::HALF : FrameBuffer::USHORT;
                    break;

                case 32:
                    dataType =
                        back == 'f' ? FrameBuffer::FLOAT : FrameBuffer::UINT;
                    break;
                }
            }
            else if (name == "minBeforeTime")
            {
                m_errorStartTime = TwkUtil::SystemClock().now() + atof(val);
            }
            else if (regex_match(name, m, hdSpecRE))
            {
                //
                //  E.g. 720p60 or just 720p
                //

                if (m[1] == "480")
                {
                    m_info.uncropWidth = m_info.width = 720;
                    m_info.uncropHeight = m_info.height = 480;
                }
                else if (m[1] == "720")
                {
                    m_info.uncropWidth = m_info.width = 1280;
                    m_info.uncropHeight = m_info.height = 720;
                }
                else if (m[1] == "1080")
                {
                    m_info.uncropWidth = m_info.width = 1920;
                    m_info.uncropHeight = m_info.height = 1080;
                }

                ostringstream width;
                width << m_info.width;
                ostringstream height;
                height << m_info.height;
                attrs.push_back(
                    new StringAttribute("MovieProc/width", width.str()));
                attrs.push_back(
                    new StringAttribute("MovieProc/height", height.str()));

                if (m[2] != "")
                {
                    string val = m[2];
                    m_info.fps = atof(val.c_str());

                    ostringstream fps;
                    fps << val;
                    attrs.push_back(
                        new StringAttribute("MovieProc/fps", fps.str()));
                }
            }

            if (statement.size() == 2 && addAttr)
            {
                attrs.push_back(
                    new StringAttribute("MovieProc/" + name, statement[1]));
            }
        }

        attrs.push_back(new StringAttribute("ImageType", m_imageType));

        if (m_imageType == "syncflash")
        {
            m_audioType = "sync";
            m_flash = true;
            m_info.audio = true;
        }
        else if (m_imageType == "noisepan")
        {
            m_imageType = "noise";
            m_hpan = 4;
        }
        else if (m_imageType == "blank")
        {
            m_info.width = 1;
            m_info.height = 1;
            m_img.setUncrop(m_info.uncropWidth, m_info.uncropHeight, 0, 0);
            m_img.attribute<string>("Type") = "Blank";
        }

        if (m_flash && m_red == 0.0 && m_green == 0.0 && m_blue == 0.0
            && m_alpha == 1.0)
        {
            //
            //  Make flash white by default; This helps testing with sync
            //  meters.
            //

            m_red = 1.0;
            m_green = 1.0;
            m_blue = 1.0;
        }

        m_img.restructure(m_info.width, m_info.height, 1, 4, dataType);
        if (m_flash)
            m_flashImg.restructure(m_info.width, m_info.height, 1, 4, dataType);

        for (int i = 0; i < attrs.size(); i++)
        {
            m_img.addAttribute(attrs[i]);
        }

        if (m_audioType != "")
        {
            m_img.attribute<string>("MovieProc/audio") = m_audioType;
            m_img.attribute<float>("MovieProc/freq") = m_audioFreq;
            m_img.attribute<float>("MovieProc/rate") = m_info.audioSampleRate;
            m_img.attribute<float>("MovieProc/amp") = m_audioAmp;

            if (m_flash)
            {
                m_img.attribute<string>("MovieProc/flash") = "NO";
                m_flashImg.attribute<string>("MovieProc/audio") = m_audioType;
                m_flashImg.attribute<float>("MovieProc/freq") = m_audioFreq;
                m_flashImg.attribute<float>("MovieProc/rate") =
                    m_info.audioSampleRate;
                m_flashImg.attribute<float>("MovieProc/amp") = m_audioAmp;
                m_flashImg.attribute<string>("MovieProc/flash") = "YES";
            }
        }

        if (m_info.audio)
        {
            if (numAudioChans < 1)
                numAudioChans = 1;
            if (numAudioChans > 8)
                numAudioChans = 8;
            m_info.audioChannels =
                layoutChannels(channelLayouts(numAudioChans).front());
        }

        if (m_flash)
        {
            renderSolid(m_flashImg, m_red, m_green, m_blue, m_alpha);
        }

        renderImage(m_imageType, m_img);

        m_img.attribute<float>("FPS") = m_info.fps;
        if (m_flash)
            m_flashImg.attribute<float>("FPS") = m_info.fps;

        int frameCount = m_info.end - m_info.start + 1;
        ostringstream attr;
        attr.str("");
        attr << frameCount << " frames, " << (double(frameCount) / m_info.fps)
             << " sec";
        m_img.attribute<string>("Duration") = attr.str();
        if (m_flash)
            m_flashImg.attribute<string>("Duration") = attr.str();

        m_img.copyAttributesTo(&m_info.proxy);
    }

    void MovieProcedural::renderImage(const string& type, FrameBuffer& fb)
    {
        if (type == "solid" || type == "error")
        {
            renderSolid(fb, m_red, m_green, m_blue, m_alpha);
        }
        else if (type == "black")
        {
            renderSolid(fb, 0, 0, 0, 1.0);
        }
        else if (type == "grey" || type == "gray")
        {
            renderSolid(fb, 0.5, 0.5, 0.5, 1.0);
        }
        else if (type == "white")
        {
            renderSolid(fb, 1.0, 1.0, 1.0, 1.0);
        }
        else if (type == "smptebars")
        {
            renderSMPTEBars(fb);
        }
        else if (type == "colorchart" || type == "srgbcolorchart")
        {
            renderSRGBMacbethColorChart(fb);
        }
        else if (type == "acescolorchart")
        {
            renderACESMacbethColorChart(fb);
        }
        else if (type == "hbramp" || type == "hramp")
        {
            renderHRamp(fb, 0, 0, 0, 0, m_red, m_green, m_blue, m_alpha);
        }
        else if (type == "hwramp")
        {
            renderHRamp(fb, 1, 1, 1, 1, m_red, m_green, m_blue, m_alpha);
        }
        else if (type == "blank")
        {
            renderSolid(fb, 0, 0, 0, 0.0);
        }
        else if (type == "noise")
        {
            renderNoise(fb);
        }
        else
        {
            renderSolid(fb, 0, 0, 0, 1);
        }
    }

    void MovieProcedural::renderNoise(FrameBuffer& fb)
    {
        renderSolid(fb, 0, 0, 0, 0);

        const int maxd = std::min(fb.width(), fb.height());
        const float steps = float(log(double(maxd)) / log(2.0));
        const float wavelength = maxd;

        vector<float> lut(steps + 1);

        for (float s = 0.0f; s <= steps; s++)
        {
            lut[size_t(s)] = wavelength / ::pow(2.0f, s);
        }

        for (int y = 0; y < fb.height(); y++)
        {
            for (int x = 0, w = fb.width(); x < w; x++)
            {
                float n = 0.0f;

                for (float s = 0.0f; s <= steps; s++)
                {
                    const float wf = lut[size_t(s)];
                    const float amp = 1.0f - s / steps;
                    const float xn0 = float(x) / wf;
                    const float yn0 = float(y) / wf;
                    const float xn1 = float(w - x - 1.0) / wf;
                    const float n0 = noise(Vec2f(xn0, yn0)) * amp;
                    const float n1 = noise(Vec2f(xn1, yn0)) * amp; // mirrored
                    const float t = pow(x / (w - 1.0), 8.0);

                    n += n0 * t + n1 * (1.0 - t); // fade between n0 and n1
                }

                n = (n + 1.0) / 2.0f;
                fb.setPixel4f(n, n, n, 1.0f, x, y);
            }
        }
    }

    void MovieProcedural::renderHRamp(FrameBuffer& fb, float r0, float g0,
                                      float b0, float a0, float r1, float g1,
                                      float b1, float a1)
    {
        const int w = fb.width();
        const int h = fb.height();

        for (int x = 0; x < w; x++)
        {
            const float t = float(double(x) / double(w - 1));

            fb.setPixel4f(lerp(r0, r1, t), lerp(g0, g1, t), lerp(b0, b1, t),
                          lerp(a0, a1, t), x, 0);
        }

        for (int y = 1; y < h; y++)
        {
            memcpy(fb.scanline<char>(y), fb.scanline<char>(0),
                   fb.scanlineSize());
        }
    }

    void MovieProcedural::renderSolid(FrameBuffer& fb, float r, float g,
                                      float b, float a)
    {
        for (int x = 0; x < fb.width(); x++)
        {
            fb.setPixel4f(r, g, b, a, x, 0);
        }

        for (int y = 1; y < fb.height(); y++)
        {
            memcpy(fb.scanline<unsigned char>(y), fb.scanline<unsigned char>(0),
                   fb.scanlineSize());
        }
    }

    void MovieProcedural::renderSMPTEBars(FrameBuffer& fb)
    {
        static Col3f mainBars[] = {Col3f(0.8),       Col3f(.8, .8, 0),
                                   Col3f(0, .8, .8), Col3f(0, .8, 0),
                                   Col3f(.8, 0, .8), Col3f(.8, 0, 0),
                                   Col3f(0, 0, .8)};

        static Col3f smallbars[] = {
            Col3f(0, 0, .8),  Col3f(.075), Col3f(.8, 0, .8), Col3f(.075),
            Col3f(0, .8, .8), Col3f(.075), Col3f(.8)};

        static Col3f botbars[] = {Col3f(.031373, 0.243137, 0.349020),
                                  Col3f(1.0),
                                  Col3f(0.227451, 0, 0.494118),
                                  Col3f(0.075),
                                  Col3f(0.0),
                                  Col3f(0.075),
                                  Col3f(0.15),
                                  Col3f(0.075)};

        static double botstops[] = {0.0, 111 / 640.0, 224 / 640.0, 336 / 640.0,
                                    // 460 / 639.0,
                                    (640 / 7 * 5) / 640.0, 488 / 640.0,
                                    520 / 640.0, (640 / 7 * 6) / 640.0};

        int modseven = fb.width() / 7;
        int lastline = fb.height() - 1;
        int midline = int(fb.height() * .32);
        int lowline = midline - int(fb.height() * 0.08);

        for (int x = 0; x < fb.width(); x++)
        {
            int cindex = x / modseven;
            if (cindex > 6)
                cindex = 6;
            Col3f c = mainBars[cindex];
            Col3f s = smallbars[cindex];
            fb.setPixel4f(c.x, c.y, c.z, 1.0, x, lastline);
            fb.setPixel4f(s.x, s.y, s.z, 1.0, x, midline);

            float pc = double(x) / double(fb.width());
            int bindex = 0;

            for (int i = 0; i < 8; i++)
            {
                if (pc >= botstops[i])
                    bindex = i;
                else
                    break;
            }

            Col3f b = botbars[bindex];
            fb.setPixel4f(b.x, b.y, b.z, 1.0, x, lowline);
        }

        unsigned char* base = fb.scanline<unsigned char>(lastline);
        unsigned char* mid = fb.scanline<unsigned char>(midline);
        unsigned char* low = fb.scanline<unsigned char>(lowline);
        size_t ss = fb.scanlineSize();

        for (int y = lastline - 1; y > midline; y--)
        {
            memcpy(fb.scanline<unsigned char>(y), base, ss);
        }

        for (int y = midline; y > lowline; y--)
        {
            memcpy(fb.scanline<unsigned char>(y), mid, ss);
        }

        for (int y = lowline; y >= 0; y--)
        {
            memcpy(fb.scanline<unsigned char>(y), low, ss);
        }

        fb.setPrimaryColorSpace(ColorSpace::SMPTE_C());
        fb.setPrimaries(Chromaticities<float>::SMPTE_C());
    }

    //
    //  Renders a Macbeth 6x4 color chart.
    //  Colors are in sRGB D65 colorspace.
    //  This colorchart is less colormetrically accurate
    //  than the ACES one and should be used in the
    //  context for display referred workflows or digital
    //  production work.
    //
    //
    void MovieProcedural::renderSRGBMacbethColorChart(FrameBuffer& fb)
    {
        // 24 color patches listed from the bottom to
        // top row of a Macbeth chart.
        static Col3f colorPatch[24] = {
            Col3f(243.0f, 243.0f, 243.0f), // White
            Col3f(200.0f, 200.0f, 200.0f), // Neutral 8
            Col3f(160.0f, 160.0f, 160.0f), // Neutral 6.5
            Col3f(122.0f, 122.0f, 122.0f), // Neutral 5
            Col3f(85.0f, 85.0f, 85.0f),    // Neutral 3.5
            Col3f(52.0f, 52.0f, 52.0f),    // Black

            Col3f(56.0f, 61.0f, 150.0f),  // Blue
            Col3f(70.0f, 148.0f, 73.0f),  // Green
            Col3f(175.0f, 54.0f, 60.0f),  // Red
            Col3f(231.0f, 199.0f, 31.0f), // Yellow
            Col3f(187.0f, 86.0f, 149.0f), // Magenta
            Col3f(8.0f, 133.0f, 161.0f),  // Cyan

            Col3f(214.0f, 126.0f, 44.0f), // Orange
            Col3f(80.0f, 91.0f, 166.0f),  // Purplish Blue
            Col3f(193.0f, 90.0f, 99.0f),  // Moderate Red
            Col3f(94.0f, 60.0f, 108.0f),  // Purple
            Col3f(157.0f, 188.0f, 64.0f), // Yellow Green
            Col3f(224.0f, 163.0f, 46.0f), // Orange Yellow

            Col3f(115.0f, 82.0f, 68.0f),   // Dark Skin
            Col3f(194.0f, 150.0f, 130.0f), // Light Skin
            Col3f(98.0f, 122.0f, 157.0f),  // Blue Sky
            Col3f(87.0f, 108.0f, 67.0f),   // Foliage
            Col3f(133.0f, 128.0f, 177.0f), // Blue Flower
            Col3f(103.0f, 189.0f, 170.0f)  // Bluish Green
        };

        renderSolid(fb, 0.0f, 0.0f, 0.0f, 1.0f);

        // These heuristic dimensions match that
        // of the physical Macbeth color chart.
        // Assumes a screen pixel is square.
        const float pixelWidth = fb.width() / 287.0f;
        const int patchWidth = (int)floorf(40.0f * pixelWidth);
        const int leftBorderWidth = (int)floorf(6.0f * pixelWidth);
        const int gridWidth = (int)floorf(7.0f * pixelWidth);

        const float pixelHeight = fb.height() / 203.0f;
        const int patchHeight = (int)floorf(40.0f * pixelHeight);
        const int topBorderHeight = (int)floorf(15.0f * pixelHeight);
        const int gridHeight = (int)floorf(7.0f * pixelHeight);

        int patchNum = 0;
        int yoffset = topBorderHeight;

        for (int row = 0; row < 4; ++row)
        {
            int xoffset = leftBorderWidth;
            for (int col = 0; col < 6; ++col)
            {
                //
                // Paint color patch
                //
                Col3f c = colorPatch[patchNum++] / 255.0f;
                for (int y = 0; y < patchHeight; ++y)
                {
                    int ypos = y + yoffset;
                    for (int x = 0; x < patchWidth; ++x)
                    {
                        fb.setPixel4f(c.x, c.y, c.z, 1.0, x + xoffset, ypos);
                    }
                }
                xoffset += patchWidth + gridWidth;
            }

            yoffset += patchHeight + gridHeight;
        }

        fb.setTransferFunction(ColorSpace::sRGB());
        fb.setPrimaryColorSpace(ColorSpace::sRGB());
        fb.setPrimaries(Chromaticities<float>::sRGB());
    }

    //
    //  Renders a Macbeth 6x4 color chart.
    //  Colors are in ACES colorspace.
    //  This colorchart is more colormetrically accurate
    //  than the sRGB one and should be used in the
    //  context for scene referred workflows or vfx work.
    //
    void MovieProcedural::renderACESMacbethColorChart(FrameBuffer& fb)
    {
        // 24 color patches listed from the bottom to
        // top row of a Macbeth chart.
        // Note these values are taken from ACES v1.0
        // synthetic color chart exr file; the 24 color
        // patches are on the left side of that image from bottom up.
        static Col3f colorPatch[24] = {
            Col3f(8.666992e-01, 8.676758e-01, 8.583984e-01), // White
            Col3f(5.737305e-01, 5.727539e-01, 5.717773e-01), // Neutral 8
            Col3f(3.535156e-01, 3.532715e-01, 3.540039e-01), // Neutral 6.5
            Col3f(2.025146e-01, 2.023926e-01, 2.028809e-01), // Neutral 5
            Col3f(9.466553e-02, 9.521484e-02, 9.637451e-02), // Neutral 3.5
            Col3f(3.744507e-02, 3.765869e-02, 3.894043e-02), // Black

            Col3f(8.734131e-02, 7.440186e-02, 2.727051e-01), // Blue
            Col3f(1.536865e-01, 2.568359e-01, 9.069824e-02), // Green
            Col3f(2.174072e-01, 7.067871e-02, 5.130005e-02), // Red
            Col3f(5.893555e-01, 5.395508e-01, 9.155273e-02), // Yellow
            Col3f(3.090820e-01, 1.481934e-01, 2.741699e-01), // Magenta
            Col3f(1.490479e-01, 2.337646e-01, 3.593750e-01), // Cyan

            Col3f(3.859863e-01, 2.274170e-01, 5.776978e-02), // Orange
            Col3f(1.381836e-01, 1.303711e-01, 3.369141e-01), // Purplish Blue
            Col3f(3.020020e-01, 1.375732e-01, 1.275635e-01), // Moderate Red
            Col3f(9.307861e-02, 6.347656e-02, 1.352539e-01), // Purple
            Col3f(3.488770e-01, 4.365234e-01, 1.061401e-01), // Yellow Green
            Col3f(4.865723e-01, 3.669434e-01, 8.062744e-02), // Orange Yellow

            Col3f(1.187744e-01, 8.709717e-02, 5.895996e-02), // Dark Skin
            Col3f(4.001465e-01, 3.190918e-01, 2.373047e-01), // Light Skin
            Col3f(1.848145e-01, 2.039795e-01, 3.129883e-01), // Blue Sky
            Col3f(1.090088e-01, 1.351318e-01, 6.494141e-02), // Foliage
            Col3f(2.668457e-01, 2.460938e-01, 4.094238e-01), // Blue Flower
            Col3f(3.227539e-01, 4.621582e-01, 4.060059e-01)  // Bluish Green
        };

        renderSolid(fb, 0.0f, 0.0f, 0.0f, 1.0f);

        // These heuristic dimensions match that
        // of the physical Macbeth color chart.
        // Assumes a screen pixel is square.
        const float pixelWidth = fb.width() / 287.0f;
        const int patchWidth = (int)floorf(40.0f * pixelWidth);
        const int leftBorderWidth = (int)floorf(6.0f * pixelWidth);
        const int gridWidth = (int)floorf(7.0f * pixelWidth);

        const float pixelHeight = fb.height() / 203.0f;
        const int patchHeight = (int)floorf(40.0f * pixelHeight);
        const int topBorderHeight = (int)floorf(15.0f * pixelHeight);
        const int gridHeight = (int)floorf(7.0f * pixelHeight);

        int patchNum = 0;
        int yoffset = topBorderHeight;

        for (int row = 0; row < 4; ++row)
        {
            int xoffset = leftBorderWidth;
            for (int col = 0; col < 6; ++col)
            {
                //
                // Paint color patch
                //
                Col3f c = colorPatch[patchNum++];
                for (int y = 0; y < patchHeight; ++y)
                {
                    int ypos = y + yoffset;
                    for (int x = 0; x < patchWidth; ++x)
                    {
                        fb.setPixel4f(c.x, c.y, c.z, 1.0, x + xoffset, ypos);
                    }
                }
                xoffset += patchWidth + gridWidth;
            }

            yoffset += patchHeight + gridHeight;
        }

        fb.setTransferFunction(ColorSpace::Linear());
        fb.setPrimaryColorSpace(ColorSpace::ACES());
        fb.setPrimaries(Chromaticities<float>::ACES());
    }

    void MovieProcedural::imagesAtFrame(const ReadRequest& request,
                                        FrameBufferVector& fbs)
    {
        int frame = request.frame;
        fbs.resize(1);
        if (!fbs.front())
            fbs.front() = new FrameBuffer();
        FrameBuffer& fb = *fbs.front();

        const double adjustedFPS = double(m_info.fps * m_interval);
        const double t = double(frame - m_info.start) / double(adjustedFPS);
        const bool flash =
            (m_info.start == frame
                 ? false
                 : (t - floor(t)) < (1.0 / double(adjustedFPS) * 0.9));
        FrameBuffer& srcFB = (m_flash && flash) ? m_flashImg : m_img;

        if (m_hpan)
        {
            //
            //  Copy the pixels from the pre-rendered frame buffers into
            //  the output frame buffer with per-scanline offset
            //

            fb.restructure(srcFB.width(), srcFB.height(), srcFB.depth(),
                           srcFB.numChannels(), srcFB.dataType(), NULL,
                           &srcFB.channelNames(), srcFB.orientation());

            const bool negative = m_hpan < 0;
            const size_t scanlineSize = srcFB.scanlineSize();
            const size_t pixelSize = srcFB.pixelSize();
            const size_t height = srcFB.height();
            const size_t offset1 =
                (pixelSize * ::abs(m_hpan) * size_t(frame - 1)) % scanlineSize;
            const size_t offset0 = scanlineSize - offset1;

            for (size_t row = 0; row < height; row++)
            {
                const Byte* src = srcFB.scanline<Byte>(row);
                Byte* dst = fb.scanline<Byte>(row);

                if (negative)
                {
                    if (offset0)
                        memcpy(dst, src + offset1, offset0);
                    if (offset1)
                        memcpy(dst + offset0, src, offset1);
                }
                else
                {
                    if (offset1)
                        memcpy(dst, src + offset0, offset1);
                    if (offset0)
                        memcpy(dst + offset1, src, offset0);
                }
            }
        }
        else
        {
            //
            //  No scanline offset. Just return the pixels
            //

            fb.restructure(srcFB.width(), srcFB.height(), srcFB.depth(),
                           srcFB.numChannels(), srcFB.dataType(),
                           srcFB.pixels<Byte>(), &srcFB.channelNames(),
                           srcFB.orientation(), false);
        }

        fb.setIdentifier("");
        identifier(frame, fb.idstream());

        if (m_imageType == "error")
        {
            if (m_errorStartTime <= 0
                || (m_errorStartTime <= TwkUtil::SystemClock().now()))
            {
                TWK_THROW_STREAM(IOException, m_errorMessage);
            }
        }

        // fb.setUncrop(&srcFB);
        srcFB.copyAttributesTo(&fb);
    }

    void MovieProcedural::identifiersAtFrame(const ReadRequest& request,
                                             IdentifierVector& ids)
    {
        int frame = request.frame;
        ostringstream str;
        identifier(frame, str);
        ids.resize(1);
        ids.front() = str.str();
    }

    void MovieProcedural::identifier(int frame, ostream& o)
    {
        if (m_imageType == "blank")
            frame = 0;
        o << m_filename << ":" << frame;
    }

    bool MovieProcedural::hasAudio() const { return m_info.audio; }

    bool MovieProcedural::canConvertAudioRate() const
    {
        return m_info.audioSampleRate == 0;
    }

    size_t MovieProcedural::audioFillBuffer(const AudioReadRequest& request,
                                            AudioBuffer& buffer)
    {
        //
        //  start and num are in audio frames (not image frames). An audio
        //  frame is a left+right sample.
        //

        const double rate = m_info.audioSampleRate;
        const double tFrame = 1.0 / m_info.fps * .5;
        const double adjustedFPS = double(m_info.fps * m_interval);
        const Time duration =
            double(m_info.end - m_info.start + 1) / double(m_info.fps);
        const Time start = request.startTime;
        const Time length = request.duration;
        const SampleTime num = timeToSamples(length, rate);
        const SampleTime margin = request.margin;
        const unsigned int numChannels = m_info.audioChannels.size();

        buffer.reconfigure(num, m_info.audioChannels, rate, start, margin);

        const bool sine = (m_audioType == "sine" || m_audioType == "scale");
        const bool sync = m_audioType == "sync";

        if (sine || sync)
        {

            float* p = buffer.pointerIncludingMargin();

            for (SampleTime i = 0; i < num; i++)
            {
                double loc = (start + samplesToTime(i, rate)) / m_interval;
                bool inSync = ((start - samplesToTime(i, rate) < 1.0 / rate)
                                   ? false
                                   : (loc - floor(loc))
                                         < (1.0 / double(adjustedFPS) * 0.9));

                if (sine || inSync)
                {
                    const Time t = samplesToTime(i, rate) + start
                                   - samplesToTime(margin, rate);

                    for (int ch = 0; ch < numChannels; ch++, p++)
                    {
                        double f = m_audioFreq;
                        if (m_audioType == "scale")
                        {
                            f *= pow(2.0, float(ch) / 12.0);
                        }

                        double v = sin(t * 2.0 * M_PI * f) * m_audioAmp;

                        // Fade in
                        if (t < tFrame)
                        {
                            v *= t / tFrame;
                        }

                        // Fade out
                        if (t > (duration - tFrame))
                        {
                            double a = (duration - t) / tFrame;
                            if (a < 0.0)
                                a = 0.0;
                            v *= a;
                        }

                        *p = float(v);
                    }
                }
                else
                {
                    for (int ch = 0; ch < numChannels; ch++, p++)
                        *p = 0.0f;
                }
            }
        }

        return num;
    }

    void MovieProcedural::flush() {}

    //----------------------------------------------------------------------

    MovieProceduralIO::MovieProceduralIO()
    {
        StringPairVector video;
        StringPairVector audio;
        unsigned int capabilities = MovieIO::MovieRead | MovieIO::AttributeRead;
        addType("movieproc", "Procedurally Generated Image/Movie", capabilities,
                video, audio);
    }

    MovieProceduralIO::~MovieProceduralIO() {}

    std::string MovieProceduralIO::about() const
    {
        return "Tweak Procedural Movie";
    }

    MovieReader* MovieProceduralIO::movieReader() const
    {
        return new MovieProcedural();
    }

    MovieWriter* MovieProceduralIO::movieWriter() const { return 0; }

    void MovieProceduralIO::getMovieInfo(const std::string& filename,
                                         MovieInfo& minfo) const
    {
        if (extension(filename) != "movieproc")
        {
            TWK_THROW_STREAM(IOException, "Not a movieproc: " << filename);
        }
        MovieProcedural movieproc;
        movieproc.open(filename);
        minfo = movieproc.info();
    }

} // namespace TwkMovie
