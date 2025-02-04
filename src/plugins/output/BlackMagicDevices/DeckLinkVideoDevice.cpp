//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <BlackMagicDevices/DeckLinkVideoDevice.h>
#include <BlackMagicDevices/BlackMagicModule.h>
#include <BlackMagicDevices/StereoVideoFrame.h>
#include <TwkExc/Exception.h>
#include <TwkFB/FastMemcpy.h>
#include <TwkFB/FastConversion.h>
#include <TwkGLF/GLFBO.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <boost/program_options.hpp> // This has to come before the ByteSwap.h
#include <TwkUtil/ByteSwap.h>
#include <algorithm>
#include <string>
#include <stl_ext/replace_alloc.h>
#include <TwkGLF/GL.h>

// This is required for 10 bit YUV with the ultastudio mini
#define DEFAULT_RINGBUFFER_SIZE 5

namespace
{
    using namespace std;

    void checkForFalse(bool b, int line)
    {
        if (!b)
            cout << "ERROR: BM_CHECK FAILED: at line " << line << endl;
    }

#define BM_CHECK(T) checkForFalse(T, __LINE__);

} // namespace

namespace BlackMagicDevices
{
    using namespace std;
    using namespace TwkApp;
    using namespace TwkGLF;
    using namespace TwkUtil;
    using namespace boost::program_options;

    pthread_mutex_t videoMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

    bool DeckLinkVideoDevice::m_infoFeedback = false;

// the following number must be positive and even.
// the reason it has to be even is because in case of 3D (stereo), we will take
// 2 consecutive frames and create a 3D frame from it, and reuse these 3D frames
// for optimization purposes
#define VIDEOBUFFERSIZE 8

    static ENVVAR_BOOL(evDetectSkippedFrames, "RV_DETECT_SKIPPED_FRAMES",
                       false);

    const char* const HDMI_HDR_METADATA_ARG = "hdmi-hdr-metadata";

    struct DeckLinkSyncMode
    {
        const char* desc;
        size_t value;
    };

    struct DeckLinkSyncSource
    {
        const char* desc;
        size_t value;
    };

    static DeckLinkDataFormat dataFormats[] = {
        {"8 Bit YUV", bmdFormat8BitYUV, VideoDevice::Y0CbY1Cr_8_422, false},
        {"10 Bit YUV", bmdFormat10BitYUV, VideoDevice::YCrCb_BM_10_422, false},
        //{"8 Bit ARGB", bmdFormat8BitARGB, VideoDevice::BGRA8, true},
        //{"8 Bit BGRA", bmdFormat8BitBGRA, VideoDevice::BGRA8, true}, //
        // CHANGED
        // NAME
        {"8 Bit RGBA", bmdFormat8BitBGRA, VideoDevice::BGRA8, true},
        //{"10 Bit RGB", bmdFormat10BitRGB, VideoDevice::RGB10X2, true},
        //{"10 Bit RGBX", bmdFormat10BitRGBX, VideoDevice::RGB10X2, true},
        //{"10 Bit RGBXLE", bmdFormat10BitRGBXLE, VideoDevice::RGB10X2, true},
        ////
        // CHANGED NAME
        {"10 Bit RGB", bmdFormat10BitRGBXLE, VideoDevice::RGB10X2, true},
        {"12 Bit RGB", bmdFormat12BitRGBLE, VideoDevice::RGB16, true},

        {"Stereo 8 Bit YUV", bmdFormat8BitYUV, VideoDevice::Y0CbY1Cr_8_422,
         false},
        {"Stereo 10 Bit YUV", bmdFormat10BitYUV, VideoDevice::YCrCb_BM_10_422,
         false},
        //{"Stereo 8 Bit ARGB", bmdFormat8BitARGB, VideoDevice::BGRA8, true},
        //{"Stereo 8 Bit BGRA", bmdFormat8BitBGRA, VideoDevice::BGRA8, true}, //
        // CHANGED NAME
        {"Stereo 8 Bit RGBA", bmdFormat8BitBGRA, VideoDevice::BGRA8, true},
        //{"Stereo 10 Bit RGB", bmdFormat10BitRGB, VideoDevice::RGB10X2, true},
        //{"Stereo 10 Bit RGBX", bmdFormat10BitRGBX, VideoDevice::RGB10X2,
        // true},
        //{"Stereo 10 Bit RGBXLE", bmdFormat10BitRGBXLE, VideoDevice::RGB10X2,
        // true}, // CHANGED NAME
        {"Stereo 10 Bit RGB", bmdFormat10BitRGBXLE, VideoDevice::RGB10X2, true},
        {"Stereo 12 Bit RGB", bmdFormat12BitRGBLE, VideoDevice::RGB16, true},
        {NULL, bmdFormat8BitBGRA, VideoDevice::BGRA8, true}};

    //
    // The 1556p formats are commented out. None of our Declink cards seem
    // to support it.
    //
    DeckLinkVideoFormat videoFormats[] = {
        {1280, 720, 1.0, 50.00, "720p 50Hz", bmdModeHD720p50},
        {1280, 720, 1.0, 59.94, "720p 59.94Hz", bmdModeHD720p5994},
        {1280, 720, 1.0, 60.00, "720p 60Hz", bmdModeHD720p60},
        {1920, 1080, 1.0, 25.00, "1080i 50Hz", bmdModeHD1080i50},
        {1920, 1080, 1.0, 29.97, "1080i 59.94Hz", bmdModeHD1080i5994},
        {1920, 1080, 1.0, 30.00, "1080i 60Hz", bmdModeHD1080i6000},
        {1920, 1080, 1.0, 23.98, "1080p 23.98Hz", bmdModeHD1080p2398},
        {1920, 1080, 1.0, 24.00, "1080p 24Hz", bmdModeHD1080p24},
        {1920, 1080, 1.0, 25.00, "1080p 25Hz", bmdModeHD1080p25},
        {1920, 1080, 1.0, 29.97, "1080p 29.97Hz", bmdModeHD1080p2997},
        {1920, 1080, 1.0, 30.00, "1080p 30Hz", bmdModeHD1080p30},
        {1920, 1080, 1.0, 50.00, "1080p 50Hz", bmdModeHD1080p50},
        {1920, 1080, 1.0, 59.94, "1080p 59.94Hz", bmdModeHD1080p5994},
        {1920, 1080, 1.0, 60.00, "1080p 60Hz", bmdModeHD1080p6000},
        {2048, 1080, 1.0, 23.98, "1080p (2048x1080) DCI 2K 23.98Hz",
         bmdMode2kDCI2398},
        {2048, 1080, 1.0, 24.00, "1080p (2048x1080) DCI 2K 24Hz",
         bmdMode2kDCI24},
        {2048, 1080, 1.0, 25.00, "1080p (2048x1080) DCI 2K 25Hz",
         bmdMode2kDCI25},
        {2048, 1080, 1.0, 29.97, "1080p (2048x1080) DCI 2K 29.97Hz",
         bmdMode2kDCI2997},
        {2048, 1080, 1.0, 30.00, "1080p (2048x1080) DCI 2K 30Hz",
         bmdMode2kDCI30},
        {2048, 1080, 1.0, 50.00, "1080p (2048x1080) DCI 2K 50Hz",
         bmdMode2kDCI50},
        {2048, 1080, 1.0, 59.94, "1080p (2048x1080) DCI 2K 59.94Hz",
         bmdMode2kDCI5994},
        {2048, 1080, 1.0, 60.00, "1080p (2048x1080) DCI 2K 60Hz",
         bmdMode2kDCI60},
        {3840, 2160, 1.0, 23.98, "2160p (3840x2160) UHD 4K 23.98Hz",
         bmdMode4K2160p2398},
        {3840, 2160, 1.0, 24.00, "2160p (3840x2160) UHD 4K 24Hz",
         bmdMode4K2160p24},
        {3840, 2160, 1.0, 25.00, "2160p (3840x2160) UHD 4K 25Hz",
         bmdMode4K2160p25},
        {3840, 2160, 1.0, 29.97, "2160p (3840x2160) UHD 4K 29.97Hz",
         bmdMode4K2160p2997},
        {3840, 2160, 1.0, 30.00, "2160p (3840x2160) UHD 4K 30Hz",
         bmdMode4K2160p30},
        {3840, 2160, 1.0, 50.00, "2160p (3840x2160) UHD 4K 50Hz",
         bmdMode4K2160p50},
        {3840, 2160, 1.0, 59.94, "2160p (3840x2160) UHD 4K 59.94Hz",
         bmdMode4K2160p5994},
        {3840, 2160, 1.0, 60.00, "2160p (3840x2160) UHD 4K 60Hz",
         bmdMode4K2160p60},
        {4096, 2160, 1.0, 23.98, "2160p (4096x2160) DCI 4K 23.98Hz",
         bmdMode4kDCI2398},
        {4096, 2160, 1.0, 24.00, "2160p (4096x2160) DCI 4K 24Hz",
         bmdMode4kDCI24},
        {4096, 2160, 1.0, 25.00, "2160p (4096x2160) DCI 4K 25Hz",
         bmdMode4kDCI25},
        {4096, 2160, 1.0, 29.97, "2160p (4096x2160) DCI 4K 29.97Hz",
         bmdMode4kDCI2997},
        {4096, 2160, 1.0, 30.00, "2160p (4096x2160) DCI 4K 30Hz",
         bmdMode4kDCI30},
        {4096, 2160, 1.0, 50.00, "2160p (4096x2160) DCI 4K 50Hz",
         bmdMode4kDCI50},
        {4096, 2160, 1.0, 59.94, "2160p (4096x2160) DCI 4K 59.94Hz",
         bmdMode4kDCI5994},
        {4096, 2160, 1.0, 60.00, "2160p (4096x2160) DCI 4K 60Hz",
         bmdMode4kDCI60},
        {7680, 4320, 1.0, 23.98, "4320p (7680x4320) UHD 8K 23.98Hz",
         bmdMode8K4320p2398},
        {7680, 4320, 1.0, 24.00, "4320p (7680x4320) UHD 8K 24Hz",
         bmdMode8K4320p24},
        {7680, 4320, 1.0, 25.00, "4320p (7680x4320) UHD 8K 25Hz",
         bmdMode8K4320p25},
        {7680, 4320, 1.0, 29.97, "4320p (7680x4320) UHD 8K 29.97Hz",
         bmdMode8K4320p2997},
        {7680, 4320, 1.0, 30.00, "4320p (7680x4320) UHD 8K 30Hz",
         bmdMode8K4320p30},
        {7680, 4320, 1.0, 50.00, "4320p (7680x4320) UHD 8K 50Hz",
         bmdMode8K4320p50},
        {7680, 4320, 1.0, 59.94, "4320p (7680x4320) UHD 8K 59.94Hz",
         bmdMode8K4320p5994},
        {7680, 4320, 1.0, 60.00, "4320p (7680x4320) UHD 8K 60Hz",
         bmdMode8K4320p60},
        {8192, 4320, 1.0, 23.98, "4320p (8192x4320) DCI 8K 23.98Hz",
         bmdMode8kDCI2398},
        {8192, 4320, 1.0, 24.00, "4320p (8192x4320) DCI 8K 24Hz",
         bmdMode8kDCI24},
        {8192, 4320, 1.0, 25.00, "4320p (8192x4320) DCI 8K 25Hz",
         bmdMode8kDCI25},
        {8192, 4320, 1.0, 29.97, "4320p (8192x4320) DCI 8K 29.97Hz",
         bmdMode8kDCI2997},
        {8192, 4320, 1.0, 30.00, "4320p (8192x4320) DCI 8K 30Hz",
         bmdMode8kDCI30},
        {8192, 4320, 1.0, 50.00, "4320p (8192x4320) DCI 8K 50Hz",
         bmdMode8kDCI50},
        {8192, 4320, 1.0, 59.94, "4320p (8192x4320) DCI 8K 59.94Hz",
         bmdMode8kDCI5994},
        {8192, 4320, 1.0, 60.00, "4320p (8192x4320) DCI 8K 60Hz",
         bmdMode8kDCI60},
        {1920, 1080, 1.0, 47.95, "1080p 47.95Hz", bmdModeHD1080p4795},
        {2048, 1080, 1.0, 47.95, "1080p (2048x1080) DCI 2K 47.95Hz",
         bmdMode2kDCI4795},
        {3840, 2160, 1.0, 47.95, "2160p (3840x2160) UHD 4K 47.95Hz",
         bmdMode4K2160p4795},
        {4096, 2160, 1.0, 47.95, "2160p (4096x2160) DCI 4K 47.95Hz",
         bmdMode4kDCI4795},
        {7680, 4320, 1.0, 47.95, "4320p (7680x4320) UHD 8K 47.95Hz",
         bmdMode8K4320p4795},
        {8192, 4320, 1.0, 47.95, "4320p (8192x4320) DCI 8K 47.95Hz",
         bmdMode8kDCI4795},
        {1920, 1080, 1.0, 48.00, "1080p 48Hz", bmdModeHD1080p48},
        {2048, 1080, 1.0, 48.00, "1080p (2048x1080) DCI 2K 48Hz",
         bmdMode2kDCI48},
        {3840, 2160, 1.0, 48.00, "2160p (3840x2160) UHD 4K 48Hz",
         bmdMode4K2160p48},
        {4096, 2160, 1.0, 48.00, "2160p (4096x2160) DCI 4K 48Hz",
         bmdMode4kDCI48},
        {7680, 4320, 1.0, 48.00, "4320p (7680x4320) UHD 8K 48Hz",
         bmdMode8K4320p48},
        {8192, 4320, 1.0, 48.00, "4320p (8192x4320) DCI 8K 48Hz",
         bmdMode8kDCI48},
        {0, 0, 1.0, 00.00, NULL, bmdModeHD720p50},
    };

    static DeckLinkAudioFormat audioFormats[] = {
        {48000.0, bmdAudioSampleRate48kHz, TwkAudio::Int16Format,
         bmdAudioSampleType16bitInteger, 2, TwkAudio::Stereo_2,
         "16 bit 48kHz Stereo"},
        {48000.0, bmdAudioSampleRate48kHz, TwkAudio::Int32Format,
         bmdAudioSampleType32bitInteger, 2, TwkAudio::Stereo_2,
         "24 bit 48kHz Stereo"},
        {48000.0, bmdAudioSampleRate48kHz, TwkAudio::Int32Format,
         bmdAudioSampleType32bitInteger, 8, TwkAudio::Surround_7_1,
         "24 bit 48kHz 7.1 Surround"},
        {48000.0, bmdAudioSampleRate48kHz, TwkAudio::Int32Format,
         bmdAudioSampleType32bitInteger, 8, TwkAudio::SDDS_7_1,
         "24 bit 48kHz 7.1 Surround SDDS"},
        {48000.0, bmdAudioSampleRate48kHz, TwkAudio::Int32Format,
         bmdAudioSampleType32bitInteger, 16, TwkAudio::Generic_16,
         "24 bit 48kHz 16 channel"}};

    // 4K/8K Transport combo box items indices
    // Note: VIDEO_TRANSPORT_DEFAULT = SingleLink but kept for backward
    // compatibility
    const size_t VIDEO_TRANSPORT_DEFAULT = 0;
    const size_t VIDEO_TRANSPORT_SINGLE_LINK = 1;
    const size_t VIDEO_TRANSPORT_DUAL_LINK = 2;
    const size_t VIDEO_TRANSPORT_QUAD_LINK = 3;
    const size_t VIDEO_TRANSPORT_QUADRANTS = 4;

    DeckLinkVideo4KTransport video4KTransports[] = {
        {"Default", VIDEO_TRANSPORT_DEFAULT},
        {"Single Link", VIDEO_TRANSPORT_SINGLE_LINK},
        {"Dual Link", VIDEO_TRANSPORT_DUAL_LINK},
        {"Quad Link", VIDEO_TRANSPORT_QUAD_LINK},
        {"Quad Link-Square Division Quad Split mode",
         VIDEO_TRANSPORT_QUADRANTS},
    };

    BMDSupportedVideoModeFlags
    getBmdSupportedVideoModeFlag(const size_t in_video4KTransport)
    {
        BMDSupportedVideoModeFlags bmdSupportedVideoModeFlag =
            bmdSupportedVideoModeDefault;
        switch (in_video4KTransport)
        {
        case VIDEO_TRANSPORT_DEFAULT:
        case VIDEO_TRANSPORT_SINGLE_LINK:
            bmdSupportedVideoModeFlag = bmdSupportedVideoModeSDISingleLink;
            break;
        case VIDEO_TRANSPORT_DUAL_LINK:
            bmdSupportedVideoModeFlag = bmdSupportedVideoModeSDIDualLink;
            break;
        case VIDEO_TRANSPORT_QUAD_LINK:
        case VIDEO_TRANSPORT_QUADRANTS:
            bmdSupportedVideoModeFlag = bmdSupportedVideoModeSDIQuadLink;
            break;
        }

        return bmdSupportedVideoModeFlag;
    }

    int getBmdLinkConfiguration(const size_t in_video4KTransport,
                                int& out_bmdQuadLinkSquareDivisionSplit)
    {
        int bmdLinkConfiguration = bmdLinkConfigurationSingleLink;
        out_bmdQuadLinkSquareDivisionSplit = -1;
        switch (in_video4KTransport)
        {
        case VIDEO_TRANSPORT_DEFAULT:
        case VIDEO_TRANSPORT_SINGLE_LINK:
            bmdLinkConfiguration = bmdLinkConfigurationSingleLink;
            break;
        case VIDEO_TRANSPORT_DUAL_LINK:
            bmdLinkConfiguration = bmdLinkConfigurationDualLink;
            break;
        case VIDEO_TRANSPORT_QUAD_LINK:
            bmdLinkConfiguration = bmdLinkConfigurationQuadLink;
            out_bmdQuadLinkSquareDivisionSplit = 0;
            break;
        case VIDEO_TRANSPORT_QUADRANTS:
            bmdLinkConfiguration = bmdLinkConfigurationQuadLink;
            out_bmdQuadLinkSquareDivisionSplit = 1;
            break;
        }

        return bmdLinkConfiguration;
    }

    static DeckLinkSyncMode syncModes[] = {{"Free Running", 0}};

    static DeckLinkSyncSource syncSources[] = {{"Composite", 0}, {"SDI", 1}};

    DeckLinkVideoDevice::PBOData::PBOData(GLuint g)
        : globject(g)
        , state(PBOData::State::Ready)
    {
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&stateMutex, NULL);
    }

    DeckLinkVideoDevice::PBOData::~PBOData()
    {
        pthread_mutex_destroy(&mutex);
        pthread_mutex_destroy(&stateMutex);
    }

    void DeckLinkVideoDevice::PBOData::lockData()
    {
        Timer timer;
        timer.start();
        pthread_mutex_lock(&mutex);
        Time t = timer.elapsed();
        if (t > 0.001)
            cout << "lockData for " << t << endl;
    }

    void DeckLinkVideoDevice::PBOData::unlockData()
    {
        pthread_mutex_unlock(&mutex);
    }

    void DeckLinkVideoDevice::PBOData::lockState()
    {
        Timer timer;
        timer.start();
        pthread_mutex_lock(&stateMutex);
        Time t = timer.elapsed();
        if (t > 0.001)
            cout << "lockState for " << t << endl;
    }

    void DeckLinkVideoDevice::PBOData::unlockState()
    {
        pthread_mutex_unlock(&stateMutex);
    }

    //
    //  DeckLinkVideoDevice
    //
    DeckLinkVideoDevice::DeckLinkVideoDevice(BlackMagicModule* m,
                                             const std::string& name,
                                             IDeckLink* d, IDeckLinkOutput* p)
        : GLBindableVideoDevice(m, name,
                                BlockingTransfer | ImageOutput | ProvidesSync
                                    | FixedResolution | Clock | AudioOutput
                                    | NormalizedCoordinates)
        , m_readyFrame(NULL)
        , m_readyStereoFrame(NULL)
        , m_needsFrameConverter(false)
        , m_hasAudio(false)
        , m_deviceAPI(d)
        , m_outputAPI(p)
        , m_readPixelFormat(bmdFormat8BitBGRA)
        , m_initialized(false)
        , m_pbos(true)
        , m_pboSize(DEFAULT_RINGBUFFER_SIZE)
        , m_videoFrameBufferSize(DEFAULT_RINGBUFFER_SIZE)
        , m_open(false)
        , m_stereo(false)
        , m_supportsStereo(false)
        , m_totalPlayoutFrames(0)
        , m_transferTextureID(-1)
        , m_internalAudioFormat(0)
        , m_internalVideoFormat(0)
        , m_internalVideo4KTransport(VIDEO_TRANSPORT_SINGLE_LINK)
        , m_internalDataFormat(0)
        , m_internalSyncMode(0)
        , m_internalSyncSource(0)
        , m_frameCompleted(true)
        , m_configuration(NULL)
    {
        m_audioFrameSizes.resize(5);

        for (int i = 0; i < NB_AUDIO_BUFFERS; i++)
        {
            m_audioData[i] = nullptr;
        }

        //
        //  Add in all supported video formats
        //
        //  NOTE: according to Blackmagic SDK, if pixel format + video
        //  format combo are supported then so are the same combo in 3D
        //  mode (stereo)
        //

        for (const DeckLinkVideoFormat* p = videoFormats; p->desc; p++)
        {
            bool videoFormatSupported = false;

            for (const DeckLinkDataFormat* q = dataFormats; q->desc; q++)
            {
                string description = q->desc;
                bool isStereo = description.find("Stereo") != string::npos;

#ifdef PLATFORM_WINDOWS
                BOOL supported = false;
#else
                bool supported = false;
#endif

                BMDDisplayMode displayMode;
                HRESULT hr = m_outputAPI->DoesSupportVideoMode(
                    bmdVideoConnectionUnspecified, p->value, q->value,
                    bmdNoVideoOutputConversion,
                    isStereo ? bmdSupportedVideoModeDualStream3D
                             : bmdSupportedVideoModeDefault,
                    &displayMode, &supported);

                if (hr == S_OK && supported)
                {
                    videoFormatSupported = true;

                    if (p == videoFormats) // first one
                    {
                        m_decklinkDataFormats.push_back(*q);
                    }
                }
            }

            if (videoFormatSupported)
                m_decklinkVideoFormats.push_back(*p);
        }

        // Get the IDeckLinkConfiguration interface
        unsigned int result = m_deviceAPI->QueryInterface(
            IID_IDeckLinkConfiguration, (void**)&m_configuration);
        if (result != S_OK)
        {
            m_configuration = NULL;
            TWK_THROW_EXC_STREAM(
                "Could not get the IDeckLinkConfiguration interface (error 0x"
                << hex << result << ")\n");
        }
    }

    DeckLinkVideoDevice::~DeckLinkVideoDevice()
    {
        if (m_open)
        {
            close();
        }

        if (m_outputAPI != NULL)
        {
            m_outputAPI->Release();
            m_outputAPI = NULL;
        }

        if (m_configuration)
        {
            m_configuration->Release();
            m_configuration = NULL;
        }
    }

    size_t DeckLinkVideoDevice::asyncMaxMappedBuffers() const
    {
        return m_pboSize;
    }

    VideoDevice::Time DeckLinkVideoDevice::deviceLatency() const
    {
        return Time(0);
    }

    size_t DeckLinkVideoDevice::numDataFormats() const
    {
        return m_decklinkDataFormats.size();
    }

    DeckLinkVideoDevice::DataFormat
    DeckLinkVideoDevice::dataFormatAtIndex(size_t i) const
    {
        return DataFormat(m_decklinkDataFormats[i].iformat,
                          m_decklinkDataFormats[i].desc);
    }

    void DeckLinkVideoDevice::setDataFormat(size_t i)
    {
        const DeckLinkDataFormat& f = m_decklinkDataFormats[i];
        m_internalDataFormat = i;
    }

    size_t DeckLinkVideoDevice::currentDataFormat() const
    {
        return m_internalDataFormat;
    }

    size_t DeckLinkVideoDevice::numAudioFormats() const { return 5; }

    DeckLinkVideoDevice::AudioFormat
    DeckLinkVideoDevice::audioFormatAtIndex(size_t index) const
    {
        const DeckLinkAudioFormat& f = audioFormats[index];
        return AudioFormat(f.hz, f.prec, f.numChannels, f.layout, f.desc);
    }

    size_t DeckLinkVideoDevice::currentAudioFormat() const
    {
        return m_internalAudioFormat;
    }

    void DeckLinkVideoDevice::setAudioFormat(size_t i)
    {
        if (i > numAudioFormats())
            i = numAudioFormats() - 1;
        const DeckLinkAudioFormat& f = audioFormats[i];
        m_internalAudioFormat = i;

        m_audioFrameSizes.clear();
    }

    size_t DeckLinkVideoDevice::numVideoFormats() const
    {
        return m_decklinkVideoFormats.size();
    }

    DeckLinkVideoDevice::VideoFormat
    DeckLinkVideoDevice::videoFormatAtIndex(size_t index) const
    {
        const DeckLinkVideoFormat& f = m_decklinkVideoFormats[index];
        return VideoFormat(f.width, f.height, f.pa, 1.0f, f.hz, f.desc);
    }

    size_t DeckLinkVideoDevice::currentVideoFormat() const
    {
        return m_internalVideoFormat;
    }

    void DeckLinkVideoDevice::setVideoFormat(size_t i)
    {
        const size_t n = numVideoFormats();
        if (i >= n)
            i = n - 1;
        const DeckLinkVideoFormat& f = m_decklinkVideoFormats[i];
        m_internalVideoFormat = i;

        //
        //  Update the data formats based on the video format
        //  NOTE: stereo has to be checked explicitly
        //

        m_decklinkDataFormats.clear();
        const DeckLinkVideoFormat* p = &f;

        for (const DeckLinkDataFormat* q = dataFormats; q->desc; q++)
        {
            string description = q->desc;
            bool isStereo = description.find("Stereo") != string::npos;

            BMDDisplayMode displayMode;

#ifdef PLATFORM_WINDOWS
            BOOL supported = false;
#else
            bool supported = false;
#endif

            HRESULT hr = m_outputAPI->DoesSupportVideoMode(
                bmdVideoConnectionUnspecified, p->value, q->value,
                bmdNoVideoOutputConversion,
                isStereo
                    ? bmdSupportedVideoModeDualStream3D
                    : getBmdSupportedVideoModeFlag(m_internalVideo4KTransport),
                &displayMode, &supported);

            if (hr == S_OK && supported)
            {
                m_decklinkDataFormats.push_back(*q);
            }
        }

        m_audioFrameSizes.clear();
    }

    size_t DeckLinkVideoDevice::numVideo4KTransports() const
    {
        return sizeof(video4KTransports) / sizeof(video4KTransports[0]);
    }

    DeckLinkVideoDevice::Video4KTransport
    DeckLinkVideoDevice::video4KTransportAtIndex(size_t i) const
    {
        const DeckLinkVideo4KTransport& m = video4KTransports[i];
        return Video4KTransport(m.desc);
    }

    void DeckLinkVideoDevice::setVideo4KTransport(size_t i)
    {
        const DeckLinkVideo4KTransport& m = video4KTransports[i];
        m_internalVideo4KTransport = i;
    }

    size_t DeckLinkVideoDevice::currentVideo4KTransport() const
    {
        return m_internalVideo4KTransport;
    }

    DeckLinkVideoDevice::Timing DeckLinkVideoDevice::timing() const
    {
        const DeckLinkVideoFormat& f =
            m_decklinkVideoFormats[m_internalVideoFormat];
        return DeckLinkVideoDevice::Timing(f.hz);
    }

    DeckLinkVideoDevice::VideoFormat DeckLinkVideoDevice::format() const
    {
        const DeckLinkVideoFormat& f =
            m_decklinkVideoFormats[m_internalVideoFormat];
        return DeckLinkVideoDevice::VideoFormat(f.width, f.height, f.pa, 1.0f,
                                                f.hz, f.desc);
    }

    size_t DeckLinkVideoDevice::numSyncModes() const { return 1; }

    DeckLinkVideoDevice::SyncMode
    DeckLinkVideoDevice::syncModeAtIndex(size_t i) const
    {
        const DeckLinkSyncMode& m = syncModes[i];
        return SyncMode(m.desc);
    }

    void DeckLinkVideoDevice::setSyncMode(size_t i)
    {
        const DeckLinkSyncMode& m = syncModes[i];
        m_internalSyncMode = i;
    }

    size_t DeckLinkVideoDevice::currentSyncMode() const
    {
        return m_internalSyncMode;
    }

    size_t DeckLinkVideoDevice::numSyncSources() const
    {
        // return 2;
        return 0;
    }

    DeckLinkVideoDevice::SyncSource
    DeckLinkVideoDevice::syncSourceAtIndex(size_t i) const
    {
        const DeckLinkSyncSource& m = syncSources[i];
        return SyncSource(m.desc);
    }

    void DeckLinkVideoDevice::setSyncSource(size_t i)
    {
        const DeckLinkSyncSource& m = syncSources[i];
        m_internalSyncSource = i;
    }

    size_t DeckLinkVideoDevice::currentSyncSource() const
    {
        return m_internalSyncSource;
    }

    void DeckLinkVideoDevice::audioFrameSizeSequence(
        AudioFrameSizeVector& fsizes) const
    {
        // same as AJA
        const DeckLinkVideoFormat& f =
            m_decklinkVideoFormats[m_internalVideoFormat];

        m_audioFrameSizes.resize(5);
        fsizes.resize(5);
        for (size_t i = 0; i < 5; i++)
        {
            m_audioFrameSizes[i] = m_audioSamplesPerFrame;
            fsizes[i] = m_audioFrameSizes[i];
        }
    }

    void DeckLinkVideoDevice::initialize()
    {
        if (m_initialized)
            return;

        m_initialized = true;
    }

    namespace
    {
        string mapToEnvVar(string name)
        {
            if (name == "TWK_BLACKMAGIC_HELP")
                return "help";
            if (name == "TWK_BLAKCMAGIC_VERBOSE")
                return "verbose";
            if (name == "TWK_BLAKCMAGIC_METHOD")
                return "method";
            if (name == "TWK_BLACKMAGIC_RING_BUFFER_SIZE")
                return "ring-buffer-size";
            if (name == "TWK_BLACKMAGIC_HDMI_HDR_METADATA")
                return HDMI_HDR_METADATA_ARG;
            return "";
        }
    } // namespace

    void DeckLinkVideoDevice::open(const StringVector& args)
    {
        if (!m_initialized)
            initialize();

        options_description desc("BlackMagic Device Options");

        desc.add_options()("help,h", "Usage Message")("verbose,v", "Verbose")(
            "method,m", value<string>(), "Method (ipbo, basic)")(
            "ring-buffer-size,s",
            value<int>()->default_value(DEFAULT_RINGBUFFER_SIZE),
            "Ring Buffer Size")(
            HDMI_HDR_METADATA_ARG, value<string>(),
            "HDMI HDR Metadata - comma-separated values - all floats except "
            "eotf "
            "which is an int: "
            "rx,ry,gx,gy,bx,by,wx,wy,minML,maxML,mCLL,mFALL,eotf");

        variables_map vm;

        try
        {
            store(command_line_parser(args).options(desc).run(), vm);
            store(parse_environment(desc, mapToEnvVar), vm);
            notify(vm);
        }
        catch (std::exception& e)
        {
            cout << "ERROR: BlackMagic_ARGS: " << e.what() << endl;
        }
        catch (...)
        {
            cout << "ERROR: BlackMagic_ARGS: exception" << endl;
        }

        if (vm.count("help") > 0)
        {
            cout << endl << desc << endl;
        }

        m_infoFeedback = vm.count("verbose") > 0;

        if (vm.count("ring-buffer-size"))
        {
            int rc = vm["ring-buffer-size"].as<int>();
            m_pboSize = rc;
            cout << "INFO: ringbuffer size " << rc << endl;
        }

        bool PBOsOK = true;
        if (vm.count("method"))
        {
            string s = vm["method"].as<string>();

            if (s == "ipbo")
            {
                PBOsOK = true;
            }
            else if (s == "basic")
            {
                PBOsOK = false;
            }
        }

        m_pbos = PBOsOK;

        if (m_pbos)
            cout << "INFO: using PBOs with immediate copy" << endl;
        else
            cout << "INFO: using basic readback" << endl;

        // HDR metada support
        m_useHDRMetadata = vm.count(HDMI_HDR_METADATA_ARG) > 0;
        if (m_useHDRMetadata)
        {
            const string hdrMetadata = vm[HDMI_HDR_METADATA_ARG].as<string>();

            if (m_infoFeedback)
            {
                cout << "INFO: BMD HDR Metadata input = " << hdrMetadata
                     << endl;
            }

            // Parse the HDR metadata provided and save it as a static in the
            // HDRVideoFrame class
            // Note that the same HDR metadata will be used for all the video
            // frames in the queue
            bool parsingSuccessful = HDRVideoFrame::SetHDRMetadata(hdrMetadata);
            if (parsingSuccessful)
            {
                if (m_infoFeedback)
                {
                    cout << "INFO: BMD HDR Metadata parsing successful ="
                         << endl
                         << HDRVideoFrame::DumpHDRMetadata() << endl;
                }
            }
            else
            {
                m_useHDRMetadata = false;
                if (m_infoFeedback)
                {
                    cout << "INFO: BMD HDR Metadata parsing error. HDR "
                            "metadata will not "
                            "be used"
                         << endl;
                }
            }
        }

        m_audioDataIndex = 0;
        m_firstThreeCounter = 0;
        m_frameCount = 0;
        m_totalPlayoutFrames = 0;
        m_lastPboData = NULL;
        m_secondLastPboData = NULL;

        IDeckLinkDisplayModeIterator* pDLDisplayModeIterator = NULL;
        IDeckLinkDisplayMode* pDLDisplayMode = NULL;

        const DeckLinkVideoFormat& v =
            m_decklinkVideoFormats[m_internalVideoFormat];
        BMDDisplayMode displayMode = v.value;

        // dynamically determine what pixel formats are supported based on the
        // desired video format
        m_decklinkDataFormats.clear();

        // play black frames when idle
        if (m_configuration->SetInt(bmdDeckLinkConfigVideoOutputIdleOperation,
                                    bmdIdleVideoOutputBlack)
            != S_OK)
        {
            TWK_THROW_EXC_STREAM("Cannot set decklink configuration\n");
        }

        // Set Video Transport (SingleLink/DualLink/QuadLink)
        int bmdQuadLinkSquareDivisionSplit = -1;
        int bmdLinkConfiguration = getBmdLinkConfiguration(
            m_internalVideo4KTransport, bmdQuadLinkSquareDivisionSplit);

        if (m_configuration->SetInt(bmdDeckLinkConfigSDIOutputLinkConfiguration,
                                    bmdLinkConfiguration)
            != S_OK)
        {
            TWK_THROW_EXC_STREAM(
                "Cannot set decklink SDI Output Link Configuration to "
                << video4KTransports[m_internalVideo4KTransport].desc);
        }
        if (bmdQuadLinkSquareDivisionSplit != -1)
        {
            if (m_configuration->SetFlag(
                    bmdDeckLinkConfigQuadLinkSDIVideoOutputSquareDivisionSplit,
                    bmdQuadLinkSquareDivisionSplit)
                != S_OK)
            {
                TWK_THROW_EXC_STREAM(
                    "Cannot set Square Division Quad Split mode\n");
            }
        }

        for (const DeckLinkDataFormat* q = dataFormats; q->desc; q++)
        {
            BMDDisplayMode displayMode;

#ifdef PLATFORM_WINDOWS
            BOOL supported = false;
#else
            bool supported = false;
#endif

            HRESULT hr = m_outputAPI->DoesSupportVideoMode(
                bmdVideoConnectionUnspecified, v.value, q->value,
                bmdNoVideoOutputConversion,
                getBmdSupportedVideoModeFlag(m_internalVideo4KTransport),
                &displayMode, &supported);
            if (hr == S_OK && supported)
            {
                m_decklinkDataFormats.push_back(*q);
            }
        }

        const DeckLinkDataFormat& d =
            m_decklinkDataFormats[m_internalDataFormat];
        const string& dname = d.desc;
        m_outputPixelFormat = d.value;

        const DeckLinkAudioFormat& a = audioFormats[m_internalAudioFormat];
        m_audioChannelCount = a.numChannels;
        m_audioSampleDepth = a.sampleDepth;
        m_audioSampleRate = a.sampleRate;
        m_audioFormat = a.prec;

        GLenumPair epair = TwkGLF::textureFormatFromDataFormat(d.iformat);
        m_textureFormat = epair.first;
        m_textureType = epair.second;

        m_stereo = dname.find("Stereo") != string::npos;

        m_videoFrameBufferSize = m_stereo ? m_pboSize * 2 : m_pboSize;

        if (m_outputAPI->GetDisplayModeIterator(&pDLDisplayModeIterator)
            != S_OK)
        {
            TWK_THROW_EXC_STREAM("Cannot get display mode iterator.");
        }

        while (pDLDisplayModeIterator->Next(&pDLDisplayMode) == S_OK)
        {
            // For debugging, to check what formats the card actually has.
            // char strbuf[255];
            // const char *displayModeStr = strbuf;
            // pDLDisplayMode->GetName(&displayModeStr);
            // cerr << "Display mode =" << displayModeStr << endl;

            if (pDLDisplayMode->GetDisplayMode() == displayMode)
                break;
            pDLDisplayMode->Release();
            pDLDisplayMode = NULL;
        }

        pDLDisplayModeIterator->Release();

        if (pDLDisplayMode == NULL)
        {
            TWK_THROW_EXC_STREAM(
                "Cannot get specified BMDDisplayMode on DeckLink.");
        }

        m_frameWidth = pDLDisplayMode->GetWidth();
        m_frameHeight = pDLDisplayMode->GetHeight();
        pDLDisplayMode->GetFrameRate(&m_frameDuration, &m_frameTimescale);

        // Set the Video Output Color Space (RGB444/YUV422)
        const int bmdEnable444 = d.rgb ? 1 : 0;
        HRESULT hr = m_configuration->SetFlag(
            bmdDeckLinkConfig444SDIVideoOutput, bmdEnable444);
        if (hr != S_OK && hr != E_NOTIMPL)
        {
            TWK_THROW_EXC_STREAM("BMD: Failed to set 444 SDI output\n");
        }

        // enable video
        BMDVideoOutputFlags videoOutputFlags =
            m_stereo ? bmdVideoOutputDualStream3D : bmdVideoOutputFlagDefault;
        if (m_outputAPI->EnableVideoOutput(displayMode, videoOutputFlags)
            != S_OK)
        {
            TWK_THROW_EXC_STREAM("Cannot enable video output on DeckLink.");
        }

        m_framesPerSecond =
            (unsigned long)((m_frameTimescale + (m_frameDuration - 1))
                            / m_frameDuration);
        m_audioSamplesPerFrame =
            (unsigned long)((m_audioSampleRate * m_frameDuration)
                            / m_frameTimescale);

        // Allocate audio buffers
        m_audioFormatSizeInBytes = TwkAudio::formatSizeInBytes(m_audioFormat);
        for (int i = 0; i < NB_AUDIO_BUFFERS; i++)
        {
            m_audioData[i] =
                new char[m_audioSamplesPerFrame * m_audioChannelCount
                         * m_audioFormatSizeInBytes];
            std::memset(m_audioData[i],
                        0, // Fill value
                        m_audioSamplesPerFrame * m_audioChannelCount
                            * m_audioFormatSizeInBytes);
        }

        //
        // regardless of the output video format, we will only read data
        // back from the graphics card in one of two formats: 8 bit BGRA,
        // or 10_10_10_2 RGBA. All other formats will need a frame
        // converter
        //

#if 0
    if (d.value != bmdFormat8BitBGRA && d.value != bmdFormat10BitRGB)
    {
        m_needsFrameConverter = true;
    }
#endif

        //
        // Create a queue of IDeckLinkMutableVideoFrame objects to use for
        // scheduling output video frames.
        //

        for (int i = 0; i < m_videoFrameBufferSize; i++)
        {
            IDeckLinkMutableVideoFrame* outputFrame = nullptr;
            const DeckLinkDataFormat& dataFormat =
                m_decklinkDataFormats[m_internalDataFormat];
            if (m_outputAPI->CreateVideoFrame(
                    m_frameWidth, m_frameHeight,
                    bytesPerRow(dataFormat.value, m_frameWidth),
                    dataFormat.value, bmdFrameFlagFlipVertical, &outputFrame)
                != S_OK)
            {
                TWK_THROW_EXC_STREAM("Cannot create video frame on DeckLink.");
            }
            m_DLOutputVideoFrameQueue.push_back(outputFrame);

            if (d.value == bmdFormat8BitYUV)
            {
                m_readPixelFormat = bmdFormat8BitBGRA;
            }
            else
            {
                m_readPixelFormat = bmdFormat10BitRGB;
            }

            if (!m_needsFrameConverter)
                continue;

            // in case we need a frame converter, the readbackFrame will be in
            // either BGRA8 or RGBA10_10_10_2
            IDeckLinkMutableVideoFrame* readbackFrame;

            if (m_outputAPI->CreateVideoFrame(
                    m_frameWidth, m_frameHeight, m_frameWidth * 4,
                    m_readPixelFormat, bmdFrameFlagFlipVertical, &readbackFrame)
                != S_OK)
            {
                TWK_THROW_EXC_STREAM(
                    "Cannot create readback frame on DeckLink.");
            }

            m_DLReadbackVideoFrameQueue.push_back(readbackFrame);
        }

        //
        //  create the 3D frames now, and reuse them throughout the
        //  playback
        //
        if (m_stereo)
        {
            for (int i = 1; i < m_DLOutputVideoFrameQueue.size(); i += 2)
            {
                m_rightEyeToStereoFrameMap[m_DLOutputVideoFrameQueue.at(i)] =
                    new StereoVideoFrame(m_DLOutputVideoFrameQueue.at(i - 1),
                                         m_DLOutputVideoFrameQueue.at(i));
            }
        }

        //
        //  Create the HDRVideoFrames
        //  Note that no video frame memory will be allocated here, only a COM
        //  object structure. An HDRVideoFrame is a COM object implementing the
        //  following COM interfaces: IUnknown, IDeckLinkVideoFrame, and
        //  IDeckLinkVideoFrameMetadataExtensions
        //
        if (m_useHDRMetadata)
        {
            for (int i = 0; i < m_DLOutputVideoFrameQueue.size(); i++)
            {
                m_FrameToHDRFrameMap[m_DLOutputVideoFrameQueue.at(i)] =
                    new HDRVideoFrame(m_DLOutputVideoFrameQueue.at(i));
            }
        }

        if (m_outputAPI->SetScheduledFrameCompletionCallback(this) != S_OK)
        {
            TWK_THROW_EXC_STREAM("Failed to set frame completion callback.");
        }

        //
        //  Set the audio output mode
        //

        if (m_outputAPI->EnableAudioOutput(
                m_audioSampleRate, m_audioSampleDepth, m_audioChannelCount,
                bmdAudioOutputStreamContinuous)
            != S_OK)
        {
            TWK_THROW_EXC_STREAM("Cannot enable audio output on DeckLink.");
        }

        if (m_outputAPI->SetAudioCallback(this) != S_OK)
        {
            TWK_THROW_EXC_STREAM("Failed to set audio completion callback.");
        }

        m_readyFrame = m_DLOutputVideoFrameQueue.at(0);
        m_readyStereoFrame =
            m_rightEyeToStereoFrameMap[m_DLOutputVideoFrameQueue.at(1)];

        m_open = true;
    }

    void DeckLinkVideoDevice::close()
    {
        if (m_open)
        {
            m_open = false;

            unbind();

            m_outputAPI->StopScheduledPlayback(0, NULL, 0);
            m_outputAPI->DisableAudioOutput();
            m_outputAPI->DisableVideoOutput();

            int rc = pthread_mutex_lock(&audioMutex);
            m_hasAudio = false;
            for (int i = 0; i < NB_AUDIO_BUFFERS; i++)
            {
                if (m_audioData[i] != nullptr)
                {
                    delete[] static_cast<char*>(m_audioData[i]);
                    m_audioData[i] = nullptr;
                }
            }
            rc = pthread_mutex_unlock(&audioMutex);

            m_needsFrameConverter = false;

            for (int i = 0; i < m_DLOutputVideoFrameQueue.size(); i++)
            {
                m_DLOutputVideoFrameQueue.at(i)->Release();
            }

            m_DLOutputVideoFrameQueue.clear();

            for (int i = 0; i < m_DLReadbackVideoFrameQueue.size(); i++)
            {
                m_DLReadbackVideoFrameQueue.at(i)->Release();
            }

            m_DLReadbackVideoFrameQueue.clear();

            if (m_stereo)
            {
                StereoFrameMap::iterator it;
                for (it = m_rightEyeToStereoFrameMap.begin();
                     it != m_rightEyeToStereoFrameMap.end(); ++it)
                {
                    delete it->second;
                }
                m_rightEyeToStereoFrameMap.clear();
            }

            if (m_useHDRMetadata)
            {
                HDRVideoFrameMap::iterator it;
                for (it = m_FrameToHDRFrameMap.begin();
                     it != m_FrameToHDRFrameMap.end(); ++it)
                {
                    delete it->second;
                }
                m_FrameToHDRFrameMap.clear();
            }
        }

        TwkGLF::GLBindableVideoDevice::close();
    }

    bool DeckLinkVideoDevice::isStereo() const { return m_stereo; }

    bool DeckLinkVideoDevice::isOpen() const { return m_open; }

    void DeckLinkVideoDevice::makeCurrent() const {}

    void DeckLinkVideoDevice::clearCaches() const {}

    void DeckLinkVideoDevice::syncBuffers() const {}

    bool DeckLinkVideoDevice::isDualStereo() const { return isStereo(); }

    void DeckLinkVideoDevice::transferAudio(void* data, size_t n) const
    {
        int rc = pthread_mutex_lock(&audioMutex);
        if (!data)
        {
            m_hasAudio = false;
            rc = pthread_mutex_unlock(&audioMutex);
            return;
        }
        m_hasAudio = true;
        rc = pthread_mutex_unlock(&audioMutex);

        std::memcpy(m_audioData[m_audioDataIndex], data,
                    m_audioFormatSizeInBytes * m_audioSamplesPerFrame
                        * m_audioChannelCount);
        m_audioDataIndex = (m_audioDataIndex + 1) % 2;
    }

    bool DeckLinkVideoDevice::transferChannel(size_t n, const GLFBO* fbo) const
    {
        HOP_PROF_FUNC();

        fbo->bind();

        IDeckLinkMutableVideoFrame* readbackVideoFrame = NULL;
        IDeckLinkMutableVideoFrame* outputVideoFrame =
            m_DLOutputVideoFrameQueue.front();
        m_DLOutputVideoFrameQueue.push_back(outputVideoFrame);
        m_DLOutputVideoFrameQueue.pop_front();

#if 0
    if (m_needsFrameConverter)
    {
        readbackVideoFrame = m_DLReadbackVideoFrameQueue.front();
        m_DLReadbackVideoFrameQueue.push_back(readbackVideoFrame);
        m_DLReadbackVideoFrameQueue.pop_front();
    }
#endif

        if (m_pbos)
            transferChannelPBO(n, fbo, outputVideoFrame, readbackVideoFrame);
        else
            transferChannelReadPixels(n, fbo, outputVideoFrame,
                                      readbackVideoFrame);

        if (n == 0 && !m_stereo)
        {
            // non stereo, then n is always 0
            m_readyFrame = outputVideoFrame;
        }
        else if (n == 1) // in case of stereo, ready frame is updated when right
                         // eye is transfered
        {
            m_readyStereoFrame = m_rightEyeToStereoFrameMap[outputVideoFrame];
        }

        return true;
    }

    void DeckLinkVideoDevice::transferChannelPBO(
        size_t n, const GLFBO* fbo,
        IDeckLinkMutableVideoFrame* outputVideoFrame,
        IDeckLinkMutableVideoFrame* readbackVideoFrame) const
    {
        HOP_CALL(glFinish();)
        HOP_PROF_FUNC();

        PBOData* lastPboData = m_lastPboData;
        if (m_stereo && n == 0)
        {
            lastPboData = m_secondLastPboData;
        }

        const DeckLinkDataFormat& d =
            m_decklinkDataFormats[m_internalDataFormat];

        // GL_UNSIGNED_INT_10_10_10_2 is a non native GPU format which leads to
        // costly GPU readbacks. To work around this performance issue we will
        // use the GPU native GL_UNSIGNED_INT_2_10_10_10_REV format instead and
        // perform a CPU format conversion after readback. This is much more
        // optimal than letting the GPU do the conversion during readback. For
        // reference: SG-14524
        GLenum textureTypeToUse = m_textureType;
        bool perform_ABGR10_to_RGBA10_conversion = false;
        if (m_textureType == GL_UNSIGNED_INT_10_10_10_2)
        {
            textureTypeToUse = GL_UNSIGNED_INT_2_10_10_10_REV;
            perform_ABGR10_to_RGBA10_conversion = true;
        }

        const bool perform_rgb16_to_rgb12_conversion =
            (d.value == bmdFormat12BitRGBLE);

        if (lastPboData)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, lastPboData->globject);
            TWK_GLDEBUG;

            lastPboData->lockState();
            lastPboData->state = PBOData::State::Transferring;
            lastPboData->unlockState();

            void* p =
                (void*)glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
            TWK_GLDEBUG;
            if (p)
            {
                void* pFrame;
                outputVideoFrame->GetBytes(&pFrame);

                if (d.iformat >= VideoDevice::Y0CbY1Cr_8_422)
                {
                    if (d.iformat == VideoDevice::Y0CbY1Cr_8_422)
                    {
                        subsample422_8bit_UYVY_MP(
                            m_frameWidth, m_frameHeight,
                            reinterpret_cast<uint8_t*>(p),
                            reinterpret_cast<uint8_t*>(pFrame));
                    }
                    else
                    {
                        subsample422_10bit_MP(
                            m_frameWidth, m_frameHeight,
                            reinterpret_cast<uint32_t*>(p),
                            reinterpret_cast<uint32_t*>(pFrame),
                            m_frameWidth * sizeof(uint32_t),
                            static_cast<size_t>(
                                outputVideoFrame->GetRowBytes()));
                    }
                }
                else
                {
                    if (perform_ABGR10_to_RGBA10_conversion)
                    {
                        convert_ABGR10_to_RGBA10_MP(
                            m_frameWidth, m_frameHeight,
                            reinterpret_cast<uint32_t*>(p),
                            reinterpret_cast<uint32_t*>(pFrame));
                    }
                    else if (perform_rgb16_to_rgb12_conversion)
                    {
                        convert_RGB16_to_RGB12_MP(
                            m_frameWidth, m_frameHeight, p, pFrame,
                            m_frameWidth * sizeof(uint16_t) * 3,
                            static_cast<size_t>(
                                outputVideoFrame->GetRowBytes()));
                    }
                    else
                    {
                        FastMemcpy_MP(pFrame, p,
                                      m_frameHeight * m_frameWidth * 4);
                    }
                }
            }

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
            TWK_GLDEBUG;
            lastPboData->lockState();
            lastPboData->state = PBOData::State::Ready;
            lastPboData->fbo->endExternalReadback();
            lastPboData->fbo = 0;
            lastPboData->unlockState();
        }

        {
#if defined(HOP_ENABLED)
            std::string hopMsg = std::string("glReadPixels() next PBO");
            const size_t pixelSize = TwkGLF::pixelSizeFromTextureFormat(
                m_textureFormat, m_textureType);
            hopMsg += std::string(" - width=") + std::to_string(m_frameWidth)
                      + std::string(", height=") + std::to_string(m_frameHeight)
                      + std::string(", pixelSize=") + std::to_string(pixelSize)
                      + std::string(", textureFormat=")
                      + std::to_string(m_textureFormat)
                      + std::string(", textureType=")
                      + std::to_string(m_textureType);
            HOP_PROF_DYN_NAME(hopMsg.c_str());
#endif

            // next pbo read
            PBOData* pboData = m_pboQueue.front();
            if (!pboData)
            {
                cerr << "ERROR: pboData is NULL!" << endl;
            }
            else
            {
                m_pboQueue.push_back(pboData);
                m_pboQueue.pop_front();
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pboData->globject);
            TWK_GLDEBUG;
            pboData->lockState();
            pboData->fbo = fbo;
            bool unmapit = pboData->state == PBOData::State::Mapped;
            pboData->unlockState();

            if (unmapit)
            {
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
                TWK_GLDEBUG;
            }

            glReadPixels(0, 0, m_frameWidth, m_frameHeight, m_textureFormat,
                         textureTypeToUse, 0);
            TWK_GLDEBUG;

            if (m_stereo && n == 0)
            {
                m_secondLastPboData = pboData;
            }
            else
            {
                m_lastPboData = pboData;
            }

            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
            HOP_CALL(glFinish();)
        }
    }

    void DeckLinkVideoDevice::transferChannelReadPixels(
        size_t n, const GLFBO* fbo,
        IDeckLinkMutableVideoFrame* outputVideoFrame,
        IDeckLinkMutableVideoFrame* readbackVideoFrame) const
    {
        HOP_PROF_FUNC();

        void* pFrame;
        if (m_needsFrameConverter)
        {
            readbackVideoFrame->GetBytes(&pFrame);
        }
        else
        {
            outputVideoFrame->GetBytes(&pFrame);
        }

        const DeckLinkDataFormat& d =
            m_decklinkDataFormats[m_internalDataFormat];

        TwkUtil::Timer timer;
        timer.start();

        glReadPixels(0, 0, m_frameWidth, m_frameHeight, m_textureFormat,
                     m_textureType, pFrame);

        timer.stop();

        fbo->endExternalReadback();

        if (m_needsFrameConverter)
        {
            TwkUtil::Timer timer;
            timer.start();

            void* outData;
            outputVideoFrame->GetBytes(&outData);
            if (d.iformat == VideoDevice::Y0CbY1Cr_8_422)
            {
                subsample422_8bit_UYVY_MP(m_frameWidth, m_frameHeight,
                                          reinterpret_cast<uint8_t*>(pFrame),
                                          reinterpret_cast<uint8_t*>(outData));
            }
            else
            {
                subsample422_10bit_MP(
                    m_frameWidth, m_frameHeight,
                    reinterpret_cast<uint32_t*>(pFrame),
                    reinterpret_cast<uint32_t*>(outData),
                    m_frameWidth * sizeof(uint32_t),
                    static_cast<size_t>(outputVideoFrame->GetRowBytes()));
            }

            timer.stop();
        }
    }

    void DeckLinkVideoDevice::transfer(const GLFBO* fbo) const
    {
        HOP_ZONE(HOP_ZONE_COLOR_4);
        HOP_PROF_FUNC();

        if (!m_open)
            return;

        transferChannel(0, fbo);

        incrementClock();

        // schedule the first 3 frames before we start the scheduler
        if (m_firstThreeCounter < 4)
        {
            ScheduleFrame();
        }

        // start the scheduler
        if (m_firstThreeCounter == 4)
        {
            if (m_outputAPI->BeginAudioPreroll() != S_OK)
                TWK_THROW_EXC_STREAM("Failed to preroll audio.");
        }

        ////////////////////////////////////////////////////////////////////////////
        // Skipped frame detector
        if (evDetectSkippedFrames.getValue())
        {
            static TwkUtil::Timer sinceLastReadyFrameTimer;
            if ((m_totalPlayoutFrames > 5))
            {
                const double secondsPerFrame = 1.0 / m_framesPerSecond;
                const double secondsSinceLastReadyFrame =
                    sinceLastReadyFrameTimer.elapsed();
#if defined(HOP_ENABLED)
                std::string hopMsg =
                    std::string("detector-msSinceLastReadyFrame=");
                hopMsg += std::to_string(secondsSinceLastReadyFrame * 1000.0);
                HOP_PROF_DYN_NAME(hopMsg.c_str());
#endif
                if (secondsSinceLastReadyFrame > (2.0 * secondsPerFrame))
                {
                    HOP_PROF("skippedFrame");
                    cout << "WARNING: Skipped Frame"
                         << " - secondsSinceLastReadyFrame="
                         << secondsSinceLastReadyFrame
                         << " - Frame Rate=" << m_framesPerSecond << endl;
                }
            }
            sinceLastReadyFrameTimer.start();
        }
        ////////////////////////////////////////////////////////////////////////////
    }

    bool DeckLinkVideoDevice::willBlockOnTransfer() const { return false; }

    bool DeckLinkVideoDevice::readyForTransfer() const
    {
        if (m_firstThreeCounter++ < 3)
            return true;

        int rc = pthread_mutex_lock(&videoMutex);
        if (!m_frameCompleted)
        {
            rc = pthread_mutex_unlock(&videoMutex);
            return false;
        }
        else
        {
            m_frameCompleted = false;
        }

        rc = pthread_mutex_unlock(&videoMutex);

        return true;
    }

    void DeckLinkVideoDevice::transfer2(const GLFBO* fbo,
                                        const GLFBO* fbo2) const
    {
        if (!m_open)
            return;

        if (m_pbos)
        {
            // Note transferChannelPBO() 'fbo' argument represents
            // the fbo to be read into the next pbo; hence we call
            // transferChannel() with the right eye fbo first.
            transferChannel(0, fbo2);
            transferChannel(1, fbo);
        }
        else
        {
            transferChannel(0, fbo);
            transferChannel(1, fbo2);
        }

        incrementClock();

        // schedule the first 3 frames before we start the scheduler
        if (m_firstThreeCounter < 4)
        {
            ScheduleFrame();
        }

        // start the scheduler
        if (m_firstThreeCounter == 4)
        {
            if (m_outputAPI->BeginAudioPreroll() != S_OK)
                TWK_THROW_EXC_STREAM("Failed to preroll audio.");
        }
    }

    void DeckLinkVideoDevice::unbind() const
    {
        if (m_pbos)
        {
            for (int i = 0; i < m_pboQueue.size(); i++)
            {
                PBOData* p = m_pboQueue.at(i);
                if (p->state == PBOData::State::NeedsUnmap)
                {
                    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, p->globject);
                    TWK_GLDEBUG;
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
                    TWK_GLDEBUG;
                    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                }
                glDeleteBuffers(1, &(p->globject));
                TWK_GLDEBUG;
                delete m_pboQueue.at(i);
            }

            m_pboQueue.clear();
        }
    }

    void DeckLinkVideoDevice::bind(const GLVideoDevice* d) const
    {
        if (m_pbos)
        {
            size_t num = m_stereo ? m_pboSize * 2 : m_pboSize;

            const size_t bytesPerPixel = std::max(
                static_cast<size_t>(4),
                pixelSizeInBytes(
                    m_decklinkDataFormats[m_internalDataFormat].iformat));

            for (size_t q = 0; q < num; q++)
            {
                GLuint glObject;
                glGenBuffers(1, &glObject);
                TWK_GLDEBUG;
                glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, glObject);
                TWK_GLDEBUG;
                glBufferData(GL_PIXEL_PACK_BUFFER_ARB,
                             m_frameWidth * bytesPerPixel * m_frameHeight, NULL,
                             GL_STATIC_READ);
                TWK_GLDEBUG;
                glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
                TWK_GLDEBUG;

                m_pboQueue.push_back(new PBOData(glObject));
            }
        }

        resetClock();
    }

    void DeckLinkVideoDevice::bind2(const GLVideoDevice* d,
                                    const GLVideoDevice* d2) const
    {
        bind(d);
    }

    void DeckLinkVideoDevice::ScheduleFrame() const
    {
        if (isOpen())
        {
            IDeckLinkVideoFrame* outputVideoFrame =
                m_stereo ? (IDeckLinkVideoFrame*)m_readyStereoFrame
                         : (IDeckLinkVideoFrame*)m_readyFrame;

            if (m_useHDRMetadata)
            {
                outputVideoFrame =
                    (IDeckLinkVideoFrame*)m_FrameToHDRFrameMap[m_readyFrame];
            }

            if (outputVideoFrame)
            {
                // Prevent undesirable audio artifacts when repeating the same
                // frame Note that some conditions might prevent RV from
                // fetching a new frame in time. In this case the same frame has
                // to be sent to the output. Repeating one frame has a minimal
                // impact on the review session. However repeating the same
                // audio snippet might cause an undesirable audio artifact. This
                // is what we want to prevent here. Example of such a condition:
                // when adding media to a sequence while a playback is already
                // in progress for example,
                bool skipAudioIfAny = false;
                static IDeckLinkVideoFrame*
                    previouslyScheduledOutputVideoFrame = nullptr;
                if (outputVideoFrame == previouslyScheduledOutputVideoFrame)
                {
                    skipAudioIfAny = true;
                }
                previouslyScheduledOutputVideoFrame = outputVideoFrame;

                HRESULT r = m_outputAPI->ScheduleVideoFrame(
                    outputVideoFrame, (m_totalPlayoutFrames * m_frameDuration),
                    m_frameDuration, m_frameTimescale);

                if (r != S_OK)
                    TWK_THROW_EXC_STREAM("Cannot schedule frame.");

                int rc = pthread_mutex_lock(&audioMutex);

                if (m_hasAudio && !skipAudioIfAny)
                {
                    rc = pthread_mutex_unlock(&audioMutex);
                    int index = (m_audioDataIndex == 1) ? 0 : 1;
                    if (m_audioData[index]
                        && m_outputAPI->ScheduleAudioSamples(
                               m_audioData[index], m_audioSamplesPerFrame,
                               (m_totalPlayoutFrames * m_audioSamplesPerFrame),
                               bmdAudioSampleRate48kHz, NULL)
                               != S_OK)
                    {
                        TWK_THROW_EXC_STREAM(
                            "Failed to perform audio transfers.");
                    }
                }
                else
                {
                    rc = pthread_mutex_unlock(&audioMutex);
                }

                m_totalPlayoutFrames++;
            }
        }
    }

    size_t DeckLinkVideoDevice::bytesPerRow(BMDPixelFormat bmdFormat,
                                            size_t width) const
    {
        switch (bmdFormat)
        {
        case bmdFormat8BitYUV:
            return m_frameWidth * 16 / 8;
        case bmdFormat8BitBGRA:
            return m_frameWidth * 32 / 8;
        case bmdFormat10BitYUV:
            return ((m_frameWidth + 47) / 48) * 128;
        case bmdFormat10BitRGBXLE:
            return ((m_frameWidth + 63) / 64) * 256;
        case bmdFormat12BitRGBLE:
            return (m_frameWidth * 36) / 8;
        default:
            TWK_THROW_EXC_STREAM("Unknown pixel format.");
        }
    }

    HRESULT
    DeckLinkVideoDevice::ScheduledFrameCompleted(
        IDeckLinkVideoFrame* completedFrame,
        BMDOutputFrameCompletionResult result)
    {
        ScheduleFrame();

        int rc = pthread_mutex_lock(&videoMutex);
        m_frameCompleted = true;
        rc = pthread_mutex_unlock(&videoMutex);

        return S_OK;
    }

    HRESULT
    DeckLinkVideoDevice::ScheduledPlaybackHasStopped() { return S_OK; }

    HRESULT
#ifdef PLATFORM_WINDOWS
    DeckLinkVideoDevice::RenderAudioSamples(BOOL preroll)
#else
    DeckLinkVideoDevice::RenderAudioSamples(bool preroll)
#endif
    {
        if (preroll)
        {
            // Start audio and video output
            m_outputAPI->StartScheduledPlayback(0, 100, 1.0);
        }
        return S_OK;
    }

} // namespace BlackMagicDevices
