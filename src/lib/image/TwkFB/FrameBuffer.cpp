//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <limits>
#include <TwkMath/Function.h>
#include <TwkExc/Exception.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/MemPool.h>
#include <iostream>
#ifndef PLATFORM_WINDOWS
#include <stdint.h>
#endif
#include <assert.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <algorithm>
#include <lcms2.h>
#include <stl_ext/replace_alloc.h>

#ifdef HAVE_HALF
#include <half.h>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
// visual studio doesn't have inttypes.h
// (http://forums.microsoft.com/MSDN/ShowPost.aspx?PostID=135168&SiteID=1)
#ifndef uint16_t
#define uint16_t __int16
#endif

#ifndef uint32_t
#define uint32_t __int32
#endif
#endif

#define SWAPSHORT(x)                            \
    ((uint16_t)((((uint16_t)(x) & 0xff00) >> 8) \
                | (((uint16_t)(x) & 0x00ff) << 8)))

namespace TwkFB
{
    using namespace std;
    using namespace TwkMath;

    namespace ColorSpace
    {

        //
        //  Why? Because DLLs on Windows don't do static global vars that's why.
        //

        std::string Primaries() { return "ColorSpace/Primaries"; }

        std::string TransferFunction() { return "ColorSpace/Transfer"; }

        std::string Conversion() { return "ColorSpace/Conversion"; }

        std::string ChromaPlacement() { return "ColorSpace/ChromaPlacement"; }

        std::string Range() { return "ColorSpace/Range"; }

        std::string sRGB() { return "sRGB"; }

        std::string Rec2020() { return "Rec2020"; }

        std::string Rec709() { return "Rec709"; }

        std::string Rec601() { return "Rec601"; }

        std::string AdobeRGB() { return "AdobeRGB"; }

        std::string P3() { return "P3"; }

        std::string XYZ() { return "XYZ"; }

        std::string ArriWideGamut() { return "ArriWideGamut"; }

        std::string ArriSceneReferred() { return "ArriSceneReferred"; }

        std::string SonySGamut() { return "Sony S-Gamut"; }

        std::string SMPTE_C() { return "SMPTE-C"; }

        std::string SMPTE240M() { return "SMPTE-240M"; }

        std::string CineonLog() { return "Cineon Log"; }

        std::string ArriLogC() { return "ARRI LogC"; }

        std::string ArriLogCFilm() { return "ARRI LogC Film"; }

        std::string ViperLog() { return "Viper Log"; }

        std::string RedSpace() { return "RedSpace"; }

        std::string RedColor() { return "RedColor"; }

        std::string RedColor2() { return "RedColor2"; }

        std::string RedColor3() { return "RedColor3"; }

        std::string RedColor4() { return "RedColor4"; }

        std::string DragonColor() { return "DragonColor"; }

        std::string DragonColor2() { return "DragonColor2"; }

        std::string RedWideGamut() { return "RedWideGamut"; }

        std::string RedLog() { return "Red Log"; }

        std::string RedLogFilm() { return "Red Log Film"; }

        std::string SonySLog() { return "SONY S-Log"; }

        std::string Panalog() { return "Panavision Log"; }

        std::string Linear() { return "Linear"; }

        std::string Gamma18() { return "Gamma 1.8"; }

        std::string Gamma22() { return "Gamma 2.2"; }

        std::string Gamma24() { return "Gamma 2.4"; }

        std::string Gamma26() { return "Gamma 2.6"; }

        std::string Gamma28() { return "Gamma 2.8"; }

        std::string GammaExponent() { return "Gamma Exponent"; }

        std::string ACES() { return "ACES"; }

        std::string CIEXYZ() { return "CIE XYZ"; }

        std::string Generic() { return "Generic"; }

        std::string None() { return "None"; }

        std::string NonColorData() { return "Not Color"; }

        std::string ICCProfile() { return "ICC Profile"; }

        std::string CTL() { return "CTL"; }

        std::string VideoRange() { return "Video Range"; }

        std::string FilmRange() { return "Film Range"; }

        std::string FullRange() { return "Full Range"; }

        std::string Left() { return "Left"; }

        std::string Center() { return "Center"; }

        std::string TopLeft() { return "TopLeft"; }

        std::string Top() { return "Top"; }

        std::string BottomLeft() { return "BottomLeft"; }

        std::string Bottom() { return "Bottom"; }

        std::string ConversionMatrix() { return "ColorSpace/ConversionMatrix"; }

        std::string RGBtoXYZMatrix() { return "ColorSpace/RGBtoXYZMatrix"; }

        std::string LinearScale() { return "ColorSpace/LinearScale"; }

        std::string FilmStyleMatrix() { return "ColorSpace/FilmStyleMatrix"; }

        std::string FilmStyleInverseMatrix()
        {
            return "ColorSpace/FilmStyleInverseMatrix";
        }

        std::string Gamma() { return "ColorSpace/Gamma"; }

        std::string BlackPoint() { return "ColorSpace/BlackPoint"; }

        std::string WhitePoint() { return "ColorSpace/WhitePoint"; }

        std::string BreakPoint() { return "ColorSpace/BreakPoint"; }

        std::string Rolloff() { return "ColorSpace/Rolloff"; }

        std::string LogCBlackSignal() { return "ColorSpace/LogCBlackSignal"; }

        std::string LogCEncodingOffset()
        {
            return "ColorSpace/LogCEncodingOffset";
        }

        std::string LogCEncodingGain() { return "ColorSpace/LogCEncodingGain"; }

        std::string LogCGraySignal() { return "ColorSpace/LogCGraySignal"; }

        std::string LogCBlackOffset() { return "ColorSpace/LogCBlackOffset"; }

        std::string LogCLinearSlope() { return "ColorSpace/LogCLinearSlope"; }

        std::string LogCLinearOffset() { return "ColorSpace/LogCLinearOffset"; }

        std::string LogCLinearCutPoint()
        {
            return "ColorSpace/LogCLinearCutPoint";
        }

        std::string LogCCutPoint() { return "ColorSpace/LogCCutPoint"; }

        std::string RedPrimary() { return "ColorSpace/RedPrimary"; }

        std::string GreenPrimary() { return "ColorSpace/GreenPrimary"; }

        std::string BluePrimary() { return "ColorSpace/BluePrimary"; }

        std::string WhitePrimary() { return "ColorSpace/WhitePrimary"; }

        std::string AdoptedNeutral() { return "ColorSpace/AdoptedNeutral"; }

        std::string ICCProfileDescription()
        {
            return "ColorSpace/ICC/Description";
        }

        std::string ICCProfileData() { return "ColorSpace/ICC/Data"; }

        std::string ICCProfileVersion() { return "ColorSpace/ICC/Version"; }

        std::string CTLProgramName() { return "ColorSpace/CTL/Name"; }

        std::string CTLProgram() { return "ColorSpace/CTL/Program"; }
    } // namespace ColorSpace

    TWK_CLASS_NEW_DELETE(FrameBuffer);

    FrameBuffer::FrameBuffer()
        : m_coordinateType(PixelCoordinates)
        , m_data(0)
        , m_allocSize(0)
        , m_deleteDataOnDestruction(true)
        , m_deletePointer(0)
        , m_nextPlane(0)
        , m_firstPlane(0)
        , m_previousPlane(0)
        , m_retrievalTime(0)
        , m_cacheLock(0)
        , m_cacheRef(0)
        , m_pixelAspect(1.0)
        , m_uncrop(false)
        , m_uncropX(0)
        , m_uncropY(0)
        , m_uncropWidth(0)
        , m_uncropHeight(0)
        , m_staticRef(0)
    {
        // restructure(2, 2, 1, 1, UCHAR);
        // cout << "FB DEFAULT CONSTRUCT: " << this << endl;

        restructure(0, 0, 0, 0, UCHAR);
    }

    FrameBuffer::FrameBuffer(const FrameBuffer& fb)
        : m_staticRef(0)
    {
        // cout << "FB COPY CONSTRUCT: " << this << endl;
        copyFrom(&fb);
    }

    FrameBuffer::FrameBuffer(CoordinateTypes coordinateType, int width,
                             int height, int depth, DataType dataType,
                             unsigned char* data,
                             const StringVector* channelNames)
        : m_coordinateType(coordinateType)
        , m_data(0)
        , m_allocSize(0)
        , m_deleteDataOnDestruction(true)
        , m_deletePointer(0)
        , m_nextPlane(0)
        , m_firstPlane(0)
        , m_previousPlane(0)
        , m_retrievalTime(0)
        , m_cacheLock(0)
        , m_cacheRef(0)
        , m_pixelAspect(1.0)
        , m_uncrop(false)
        , m_uncropX(0)
        , m_uncropY(0)
        , m_uncropWidth(0)
        , m_uncropHeight(0)
        , m_staticRef(0)
    {
        // cout << "FB CONSTRUCT 4: " << this << endl;
        restructure(width, height, depth, channelNames->size(), dataType, data,
                    channelNames, BOTTOMLEFT, true, /* Manage the data buffer */
                    0,                              /* No extra lines */
                    0);                             /* No extra pixels */
    }

    FrameBuffer::FrameBuffer(int width, int height, int numChannels,
                             DataType dataType, unsigned char* data,
                             const StringVector* channelNames,
                             Orientation orient, int extraScanlines,
                             int extraScanlinePixels)
        : m_coordinateType(PixelCoordinates)
        , m_data(0)
        , m_allocSize(0)
        , m_deleteDataOnDestruction(true)
        , m_deletePointer(0)
        , m_nextPlane(0)
        , m_firstPlane(0)
        , m_previousPlane(0)
        , m_retrievalTime(0)
        , m_cacheLock(0)
        , m_cacheRef(0)
        , m_pixelAspect(1.0)
        , m_uncrop(false)
        , m_uncropX(0)
        , m_uncropY(0)
        , m_uncropWidth(0)
        , m_uncropHeight(0)
        , m_staticRef(0)
    {
        // cout << "FB CONSTRUCT 3: " << this << endl;
        restructure(width, height, 1, numChannels, dataType, data, channelNames,
                    orient, (data ? false : true), extraScanlines,
                    extraScanlinePixels);
    }

    FrameBuffer::FrameBuffer(int width, int height, int numChannels,
                             DataType dataType, unsigned char* data,
                             const StringVector* channelNames,
                             Orientation orient, bool deleteOnDestruction,
                             int extraScanlines, int extraScanlinePixels)
        : m_coordinateType(PixelCoordinates)
        , m_data(0)
        , m_allocSize(0)
        , m_deleteDataOnDestruction(true)
        , m_deletePointer(0)
        , m_nextPlane(0)
        , m_firstPlane(0)
        , m_previousPlane(0)
        , m_retrievalTime(0)
        , m_cacheLock(0)
        , m_cacheRef(0)
        , m_pixelAspect(1.0)
        , m_uncrop(false)
        , m_uncropX(0)
        , m_uncropY(0)
        , m_uncropWidth(0)
        , m_uncropHeight(0)
        , m_staticRef(0)
    {
        // cout << "FB CONSTRUCT 2: " << this << endl;
        restructure(width, height, 1, numChannels, dataType, data, channelNames,
                    orient, deleteOnDestruction, extraScanlines,
                    extraScanlinePixels);
    }

    FrameBuffer::FrameBuffer(CoordinateTypes coordinateType, int width,
                             int height, int depth, int numChannels,
                             DataType dataType, unsigned char* data,
                             const StringVector* channelNames,
                             Orientation orient, bool deleteOnDestruction,
                             int extraScanlines, int extraScanlinePixels)
        : m_coordinateType(coordinateType)
        , m_data(0)
        , m_allocSize(0)
        , m_deleteDataOnDestruction(true)
        , m_deletePointer(0)
        , m_nextPlane(0)
        , m_firstPlane(0)
        , m_previousPlane(0)
        , m_retrievalTime(0)
        , m_cacheLock(0)
        , m_cacheRef(0)
        , m_pixelAspect(1.0)
        , m_uncrop(false)
        , m_uncropX(0)
        , m_uncropY(0)
        , m_uncropWidth(0)
        , m_uncropHeight(0)
        , m_staticRef(0)
    {
        // cout << "FB CONSTRUCT 1: " << this << endl;
        restructure(width, height, depth, numChannels, dataType, data,
                    channelNames, orient, deleteOnDestruction, extraScanlines,
                    extraScanlinePixels);
    }

    FrameBuffer::HashStream& FrameBuffer::idstream()
    {
        assert(isRootPlane());
#ifdef NDEBUG
        if (!isRootPlane())
        {
            cout
                << "ERROR: Cannot access idstream on non-root FrameBuffer plane"
                << endl;
        }
#endif
        return m_idstream;
    }

    std::string FrameBuffer::identifier() const
    {
        if (isRootPlane())
            return m_idstream.str();

        ostringstream id;
        id << firstPlane()->identifier();
        size_t count = 0;
        const FrameBuffer* f = this;
        while (f->previousPlane())
        {
            count++;
            f = f->previousPlane();
        }
        id << "/" << count;
        return id.str();
    }

    void FrameBuffer::setIdentifier(const std::string& s)
    {
        assert(isRootPlane());
#ifdef NDEBUG
        if (!isRootPlane())
        {
            cout << "ERROR: Cannot setIdentifier on non-root FrameBuffer plane"
                 << endl;
            return;
        }
#endif
        m_idstream.str(s);
        if (!s.empty())
            m_idstream.seekp(s.size());
    }

    void FrameBuffer::restructure(int width, int height, int depth,
                                  int numChannels, DataType dataType,
                                  unsigned char* data,
                                  const StringVector* channelNames,
                                  Orientation orient, bool deleteOnDestruction,
                                  int extraScanlines, int extraScanlinePixels,
                                  unsigned char* deletePointer,
                                  bool clearAttributes)
    {
        // cout << "RESTRUCTURE " << this << " : " << identifier() << endl;

        assert(m_data != (unsigned char*)0xdeadc0de);
        assert(dataType >= 0 && dataType < __NUM_TYPES__);

        if (height == 0)
            height = 1;
        if (depth == 0)
            depth = 1;
        else if (depth > 1)
            m_coordinateType = NormalizedCoordinates;

        m_width = width;
        m_height = height;
        m_uncrop = false;
        m_uncropWidth = width;
        m_uncropHeight = height;
        m_uncropX = 0;
        m_uncropY = 0;
        m_slopHeight = height + extraScanlines;
        m_scanlinePixelPadding = extraScanlinePixels;
        m_depth = depth;
        m_numChannels = numChannels;
        m_dataType = dataType;
        m_orientation = orient;
        m_yuv = false;
        m_yryby = false;

        if (clearAttributes)
            m_attributes.clear();

        m_channelNames.clear();

        recalcStrides();

        //
        //  NOTE: the allocated size is larger than the data size by 1
        //  scan line.
        //

        size_t oldsize = m_allocSize;
        // m_allocSize = m_dataSize + m_scanlineSize;
        m_allocSize = m_dataSize;

        if (!data && oldsize == m_allocSize && deleteOnDestruction
            && m_deleteDataOnDestruction)
        {
            // the structure change allows us to reuse the data
        }
        else
        {
            if (m_deleteDataOnDestruction)
            {
                if (m_deletePointer)
                {
                    deallocateLargeBlock(m_deletePointer);
                }
                else if (m_data)
                {
                    deallocateLargeBlock(m_data);
                }
            }

            if (m_allocSize || data)
            {
                m_data = data ? data
                              : (unsigned char*)allocateLargeBlock(m_allocSize);
                // cout << "FB: allocated (1) " << (void*)m_data << endl;
            }
            else
            {
                m_data = 0;
            }
        }

        m_deleteDataOnDestruction = deleteOnDestruction;
        m_deletePointer = deletePointer;

        if (channelNames != NULL)
        {
            int R = -2;
            int G = -2;
            int B = -2;
            int A = -1;
            int Y = -2;
            int U = -2;
            int V = -2;
            int RY = -2;
            int BY = -2;

            assert(channelNames->size() == numChannels);

            for (int i = 0; i < channelNames->size(); ++i)
            {
                const std::string& n = (*channelNames)[i];

                if (n == "R")
                    R = i;
                else if (n == "G")
                    G = i;
                else if (n == "B")
                    B = i;
                else if (n == "A")
                    A = i;
                else if (n == "Y")
                    Y = i;
                else if (n == "U")
                    U = i;
                else if (n == "V")
                    V = i;
                else if (n == "RY")
                    RY = i;
                else if (n == "BY")
                    BY = i;

                m_channelNames.push_back(n);
            }

            if (numChannels == 1)
            {
                m_colorPermute[0] = 0;
                m_colorPermute[1] = 0;
                m_colorPermute[2] = 0;
                m_colorPermute[3] = 0;
            }
            else if (R != -2 && G != -2 && B != -2 && numChannels > 2)
            {
                m_colorPermute[0] = R;
                m_colorPermute[1] = G;
                m_colorPermute[2] = B;
                m_colorPermute[3] = A;
            }
            else if (Y != -2 && U != -2 && V != -2)
            {
                m_colorPermute[0] = Y;
                m_colorPermute[1] = U;
                m_colorPermute[2] = V;
                m_colorPermute[3] = A;
                m_yuv = true;
            }
            else if (Y != -2 && RY != -2 && BY != -2)
            {
                m_colorPermute[0] = Y;
                m_colorPermute[1] = RY;
                m_colorPermute[2] = BY;
                m_colorPermute[3] = A;
                m_yryby = true;
            }
            else if (Y != -2 && numChannels <= 2)
            {
                m_colorPermute[0] = Y;
                m_colorPermute[1] = Y;
                m_colorPermute[2] = Y;
                m_colorPermute[3] = A;
            }
            else
            {
                m_colorPermute[0] = 0;
                m_colorPermute[1] = numChannels > 1 ? 1 : 0;
                m_colorPermute[2] = numChannels > 2 ? 2 : 1;
                m_colorPermute[3] = numChannels > 3 ? 3 : 2;
            }
        }
        else
        {
            if (m_dataType == PACKED_R10_G10_B10_X2)
            {
                m_channelNames.push_back("RGB10");
            }
            else if (m_dataType == PACKED_X2_B10_G10_R10)
            {
                m_channelNames.push_back("BGR10");
            }
            else if (m_dataType == PACKED_Cb8_Y8_Cr8_Y8)
            {
                m_channelNames.push_back("VYUY");
            }
            else if (m_dataType == PACKED_Y8_Cb8_Y8_Cr8)
            {
                m_channelNames.push_back("YUYV");
            }
            else
            {
                switch (numChannels)
                {
                case 1:
                    m_channelNames.push_back("Y");
                    m_colorPermute[0] = 0; // flood all
                    m_colorPermute[1] = 0;
                    m_colorPermute[2] = 0;
                    m_colorPermute[3] = -1; // alpha 1
                    break;
                case 2:
                    m_channelNames.push_back("Y");
                    m_channelNames.push_back("A");
                    m_colorPermute[0] = 0; // flood all
                    m_colorPermute[1] = 0;
                    m_colorPermute[2] = 0;
                    m_colorPermute[3] = 1;
                    break;
                case 3:
                    m_channelNames.push_back("R");
                    m_channelNames.push_back("G");
                    m_channelNames.push_back("B");
                    m_colorPermute[0] = 0;
                    m_colorPermute[1] = 1;
                    m_colorPermute[2] = 2;
                    m_colorPermute[3] = -1;
                    break;
                case 4:
                    m_channelNames.push_back("R");
                    m_channelNames.push_back("G");
                    m_channelNames.push_back("B");
                    m_channelNames.push_back("A");
                    m_colorPermute[0] = 0;
                    m_colorPermute[1] = 1;
                    m_colorPermute[2] = 2;
                    m_colorPermute[3] = 3;
                    break;
                default:
                    for (int i = 0; i < numChannels; ++i)
                    {
                        char name[17];
                        name[16] = 0;
                        snprintf(name, 16, "Channel_%d", i);
                        m_channelNames.push_back(name);
                    }
                    m_colorPermute[0] = 0;
                    m_colorPermute[1] = 1;
                    m_colorPermute[2] = 2;
                    m_colorPermute[3] = 3;
                    break;
                }
            }
        }
    }

    void FrameBuffer::restructurePlanar(int w, int h,
                                        const StringVector& planeNames,
                                        DataType type, Orientation orient)
    {
        Samplings samps(planeNames.size());
        fill(samps.begin(), samps.end(), 1);
        restructurePlanar(w, h, samps, samps, planeNames, type, orient);
    }

    void FrameBuffer::restructurePlanar(int w, int h,
                                        const Samplings& xSamplings,
                                        const Samplings& ySamplings,
                                        const StringVector& planeNames,
                                        DataType type, Orientation orient,
                                        size_t nchannels)
    {
        // assert(xSamplings.size() != planeNames.size() / nchannels - 1);
        // assert(ySamplings.size() != planeNames.size() / nchannels - 1);

        size_t nplanes = planeNames.size() / nchannels;

        vector<FrameBuffer*> planes(nplanes);
        fill(planes.begin(), planes.end(), (FrameBuffer*)0);
        int i = 0;

        for (FrameBuffer* fb = this; fb; fb = fb->nextPlane(), i++)
        {
            planes[i] = fb;
        }

        planes.front()->restructure(w / xSamplings[0], h / ySamplings[0], 0,
                                    nchannels, type, 0, 0, orient, true);

        for (size_t i = 0; i < nchannels; i++)
        {
            planes.front()->setChannelName(i, planeNames[i]);
        }

        for (int q = 1; q < nplanes; q++)
        {
            FrameBuffer* fb = 0;

            if (fb = planes[q])
            {
                fb->restructure(w / xSamplings[q], h / ySamplings[q], 0,
                                nchannels, type, 0, 0, orient, true);
            }
            else
            {
                fb = new FrameBuffer(PixelCoordinates, w / xSamplings[q],
                                     h / ySamplings[q], 0, nchannels, type, 0,
                                     0, orient, true);
                appendPlane(fb);
            }

            for (size_t i = 0; i < nchannels; i++)
            {
                fb->setChannelName(i, planeNames[q * nchannels + i]);
            }
        }
    }

    FrameBuffer::~FrameBuffer()
    {
        assert(m_nextPlane != (FrameBuffer*)0xdeadc0de);
        // cout << "FB DELETE: " << this << " : " << identifier() << endl;

        clear();
        m_previousPlane = 0;
        m_firstPlane = 0;
        if (m_nextPlane)
            delete m_nextPlane;

        // assert(findAttribute("CachedFrame") == 0);
        // assert(m_cacheLock == 0);
        // assert(m_cacheRef == 0);

        m_width = 0x99;
        m_height = 0x99;
        m_depth = 0x99;
        m_slopHeight = 0x99;
        m_numChannels = 0x99;
        m_bytesPerChannel = 0x99;
        m_dataSize = 0x99;
        m_allocSize = 0x99;
        m_pixelSize = 0x99;
        m_scanlineSize = 0x99;
        m_planeSize = 0x99;
        m_colorPermute[0] = 0x99;
        m_colorPermute[1] = 0x99;
        m_colorPermute[2] = 0x99;
        m_colorPermute[3] = 0x99;
        m_uncropHeight = 0x99;
        m_uncropWidth = 0x99;
        m_uncropX = 0x99;
        m_uncropY = 0x99;

        m_previousPlane = (FrameBuffer*)0xdeadc0de;
        m_nextPlane = (FrameBuffer*)0xdeadc0de;
        m_firstPlane = (FrameBuffer*)0xdeadc0de;
        m_data = (unsigned char*)0xdeadc0de;
    }

    size_t FrameBuffer::totalImageSize() const
    {
        size_t total = 0;
        for (const FrameBuffer* f = firstPlane(); f; f = f->nextPlane())
            total += f->allocSize();
        return total;
    }

    void FrameBuffer::clear()
    {
        if (m_deleteDataOnDestruction)
        {
            if (m_deletePointer)
            {
                // cout << "FB: deallocate m_deletePointer" << endl;
                deallocateLargeBlock(m_deletePointer);
            }
            else if (m_data)
            {
                // cout << "FB: deallocate m_data" << endl;
                deallocateLargeBlock(m_data);
            }
        }

        clearAttributes();
    }

    void FrameBuffer::setChannelName(int channel, const std::string& name)
    {
        assert(channel >= 0 && channel < m_channelNames.size());

        m_channelNames[channel] = name;
        m_yuv = hasChannel("Y") && hasChannel("U") && hasChannel("V");
        m_yryby = hasChannel("Y") && hasChannel("RY") && hasChannel("BY");
    }

    void FrameBuffer::recalcStrides()
    {
        switch (m_dataType)
        {
#ifdef HAVE_HALF
        case HALF:
            m_bytesPerChannel = sizeof(half);
            break;
#endif
        case UCHAR:
            m_bytesPerChannel = sizeof(unsigned char);
            break;
        // case USHORT_BIG:
        case USHORT:
            m_bytesPerChannel = sizeof(unsigned short);
            break;
        // case UINT_BIG:
        case UINT:
            m_bytesPerChannel = sizeof(unsigned int);
            break;
        // case FLOAT_BIG:
        case FLOAT:
            m_bytesPerChannel = sizeof(float);
            break;
        // case DOUBLE_BIG:
        case DOUBLE:
            m_bytesPerChannel = sizeof(double);
            break;
        case PACKED_R10_G10_B10_X2:
        case PACKED_X2_B10_G10_R10:
            m_bytesPerChannel = sizeof(unsigned int);
            break;
        case PACKED_Cb8_Y8_Cr8_Y8:
        case PACKED_Y8_Cb8_Y8_Cr8:
            m_bytesPerChannel = sizeof(unsigned short);
            break;
        default:
            TWK_THROW_EXC_STREAM("Unknown data type " << m_dataType);
        }

        m_pixelSize = m_numChannels * m_bytesPerChannel;
        m_scanlineSize = m_pixelSize * m_width;
        m_scanlinePaddedSize = m_pixelSize * (m_width + m_scanlinePixelPadding);
        m_planeSize = m_scanlinePaddedSize * m_slopHeight;
        m_dataSize = m_planeSize * (m_depth ? m_depth : 1);
    }

    void FrameBuffer::convertTo(DataType newDataType)
    {
        abort();

        if (newDataType == m_dataType)
        {
            return;
        }

        const int npc = m_height * m_depth * m_scanlinePaddedSize;

        switch (newDataType)
        {
        case UCHAR:
        {
            unsigned char* newData = new unsigned char[npc];

            switch (m_dataType)
            {
            case USHORT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] =
                        (unsigned char)(pixels<unsigned short>()[i] >> 8);
                }
                break;
            case UINT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned char)pixels<unsigned int>()[i];
                }
                break;
#ifdef HAVE_HALF
            case HALF:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned char)clamp(
                        pixels<half>()[i] * 255.0f, 0.0f, 255.0f);
                }
                break;
#endif
            case FLOAT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned char)clamp(
                        pixels<float>()[i] * 255.0f, 0.0f, 255.0f);
                }
                break;
            case DOUBLE:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned char)clamp(
                        pixels<double>()[i] * 255.0, 0.0, 255.0);
                }
                break;

            default:
                break;
            }

            delete[] m_data;
            m_data = newData;
            m_dataType = newDataType;
        }
        break;

        case USHORT:
        {
            unsigned short* newData = new unsigned short[npc];

            switch (m_dataType)
            {
            case UCHAR:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned short)pixels<char>()[i] << 8;
                }
                break;
            case UINT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned short)(pixels<unsigned int>()[i]);
                }
                break;
#ifdef HAVE_HALF
            case HALF:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned short)clamp(
                        pixels<half>()[i] * 65535.0f, 0.0f, 65535.0f);
                }
                break;
#endif
            case FLOAT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned short)clamp(
                        pixels<float>()[i] * 65535.0f, 0.0f, 65535.0f);
                }
                break;
            case DOUBLE:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned short)clamp(
                        pixels<double>()[i] * 65535.0, 0.0, 65535.0);
                }
                break;

            default:
                break;
            }

            delete[] m_data;
            m_data = (unsigned char*)newData;
            m_dataType = newDataType;
        }
        break;

        case UINT:
        {
            unsigned int* newData = new unsigned int[npc];

            switch (m_dataType)
            {
            case UCHAR:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned int)pixels<char>()[i];
                }
                break;
            case USHORT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned int)pixels<unsigned short>()[i];
                }
                break;
#ifdef HAVE_HALF
            case HALF:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned int)pixels<half>()[i];
                }
                break;
#endif
            case FLOAT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned int)pixels<float>()[i];
                }
                break;
            case DOUBLE:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (unsigned int)pixels<double>()[i];
                }
                break;

            default:
                break;
            }

            delete[] m_data;
            m_data = (unsigned char*)newData;
            m_dataType = newDataType;
        }
        break;

#ifdef HAVE_HALF
        case HALF:
        {
            half* newData = new half[npc];

            switch (m_dataType)
            {
            case UCHAR:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (half)pixels<unsigned char>()[i] / 256.0f;
                }
                break;
            case USHORT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] =
                        (half)(double(pixels<unsigned short>()[i]) / 65536.0);
                }
                break;
            case UINT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (half)pixels<unsigned int>()[i];
                }
                break;
            case FLOAT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (half)pixels<float>()[i];
                }
                break;
            case DOUBLE:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (half)pixels<double>()[i];
                }
                break;
            default:
                break;
            }

            delete[] m_data;
            m_data = (unsigned char*)newData;
            m_dataType = newDataType;
        }
        break;
#endif

        case FLOAT:
        {
            float* newData = new float[npc];

            switch (m_dataType)
            {
            case UCHAR:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (float)pixels<unsigned char>()[i] / 256.0f;
                }
                break;
            case USHORT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (float)pixels<unsigned short>()[i] / 65536.0f;
                }
                break;
            case UINT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (float)pixels<unsigned int>()[i];
                }
                break;
#ifdef HAVE_HALF
            case HALF:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (float)pixels<half>()[i];
                }
                break;
#endif
            case DOUBLE:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (float)pixels<double>()[i];
                }
                break;
            default:
                break;
            }

            delete[] m_data;
            m_data = (unsigned char*)newData;
            m_dataType = newDataType;
        }
        break;

        case DOUBLE:
        {
            double* newData = new double[npc];

            switch (m_dataType)
            {
            case UCHAR:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (double)pixels<unsigned char>()[i] / 256.0;
                }
                break;
            case USHORT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (double)pixels<unsigned short>()[i] / 65536.0;
                }
                break;
            case UINT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (double)pixels<unsigned int>()[i];
                }
                break;
#ifdef HAVE_HALF
            case HALF:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (double)pixels<half>()[i];
                }
                break;
#endif
            case FLOAT:
                for (int i = 0; i < npc; ++i)
                {
                    newData[i] = (double)pixels<float>()[i];
                }
                break;
            default:
                break;
            }

            delete[] m_data;
            m_data = (unsigned char*)newData;
            m_dataType = newDataType;
        }
        break;

        default:
            TWK_THROW_EXC_STREAM("Attempted conversion to unknown data stype "
                                 << newDataType);
        }

        recalcStrides();
    }

    void FrameBuffer::insertChannel(const std::string& name, int channel)
    {
        switch (m_dataType)
        {
        case UCHAR:
            insertChannelByType<unsigned char>(name, channel);
            break;
        case USHORT:
            insertChannelByType<unsigned short>(name, channel);
            break;
        case UINT:
            insertChannelByType<unsigned int>(name, channel);
            break;
#ifdef HAVE_HALF
        case HALF:
            insertChannelByType<half>(name, channel);
            break;
#endif
        case FLOAT:
            insertChannelByType<float>(name, channel);
            break;
        case DOUBLE:
            insertChannelByType<double>(name, channel);
            break;
        default:
            break;
        }
    }

    void FrameBuffer::removeChannel(int channel)
    {
        switch (m_dataType)
        {
        case UCHAR:
            removeChannelByType<unsigned char>(channel);
            break;
        // case USHORT_BIG:
        case USHORT:
            removeChannelByType<unsigned short>(channel);
            break;
        // case UINT_BIG:
        case UINT:
            removeChannelByType<unsigned int>(channel);
            break;
#ifdef HAVE_HALF
        // case HALF_BIG:
        case HALF:
            removeChannelByType<half>(channel);
            break;
#endif
        // case FLOAT_BIG:
        case FLOAT:
            removeChannelByType<float>(channel);
            break;
        // case DOUBLE_BIG:
        case DOUBLE:
            removeChannelByType<double>(channel);
            break;
        default:
            break;
        }
    }

    void FrameBuffer::removeChannel(const string& name)
    {
        removeChannel(channel(name));
    }

    bool FrameBuffer::hasChannel(const std::string& name) const
    {
        for (int i = 0; i < m_channelNames.size(); ++i)
        {
            if (m_channelNames[i] == name)
            {
                return true;
            }
        }
        return false;
    }

    bool FrameBuffer::hasAttribute(const std::string& name) const
    {
        for (int i = 0; i < m_attributes.size(); ++i)
        {
            if (m_attributes[i]->name() == name)
            {
                return true;
            }
        }
        return false;
    }

    void FrameBuffer::setPixel3f(float r, float g, float b, int x, int y)
    {
        if (x >= m_width || y >= m_height || m_width == 0 || m_height == 0)
        {
            throw NullImageException();
        }

        int ir = m_colorPermute[0];
        int ig = m_colorPermute[1];
        int ib = m_colorPermute[2];
        int ia = m_colorPermute[3];

        if (m_numChannels >= 3)
        {
            switch (m_dataType)
            {
            case FLOAT:
                pixel<float>(x, y, ir) = r;
                pixel<float>(x, y, ig) = g;
                pixel<float>(x, y, ib) = b;
                break;
            case UCHAR:
                pixel<unsigned char>(x, y, ir) =
                    (unsigned char)clamp(r * 255.0f, 0.0f, 255.0f);
                ;
                pixel<unsigned char>(x, y, ig) =
                    (unsigned char)clamp(g * 255.0f, 0.0f, 255.0f);
                ;
                pixel<unsigned char>(x, y, ib) =
                    (unsigned char)clamp(b * 255.0f, 0.0f, 255.0f);
                ;
                break;
            case USHORT:
                pixel<unsigned short>(x, y, ir) =
                    (unsigned short)clamp(r * 65535.0f, 0.0f, 65535.0f);
                pixel<unsigned short>(x, y, ig) =
                    (unsigned short)clamp(g * 65535.0f, 0.0f, 65535.0f);
                pixel<unsigned short>(x, y, ib) =
                    (unsigned short)clamp(b * 65535.0f, 0.0f, 65535.0f);
                break;
            case UINT:
                pixel<unsigned int>(x, y, ir) = (unsigned int)r;
                pixel<unsigned int>(x, y, ig) = (unsigned int)g;
                pixel<unsigned int>(x, y, ib) = (unsigned int)b;
                break;
            case HALF:
                pixel<half>(x, y, ir) = (float)r;
                pixel<half>(x, y, ig) = (float)g;
                pixel<half>(x, y, ib) = (float)b;
                break;
            case DOUBLE:
                pixel<double>(x, y, ir) = (double)r;
                pixel<double>(x, y, ig) = (double)g;
                pixel<double>(x, y, ib) = (double)b;
                break;

            case PACKED_Cb8_Y8_Cr8_Y8:
            case PACKED_Y8_Cb8_Y8_Cr8:
                break;
            default:
                break;
            }
        }
        else if (m_numChannels == 2)
        {
            switch (m_dataType)
            {
            case FLOAT:
                pixel<float>(x, y, 0) = r;
                pixel<float>(x, y, 1) = g;
                break;
            case UCHAR:
                pixel<unsigned char>(x, y, 0) =
                    (unsigned char)clamp(r * 255.0f, 0.0f, 255.0f);
                ;
                pixel<unsigned char>(x, y, 1) =
                    (unsigned char)clamp(g * 255.0f, 0.0f, 255.0f);
                ;
                break;
            case USHORT:
                pixel<unsigned short>(x, y, 0) =
                    (unsigned short)clamp(r * 65535.0f, 0.0f, 65535.0f);
                pixel<unsigned short>(x, y, 1) =
                    (unsigned short)clamp(g * 65535.0f, 0.0f, 65535.0f);
                break;
            case UINT:
                pixel<unsigned int>(x, y, 0) = (unsigned int)r;
                pixel<unsigned int>(x, y, 1) = (unsigned int)g;
                break;
            case HALF:
                pixel<half>(x, y, 0) = (float)r;
                pixel<half>(x, y, 1) = (float)g;
                break;
            case DOUBLE:
                pixel<double>(x, y, 0) = (double)r;
                pixel<double>(x, y, 1) = (double)g;
                break;

            case PACKED_Cb8_Y8_Cr8_Y8:
            case PACKED_Y8_Cb8_Y8_Cr8:
                break;
            default:
                break;
            }
        }
        else if (m_numChannels == 1)
        {
            switch (m_dataType)
            {
            case FLOAT:
                pixel<float>(x, y, 0) = r;
                break;
            case UCHAR:
                pixel<unsigned char>(x, y, 0) =
                    (unsigned char)clamp(r * 255.0f, 0.0f, 255.0f);
                ;
                break;
            case USHORT:
                pixel<unsigned short>(x, y, 0) =
                    (unsigned short)clamp(r * 65535.0f, 0.0f, 65535.0f);
                break;
            case UINT:
                pixel<unsigned int>(x, y, 0) = (unsigned int)r;
                break;
            case HALF:
                pixel<half>(x, y, 0) = (float)r;
                break;
            case DOUBLE:
                pixel<double>(x, y, 0) = (double)r;
                break;

            case PACKED_Cb8_Y8_Cr8_Y8:
            case PACKED_Y8_Cb8_Y8_Cr8:
                break;
            default:
                break;
            }
        }
    }

    void FrameBuffer::setPixel4f(float r, float g, float b, float a, int x,
                                 int y)
    {
        if (x >= m_width || y >= m_height || m_width == 0 || m_height == 0)
        {
            throw NullImageException();
        }

        int ir = m_colorPermute[0];
        int ig = m_colorPermute[1];
        int ib = m_colorPermute[2];
        int ia = m_colorPermute[3];

        switch (m_dataType)
        {
        case FLOAT:
            pixel<float>(x, y, ir) = r;
            if (m_numChannels > 1)
                pixel<float>(x, y, ig) = g;
            if (m_numChannels > 2)
                pixel<float>(x, y, ib) = b;
            if (m_numChannels > 3)
                pixel<float>(x, y, ia) = a;
            break;
        case UCHAR:
            pixel<unsigned char>(x, y, ir) =
                (unsigned char)clamp(r * 255.0f, 0.0f, 255.0f);
            ;
            if (m_numChannels > 1)
                pixel<unsigned char>(x, y, ig) =
                    (unsigned char)clamp(g * 255.0f, 0.0f, 255.0f);
            ;
            if (m_numChannels > 2)
                pixel<unsigned char>(x, y, ib) =
                    (unsigned char)clamp(b * 255.0f, 0.0f, 255.0f);
            ;
            if (m_numChannels > 3)
                pixel<unsigned char>(x, y, ia) =
                    (unsigned char)clamp(a * 255.0f, 0.0f, 255.0f);
            ;
            break;
        case USHORT:
            pixel<unsigned short>(x, y, ir) =
                (unsigned short)clamp(r * 65535.0f, 0.0f, 65535.0f);
            if (m_numChannels > 1)
                pixel<unsigned short>(x, y, ig) =
                    (unsigned short)clamp(g * 65535.0f, 0.0f, 65535.0f);
            if (m_numChannels > 2)
                pixel<unsigned short>(x, y, ib) =
                    (unsigned short)clamp(b * 65535.0f, 0.0f, 65535.0f);
            if (m_numChannels > 3)
                pixel<unsigned short>(x, y, ia) =
                    (unsigned short)clamp(a * 65535.0f, 0.0f, 65535.0f);
            break;
        case UINT:
            pixel<unsigned int>(x, y, ir) = (unsigned int)r;
            if (m_numChannels > 1)
                pixel<unsigned int>(x, y, ig) = (unsigned int)g;
            if (m_numChannels > 2)
                pixel<unsigned int>(x, y, ib) = (unsigned int)b;
            if (m_numChannels > 3)
                pixel<unsigned int>(x, y, ia) = (unsigned int)a;
            break;
#ifdef HAVE_HALF
        case HALF:
            pixel<half>(x, y, ir) = (float)r;
            if (m_numChannels > 1)
                pixel<half>(x, y, ig) = (float)g;
            if (m_numChannels > 2)
                pixel<half>(x, y, ib) = (float)b;
            if (m_numChannels > 3)
                pixel<half>(x, y, ia) = (float)a;
            break;
#endif
        case DOUBLE:
            pixel<double>(x, y, ir) = (double)r;
            if (m_numChannels > 1)
                pixel<double>(x, y, ig) = (double)g;
            if (m_numChannels > 2)
                pixel<double>(x, y, ib) = (double)b;
            if (m_numChannels > 3)
                pixel<double>(x, y, ia) = (double)a;
            break;

        case PACKED_R10_G10_B10_X2:
        {
            Pixel10 P;
            P.red = int(r * 1023.0 + 0.5);
            P.green = int(g * 1023.0 + 0.5);
            P.blue = int(b * 1023.0 + 0.5);
            P.unused = 0;
            pixel<Pixel10>(x, y, 0) = P;
            break;
        }

        case PACKED_X2_B10_G10_R10:
        {
            Pixel10Rev P;
            P.red = int(r * 1023.0 + 0.5);
            P.green = int(g * 1023.0 + 0.5);
            P.blue = int(b * 1023.0 + 0.5);
            P.unused = 0;
            pixel<Pixel10Rev>(x, y, 0) = P;
            break;
        }

        case PACKED_Cb8_Y8_Cr8_Y8:
        case PACKED_Y8_Cb8_Y8_Cr8:
            break;
        default:
            break;
        }
    }

    std::string FrameBuffer::dataTypeStr() const
    {
        switch (m_dataType)
        {
        case FLOAT:
            return "float";
        case UCHAR:
            return "unsigned char";
        // case USHORT_BIG:
        case USHORT:
            return "unsigned short";
        // case UINT_BIG:
        case UINT:
            return "unsigned int";
#ifdef HAVE_HALF
        // case HALF_BIG:
        case HALF:
            return "half";
#endif
        // case DOUBLE_BIG:
        case DOUBLE:
            return "double";
        default:
            break;
        }

        return "?unknown";
    }

    void FrameBuffer::getPixel4f(int x, int y, float* p) const
    {
        float& r = p[0];
        float& g = p[1];
        float& b = p[2];
        float& a = p[3];

        if (x >= m_width || y >= m_height || m_width == 0 || m_height == 0)
        {
            throw NullImageException();
        }

        r = g = b = a = 0.0f;

        int ir = m_colorPermute[0];
        int ig = m_colorPermute[1];
        int ib = m_colorPermute[2];
        int ia = m_colorPermute[3];

        switch (m_dataType)
        {
        case FLOAT:
            r = pixel<float>(x, y, ir);
            if (m_numChannels > 1)
                g = pixel<float>(x, y, ig);
            if (m_numChannels > 2)
                b = pixel<float>(x, y, ib);
            if (m_numChannels > 3)
                a = pixel<float>(x, y, ia);
            break;
        case UCHAR:
            r = pixel<unsigned char>(x, y, ir) / 255.0f;
            if (m_numChannels > 1)
                g = pixel<unsigned char>(x, y, ig) / 255.0f;
            if (m_numChannels > 2)
                b = pixel<unsigned char>(x, y, ib) / 255.0f;
            if (m_numChannels > 3)
                a = pixel<unsigned char>(x, y, ia) / 255.0f;
            break;
        case USHORT:
            r = pixel<unsigned short>(x, y, ir) / 65535.0f;
            if (m_numChannels > 1)
                g = pixel<unsigned short>(x, y, ig) / 65535.0f;
            if (m_numChannels > 2)
                b = pixel<unsigned short>(x, y, ib) / 65535.0f;
            if (m_numChannels > 3)
                a = pixel<unsigned short>(x, y, ia) / 65535.0f;
            break;
        case UINT:
            r = pixel<unsigned int>(x, y, ir);
            if (m_numChannels > 1)
                g = pixel<unsigned int>(x, y, ig);
            if (m_numChannels > 2)
                b = pixel<unsigned int>(x, y, ib);
            if (m_numChannels > 3)
                a = pixel<unsigned int>(x, y, ia);
            break;
#ifdef HAVE_HALF
        case HALF:
            r = pixel<half>(x, y, ir);
            if (m_numChannels > 1)
                g = pixel<half>(x, y, ig);
            if (m_numChannels > 2)
                b = pixel<half>(x, y, ib);
            if (m_numChannels > 3)
                a = pixel<half>(x, y, ia);
            break;
#endif
        case DOUBLE:
            r = pixel<double>(x, y, ir);
            if (m_numChannels > 1)
                g = pixel<double>(x, y, ig);
            if (m_numChannels > 2)
                b = pixel<double>(x, y, ib);
            if (m_numChannels > 3)
                a = pixel<double>(x, y, ia);
            break;

        case PACKED_R10_G10_B10_X2:
        {
            Pixel10 P = pixel<Pixel10>(x, y, 0);
            r = P.red / 1023.0;
            g = P.green / 1023.0;
            b = P.blue / 1023.0;
            a = 1.0;
        }
        break;

        case PACKED_X2_B10_G10_R10:
        {
            Pixel10Rev P = pixel<Pixel10Rev>(x, y, 0);
            r = P.red / 1023.0;
            g = P.green / 1023.0;
            b = P.blue / 1023.0;
            a = 1.0;
        }
        break;

        case PACKED_Cb8_Y8_Cr8_Y8:
        case PACKED_Y8_Cb8_Y8_Cr8:
        {
            int x0, x1;
            if (x & 1)
            {
                x0 = x - 1;
                x1 = x;
            }
            else
            {
                x0 = x;
                x1 = x + 1;
            }

            unsigned short P = pixel<unsigned short>(x, y, 0);
            unsigned short P1 = pixel<unsigned short>(x0, y, 0);
            unsigned short P0 = pixel<unsigned short>(x1, y, 0);

            if (m_dataType == PACKED_Y8_Cb8_Y8_Cr8)
            {
                P = SWAPSHORT(P);
                P1 = SWAPSHORT(P1);
                P0 = SWAPSHORT(P0);
            }

            b = (float(P0 >> 8) - 127.0f) / 122.0f + 0.5;
            g = (float(P1 >> 8) - 127.0f) / 122.0f + 0.5;
            r = float((P & 0xff) - 16) / 219.0;
            a = 1.0f;
        }
        break;
        default:
            break;
        }
    }

    // Pixels indices are set on the center of the pixels!
    void FrameBuffer::getPixelBilinear4f(float x, float y, float* p) const
    {
        float& r = p[0];
        float& g = p[1];
        float& b = p[2];
        float& a = p[3];

        // Clamp the right edge
        if (x > m_width - 1 && x < m_width)
            x = m_width - 1;
        if (y > m_height - 1 && y < m_height)
            y = m_height - 1;

        if (x > m_width - 1 || y > m_height - 1 || x < 0 || y < 0
            || m_width == 0 || m_height == 0)
        {
            throw NullImageException();
        }

        r = g = b = a = 0.0f;

        int ir = m_colorPermute[0];
        int ig = m_colorPermute[1];
        int ib = m_colorPermute[2];
        int ia = m_colorPermute[3];

        // Bilinear fun
        float r00, r01, r10, r11;
        float g00, g01, g10, g11;
        float b00, b01, b10, b11;
        float a00, a01, a10, a11;

        int xFloor = (int)floor(x);
        int xCeil = (int)ceil(x);
        int yFloor = (int)floor(y);
        int yCeil = (int)ceil(y);

        switch (m_dataType)
        {
        case FLOAT:
        {
            r00 = pixel<float>(xFloor, yFloor, ir);
            r01 = pixel<float>(xFloor, yCeil, ir);
            r10 = pixel<float>(xCeil, yFloor, ir);
            r11 = pixel<float>(xCeil, yCeil, ir);
        }
            if (m_numChannels > 1)
            {
                g00 = pixel<float>(xFloor, yFloor, ig);
                g01 = pixel<float>(xFloor, yCeil, ig);
                g10 = pixel<float>(xCeil, yFloor, ig);
                g11 = pixel<float>(xCeil, yCeil, ig);
            }
            if (m_numChannels > 2)
            {
                b00 = pixel<float>(xFloor, yFloor, ib);
                b01 = pixel<float>(xFloor, yCeil, ib);
                b10 = pixel<float>(xCeil, yFloor, ib);
                b11 = pixel<float>(xCeil, yCeil, ib);
            }
            if (m_numChannels > 3)
            {
                a00 = pixel<float>(xFloor, yFloor, ia);
                a01 = pixel<float>(xFloor, yCeil, ia);
                a10 = pixel<float>(xCeil, yFloor, ia);
                a11 = pixel<float>(xCeil, yCeil, ia);
            }
            break;
        case UCHAR:
        {
            r00 = pixel<unsigned char>(xFloor, yFloor, ir) / 255.0f;
            r01 = pixel<unsigned char>(xFloor, yCeil, ir) / 255.0f;
            r10 = pixel<unsigned char>(xCeil, yFloor, ir) / 255.0f;
            r11 = pixel<unsigned char>(xCeil, yCeil, ir) / 255.0f;
        }
            if (m_numChannels > 1)
            {
                g00 = pixel<unsigned char>(xFloor, yFloor, ig) / 255.0f;
                g01 = pixel<unsigned char>(xFloor, yCeil, ig) / 255.0f;
                g10 = pixel<unsigned char>(xCeil, yFloor, ig) / 255.0f;
                g11 = pixel<unsigned char>(xCeil, yCeil, ig) / 255.0f;
            }
            if (m_numChannels > 2)
            {
                b00 = pixel<unsigned char>(xFloor, yFloor, ib) / 255.0f;
                b01 = pixel<unsigned char>(xFloor, yCeil, ib) / 255.0f;
                b10 = pixel<unsigned char>(xCeil, yFloor, ib) / 255.0f;
                b11 = pixel<unsigned char>(xCeil, yCeil, ib) / 255.0f;
            }
            if (m_numChannels > 3)
            {
                a00 = pixel<unsigned char>(xFloor, yFloor, ia) / 255.0f;
                a01 = pixel<unsigned char>(xFloor, yCeil, ia) / 255.0f;
                a10 = pixel<unsigned char>(xCeil, yFloor, ia) / 255.0f;
                a11 = pixel<unsigned char>(xCeil, yCeil, ia) / 255.0f;
            }
            break;
        case USHORT:
        {
            r00 = pixel<unsigned short>(xFloor, yFloor, ir) / 65535.0f;
            r01 = pixel<unsigned short>(xFloor, yCeil, ir) / 65535.0f;
            r10 = pixel<unsigned short>(xCeil, yFloor, ir) / 65535.0f;
            r11 = pixel<unsigned short>(xCeil, yCeil, ir) / 65535.0f;
        }
            if (m_numChannels > 1)
            {
                g00 = pixel<unsigned short>(xFloor, yFloor, ig) / 65535.0f;
                g01 = pixel<unsigned short>(xFloor, yCeil, ig) / 65535.0f;
                g10 = pixel<unsigned short>(xCeil, yFloor, ig) / 65535.0f;
                g11 = pixel<unsigned short>(xCeil, yCeil, ig) / 65535.0f;
            }
            if (m_numChannels > 2)
            {
                b00 = pixel<unsigned short>(xFloor, yFloor, ib) / 65535.0f;
                b01 = pixel<unsigned short>(xFloor, yCeil, ib) / 65535.0f;
                b10 = pixel<unsigned short>(xCeil, yFloor, ib) / 65535.0f;
                b11 = pixel<unsigned short>(xCeil, yCeil, ib) / 65535.0f;
            }
            if (m_numChannels > 3)
            {
                a00 = pixel<unsigned short>(xFloor, yFloor, ia) / 65535.0f;
                a01 = pixel<unsigned short>(xFloor, yCeil, ia) / 65535.0f;
                a10 = pixel<unsigned short>(xCeil, yFloor, ia) / 65535.0f;
                a11 = pixel<unsigned short>(xCeil, yCeil, ia) / 65535.0f;
            }
            break;
        case UINT:
        {
            r00 = pixel<unsigned int>(xFloor, yFloor, ir);
            r01 = pixel<unsigned int>(xFloor, yCeil, ir);
            r10 = pixel<unsigned int>(xCeil, yFloor, ir);
            r11 = pixel<unsigned int>(xCeil, yCeil, ir);
        }
            if (m_numChannels > 1)
            {
                g00 = pixel<unsigned int>(xFloor, yFloor, ig);
                g01 = pixel<unsigned int>(xFloor, yCeil, ig);
                g10 = pixel<unsigned int>(xCeil, yFloor, ig);
                g11 = pixel<unsigned int>(xCeil, yCeil, ig);
            }
            if (m_numChannels > 2)
            {
                b00 = pixel<unsigned int>(xFloor, yFloor, ib);
                b01 = pixel<unsigned int>(xFloor, yCeil, ib);
                b10 = pixel<unsigned int>(xCeil, yFloor, ib);
                b11 = pixel<unsigned int>(xCeil, yCeil, ib);
            }
            if (m_numChannels > 3)
            {
                a00 = pixel<unsigned int>(xFloor, yFloor, ia);
                a01 = pixel<unsigned int>(xFloor, yCeil, ia);
                a10 = pixel<unsigned int>(xCeil, yFloor, ia);
                a11 = pixel<unsigned int>(xCeil, yCeil, ia);
            }
            break;
#ifdef HAVE_HALF
        case HALF:
        {
            r00 = pixel<half>(xFloor, yFloor, ir);
            r01 = pixel<half>(xFloor, yCeil, ir);
            r10 = pixel<half>(xCeil, yFloor, ir);
            r11 = pixel<half>(xCeil, yCeil, ir);
        }
            if (m_numChannels > 1)
            {
                g00 = pixel<half>(xFloor, yFloor, ig);
                g01 = pixel<half>(xFloor, yCeil, ig);
                g10 = pixel<half>(xCeil, yFloor, ig);
                g11 = pixel<half>(xCeil, yCeil, ig);
            }
            if (m_numChannels > 2)
            {
                b00 = pixel<half>(xFloor, yFloor, ib);
                b01 = pixel<half>(xFloor, yCeil, ib);
                b10 = pixel<half>(xCeil, yFloor, ib);
                b11 = pixel<half>(xCeil, yCeil, ib);
            }
            if (m_numChannels > 3)
            {
                a00 = pixel<half>(xFloor, yFloor, ia);
                a01 = pixel<half>(xFloor, yCeil, ia);
                a10 = pixel<half>(xCeil, yFloor, ia);
                a11 = pixel<half>(xCeil, yCeil, ia);
            }
            break;
#endif
        case DOUBLE:
        {
            r00 = pixel<double>(xFloor, yFloor, ir);
            r01 = pixel<double>(xFloor, yCeil, ir);
            r10 = pixel<double>(xCeil, yFloor, ir);
            r11 = pixel<double>(xCeil, yCeil, ir);
        }
            if (m_numChannels > 1)
            {
                g00 = pixel<double>(xFloor, yFloor, ig);
                g01 = pixel<double>(xFloor, yCeil, ig);
                g10 = pixel<double>(xCeil, yFloor, ig);
                g11 = pixel<double>(xCeil, yCeil, ig);
            }
            if (m_numChannels > 2)
            {
                b00 = pixel<double>(xFloor, yFloor, ib);
                b01 = pixel<double>(xFloor, yCeil, ib);
                b10 = pixel<double>(xCeil, yFloor, ib);
                b11 = pixel<double>(xCeil, yCeil, ib);
            }
            if (m_numChannels > 3)
            {
                a00 = pixel<double>(xFloor, yFloor, ia);
                a01 = pixel<double>(xFloor, yCeil, ia);
                a10 = pixel<double>(xCeil, yFloor, ia);
                a11 = pixel<double>(xCeil, yCeil, ia);
            }
            break;

        case PACKED_R10_G10_B10_X2:
        {
            Pixel10 p00 = pixel<Pixel10>(xFloor, yFloor, ir);
            Pixel10 p01 = pixel<Pixel10>(xFloor, yCeil, ir);
            Pixel10 p10 = pixel<Pixel10>(xCeil, yFloor, ir);
            Pixel10 p11 = pixel<Pixel10>(xCeil, yCeil, ir);

            r00 = p00.red;
            r01 = p01.red;
            r10 = p10.red;
            r11 = p11.red;
            g00 = p00.green;
            g01 = p01.green;
            g10 = p10.green;
            g11 = p11.green;
            b00 = p00.blue;
            b01 = p01.blue;
            b10 = p10.blue;
            b11 = p11.blue;
            a00 = 1;
            a01 = 1;
            a10 = 1;
            a11 = 1;
        }
        break;

        case PACKED_X2_B10_G10_R10:
        {
            Pixel10Rev p00 = pixel<Pixel10Rev>(xFloor, yFloor, ir);
            Pixel10Rev p01 = pixel<Pixel10Rev>(xFloor, yCeil, ir);
            Pixel10Rev p10 = pixel<Pixel10Rev>(xCeil, yFloor, ir);
            Pixel10Rev p11 = pixel<Pixel10Rev>(xCeil, yCeil, ir);

            r00 = p00.red;
            r01 = p01.red;
            r10 = p10.red;
            r11 = p11.red;
            g00 = p00.green;
            g01 = p01.green;
            g10 = p10.green;
            g11 = p11.green;
            b00 = p00.blue;
            b01 = p01.blue;
            b10 = p10.blue;
            b11 = p11.blue;
            a00 = 1;
            a01 = 1;
            a10 = 1;
            a11 = 1;
        }
        break;
        default:
            break;
        }

        // Time for some LERP'in
        const float dx = x - xFloor;
        const float dy = y - yFloor;

        r = TwkMath::lerp(TwkMath::lerp(r00, r10, dx),
                          TwkMath::lerp(r01, r11, dx), dy);
        g = TwkMath::lerp(TwkMath::lerp(g00, g10, dx),
                          TwkMath::lerp(g01, g11, dx), dy);
        b = TwkMath::lerp(TwkMath::lerp(b00, b10, dx),
                          TwkMath::lerp(b01, b11, dx), dy);
    }

    void FrameBuffer::getPixelBilinearRGB4f(float x, float y, float* p) const
    {
        getPixelBilinear4f(x, y, p);

        if (isYUV())
        {
            float& r = p[0];
            float& g = p[1];
            float& b = p[2];

            r += 0.0;
            g += -0.5;
            b += -0.5;

            const float r0 = r;
            const float g0 = g;
            const float b0 = b;

            r = r0 + 1.402 * b0;
            g = r0 + -0.344136286201 * g0 + -0.714136286201 * b0;
            b = r0 + 1.772 * g0;
        }
    }

    FrameBuffer* FrameBuffer::copy() const
    {
        FrameBuffer* fb = referenceCopy();
        fb->ownData();
        return fb;
    }

    FrameBuffer* FrameBuffer::copyPlane() const
    {
        vector<string> chname(1);
        chname.front() = m_channelNames.front();

        FrameBuffer* fb = new FrameBuffer(
            m_coordinateType, m_width, m_height, m_depth, m_numChannels,
            m_dataType, 0, &m_channelNames, m_orientation, true,
            extraScanlines(), scanlinePixelPadding());

        fb->m_coordinateType = coordinateType();

        memcpy(fb->m_data, m_data, m_allocSize);
        fb->setUncrop(m_uncropWidth, m_uncropHeight, m_uncropX, m_uncropY);
        fb->setUncropActive(m_uncrop);
        return fb;
    }

    FrameBuffer* FrameBuffer::referenceCopy() const
    {
        FrameBuffer* fb = new FrameBuffer(
            m_coordinateType, m_width, m_height, m_depth, m_numChannels,
            m_dataType, m_data, &m_channelNames, m_orientation, false,
            extraScanlines(), scanlinePixelPadding());

        fb->m_coordinateType = coordinateType();

        copyAttributesTo(fb);
        if (isRootPlane())
            fb->setIdentifier(identifier());
        fb->setUncrop(m_uncropWidth, m_uncropHeight, m_uncropX, m_uncropY);
        fb->setUncropActive(m_uncrop);

        if (hasAttribute("PixelAspectRatio"))
        {
            fb->setPixelAspectRatio(pixelAspectRatio());
        }

        if (nextPlane())
        {
            fb->appendPlane(nextPlane()->referenceCopy());
        }

        return fb;
    }

    void FrameBuffer::copyFrom(const FrameBuffer* fb)
    {
        if (fb == this)
            return;
        referenceCopyFrom(fb);
        ownData();
    }

    void FrameBuffer::referenceCopyFrom(const FrameBuffer* fb)
    {
        m_coordinateType = fb->coordinateType();

        restructure(fb->width(), fb->height(), fb->depth(), fb->numChannels(),
                    fb->dataType(),
                    const_cast<FrameBuffer*>(fb)->pixels<unsigned char>(),
                    &fb->channelNames(), fb->orientation(), false,
                    fb->extraScanlines(), fb->scanlinePixelPadding());

        fb->copyAttributesTo(this);
        setPixelAspectRatio(fb->pixelAspectRatio());
        if (isRootPlane())
            setIdentifier(fb->identifier());
        setUncrop(fb->uncropWidth(), fb->uncropHeight(), fb->uncropX(),
                  fb->uncropY());
        setUncropActive(fb->uncrop());

        if (fb->nextPlane())
        {
            if (nextPlane())
            {
                nextPlane()->referenceCopyFrom(fb->nextPlane());
            }
            else
            {
                appendPlane(fb->nextPlane()->referenceCopy());
            }
        }
    }

    void FrameBuffer::clearAttributes()
    {
        for (size_t i = 0; i < m_attributes.size(); i++)
        {
            delete m_attributes[i];
        }

        m_attributes.clear();
    }

    void FrameBuffer::outputAttrs(ostream& o) const
    {
        for (AttributeVector::const_iterator i = m_attributes.begin();
             i != m_attributes.end(); ++i)
        {
            FBAttribute* a = *i;
            o << a->name() << " = " << a->valueAsString() << endl;
        }
    }

    void FrameBuffer::outputInfo(ostream& o) const
    {
        const char* type = 0;
        const char* otype = 0;

        switch (dataType())
        {
        case BIT:
            type = "1 bit";
            break;
        case UCHAR:
            type = "8 bit int";
            break;
        // case USHORT_BIG:
        case USHORT:
            type = "16 bit int";
            break;
        // case UINT_BIG:
        case UINT:
            type = "32 bit int";
            break;
        // case HALF_BIG:
        case HALF:
            type = "16 bit float";
            break;
        // case FLOAT_BIG:
        case FLOAT:
            type = "32 bit float";
            break;
        // case DOUBLE_BIG:
        case DOUBLE:
            type = "64 bit float";
            break;
        case PACKED_Cb8_Y8_Cr8_Y8:
            type = "32 bit packed VYUY";
            break;
        case PACKED_Y8_Cb8_Y8_Cr8:
            type = "32 bit packed YUYV";
            break;
        case PACKED_R10_G10_B10_X2:
            type = "32 bit packed 10 bit RGB";
            break;
        case PACKED_X2_B10_G10_R10:
            type = "32 bit packed 10 bit BGR";
            break;
        default:
            type = "bad bit depth";
            break;
        }

        switch (orientation())
        {
        case NATURAL:
            otype = "bottom left";
            break;
        case TOPLEFT:
            otype = "top left";
            break;
        case TOPRIGHT:
            otype = "top right";
            break;
        case BOTTOMRIGHT:
            otype = "bottom right";
            break;
        default:
            break;
        }

        o << "FB: " << width() << " x " << height() << "  " << type
          << " per channel,"
          << " origin " << otype << ", channels = [";

        for (int i = 0; i < m_channelNames.size(); i++)
        {
            if (i)
                o << ", ";
            o << m_channelNames[i];
        }

        o << "]";

        if (m_uncrop)
        {
            o << " uncrop = ["
              << " " << m_uncropWidth << " " << m_uncropHeight << " "
              << m_uncropX << " " << m_uncropY << "]";
        }

        if (m_nextPlane)
        {
            o << " ++ ";
            m_nextPlane->outputInfo(o);
        }
    }

    void
    FrameBuffer::appendAttributesAndPrefixTo(FrameBuffer* fb,
                                             const std::string& prefix) const
    {
        if (fb == this)
            return;

        for (int i = 0; i < attributes().size(); i++)
        {
            fb->addAttribute(attributes()[i]->copyWithPrefix(prefix));
        }
    }

    void FrameBuffer::copyAttributesTo(FrameBuffer* fb) const
    {
        if (fb == this)
            return;

        fb->clearAttributes();

        for (int i = 0; i < attributes().size(); i++)
        {
            fb->addAttribute(attributes()[i]->copy());
        }
    }

    const FBAttribute* FrameBuffer::findAttribute(const std::string& name) const
    {
        for (int i = 0; i < m_attributes.size(); i++)
        {
            if (m_attributes[i]->name() == name)
            {
                return m_attributes[i];
            }
        }

        return 0;
    }

    FBAttribute* FrameBuffer::findAttribute(const std::string& name)
    {
        for (int i = 0; i < m_attributes.size(); i++)
        {
            if (m_attributes[i]->name() == name)
            {
                return m_attributes[i];
            }
        }

        return 0;
    }

    void FrameBuffer::deleteAttribute(const FBAttribute* a)
    {
        AttributeVector::iterator i =
            find(m_attributes.begin(), m_attributes.end(), a);

        if (i != m_attributes.end())
        {
            delete *i;
            m_attributes.erase(i);
        }
    }

    void FrameBuffer::setPrimaryColorSpace(const string& name)
    {
        attribute<string>(ColorSpace::Primaries()) = name;
    }

    void FrameBuffer::setTransferFunction(const string& name)
    {
        attribute<string>(ColorSpace::TransferFunction()) = name;
    }

    void FrameBuffer::setConversion(const string& name)
    {
        attribute<string>(ColorSpace::Conversion()) = name;
    }

    void FrameBuffer::setRange(const string& name)
    {
        attribute<string>(ColorSpace::Range()) = name;
    }

    void FrameBuffer::setChromaPlacement(const string& name)
    {
        attribute<string>(ColorSpace::ChromaPlacement()) = name;
    }

    void FrameBuffer::setGamma(float exponent)
    {
        attribute<float>(ColorSpace::Gamma()) = exponent;
    }

    bool FrameBuffer::hasPrimaryColorSpace() const
    {
        return findAttribute(ColorSpace::Primaries()) != 0;
    }

    bool FrameBuffer::hasConversion() const
    {
        return findAttribute(ColorSpace::Conversion()) != 0;
    }

    bool FrameBuffer::hasRange() const
    {
        return findAttribute(ColorSpace::Range()) != 0;
    }

    bool FrameBuffer::hasRGBtoXYZMatrix() const
    {
        return findAttribute(ColorSpace::RGBtoXYZMatrix()) != 0;
    }

    bool FrameBuffer::hasLogCParameters() const
    {
        return findAttribute(ColorSpace::LogCBlackOffset()) != 0;
    }

    bool FrameBuffer::hasTransferFunction() const
    {
        return findAttribute(ColorSpace::TransferFunction()) != 0;
    }

    bool FrameBuffer::hasPrimaries() const
    {
        return findAttribute(ColorSpace::WhitePrimary()) != 0;
    }

    bool FrameBuffer::hasAdoptedNeutral() const
    {
        return findAttribute(ColorSpace::AdoptedNeutral()) != 0;
    }

    void FrameBuffer::primaries(const std::string& primaryName, float& x,
                                float& y) const
    {
        if (const FBAttribute* a = findAttribute(primaryName))
        {
            if (const Vec2fAttribute* sa =
                    dynamic_cast<const Vec2fAttribute*>(a))
            {
                x = sa->value().x;
                y = sa->value().y;
                return;
            }
        }

        x = 0;
        y = 0;
    }

    void FrameBuffer::setAdoptedNeutral(float x, float y)
    {
        attribute<Vec2f>(ColorSpace::AdoptedNeutral()) = Vec2f(x, y);
    }

    void FrameBuffer::setPrimaries(float xWhite, float yWhite, float xRed,
                                   float yRed, float xGreen, float yGreen,
                                   float xBlue, float yBlue)
    {
        attribute<Vec2f>(ColorSpace::BluePrimary()) = Vec2f(xBlue, yBlue);
        attribute<Vec2f>(ColorSpace::GreenPrimary()) = Vec2f(xGreen, yGreen);
        attribute<Vec2f>(ColorSpace::RedPrimary()) = Vec2f(xRed, yRed);
        attribute<Vec2f>(ColorSpace::WhitePrimary()) = Vec2f(xWhite, yWhite);
    }

    void FrameBuffer::setPrimaries(const Chromaticities& c)
    {
        attribute<Vec2f>(ColorSpace::BluePrimary()) = c.blue;
        attribute<Vec2f>(ColorSpace::GreenPrimary()) = c.green;
        attribute<Vec2f>(ColorSpace::RedPrimary()) = c.red;
        attribute<Vec2f>(ColorSpace::WhitePrimary()) = c.white;
    }

    FrameBuffer::Chromaticities FrameBuffer::chromaticities() const
    {
        Chromaticities chr = Chromaticities::Rec709();

        if (hasAttribute(ColorSpace::BluePrimary()))
            chr.blue = attribute<Vec2f>(ColorSpace::BluePrimary());
        if (hasAttribute(ColorSpace::GreenPrimary()))
            chr.green = attribute<Vec2f>(ColorSpace::GreenPrimary());
        if (hasAttribute(ColorSpace::RedPrimary()))
            chr.red = attribute<Vec2f>(ColorSpace::RedPrimary());
        if (hasAttribute(ColorSpace::WhitePrimary()))
            chr.white = attribute<Vec2f>(ColorSpace::WhitePrimary());

        return chr;
    }

    void FrameBuffer::setMatrixAttribute(std::string name, float r0c0,
                                         float r0c1, float r0c2, float r0c3,
                                         float r1c0, float r1c1, float r1c2,
                                         float r1c3, float r2c0, float r2c1,
                                         float r2c2, float r2c3, float r3c0,
                                         float r3c1, float r3c2, float r3c3)
    {
        attribute<Mat44f>(name) =
            Mat44f(r0c0, r0c1, r0c2, r0c3, r1c0, r1c1, r1c2, r1c3, r2c0, r2c1,
                   r2c2, r2c3, r3c0, r3c1, r3c2, r3c3);
    }

    void FrameBuffer::setRGBToXYZMatrix(float r0c0, float r0c1, float r0c2,
                                        float r0c3, float r1c0, float r1c1,
                                        float r1c2, float r1c3, float r2c0,
                                        float r2c1, float r2c2, float r2c3,
                                        float r3c0, float r3c1, float r3c2,
                                        float r3c3)
    {
        setMatrixAttribute(ColorSpace::RGBtoXYZMatrix(), r0c0, r0c1, r0c2, r0c3,
                           r1c0, r1c1, r1c2, r1c3, r2c0, r2c1, r2c2, r2c3, r3c0,
                           r3c1, r3c2, r3c3);
    }

    const string& FrameBuffer::primaryColorSpace() const
    {
        static string generic = "Generic";

        if (const FBAttribute* a = findAttribute(ColorSpace::Primaries()))
        {
            if (const StringAttribute* sa =
                    dynamic_cast<const StringAttribute*>(a))
            {
                return sa->value();
            }
        }

        return generic;
    }

    const string& FrameBuffer::transferFunction() const
    {
        static string none = "None";

        if (const FBAttribute* a =
                findAttribute(ColorSpace::TransferFunction()))
        {
            if (const StringAttribute* sa =
                    dynamic_cast<const StringAttribute*>(a))
            {
                return sa->value();
            }
        }

        return none;
    }

    const string& FrameBuffer::conversion() const
    {
        static string none = "None";

        if (const FBAttribute* a = findAttribute(ColorSpace::Conversion()))
        {
            if (const StringAttribute* sa =
                    dynamic_cast<const StringAttribute*>(a))
            {
                return sa->value();
            }
        }

        return none;
    }

    const string& FrameBuffer::range() const
    {
        static string none = "None";

        if (const FBAttribute* a = findAttribute(ColorSpace::Range()))
        {
            if (const StringAttribute* sa =
                    dynamic_cast<const StringAttribute*>(a))
            {
                return sa->value();
            }
        }

        return none;
    }

    void FrameBuffer::setICCprofile(const void* prof, size_t len)
    {
        cmsHPROFILE hProfile = cmsOpenProfileFromMem(prof, len);

        assert(hProfile != NULL);

        if (hProfile)
        {
            attribute<float>(ColorSpace::ICCProfileVersion()) =
                float(cmsGetProfileVersion(hProfile));

            char temp[256];
            cmsGetProfileInfoASCII(hProfile, cmsInfoDescription, "en", "US",
                                   temp, 256);

            attribute<string>(ColorSpace::ICCProfileDescription()) = temp;

            if (FBAttribute* a = findAttribute(ColorSpace::ICCProfileData()))
            {
                if (DataContainerAttribute* da =
                        dynamic_cast<DataContainerAttribute*>(a))
                {
                    da->set(prof, len);
                }
            }
            else
            {
                addAttribute(new DataContainerAttribute(
                    ColorSpace::ICCProfileData(), prof, len));
            }

            cmsCloseProfile(hProfile);
        }
    }

    const DataContainer* FrameBuffer::iccProfile() const
    {
        if (const FBAttribute* a = findAttribute(ColorSpace::ICCProfileData()))
        {
            if (const DataContainerAttribute* da =
                    dynamic_cast<const DataContainerAttribute*>(a))
            {
                return da->dataContainer();
            }
        }

        return 0;
    }

    void FrameBuffer::ownData()
    {
        assert(m_data != (unsigned char*)0xdeadc0de);
        assert(m_nextPlane != (FrameBuffer*)0xdeadc0de);
        assert(m_previousPlane != (FrameBuffer*)0xdeadc0de);
        assert(m_firstPlane != (FrameBuffer*)0xdeadc0de);

        // If this FrameBuffer is a proxy buffer, ie.: if its data is a
        // reference to another FrameBuffer's data, it can't be made owner of
        // the data.
        if (findAttribute("ProxyBufferOwnerPtr"))
        {
            return;
        }

        // If this FrameBuffer has already been made owner of its data, do
        // nothing.
        if (m_deleteDataOnDestruction)
            return;
        unsigned char* data = (unsigned char*)allocateLargeBlock(m_allocSize);

        // cout << "FB: allocated (2) " << (void*)data << endl;
        if (!data)
            TWK_THROW_STREAM(IOException, "Out of memory");
        memcpy(data, m_data, m_allocSize);
        m_data = data;
        m_deleteDataOnDestruction = true;

        if (nextPlane())
            nextPlane()->ownData();
    }

    void FrameBuffer::relinquishDataAndReset()
    {
        assert(m_data != (unsigned char*)0xdeadc0de);

        m_data = 0;
        m_width = 0;
        m_deleteDataOnDestruction = false;

        if (nextPlane())
        {
            nextPlane()->relinquishDataAndReset();
        }

        return;
    }

    void FrameBuffer::appendPlane(FrameBuffer* fb)
    {
        if (!fb)
            return;
        assert(nextPlane() != (FrameBuffer*)0xdeadc0de);

        if (fb->previousPlane())
        {
            TWK_THROW_EXC_STREAM(
                "Attempt to append a plane to FrameBuffer that is already "
                "part of a multi-plane FrameBuffer failed.");
        }

        FrameBuffer* f = this;
        while (f->m_nextPlane)
            f = f->m_nextPlane;
        f->m_nextPlane = fb;
        fb->m_previousPlane = f;
        fb->m_firstPlane = f->m_firstPlane ? f->m_firstPlane : f;
    }

    void FrameBuffer::removePlane(FrameBuffer* fb)
    {
        if (!fb->nextPlane() && !fb->previousPlane())
        {
            TWK_THROW_EXC_STREAM(
                "Attempt to remove a plane from a FrameBuffer that it is "
                "not part of.");
        }

        if (fb == this)
            return;

        FrameBuffer* f = m_firstPlane ? m_firstPlane : this;
        while (f->m_nextPlane != fb)
            f = f->m_nextPlane;

        f->m_nextPlane = fb->m_nextPlane;
        fb->m_nextPlane = 0;
        fb->m_firstPlane = 0;
        fb->m_previousPlane = 0;
    }

    size_t FrameBuffer::numPlanes() const
    {
        if (!firstPlane())
            return 1;
        size_t count = 0;
        for (const FrameBuffer* f = firstPlane(); f; f = f->nextPlane())
            count++;
        return count;
    }

    void FrameBuffer::deleteAllPlanes()
    {
        if (isPlanar())
        {
            assert(isRootPlane());
            deleteNextPlane();
        }
    }

    void FrameBuffer::deleteNextPlane()
    {
        if (FrameBuffer* fb = nextPlane())
        {
            fb->deleteNextPlane();
            removePlane(fb);
            delete fb;
        }
    }

    void FrameBuffer::shallowCopy(FrameBuffer* fb)
    {
        if (fb == this)
            return;

        restructure(fb->width(), fb->height(), fb->depth(), fb->numChannels(),
                    fb->dataType(), fb->pixels<unsigned char>(),
                    &fb->channelNames(), fb->orientation(), false);

        fb->copyAttributesTo(this);
        if (isRootPlane())
            setIdentifier(fb->identifier());

        if (fb->nextPlane())
        {
            if (nextPlane())
            {
                nextPlane()->shallowCopy(fb->nextPlane());
            }
            else
            {
                FrameBuffer* nfb = new FrameBuffer();
                appendPlane(nfb);
                nfb->shallowCopy(fb->nextPlane());
            }
        }
    }

    bool FrameBuffer::isYRYBYPlanar() const
    {
        if (numChannels() == 1)
        {
            if (channelName(0) == "Y")
            {
                if (const FrameBuffer* ry = nextPlane())
                {
                    if (ry->numChannels() == 1)
                    {
                        if (ry->channelName(0) == "RY")
                        {
                            if (const FrameBuffer* by = ry->nextPlane())
                            {
                                if (by->numChannels() == 1)
                                {
                                    return by->channelName(0) == "BY";
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    bool FrameBuffer::isYUVPlanar() const
    {
        if (numChannels() == 1)
        {
            if (channelName(0) == "Y")
            {
                if (const FrameBuffer* ry = nextPlane())
                {
                    if (ry->numChannels() == 1)
                    {
                        if (ry->channelName(0) == "U")
                        {
                            if (const FrameBuffer* by = ry->nextPlane())
                            {
                                if (by->numChannels() == 1)
                                {
                                    return by->channelName(0) == "V";
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    bool FrameBuffer::isYUVBiPlanar() const
    {
        if (hasChannel("Y"))
        {
            if (const FrameBuffer* uv = nextPlane())
            {
                return uv->numChannels() == 2 && uv->hasChannel("U")
                       && uv->hasChannel("V");
            }
        }

        return false;
    }

    bool FrameBuffer::isYA2C2Planar() const
    {
        if (numChannels() == 2)
        {
            if (channelName(0) == "Y" && channelName(1) == "A")
            {
                if (const FrameBuffer* c = nextPlane())
                {
                    if (c->numChannels() == 2)
                    {
                        return c->channelName(0) == "RY"
                               && c->channelName(1) == "BY";
                    }
                }
            }
        }

        return false;
    }

    bool FrameBuffer::isRGBPlanar() const
    {
        if (numChannels() == 1)
        {
            if (channelName(0) == "R")
            {
                if (const FrameBuffer* ry = nextPlane())
                {
                    if (ry->numChannels() == 1)
                    {
                        if (ry->channelName(0) == "G")
                        {
                            if (const FrameBuffer* by = ry->nextPlane())
                            {
                                if (by->numChannels() == 1)
                                {
                                    return by->channelName(0) == "B";
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    void FrameBuffer::setUncropActive(bool b)
    {
        m_uncrop = b;
        if (nextPlane())
            nextPlane()->setUncropActive(b);
    }

    void FrameBuffer::setUncrop(int w, int h, int x, int y)
    {
        m_uncropWidth = w;
        m_uncropHeight = h;
        m_uncropX = x;
        m_uncropY = y;
        m_uncrop = true;

        if (FrameBuffer* fb = nextPlane())
        {
            int ucw = w;
            int uch = h;
            int ucx = x;
            int ucy = y;

            if (fb->width() != width() || fb->height() != height())
            {
                const double wr = double(fb->width()) / double(width());
                const double hr = double(fb->height()) / double(height());
                ucw = int(wr * double(w));
                uch = int(hr * double(h));
                ucx = int(double(x) * wr);
                ucy = int(double(y) * hr);
            }

            fb->setUncrop(ucw, uch, ucx, ucy);
        }
    }

    void FrameBuffer::setUncrop(const FrameBuffer* fb)
    {
        if (fb == this)
            return;
        setUncrop(fb->uncropWidth(), fb->uncropHeight(), fb->uncropX(),
                  fb->uncropY());
        setUncropActive(fb->uncrop());
    }

    float FrameBuffer::uncroppedAspect() const
    {
        return float(m_uncropWidth) / float(m_uncropHeight);
    }

    void* FrameBuffer::allocateLargeBlock(size_t s)
    {
        return TwkUtil::MemPool::alloc(s);
    }

    void FrameBuffer::deallocateLargeBlock(void* p)
    {
        return TwkUtil::MemPool::dealloc(p);
    }

    size_t FrameBuffer::sizeOfDataType(DataType t)
    {
        switch (t)
        {
        default:
        case UCHAR:
            return sizeof(unsigned char);
        case HALF:
            return sizeof(half);
        // case USHORT_BIG:
        case USHORT:
            return sizeof(unsigned short);
        // case UINT_BIG:
        case UINT:
            return sizeof(unsigned int);
        // case FLOAT_BIG:
        case FLOAT:
            return sizeof(float);
        // case DOUBLE_BIG:
        case DOUBLE:
            return sizeof(double);
        case PACKED_Y8_Cb8_Y8_Cr8:
        case PACKED_Cb8_Y8_Cr8_Y8:
        case PACKED_R10_G10_B10_X2:
        case PACKED_X2_B10_G10_R10:
            return sizeof(unsigned short);
        }
    }

} //  End namespace TwkFB

#ifdef _MSC_VER
#undef snprintf
#endif
