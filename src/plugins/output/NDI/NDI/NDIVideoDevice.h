//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GL.h>
#include <TwkUtil/Timer.h>
#include <boost/thread.hpp>
#include <cstddef>
#include <string>

#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>
#include <windows.h>
#include <process.h>
#endif

#ifdef PLATFORM_LINUX
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#endif

#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLFence.h>

#include <Processing.NDI.Lib.h>

#include <iostream>
#include <stl_ext/thread_group.h>
#include <deque>

namespace NDI
{
    class NDIModule;

    using NDIVideoFrame = unsigned char;

    struct NDIDataFormat
    {
        TwkApp::VideoDevice::InternalDataFormat iformat{
            TwkApp::VideoDevice::RGBA8};
        NDIlib_FourCC_video_type_e ndiFormat{NDIlib_FourCC_type_RGBA};
        bool isRGB{true};
        const char* description{nullptr};
    };

    struct NDIVideoFormat
    {
        int width{0};
        int height{0};
        float pixelAspect{1.0f};
        double hertz{0.0};
        float frame_rate_N{0.0f};
        float frame_rate_D{0.0f};
        const char* description{nullptr};
    };

    struct NDIAudioFormat
    {
        int hertz{0};
        TwkAudio::Format precision{TwkAudio::Int16Format};
        size_t numChannels{0};
        TwkAudio::Layout layout{TwkAudio::Stereo_2};
        const char* description{nullptr};
    };

    using NDIVideoFormatVector = std::vector<NDIVideoFormat>;
    using NDIDataFormatVector = std::vector<NDIDataFormat>;

    class NDIVideoDevice : public TwkGLF::GLBindableVideoDevice
    {
    public:
        using Timer = TwkUtil::Timer;
        using GLFence = TwkGLF::GLFence;
        using GLFBO = TwkGLF::GLFBO;
        using DLVideoFrameDeque = std::deque<NDIVideoFrame*>;

        struct PBOData
        {
            enum class State
            {
                Mapped,
                Transferring,
                NeedsUnmap,
                Ready
            };

            PBOData(GLuint g);
            ~PBOData();

            void lockData();
            void unlockData();
            void lockState();
            void unlockState();

            GLuint globject;
            State state;
            const GLFBO* fbo;

        private:
            pthread_mutex_t mutex;
            pthread_mutex_t stateMutex;
        };

        using PBOQueue = std::deque<PBOData*>;

        NDIVideoDevice(NDIModule* ndiModule, const std::string& name);
        virtual ~NDIVideoDevice() final;

        size_t asyncMaxMappedBuffers() const override;
        Time deviceLatency() const override;

        size_t numVideoFormats() const override;
        VideoFormat videoFormatAtIndex(size_t index) const override;
        void setVideoFormat(size_t index) override;
        size_t currentVideoFormat() const override;

        size_t numAudioFormats() const override;
        AudioFormat audioFormatAtIndex(size_t index) const override;
        void setAudioFormat(size_t index) override;
        size_t currentAudioFormat() const override;

        size_t numDataFormats() const override;
        DataFormat dataFormatAtIndex(size_t index) const override;
        void setDataFormat(size_t index) override;
        size_t currentDataFormat() const override;

        size_t numSyncSources() const override;
        SyncSource syncSourceAtIndex(size_t index) const override;
        size_t currentSyncSource() const override;

        size_t numSyncModes() const override;
        SyncMode syncModeAtIndex(size_t index) const override;
        void setSyncMode(size_t index) override;
        size_t currentSyncMode() const override;

        bool readyForTransfer() const override;
        void transfer(const TwkGLF::GLFBO* glfbo) const override;
        void transfer2(const TwkGLF::GLFBO* glfbo1,
                       const TwkGLF::GLFBO* glfbo2) const override;
        void transferAudio(void* interleavedData, size_t n) const override;
        bool willBlockOnTransfer() const override;

        size_t width() const override { return m_frameWidth; }

        size_t height() const override { return m_frameHeight; }

        void open(const StringVector& args) override;
        void close() override;
        bool isOpen() const override;
        void clearCaches() const override;
        VideoFormat format() const override;
        Timing timing() const override;

        void unbind() const override;
        void bind(const TwkGLF::GLVideoDevice* device) const override;
        void bind2(const TwkGLF::GLVideoDevice* device1,
                   const TwkGLF::GLVideoDevice* device2) const override;
        void
        audioFrameSizeSequence(AudioFrameSizeVector& fsizes) const override;

    private:
        void initialize();
        bool transferChannel(size_t index, const TwkGLF::GLFBO*) const;
        void transferChannelPBO(size_t index, const TwkGLF::GLFBO*,
                                NDIVideoFrame*, NDIVideoFrame*) const;
        void transferChannelReadPixels(size_t index, const TwkGLF::GLFBO*,
                                       NDIVideoFrame*, NDIVideoFrame*) const;

        NDIVideoFormatVector m_ndiVideoFormats;
        NDIDataFormatVector m_ndiDataFormats;
        NDIlib_send_instance_t m_ndiSender{nullptr};
        mutable NDIlib_video_frame_v2_t m_ndiVideoFrame;
        mutable NDIlib_audio_frame_interleaved_16s_t
            m_ndiInterleaved16AudioFrame;
        mutable NDIlib_audio_frame_interleaved_32s_t
            m_ndiInterleaved32AudioFrame;
        mutable NDIlib_audio_frame_interleaved_32f_t
            m_ndiInterleaved32fAudioFrame;
        mutable NDIVideoFrame* m_readyFrame{nullptr};
        mutable DLVideoFrameDeque m_DLOutputVideoFrameQueue;
        mutable DLVideoFrameDeque
            m_DLReadbackVideoFrameQueue; // only rgb formats
        mutable bool m_needsFrameConverter{false};
        mutable bool m_hasAudio{false};
        mutable PBOQueue m_pboQueue;
        mutable PBOData* m_lastPboData{nullptr};
        char* m_audioData{nullptr};
        mutable int m_audioDataIndex{0};
        bool m_isInitialized{false};
        bool m_isPbos{false};
        size_t m_pboSize{0};
        size_t m_videoFrameBufferSize{0};
        bool m_isOpen{false};
        size_t m_frameWidth{0};
        size_t m_frameHeight{0};
        mutable size_t m_totalPlayoutFrames{0};
        size_t m_internalAudioFormat{0};
        size_t m_internalVideoFormat{0};
        size_t m_internalDataFormat{0};
        size_t m_internalSyncMode{0};
        size_t m_audioFormatSizeInBytes{0};
        unsigned long m_audioSamplesPerFrame{0};
        unsigned long m_audioChannelCount{0};
        float m_audioSampleRate{0.0f};
        TwkAudio::Format m_audioFormat{TwkAudio::Int16Format};
        GLenum m_textureFormat{GL_RGBA};
        GLenum m_textureType{GL_UNSIGNED_BYTE};
        static bool m_isInfoFeedback;
    };

} // namespace NDI
