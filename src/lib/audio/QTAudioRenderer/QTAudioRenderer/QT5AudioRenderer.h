//******************************************************************************
//  Copyright (c) 2013, 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __audio__QTAudioRenderer__h__
#define __audio__QTAudioRenderer__h__

#include <IPCore/Session.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/Application.h>
#include <TwkMath/Time.h>
#include <TwkUtil/Clock.h>

#include <QtCore/QIODevice>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QAudioFormat>
#include <QtMultimedia/QAudioDeviceInfo>
#include <QtCore/QThread>
#include <QtCore/QMutex>

#include <iostream>

namespace IPCore
{

    class QTAudioThread;
    class QTAudioRenderer;

    class QTAudioIODevice : public QIODevice
    {
        Q_OBJECT

    public:
        QTAudioIODevice(QTAudioThread& audioThread);
        virtual ~QTAudioIODevice();

        virtual qint64 readData(char* data, qint64 maxlen);
        virtual qint64 writeData(const char* data, qint64 len);
        qint64 bytesAvailable() const;

        void start();

    public slots:
        void resetDevice();
        void stopDevice();

    private:
        QTAudioThread& m_thread;
    };

    class QTAudioOutput : public QAudioOutput
    {
        Q_OBJECT

    public:
        QTAudioOutput(QAudioDeviceInfo& audioDeviceInfo,
                      QAudioFormat& audioFormat, QTAudioIODevice& ioDevice,
                      QTAudioThread& audioThread);
        ~QTAudioOutput();

    public slots:
        void startAudio();
        void resetAudio();
        void stopAudio();
        void suspendAudio();
        void suspendAndResetAudio();
        void setAudioOutputBufferSize();
        void play(IPCore::Session* s);

    private:
        int calcAudioBufferSize(const int channels, const int sampleRate,
                                const int sampleSizeInBytes,
                                const int defaultBufferSize) const;
        std::string toString(QAudio::State state);

    private:
        QAudioDeviceInfo& m_device;
        QAudioFormat& m_format;
        QTAudioIODevice& m_ioDevice;
        QTAudioThread& m_thread;
    };

    class QTAudioThread : public QThread
    {
        Q_OBJECT

    public:
        QTAudioThread(QTAudioRenderer& audioRenderer, QObject* parent = 0);
        ~QTAudioThread();

        void startMe();

        size_t processedSamples() const;
        void setProcessedSamples(size_t n);

        size_t startSample() const;
        void setStartSample(size_t value);

        TwkAudio::Time actualPlayedTimeOffset() const;
        void setActualPlayedTimeOffset(TwkAudio::Time t);

        void setDeviceLatency(double t);

        bool preRollDisable() const;
        void setPreRollDisable(bool disable);

        void setPreRollDelay(TwkAudio::Time t);

        int bytesPerSample() const;
        size_t framesPerBuffer() const;

        bool holdOpen() const;

        const AudioRenderer::DeviceState& deviceState() const;
        void setDeviceState(AudioRenderer::DeviceState& state);

        void emitStartAudio();
        void emitResetAudio();
        void emitStopAudio();
        void emitSuspendAudio();
        void emitSuspendAndResetAudio();

        void emitResetDevice();
        void emitStopDevice();

        void emitPlay(IPCore::Session* s);

        //
        //  callback for QTAudioIODevice's readData() to call
        //
        qint64 qIODeviceCallback(char* data, qint64 maxLenInBytes);

        void startOfInitialization();
        TwkUtil::SystemClock::Time endOfInitialization();
        bool isInInitialization() const;

        TwkUtil::SystemClock::Time getLastDeviceLatency() const
        {
            return m_lastDeviceLatency;
        }

    protected:
        virtual void run();

    private:
        bool createAudioOutput();

        void detachAudioOutputDevice();

    private:
        QMutex m_mutex;

        QObject* m_parent;

        QTAudioIODevice* m_ioDevice;
        QTAudioOutput* m_audioOutput;
        TwkAudio::AudioBuffer m_abuffer;
        QTAudioRenderer& m_audioRenderer;

        int m_bytesPerSample;
        size_t m_preRollSamples;
        size_t m_processedSamples;
        size_t m_startSample;
        int m_preRollMode;
        bool m_preRollDisable;

        bool m_patch9355Enabled;
        TwkUtil::SystemClock m_systemClock;
        TwkUtil::SystemClock::Time m_startOfInitialization;
        TwkUtil::SystemClock::Time m_endOfInitialization;
        TwkUtil::SystemClock::Time m_lastDeviceLatency;
    };

    class QTAudioRenderer : public IPCore::AudioRenderer
    {
    public:
        typedef TwkAudio::AudioBuffer AudioBuffer;

        QTAudioRenderer(const RendererParameters&, QObject* parent);
        virtual ~QTAudioRenderer();

        //
        //  AudioRenderer API
        //

        virtual void availableLayouts(const Device&, LayoutsVector&);
        virtual void availableFormats(const Device&, FormatVector&);
        virtual void availableRates(const Device&, Format, RateVector&);

        //
        //  play() will return almost immediately -- a worker thread will
        //  be released and start playing.
        //

        virtual void play();
        virtual void play(IPCore::Session*);

        //
        //  stop() will cause the worker thread to wait until play.
        //

        virtual void stop();
        virtual void stop(IPCore::Session*);

        //
        //  shutdown() close all hardware devices
        //

        virtual void shutdown();

        //
        // T is the Application type that is adding this
        // renderer as an audio module e.g. 'RvApplication'
        // or 'NoodleApplication'.
        template <class T>
        static IPCore::AudioRenderer::Module addQTAudioModule();

        TwkAudio::Format getTwkAudioFormat() const;
        TwkAudio::Format
        convertToTwkAudioFormat(int fmtSize,
                                QAudioFormat::SampleType fmtType) const;

        void setSampleSizeAndType(Layout twkLayout, Format twkFormat,
                                  QAudioFormat& qformat) const;

        friend class QTAudioThread;

    private:
        bool supportsRequiredChannels(const QAudioDeviceInfo& info) const;

        void init();

        void initDeviceList();

    private:
        QObject* m_parent;
        QTAudioThread* m_thread;
        QAudioFormat m_format;
        QAudioDeviceInfo m_device;
        QList<QAudioDeviceInfo> m_deviceList;
        std::string m_codec;
    };

    template <class T>
    static IPCore::AudioRenderer*
    createQTAudio(const IPCore::AudioRenderer::RendererParameters& p)
    {
        return (new QTAudioRenderer(p, static_cast<T*>(IPCore::App())));
    }

    template <class T>
    IPCore::AudioRenderer::Module QTAudioRenderer::addQTAudioModule()
    {
        return IPCore::AudioRenderer::Module("Platform Audio", "",
                                             createQTAudio<T>);
    }

} // namespace IPCore

#endif // __audio__QTAudioRenderer__h__
