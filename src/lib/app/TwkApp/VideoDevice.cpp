//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <string>
#include <assert.h>
#include <stdlib.h>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

namespace TwkApp
{
    using namespace std;
    using namespace boost;

    VideoDevice::VideoDevice(VideoModule* module, const string& name,
                             unsigned int caps)
        : EventNode(name.c_str())
        , m_module(module)
        , m_name(name)
        , m_capabilities(caps)
        , m_displayMode(IndependentDisplayMode)
        , m_useAudioOutput(false)
        , m_useTimecodeOutput(false)
        , m_useLatencyForAudio(false)
        , m_swapStereoEyes(false)
        , m_fixedLatency(0)
        , m_frameLatency(0)
        , m_frameCount(0)
        , m_currentTime(0)
    {
        m_physicalDevice = this;
    }

    VideoDevice::~VideoDevice() {}

    VideoDevice::Offset VideoDevice::offset() const { return Offset(0, 0); }

    bool VideoDevice::isStereo() const { return false; }

    bool VideoDevice::isDualStereo() const { return false; }

    string VideoDevice::hardwareIdentification() const { return name(); }

    float VideoDevice::pixelScale() const { return 1.0f; }

    float VideoDevice::pixelAspect() const { return 1.0f; }

    void VideoDevice::syncBuffers() const {}

    bool VideoDevice::isSyncing() const { return false; }

    void VideoDevice::blockUntilSyncComplete() const {}

    size_t VideoDevice::asyncMaxMappedBuffers() const { return 1; }

    void VideoDevice::audioFrameSizeSequence(AudioFrameSizeVector&) const {}

    size_t VideoDevice::currentAudioFrameSize() const
    {
        if (m_audioFrameSizes.empty())
        {
            audioFrameSizeSequence(m_audioFrameSizes);
        }

        return m_audioFrameSizes[currentAudioFrameSizeIndex()];
    }

    size_t VideoDevice::currentAudioFrameSizeIndex() const
    {
        if (m_audioFrameSizes.empty())
        {
            audioFrameSizeSequence(m_audioFrameSizes);
        }

        return m_frameCount % m_audioFrameSizes.size();
    }

    void VideoDevice::transferAudio(void* interleavedData, size_t n) const {}

    size_t VideoDevice::numAudioFormats() const { return 0; }

    VideoDevice::AudioFormat VideoDevice::audioFormatAtIndex(size_t) const
    {
        return AudioFormat();
    }

    void VideoDevice::setAudioFormat(size_t) { m_audioFrameSizes.clear(); }

    size_t VideoDevice::currentAudioFormat() const { return 0; }

    size_t VideoDevice::numVideoFormats() const { return 0; }

    VideoDevice::VideoFormat VideoDevice::videoFormatAtIndex(size_t) const
    {
        return VideoFormat();
    }

    void VideoDevice::setVideoFormat(size_t) { m_audioFrameSizes.clear(); }

    size_t VideoDevice::currentVideoFormat() const { return 0; }

    size_t VideoDevice::numDataFormats() const { return 0; }

    VideoDevice::DataFormat VideoDevice::dataFormatAtIndex(size_t) const
    {
        return DataFormat();
    }

    void VideoDevice::setDataFormat(size_t) {}

    size_t VideoDevice::currentDataFormat() const { return 0; }

    size_t VideoDevice::numSyncModes() const { return 0; }

    VideoDevice::SyncMode VideoDevice::syncModeAtIndex(size_t) const
    {
        return SyncMode();
    }

    void VideoDevice::setSyncMode(size_t) {}

    size_t VideoDevice::currentSyncMode() const { return 0; }

    size_t VideoDevice::numSyncSources() const { return 0; }

    VideoDevice::SyncSource VideoDevice::syncSourceAtIndex(size_t) const
    {
        return SyncSource();
    }

    void VideoDevice::setSyncSource(size_t) {}

    size_t VideoDevice::currentSyncSource() const { return 0; }

    size_t VideoDevice::numVideo4KTransports() const { return 0; }

    VideoDevice::Video4KTransport
    VideoDevice::video4KTransportAtIndex(size_t) const
    {
        return Video4KTransport();
    }

    void VideoDevice::setVideo4KTransport(size_t) {}

    size_t VideoDevice::currentVideo4KTransport() const { return 0; }

    VideoDevice::Time VideoDevice::outputTime() const
    {
        return Time(m_currentTime);
    }

    VideoDevice::Time VideoDevice::inputTime() const { return Time(0); }

    void VideoDevice::resetClock() const
    {
        m_frameCount = 0;
        m_currentTime = 0;
    }

    void VideoDevice::incrementClock() const
    {
        //
        //  Do the clock by incrementing a frame count and then using
        //  division on that to reduce floating point precision loss
        //

        m_frameCount++;
        const Time hz = videoFormatAtIndex(currentVideoFormat()).hz;
        m_currentTime = Time(m_frameCount) / hz;
    }

    VideoDevice::Time VideoDevice::nextFrameTime() const
    {
        return m_currentTime;
    }

    size_t VideoDevice::nextFrame() const { return m_frameCount; }

    bool VideoDevice::willBlockOnTransfer() const { return false; }

    VideoDevice::Time VideoDevice::deviceLatency() const { return Time(0); }

    VideoDevice::Time VideoDevice::fixedLatency() const
    {
        return m_fixedLatency;
    }

    void VideoDevice::setFixedLatency(Time l) { m_fixedLatency = l; }

    VideoDevice::Time VideoDevice::frameLatencyInFrames() const
    {
        return m_frameLatency;
    }

    VideoDevice::Time VideoDevice::frameLatencyInSeconds() const
    {
        return m_frameLatency / videoFormatAtIndex(currentVideoFormat()).hz;
    }

    void VideoDevice::setFrameLatency(Time frames) { m_frameLatency = frames; }

    VideoDevice::Time VideoDevice::totalLatencyInSeconds() const
    {
        return deviceLatency() + frameLatencyInSeconds() + fixedLatency();
    }

    VideoDevice::Resolution VideoDevice::internalResolution() const
    {
        return resolution();
    }

    VideoDevice::Offset VideoDevice::internalOffset() const { return offset(); }

    VideoDevice::Timing VideoDevice::internalTiming() const { return timing(); }

    VideoDevice::VideoFormat VideoDevice::internalFormat() const
    {
        return format();
    }

    size_t VideoDevice::internalWidth() const { return width(); }

    size_t VideoDevice::internalHeight() const { return height(); }

    size_t VideoDevice::pixelSizeInBytes(InternalDataFormat f)
    {
        switch (f)
        {
        case RGB8:
            return sizeof(unsigned char) * 3;
        case RGBA8:
        case BGRA8:
            return sizeof(unsigned char) * 4;
        case RGB16:
            return sizeof(unsigned short) * 3;
        case RGBA16:
            return sizeof(unsigned short) * 4;
        case RGB10X2:
            return sizeof(unsigned int);
        case RGB10X2Rev:
            return sizeof(unsigned int);
        case RGB16F:
            return sizeof(unsigned short) * 3;
        case RGBA16F:
            return sizeof(unsigned short) * 4;
        case RGB32F:
            return sizeof(float) * 3;
        case RGBA32F:
            return sizeof(float) * 4;
        case CbY0CrY1_8_422:
        case Y0CbY1Cr_8_422:
        case Y1CbY0Cr_8_422:
            return sizeof(unsigned char) * 3;
        case YCrCb_AJA_10_422:
        case YCrCb_BM_10_422:
            return sizeof(unsigned int);
        case YCbCr_P216_16_422:
            return sizeof(unsigned short) * 2;
        default:
            abort();
        }

        return 0;
    }

    void VideoDevice::beginTransfer() const {}

    void VideoDevice::endTransfer() const {}

    size_t VideoDevice::hashID(IDType t) const
    {
        string idstr = humanReadableID(t);

        //
        //  remove all whitespace to make it robust against against any
        //  whitespace changes or leading/trailing whitespace.
        //

        boost::hash<string> string_hash;
        string noWhiteSpace = algorithm::replace_all_copy(idstr, " ", "");
        return string_hash(noWhiteSpace);
    }

    string VideoDevice::humanReadableID(IDType t) const
    {
        ostringstream str;
        const string& vd = videoFormatAtIndex(currentVideoFormat()).description;
        const string& dd = dataFormatAtIndex(currentDataFormat()).description;
        const string& moduleName = module()->name();

        switch (t)
        {
        case HostnameVideoAndDataFormatID:
            str << asio::ip::host_name() << "/" << moduleName << "/" << name()
                << "/" << vd << "/" << dd;
            break;
        case VideoAndDataFormatID:
            str << moduleName << "/" << name() << "/" << vd << "/" << dd;
            break;
        case VideoFormatID:
            str << moduleName << "/" << name() << "/" << vd;
            break;
        case DataFormatID:
            str << moduleName << "/" << name() << "/" << dd;
            break;
        case DeviceNameID:
            str << moduleName << "/" << name();
            break;
        case ModuleNameID:
            str << moduleName;
            break;
        }

        return str.str();
    }

    VideoDevice::ColorProfile VideoDevice::colorProfile() const
    {
        return ColorProfile();
    }

} // namespace TwkApp
