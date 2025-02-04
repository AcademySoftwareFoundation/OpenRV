//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <TwkGLF/GLFBO.h>
#include <limits>

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <GL/gl.h>
#include <GL/glu.h>
#define DEFAULT_RINGBUFFER_SIZE 4
#endif

#ifdef PLATFORM_LINUX
#include <GL/gl.h>
#include "ntv2linuxpublicinterface.h"
#define DEFAULT_RINGBUFFER_SIZE 3
#endif

#ifdef PLATFORM_DARWIN
#define DEFAULT_RINGBUFFER_SIZE 4
#endif

#include "ntv2devicefeatures.h"
#include "ntv2formatdescriptor.h"
#include "ntv2vpid.h"
#include <stl_ext/replace_alloc.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <AJADevices/KonaVideoDevice.h>
#include <AJADevices/AJAModule.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/ProcessInfo.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/Macros.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/ThreadName.h>
#include <TwkFB/FastMemcpy.h>

#include <algorithm>
#include <cstdlib> // atof, atoi
#include <string>
#include <fstream>
#include <sstream>

#define GC_EXCLUDE_THIS_THREAD

namespace AJADevices
{
    using namespace std;
    using namespace TwkApp;
    using namespace TwkGLF;
    using namespace TwkUtil;
    using namespace boost::program_options;
    using namespace boost::algorithm;
    using namespace boost;

    // 4K Transport combo box items indices
    const size_t VIDEO_4K_TRANSPORT_QUADRANTS = 0;
    const size_t VIDEO_4K_TRANSPORT_TSI = 1;

    const char* const HDMI_HDR_METADATA_ARG = "hdmi-hdr-metadata";

    KonaVideoDevice::FrameData::FrameData()
        : fbo(0)
        , globject(0)
        , mappedPointer(0)
        , imageData(0)
        , state(FrameData::State::NotReady)
        , fence(0)
        , locked(false)
    {
    }

    KonaVideoDevice::FrameData::FrameData(const KonaVideoDevice::FrameData& f)
        : fbo(f.fbo)
        , globject(f.globject)
        , mappedPointer(f.mappedPointer)
        , imageData(f.imageData)
        , audioBuffer(f.audioBuffer)
        , state(f.state)
        , fence(f.fence)
        , locked(false)
    {
    }

    KonaVideoDevice::FrameData&
    KonaVideoDevice::FrameData::operator=(const KonaVideoDevice::FrameData& f)
    {
        fbo = f.fbo;
        globject = f.globject;
        imageData = f.imageData;
        state = f.state;
        mappedPointer = f.mappedPointer;
        audioBuffer = f.audioBuffer;
        fence = f.fence;
        locked = false;
        return *this;
    }

    KonaVideoDevice::FrameData::~FrameData() {}

    void KonaVideoDevice::FrameData::lockImage(const char* threadName)
    {
        Timer timer;
        timer.start();
        imageMutex.lock();
        Time t = timer.elapsed();
        if (t > 0.001 && m_infoFeedback)
            cout << "INFO: " << threadName << ": lockImage for " << t << endl;
    }

    void KonaVideoDevice::FrameData::unlockImage() { imageMutex.unlock(); }

    void KonaVideoDevice::FrameData::lockAudio(const char* threadName)
    {
        Timer timer;
        timer.start();
        audioMutex.lock();
        Time t = timer.elapsed();
        if (t > 0.001 && m_infoFeedback)
            cout << "INFO: " << threadName << ": lockAudio for " << t << endl;
    }

    void KonaVideoDevice::FrameData::unlockAudio() { audioMutex.unlock(); }

    void KonaVideoDevice::FrameData::lockState(const char* threadName)
    {
        Timer timer;
        timer.start();
        stateMutex.lock();
        Time t = timer.elapsed();
        if (t > 0.001 && m_infoFeedback)
            cout << "INFO: " << threadName << ": lockState for " << t << endl;
    }

    void KonaVideoDevice::FrameData::unlockState() { stateMutex.unlock(); }

    KonaVideoDevice::VideoChannel::VideoChannel(NTV2FrameBufferFormat format,
                                                NTV2Channel ch, size_t bsize,
                                                size_t n)
        : bufferSizeInBytes(bsize)
        , channel(ch)
        , data(n)
    {
    }

    KonaVideoDevice::VideoChannel::~VideoChannel()
    {
        for (size_t i = 0; i < data.size(); i++)
        {
            FrameData& f = data[i];
            if (f.imageData)
                TWK_DEALLOCATE_ARRAY(f.imageData);
        }
    }

    namespace
    {

        //
        // RV is really only able to do stereo right now mostly because of the
        //  audio I/O.
        //

        KonaAudioFormat audioFormats[] = {
            {48000.0, TwkAudio::Int32Format, 2, TwkAudio::Stereo_2,
             "24 bit 48kHz Stereo"},
            {48000.0, TwkAudio::Int32Format, 6, TwkAudio::Surround_5_1,
             "24 bit 48kHz 5.1 Surround"},
            {48000.0, TwkAudio::Int32Format, 8, TwkAudio::Surround_7_1,
             "24 bit 48kHz 7.1 Surround"},
            {48000.0, TwkAudio::Int32Format, 8, TwkAudio::SDDS_7_1,
             "24 bit 48kHz 7.1 Surround SDDS"},
            {48000.0, TwkAudio::Int32Format, 16, TwkAudio::Generic_16,
             "24 bit 48kHz 16 channel"}
            // {96000.0, TwkAudio::Int32Format, 2, TwkAudio::Stereo_2, "24 bit
            // 96kHz Stereo"}
        };

        KonaSyncMode syncModes[] = {
            {"Free Running", 0}
            //{"Gen Lock", NV_CTRL_GVO_SYNC_MODE_GENLOCK},
            //{"Frame Lock", NV_CTRL_GVO_SYNC_MODE_FRAMELOCK}
        };

        KonaSyncSource syncSources[] = {{"Composite", 0}, {"SDI", 1}};

        KonaVideo4KTransport video4KTransports[] = {
            {"Quadrants 4-wire", VIDEO_4K_TRANSPORT_QUADRANTS},
            {"SMPTE 2SI", VIDEO_4K_TRANSPORT_TSI}};

        KonaDataFormat dataFormatsCP[] = {
            //
            //  In this case we're going to get everything from the AJA
            //  control panel.
            //

            {"AJA Control Panel", (NTV2FrameBufferFormat)999, VideoDevice::RGB8,
             DataFormatFlags::NoFlags},
            // End of list
            {"", (NTV2FrameBufferFormat)999, VideoDevice::RGB8,
             DataFormatFlags::NoFlags}};

        KonaDataFormat dataFormatsYUV[] = {
            //
            //  These are for t-tap: it can only receive YUV formats so the
            //  renderer will use the GPU to convert to YUV and then in
            //  software we do the 4:2:2 down convert (until we have a packing
            //  shader).
            //
            //  Using GL_PACK_ROW_LENGTH slows down glReadPixels sufficiently
            //  to make it not worthwhile.
            //

            {"8 Bit YCrCb 4:2:2 (YUV-8)", NTV2_FBF_8BIT_YCBCR,
             VideoDevice::Y0CbY1Cr_8_422, P2P},
            {"10 Bit YCrCb 4:2:2 (YUV-10)", NTV2_FBF_10BIT_YCBCR,
             VideoDevice::YCrCb_AJA_10_422, P2P},
            // End of list
            {"", NTV2_FBF_10BIT_YCBCR, VideoDevice::YCrCb_AJA_10_422, P2P}};

        KonaDataFormat dataFormats[] = {
            //
            //  NOTE: NTV2_FBF_10BIT_RGB matches the nvidia cards *internal*
            //  10 bit format. The corresponding GL/DVP type is
            //  UNSIGNED_INT_2_10_10_10_REV.  At least with nvidia using any
            //  other 10 bit format causes a huge software conversion hit even
            //  with DVP.
            //
            //  Note: it doesn't matter which 16 bit RGB we use
            //  (BGR or RGB) they all are done in hardware
            //

            // name, kona FB format, video data format, rgb

            // Commented out "8 bit internal" formats cause they
            // appear buggy and are not useful to users anyhow.

            // YUV formats
            //{"10 Bit YCrCb 4:2:2 (8 bit internal)", NTV2_FBF_24BIT_RGB,
            // VideoDevice::RGB8, P2P},
            {"10 Bit YCrCb 4:2:2", NTV2_FBF_10BIT_RGB, VideoDevice::RGB10X2,
             P2P},
            {"10 Bit YCrCb 4:2:2 (Legal SDI Range)", NTV2_FBF_10BIT_RGB,
             VideoDevice::RGB10X2, P2P | LegalRangeY},

            // 3G RGB formats
            {"8 Bit 3G RGB", NTV2_FBF_24BIT_RGB, VideoDevice::RGB8, RGB_3G},
            {"8 Bit 3G RGBA", NTV2_FBF_ABGR, VideoDevice::RGBA8, RGB_3G},
            {"10 Bit 3G RGB", NTV2_FBF_10BIT_RGB, VideoDevice::RGB10X2, RGB_3G},
            {"12 Bit 3G RGB", NTV2_FBF_48BIT_RGB, VideoDevice::RGB16, RGB_3G},

            // Dual link RGB formats
            {"10 Bit Dual Link RGB", NTV2_FBF_10BIT_RGB, VideoDevice::RGB10X2,
             RGB_DualLink},
            {"12 Bit Dual Link RGB", NTV2_FBF_48BIT_RGB, VideoDevice::RGB16,
             RGB_DualLink},

            // Stereo
            // {"Stereo Dual 10 Bit YCrCb 4:2:2 (8 bit internal)",
            // NTV2_FBF_24BIT_RGB, VideoDevice::RGB8, P2P},
            {"Stereo Dual 10 Bit YCrCb 4:2:2", NTV2_FBF_10BIT_RGB,
             VideoDevice::RGB10X2, P2P},
            {"Stereo Dual 10 Bit YCrCb 4:2:2 (Legal SDI Range)",
             NTV2_FBF_10BIT_RGB, VideoDevice::RGB10X2, P2P | LegalRangeY},
            {"Stereo Dual 10 Bit 3G RGB", NTV2_FBF_10BIT_RGB,
             VideoDevice::RGB10X2, RGB_3G},
            {"Stereo Dual 12 Bit 3G RGB", NTV2_FBF_48BIT_RGB,
             VideoDevice::RGB16, RGB_3G},
            // End of list
            {"", NTV2_FBF_48BIT_RGB, VideoDevice::RGB16, RGB_3G}};

        KonaVideoFormat videoFormatsCP[] = {
            {0, 0, 0.0, 0.00, "AJA Control Panel", NTV2_FORMAT_UNKNOWN},
            // End of list
            {0, 0, 0.0, 0.00, "", NTV2_FORMAT_UNKNOWN}};

        KonaVideoFormat videoFormats[] = {
            {1280, 720, 1.0, 50.00, "720p 50Hz", NTV2_FORMAT_720p_5000},
            {1280, 720, 1.0, 59.94, "720p 59.94Hz", NTV2_FORMAT_720p_5994},
            {1280, 720, 1.0, 60.00, "720p 60Hz", NTV2_FORMAT_720p_6000},

            {1920, 1080, 1.0, 23.98, "1080p 23.98Hz", NTV2_FORMAT_1080p_2398},
            {1920, 1080, 1.0, 24.00, "1080p 24Hz", NTV2_FORMAT_1080p_2400},
            {1920, 1080, 1.0, 25.00, "1080p 25Hz", NTV2_FORMAT_1080p_2500},
            {1920, 1080, 1.0, 29.97, "1080p 29.97Hz", NTV2_FORMAT_1080p_2997},
            {1920, 1080, 1.0, 30.00, "1080p 30Hz", NTV2_FORMAT_1080p_3000},

            {1920, 1080, 1.0, 50.00, "1080p 50Hz Level B",
             NTV2_FORMAT_1080p_5000_B},
            {1920, 1080, 1.0, 59.94, "1080p 59.94Hz Level B",
             NTV2_FORMAT_1080p_5994_B},
            {1920, 1080, 1.0, 60.00, "1080p 60Hz Level B",
             NTV2_FORMAT_1080p_6000_B},

            {1920, 1080, 1.0, 50.00, "1080p 50Hz Level A",
             NTV2_FORMAT_1080p_5000_A},
            {1920, 1080, 1.0, 59.94, "1080p 59.94Hz Level A",
             NTV2_FORMAT_1080p_5994_A},
            {1920, 1080, 1.0, 60.00, "1080p 60Hz Level A",
             NTV2_FORMAT_1080p_6000_A},

            {1920, 1080, 1.0, 50.00 / 2.0, "1080i 50Hz",
             NTV2_FORMAT_1080i_5000},
            {1920, 1080, 1.0, 59.94 / 2.0, "1080i 59.94Hz",
             NTV2_FORMAT_1080i_5994},
            {1920, 1080, 1.0, 60.00 / 2.0, "1080i 60Hz",
             NTV2_FORMAT_1080i_6000},

            {1920, 1080, 1.0, 23.98, "1080psf 23.98Hz",
             NTV2_FORMAT_1080psf_2398},
            {1920, 1080, 1.0, 24.00, "1080psf 24Hz", NTV2_FORMAT_1080psf_2400},
            {1920, 1080, 1.0, 25.00, "1080psf 25Hz",
             NTV2_FORMAT_1080psf_2500_2},
            {1920, 1080, 1.0, 30.00, "1080psf 30Hz",
             NTV2_FORMAT_1080psf_3000_2},

            {2048, 1080, 1.0, 23.98, "1080p (2048x1080) 2K DCI 23.98Hz",
             NTV2_FORMAT_1080p_2K_2398}, // this needs to be looked at, might be
                                         // hd link
            {2048, 1080, 1.0, 24.00, "1080p (2048x1080) 2K DCI 24Hz",
             NTV2_FORMAT_1080p_2K_2400},
            {2048, 1080, 1.0, 25.00, "1080p (2048x1080) 2K DCI 25Hz",
             NTV2_FORMAT_1080p_2K_2500},
            {2048, 1080, 1.0, 29.97, "1080p (2048x1080) 2K DCI 29.97Hz",
             NTV2_FORMAT_1080p_2K_2997},
            {2048, 1080, 1.0, 30.00, "1080p (2048x1080) 2K DCI 30Hz",
             NTV2_FORMAT_1080p_2K_3000},

            {2048, 1080, 1.0, 47.95, "1080p (2048x1080) 2K DCI 47.95Hz Level A",
             NTV2_FORMAT_1080p_2K_4795_A},
            {2048, 1080, 1.0, 48.00, "1080p (2048x1080) 2K DCI 48Hz Level A",
             NTV2_FORMAT_1080p_2K_4800_A},
            {2048, 1080, 1.0, 59.94, "1080p (2048x1080) 2K DCI 59.94Hz Level A",
             NTV2_FORMAT_1080p_2K_5994_A},
            {2048, 1080, 1.0, 60.00, "1080p (2048x1080) 2K DCI 60Hz Level A",
             NTV2_FORMAT_1080p_2K_6000_A},

            {2048, 1080, 1.0, 47.95, "1080p (2048x1080) 2K DCI 47.95Hz Level B",
             NTV2_FORMAT_1080p_2K_4795_B},
            {2048, 1080, 1.0, 48.00, "1080p (2048x1080) 2K DCI 48Hz Level B",
             NTV2_FORMAT_1080p_2K_4800_B},
            {2048, 1080, 1.0, 59.94, "1080p (2048x1080) 2K DCI 59.94Hz Level B",
             NTV2_FORMAT_1080p_2K_5994_B},
            {2048, 1080, 1.0, 60.00, "1080p (2048x1080) 2K DCI 60Hz Level B",
             NTV2_FORMAT_1080p_2K_6000_B},

            {2048, 1080, 1.0, 23.98, "1080psf (2048x1080) 2K DCI 23.98Hz",
             NTV2_FORMAT_1080psf_2K_2398},
            {2048, 1080, 1.0, 24.00, "1080psf (2048x1080) 2K DCI 24Hz",
             NTV2_FORMAT_1080psf_2K_2400},
            {2048, 1080, 1.0, 25.00, "1080psf (2048x1080) 2K DCI 25Hz",
             NTV2_FORMAT_1080psf_2K_2500},

            // {2048, 1556, 1.0, 23.93, "1556psf (2048x1556) 2K 23.98Hz",
            // NTV2_FORMAT_2K_2398}, {2048, 1556, 1.0, 24.00, "1556psf
            // (2048x1556) 2K 24Hz", NTV2_FORMAT_2K_2400}, {2048,
            // 1556, 1.0, 25.00, "1556psf (2048x1556) 2K 25Hz",
            // NTV2_FORMAT_2K_2500, false},

            {3840, 2160, 1.0, 23.98, "2160p (3840x2160) Quad 4K UHD 23.98Hz",
             NTV2_FORMAT_4x1920x1080p_2398},
            {3840, 2160, 1.0, 24.00, "2160p (3840x2160) Quad 4K UHD 24Hz",
             NTV2_FORMAT_4x1920x1080p_2400},
            {3840, 2160, 1.0, 25.00, "2160p (3840x2160) Quad 4K UHD 25Hz",
             NTV2_FORMAT_4x1920x1080p_2500},
            {3840, 2160, 1.0, 29.97, "2160p (3840x2160) Quad 4K UHD 29.97Hz",
             NTV2_FORMAT_4x1920x1080p_2997},
            {3840, 2160, 1.0, 30.00, "2160p (3840x2160) Quad 4K UHD 30Hz",
             NTV2_FORMAT_4x1920x1080p_3000},
            {3840, 2160, 1.0, 50.00,
             "2160p (3840x2160) Quad 4K UHD 50Hz Level A",
             NTV2_FORMAT_4x1920x1080p_5000},
            {3840, 2160, 1.0, 59.94,
             "2160p (3840x2160) Quad 4K UHD 59.94Hz Level A",
             NTV2_FORMAT_4x1920x1080p_5994},
            {3840, 2160, 1.0, 60.00,
             "2160p (3840x2160) Quad 4K UHD 60Hz Level A",
             NTV2_FORMAT_4x1920x1080p_6000},
            {3840, 2160, 1.0, 50.00,
             "2160p (3840x2160) Quad 4K UHD 50Hz Level B",
             NTV2_FORMAT_4x1920x1080p_5000_B},
            {3840, 2160, 1.0, 59.94,
             "2160p (3840x2160) Quad 4K UHD 59.94Hz Level B",
             NTV2_FORMAT_4x1920x1080p_5994_B},
            {3840, 2160, 1.0, 60.00,
             "2160p (3840x2160) Quad 4K UHD 60Hz Level B",
             NTV2_FORMAT_4x1920x1080p_6000_B},

            {3840, 2160, 1.0, 23.98, "2160psf (3840x2160) Quad 4K UHD 23.98Hz",
             NTV2_FORMAT_4x1920x1080psf_2398},
            {3840, 2160, 1.0, 24.00, "2160psf (3840x2160) Quad 4K UHD 24Hz",
             NTV2_FORMAT_4x1920x1080psf_2400},
            {3840, 2160, 1.0, 25.00, "2160psf (3840x2160) Quad 4K UHD 25Hz",
             NTV2_FORMAT_4x1920x1080psf_2500},
            {3840, 2160, 1.0, 29.97, "2160psf (3840x2160) Quad 4K UHD 29.97Hz",
             NTV2_FORMAT_4x1920x1080psf_2997},
            {3840, 2160, 1.0, 30.00, "2160psf (3840x2160) Quad 4K UHD 30Hz",
             NTV2_FORMAT_4x1920x1080psf_3000},

            {4096, 2160, 1.0, 23.98, "2160p (4096x2160) Quad 4K DCI 23.98Hz",
             NTV2_FORMAT_4x2048x1080p_2398},
            {4096, 2160, 1.0, 24.00, "2160p (4096x2160) Quad 4K DCI 24Hz",
             NTV2_FORMAT_4x2048x1080p_2400},
            {4096, 2160, 1.0, 25.00, "2160p (4096x2160) Quad 4K DCI 25Hz",
             NTV2_FORMAT_4x2048x1080p_2500},
            {4096, 2160, 1.0, 29.97, "2160p (4096x2160) Quad 4K DCI 29.97Hz",
             NTV2_FORMAT_4x2048x1080p_2997},
            {4096, 2160, 1.0, 30.00, "2160p (4096x2160) Quad 4K DCI 30Hz",
             NTV2_FORMAT_4x2048x1080p_3000},
            {4096, 2160, 1.0, 47.95,
             "2160p (4096x2160) Quad 4K DCI 47.95Hz Level A",
             NTV2_FORMAT_4x2048x1080p_4795},
            {4096, 2160, 1.0, 48.00,
             "2160p (4096x2160) Quad 4K DCI 48Hz Level A",
             NTV2_FORMAT_4x2048x1080p_4800},
            {4096, 2160, 1.0, 50.00,
             "2160p (4096x2160) Quad 4K DCI 50Hz Level A",
             NTV2_FORMAT_4x2048x1080p_5000},
            {4096, 2160, 1.0, 59.94,
             "2160p (4096x2160) Quad 4K DCI 59.94Hz Level A",
             NTV2_FORMAT_4x2048x1080p_5994},
            {4096, 2160, 1.0, 60.00,
             "2160p (4096x2160) Quad 4K DCI 60Hz Level A",
             NTV2_FORMAT_4x2048x1080p_6000},
            {4096, 2160, 1.0, 47.95,
             "2160p (4096x2160) Quad 4K DCI 47.95Hz Level B",
             NTV2_FORMAT_4x2048x1080p_4795_B},
            {4096, 2160, 1.0, 48.00,
             "2160p (4096x2160) Quad 4K DCI 48Hz Level B",
             NTV2_FORMAT_4x2048x1080p_4800_B},
            {4096, 2160, 1.0, 50.00,
             "2160p (4096x2160) Quad 4K DCI 50Hz Level B",
             NTV2_FORMAT_4x2048x1080p_5000_B},
            {4096, 2160, 1.0, 59.94,
             "2160p (4096x2160) Quad 4K DCI 59.94Hz Level B",
             NTV2_FORMAT_4x2048x1080p_5994_B},
            {4096, 2160, 1.0, 60.00,
             "2160p (4096x2160) Quad 4K DCI 60Hz Level B",
             NTV2_FORMAT_4x2048x1080p_6000_B},
            {4096, 2160, 1.0, 119.88, "2160p (4096x2160) Quad 4K DCI 119.88Hz",
             NTV2_FORMAT_4x2048x1080p_11988},
            {4096, 2160, 1.0, 120.00, "2160p (4096x2160) Quad 4K DCI 120Hz",
             NTV2_FORMAT_4x2048x1080p_12000},

            {4096, 2160, 1.0, 23.98, "2160psf (4096x2160) Quad 4K DCI 23.98Hz",
             NTV2_FORMAT_4x2048x1080psf_2398},
            {4096, 2160, 1.0, 24.00, "2160psf (4096x2160) Quad 4K DCI 24Hz",
             NTV2_FORMAT_4x2048x1080psf_2400},
            {4096, 2160, 1.0, 25.00, "2160psf (4096x2160) Quad 4K DCI 25Hz",
             NTV2_FORMAT_4x2048x1080psf_2500},
            {4096, 2160, 1.0, 29.97, "2160psf (4096x2160) Quad 4K DCI 29.97Hz",
             NTV2_FORMAT_4x2048x1080psf_2997},
            {4096, 2160, 1.0, 30.00, "2160psf (4096x2160) Quad 4K DCI 30Hz",
             NTV2_FORMAT_4x2048x1080psf_3000},

            {3840, 2160, 1.0, 23.98, "2160p (3840x2160) 4K UHD 23.98Hz",
             NTV2_FORMAT_3840x2160p_2398},
            {3840, 2160, 1.0, 24.00, "2160p (3840x2160) 4K UHD 24Hz",
             NTV2_FORMAT_3840x2160p_2400},
            {3840, 2160, 1.0, 25.00, "2160p (3840x2160) 4K UHD 25Hz",
             NTV2_FORMAT_3840x2160p_2500},
            {3840, 2160, 1.0, 29.97, "2160p (3840x2160) 4K UHD 29.97Hz",
             NTV2_FORMAT_3840x2160p_2997},
            {3840, 2160, 1.0, 30.00, "2160p (3840x2160) 4K UHD 30Hz",
             NTV2_FORMAT_3840x2160p_3000},
            {3840, 2160, 1.0, 50.00, "2160p (3840x2160) 4K UHD 50Hz Level A",
             NTV2_FORMAT_3840x2160p_5000},
            {3840, 2160, 1.0, 59.94, "2160p (3840x2160) 4K UHD 59.94Hz Level A",
             NTV2_FORMAT_3840x2160p_5994},
            {3840, 2160, 1.0, 60.00, "2160p (3840x2160) 4K UHD 60Hz Level A",
             NTV2_FORMAT_3840x2160p_6000},
            {3840, 2160, 1.0, 50.00, "2160p (3840x2160) 4K UHD 50Hz Level B",
             NTV2_FORMAT_3840x2160p_5000_B},
            {3840, 2160, 1.0, 59.94, "2160p (3840x2160) 4K UHD 59.94Hz Level B",
             NTV2_FORMAT_3840x2160p_5994_B},
            {3840, 2160, 1.0, 60.00, "2160p (3840x2160) 4K UHD 60Hz Level B",
             NTV2_FORMAT_3840x2160p_6000_B},
            {3840, 2160, 1.0, 23.98, "2160psf (3840x2160) 4K UHD 23.98Hz",
             NTV2_FORMAT_3840x2160psf_2398},
            {3840, 2160, 1.0, 24.00, "2160psf (3840x2160) 4K UHD 24Hz",
             NTV2_FORMAT_3840x2160psf_2400},
            {3840, 2160, 1.0, 25.00, "2160psf (3840x2160) 4K UHD 25Hz",
             NTV2_FORMAT_3840x2160psf_2500},
            {3840, 2160, 1.0, 29.97, "2160psf (3840x2160) 4K UHD 29.97Hz",
             NTV2_FORMAT_3840x2160psf_2997},
            {3840, 2160, 1.0, 30.00, "2160psf (3840x2160) 4K UHD 30Hz",
             NTV2_FORMAT_3840x2160psf_3000},

            {4096, 2160, 1.0, 23.98, "2160p (4096x2160) 4K DCI 23.98Hz",
             NTV2_FORMAT_4096x2160p_2398},
            {4096, 2160, 1.0, 24.00, "2160p (4096x2160) 4K DCI 24Hz",
             NTV2_FORMAT_4096x2160p_2400},
            {4096, 2160, 1.0, 25.00, "2160p (4096x2160) 4K DCI 25Hz",
             NTV2_FORMAT_4096x2160p_2500},
            {4096, 2160, 1.0, 29.97, "2160p (4096x2160) 4K DCI 29.97Hz",
             NTV2_FORMAT_4096x2160p_2997},
            {4096, 2160, 1.0, 30.00, "2160p (4096x2160) 4K DCI 30.00Hz",
             NTV2_FORMAT_4096x2160p_3000},
            {4096, 2160, 1.0, 47.95, "2160p (4096x2160) 4K DCI 47.95Hz Level A",
             NTV2_FORMAT_4096x2160p_4795},
            {4096, 2160, 1.0, 48.00, "2160p (4096x2160) 4K DCI 48.00Hz Level A",
             NTV2_FORMAT_4096x2160p_4800},
            {4096, 2160, 1.0, 50.00, "2160p (4096x2160) 4K DCI 50.00Hz Level A",
             NTV2_FORMAT_4096x2160p_5000},
            {4096, 2160, 1.0, 59.94, "2160p (4096x2160) 4K DCI 59.94Hz Level A",
             NTV2_FORMAT_4096x2160p_5994},
            {4096, 2160, 1.0, 60.00, "2160p (4096x2160) 4K DCI 60.00Hz Level A",
             NTV2_FORMAT_4096x2160p_6000},
            {4096, 2160, 1.0, 119.88, "2160p (4096x2160) 4K DCI 119.88Hz",
             NTV2_FORMAT_4096x2160p_11988},
            {4096, 2160, 1.0, 120.00, "2160p (4096x2160) 4K DCI 120Hz",
             NTV2_FORMAT_4096x2160p_12000},
            {4096, 2160, 1.0, 47.95, "2160p (4096x2160) 4K DCI 47.95Hz Level B",
             NTV2_FORMAT_4096x2160p_4795_B},
            {4096, 2160, 1.0, 48.00, "2160p (4096x2160) 4K DCI 48.00Hz Level B",
             NTV2_FORMAT_4096x2160p_4800_B},
            {4096, 2160, 1.0, 50.00, "2160p (4096x2160) 4K DCI 50.00Hz Level B",
             NTV2_FORMAT_4096x2160p_5000_B},
            {4096, 2160, 1.0, 59.94, "2160p (4096x2160) 4K DCI 59.94Hz Level B",
             NTV2_FORMAT_4096x2160p_5994_B},
            {4096, 2160, 1.0, 60.00, "2160p (4096x2160) 4K DCI 60.00Hz Level B",
             NTV2_FORMAT_4096x2160p_6000_B},
            {4096, 2160, 1.0, 23.98, "2160psf (4096x2160) 4K DCI 23.98Hz",
             NTV2_FORMAT_4096x2160psf_2398},
            {4096, 2160, 1.0, 24.00, "2160psf (4096x2160) 4K DCI 24Hz",
             NTV2_FORMAT_4096x2160psf_2400},
            {4096, 2160, 1.0, 25.00, "2160psf (4096x2160) 4K DCI 25Hz",
             NTV2_FORMAT_4096x2160psf_2500},
            {4096, 2160, 1.0, 29.97, "2160psf (4096x2160) 4K DCI 29.97Hz",
             NTV2_FORMAT_4096x2160psf_2997},
            {4096, 2160, 1.0, 30.00, "2160psf (4096x2160) 4K DCI 30.00Hz",
             NTV2_FORMAT_4096x2160psf_3000},

            {7680, 4320, 1.0, 23.98, "4320p (7680x4320) Quad 8K UHD 23.98Hz",
             NTV2_FORMAT_4x3840x2160p_2398},
            {7680, 4320, 1.0, 24.00, "4320p (7680x4320) Quad 8K UHD 24Hz",
             NTV2_FORMAT_4x3840x2160p_2400},
            {7680, 4320, 1.0, 25.00, "4320p (7680x4320) Quad 8K UHD 25Hz",
             NTV2_FORMAT_4x3840x2160p_2500},
            {7680, 4320, 1.0, 29.97, "4320p (7680x4320) Quad 8K UHD 29.97Hz",
             NTV2_FORMAT_4x3840x2160p_2997},
            {7680, 4320, 1.0, 30.00, "4320p (7680x4320) Quad 8K UHD 30Hz",
             NTV2_FORMAT_4x3840x2160p_3000},
            {7680, 4320, 1.0, 50.00,
             "4320p (7680x4320) Quad 8K UHD 50Hz Level A",
             NTV2_FORMAT_4x3840x2160p_5000},
            {7680, 4320, 1.0, 59.94,
             "4320p (7680x4320) Quad 8K UHD 59.94Hz Level A",
             NTV2_FORMAT_4x3840x2160p_5994},
            {7680, 4320, 1.0, 60.00,
             "4320p (7680x4320) Quad 8K UHD 60Hz Level A",
             NTV2_FORMAT_4x3840x2160p_6000},
            {7680, 4320, 1.0, 50.00,
             "4320p (7680x4320) Quad 8K UHD 50Hz Level B",
             NTV2_FORMAT_4x3840x2160p_5000_B},
            {7680, 4320, 1.0, 59.94,
             "4320p (7680x4320) Quad 8K UHD 59.94Hz Level B",
             NTV2_FORMAT_4x3840x2160p_5994_B},
            {7680, 4320, 1.0, 60.00,
             "4320p (7680x4320) Quad 8K UHD 60Hz Level B",
             NTV2_FORMAT_4x3840x2160p_6000_B},

            {8192, 4320, 1.0, 23.98, "4320p (8192x4320) Quad 8K DCI 23.98Hz",
             NTV2_FORMAT_4x4096x2160p_2398},
            {8192, 4320, 1.0, 24.00, "4320p (8192x4320) Quad 8K DCI 24Hz",
             NTV2_FORMAT_4x4096x2160p_2400},
            {8192, 4320, 1.0, 25.00, "4320p (8192x4320) Quad 8K DCI 25Hz",
             NTV2_FORMAT_4x4096x2160p_2500},
            {8192, 4320, 1.0, 29.97, "4320p (8192x4320) Quad 8K DCI 29.97Hz",
             NTV2_FORMAT_4x4096x2160p_2997},
            {8192, 4320, 1.0, 30.00, "4320p (8192x4320) Quad 8K DCI 30Hz",
             NTV2_FORMAT_4x4096x2160p_3000},
            {8192, 4320, 1.0, 47.95,
             "4320p (8192x4320) Quad 8K DCI 47.95Hz Level A",
             NTV2_FORMAT_4x4096x2160p_4795},
            {8192, 4320, 1.0, 48.00,
             "4320p (8192x4320) Quad 8K DCI 48Hz Level A",
             NTV2_FORMAT_4x4096x2160p_4800},
            {8192, 4320, 1.0, 50.00,
             "4320p (8192x4320) Quad 8K DCI 50Hz Level A",
             NTV2_FORMAT_4x4096x2160p_5000},
            {8192, 4320, 1.0, 59.94,
             "4320p (8192x4320) Quad 8K DCI 59.94Hz Level A",
             NTV2_FORMAT_4x4096x2160p_5994},
            {8192, 4320, 1.0, 60.00,
             "4320p (8192x4320) Quad 8K DCI 60Hz Level A",
             NTV2_FORMAT_4x4096x2160p_6000},
            {8192, 4320, 1.0, 47.95,
             "4320p (8192x4320) Quad 8K DCI 47.95Hz Level B",
             NTV2_FORMAT_4x4096x2160p_4795_B},
            {8192, 4320, 1.0, 48.00,
             "4320p (8192x4320) Quad 8K DCI 48Hz Level B",
             NTV2_FORMAT_4x4096x2160p_4800_B},
            {8192, 4320, 1.0, 50.00,
             "4320p (8192x4320) Quad 8K DCI 50Hz Level B",
             NTV2_FORMAT_4x4096x2160p_5000_B},
            {8192, 4320, 1.0, 59.94,
             "4320p (8192x4320) Quad 8K DCI 59.94Hz Level B",
             NTV2_FORMAT_4x4096x2160p_5994_B},
            {8192, 4320, 1.0, 60.00,
             "4320p (8192x4320) Quad 8K DCI 60Hz Level B",
             NTV2_FORMAT_4x4096x2160p_6000_B},

            // End of list
            {0, 0, 0.0, 0.00, "", NTV2_FORMAT_UNKNOWN}};

    } // namespace

    //
    //  KonaVideoDevice
    //

    bool KonaVideoDevice::m_infoFeedback = false;

    KonaVideoDevice::KonaVideoDevice(AJAModule* m, const string& name,
                                     unsigned int deviceIndex,
                                     unsigned int appID, OperationMode mode)
        : GLBindableVideoDevice(m, name,
                                BlockingTransfer | ASyncReadBack | ImageOutput
                                    | ProvidesSync | FixedResolution
                                    | NormalizedCoordinates | FlippedImage
                                    | Clock | AudioOutput)
        , m_appID(appID)
        , m_deviceIndex(deviceIndex)
        , m_card(0)
        , m_operationMode(mode)
        , m_bound(false)
        , m_quad(false)
        , m_quadQuad(false)
        , m_usePausing(false)
        , m_setDesiredFrame(true)
        , m_acquire(true)
        , m_pbos(false)
        ,
#ifdef PLATFORM_DARWIN
        m_immediateCopy(false)
        ,
#else
        m_immediateCopy(true)
        ,
#endif
        m_allowSegmentedTransfer(false)
        , m_simpleRouting(false)
        , m_useHDMIHDRMetadata(false)
        , m_audioFormat(0)
        , m_videoFormat(0)
        , m_dataFormat(0)
        , m_syncMode(0)
        , m_syncSource(0)
        , m_video4KTransport(VIDEO_4K_TRANSPORT_QUADRANTS)
        , m_internalAudioFormat(0)
        , m_internalVideoFormat(0)
        , m_internalDataFormat(0)
        , m_internalSyncMode(0)
        , m_internalVideo4KTransport(VIDEO_4K_TRANSPORT_QUADRANTS)
        , m_internalSyncSource(0)
        , m_width(0)
        , m_height(0)
        , m_channels(0)
        , m_open(false)
        , m_ringBufferSize(DEFAULT_RINGBUFFER_SIZE)
        , m_highwater(m_ringBufferSize - 1)
        , m_paused(false)
        , m_starting(false)
        , m_hdmiOutputBitDepthOverride(0)
        , // 0 = Same as Video Output data format
        m_autocirculateRunning(false)
        , m_readBufferIndex(0)
        , m_readBufferCount(0)
        , m_writeBufferIndex(0)
        , m_writeBufferCount(0)
    {

        //
        //  Query the card on construction so we can find out its
        //  capabilities. The format lists need to be created by the time
        //  the constructor is finished.
        //
        //  NOTES:
        //
        //      * bidirection means the Kona has 4K quad firmware
        //        installed. In this mode the card can be configured as
        //        either four inputs or four outputs but no mixed ins and
        //        outs.
        //
        //      * Dual link is "SDI 1.5G" which allows double the
        //        bandwidth by using two "1G" outputs. "3G" means
        //        dual-link over 1 (3G) output. We need to support 1.5G
        //        output for DCI devices (projectors) and other older
        //        equipement. Not all formats can be output over 1.5G.
        //
        //        another way to look at it is SDI used to be 1.5gigabit for
        //        many years, and most facility routing was 1.5GB. Now a days it
        //        can be 3GB or higher, but there are still many 1.5GB
        //        projectors and routers in the field.
        //
        //        a single 1.5GB stream will carry 1080p YCbCr 4:2:2 10bit at
        //        best. a single 3GB stream will carry 10 or 12 bit RGB 1080p at
        //        best. a single 4k stream is made up of 4 2k streams. hence the
        //        existance of 12G. So, if you need 10bit RGB and you only have
        //        a 1.5GB routing or destination device then you will need 2
        //        wires, AKA dual-link, to get to the 3G you need.
        //
        //      * The NTV2 SDK covers most of AJA's devices including the
        //        mini outputs for thunderbolt so we have look out for
        //        devices which do only mono HD formats for example.
        //
        //      * TSI: As of SDK 14, a new output format is available for 4K
        //      (UHD and DCI):
        //        Two-sample-interleave aka TSI aka 2SI. This affects both the
        //        SDI and HDMI outputs. You then have two options: 1 - Output
        //        "traditional" SDI (in this case each of the 4 outputs carries
        //        a different
        //            quadrant of the original 4K image) -> HDMI out needs to be
        //            downconverted from 4K to HD (this is a limitation imposed
        //            by AJA);
        //
        //        - OR -
        //
        //        2 - Output TSI SDI (in this case each of the 4 outputs carries
        //        a complete image at 1/4 the
        //            resolution, i.e a valid HD-resolution image;
        //            TSI-compatible monitors can take these 4 HD images and
        //            recreate the original 4K source) -> HDMI out can then be a
        //            true 4K image (no need to downconvert).
        //
        //        Again, this affects *only* 4K timings.

        m_card = new CNTV2Card(m_deviceIndex);
        bool ok = m_card->IsOpen();

        if (ok)
        {
            m_deviceID = m_card->GetDeviceID();
            m_bidirectional = NTV2DeviceHasBiDirectionalSDI(m_deviceID);
            m_deviceNumVideoOutputs = NTV2DeviceGetNumVideoOutputs(m_deviceID);
            m_deviceNumVideoChannels =
                NTV2DeviceGetNumVideoChannels(m_deviceID);
            m_deviceHasDualLink = NTV2DeviceCanDoDualLink(m_deviceID);
            m_deviceHas3G = NTV2DeviceCanDo3GOut(m_deviceID, 0);
            m_deviceHDMIVersion = NTV2DeviceGetHDMIVersion(m_deviceID);
            m_deviceHasHDMIStereo = NTV2DeviceCanDoHDMIOutStereo(m_deviceID);
            m_deviceHasCSC = NTV2DeviceCanDoWidget(m_deviceID, NTV2_WgtCSC1);
            m_numHDMIOutputs = NTV2DeviceGetNumHDMIVideoOutputs(m_deviceID);
            m_deviceMaxAudioChannels =
                NTV2DeviceGetMaxAudioChannels(m_deviceID);
            m_deviceHas96kAudio = NTV2DeviceCanDoAudio96K(m_deviceID);

            bool stereo =
                m_deviceNumVideoChannels >= 2 && m_deviceNumVideoOutputs >= 2;
            bool allformats = getenv("TWK_AJA_ALL_FORMATS") != NULL;

            if (m_operationMode == OperationMode::ProMode)
            {
                for (const KonaVideoFormat* p = videoFormats; !p->desc.empty();
                     p++)
                {
                    if (NTV2DeviceCanDoVideoFormat(m_deviceID, p->value))
                    {
                        m_konaVideoFormats.push_back(*p);
                    }
                }

                for (const KonaDataFormat* p = dataFormats; !p->desc.empty();
                     p++)
                {
                    if (NTV2DeviceCanDoFrameBufferFormat(m_deviceID, p->value))
                    {
                        bool stereoFormat =
                            p->desc.find("Stereo") != string::npos;
                        bool dualLink = p->flags & DualLink;
                        bool rgb = p->flags & RGB444;

                        if (stereoFormat && !stereo)
                            continue;
                        if (dualLink && !m_deviceHasDualLink)
                            continue;
                        if (!rgb && !m_deviceHasCSC)
                            continue;
                        m_konaDataFormats.push_back(*p);
                    }
                }

                if (m_konaDataFormats.empty() || allformats || !m_deviceHasCSC)
                {
                    //
                    //  The t-tap can't handle format conversions so it wants
                    //  raw YUV. So in this case we'll add these formats
                    //
                    //  The Kona5-8K has no color space converter so we'll add
                    //  the YUV formats as well to offer more options to the
                    //  user.
                    //

                    for (const KonaDataFormat* p = dataFormatsYUV;
                         !p->desc.empty(); p++)
                    {
                        if (NTV2DeviceCanDoFrameBufferFormat(m_deviceID,
                                                             p->value))
                        {
                            bool stereoFormat =
                                p->desc.find("Stereo") != string::npos;
                            bool dualLink = p->flags & DualLink;

                            if (stereoFormat && !stereo)
                                continue;
                            if (dualLink && !m_deviceHasDualLink)
                                continue;

                            m_konaDataFormats.push_back(*p);
                        }
                    }
                }
            }
            else
            {
                for (const KonaVideoFormat* p = videoFormatsCP;
                     !p->desc.empty(); p++)
                {
                    m_konaVideoFormats.push_back(*p);
                }

                for (const KonaDataFormat* p = dataFormatsCP; !p->desc.empty();
                     p++)
                {
                    m_konaDataFormats.push_back(*p);
                }
            }
        }

        delete m_card;
        m_card = 0;
    }

    KonaVideoDevice::~KonaVideoDevice()
    {
        if (m_open)
            close();
    }

    bool KonaVideoDevice::tsiEnabled() const
    {
        return m_video4KTransport == VIDEO_4K_TRANSPORT_TSI;
    }

    KonaVideoDevice::Resolution KonaVideoDevice::resolution() const
    {
        if (m_open)
            return Resolution(m_width, m_height, 1.0, 1.0);
        const KonaVideoFormat& f = m_actualVideoFormat;
        return Resolution(f.width, f.height, f.pa, 1.0);
    }

    KonaVideoDevice::Offset KonaVideoDevice::offset() const
    {
        return KonaVideoDevice::Offset(0, 0);
    }

    KonaVideoDevice::Timing KonaVideoDevice::timing() const
    {
        if (m_outFormat)
            return Timing(m_outRate);
        const KonaVideoFormat& f = m_actualVideoFormat;
        return KonaVideoDevice::Timing(f.hz);
    }

    KonaVideoDevice::VideoFormat KonaVideoDevice::format() const
    {
        const KonaVideoFormat& f = m_actualVideoFormat;
        return KonaVideoDevice::VideoFormat(f.width, f.height, f.pa, 1.0f, f.hz,
                                            f.desc);
    }

    size_t KonaVideoDevice::width() const
    {
        if (m_open)
            return m_width;
        const KonaVideoFormat& f = m_actualVideoFormat;
        return f.width;
    }

    size_t KonaVideoDevice::height() const
    {
        if (m_open)
            return m_height;
        const KonaVideoFormat& f = m_actualVideoFormat;
        return f.height;
    }

    namespace
    {

        void checkForFalse(bool b, bool infoFeedback, int line)
        {
            if (infoFeedback && !b)
                cout << "ERROR: AJA_CHECK FAILED: at line " << line << endl;
        }

        string mapToEnvVar(string name)
        {
            if (name == "TWK_AJA_HELP")
                return "help";
            if (name == "TWK_AJA_VERBOSE")
                return "verbose";
            if (name == "TWK_AJA_REC601_MATRIX")
                return "rec601";
            if (name == "TWK_AJA_PROFILE")
                return "profile";
            if (name == "TWK_AJA_METHOD")
                return "method";
            if (name == "TWK_AJA_FLIP")
                return "flip";
            if (name == "TWK_AJA_ENABLE_HARDWARE_PAUSE")
                return "enable-hardware-pause";
            if (name == "TWK_AJA_RING_BUFFER_SIZE")
                return "ring-buffer-size";
            if (name == "TWK_AJA_ACQUIRE")
                return "acquire";
            if (name == "TWK_AJA_NO_AQUIRE")
                return "no-acquire";
            if (name == "TWK_AJA_NO_ACQUIRE")
                return "no-acquire";
            if (name == "TWK_AJA_LEVEL_A")
                return "level-a";
            if (name == "TWK_AJA_DISABLE_TASKS")
                return "disable-tasks";
            if (name == "TWK_AJA_HDMI_HDR_METADATA")
                return HDMI_HDR_METADATA_ARG;
            if (name == "TWK_AJA_HDMI_OUTPUT_BIT_DEPTH_OVERRIDE")
                return "hdmi-output-bit-depth-override";
            return "";
        }

    } // namespace

#define AJA_CHECK(T) checkForFalse(T, m_infoFeedback, __LINE__);

    void KonaVideoDevice::queryCard()
    {
        m_actualVideoFormat = videoFormatsCP[0];
        m_actualDataFormat = dataFormatsCP[0];

        if (!m_open)
            m_card = new CNTV2Card(m_deviceIndex);

        m_channelVector.clear();

        for (unsigned int i = NTV2_CHANNEL1;
             i < (NTV2_CHANNEL1 + m_deviceNumVideoChannels); i++)
        {
            bool enabled = false;

            m_card->IsChannelEnabled((NTV2Channel)i, enabled);

            if (enabled)
            {
                ULWord vpidA;
                ULWord vpidB;
                m_card->GetFrameBufferFormat((NTV2Channel)i,
                                             m_actualDataFormat.value);
                m_card->GetVideoFormat(m_actualVideoFormat.value,
                                       (NTV2Channel)i);
                m_card->GetSDIOutVPID(vpidA, vpidB, (NTV2Channel)i);
                m_channelVector.push_back((NTV2Channel)i);
                m_channelVPIDVector.push_back(make_pair(vpidA, vpidB));
            }
        }

        NTV2FrameRate frameRate =
            GetNTV2FrameRateFromVideoFormat(m_actualVideoFormat.value);
        m_actualVideoFormat.width = GetDisplayWidth(m_actualVideoFormat.value);
        m_actualVideoFormat.height =
            GetDisplayHeight(m_actualVideoFormat.value);
        m_actualVideoFormat.hz = GetFramesPerSecond(frameRate);
        m_width = m_actualVideoFormat.width;
        m_height = m_actualVideoFormat.height;
        m_outRate = m_actualVideoFormat.hz;
        m_channels = 1;
        m_actualVideoFormat.desc =
            NTV2VideoFormatToString(m_actualVideoFormat.value);

        if (!IsProgressivePicture(m_actualVideoFormat.value)
            && !IsPSF(m_actualVideoFormat.value))
        {
            m_actualVideoFormat.hz /= 2.0f;
        }

        //
        //  If for some reason the control panel chooses a format
        //  we don't like switch to a similar one that we do.
        //

        switch (m_actualDataFormat.value)
        {
        case NTV2_FBF_8BIT_YCBCR_YUY2:
        case NTV2_FBF_8BIT_DVCPRO:
        case NTV2_FBF_8BIT_YCBCR_420PL3:
        case NTV2_FBF_8BIT_HDV:
        case NTV2_FBF_8BIT_YCBCR:
        case NTV2_FBF_PRORES_DVCPRO:
        case NTV2_FBF_PRORES_HDV:
            m_actualDataFormat.iformat = VideoDevice::Y0CbY1Cr_8_422;
            m_actualDataFormat.value = NTV2_FBF_8BIT_YCBCR;
            break;
        case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
        case NTV2_FBF_10BIT_RGB_PACKED:
        case NTV2_FBF_10BIT_RGB:
            m_actualDataFormat.iformat = VideoDevice::RGB10X2;
            m_actualDataFormat.value = NTV2_FBF_10BIT_RGB;
            break;
        case NTV2_FBF_10BIT_YCBCR_DPX:
        case NTV2_FBF_10BIT_YCBCRA:
        case NTV2_FBF_10BIT_YCBCR:
            m_actualDataFormat.iformat = VideoDevice::YCrCb_AJA_10_422;
            m_actualDataFormat.value = NTV2_FBF_10BIT_YCBCR;
            break;
        case NTV2_FBF_ARGB:
        case NTV2_FBF_RGBA:
            m_actualDataFormat.iformat = VideoDevice::RGBA8;
            m_actualDataFormat.value = NTV2_FBF_ABGR;
            break;
        default:
            break;
        }

        m_actualDataFormat.desc =
            NTV2FrameBufferFormatToString(m_actualDataFormat.value);
        m_yuvInternalFormat =
            m_actualDataFormat.iformat >= VideoDevice::CbY0CrY1_8_422;

        NTV2Standard standard;
        m_card->GetStandard(standard);

        NTV2FormatDescriptor fd(standard, m_actualDataFormat.value);

        m_linePitch = fd.linePitch;

        if (!m_open)
        {
            delete m_card;
            m_card = 0;
        }
    }

    unsigned int KonaVideoDevice::appID() const { return m_appID; }

    void KonaVideoDevice::open(const StringVector& args)
    {
        if (m_open)
            return;

        bool ok = true;
        const bool proMode = m_operationMode == OperationMode::ProMode;

        options_description desc("AJA NTV2 Device Options");

        desc.add_options()("help,h", "Usage Message")("verbose,v", "Verbose")(
            "rec601", "Use Rec.601 Color Matrix (default is Rec.709)")(
            "limiting-broadcast-range",
            "Force limiting to broadcast video range")(
            "limiting-legal-range", "Force limiting to legal SDI range")(
            "limiting-full-range",
            "Force limiting to full SDI range (no limiting)")(
            "profile,p", "Output Debugging Profile (twk_aja_profile_<ID>.dat)")(
            "method,m", value<string>(),
            "Method (ipbo, ppbo, basic)")("flip", "Flip Image Orientation")(
            "hdmi-stereo-mode", value<string>(),
            "HDMI stereo mode (side-by-side, top-and-bottom, or frame-packed)")(
            HDMI_HDR_METADATA_ARG, value<string>(),
            "HDMI HDR Metadata - comma-separated values: "
            "rx,ry,gx,gy,bx,by,wx,wy,minML,maxML,mCLL,mFALL,eotf,smdID")(
            "no-set-desired-frame",
            "Don't set desiredFrame field in transfer struct")(
            "enable-hardware-pause", "Enable hardware pause states")(
            "allow-segmented-transfer", "Enable segmented DMA transfers")(
            "simple-routing", "Omit all but minimal routing requirements")(
            "ring-buffer-size,s",
            value<int>()->default_value(DEFAULT_RINGBUFFER_SIZE),
            "Ring Buffer Size")(
            "acquire",
            "Acquire device from AJA control panel (which is the default)")(
            "no-acquire,n",
            "Do not attempt to acquire device from AJA control panel")(
            "no-aquire", "Deprecated. Please use no-acquire instead")(
            "level-a,a", "Enable Level A timing where possible.")(
            "disable-tasks", "Set card control to 'Disable tasks' mode.")(
            "hdmi-output-bit-depth-override", value<int>()->default_value(0),
            "HDMI Output Bit Depth override - (0=Same as Video Output data "
            "format)");

        variables_map vm;

        try
        {
            store(command_line_parser(args).options(desc).run(), vm);
            store(parse_environment(desc, mapToEnvVar), vm);
            notify(vm);
        }
        catch (std::exception& e)
        {
            cout << "ERROR: AJA_ARGS: " << e.what() << endl;
        }
        catch (...)
        {
            cout << "ERROR: AJA_ARGS: exception" << endl;
        }

        if (vm.count("help") > 0)
        {
            cout << endl << desc << endl;
        }

        m_infoFeedback = vm.count("verbose") > 0;
        m_profile = vm.count("profile") > 0;
        m_usePausing = vm.count("enable-hardware-pause") > 0;
        m_setDesiredFrame = vm.count("no-set-desired-frame") == 0;
        m_allowSegmentedTransfer = vm.count("allow-segmented-transfer") > 0;
        m_simpleRouting = vm.count("simple-routing") > 0;
        m_useHDMIHDRMetadata = vm.count(HDMI_HDR_METADATA_ARG) > 0;
        const bool rec601 = vm.count("rec601") > 0;
        const bool flip = vm.count("flip") > 0;
        const bool useLevelA = vm.count("level-a") > 0;
        const bool useDisableTasks = vm.count("disable-tasks") > 0;

        m_acquire = true;

        // Check for acquire overrides
        // Note: Maintain compatibility with mispelled 'no-aquire' as it was
        // used prior to RV 7.5.1
        if ((vm.count("no-acquire") > 0) || (vm.count("no-aquire") > 0))
        {
            m_acquire = false;
        }
        else if (vm.count("acquire") > 0)
        {
            m_acquire = true;
        }

        if (vm.count("ring-buffer-size"))
        {
            int rc = vm["ring-buffer-size"].as<int>();
            m_ringBufferSize = rc;
            m_highwater = m_ringBufferSize - 1;
            if (m_infoFeedback)
                cout << "INFO: ringbuffer size = " << rc << endl;
        }

        if (vm.count("hdmi-output-bit-depth-override"))
        {
            m_hdmiOutputBitDepthOverride =
                vm["hdmi-output-bit-depth-override"].as<int>();
            if (m_infoFeedback)
            {
                if (m_hdmiOutputBitDepthOverride != 0)
                    cout << "INFO: HDMI Output bit depth = "
                         << m_hdmiOutputBitDepthOverride << endl;
                else
                    cout << "INFO: HDMI Output bit depth will match the Video "
                            "Output "
                            "data format."
                         << endl;
            }
        }

        if (m_useHDMIHDRMetadata)
        {
            parseHDMIHDRMetadata(vm[HDMI_HDR_METADATA_ARG].as<string>());
        }

        if (m_infoFeedback)
        {
            cout << "INFO: simple-routing = " << (int)m_simpleRouting << endl;
        }

        m_card = new CNTV2Card(m_deviceIndex);

        m_open = m_card->IsOpen();

        if (m_open)
        {
            if (m_threadGroup.num_threads() < 1)
                m_threadGroup.add_thread();

            m_deviceID = m_card->GetDeviceID();

            if (m_bidirectional)
            {
                if (m_infoFeedback)
                    cout << "INFO: bidirectional SDI" << endl;
            }
            else
            {
                if (m_infoFeedback)
                    cout << "INFO: unidirectional SDI" << endl;
            }

            const KonaVideoFormat& f =
                m_konaVideoFormats[m_internalVideoFormat];
            const KonaDataFormat& d = m_konaDataFormats[m_internalDataFormat];
            const string& dname = d.desc;
            const bool rgb = d.flags & RGB444;

            if (m_infoFeedback)
                cout << "INFO: KONA video format (" << (int)f.value
                     << ") = " << f.desc << endl;
            if (m_infoFeedback)
                cout << "INFO: KONA data format (" << (int)d.value
                     << ") = " << d.desc << endl;

            if (m_acquire)
            {
                if (m_infoFeedback)
                    cout << "INFO: Kona AcquireStreamForApplication" << endl;

                if (!m_card->AcquireStreamForApplication(
                        appID(), (uint32_t)TwkUtil::processID()))
                {
                    m_open = false;
                    delete m_card;
                    m_card = 0;
                    TWK_THROW_EXC_STREAM(
                        "AJA: Failed to acquire application stream " << name());
                }

                m_card->GetEveryFrameServices(m_taskMode);
                if (useDisableTasks)
                {
                    m_card->SetEveryFrameServices(NTV2_DISABLE_TASKS);
                }
                else
                {
                    m_card->SetEveryFrameServices(NTV2_OEM_TASKS);
                }
            }
            else
            {
                if (m_infoFeedback)
                    cout
                        << "INFO: skipping Kona AcquireStreamForApplication ..."
                        << endl;
            }

            m_stereo = dname.find("Stereo") != string::npos;
            m_3GA = IsVideoFormatA(f.value)
                    || (!IsVideoFormatB(f.value) && useLevelA);
            m_3G = (d.flags & ThreeG) || NTV2_IS_3G_FORMAT(f.value);
            m_3GB = (m_3GA ? false
                           : ((d.flags & ThreeG) || IsVideoFormatB(f.value))
                                 && !IsVideoFormatA(f.value));
            m_dualLink = d.flags & DualLink;
            m_channels = channelsFromFormat(d.value);
            m_quad = NTV2_IS_QUAD_FRAME_FORMAT(f.value);
            m_quadQuad = NTV2_IS_QUAD_QUAD_FORMAT(f.value);
            if (m_quadQuad)
            {
                m_3G = false;
                m_3GA = false;
                m_3GB = false;
            }

            if (m_infoFeedback)
            {
                cout << "INFO: KONA enable 3GA = " << (int)m_3GA << endl;
                cout << "INFO: KONA enable 3G = " << (int)m_3G << endl;
                cout << "INFO: KONA enable 3GB = " << (int)m_3GB << endl;
            }

            NTV2VideoLimiting limiting =
                rgb ? NTV2_VIDEOLIMITING_LEGALSDI
                    : NTV2_VIDEOLIMITING_LEGALBROADCAST;

            if ((d.flags & LegalRangeY) || vm.count("limiting-legal-range") > 0)
            {
                limiting = NTV2_VIDEOLIMITING_LEGALSDI;
                if (m_infoFeedback)
                    cout << "INFO: limiting legal SDI" << endl;
            }

            if ((d.flags & FullRangeY) || vm.count("limiting-full-range") > 0)
            {
                limiting = NTV2_VIDEOLIMITING_OFF;
                if (m_infoFeedback)
                    cout << "INFO: limiting off" << endl;
            }

            if (vm.count("limiting-broadcast-range") > 0)
            {
                limiting = NTV2_VIDEOLIMITING_LEGALBROADCAST;
                if (m_infoFeedback)
                    cout << "INFO: limiting broadcast" << endl;
            }

            if (m_infoFeedback)
                cout << "INFO: limiting = " << limiting << endl;
            m_card->SetVideoLimiting(limiting);

            bool PBOsOK = true;

            if (vm.count("method"))
            {
                string s = vm["method"].as<string>();

                if (s == "ipbo")
                {
                    m_immediateCopy = true;
                }
                else if (s == "ppbo")
                {
                    m_immediateCopy = false;
                }
                else if (s == "basic")
                {
                    PBOsOK = false;
                }
            }

            m_pbos = PBOsOK;

            if (m_pbos && m_immediateCopy)
                cout << "INFO: using PBOs with immediate copy" << endl;
            else if (m_pbos && !m_immediateCopy)
                cout << "INFO: using PBOs with pointer exchange" << endl;
            else
                cout << "INFO: using basic readback" << endl;

            //
            //  Set up routing according to the format. There are a three
            //  major cases. Glossary:
            //
            //      channel     An graph which starts with a unique FB
            //      FB          framebuffer on card in NTV2FrameBufferFormat
            //      CC          color converter component (bidirectional RGB <->
            //      YUV) DL          dual link converter SDI         SDI output
            //      3G SDI      one channel dual link over one SDI spigot
            //      SDI x 2     one channel dual link over two SDI spigots
            //      2 x SDI     two independent SDI channels
            //
            //  I think you could have 2 x 3G SDI as well, but its unknown
            //  what would use that. The usual older Kona 3G can do 2
            //  independent channels of output over 2 SDI spigots. The new
            //  firmware allows it to use the 2 existing inputs as 2 new
            //  outputs for a total of 4 independent outputs. This is how
            //  4K is done.
            //
            //  Any YUV 2K HD or smaller is:
            //
            //      FB -> CC -> SDI
            //
            //  For high bandwidth formats the card will automatically use
            //  a 3G SDI output.
            //
            //  (This seems to be not longer true.  If the output does not have
            //  3G/3Gb enabled, it does not automatically happen.  So we set it
            //  explicitly below, with SetSDIOut3GEnable and SetSDIOut3GbEnable.
            //  --alan)
            //
            //  Any RGB HD is dual link 3G (SDI x 2) on
            //  1 spigot, or non-3G over 2 spigots:
            //
            //      FB -> DL => 3G SDI
            //      FB -> DL => SDI x 2
            //
            //  But we're sticking with just 3G for the moment. Finally
            //  for larger bandwidth, the two spigot approach will
            //  automatically switch to 3G SDI x 2.
            //
            //  Stereo is done using 2 spigots with independent channels:
            //
            //      FB1 -> CC1 -> SDI1 (yuv)
            //      FB2 -> CC2 -> SDI2 (yuv)
            //          -or-
            //      FB1 -> DL => 3G SDI1 (rgb)
            //      FB2 -> DL => 3G SDI2 (rgb)
            //
            //  When using independent channels, its necessary to tie the
            //  channels together so that they are synced.
            //

            size_t numFrameStores = NTV2DeviceGetNumFrameStores(m_deviceID);
            if (m_infoFeedback)
                cout << "INFO: numFrameStores = " << numFrameStores << endl;

            if (proMode)
            {
                //
                //  PRO Mode -- we set up routing. This allows additional
                //  output modes (stereo) not covered by the control
                //  panel.
                //
                m_card->ClearRouting();

                //
                //  Make sure all the frame stores are available for use
                //
                m_card->EnableChannel(NTV2_CHANNEL1);
                if (numFrameStores >= 2)
                    m_card->EnableChannel(NTV2_CHANNEL2);

                if (tsiEnabled())
                {
                    if (numFrameStores >= 3)
                        m_card->DisableChannel(NTV2_CHANNEL3);
                    if (numFrameStores >= 4)
                        m_card->DisableChannel(NTV2_CHANNEL4);
                }
                else
                {
                    if (numFrameStores >= 3)
                        m_card->EnableChannel(NTV2_CHANNEL3);
                    if (numFrameStores >= 4)
                        m_card->EnableChannel(NTV2_CHANNEL4);
                }

                m_actualDataFormat = d;
                m_actualVideoFormat = f;

                m_yuvInternalFormat = d.iformat >= VideoDevice::CbY0CrY1_8_422;

                AJA_CHECK(m_card->SetVideoFormat(f.value));

                AJA_CHECK(m_card->SetFrameBufferFormat(NTV2_CHANNEL1, d.value));
                AJA_CHECK(m_card->SetFrameBufferFormat(NTV2_CHANNEL2, d.value));

                if (m_quad || m_quadQuad)
                {
                    AJA_CHECK(
                        m_card->SetFrameBufferFormat(NTV2_CHANNEL3, d.value));
                    AJA_CHECK(
                        m_card->SetFrameBufferFormat(NTV2_CHANNEL4, d.value));
                }

                if (m_quad || m_quadQuad)
                    m_channelVector.resize(4);
                else if (m_stereo)
                    m_channelVector.resize(2);
                else
                    m_channelVector.resize(1);

                for (unsigned int i = 0; i < m_channelVector.size(); i++)
                {
                    m_channelVector[i] = (NTV2Channel)(i + NTV2_CHANNEL1);
                }

                AJA_CHECK(m_card->SetReference(NTV2_REFERENCE_FREERUN));

                NTV2VANCMode vancMode;
                AJA_CHECK(m_card->GetVANCMode(vancMode));

                bool progressive =
                    NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(f.value)
                    && !NTV2_IS_PSF_VIDEO_FORMAT(f.value);

                //
                // standard needs to overriden to 1080p or 1080.
                //
                NTV2Standard standard = GetNTV2StandardFromVideoFormat(f.value);

                if (m_infoFeedback)
                    cout << "INFO: Card STANDARD request: " << (int)standard
                         << endl;
                if (standard != NTV2_STANDARD_720
                    && !NTV2_IS_SD_STANDARD(standard))
                {
                    if (progressive)
                    {
                        standard = NTV2_STANDARD_1080p;
                    }
                    else
                    {
                        standard = NTV2_STANDARD_1080;
                    }
                }

                m_card->SetSDITransmitEnable(NTV2_CHANNEL1, true);
                m_card->SetSDITransmitEnable(NTV2_CHANNEL2, true);

                if (m_quad || m_quadQuad)
                {
                    m_card->SetSDITransmitEnable(NTV2_CHANNEL3, true);
                    m_card->SetSDITransmitEnable(NTV2_CHANNEL4, true);
                }

                const bool needRGBLevelAConversion = (rgb & m_3GA);

                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL1,
                                                     needRGBLevelAConversion);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL2,
                                                     needRGBLevelAConversion);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL3,
                                                     needRGBLevelAConversion);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL4,
                                                     needRGBLevelAConversion);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL5,
                                                     needRGBLevelAConversion);

                if (m_infoFeedback)
                {
                    bool isLevelA = false;
                    m_card->GetSDIOutRGBLevelAConversion(NTV2_CHANNEL1,
                                                         isLevelA);
                    cout << "INFO: Level A status = " << (int)isLevelA << endl;
                }

                m_card->SetSDIOut3GEnable(NTV2_CHANNEL1, m_3G);
                m_card->SetSDIOut3GbEnable(NTV2_CHANNEL1, m_3GB);

                m_card->SetSDIOut3GEnable(NTV2_CHANNEL2, m_3G);
                m_card->SetSDIOut3GbEnable(NTV2_CHANNEL2, m_3GB);

                m_card->SetSDIOut3GEnable(NTV2_CHANNEL3, m_3G);
                m_card->SetSDIOut3GbEnable(NTV2_CHANNEL3, m_3GB);

                m_card->SetSDIOut3GEnable(NTV2_CHANNEL4, m_3G);
                m_card->SetSDIOut3GbEnable(NTV2_CHANNEL4, m_3GB);

                m_card->SetSDIOut3GEnable(NTV2_CHANNEL5, m_3G);
                m_card->SetSDIOut3GbEnable(NTV2_CHANNEL5, m_3GB);

                // SB - are there 3G dual link formats? duallink is 1.5g only?
                m_card->SetDualLinkOutputEnable(m_dualLink);

                //
                //  NOTE: we were told at one point not to set the
                //  orientation. Now it looks like we have to in order to
                //  get 4K modes to work.
                //

                m_card->SetFrameBufferOrientation(
                    NTV2_CHANNEL1, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
                m_card->SetFrameBufferOrientation(
                    NTV2_CHANNEL2, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
                m_card->SetFrameBufferOrientation(
                    NTV2_CHANNEL3, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
                m_card->SetFrameBufferOrientation(
                    NTV2_CHANNEL4, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);

                m_card->SetTsiFrameEnable(false, NTV2_CHANNEL1);
                m_card->SetTsiFrameEnable(false, NTV2_CHANNEL2);
                m_card->SetTsiFrameEnable(false, NTV2_CHANNEL3);
                m_card->SetTsiFrameEnable(false, NTV2_CHANNEL4);
                m_card->SetTsiFrameEnable(false, NTV2_CHANNEL5);

                if (rgb)
                {
                    if (m_quad || m_quadQuad)
                    {
                        routeQuadRGB(standard, f, d);
                    }
                    else if (m_stereo)
                    {
                        routeStereoRGB(standard, f, d);
                    }
                    else
                    {
                        routeMonoRGB(standard, f, d);
                    }
                }
                else if (m_quad || m_quadQuad)
                {
                    routeQuadYUV(standard, f, d);
                }
                else if (m_stereo)
                {
                    routeStereoYUV(standard, f, d);
                }
                else // YUV mono
                {
                    routeMonoYUV(standard, f, d);
                }

                m_card->GetStandard(standard);
                if (m_infoFeedback)
                    cout << "INFO: Card STANDARD " << (int)standard << endl;

                m_card->SetReference(NTV2_REFERENCE_INPUT1);

                if (!rgb)
                {
                    //
                    //  Set color space (state on CSC "node", might use 2 for
                    //  stereo)
                    //

                    NTV2ColorSpaceMatrixType m =
                        rec601 ? NTV2_Rec601Matrix : NTV2_Rec709Matrix;

                    AJA_CHECK(
                        m_card->SetColorSpaceMatrixSelect(m, NTV2_CHANNEL1));
                    AJA_CHECK(
                        m_card->SetColorSpaceMatrixSelect(m, NTV2_CHANNEL2));

                    if (m_quad || m_quadQuad)
                    {
                        AJA_CHECK(m_card->SetColorSpaceMatrixSelect(
                            m, NTV2_CHANNEL3));
                        AJA_CHECK(m_card->SetColorSpaceMatrixSelect(
                            m, NTV2_CHANNEL4));
                    }

                    //
                    //  Set color range to FULL, not SMPTE (state on CSC "node",
                    //  might use 2 for stereo)
                    //
                    //  NOTE: this is refering to what the hardware expects
                    //  the *input* range to be not the output.
                    //

                    NTV2RGBBlackRange range = NTV2_RGBBLACKRANGE_0_0x3FF;

                    AJA_CHECK(m_card->SetColorSpaceRGBBlackRange(
                        range, NTV2_CHANNEL1));
                    AJA_CHECK(m_card->SetColorSpaceRGBBlackRange(
                        range, NTV2_CHANNEL2));

                    if (m_quad || m_quadQuad)
                    {
                        AJA_CHECK(m_card->SetColorSpaceRGBBlackRange(
                            range, NTV2_CHANNEL3));
                        AJA_CHECK(m_card->SetColorSpaceRGBBlackRange(
                            range, NTV2_CHANNEL4));
                    }
                }

                //
                //  Audio and Video Buffer Setup
                //

                NTV2FormatDescriptor fd(standard, d.value);

                m_width = f.width;
                m_height = f.height;
                m_linePitch = fd.linePitch;
                m_outRate = (int)f.hz;
                m_outFormat = 0;
            }
            else
            {
                queryCard();

                m_stereo = m_channelVector.size() == 2;
                m_quad = NTV2_IS_QUAD_FRAME_FORMAT(m_actualVideoFormat.value);
                m_quadQuad =
                    NTV2_IS_QUAD_QUAD_FORMAT(m_actualVideoFormat.value);

                for (size_t i = 0; i < m_channelVector.size(); i++)
                {
                    m_card->SetFrameBufferFormat(m_channelVector[i],
                                                 m_actualDataFormat.value);
                    m_card->SetSDIOutVPID(m_channelVPIDVector[i].first,
                                          m_channelVPIDVector[i].second,
                                          (NTV2Channel)m_channelVector[i]);

                    //
                    //  NOTE: we were told at one point not to set the
                    //  orientation. Now it looks like we have to in order to
                    //  get 4K modes to work.
                    //

                    m_card->SetFrameBufferOrientation(
                        m_channelVector[i],
                        NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
                }
            }

            m_bufferSize = m_width * m_height * m_channels;
            m_bufferSizeInBytes =
                m_width * m_height
                * pixelSizeInBytes(m_actualDataFormat.iformat);
            m_bufferStride =
                m_width * pixelSizeInBytes(m_actualDataFormat.iformat);

            if (m_infoFeedback)
                cout << "INFO: " << m_width << "x" << m_height << "@"
                     << m_outRate << ", "
                     << NTV2VideoFormatToString(m_actualVideoFormat.value)
                     << ", "
                     << NTV2FrameBufferFormatToString(m_actualDataFormat.value)
                     << endl;

            m_fboInternalFormat = TwkGLF::internalFormatFromDataFormat(
                m_actualDataFormat.iformat);
            GLenumPair epair =
                TwkGLF::textureFormatFromDataFormat(m_actualDataFormat.iformat);
            m_textureFormat = epair.first;
            m_textureType = epair.second;
            m_texturePadding = 0;
            if (m_textureType == GL_UNSIGNED_INT_10_10_10_2)
                m_textureType = GL_UNSIGNED_INT_2_10_10_10_REV;

            m_videoChannels.clear();

            m_videoChannels.push_back(
                new VideoChannel(m_actualDataFormat.value, m_channelVector[0],
                                 m_bufferSizeInBytes, m_ringBufferSize));

            if (m_stereo)
            {
                m_videoChannels.push_back(new VideoChannel(
                    m_actualDataFormat.value, m_channelVector[1],
                    m_bufferSizeInBytes, m_ringBufferSize));
            }

            ULWord u372;
            m_card->GetSmpte372(u372);
            //  cerr << "INFO: KONA u372 support: " << u372 << endl;

            for (size_t i = 0; i < m_videoChannels.size(); i++)
            {
                VideoChannel* vc = m_videoChannels[i];
                ULWord nch = (ULWord)audioFormatChannelCountForCard();

                m_card->SetNumberAudioChannels(nch,
                                               NTV2AudioSystem(vc->channel));

                if (m_deviceHas96kAudio
                    && audioFormats[m_internalAudioFormat].hz == 96000)
                {
                    m_card->SetAudioRate(NTV2_AUDIO_96K,
                                         NTV2AudioSystem(vc->channel));
                }
                else
                {
                    m_card->SetAudioRate(NTV2_AUDIO_48K,
                                         NTV2AudioSystem(vc->channel));
                }

                m_card->SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG,
                                           NTV2AudioSystem(vc->channel));

                m_card->SetSDIOutputAudioSystem(vc->channel,
                                                NTV2AudioSystem(vc->channel));
                m_card->SetSDIOutputDS2AudioSystem(
                    vc->channel, NTV2AudioSystem(vc->channel));

                //  Following comments below taken from AJA demo player app.
                //	If the last app using the device left it in end-to-end
                // mode (input
                // passthru), 	then loopback must be disabled, or else the
                // output will contain whatever audio 	is present in whatever
                // signal is feeding the device's SDI input...
                m_card->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF,
                                         NTV2AudioSystem(vc->channel));

                // AUTOCIRCULATE_TRANSFER_STRUCT& s = vc->transfer;
                // s.bDisableExtraAudioInfo         = true;
                // s.frameBufferFormat              = m_actualDataFormat.value;
                // s.audioNumChannels               = nch;

                // cout << dec << "**** SETTING BUFFERFORMAT to: " <<
                // m_actualDataFormat.value  << " at line " << __LINE__ << endl;
                vc->auto_transfer.SetFrameBufferFormat(
                    m_actualDataFormat.value);
            }

            if (m_autocirculateRunning)
            {
                for (size_t i = 0; i < m_videoChannels.size(); i++)
                {
                    m_card->AutoCirculateStop(m_videoChannels[i]->channel);
                }

                m_autocirculateRunning = false;
            }

            for (size_t i = 0; i < m_videoChannels.size(); i++)
            {
                VideoChannel* vc = m_videoChannels[i];

                //
                //  NOTE: in the stereo case, the start and end frames of
                //  the autocirculate buffer cannot overlap because its
                //  allocated as one contiguous space in the driver.
                //

                // ok = m_card->InitAutoCirculate(vc->transfer.channelSpec, //
                // which output
                //                                i * m_ringBufferSize, // start
                //                                internal frame (i+1) *
                //                                m_ringBufferSize - 1,// end
                //                                internal frame 1,           //
                //                                must be 1
                //                                (NTV2AudioSystem)(NTV2_AUDIOSYSTEM_1
                //                                + i), // ?? as opposed to 2-4
                //                                hasAudioOutput(), // audio or
                //                                not false,    // RP188
                //                                timecode false,    // fb
                //                                format will change during
                //                                playback false,    // handle
                //                                orientation changes false, //
                //                                use color correction false, //
                //                                use vid proc false,    // with
                //                                custom ANC data false);   //
                //                                LTC

                ok = m_card->AutoCirculateInitForOutput(
                    vc->channel, m_ringBufferSize,
                    audioOutputEnabled()
                        ? (NTV2AudioSystem)(NTV2_AUDIOSYSTEM_1 + i)
                        : NTV2_AUDIOSYSTEM_INVALID, // ?? as opposed to 2-4
                    0,                              // option flags (e.g.,
                       // AUTOCIRCULATE_WITH_RP188,AUTOCIRCULATE_WITH_LTC,
                       // AUTOCIRCULATE_WITH_ANC, etc.)
                    1,                             // in num channels
                    i * m_ringBufferSize,          // start frame
                    (i + 1) * m_ringBufferSize - 1 // end frame
                );

                if (!ok && m_infoFeedback)
                {
                    cout << "ERROR: AutoCirculateInitForOutput #" << i
                         << " failed " << vc->channel << endl;
                }
            }
            if (ok)
            {
                AJA_CHECK(m_card->AutoCirculateFlush(NTV2_CHANNEL1));

                if (m_stereo)
                {
                    AJA_CHECK(m_card->AutoCirculateFlush(NTV2_CHANNEL2));
                }
            }
        }
        else
        {
            ok = false;
        }

        if (!ok)
        {
            m_card->AutoCirculateStop(NTV2_CHANNEL1);
            m_card->AutoCirculateStop(NTV2_CHANNEL2);

            if (proMode)
            {
                m_card->ClearRouting();
            }

            for (size_t i = 0; i < m_videoChannels.size(); i++)
            {
                VideoChannel* vc = m_videoChannels[i];
                delete vc;
            }

            m_videoChannels.clear();
            delete m_card;
            m_card = 0;
            m_open = false;
            TWK_THROW_EXC_STREAM("Failed to open AJA video device " << name());
        }
    }

    void KonaVideoDevice::routeQuadRGB(NTV2Standard standard,
                                       const KonaVideoFormat& f,
                                       const KonaDataFormat& d)
    {
        if (m_infoFeedback)
            cout << "INFO: KONA quad 4K RGB format" << endl;

        ULWord vpidA;
        ULWord vpidB;
        ULWord vpidC;
        ULWord vpidD;

        CNTV2VPID::SetVPIDData(vpidA, f.value, d.value, false, false,
                               VPIDChannel_1);
        CNTV2VPID::SetVPIDData(vpidB, f.value, d.value, false, false,
                               VPIDChannel_2);
        CNTV2VPID::SetVPIDData(vpidC, f.value, d.value, false, false,
                               VPIDChannel_3);
        CNTV2VPID::SetVPIDData(vpidD, f.value, d.value, false, false,
                               VPIDChannel_4);

        m_card->SetSDIOutputStandard(NTV2_CHANNEL1, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL2, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL3, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL4, standard);

        m_card->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);

        // Enable the requested quad (4K) or quad-quad (8K) configuration
        if (m_quadQuad)
        {
            // Enable quad-quad-frame (8K) squares mode on the device.
            m_card->SetQuadQuadSquaresEnable(true);
        }
        else
        {
            // Enable quad-frame (4K) squares mode on the device.
            m_card->Set4kSquaresEnable(true, NTV2_CHANNEL1);
        }

        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL2);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL3);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL4);

        m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL1);
        m_card->SetSDIOutVPID(vpidB, 0, NTV2_CHANNEL2);
        m_card->SetSDIOutVPID(vpidC, 0, NTV2_CHANNEL3);
        m_card->SetSDIOutVPID(vpidD, 0, NTV2_CHANNEL4);

        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL3, NTV2_AUDIOSYSTEM_3);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL4, NTV2_AUDIOSYSTEM_4);

        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);
        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL3, NTV2_AUDIOSYSTEM_3);
        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL4, NTV2_AUDIOSYSTEM_4);

        if (m_quadQuad)
        {
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL1);
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL2);
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL3);
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL4);
        }
        else
        {
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL1);
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL2);
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL3);
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL4);
        }

        //      (1) FB1 -> DL -> SDI1
        //      (2) FB2 -> DL -> SDI2
        //      (3) FB3 -> DL -> SDI3
        //      (4) FB4 -> DL -> SDI4

        routeMux(tsiEnabled());
        routeCSC(tsiEnabled(), true);

        if (tsiEnabled())
        {
            m_card->Connect(NTV2_XptDualLinkOut1Input, NTV2_Xpt425Mux1ARGB);
            m_card->Connect(NTV2_XptDualLinkOut2Input, NTV2_Xpt425Mux1BRGB);
            m_card->Connect(NTV2_XptDualLinkOut3Input, NTV2_Xpt425Mux2ARGB);
            m_card->Connect(NTV2_XptDualLinkOut4Input, NTV2_Xpt425Mux2BRGB);

            m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
            m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut2);
            m_card->Connect(NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);
            m_card->Connect(NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut4);

            m_card->Connect(NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
            m_card->Connect(NTV2_XptSDIOut2InputDS2, NTV2_XptDuallinkOut2DS2);
            m_card->Connect(NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
            m_card->Connect(NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
        }
        else
        {
            m_card->Connect(NTV2_XptDualLinkOut1Input, NTV2_XptFrameBuffer1RGB);
            m_card->Connect(NTV2_XptDualLinkOut2Input, NTV2_XptFrameBuffer2RGB);
            m_card->Connect(NTV2_XptDualLinkOut3Input, NTV2_XptFrameBuffer3RGB);
            m_card->Connect(NTV2_XptDualLinkOut4Input, NTV2_XptFrameBuffer4RGB);

            m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
            m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut2);
            m_card->Connect(NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);
            m_card->Connect(NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut4);

            m_card->Connect(NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
            m_card->Connect(NTV2_XptSDIOut2InputDS2, NTV2_XptDuallinkOut2DS2);
            m_card->Connect(NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
            m_card->Connect(NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
        }

        route4KDownConverter(tsiEnabled(), true);
        routeMonitorOut(tsiEnabled(), true);
        routeHDMI(standard, d, tsiEnabled(), true);
    }

    void KonaVideoDevice::routeStereoRGB(NTV2Standard standard,
                                         const KonaVideoFormat& f,
                                         const KonaDataFormat& d)
    {
        if (m_infoFeedback)
            cout << "INFO: KONA stereo RGB format" << endl;

        ULWord vpidA;
        ULWord vpidB;

        CNTV2VPID::SetVPIDData(vpidA, f.value, d.value, false, false,
                               VPIDChannel_1);
        CNTV2VPID::SetVPIDData(vpidB, f.value, d.value, false, false,
                               VPIDChannel_2);

        m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL1);
        m_card->SetSDIOutVPID(vpidB, 0, NTV2_CHANNEL2);

        m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
        m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);

        m_card->SetSDIOutputStandard(NTV2_CHANNEL1, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL2, standard);

        m_card->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);

        m_card->Set4kSquaresEnable(false, NTV2_CHANNEL1);

        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL2);

        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);

        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);

        m_card->Connect(NTV2_XptDualLinkOut1Input, NTV2_XptFrameBuffer1RGB);
        m_card->Connect(NTV2_XptDualLinkOut2Input, NTV2_XptFrameBuffer2RGB);

        m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
        m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut2);

        m_card->Connect(NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
        m_card->Connect(NTV2_XptSDIOut2InputDS2, NTV2_XptDuallinkOut2DS2);

        if (m_deviceNumVideoOutputs > 2)
        {
            m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
            m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);
        }

        if (m_deviceNumVideoOutputs > 4)
        {
            m_card->SetSDITransmitEnable(NTV2_CHANNEL5, true);
            m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL5);
            m_card->SetSDIOutputStandard(NTV2_CHANNEL5, standard);
            m_card->SetMode(NTV2_CHANNEL5, NTV2_MODE_DISPLAY);
            m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL5);
            m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL5, NTV2_AUDIOSYSTEM_5);
        }
    }

    void KonaVideoDevice::routeMonoRGB(NTV2Standard standard,
                                       const KonaVideoFormat& f,
                                       const KonaDataFormat& d)
    {
        if (m_infoFeedback)
            cout << "INFO: KONA mono RGB format" << endl;

        ULWord vpidA;
        ULWord vpidB;

        CNTV2VPID::SetVPIDData(vpidA, f.value, d.value, false, false,
                               VPIDChannel_1);
        CNTV2VPID::SetVPIDData(vpidB, f.value, d.value, false, false,
                               VPIDChannel_2);
        m_card->SetSDIOutVPID(vpidA, vpidB, NTV2_CHANNEL1);

        m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
        m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);

        m_card->SetSDIOutputStandard(NTV2_CHANNEL1, standard);

        m_card->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY);

        m_card->Set4kSquaresEnable(false, NTV2_CHANNEL1);

        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL1);

        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);

        if (m_deviceNumVideoOutputs > 1)
        {
            m_card->SetSDIOutVPID(vpidA, vpidB, NTV2_CHANNEL2);
            m_card->SetSDIOutputStandard(NTV2_CHANNEL2, standard);
            m_card->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
            m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL2);
            m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);
            m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL2,
                                               NTV2_AUDIOSYSTEM_2);
            m_card->SetSDITransmitEnable(NTV2_CHANNEL2, true);

            if (m_deviceNumVideoOutputs > 2)
            {
                m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
                m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);
            }

            if (m_deviceNumVideoOutputs > 4)
            {
                m_card->SetSDIOutVPID(vpidA, vpidB, NTV2_CHANNEL5);
                m_card->SetSDIOutputStandard(NTV2_CHANNEL5, standard);
                m_card->SetMode(NTV2_CHANNEL5, NTV2_MODE_DISPLAY);
                m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL5);
                m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL5,
                                                NTV2_AUDIOSYSTEM_5);
                m_card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL5,
                                                   NTV2_AUDIOSYSTEM_5);
                m_card->SetSDITransmitEnable(NTV2_CHANNEL5, true);
            }
        }

        if (m_numHDMIOutputs > 0 && !m_simpleRouting)
        {
            AJA_CHECK(m_card->SetHDMIOutVideoStandard(standard));
            m_card->Connect(NTV2_XptHDMIOutInput, NTV2_XptFrameBuffer1RGB);
            m_card->SetHDMIOutColorSpace(NTV2_HDMIColorSpaceRGB);
            m_card->SetHDMIOutBitDepth(getHDMIOutBitDepth(d.value));
            setHDMIHDRMetadata();
        }

        //
        //  NOTE: Some firmware on the Kona 4 is missing
        //  the connection from DL1 to SDI2. Normally this
        //  results in a bad signal at SDI2, but one user
        //  claims they get a bad signal at SDI1 as well.
        //
        //      FB -> DL1 => 3G SDI1
        //             +  => 3G SDI2 (identical to SDI1)
        //
        //      -or for dual link-
        //
        //      FB -> DL1 -> SDI1
        //             +  -> SDI2
        //

        m_card->Connect(NTV2_XptDualLinkOut1Input, NTV2_XptFrameBuffer1RGB);
        m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);

        if (m_dualLink)
        {
            m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut1DS2);
        }
        else
        {
            m_card->Connect(NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);

            if (!m_simpleRouting)
            {
                //
                //  Additional SDI2 output of the same 3G signal
                //
                m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut2);
                m_card->Connect(NTV2_XptSDIOut2InputDS2,
                                NTV2_XptDuallinkOut2DS2);
            }
        }

        if (m_deviceNumVideoOutputs > 4)
        {
            //
            //  Make SDI5 monitor 3G regardless of whether
            //  or not dual link was selected above
            //
            m_card->SetSDIOut3GEnable(NTV2_CHANNEL5, true);
            m_card->SetSDIOut3GbEnable(NTV2_CHANNEL5, true);
            m_card->Connect(NTV2_XptSDIOut5Input, NTV2_XptDuallinkOut1);
            m_card->Connect(NTV2_XptSDIOut5InputDS2, NTV2_XptDuallinkOut1DS2);
        }
    }

    void KonaVideoDevice::routeQuadYUV(NTV2Standard standard,
                                       const KonaVideoFormat& f,
                                       const KonaDataFormat& d)
    {
        if (m_infoFeedback)
            cout << "INFO: KONA quad 4K non-RGB format" << endl;

        ULWord vpidA;
        ULWord vpidB;
        ULWord vpidC;
        ULWord vpidD;

        CNTV2VPID::SetVPIDData(vpidA, f.value, d.value, false, false,
                               VPIDChannel_1);
        CNTV2VPID::SetVPIDData(vpidB, f.value, d.value, false, false,
                               VPIDChannel_2);
        CNTV2VPID::SetVPIDData(vpidC, f.value, d.value, false, false,
                               VPIDChannel_3);
        CNTV2VPID::SetVPIDData(vpidD, f.value, d.value, false, false,
                               VPIDChannel_4);

        m_card->SetSDIOutputStandard(NTV2_CHANNEL1, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL2, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL3, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL4, standard);

        m_card->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);

        // Enable the requested quad (4K) or quad-quad (8K) configuration
        if (m_quadQuad)
        {
            // Enable quad-quad-frame (8K) squares mode on the device.
            m_card->SetQuadQuadSquaresEnable(true);
        }
        else
        {
            // Enable quad-frame (4K) squares mode on the device.
            m_card->Set4kSquaresEnable(true, NTV2_CHANNEL1);
        }

        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL2);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL3);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL4);

        m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL1);
        m_card->SetSDIOutVPID(vpidB, 0, NTV2_CHANNEL2);
        m_card->SetSDIOutVPID(vpidC, 0, NTV2_CHANNEL3);
        m_card->SetSDIOutVPID(vpidD, 0, NTV2_CHANNEL4);

        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL3, NTV2_AUDIOSYSTEM_3);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL4, NTV2_AUDIOSYSTEM_4);

        if (m_quadQuad)
        {
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL1);
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL2);
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL3);
            m_card->SetQuadQuadSquaresEnable(!tsiEnabled(), NTV2_CHANNEL4);
        }
        else
        {
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL1);
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL2);
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL3);
            m_card->SetTsiFrameEnable(tsiEnabled(), NTV2_CHANNEL4);
        }

        //      (1) FB1 -> CC1 -> SDI1
        //      (2) FB2 -> CC2 -> SDI2
        //      (3) FB3 -> CC3 -> SDI3
        //      (4) FB4 -> CC4 -> SDI4

        routeMux(tsiEnabled());
        routeCSC(tsiEnabled(), false);

        if (m_deviceHasCSC)
        {
            m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
            m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptCSC2VidYUV);
            m_card->Connect(NTV2_XptSDIOut3Input, NTV2_XptCSC3VidYUV);
            m_card->Connect(NTV2_XptSDIOut4Input, NTV2_XptCSC4VidYUV);
        }
        else
        {
            m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
            m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2YUV);
            m_card->Connect(NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer3YUV);
            m_card->Connect(NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer4YUV);
        }

        route4KDownConverter(tsiEnabled(), false);
        routeMonitorOut(tsiEnabled(), false);
        routeHDMI(standard, d, tsiEnabled(), false);
    }

    void KonaVideoDevice::routeMonitorOut(bool tsiEnabled, bool outputIsRGB)
    {
        // Monitor Out (aka SDI5).
        if (::NTV2DeviceGetNumVideoOutputs(m_deviceID) < 5)
            return;

        // SDI5, being a 3G output, cannot take 4K directly.
        // In Quadrant mode (i.e. non-tsi), we can use the output of the
        // 4k Downconverter.
        // In TSI mode however, we can't use the 4K Downconverter (AJA
        // limitation, the 4KDC doesn't understand tsi). Instead we
        // connect directly to one of the TSI outputs (since each
        // output is a valide HD signal). The downside is that it doesn't
        // look as good as a properly downconverted 4K (since tsi simply
        // takes 1 out of every 4 pixels).

        if (tsiEnabled)
        {
            // FB(RGB) -> MUX(RGB) -> SDI5
            if (outputIsRGB)
            {
                m_card->Connect(NTV2_XptSDIOut5Input, NTV2_Xpt425Mux1ARGB);
            }
            else
            {
                // FB(RGB) -> MUX(RGB) -> CSC(YUV) -> SDI5
                m_card->Connect(NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
            }
        }
        else if (::NTV2DeviceCanDoWidget(m_deviceID, NTV2_Wgt4KDownConverter))
        {
            if (outputIsRGB)
            {
                // FB(RGB) -> CSC(YUV) -> 4KDC(YUV) -> DL5 -> SDI5
                m_card->Connect(NTV2_XptDualLinkOut5Input,
                                NTV2_Xpt4KDownConverterOut);
                m_card->Connect(NTV2_XptSDIOut5Input, NTV2_XptDuallinkOut5);
                m_card->Connect(NTV2_XptSDIOut5InputDS2,
                                NTV2_XptDuallinkOut5DS2);
            }
            else
            {
                // FB(RGB) -> CSC(YUV) -> 4DK(YUV) -> SDI5
                m_card->Connect(NTV2_XptSDIOut5Input,
                                NTV2_Xpt4KDownConverterOut);
            }
        }
    }

    void KonaVideoDevice::routeStereoYUV(NTV2Standard standard,
                                         const KonaVideoFormat& f,
                                         const KonaDataFormat& d)
    {
        if (m_infoFeedback)
            cout << "INFO: KONA stereo non-RGB format" << endl;

        ULWord vpidA;
        ULWord vpidB;

        CNTV2VPID::SetVPIDData(vpidA, f.value, d.value, false, false,
                               VPIDChannel_1);
        CNTV2VPID::SetVPIDData(vpidB, f.value, d.value, false, false,
                               VPIDChannel_2);

        m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL1);
        m_card->SetSDIOutVPID(vpidB, 0, NTV2_CHANNEL2);

        m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
        m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);

        m_card->SetSDIOutputStandard(NTV2_CHANNEL1, standard);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL2, standard);

        m_card->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY);
        m_card->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);

        m_card->Set4kSquaresEnable(false, NTV2_CHANNEL1);

        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL2);

        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);

        if (m_deviceNumVideoOutputs > 2)
        {
            m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
            m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);
            m_card->SetSDIOutputStandard(NTV2_CHANNEL3, standard);
            m_card->SetSDIOutputStandard(NTV2_CHANNEL4, standard);
            m_card->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
            m_card->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);
            m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL3);
            m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL4);
        }

        if (m_deviceNumVideoOutputs > 4)
        {
            m_card->SetSDITransmitEnable(NTV2_CHANNEL5, true);
            m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL5);
            m_card->SetSDIOutputStandard(NTV2_CHANNEL5, standard);
            m_card->SetMode(NTV2_CHANNEL5, NTV2_MODE_DISPLAY);
            m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL5);
            m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL5, NTV2_AUDIOSYSTEM_5);
        }

        //      (left) FB1 -> CC1 -> SDI1
        //      (right) FB2 -> CC2 -> SDI2
        m_card->Connect(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1RGB);
        m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
        m_card->Connect(NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2RGB);
        m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptCSC2VidYUV);

        if (m_deviceNumVideoOutputs > 4)
        {
            //
            //  In the yuv stereo case we connect SDI 5
            //  monitor to the first eye. Alternately, we
            //  could try using the downconverter with just
            //  two inputs to see both eyes, but it seems
            //  problematic.
            //

            m_card->Connect(NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
        }

        if (m_numHDMIOutputs > 0)
        {
            if (m_deviceHasHDMIStereo)
            {
                if (m_deviceHDMIVersion >= 2)
                {
                    m_card->SetHDMIV2Mode(NTV2_HDMI_V2_HDSD_BIDIRECTIONAL);
                    m_card->SetHDMIOutVideoStandard(standard);
                    m_card->Connect(NTV2_XptHDMIOutInput,
                                    NTV2_XptStereoCompressorOut);
                    m_card->Connect(NTV2_XptHDMIOutQ1Input, NTV2_XptCSC1VidYUV);
                    m_card->Connect(NTV2_XptHDMIOutQ2Input, NTV2_XptCSC1VidYUV);
                    m_card->SetHDMIOut3DPresent(true);
                    m_card->SetHDMIOut3DMode(NTV2_HDMI3DSideBySide);
                }
                else
                {
                    // m_card->SetStereoCompressorOutputMode(NTV2_STEREOCOMPRESSOR_SIDExSIDE);
                    // m_card->SetStereoCompressorLeftSource(NTV2K2_XptCSC1VidYUV);
                    // m_card->SetStereoCompressorRightSource(NTV2K2_XptCSC2VidYUV);
                    m_card->SetHDMIOutVideoStandard(standard);
                    m_card->Connect(NTV2_XptHDMIOutInput, NTV2_XptCSC1VidYUV);
                    m_card->SetHDMIOut3DPresent(true);
                    m_card->SetHDMIOut3DMode(NTV2_HDMI3DSideBySide);
                }
            }
            else
            {
                m_card->SetHDMIOutVideoStandard(standard);
                m_card->Connect(NTV2_XptHDMIOutInput, NTV2_XptCSC1VidYUV);
            }

            m_card->SetHDMIOutBitDepth(getHDMIOutBitDepth(d.value));

            setHDMIHDRMetadata();
        }
    }

    void KonaVideoDevice::routeMonoYUV(NTV2Standard standard,
                                       const KonaVideoFormat& f,
                                       const KonaDataFormat& d)
    {
        if (m_infoFeedback)
            cout << "INFO: KONA mono non-RGB format" << endl;

        ULWord vpidA;
        ULWord vpidB;

        CNTV2VPID::SetVPIDData(vpidA, f.value, d.value, false, false,
                               VPIDChannel_1);
        m_card->SetSDIOutVPID(vpidA, 0, NTV2_CHANNEL1);
        m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, NTV2_AUDIOSYSTEM_1);
        m_card->SetSDIOutputStandard(NTV2_CHANNEL1, standard);
        m_card->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY);
        m_card->Set4kSquaresEnable(false, NTV2_CHANNEL1);
        m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL1);

        if (m_deviceNumVideoOutputs > 1)
        {
            CNTV2VPID::SetVPIDData(vpidB, f.value, d.value, false, false,
                                   VPIDChannel_2);
            m_card->SetSDIOutVPID(vpidB, 0, NTV2_CHANNEL2);
            m_card->SetSDITransmitEnable(NTV2_CHANNEL2, false);
            m_card->SetSDIOutputStandard(NTV2_CHANNEL2, standard);
            m_card->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
            m_card->SubscribeOutputVerticalEvent(NTV2_CHANNEL2);
            m_card->SetSDIOutputAudioSystem(NTV2_CHANNEL2, NTV2_AUDIOSYSTEM_2);

            if (m_deviceNumVideoOutputs > 2)
            {
                m_card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
                m_card->SetSDITransmitEnable(NTV2_CHANNEL4, false);
            }

            if (m_deviceNumVideoOutputs > 4)
            {
                m_card->SetSDITransmitEnable(NTV2_CHANNEL5, true);
            }
        }

        if (m_yuvInternalFormat || !m_deviceHasCSC)
        {
            //
            //  T-tap. Just hook up directly
            //
            //  FB (RGB) -> SDI
            //  FB (YUV) -> HDMI
            //
            if (m_infoFeedback)
                cout << "INFO: YUV internal format/No CSC." << endl;

            m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1RGB);

            if (m_numHDMIOutputs > 0)
            {
                AJA_CHECK(m_card->SetHDMIOutVideoStandard(standard));
                m_card->Connect(NTV2_XptHDMIOutInput, NTV2_XptFrameBuffer1YUV);
                m_card->SetHDMIOutColorSpace(NTV2_HDMIColorSpaceYCbCr);
                m_card->SetHDMIOutBitDepth(getHDMIOutBitDepth(d.value));
                setHDMIHDRMetadata();
            }
        }
        else
        {
            //      FB -> CC -> SDI (all outputs)
            // Route video out Output 1 via FrameStore 1
            m_card->Connect(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1RGB);
            m_card->Connect(NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
            m_card->Connect(NTV2_XptSDIOut2Input, NTV2_XptCSC1VidYUV);
            m_card->Connect(NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
            m_card->Connect(NTV2_XptSDIOut4Input, NTV2_XptCSC1VidYUV);

            if (m_deviceNumVideoOutputs > 4)
            {
                m_card->Connect(NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
            }

            if (m_numHDMIOutputs > 0)
            {
                AJA_CHECK(m_card->SetHDMIOutVideoStandard(standard));
                m_card->Connect(NTV2_XptHDMIOutInput, NTV2_XptCSC1VidYUV);
                m_card->SetHDMIOutColorSpace(NTV2_HDMIColorSpaceYCbCr);
                m_card->SetHDMIOutBitDepth(getHDMIOutBitDepth(d.value));
                setHDMIHDRMetadata();
            }
        }
    }

    void KonaVideoDevice::routeMux(bool tsiEnabled)
    {
        // MUXes are only used in TSI mode.

        if (!tsiEnabled)
            return;

        // Connect mux inputs to framestore outputs.
        m_card->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptFrameBuffer1RGB);
        m_card->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptFrameBuffer1_DS2RGB);
        m_card->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptFrameBuffer2RGB);
        m_card->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptFrameBuffer2_DS2RGB);
    }

    void KonaVideoDevice::routeCSC(bool tsiEnabled, bool outputIsRGB)
    {
        // CSCs are not used if output is RGB (since framebuffers are also RGB,
        // hence no conversion needed).

        if (outputIsRGB)
            return;

        if (tsiEnabled)
        {
            m_card->Connect(NTV2_XptCSC1VidInput, NTV2_Xpt425Mux1ARGB);
            m_card->Connect(NTV2_XptCSC2VidInput, NTV2_Xpt425Mux1BRGB);
            m_card->Connect(NTV2_XptCSC3VidInput, NTV2_Xpt425Mux2ARGB);
            m_card->Connect(NTV2_XptCSC4VidInput, NTV2_Xpt425Mux2BRGB);
        }
        else
        {
            m_card->Connect(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1RGB);
            m_card->Connect(NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2RGB);
            m_card->Connect(NTV2_XptCSC3VidInput, NTV2_XptFrameBuffer3RGB);
            m_card->Connect(NTV2_XptCSC4VidInput, NTV2_XptFrameBuffer4RGB);
        }
    }

    void KonaVideoDevice::route4KDownConverter(bool tsiEnabled,
                                               bool outputIsRGB)
    {
        if (!::NTV2DeviceCanDoWidget(m_deviceID, NTV2_Wgt4KDownConverter))
            return;

        m_card->Enable4KDCRGBMode(outputIsRGB);

        NTV2OutputCrosspointID source1Xpt = NTV2_XptBlack;
        NTV2OutputCrosspointID source2Xpt = NTV2_XptBlack;
        NTV2OutputCrosspointID source3Xpt = NTV2_XptBlack;
        NTV2OutputCrosspointID source4Xpt = NTV2_XptBlack;

        // 4k Downconverter doesn't understand TSI (AJA limitation) so
        // we only route it in quad-mode.
        if (!tsiEnabled)
        {
            if (outputIsRGB)
            {
                // FB(RGB) -> 4KDC(RGB)
                source1Xpt = NTV2_XptFrameBuffer1RGB;
                source2Xpt = NTV2_XptFrameBuffer2RGB;
                source3Xpt = NTV2_XptFrameBuffer3RGB;
                source4Xpt = NTV2_XptFrameBuffer4RGB;
            }
            else
            {
                // FB(RGB) -> CSC(YUV) -> 4KDC(YUV)
                source1Xpt = NTV2_XptCSC1VidYUV;
                source2Xpt = NTV2_XptCSC2VidYUV;
                source3Xpt = NTV2_XptCSC3VidYUV;
                source4Xpt = NTV2_XptCSC4VidYUV;
            }
        }

        m_card->Connect(NTV2_Xpt4KDCQ1Input, source1Xpt);
        m_card->Connect(NTV2_Xpt4KDCQ2Input, source2Xpt);
        m_card->Connect(NTV2_Xpt4KDCQ3Input, source3Xpt);
        m_card->Connect(NTV2_Xpt4KDCQ4Input, source4Xpt);
    }

    void KonaVideoDevice::routeHDMI(NTV2Standard standard,
                                    const KonaDataFormat& d, bool tsiEnabled,
                                    bool outputIsRGB)
    {
        if (::NTV2DeviceGetNumHDMIVideoOutputs(m_deviceID) < 1)
            return;
        if (::NTV2DeviceGetHDMIVersion(m_deviceID) < 2)
            return;

        AJA_CHECK(m_card->SetHDMIOutVideoStandard(standard));
        m_card->SetHDMIOutTsiIO(tsiEnabled);
        m_card->SetHDMIV2Mode(NTV2_HDMI_V2_4K_PLAYBACK);

        m_card->SetHDMIOutColorSpace(outputIsRGB ? NTV2_HDMIColorSpaceRGB
                                                 : NTV2_HDMIColorSpaceYCbCr);

        NTV2OutputCrosspointID source1Xpt = NTV2_XptBlack;
        NTV2OutputCrosspointID source2Xpt = NTV2_XptBlack;
        NTV2OutputCrosspointID source3Xpt = NTV2_XptBlack;
        NTV2OutputCrosspointID source4Xpt = NTV2_XptBlack;

        if (tsiEnabled)
        {
            if (outputIsRGB)
            {
                // FB(RGB) -> MUX(RGB) -> HDMI(RGB)
                source1Xpt = NTV2_Xpt425Mux1ARGB;
                source2Xpt = NTV2_Xpt425Mux1BRGB;
                source3Xpt = NTV2_Xpt425Mux2ARGB;
                source4Xpt = NTV2_Xpt425Mux2BRGB;
            }
            else
            {
                // FB(RGB) -> MUX(RGB) -> CSC(YUV) -> HDMI(YUV)
                source1Xpt = NTV2_XptCSC1VidYUV;
                source2Xpt = NTV2_XptCSC2VidYUV;
                source3Xpt = NTV2_XptCSC3VidYUV;
                source4Xpt = NTV2_XptCSC4VidYUV;
            }
        }
        else
        {
            if (outputIsRGB)
            {
                // FB(RGB) -> 4KDC(RGB) -> HDMI(RGB)
                source1Xpt = NTV2_Xpt4KDownConverterOutRGB;
            }
            else
            {
                // FB(RGB) -> CSC(YUV) -> 4KDC(YUV) -> HDMI(YUV)
                source1Xpt = NTV2_Xpt4KDownConverterOut;
            }
        }

        m_card->Connect(NTV2_XptHDMIOutInput, source1Xpt);
        m_card->Connect(NTV2_XptHDMIOutQ1Input, source1Xpt);
        m_card->Connect(NTV2_XptHDMIOutQ2Input, source2Xpt);
        m_card->Connect(NTV2_XptHDMIOutQ3Input, source3Xpt);
        m_card->Connect(NTV2_XptHDMIOutQ4Input, source4Xpt);

        m_card->SetHDMIOutBitDepth(getHDMIOutBitDepth(d.value));

        setHDMIHDRMetadata();
    }

    void KonaVideoDevice::setHDMIHDRMetadata()
    {
        if (!::NTV2DeviceCanDoHDMIHDROut(m_deviceID))
            return;

        m_card->EnableHDMIHDR(false);

        if (m_useHDMIHDRMetadata)
        {
            AJA_CHECK(m_card->SetHDRData(m_hdrMetadata));

            m_card->EnableHDMIHDR(true);
        }
    }

    void KonaVideoDevice::close()
    {
        const bool proMode = m_operationMode == OperationMode::ProMode;

        if (m_open)
        {
            unbind();

            //
            //  Unhook processing graph
            //
            if (proMode)
            {
                //
                // The level A setting appears to be sticky even once the
                // processing graph is hooked leaving the spigot in a
                // state where level A and level B timings are both
                // under some circumstances. This can cause some devices
                // like projectors to flicker. So we set level A back
                // to its default state of false.
                //
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL1, false);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL2, false);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL3, false);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL4, false);
                m_card->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL5, false);

                m_card->ClearRouting();
            }

            if (m_acquire)
            {
                m_card->SetEveryFrameServices(m_taskMode);
                m_card->ReleaseStreamForApplication(
                    appID(), (uint32_t)TwkUtil::processID());
            }

            for (size_t i = 0; i < m_videoChannels.size(); i++)
            {
                VideoChannel* vc = m_videoChannels[i];
                delete vc;
            }

            m_videoChannels.clear();

            m_open = false;
            delete m_card;
            m_card = 0;
        }

        TwkGLF::GLBindableVideoDevice::close();
    }

    static void trampoline(void* d)
    {
        setThreadName("AJA Device");
        GC_EXCLUDE_THIS_THREAD;
        ((KonaVideoDevice*)d)->threadMain();
    }

    void KonaVideoDevice::lockDevice(bool lock, const char* threadName) const
    {
        if (lock)
        {
            Timer timer;
            timer.start();
            m_deviceMutex.lock();
            Time t = timer.elapsed();
            if (t > 0.001 && m_infoFeedback)
                cout << "INFO: " << threadName << ": lockDevice for " << t
                     << endl;
        }
        else
        {
            m_deviceMutex.unlock();
        }
    }

    namespace
    {

        //
        //  In order to do this with a shader and have it be efficient we have
        //  to write the shader to completely pack the subsampled data with no
        //  scanline padding. Until we have that this will suffice.
        //
        //  NOTE: this is discarding every other chroma sample instead of
        //  interpolating. Not the best way to do this.
        //

        void subsample422_8bit_UYVY(int width, int height,
                                    unsigned char* buffer)
        {
            Timer timer;
            timer.start();

            unsigned char* p1 = buffer;

            if (width % 6 == 0)
            {
                for (size_t row = 0; row < height; row++)
                {
                    for (
                        unsigned char* __restrict p0 = buffer + row * 3 * width, * __restrict e =
                                                                                     p0
                                                                                     + 3 * width;
                        p0 < e; p0 += 6, p1 += 4)
                    {
                        //
                        //  The commented out parts do the interpolation, but
                        //  this can be done as a small blur after the YUV
                        //  conversion. That saves a few microseconds
                        //

                        // p1[0] = (int(p0[1]) + int(p0[4])) >> 1;
                        p1[0] = p0[1];
                        p1[1] = p0[0];
                        // p1[2] = (int(p0[2]) + int(p0[5])) >> 1;
                        p1[2] = p0[2];
                        p1[3] = p0[3];
                    }
                }
            }
            else
            {
                for (size_t row = 0; row < height; row++)
                {
                    size_t count = 0;

                    for (
                        unsigned char* __restrict p0 = buffer + row * 3 * width, * __restrict e =
                                                                                     p0
                                                                                     + 3 * width;
                        p0 < e; p0 += 3, count++)
                    {
                        *p1 = p0[count % 2 + 1];
                        p1++;
                        *p1 = *p0;
                        p1++;
                    }
                }
            }

            timer.stop();
            // cout << timer.elapsed() << endl;
        }

#define R10MASK 0x3FF00000
#define G10MASK 0xFFC00
#define B10MASK 0x3FF

        void subsample422_10bit(int width, int height, uint32_t* buffer)
        {
            Timer timer;
            timer.start();
            uint32_t* p1 = buffer;

            //
            //  NOTE: 2_10_10_10_INT_REV is *backwards* eventhough its GL_RGB
            //  (hence the REV). So Y is the lowest sig bits.
            //

            const size_t padding = (width % 6) * 6; // scanline padding

            for (size_t row = 0; row < height; row++)
            {
                for (
                    uint32_t* __restrict p0 = buffer + row * width, * __restrict e =
                                                                        p0
                                                                        + width
                                                                        - (width
                                                                           % 6);
                    p0 < e; p0 += 6)
                {
                    const uint32_t A = *p0;
                    const uint32_t B = p0[1];
                    const uint32_t C = p0[2];
                    const uint32_t D = p0[3];
                    const uint32_t E = p0[4];
                    const uint32_t F = p0[5];

                    *p1 = (A & R10MASK) | ((A << 10) & G10MASK)
                          | ((A >> 10) & B10MASK);
                    p1++;
                    *p1 = ((C << 20) & R10MASK) | (B & G10MASK) | (B & B10MASK);
                    p1++;
                    *p1 = ((C << 10) & R10MASK) | ((D << 10) & G10MASK)
                          | ((B >> 20) & B10MASK);
                    p1++;
                    *p1 = ((F << 20) & R10MASK) | ((D >> 10) & G10MASK)
                          | (E & B10MASK);
                    p1++;
                }

                for (uint32_t* __restrict pe = p1 + padding; p1 < pe; p1++)
                    *p1 = 0;
                // p1 += padding;
            }

            timer.stop();
            // cout << timer.elapsed() << endl;
        }

    } // namespace

    void KonaVideoDevice::threadMain()
    {
        lockDevice(true);
        lockDevice(false);

        size_t nchannels = m_videoChannels.size();
        const bool doingStereo = (nchannels == 2);

        lockDevice(true, "TRANSFER");
        bool threadStop = m_threadStop;
        lockDevice(false);

        bool wasPaused = false;
        bool sleeping = false;

        for (size_t vi = 0; !threadStop; vi = (vi + 1) % nchannels)
        {
            VideoChannel* vc = m_videoChannels[vi];
            AJA_CHECK(
                m_card->AutoCirculateGetStatus(vc->channel, vc->auto_status));
            const int d = vc->auto_status.acFramesDropped;

            sleeping = false;

            const NTV2AutoCirculateState state = vc->auto_status.acState;
            const bool running = state == NTV2_AUTOCIRCULATE_RUNNING;
            const bool paused = state == NTV2_AUTOCIRCULATE_PAUSED;
            const bool init = state == NTV2_AUTOCIRCULATE_INIT;
            const bool starting = state == NTV2_AUTOCIRCULATE_STARTING;
            const int bufferLevel = vc->auto_status.acBufferLevel;
            const bool dropped = vc->auto_status.acFramesDropped != d;
            const bool lastChannel = vi == nchannels - 1;

            //
            //  Possible start up autocirc or block if buffer is full
            //

            if (vi == 0)
            {
                showAutoCirculateState(state, bufferLevel);

                if (init)
                {
                    lockDevice(true, "TRANSFER INIT");
                    size_t rcount = m_readBufferCount;

                    if (rcount >= m_highwater && !m_autocirculateRunning)
                    {
                        AJA_CHECK(m_card->AutoCirculateStart(NTV2_CHANNEL1));

                        if (doingStereo)
                        {
                            AJA_CHECK(
                                m_card->AutoCirculateStart(NTV2_CHANNEL2));
                        }
                        m_autocirculateRunning = true;
                        m_starting = true;
                    }

                    threadStop = m_threadStop;
                    lockDevice(false);
                    vi = nchannels - 1;
                    continue;
                }
                else if (m_starting)
                {
                    if (!starting)
                    {
                        lockDevice(true, "TRANSFER");
                        m_starting = false;
                        threadStop = m_threadStop;
                        lockDevice(false);
                    }
                }
                else if (running && bufferLevel == 0 && m_ringBufferSize > 2
                         && !wasPaused)
                {
                    if (m_usePausing)
                    {
                        //
                        //  Pause.
                        //
                        //  NOTE: This is tricky for some reason so
                        //  normally we won't use this path through the
                        //  code. I think the problem is caused by the
                        //  hack to get around desiredFrame==-1 failing in
                        //  TransferWithAutoCirculate(). (We're specifying
                        //  exactly which frame to use instead of relying
                        //  on the driver).
                        //
                        m_card->AutoCirculatePause(NTV2_CHANNEL1);
                        if (doingStereo)
                        {
                            m_card->AutoCirculatePause(NTV2_CHANNEL2);
                        }

                        lockDevice(true, "TRANSFER");
                        m_paused = true;
                        threadStop = m_threadStop;
                        lockDevice(false);
                    }
                    else
                    {
                        sleeping = true;
#ifdef PLATFORM_WINDOWS
                        m_card->WaitForOutputVerticalInterrupt(NTV2_CHANNEL1);
#else
                        AJA_CHECK(m_card->WaitForOutputVerticalInterrupt(
                            NTV2_CHANNEL1));
#endif
                    }
                }
                else if (paused)
                {
                    lockDevice(true, "TRANSFER");
                    size_t rs = m_ringBufferSize;
                    size_t ri = m_readBufferIndex;
                    size_t wi = m_writeBufferIndex;

                    if (bufferLevel >= rs - 1 || ((ri + 1) % rs == wi))
                    {
                        //
                        //  Restart after pause
                        //
                        m_card->AutoCirculatePause(NTV2_CHANNEL1);
                        if (doingStereo)
                        {
                            m_card->AutoCirculatePause(NTV2_CHANNEL2);
                        }

                        m_paused = false;
                        wasPaused = true;
                        resetClock();
                    }

                    threadStop = m_threadStop;
                    lockDevice(false);
                    AJA_CHECK(
                        m_card->WaitForOutputVerticalInterrupt()); // XXX best
                                                                   // guess
                }
                else if (bufferLevel >= m_ringBufferSize - 1)
                {
                    AJA_CHECK(
                        m_card->WaitForOutputVerticalInterrupt()); // XXX best
                                                                   // guess
                    wasPaused = false;
                    vi = nchannels - 1;
                    continue;
                }
            }

            if (!sleeping && dropped && m_infoFeedback)
                cout << "INFO: dropped frame " << m_frameCount << endl;

            bool wrote = false;
            //
            //  Maybe transfer frame
            //

            lockDevice(true, "TRANSFER");
            size_t wi = m_writeBufferIndex;
            FrameData& f = vc->data[wi];
            lockDevice(false);

            f.lockState("TRANSFER");
            const bool readyForTransfer = f.state == FrameData::State::Mapped;
            const bool reading = f.state == FrameData::State::Reading;
            const bool shouldLock = readyForTransfer | reading;
            f.unlockState();

            if (shouldLock)
            {
                f.lockImage("TRANSFER");

                //
                //  Update threadStop again in case we were blocked
                //

                lockDevice(true, "TRANSFER");
                threadStop = m_threadStop;
                lockDevice(false);

                if (!threadStop)
                {
                    f.lockState("TRANSFER");
                    f.state = FrameData::State::Transfering;
                    f.unlockState();

                    AUTOCIRCULATE_TRANSFER& t = vc->auto_transfer;

                    // t.acVideoBuffer = f.mappedPointer ?
                    // (ULWord*)f.mappedPointer : (ULWord*)f.imageData; cout <<
                    // "**** SetVideoBuffer: m_bufferStride: " << m_bufferStride
                    // << " m_height: " << m_height
                    // << endl;
                    //  this function is returning false all the time but its
                    //  working?
                    t.SetVideoBuffer(f.mappedPointer ? (ULWord*)f.mappedPointer
                                                     : (ULWord*)f.imageData,
                                     m_bufferStride * m_height);

                    if (!t.acVideoBuffer)
                    {
                        cout << "ERROR: t.acVideoBuffer is " << t.acVideoBuffer
                             << endl;
                        m_threadStop = true; // ???
                    }

                    if (audioOutputEnabled() && !threadStop)
                    {
                        // may have to restart audio after a drop??
                        if (dropped)
                        {
                            AJA_CHECK(m_card->WriteRegister(kRegAud1Control,
                                                            0x80E00100));
                            if (m_stereo)
                                m_card->WriteRegister(kRegAud1Control,
                                                      0x80C00100);
                            AJA_CHECK(m_card->SetNumberAudioChannels(
                                audioFormatChannelCountForCard(),
                                NTV2AudioSystem(vc->channel)));
                        }

                        f.lockAudio("TRANSFER");
                        // t.acAudioBuffer     =
                        // (NTV2_POINTER)&(f.audioBuffer.front());
                        t.SetAudioBuffer((ULWord*)&(f.audioBuffer.front()),
                                         f.audioBuffer.size() * sizeof(int));
                        // t.acAudioBufferSize = f.audioBuffer.size() *
                        // sizeof(int); t.acAudioStartSample = 0;
                        f.unlockAudio();
                    }
                    else
                    {
                        t.SetAudioBuffer((ULWord*)(0), 0);
                    }

                    if (m_yuvInternalFormat && !m_immediateCopy)
                    {
                        switch (m_actualDataFormat.iformat)
                        {
                        case Y0CbY1Cr_8_422:
                            // cout << "SUB 8" << endl;
                            subsample422_8bit_UYVY(
                                m_width, m_height,
                                (unsigned char*)
                                    t.acVideoBuffer.GetHostPointer());
                            break;
                        case YCrCb_AJA_10_422:
                            // cout << "SUB 10" << endl;
                            subsample422_10bit(
                                m_width, m_height,
                                (uint32_t*)t.acVideoBuffer.GetHostPointer());
                            break;
                        default:
                            break;
                        }
                    }
                    startAJATransfer();

                    if (!threadStop)
                    {
                        if (!m_card->AutoCirculateTransfer(vc->channel, t))
                        {
                            if (m_infoFeedback)
                                cout << "INFO: KONA: AutoCirculateTransfer "
                                        "failed"
                                     << endl;
                        }
                    }
                    endAJATransfer();

                    wrote = true;
                    f.lockState("TRANSFER");
                    f.state = FrameData::State::NeedsUnmap;
                    f.unlockState();
                }

                f.unlockImage();
            }

            lockDevice(true, "TRANSFER");

            if (wrote && lastChannel)
            {
                m_writeBufferCount++;
                m_writeBufferIndex = m_writeBufferCount % m_ringBufferSize;
            }

            threadStop = m_threadStop;
            lockDevice(false);

            if (!wrote)
            {
                AJA_CHECK(m_card->WaitForOutputVerticalInterrupt(
                    NTV2_CHANNEL1)); // XXX best guess
                vi--;
            }
        }

        //
        //  Shutdown ring buffer and exit thread
        //

        for (size_t i = 0; i < nchannels; i++)
        {
            m_card->AutoCirculateStop(m_videoChannels[i]->channel);
        }

        lockDevice(true, "TRANSFER");
        m_autocirculateRunning = false;
        m_threadDone = true;
        lockDevice(false);
    }

    void KonaVideoDevice::bind(const GLVideoDevice* d) const
    {
        if (!m_open)
            return;

        m_threadStop = false;
        m_threadDone = false;
        m_lastState = -1;
        m_lastBL = -1;

        m_gpuTimes.clear();
        m_konaTimes.clear();
        m_globalTimer.start();

        if (m_pbos)
        {
            //
            //  Generate the PBO buffers
            //

            for (size_t q = 0; q < m_videoChannels.size(); q++)
            {
                VideoChannel* vc = m_videoChannels[q];

                for (size_t i = 0; i < m_ringBufferSize; i++)
                {
                    FrameData& f = vc->data[i];

                    glGenBuffers(1, &f.globject);
                    TWK_GLDEBUG;
                    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, f.globject);
                    TWK_GLDEBUG;
                    glBufferData(GL_PIXEL_PACK_BUFFER_ARB, m_bufferSizeInBytes,
                                 NULL, GL_DYNAMIC_READ);
                    TWK_GLDEBUG;
                    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
                    TWK_GLDEBUG;

                    if (m_immediateCopy)
                    {
                        f.imageData = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(
                            unsigned char, m_bufferSizeInBytes);
                    }

                    f.state = FrameData::State::Ready;
                }
            }
        }
        else // readpixels
        {
            for (size_t q = 0; q < m_videoChannels.size(); q++)
            {
                VideoChannel* vc = m_videoChannels[q];

                for (size_t i = 0; i < m_ringBufferSize; i++)
                {
                    FrameData& f = vc->data[i];
                    f.imageData = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(
                        unsigned char, m_bufferSizeInBytes);
                    f.state = FrameData::State::Ready;
                }
            }
        }

        m_threadGroup.dispatch(trampoline, (void*)this);

        resetClock();
        m_readBufferIndex = 0;
        m_readBufferCount = 0;
        m_writeBufferIndex = 0;
        m_writeBufferCount = 0;
        m_bound = true;
    }

    void KonaVideoDevice::bind2(const GLVideoDevice* d,
                                const GLVideoDevice* d2) const
    {
        bind(d);
    }

    bool KonaVideoDevice::isStereo() const { return m_stereo; }

    bool KonaVideoDevice::isDualStereo() const { return isStereo(); }

    bool KonaVideoDevice::willBlockOnTransfer() const
    {
        VideoChannel* vc = m_videoChannels[0];

        lockDevice(true, "WILLBLOCK");
        FrameData& fd = vc->data[m_readBufferIndex];
        const bool paused = m_paused;
        const bool starting = m_starting;
        lockDevice(false);

        fd.lockState("WILLBLOCK");
        FrameData::State s = fd.state;
        fd.unlockState();

        return !starting && !paused
               && (s == FrameData::State::Transfering
                   || s == FrameData::State::Mapped
                   || s == FrameData::State::Reading);
    }

    void KonaVideoDevice::unbind() const
    {
        if (!m_bound)
            return;
        m_globalTimer.stop();

        //
        //  Shutdown transfer thread
        //

        lockDevice(true, "UNBIND");
        m_threadStop = true;
        lockDevice(false);

        lockDevice(true, "UNBIND");
        bool threadDone = m_threadDone;
        lockDevice(false);

        //
        //  Unlock any images that were locked by the reader thread.
        //

        for (size_t q = 0; q < m_videoChannels.size(); q++)
        {
            VideoChannel* vc = m_videoChannels[q];

            for (size_t i = 0; i < vc->data.size(); i++)
            {
                //
                //  Careful with the locking order here. If the thread is
                //  in the middle of an operation,
                //  we don't want to pull the rug out from under it until
                //  after its completed.
                //

                FrameData& f = vc->data[i];
                f.lockState("UNBIND");
                if (f.state == FrameData::State::Reading && f.locked)
                {
                    f.locked = false;
                    f.unlockImage();
                }
                f.unlockState();
                f.lockImage("UNBIND");
                f.fbo = 0;
                f.unlockImage();
            }
        }

        if (!threadDone)
            m_threadGroup.control_wait();

        //
        //  Delete all allocated resources
        //

        for (size_t q = 0; q < m_videoChannels.size(); q++)
        {
            VideoChannel* vc = m_videoChannels[q];

            for (size_t i = 0; i < vc->data.size(); i++)
            {
                FrameData& f = vc->data[i];

                if (m_pbos && !m_immediateCopy
                    && f.state == FrameData::State::Mapped)
                {
                    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, f.globject);
                    TWK_GLDEBUG;
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
                    TWK_GLDEBUG;
                    f.globject = 0;
                }

                if (m_pbos)
                {
                    glDeleteBuffers(1, &f.globject);
                    TWK_GLDEBUG;
                }

                if (f.imageData)
                {
                    TWK_DEALLOCATE_ARRAY(f.imageData);
                    f.imageData = 0;
                }
            }

            m_card->AutoCirculateStop(vc->channel);
        }

        m_bound = false;

        if (m_profile)
        {
            if (!m_gpuTimes.empty())
            {
                Time accumTime = 0;
                Time minTime = numeric_limits<double>::max();
                Time maxTime = -numeric_limits<double>::max();

                for (size_t i = 5; i < m_gpuTimes.size(); i++)
                {
                    Time t = m_gpuTimes[i];
                    accumTime += t;
                    minTime = std::min(minTime, t);
                    maxTime = std::max(maxTime, t);
                }

                cout << "INFO: GPU: " << (accumTime / m_gpuTimes.size())
                     << ", min=" << minTime << ", max=" << maxTime
                     << ", count=" << m_gpuTimes.size() << endl;
            }

            if (!m_konaTimes.empty())
            {
                Time accumTime = 0;
                Time minTime = numeric_limits<double>::max();
                Time maxTime = -numeric_limits<double>::max();

                for (size_t i = 5; i < m_konaTimes.size(); i++)
                {
                    Time t = m_konaTimes[i];
                    accumTime += t;
                    minTime = std::min(minTime, t);
                    maxTime = std::max(maxTime, t);
                }

                cout << "INFO: KONA: " << (accumTime / m_konaTimes.size())
                     << ", min=" << minTime << ", max=" << maxTime
                     << ", count=" << m_konaTimes.size() << endl;
            }

            if (!m_gpuTimes.empty() && !m_konaTimes.empty())
            {
                ostringstream filename;
                filename << "twk_aja_profile_" << TwkUtil::processID()
                         << ".csv";
                ofstream file(filename.str().c_str());

                file << "GPUStart,GPUDuration,NTV2Begin,NTV2Duration" << endl;

                for (size_t i = 0,
                            s = std::min(m_gpuTimes.size(), m_konaTimes.size());
                     i < s; i++)
                {
                    file << m_gpuBeginTime[i] << "," << m_gpuTimes[i] << ","
                         << m_konaBeginTime[i] << "," << m_konaTimes[i] << endl;
                }
            }
        }
    }

    void KonaVideoDevice::resetClock() const { VideoDevice::resetClock(); }

    void KonaVideoDevice::transferChannelPBO(VideoChannel* vc,
                                             const GLFBO* fbo) const
    {
        HOP_CALL(glFinish();)
        HOP_PROF_FUNC();

        //
        //  Transfer last read PBO or pinned buffer
        //
        //  NOTE: we don't have to lock the device to read
        //  m_readBufferIndex because only this thread is allowed to change
        //  it.

        const size_t previousReadIndex =
            (m_readBufferIndex + m_ringBufferSize - 1) % m_ringBufferSize;

        lockDevice(true, "PBO READER");
        lockDevice(false);

        if (m_readBufferCount > 0)
        {
            FrameData& fdprev = vc->data[previousReadIndex];

            fdprev.lockState("PBO READER");
            bool reading = fdprev.state == FrameData::State::Reading;
            fdprev.unlockState();
            assert(reading);

            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, fdprev.globject);
            TWK_GLDEBUG;
            GLubyte* p = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,
                                               GL_READ_ONLY_ARB);
            TWK_GLDEBUG;

            if (p)
            {
                if (m_immediateCopy)
                {
                    if (m_yuvInternalFormat)
                    {
                        switch (m_actualDataFormat.iformat)
                        {
                        case Y0CbY1Cr_8_422:
                            subsample422_8bit_UYVY_MP(
                                m_width, m_height,
                                reinterpret_cast<uint8_t*>(p),
                                reinterpret_cast<uint8_t*>(fdprev.imageData));
                            break;
                        case YCrCb_AJA_10_422:
                        {
                            const size_t srcStrideInBytes =
                                m_width * sizeof(uint32_t);
                            const size_t dstStrideInBytes =
                                ROUNDUP(m_width, 24) * 4 * sizeof(uint32_t) / 6;
                            subsample422_10bit_MP(
                                m_width, m_height,
                                reinterpret_cast<uint32_t*>(p),
                                reinterpret_cast<uint32_t*>(fdprev.imageData),
                                srcStrideInBytes, dstStrideInBytes);
                        }
                        break;
                        default:
                            std::cerr << "ERROR: YUV format not supported by "
                                         "subsampling "
                                         "operation."
                                      << std::endl;
                            break;
                        }
                    }
                    else
                    {
                        FastMemcpy_MP(fdprev.imageData, p, m_bufferSizeInBytes);
                    }
                }
                else
                {
                    fdprev.mappedPointer = p;
                }

                fdprev.lockState("PBO READER");
                if (fdprev.mappedPointer || fdprev.imageData)
                    fdprev.state = FrameData::State::Mapped;
                if (m_immediateCopy)
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
                TWK_GLDEBUG;
                fdprev.fbo->endExternalReadback();
                fdprev.fbo = 0;
                fdprev.unlockState();
            }
            else
            {
                fdprev.imageData = 0;
                fdprev.mappedPointer = 0;
                fdprev.fbo->endExternalReadback();
                fdprev.fbo = 0;
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
                TWK_GLDEBUG;
                cout << "ERROR: not mapped" << endl;
            }

            endGPUTransfer();

            m_mappedBufferCount++;
            fdprev.locked = false;
            fdprev.unlockImage();
        }

        //
        //  Initiate next PBO read.
        //
        //  NOTE: we don't have to lock the device to read
        //  m_readBufferIndex because only this thread is allowed to change
        //  it.
        //

        FrameData& fdread = vc->data[m_readBufferIndex];

        //
        //  May block at this point if transferring
        //

        fdread.lockImage("PBO READER"); // until unmapped above
        fdread.locked = true;

        fdread.lockState("PBO READER");
        bool transfer = fdread.state == FrameData::State::Transfering;
        bool unmapit = fdread.state == FrameData::State::NeedsUnmap || transfer;
        bool mapped = fdread.state == FrameData::State::Mapped;
        if (!mapped)
        {
            fdread.state = FrameData::State::Reading;
            fdread.fbo = fbo;
        }
        fdread.unlockState();

        if (mapped)
        {
            //
            //  This can happen in the non immediateCopy case. We're
            //  basically about to read over an already mapped PBO.
            //
            //  Prob can't happen anymore now that renderer waits until
            //  external readback happens on FBOs.
            //

            while (mapped)
            {
                fdread.locked = false;
                fdread.unlockImage();
                fdread.lockState("PBO READER WAIT");
                transfer = fdread.state == FrameData::State::Transfering;
                unmapit =
                    fdread.state == FrameData::State::NeedsUnmap || transfer;
                mapped = fdread.state == FrameData::State::Mapped;
                fdread.unlockState();
#ifndef PLATFORM_WINDOWS
                usleep(1);
#endif
                fdread.lockImage("PBO READER");
                fdread.locked = true;
            }

            fdread.lockState("PBO READER");
            fdread.state = FrameData::State::Reading;
            fdread.fbo = fbo;
            fdread.unlockState();
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, fdread.globject);
        TWK_GLDEBUG;

        if (unmapit)
        {
            if (!m_immediateCopy)
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
            TWK_GLDEBUG;
            fdread.mappedPointer = 0;
            m_mappedBufferCount--;
        }

        startGPUTransfer();

        glReadPixels(0, 0, m_width, m_height, m_textureFormat, m_textureType,
                     0);
        TWK_GLDEBUG;

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        HOP_CALL(glFinish();)
    }

    void KonaVideoDevice::transferChannelReadPixels(VideoChannel* vc,
                                                    const GLFBO* fbo) const
    {
        HOP_PROF_FUNC();

        //
        //  Vanillia glReadPixels case (only for comparison)
        //

        FrameData& fdread = vc->data[m_readBufferIndex];

        fdread.lockImage("READPIXELS READER");
        fdread.lockState("READPIXELS READER");
        fdread.state = FrameData::State::Reading;
        fdread.fbo = fbo;
        fdread.unlockState();

        startGPUTransfer();

        glReadPixels(0, 0, m_width, m_height, m_textureFormat, m_textureType,
                     fdread.imageData);
        TWK_GLDEBUG;

        endGPUTransfer();

        fdread.lockState("READPIXELS READER");
        fdread.state = FrameData::State::Mapped;
        fdread.fbo->endExternalReadback();
        fdread.fbo = 0;
        fdread.unlockState();
        fdread.unlockImage();
    }

    void KonaVideoDevice::transferChannel(size_t n, const GLFBO* fbo) const
    {
        HOP_ZONE(HOP_ZONE_COLOR_4);
        HOP_PROF_FUNC();

        VideoChannel* vc = m_videoChannels[n];

        fbo->bind();
        fbo->beginExternalReadback();

        if (m_pbos)
            transferChannelPBO(vc, fbo);
        else
            transferChannelReadPixels(vc, fbo);

        m_card->AutoCirculateGetStatus(vc->channel, vc->auto_status);
        m_bufferLevel = vc->auto_status.acBufferLevel;
    }

    void KonaVideoDevice::transfer(const GLFBO* fbo) const
    {
        HOP_ZONE(HOP_ZONE_COLOR_4);
        HOP_PROF_FUNC();

        if (!m_open)
            return;

        transferChannel(0, fbo);

        lockDevice(true, "READER in transfer");
        m_readBufferCount++;
        m_readBufferIndex = m_readBufferCount % m_ringBufferSize;
        incrementClock();

        lockDevice(false);
    }

    void KonaVideoDevice::transfer2(const GLFBO* fbo, const GLFBO* fbo2) const
    {
        if (!m_open)
            return;

        transferChannel(0, fbo);
        transferChannel(1, fbo2);

        lockDevice(true, "READER in transfer2");
        m_readBufferCount++;
        m_readBufferIndex = m_readBufferCount % m_ringBufferSize;
        incrementClock();

        lockDevice(false);
    }

    VideoDevice::Time KonaVideoDevice::outputTime() const
    {
        lockDevice(true, "OUTPUT TIME");
        Time t = VideoDevice::outputTime();
        lockDevice(false);
        return t;
    }

    VideoDevice::Time KonaVideoDevice::deviceLatency() const
    {
        return double(m_bufferLevel) / Time(m_actualVideoFormat.hz);
    }

    size_t KonaVideoDevice::asyncMaxMappedBuffers() const
    {
        return m_ringBufferSize;
    }

    VideoDevice::Time KonaVideoDevice::inputTime() const { return 0; }

    bool KonaVideoDevice::isOpen() const { return m_open; }

    void KonaVideoDevice::beginTransfer() const {}

    void KonaVideoDevice::endTransfer() const {}

    void KonaVideoDevice::makeCurrent() const {}

    void KonaVideoDevice::clearCaches() const {}

    void KonaVideoDevice::syncBuffers() const {}

    //----------------------------------------------------------------------
    //
    //  UI Related
    //

#define TOTAL_AUDIO_FRAME_SIZE 5

    void
    KonaVideoDevice::audioFrameSizeSequence(AudioFrameSizeVector& fsizes) const
    {
        //
        //  The Kona SDK (and prob any SDK) will only produce 5 frame
        //  repeating patterns
        //
        //  XXX  If this is locked to TOTAL_AUDIO_FRAME_SIZE frames, will it
        //  work when we arbitrarily set ring buffer size ?

        const KonaVideoFormat& f = m_actualVideoFormat;
        NTV2FrameRate frate = GetNTV2FrameRateFromVideoFormat(f.value);

        fsizes.resize(TOTAL_AUDIO_FRAME_SIZE);
        ULWord u372;
        m_card->GetSmpte372(u372);
        m_audioFrameSizes.resize(TOTAL_AUDIO_FRAME_SIZE);

        for (size_t i = 0; i < TOTAL_AUDIO_FRAME_SIZE; i++)
        {
            if (m_deviceHas96kAudio
                && audioFormats[m_internalAudioFormat].hz == 96000)
            {
                fsizes[i] =
                    GetAudioSamplesPerFrame(frate, NTV2_AUDIO_96K, i, u372);
            }
            else
            {
                fsizes[i] =
                    GetAudioSamplesPerFrame(frate, NTV2_AUDIO_48K, i, u372);
            }
            m_audioFrameSizes[i] = fsizes[i];
        }
    }

    //
    // This returns a permissible value for setting the
    // number of audio channels on a Kona card.
    //
    int KonaVideoDevice::audioFormatChannelCountForCard() const
    {
        int nch = audioFormats[m_internalAudioFormat].numChannels;
        // Make sure nch is a value greater than or equal to our
        // chosen audio format;
        // Currently nch can only be 6, 8 or 16.
        if (nch > 8)
        {
            nch = 16;
        }
        else if (nch > 6)
        {
            nch = 8;
        }
        else
        {
            nch = 6;
        }

        return nch;
    }

    size_t KonaVideoDevice::numAudioFormats() const
    {
        if (m_deviceMaxAudioChannels <= 6)
        {
            // Includes stereo and 5.1 formats
            return 2;
        }
        else if (m_deviceMaxAudioChannels <= 8)
        {
            // Includes 7.1 formats
            return 4;
        }

        return 5;
    }

    KonaVideoDevice::AudioFormat
    KonaVideoDevice::audioFormatAtIndex(size_t index) const
    {
        const KonaAudioFormat& f = audioFormats[index];
        return AudioFormat(f.hz, f.prec, f.numChannels, f.layout, f.desc);
    }

    size_t KonaVideoDevice::currentAudioFormat() const
    {
        return m_internalAudioFormat;
    }

    void KonaVideoDevice::setAudioFormat(size_t i)
    {
        if (i > numAudioFormats())
            i = numAudioFormats() - 1;
        m_internalAudioFormat = i;
        m_audioFormat = i;
        m_audioFrameSizes.clear();
    }

    size_t KonaVideoDevice::numVideoFormats() const
    {
        return m_konaVideoFormats.size();
    }

    KonaVideoDevice::VideoFormat
    KonaVideoDevice::videoFormatAtIndex(size_t index) const
    {
        // cout << "videoFormatAtIndex: " << index << endl;

        if (m_operationMode == OperationMode::SimpleMode)
        {
            if (!m_open)
                const_cast<KonaVideoDevice*>(this)->queryCard();
            const KonaVideoFormat& f = m_actualVideoFormat;
            return VideoFormat(f.width, f.height, f.pa, 1.0f, f.hz, f.desc);
        }

        if (index >= m_konaVideoFormats.size())
        {
            return VideoFormat();
        }

        const KonaVideoFormat& f = m_konaVideoFormats[index];
        return VideoFormat(f.width, f.height, f.pa, 1.0f, f.hz, f.desc);
    }

    size_t KonaVideoDevice::currentVideoFormat() const
    {
        return m_internalVideoFormat;
    }

    void KonaVideoDevice::setVideoFormat(size_t i)
    {
        const size_t n = numVideoFormats();
        if (i >= n)
            i = n - 1;
        const KonaVideoFormat& f = m_konaVideoFormats[i];
        m_internalVideoFormat = i;
        m_videoFormat = f.value;
        m_audioFrameSizes.clear();
    }

    size_t KonaVideoDevice::numDataFormats() const
    {
        return m_konaDataFormats.size();
    }

    KonaVideoDevice::DataFormat
    KonaVideoDevice::dataFormatAtIndex(size_t i) const
    {
        if (m_operationMode == OperationMode::SimpleMode)
        {
            if (!m_open)
                const_cast<KonaVideoDevice*>(this)->queryCard();
            return DataFormat(m_actualDataFormat.iformat,
                              m_actualDataFormat.desc);
        }

        if (i >= m_konaDataFormats.size())
        {
            return DataFormat();
        }

        return DataFormat(m_konaDataFormats[i].iformat,
                          m_konaDataFormats[i].desc);
    }

    void KonaVideoDevice::setDataFormat(size_t i)
    {
        const KonaDataFormat& f = m_konaDataFormats[i];
        m_internalDataFormat = i;
        m_dataFormat = f.value;
    }

    size_t KonaVideoDevice::currentDataFormat() const
    {
        return m_internalDataFormat;
    }

    size_t KonaVideoDevice::numSyncModes() const { return 1; }

    KonaVideoDevice::SyncMode KonaVideoDevice::syncModeAtIndex(size_t i) const
    {
        const KonaSyncMode& m = syncModes[i];
        return SyncMode(m.desc);
    }

    void KonaVideoDevice::setSyncMode(size_t i)
    {
        const KonaSyncMode& m = syncModes[i];
        m_internalSyncMode = i;
        m_syncMode = m.value;
    }

    size_t KonaVideoDevice::currentSyncMode() const
    {
        return m_internalSyncMode;
    }

    size_t KonaVideoDevice::numSyncSources() const
    {
        return m_syncMode > 0 ? 2 : 0;
    }

    KonaVideoDevice::SyncSource
    KonaVideoDevice::syncSourceAtIndex(size_t i) const
    {
        const KonaSyncSource& m = syncSources[i];
        return SyncSource(m.desc);
    }

    void KonaVideoDevice::setSyncSource(size_t i)
    {
        const KonaSyncSource& m = syncSources[i];
        m_internalSyncSource = i;
        m_syncSource = m.value;
    }

    size_t KonaVideoDevice::currentSyncSource() const
    {
        return m_internalSyncSource;
    }

    size_t KonaVideoDevice::numVideo4KTransports() const
    {
        // Don't offer 4K transport choice if device doesn't support TSI
        if (!::NTV2DeviceCanDo425Mux(m_deviceID)
            && !::NTV2DeviceCanDo8KVideo(m_deviceID))
            return 0;

        return sizeof(video4KTransports) / sizeof(video4KTransports[0]);
    }

    KonaVideoDevice::Video4KTransport
    KonaVideoDevice::video4KTransportAtIndex(size_t i) const
    {
        const KonaVideo4KTransport& m = video4KTransports[i];
        return Video4KTransport(m.desc);
    }

    void KonaVideoDevice::setVideo4KTransport(size_t i)
    {
        const KonaVideo4KTransport& m = video4KTransports[i];
        m_internalVideo4KTransport = i;
        m_video4KTransport = m.value;
    }

    size_t KonaVideoDevice::currentVideo4KTransport() const
    {
        return m_internalVideo4KTransport;
    }

    void KonaVideoDevice::transferAudio(void* data, size_t n) const
    {
        int* inbuffer = (int*)data;
        VideoChannel* vc0 = m_videoChannels[0];
        FrameData& f0 = vc0->data[m_readBufferIndex];
        const KonaAudioFormat& format = audioFormats[m_internalAudioFormat];

        for (size_t ci = 0; ci < m_videoChannels.size(); ci++)
        {
            VideoChannel* vc = m_videoChannels[ci];
            FrameData& f = vc->data[m_readBufferIndex];

            AudioBuffer& buffer = f.audioBuffer;
            // const size_t  och      = vc->transfer.audioNumChannels;

            ULWord och(0);
            m_card->GetNumberAudioChannels(och);
            const size_t ich = format.numChannels;
            // m_card->AutoCirculateGetStatus(vc->channel, vc->auto_status);
            // const int     outframe = vc->auto_status.acActiveFrame;

            if (vc->auto_status.acState != NTV2_AUTOCIRCULATE_RUNNING)
                break;

            f.lockAudio("AUDIO");

            if (ci == 0)
            {
                if (n)
                {
                    buffer.resize(n * och);
                    if ((size_t)&buffer.front() & 0xf)
                        cout << "NOT ALIGNED" << endl;
                }
                else
                {
                    audioFrameSizeSequence(m_audioFrameSizes);
                    buffer.resize(m_audioFrameSizes[m_frameCount
                                                    % m_audioFrameSizes.size()]
                                  * och);
                    memset(&buffer.front(), 0, buffer.size() * sizeof(int));
                }

                for (size_t i = 0; i < n; i++)
                {
                    for (size_t c = 0; c < och; c++)
                    {
                        buffer[i * och + c] = inbuffer[i * ich + c % ich];
                    }
                }
            }
            else
            {
                //
                //  Copy channel 0 into the other channels
                //

                buffer.resize(f0.audioBuffer.size());
                if ((size_t)&buffer.front() & 0xf)
                    cout << "NOT ALIGNED" << endl;
                copy(f0.audioBuffer.begin(), f0.audioBuffer.end(),
                     buffer.begin());
            }

            f.unlockAudio();
        }
    }

    unsigned int
    KonaVideoDevice::channelsFromFormat(NTV2FrameBufferFormat f) const
    {
        switch (f)
        {
        case NTV2_FBF_10BIT_YCBCR:
        case NTV2_FBF_8BIT_YCBCR:
        case NTV2_FBF_24BIT_RGB:
        case NTV2_FBF_24BIT_BGR:
        case NTV2_FBF_48BIT_RGB:
            return 3;
        case NTV2_FBF_ARGB:
        case NTV2_FBF_RGBA:
        case NTV2_FBF_ABGR:
        case NTV2_FBF_16BIT_ARGB:
            return 4;
        case NTV2_FBF_10BIT_RGB:
        case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_8BIT_YCBCR_YUY2:
        case NTV2_FBF_10BIT_DPX_LE:
        case NTV2_FBF_10BIT_RGB_PACKED:
            return 1;

            // not yet supported
        default:
        case NTV2_FBF_10BIT_YCBCR_DPX:
        case NTV2_FBF_8BIT_DVCPRO:
        case NTV2_FBF_8BIT_YCBCR_420PL3:
        case NTV2_FBF_8BIT_HDV:
        case NTV2_FBF_10BIT_YCBCRA:
        case NTV2_FBF_PRORES_DVCPRO:
        case NTV2_FBF_PRORES_HDV:
        case NTV2_FBF_10BIT_ARGB:
            break;
        }

        return 0;
    }

    // Returns the HDMI output bit depth to use
    // Note that by default, the HDMI output bit depth is the same as the Video
    // Output data format. But this default behaviour can be overriden by the
    // user via the TWK_AJA_HDMI_OUTPUT_BIT_DEPTH_OVERRIDE environment variable
    // or the hdmi-output-bit-depth-override video output option.
    //
    NTV2HDMIBitDepth
    KonaVideoDevice::getHDMIOutBitDepth(const NTV2FrameBufferFormat f) const
    {
        if (m_hdmiOutputBitDepthOverride != 0)
        {
            switch (m_hdmiOutputBitDepthOverride)
            {
            case 8:
                return NTV2_HDMI8Bit;
            case 10:
                return NTV2_HDMI10Bit;
            case 12:
                return NTV2_HDMI12Bit;
            default:
                cerr << "WARNING: Invalid HDMI outbut bit depth override, "
                        "defaulting "
                        "to 8 bits"
                     << endl;
                return NTV2_HDMI8Bit;
            }
        }

        if (NTV2_IS_FBF_10BIT(f))
        {
            return NTV2_HDMI10Bit;
        }
        else if (NTV2_IS_FBF_12BIT_RGB(f))
        {
            return NTV2_HDMI12Bit;
        }

        return NTV2_HDMI8Bit;
    }

    void KonaVideoDevice::showAutoCirculateState(NTV2AutoCirculateState state,
                                                 int bufferLevel)
    {
        if (state != m_lastState && m_infoFeedback)
        {
            cout << "INFO: state = ";
            switch (state)
            {
            case NTV2_AUTOCIRCULATE_DISABLED:
                cout << "DISABLED";
                break;
            case NTV2_AUTOCIRCULATE_INIT:
                cout << "INIT";
                break;
            case NTV2_AUTOCIRCULATE_STARTING:
                cout << "STARTING";
                break;
            case NTV2_AUTOCIRCULATE_PAUSED:
                cout << "PAUSED";
                break;
            case NTV2_AUTOCIRCULATE_STOPPING:
                cout << "STOPPING";
                break;
            case NTV2_AUTOCIRCULATE_RUNNING:
                cout << "RUNNING";
                break;
            case NTV2_AUTOCIRCULATE_STARTING_AT_TIME:
                cout << "STARTING_AT_TIME";
                break;
            default:
                cout << "UNKNOWN=" << state;
                break;
            }

            cout << ", bl = " << bufferLevel << endl;

            m_lastBL = bufferLevel;
            m_lastState = state;
        }
    }

    void KonaVideoDevice::startGPUTransfer() const
    {
        if (m_profile)
            m_gpuBeginTime.push_back(m_globalTimer.elapsed());
    }

    void KonaVideoDevice::endGPUTransfer() const
    {
        if (m_profile)
            m_gpuTimes.push_back(m_globalTimer.elapsed()
                                 - m_gpuBeginTime.back());
    }

    void KonaVideoDevice::startAJATransfer() const
    {
        if (m_profile)
            m_konaBeginTime.push_back(m_globalTimer.elapsed());
    }

    void KonaVideoDevice::endAJATransfer() const
    {
        if (m_profile)
            m_konaTimes.push_back(m_globalTimer.elapsed()
                                  - m_konaBeginTime.back());
    }

    void KonaVideoDevice::packBufferCopy(unsigned char* src, size_t srcRowSize,
                                         unsigned char* dst, size_t dstRowSize,
                                         size_t rows)
    {
        for (size_t row = 0; row < rows; row++)
        {
            memcpy(dst + dstRowSize * row, src + dstRowSize * row, dstRowSize);
        }
    }

    void KonaVideoDevice::parseHDMIHDRMetadata(std::string data)
    {
        // Very, very basic and brutal parsing routine for the hdr
        // argument string.
        // No error checking, no validation, no plan B, data must be
        // exactly as expected or it won't work.
        //
        // Sample values (from AJA's BT2020 parameters):
        //    redPrimaryX:                    0.708000004
        //    redPrimaryY:                    0.291999996
        //    greenPrimaryX:                  0.170000002
        //    greenPrimaryY:                  0.79699999
        //    bluePrimaryX:                   0.130999997
        //    bluePrimaryY:                   0.0460000001
        //    whitePointX:                    0.312700003
        //    whitePointY:                    0.328999996
        //    minMasteringLuminance:          0.00499999989
        //    maxMasteringLuminance:          10000
        //    maxContentLightLevel:           0
        //    maxFrameAverageLightLevel:      0
        //    electroOpticalTransferFunction: 2
        //    staticMetadataDescriptorID:     0
        //
        // Expected format (to be entered in the "Additional Options"
        // box of the Video preferences panel):
        //
        // --hdmi-hdr-metadata=0.708000004,0.291999996,0.170000002,0.79699999,0.130999997,0.0460000001,0.312700003,0.328999996,0.00499999989,10000,0,0,2,0
        //
        // or via env var:
        //
        // setenv TWK_AJA_HDMI_HDR_METADATA
        // "0.708000004,0.291999996,0.170000002,0.79699999,0.130999997,0.0460000001,0.312700003,0.328999996,0.00499999989,10000,0,0,2,0"
        //
        // (the types - float vs int - are important!)

        if (m_infoFeedback)
            cout << "INFO: HDMI HDR Metadata = " << data << endl;

        // redPrimaryX
        size_t pos = data.find(",");
        m_hdrMetadata.redPrimaryX = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // redPrimaryY
        pos = data.find(",");
        m_hdrMetadata.redPrimaryY = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // greenPrimaryX
        pos = data.find(",");
        m_hdrMetadata.greenPrimaryX = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // greenPrimaryY
        pos = data.find(",");
        m_hdrMetadata.greenPrimaryY = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // bluePrimaryX
        pos = data.find(",");
        m_hdrMetadata.bluePrimaryX = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // bluePrimaryY
        pos = data.find(",");
        m_hdrMetadata.bluePrimaryY = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // whitePointX
        pos = data.find(",");
        m_hdrMetadata.whitePointX = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // whitePointY
        pos = data.find(",");
        m_hdrMetadata.whitePointY = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // minMasteringLuminance
        pos = data.find(",");
        m_hdrMetadata.minMasteringLuminance = atof(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // maxMasteringLuminance
        pos = data.find(",");
        m_hdrMetadata.maxMasteringLuminance = atoi(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // maxContentLightLevel
        pos = data.find(",");
        m_hdrMetadata.maxContentLightLevel = atoi(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // maxFrameAverageLightLevel
        pos = data.find(",");
        m_hdrMetadata.maxFrameAverageLightLevel =
            atoi(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // electroOpticalTransferFunction
        pos = data.find(",");
        m_hdrMetadata.electroOpticalTransferFunction =
            atoi(data.substr(0, pos).c_str());
        data.erase(0, pos + 1);

        // staticMetadataDescriptorID
        pos = data.find(",");
        m_hdrMetadata.staticMetadataDescriptorID =
            atoi(data.substr(0, pos).c_str());
    }

} // namespace AJADevices
