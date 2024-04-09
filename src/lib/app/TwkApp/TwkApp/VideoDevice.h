//
//  Copyright (c) 2011 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __TwkApp__VideoDevice__h__
#define __TwkApp__VideoDevice__h__
#include <iostream>
#include <string>
#include <TwkApp/EventNode.h>
#include <TwkAudio/AudioFormats.h>

namespace TwkApp {
class VideoModule;

//
//  VideoDevice
//
//  Wraps APIs necessary to output and/or capture from a video
//  stream. Examples sub-classes:
//
//      * GL window system default frame buffer(s)
//      * SDI+GL output buffer for nvidia cards
//      * SDI stand-alone 
//      * Slave GL window on multi-head machine
//

class VideoDevice : public EventNode
{
public:

    typedef double                   Time;
    typedef std::vector<size_t>      AudioFrameSizeVector;
    typedef std::vector<std::string> StringVector;

    enum Capabilities
    {
        NoCapabilities        = 0,
        ImageOutput           = 1 << 0, // can display images/video
        ImageCapture          = 1 << 1, // can capture images/video
        ProvidesSync          = 1 << 2, // provides a (wait on) sync call
        FixedResolution       = 1 << 3, // limited set of fixed resolutions
        SubWindow             = 1 << 4, // device is a subwindow of something larger
        Clock                 = 1 << 5, // has its own clock to sync to
        AudioOutput           = 1 << 6, // can output audio samples (with the video)
        AudioCapture          = 1 << 7, // can capture audio samples (with the video)
        TimeCodeOutput        = 1 << 8, // can output VITC timecode
        BlockingTransfer      = 1 << 9, // transfer functions may block
        ASyncReadBack         = 1 << 10, // will read frames back asynchronously
        FlippedImage          = 1 << 11, // image should be rendered flipped
        NormalizedCoordinates = 1 << 12 // passed in FBO attachements
                                        // are GL_TEXTURE_2D instead
                                        // of GL_TEXTURE_RECTANGLE
    };

    enum DisplayMode
    {
        IndependentDisplayMode  = 0,
        MirrorDisplayMode       = 1,
        NotADisplayMode         = 2
    };

    //
    //  The InternalDataFormat is the final format that the renderer
    //  will produce before transfering to the video device. This is
    //  only meaningful for GLBindableVideoDevice currently, but could
    //  be extended to non-GL devices as well.
    //

    enum InternalDataFormat
    {
        RGB8,
        RGBA8,
        BGRA8,
        RGB16,
        RGBA16,
        RGB10X2,
        RGB10X2Rev,
        RGB16F,
        RGBA16F,
        RGB32F,
        RGBA32F,

        //
        //  NOTE: currently the renderer will convert to YUV but will
        //  not do the subsampling.
        //

        CbY0CrY1_8_422,         // aka 2vuy or UYVY
        Y0CbY1Cr_8_422,         // aka yuvs or YUY2
        Y1CbY0Cr_8_422,
        YCrCb_AJA_10_422,       // v210
        YCrCb_BM_10_422,        //

        YCbCr_P216_16_422       // Semi-Planar, 4:2:2, 16-bit per component.
                                // The first buffer is a 16bpp luminance buffer.
	                            // Immediately after this is an interleaved buffer of 16bpp Cb, Cr pairs.
    };

    struct Resolution
    {
        Resolution(size_t w, size_t h, float pa, float ps)
            : width(w), height(h), pixelAspect(pa), pixelScale(ps) {}
        size_t width;
        size_t height;
        float pixelAspect;
        float pixelScale;       // e.g. retina might have 2.0 here
    };

    struct Offset
    {
        Offset(int x_, int y_) : x(x_), y(y_) {}
        int x;
        int y;
    };

    struct Timing
    {
        Timing(float h) : hz(h) {}
        float hz;
    };

    struct VideoFormat : public Resolution, public Timing
    {
        VideoFormat() : Resolution(0,0,0,0), Timing(0) {}
        VideoFormat(size_t w, size_t h, float pa, float ps,
                    float hertz,
                    const std::string& desc = "")
            : Resolution(w, h, pa, ps),
              Timing(hertz),
              description(desc) {}
        
        std::string description;
    };

    struct DataFormat
    {
        DataFormat(InternalDataFormat f, const std::string& desc = "") 
            : iformat(f),
              description(desc) {}
        DataFormat(const std::string& desc = "") 
            : iformat(RGBA16F),
              description(desc) {}
        InternalDataFormat iformat;
        std::string description;
    };

    struct SyncMode
    {
        SyncMode(const std::string& desc = "")
            : description(desc) {}
        std::string description;
    };

    struct SyncSource
    {
        SyncSource(const std::string& desc = "") 
            : description(desc) {}
        std::string description;
    };

    struct Video4KTransport
    {
        Video4KTransport(const std::string& desc = "")
            : description(desc) {}
        std::string description;
    };

    struct Margins
    {
        Margins() : left(0), right(0), top(0), bottom(0) {}
        Margins(float l, float r, float t, float b)
            : left(l), right(r), top(t), bottom(b) {}
        float left;
        float right;
        float top;
        float bottom;
    };

    struct AudioFormat
    {
        AudioFormat() : hz(Time(0)),
                        format(TwkAudio::Float32Format),
                        numChannels(0),
                        layout(TwkAudio::UnknownLayout) {}

        AudioFormat(Time rate,
                    TwkAudio::Format t,
                    size_t n,
                    TwkAudio::Layout layout,
                    const std::string& desc)
            : hz(rate),
              format(t),
              description(desc),
              numChannels(n),
              layout(layout) {}

        Time               hz;
        TwkAudio::Format   format;
        size_t             numChannels;
        TwkAudio::Layout   layout;
        std::string        description;
    };

    enum ColorProfileType
    {
        NoColorProfile,
        ICCProfile
    };

    struct ColorProfile
    {
        ColorProfile(ColorProfileType t = NoColorProfile,
                     const std::string& desc = "",
                     const std::string& u = "") 
            : type(t),
              description(desc),
              url(u) {}

        ColorProfileType type;
        std::string      description;
        std::string      url;
    };

    class AudioInterface
    {
      public:
        AudioInterface();
        virtual ~AudioInterface();

        virtual size_t numChannels() const = 0;
        virtual void audioFillChannel(size_t startSample, 
                                      double rate,
                                      size_t channel,
                                      float* buffer,
                                      size_t numSamples);
    };

    VideoDevice(VideoModule*,
                const std::string& name,
                unsigned int capabilities);

    virtual ~VideoDevice();

    //
    //  Query state
    //

    const std::string& name() const { return m_name; }
    unsigned int capabilities() const { return m_capabilities; }
    const VideoModule* module() const { return m_module; }
    bool swapStereoEyes() const { return m_swapStereoEyes; }

    void setSwapStereoEyes (bool swap) { m_swapStereoEyes = swap; }

    //
    //  Hash ID
    //
    //  This is the device name current video and data formats hashed
    //  together or just a subset of that. Use this to match display
    //  profiles for devices. For example you can find a general
    //  device hash using NameHash or a specific one using
    //  VideoAndDataFormatHash
    //

    enum IDType
    {
        HostnameVideoAndDataFormatID,
        VideoAndDataFormatID,
        VideoFormatID,
        DataFormatID,
        DeviceNameID,
        ModuleNameID
    };

    size_t hashID(IDType t = HostnameVideoAndDataFormatID) const;
    std::string humanReadableID(IDType t = HostnameVideoAndDataFormatID) const;

    //
    //  Audio
    //

    bool hasAudioOutput() const { return (m_capabilities & AudioOutput) != 0; }
    bool audioOutputEnabled() const { return hasAudioOutput() && m_useAudioOutput; }
    void setAudioOutputEnabled(bool b) { m_useAudioOutput = b; }

    bool useLatencyForAudio() const { return m_useLatencyForAudio; }
    void setUseLatencyForAudio(bool b) { m_useLatencyForAudio = b; }

    virtual size_t numAudioFormats() const;
    virtual AudioFormat audioFormatAtIndex(size_t) const;
    virtual void setAudioFormat(size_t);
    virtual size_t currentAudioFormat() const;

    //
    //  Transfer begin/end These are called before a single frame of data
    //  is transfered. The transfer can occur as a number of API
    //  calls. Once endTransfer() is called you know no more transfer calls
    //  will occur.
    //

    virtual void beginTransfer() const;
    virtual void endTransfer() const;
    
    //
    //  Called after openning and probably after binding if result is
    //  empty vector then its not ready yet. The vector will have
    //  either a single element, meaning the audio frame packet size
    //  is the same for every frame, or it will have a pattern which
    //  should be used for successive frames. 
    //
    //  So for example, it might have:
    //
    //      801 800 801 800 801 
    //
    //  for 59.94Hz playback since at 48khz its not divisible. This
    //  means you output audio like this:
    //
    //      1   2   3   4   5   6   7   8   9   10  ....
    //      801 800 801 800 801 801 800 801 800 801 ....
    //
    //

    virtual void audioFrameSizeSequence(AudioFrameSizeVector&) const;

    //
    //  Return the audio frame size for the upcoming output frame
    //

    virtual size_t currentAudioFrameSize() const;

    //
    //  Return the current audio frame size index into the array
    //  return by audioFrameSizeSequence().
    //

    virtual size_t currentAudioFrameSizeIndex() const;

    //
    //  Send audio to device. The number of samples will be initially
    //  decided by the result of audioFrameSizeSequence().
    //

    virtual void transferAudio(void* interleavedData, size_t n) const;

    //
    //  Timecode
    //

    bool hasTimecodeOutput() const { return (m_capabilities & TimeCodeOutput) != 0; }
    bool timecodeOutputEnabled() const { return hasTimecodeOutput() && m_useTimecodeOutput; }
    void setTimecodeOutputEnabled(bool b) { m_useTimecodeOutput = b; }

    //
    //  Clock(s). If it has the capability these should return the
    //  global time of the current output and the time of the expected
    //  input. These can differ if the devices buffers frames before
    //  output. In that case 
    //

    bool hasClock() const { return (m_capabilities & Clock) != 0; }

    virtual Time outputTime() const;
    virtual Time inputTime() const;

    virtual void resetClock() const;
    virtual Time nextFrameTime() const;
    virtual size_t nextFrame() const;

    virtual bool willBlockOnTransfer() const;

    //
    //  If ASyncReadBack is a capability than this function should
    //  return the number of internal buffers the renderer should
    //  allocate (FBOs in this case). I.e. the maximum number of
    //  mapped buffers required.
    //

    virtual size_t asyncMaxMappedBuffers() const;

    //
    //  The human identifiable name of the device: E.g. "DELL U2410" or
    //  "NVidia SDI" . Defaults to the device name.
    //

    virtual std::string hardwareIdentification() const;

    //
    //  Configurations
    //

    virtual size_t numVideoFormats() const;
    virtual VideoFormat videoFormatAtIndex(size_t) const;
    virtual void setVideoFormat(size_t);
    virtual size_t currentVideoFormat() const;

    virtual size_t numDataFormats() const;
    virtual DataFormat dataFormatAtIndex(size_t) const;
    virtual void setDataFormat(size_t);
    virtual size_t currentDataFormat() const;

    virtual size_t numSyncModes() const;
    virtual SyncMode syncModeAtIndex(size_t) const;
    virtual void setSyncMode(size_t);
    virtual size_t currentSyncMode() const;

    virtual size_t numSyncSources() const;
    virtual SyncSource syncSourceAtIndex(size_t) const;
    virtual void setSyncSource(size_t);
    virtual size_t currentSyncSource() const;

    virtual size_t numVideo4KTransports() const;
    virtual Video4KTransport video4KTransportAtIndex(size_t) const;
    virtual void setVideo4KTransport(size_t);
    virtual size_t currentVideo4KTransport() const;

    //
    //  These are three independent video latencies. The first is
    //  computed by the device (if it can).
    //
    //  The fixedLatency() is in seconds and does not vary with the
    //  video format.
    //
    //  The frameLatency() is measured in frames (not seconds) and
    //  varies with the video frame rate.
    //
    //  The totalLatencyInSeconds() is the sum of all three in seconds
    //  and can be used to offset audio in order to achieve some
    //  semblance of sync
    //

    virtual Time deviceLatency() const;

    Time fixedLatency() const;
    void setFixedLatency(Time);

    Time frameLatencyInFrames() const; 
    Time frameLatencyInSeconds() const; 
    void setFrameLatency(Time frames);

    Time totalLatencyInSeconds() const;

    //
    //  Current Format
    //
    //  NOTE: internalWidth() and internalHeight() can differ from
    //  width() and height() in some cases. Renderers should use
    //  internalWidth() and internalHeight() to determine actual
    //  render buffer sizes. E.g. frame packed resolutions like:
    //
    //      1920 x (1080*2 + padding)
    //
    //  would have an internal size of 1920 x 1080.
    //

    virtual Resolution resolution() const = 0;
    virtual Offset offset() const;  // defaults to 0,0
    virtual Timing timing() const = 0;
    virtual VideoFormat format() const = 0;

    virtual size_t width() const = 0;
    virtual size_t height() const = 0;
    virtual float pixelScale() const;
    virtual float pixelAspect() const;

    virtual Resolution internalResolution() const;
    virtual Offset internalOffset() const;
    virtual Timing internalTiming() const;
    virtual VideoFormat internalFormat() const;
    virtual size_t internalWidth() const;
    virtual size_t internalHeight() const;

    virtual bool isStereo() const;
    virtual bool isDualStereo() const;

    //
    //  Display Mode
    //

    void setDisplayMode(DisplayMode m) { m_displayMode = m; }
    DisplayMode displayMode() const { return m_displayMode; }

    //
    //  Open the device for use. Video devices are not opened by
    //  default. The args are video device specific arguments (like
    //  command line arguments).
    //

    virtual void open(const StringVector& args) = 0;
    virtual void close() = 0;

    virtual bool isOpen() const = 0;

    //
    //  Clear any state caching
    //

    virtual void clearCaches() const = 0;

    //
    //  Sync
    //
    //  isSyncing() should return true if the device is currently
    //  blocked in syncBuffers in another thread.
    //
    //  blockUntilSyncComplete() called from a thread different than
    //  the one that called syncBuffers() should cause the calling
    //  thread to block until syncBuffers() returns to its caller. 
    //

    virtual void syncBuffers() const;
    virtual bool isSyncing() const;
    virtual void blockUntilSyncComplete() const;

    //
    //  Margins
    //

    void setMargins(float l, float r, float t, float b) const { m_margins = Margins(l, r, t, b); }
    void setMargins(const Margins& m) const { m_margins = m; }
    const Margins& margins() const { return m_margins; }

    //
    //  Other
    //

    static size_t pixelSizeInBytes(InternalDataFormat);

    //
    //  Device Color Profile
    //

    virtual ColorProfile colorProfile() const;

    //virtual int qtScreen() const { return -1; }

    //
    //  Physical device.  By default, this device _is_ this physical device,
    //  but from some devices (where the underlying physical screen can
    //  change), the physical device will be the "real" one.
    //

    VideoDevice* physicalDevice() { return m_physicalDevice; }
    const VideoDevice* physicalDevice() const { return m_physicalDevice; }
    virtual void setPhysicalDevice (VideoDevice *d) { m_physicalDevice = d; }

protected:
    void setCapabilities(unsigned int caps) { m_capabilities = caps; }
    void incrementClock() const;

protected:
    std::string                  m_name;
    unsigned int                 m_capabilities;
    VideoModule*                 m_module;
    DisplayMode                  m_displayMode;
    bool                         m_useAudioOutput;
    bool                         m_useTimecodeOutput;
    bool                         m_useLatencyForAudio;
    bool                         m_swapStereoEyes;
    Time                         m_fixedLatency;
    Time                         m_frameLatency;
    VideoDevice*                 m_physicalDevice;
    mutable AudioFrameSizeVector m_audioFrameSizes;
    mutable size_t               m_frameCount;
    mutable Time                 m_currentTime;
    mutable Margins              m_margins;
};

} // TwkApp

#endif // __TwkApp__VideoDevice__h__
