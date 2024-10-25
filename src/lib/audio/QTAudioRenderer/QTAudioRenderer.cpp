///
//  Copyright (c) 2013,2014 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <string>
#include <QTAudioRenderer/QTAudioRenderer.h>

#include <IPCore/IPGraph.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/Log.h>
#include <TwkUtil/ThreadName.h>

#include <QtCore/qmath.h>
#include <QtCore/qendian.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include <iostream>
#include <sstream>

#include <stdlib.h>

//
// This define is really a workaround to avoid
// a 'Pollable event error' in the linux kernel
// when a QtAudio device is start and stopped often
// (for the case hold open false; i.e. audio resources
// are released on stop).
// If we run the audio in the main thread, this error
// does not occur. So thats the workaround, otherwise
// rv will crash.
// Followup; it looks like this is not longer
// reproducible under centos7 or my centos6 vm
// (parallels 11). So commenting out RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN.
//
// 
//
#ifdef PLATFORM_LINUX
//#define RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
#endif

namespace IPCore {
using namespace std;


#ifdef DEBUG_QTAUDIO
#define QTAUDIO_DEBUG(_f) \
    if (AudioRenderer::debugVerbose) TwkUtil::Log("AUDIO") << _f << " Qthread:" << QThread::currentThread();
#else
#define QTAUDIO_DEBUG(_f)
#endif

static ENVVAR_BOOL( evApplyWasapiFix, "RV_AUDIO_APPLY_WASAPI_FIX", true );

// Utility function that returns the Sample size.
int sampleSize(const QAudioFormat& format) {
    #ifdef RV_VFX_CY2023
        return format.sampleSize() / 8;
    #else
        return format.bytesPerSample();
    #endif
}

// NOTE_QT: In theory, this should be called sampleFormat for Qt6. Keep it as is for now.
SampleFormat sampleType(const QAudioFormat& format)
{
    #ifdef RV_VFX_CY2023
        return format.sampleType();
    #else
        return format.sampleFormat();
    #endif
}
//----------------------------------------------------------------------
//      QTAudioThread
//
//      This is the Qt thread class that lives
//      with main/UI thread but holds the
//      audio thread and has interThread "emit" methods to invoke methods
//      in the audio thread which control
//      the start/resume/stop/open behavior of
//      the QAudioOutput and QIODevice running within it.
//
//----------------------------------------------------------------------

QTAudioThread::QTAudioThread(QTAudioRenderer &audioRenderer,
                             QObject* parent) :
    QThread(parent)
    , m_parent(parent)
    , m_ioDevice(0)
    , m_audioOutput(0)
    , m_audioRenderer(audioRenderer)
    , m_preRollSamples(0)
    , m_processedSamples(0)
    , m_startSample(0)
    , m_preRollDisable(false)
    , m_startOfInitialization(0)
    , m_endOfInitialization(-1)
    , m_lastDeviceLatency(0)
    , m_patch9355Enabled(false)

{
    QTAUDIO_DEBUG("QTAudioThread")

    // Cache some format related variables so we dont have to
    // precompute them each time.
    m_bytesPerSample = audioRenderer.m_format.channelCount() * sampleSize(audioRenderer.m_format);

    m_preRollMode = m_audioRenderer.m_parameters.preRoll;

    qRegisterMetaType<IPCore::Session*>("IPCore::Session*");

    m_patch9355Enabled = (getenv("RV_AUDIO_AUTO_ADJUST") != NULL);
}

QTAudioThread::~QTAudioThread()
{
    detachAudioOutputDevice();
    disconnect(m_parent);
}


void
QTAudioThread::startMe()
{
#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        if (!createAudioOutput()) return;
        m_audioOutput->setAudioOutputBufferSize();
        return;
    }
#endif

    start();
    unsigned long waitTime = 0; // in ms; 
    do {
        wait(300);  // in ms;
        waitTime += 300;
        QTAUDIO_DEBUG("QTAudioThread::startMe(): waiting for thread to start...")
    } while (!isRunning() && waitTime < 1000);
    QMetaObject::invokeMethod(m_audioOutput, "setAudioOutputBufferSize", Qt::QueuedConnection);
}


size_t
QTAudioThread::processedSamples() const
{
    return m_processedSamples;
}

void
QTAudioThread::setProcessedSamples(size_t n)
{
    m_mutex.lock();
    m_processedSamples = n;
    m_mutex.unlock();
}

size_t
QTAudioThread::startSample() const
{
    return m_startSample;
}

void
QTAudioThread::setStartSample(size_t n)
{
    m_mutex.lock();
    m_startSample = n;
    m_mutex.unlock();
}

void
QTAudioThread::setDeviceLatency(double t)
{
    m_mutex.lock();
    m_audioRenderer.m_deviceState.latency =  t + m_audioRenderer.m_parameters.latency; 
    m_mutex.unlock();
}

bool
QTAudioThread::preRollDisable() const
{
    return m_preRollDisable; 
}

void
QTAudioThread::setPreRollDisable(bool disable)
{
    m_mutex.lock();
    m_preRollDisable = disable; 
    m_mutex.unlock();
}


void
QTAudioThread::setPreRollDelay(TwkAudio::Time t)
{
    m_mutex.lock();
    m_audioRenderer.setPreRollDelay(t);
    if (t == 0 ) 
    {
        m_preRollDisable = false;
        m_preRollSamples = 0;
    }
    m_mutex.unlock();
}

int
QTAudioThread::bytesPerSample() const
{
    return m_bytesPerSample;
}

size_t
QTAudioThread::framesPerBuffer() const
{
    return m_audioRenderer.m_parameters.framesPerBuffer;
}


bool 
QTAudioThread::holdOpen() const
{ 
    return m_audioRenderer.m_parameters.holdOpen;
}


void
QTAudioThread::emitStartAudio()
{
    QTAUDIO_DEBUG("QTAudioThread::emitStartAudio")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_audioOutput->startAudio();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_audioOutput, "startAudio", Qt::QueuedConnection);
}

const AudioRenderer::DeviceState&
QTAudioThread::deviceState() const
{
    return m_audioRenderer.deviceState();
}

void
QTAudioThread::setDeviceState(AudioRenderer::DeviceState &state)
{
    m_mutex.lock();
    m_audioRenderer.setDeviceState(state);
    m_mutex.unlock();
}

void
QTAudioThread::emitResetAudio()
{
    QTAUDIO_DEBUG("QTAudioThread::emitResetAudio")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_audioOutput->resetAudio();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_audioOutput, "resetAudio", Qt::QueuedConnection);
}

void
QTAudioThread::emitStopAudio()
{
    QTAUDIO_DEBUG("QTAudioThread::emitStopAudio")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_audioOutput->stopAudio();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_audioOutput, "stopAudio", Qt::BlockingQueuedConnection);
}


void
QTAudioThread::emitSuspendAudio()
{
    QTAUDIO_DEBUG("QTAudioThread::emitSuspendAudio")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_audioOutput->suspendAudio();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_audioOutput, "suspendAudio", Qt::BlockingQueuedConnection);
}


void
QTAudioThread::emitSuspendAndResetAudio()
{
    QTAUDIO_DEBUG("QTAudioThread::emitSuspendAndResetAudio")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_audioOutput->suspendAndResetAudio();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_audioOutput, "suspendAndResetAudio", Qt::BlockingQueuedConnection);
}


void
QTAudioThread::emitResetDevice()
{
    QTAUDIO_DEBUG("QTAudioThread::emitResetDevice")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_ioDevice->resetDevice();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_ioDevice, "resetDevice", Qt::QueuedConnection);
}

void
QTAudioThread::emitStopDevice()
{
    QTAUDIO_DEBUG("QTAudioThread::emitStopDevice")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_ioDevice->stopDevice();
        return;
    }
#endif

    QMetaObject::invokeMethod(m_ioDevice, "stopDevice", Qt::BlockingQueuedConnection);
}

void
QTAudioThread::emitPlay(IPCore::Session *s)
{
    QTAUDIO_DEBUG("QTAudioThread::emitPlay")

#ifdef RUN_IN_MAIN_THREAD_ON_NO_HOLDOPEN
    if (!m_audioRenderer.m_parameters.holdOpen)
    {
        m_audioOutput->play(s);
        return;
    }
#endif

    // This needs to be a blocking connection to ensure play is as close to
    // starting for real. This improves the av sync lag especially om Windows.
    //
    QMetaObject::invokeMethod(m_audioOutput, "play", Qt::BlockingQueuedConnection, Q_ARG(IPCore::Session*, s));
}

void
QTAudioThread::run()
{
    QTAUDIO_DEBUG("QTAudioThread::run")
    TwkUtil::setThreadName("QTAudioThread");

    // IMPORTANT: createAudioOutput() call
    // can and must only be called within run()
    // so that the QTAudioOuput and QTAudioIODevice
    // is created within run()'s execution thread.
    if (!createAudioOutput()) return;

    exec();
}

bool
QTAudioThread::createAudioOutput()
{
    QMutexLocker lock(&m_mutex);
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "createAudioOutput()";

    // Create the QAudioOutput and QIO devices

    m_bytesPerSample = m_audioRenderer.m_format.channelCount() * sampleSize(m_audioRenderer.m_format);

    if (m_ioDevice = new QTAudioIODevice(*this))
    {
        if (m_audioOutput = new QTAudioOutput(m_audioRenderer.m_device,
                                              m_audioRenderer.m_format,
                                              *m_ioDevice,
                                              *this))
        {
            m_ioDevice->start();
            return true;
        }
        else
        {
            m_audioRenderer.setErrorCondition("Unable to create QIODevice for QAudioOutput.");
            return false;
        }
    }
    else
    {
        m_audioRenderer.setErrorCondition("Unable to create QAudioOutput.");
        return false;
    }

    return true;
}

void
QTAudioThread::detachAudioOutputDevice()
{
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "detachAudioOutputDevice";

    if (m_ioDevice)
    {
        emitStopDevice();
    }

    if (m_audioOutput)
    {
        emitStopAudio();
    }

    quit();
    wait();

    if (m_audioOutput)
    {
        delete m_audioOutput;
        m_audioOutput = 0;
    }

    if (m_ioDevice)
    {
        delete m_ioDevice;
        m_ioDevice = 0;
    }

}

//
// This is the callback called by QTAudioIODevice::readData() for
// pushing audio data to the audio device.
//
qint64
QTAudioThread::qIODeviceCallback(char* data, qint64 maxLenInBytes)
{
    if (m_patch9355Enabled && isInInitialization())
    {
        TwkUtil::SystemClock::Time dura = endOfInitialization();
        setDeviceLatency(dura);
        m_lastDeviceLatency = dura;
        if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "Initialization duration:" << dura << "s";
    }

    if (AudioRenderer::debugVerbose)
    {
        TwkUtil::Log("AUDIO") 
            << "qIODeviceCallback: asking maxLenInBytes=" << (int) maxLenInBytes
            << " isPlayingAudio=" << (int) m_audioRenderer.isPlaying()
            << " audiooutput state=" << (int) m_audioOutput->state();
    }

    const AudioRenderer::DeviceState &state = deviceState();

    if (data &&
        (m_audioRenderer.isPlaying()) &&
        (maxLenInBytes >= m_bytesPerSample) &&
        ((m_audioOutput->state() == QAudio::ActiveState) || 
         (m_audioOutput->state() == QAudio::IdleState)))
    {
        const int bufferSize = m_audioOutput->bufferSize(); 

#if defined( RV_VFX_CY2023 )
        const int periodSize = m_audioOutput->periodSize();
#else
        // TODO_QT THIS MIGHT REQUIRE TRY AND ERROR
        // Calculate the period size based on the buffer size and bytes per frame.
        const int periodSize = bufferSize / m_audioRenderer.m_format.bytesPerFrame();
#endif

#ifdef PLATFORM_DARWIN
        const bool doPreRoll = false;
#else
        const bool doPreRoll = ((m_preRollMode && !m_preRollDisable && maxLenInBytes >= bufferSize - periodSize)?true:false);
#endif

#ifdef PLATFORM_DARWIN
        //
        //  Figure out the latency.
        //  Only valid for OSX as processedUSecs() doesnt return 
        //  anything useful on Linux/Windows.
        qint64 processedUSecs = m_audioOutput->processedUSecs();
        TwkAudio::Time actualPlayedTime = TwkAudio::Time(processedUSecs / 1000000.0);
        TwkAudio::Time targetPlayedTime = TwkAudio::samplesToTime(m_processedSamples, state.rate);
        setDeviceLatency(targetPlayedTime - actualPlayedTime);
#endif

        //
        // I am qualifying this on a per platform basis. 
        //
        // On OSX, I found that QAudioOutput requires that you write 
        // periodSize() bytes at a time. Otherwise I found subtle crackles
        // in the playback. Try a sine 48kHz test.
        // 
        // On Linux, we want to write no more than bufferSize.
        // Writing anything smaller can potential cause a corrupt audio
        // buffer when running multiple RV through mixer like pulse.
        // 
#ifdef PLATFORM_DARWIN
        if (periodSize && maxLenInBytes > periodSize)
        {
            maxLenInBytes = periodSize;
        }
#else
        if (bufferSize && maxLenInBytes > bufferSize)
        {
            maxLenInBytes = bufferSize;
        }
#endif

        //
        // If the request bytes i.e. maxLenInBytes is within
        // a periodSize worths of the bufferSize we do a silent preroll
        // fill of the audio device.
        // This prevents almost any lag time on start of play for an
        // oversized audiobufferfill request.
        // NB: This does not apply to DARWIN where writes happen
        // at max periodSize amounts.
        if (doPreRoll && m_preRollSamples == 0) 
        {
            memset(data, 0, maxLenInBytes);

            const size_t preRollSamplesWritten = (size_t) (maxLenInBytes / m_bytesPerSample);
            m_preRollSamples += preRollSamplesWritten;
            m_processedSamples += preRollSamplesWritten;

            if (AudioRenderer::debug)
            {
                TwkUtil::Log("AUDIO") 
                    << "qIODeviceCallback: PreRollWrite: " 
                    << " bufferSize=" << bufferSize
                    << " periodSize=" << periodSize
                    << " maxLenInBytes=" << maxLenInBytes 
                    << " PreRollDelay=" << TwkAudio::samplesToTime(preRollSamplesWritten, state.rate);
            }

            return  maxLenInBytes;
        }

        if (m_preRollMode && !m_preRollDisable && m_preRollSamples > 0)
        {
            //
            // After much fiddling I found this to be the best rule.
            // I noticed that there seems to be a residual buffer of one
            // period size (noticed from the difference in the first two the buffer
            // request when play first starts).
            // NB: the preRoll delay is divided by the number of channels since
            // this is actual delay in time during playback.
            //  
#ifdef PLATFORM_LINUX
            TwkAudio::Time writtenPreRollTime = 
                TwkAudio::samplesToTime(m_preRollSamples-(periodSize /m_bytesPerSample), state.rate) / TwkAudio::channelsCount(state.layout);
#else
            TwkAudio::Time writtenPreRollTime = 
                TwkAudio::samplesToTime(m_preRollSamples, state.rate) / TwkAudio::channelsCount(state.layout);
#endif

            m_audioRenderer.setPreRollDelay(writtenPreRollTime);

            if (AudioRenderer::debug)
            {
                TwkUtil::Log("AUDIO")
                    << "qIODeviceCallback: PreRollWrite: " 
                    << " Total PreRoll Delay=" << m_audioRenderer.preRollDelay();
            }
        }

        m_preRollDisable = true;

        if (AudioRenderer::debugVerbose)
        {
            TwkUtil::Log("AUDIO") 
                << "qIODeviceCallback: NormalWrite: " 
                << " m_startSample=" << m_startSample 
                << " startSampleTime=" << TwkAudio::samplesToTime(m_startSample, state.rate)
                << " m_deviceState.latency: " << m_audioRenderer.m_deviceState.latency
                << " maxLenInBytes=" << maxLenInBytes;
        }
        
        size_t numSamplesToWrite = (size_t) (maxLenInBytes / m_bytesPerSample); 
        size_t numSamplesForAbuffer = numSamplesToWrite;
        size_t bytesWrittenToDevice = 0;

        if (numSamplesForAbuffer)
        {
            QMutexLocker lock(&m_mutex);

            //  Fetch the samples
            //
            m_abuffer.reconfigure(numSamplesForAbuffer,
                                  state.layout,
                                  TwkAudio::Time(state.rate),
                                  TwkAudio::samplesToTime(m_startSample, state.rate));

            m_abuffer.zero();

            try
            {
                m_audioRenderer.audioFillBuffer(m_abuffer);
            }
            catch (std::exception &exc)
            {
                cout << "WARNING: QAudio fillBuffer exception: " << exc.what() << endl;
            }

            switch (state.format)
            {
              case TwkAudio::Float32Format:
                  bytesWrittenToDevice = m_abuffer.sizeInBytes();
                  m_startSample += numSamplesForAbuffer;
                  m_processedSamples += numSamplesForAbuffer;
                  memcpy(data, m_abuffer.pointer(), bytesWrittenToDevice);
                  break;

              case TwkAudio::Int32Format:
                  bytesWrittenToDevice = m_abuffer.sizeInBytes();
                  m_startSample += numSamplesForAbuffer;
                  m_processedSamples += numSamplesForAbuffer;
                  transform(m_abuffer.pointer(),
                            m_abuffer.pointer() + m_abuffer.sizeInFloats(),
                            (int*) data,
                            AudioRenderer::toType<int>);
                  break;

              case TwkAudio::Int24Format:
                  bytesWrittenToDevice = m_abuffer.sizeInFloats() * sizeof(char) * 3;
                  m_startSample += numSamplesForAbuffer;
                  m_processedSamples += numSamplesForAbuffer;
                  AudioRenderer::transformFloat32ToInt24(m_abuffer.pointer(),
                                                         data,
                                                         m_abuffer.sizeInFloats(),
#if defined( RV_VFX_CY2023 )
    (m_audioRenderer.m_format.byteOrder() == QAudioFormat::LittleEndian));
#else
    // TODO_QT QUERY THE SYSTEM
    true);
#endif
                                                         
                  break;

              case TwkAudio::Int16Format:
                  bytesWrittenToDevice = m_abuffer.sizeInFloats() * sizeof(short);
                  m_startSample += numSamplesForAbuffer;
                  m_processedSamples += numSamplesForAbuffer;
                  transform(m_abuffer.pointer(),
                            m_abuffer.pointer() + m_abuffer.sizeInFloats(),
                            (short*) data,
                            AudioRenderer::toType<short>);
                  break;

              case TwkAudio::Int8Format:
                  bytesWrittenToDevice = m_abuffer.sizeInFloats() * sizeof(signed char);
                  m_startSample += numSamplesForAbuffer;
                  m_processedSamples += numSamplesForAbuffer;
#ifdef PLATFORM_DARWIN
                  transform(m_abuffer.pointer(),
                            m_abuffer.pointer() + m_abuffer.sizeInFloats(),
                            (signed char*) data,
                            AudioRenderer::toType<signed char>);
#else
                  transform(m_abuffer.pointer(),
                            m_abuffer.pointer() + m_abuffer.sizeInFloats(),
                            (unsigned char*) data,
                            AudioRenderer::toUnsignedType<unsigned char>);
#endif
                  break;

              default:
                  cout << "WARNING: Unsupported format for QAudio: " << endl;
                  break;
            }
        }

        if (AudioRenderer::debugVerbose)
        {
            TwkUtil::Log("AUDIO")
                << "qIODeviceCallback: NormalWrite: "
                << " numSamplesForAbuffer=" << numSamplesForAbuffer
                << " bytesWrittenToDevice=" << bytesWrittenToDevice
                << " m_processedSamples=" << m_processedSamples;

            if (bytesWrittenToDevice != maxLenInBytes)
            {
                cout << "AUDIO: qIODeviceCallback: WARNING bytesWrittenToDevice != maxLenInBytes" << endl;
            }
        }

        return ((qint64) bytesWrittenToDevice);
    }
    else
    {
        return 0;
    }
}

void 
QTAudioThread::startOfInitialization()
{
    m_startOfInitialization = m_systemClock.now();
    m_endOfInitialization = -1;
}

TwkUtil::SystemClock::Time 
QTAudioThread::endOfInitialization()
{
    m_endOfInitialization = m_systemClock.now();
    return (m_endOfInitialization - m_startOfInitialization);
}

bool QTAudioThread::isInInitialization() const
{
    return (m_endOfInitialization < 0);
}
    
//----------------------------------------------------------------------
//      QTAudioIODevice
//
//      This is the Qt audio IO device class that lives
//      with the audio thread.
//----------------------------------------------------------------------
QTAudioIODevice::QTAudioIODevice(QTAudioThread &audiothread) :
    m_thread(audiothread)
{
    QTAUDIO_DEBUG("QTAudioIODevice:")
}

QTAudioIODevice::~QTAudioIODevice()
{
}

void
QTAudioIODevice::start()
{
    QTAUDIO_DEBUG("QTAudioIODevice::start")
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void
QTAudioIODevice::stopDevice()
{
    QTAUDIO_DEBUG("QTAudioIODevice::stopDevice")
    close();
}

void
QTAudioIODevice::resetDevice()
{
    QTAUDIO_DEBUG("QTAudioIODevice::resetDevice")
    reset();
}

qint64 QTAudioIODevice::readData(char* data, qint64 maxLenInBytes)
{
    //  QTAUDIO_DEBUG("QTAudioIODevice::readData")
    int bytesWritten = m_thread.qIODeviceCallback(data, maxLenInBytes);
    return bytesWritten;
}

qint64 QTAudioIODevice::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 QTAudioIODevice::bytesAvailable() const
{

    return QIODevice::bytesAvailable();
}

//----------------------------------------------------------------------
//      QTAudioOutput
//
//      This is the Qt audio output class that lives
//      with the audio thread.
//----------------------------------------------------------------------
#if defined( RV_VFX_CY2023 )
QTAudioOutput::QTAudioOutput(QAudioDeviceInfo &audioDevice,
#else
QTAudioOutput::QTAudioOutput(QAudioDevice &audioDevice,
#endif
                             QAudioFormat &audioFormat,
                             QTAudioIODevice &ioDevice,
                             QTAudioThread &audioThread) :
#if defined( RV_VFX_CY2023 )
    QAudioOutput(audioDevice, audioFormat)
#else
    QAudioSink(audioDevice, audioFormat)
#endif
    , m_device(audioDevice)
    , m_format(audioFormat)
    , m_ioDevice(ioDevice)
    , m_thread(audioThread)
{
    QTAUDIO_DEBUG("QTAudioOutput")
}

QTAudioOutput::~QTAudioOutput()
{
}

std::string QTAudioOutput::toString(QAudio::State state )
{
    switch(state)
    {
    case QAudio::ActiveState: return "active";
    case QAudio::SuspendedState: return "suspended";
    case QAudio::StoppedState: return "stopped";
    case QAudio::IdleState: return "idle";
    default: return "???";
    }
}

void
QTAudioOutput::startAudio()
{
    QTAUDIO_DEBUG("QTAudioOutput::startAudio check")

    if (state() == QAudio::StoppedState)
    {
        m_thread.startOfInitialization();

        QTAUDIO_DEBUG("QTAudioOutput::startAudio start()")
#ifdef PLATFORM_WINDOWS
        // This is needed for Windows because when QAudioOutput
        // reset() or stop() is called (thus putting the
        // device into the StoppedState), the bufferSize also gets
        // reset to zero.
        // So we need to recalc the buffer size we want to use
        // again before calling start().
        setAudioOutputBufferSize();
#endif
        m_thread.setStartSample(0);
        m_thread.setProcessedSamples(0);
        m_thread.setPreRollDelay(0);
        start(&m_ioDevice);

    }
    else if (state() == QAudio::SuspendedState)
    {
        QTAUDIO_DEBUG("QTAudioOutput::startAudio resume()")
#ifdef PLATFORM_LINUX
        m_thread.setPreRollDelay(0);
#endif
        resume();
    }
}

void
QTAudioOutput::resetAudio()
{
    QTAUDIO_DEBUG("QTAudioOutput::resetAudio")
    reset();
}

void
QTAudioOutput::stopAudio()
{
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "Stop audio output";

    QTAUDIO_DEBUG("QTAudioOutput::stopAudio")

    if (state() != QAudio::StoppedState)
    {
        stop();
    }
}

void
QTAudioOutput::suspendAudio()
{
    QTAUDIO_DEBUG("QTAudioOutput::suspendAudio")

    if (state() != QAudio::StoppedState)
    {
        suspend();
    }
}

void
QTAudioOutput::suspendAndResetAudio()
{
    QTAUDIO_DEBUG("QTAudioOutput::suspendAndResetAudio")

    if (state() != QAudio::StoppedState)
    {
        suspend();
        QTAUDIO_DEBUG("QTAudioOutput::suspendAndResetAudio - before reset")
        reset();
        QTAUDIO_DEBUG("QTAudioOutput::suspendAndResetAudio - after reset")
    }
}

int
QTAudioOutput::calcAudioBufferSize(const int channels,
                                   const int sampleRate,
                                   const int sampleSizeInBytes,
                                   const int defaultBufferSize) const
{
    // For sample rates 8k,11k 8bit, make the bufferSize
    // small to avoid audio cache misses. 
    if (m_format.sampleRate() < 22000 &&
        m_thread.bytesPerSample() <= 2 )
    {
        return 1024;
    }

    // QAudioOutput's default is 8192 bytes. (OSX)
    // QAudioOutput's default is 200ms for all channels. (Windows)
    const float minBufferSizeInTimePerChannel = 0.1f; 

    // Compute buffer size so it is at least minBufferSizeInTimePerChannel.
    float defaultBufferSizeInTimePerChannel = (defaultBufferSize / sampleSizeInBytes) / ((float) (sampleRate * channels));

    if (defaultBufferSizeInTimePerChannel < minBufferSizeInTimePerChannel)
    {
        return ((int) ceilf(minBufferSizeInTimePerChannel * ((float) (sampleRate * channels * sampleSizeInBytes))));
    }
    else
    {
        return defaultBufferSize;
    }
}

void
QTAudioOutput::setAudioOutputBufferSize()
{
    QTAUDIO_DEBUG("QTAudioOutput::setAudioOutputBufferSize")

    // For linux we need to start the m_audioOutput to determine the buffersize thats
    // used.
    // On Windows, if we dont set the bufferSize (use the default), then the size of the buffer
    // is set by the audioOutput start() call.

    // The AudioRenderer::defaultParameters().framesPerBuffer is zero it means the
    // audio preference "Audio Cache samples" is zero;
    // in which case we calculate what the audioOutput
    // bufferSize should be.
    
    if (AudioRenderer::defaultParameters().framesPerBuffer == 0)
    {
#ifdef PLATFORM_DARWIN
        setBufferSize(calcAudioBufferSize(m_format.channelCount(),
                                          m_format.sampleRate(),
                                          sampleSize(m_format),
                                          bufferSize()));
#endif

#ifdef PLATFORM_WINDOWS
        // Note On Windows we need to set the buffer size 10x smaller because it is resize to 
        //      a 10x value when the AudioOutput is started.
        // First determine if this is a WASAPI device
        // Note that on windows, an audio device can come from of those two Qt
        // Audio Plugins: 
        // WASAPI (Windows Audio Session API) or WinMM (Windows Multimedia).
        // Robert's fix above needs to be applied only to WASAPI audio devices.
        // In Qt 5.12.5, there is no mechanism to identify from which audio 
        // plugin an audio device orginated. So for the time being we will always
        // assume that it is an AWASPI device unless we detect that the WASAPI Qt
        // audio plugin dll was removed from the Qt/Audio plugin directory.
        // This is a temporary mechanism that will unblock our clients
        // without introducing an RV preference that would have polluted the RV
        // preferences unnecessarily.
        const bool audioDeviceIsWASAPI = m_device.realm() == "wasapi";
        if (audioDeviceIsWASAPI && evApplyWasapiFix.getValue())
        {
            setBufferSize(calcAudioBufferSize(m_format.channelCount(),
                                            m_format.sampleRate(),
                                            sampleSize(m_format),
                                            bufferSize()) / 10);
        }
        else
        {
            setBufferSize(calcAudioBufferSize(m_format.channelCount(),
                                            m_format.sampleRate(),
                                            sampleSize(m_format),
                                            bufferSize()));
        }
#endif
    }
    else
    {
        // Note: This might not always succeed as audio device buffer cannot
        //       be set below a certain size.
        //       If you hear crackles on playback at high sample rates
        //       this is probably due to the buffer not being filled fast
        //       enough because it is too small... so a user can increase
        //       the size in the preference tab i.e. "Device Packet Size".

#ifdef PLATFORM_LINUX
        // For linux it is in ms * 1000.
        int targetBufferSize = (1000000 * m_thread.framesPerBuffer() / m_format.sampleRate());
#else
        int targetBufferSize = m_thread.framesPerBuffer() * m_thread.bytesPerSample();
#endif
        setBufferSize(targetBufferSize);
        if (bufferSize() != targetBufferSize)
        {
            cout << "Warning: Audio Device Cache of size " << m_thread.framesPerBuffer() <<
                " samples cannot be set below the device min sample limit of "
                << bufferSize() / m_thread.bytesPerSample()
                << " samples. Using device min limit instead." << endl;
        }
    }

    AudioRenderer::DeviceState newState = m_thread.deviceState();
    newState.framesPerBuffer = bufferSize() / m_thread.bytesPerSample();
    m_thread.setDeviceState(newState);

    if (AudioRenderer::debug)
    {
        TwkUtil::Log("AUDIO") << "setAudioOutputBufferSize: bufferSize= " << bufferSize();
    }
}

void
QTAudioOutput::play(IPCore::Session* s)
{
    if (AudioRenderer::debug) TwkUtil::Log("QTAudioOutput") << "play()";


#ifdef DEBUG_QTAUDIO
    if (AudioRenderer::debugVerbose)
        TwkUtil::Log("QTAudioThread") << "setup thread:" <<  QThread::currentThread() 
            << " isRunning=" << (int) m_thread.isRunning();
#endif

    m_thread.setDeviceLatency(m_thread.getLastDeviceLatency());

    AudioRenderer::DeviceState newState = m_thread.deviceState();
    newState.framesPerBuffer = bufferSize() / m_thread.bytesPerSample();
    m_thread.setDeviceState(newState);

    s->audioConfigure();

    if (AudioRenderer::debug)
    {
        TwkUtil::Log("QTAudioOutput") << "play session:setup: startSample =" << m_thread.startSample()
             << " shift=" << s->shift()
             << " isPlaying=" << (int) s->isPlaying() 
             << " isScrubbingAudio=" << (int) s->isScrubbingAudio() 
             << " audioOutput state=" << (int) state()
             << " audioOutput bufferSize=" << (int) bufferSize()
#if defined( RV_VFX_CY2023 )
             << " audioOutput periodSize=" << (int) periodSize();
             // TODO_QT: Is there a equivalent in Qt6?
#else
;
#endif
    }
}

//----------------------------------------------------------------------
//      QTAudioRenderer
//
//      This is the audio render class that lives
//      with the main/UI thread.
//
//----------------------------------------------------------------------
QTAudioRenderer::QTAudioRenderer(const RendererParameters &params,
                                 QObject* parent) :
    AudioRenderer(params)
    , m_parent(parent)
    , m_codec("audio/pcm")
{
    init();
    QTAUDIO_DEBUG("QTAudioRenderer")
}

QTAudioRenderer::~QTAudioRenderer()
{
    if (m_thread)
    {
        delete m_thread;
    }
}

TwkAudio::Format
QTAudioRenderer::getTwkAudioFormat() const
{
#if defined( RV_VFX_CY2023 )
    return convertToTwkAudioFormat(sampleSize(m_format),
                                   sampleType(m_format));
#else
    return convertToTwkAudioFormat(sampleType(m_format));
#endif
}

TwkAudio::Format
#if defined( RV_VFX_CY2023 )
QTAudioRenderer::convertToTwkAudioFormat(int fmtSize,
                                         SampleFormat fmtType) const
#else
QTAudioRenderer::convertToTwkAudioFormat(SampleFormat fmtType) const
#endif
{
    //TODO_QT
    
    // Qt5
    // QAudioFormat::Unknown	    0	Not Set
    // QAudioFormat::SignedInt	    1	Samples are signed integers
    // QAudioFormat::UnSignedInt	2	Samples are unsigned intergers
    // QAudioFormat::Float	        3	Samples are floats

    // Qt6
    // QAudioFormat::Unknown	    0	Not Set
    // QAudioFormat::UInt8	        1	Samples are 8 bit unsigned integers
    // QAudioFormat::Int16	        2	Samples are 16 bit signed integers
    // QAudioFormat::Int32	        3	Samples are 32 bit signed integers
    // QAudioFormat::Float	        4	Samples are floatse

    // Keeping the fmtSize parameters for now, but it is not needed because 
    // QAudioFormat::SampleFormat tell us the size and the type.

#if defined( RV_VFX_CY2023 )
    switch (fmtSize)
    {
      case 8:
          switch (fmtType)
          {
            case QAudioFormat::SignedInt:
                return TwkAudio::Int8Format;
                break;

            default:
                break;
          }

          break;

      case 16:
          switch (fmtType)
          {
            case QAudioFormat::SignedInt:
                return TwkAudio::Int16Format;
                break;

            default:
                break;
          }

          break;

      case 24:
          switch (fmtType)
          {
            case QAudioFormat::SignedInt:
                return TwkAudio::Int24Format;
                break;

            default:
                break;
          }

          break;

      case 32:
          switch (fmtType)
          {
            case QAudioFormat::SignedInt:
                return TwkAudio::Int32Format;
                break;

            case QAudioFormat::Float:
                return TwkAudio::Float32Format;
                break;

            default:
                break;
          }

          break;

      case 64:
          break;
    }
#else
    switch (fmtType) {
        case QAudioFormat::UInt8:
            // skip for now
            break;
        case QAudioFormat::Int16:
            return TwkAudio::Int16Format;
        case QAudioFormat::Int32:
            return TwkAudio::Int32Format;
        case QAudioFormat::Float:
            return TwkAudio::Float32Format;
        default:
            break;
    }
#endif

    return TwkAudio::UnknownFormat;
}

void
QTAudioRenderer::availableLayouts(const Device &d, LayoutsVector &layouts)
{
#if defined( RV_VFX_CY2023 )
    const QAudioDeviceInfo &device  = m_deviceList[d.index];
    const QList<int> channelCounts  = device.supportedChannelCounts();
#else
    const QAudioDevice &device      = m_deviceList[d.index];
#endif

    layouts.clear();

#if defined( RV_VFX_CY2023 )
    for (QList<int>::const_iterator ci = channelCounts.begin(); ci != channelCounts.end(); ci++)
    {
        LayoutsVector l = TwkAudio::channelLayouts(*ci);
        for (int i = 0; i < l.size(); i++) layouts.push_back(l[i]);
    }
#else
    QAudioFormat testFormat;
    for (int channelCount = device.minimumChannelCount(); channelCount <= device.maximumChannelCount(); ++channelCount)
    {
        testFormat.setChannelCount(channelCount);
        if (device.isFormatSupported(testFormat))
        {
            LayoutsVector l = TwkAudio::channelLayouts(channelCount);
            for (int i = 0; i < l.size(); i++) layouts.push_back(l[i]);
        }
    }
#endif

}

#if defined( RV_VFX_CY2023 )
void
QTAudioRenderer::availableFormats(const Device &d, FormatVector &formats)
{
    const QAudioDeviceInfo &info                 = m_deviceList[d.index];
    const QList<int> sizes                       = info.supportedSampleSizes();
    const QList<SampleFormat> types              = info.supportedSampleTypes();
    const QList<int> rates                       = info.supportedSampleRates();
    const int channelCount                       = TwkAudio::channelsCount(d.layout);  

    formats.clear();

    for (QList<int>::const_iterator ci = sizes.begin(); ci != sizes.end(); ci++)
    {
        for (QList<SampleFormat>::const_iterator cj = types.begin(); cj != types.end(); cj++)
        {
            TwkAudio::Format fmt = convertToTwkAudioFormat(*ci, *cj);

            if (fmt != TwkAudio::UnknownFormat)
            {
                QAudioFormat f;

                f.setChannelCount(channelCount);
                f.setCodec(QString(m_codec.c_str()));
                f.setByteOrder(QAudioFormat::LittleEndian);
                f.setSampleSize(*ci);
                f.setSampleType(*cj);

                // Now we check that a supported format
                // has a supported rate.
                // Oddly on linux a supported format can
                // have no rate thats supported!
                for (size_t i = 0; i < rates.size(); i++)
                {
                    f.setSampleRate(rates[i]);
                    if (info.isFormatSupported(f))
                    {
                        formats.push_back(fmt);
                        break;
                    }
                }
            }
        }
    }
}
#else
void
QTAudioRenderer::availableFormats(const Device &d, FormatVector &formats)
{
    const QAudioDevice &device      = m_deviceList[d.index];
    const QList<SampleFormat> types = device.supportedSampleFormats();
    const int channelCount          = TwkAudio::channelsCount(d.layout);  

    formats.clear();

    for (SampleFormat type : types)
    {
        TwkAudio::Format fmt = convertToTwkAudioFormat(type);
        if (fmt != TwkAudio::UnknownFormat)
        {
            QAudioFormat f;
            f.setChannelCount(channelCount);
            f.setSampleFormat(type);

            // NOTE_QT:
            // Cannot set codec - Look at QMediaFormat maybe
            // Cannot set byte order - Qt will always expect and use samples in the endianness of the host platform.
            // Cannot set custom sample size - more rigid with popular size (unit8, int16, int32, float)
            // Size and type replaced by SampleFormat.
            f.setSampleFormat(type);

            // Checks for supported sample rates within the device's range. Checks every 100Hz.
            // TODO_QT: This can be optimized or improved for faster detection.
            for (int rate = device.minimumSampleRate(); rate <= device.maximumSampleRate(); rate +=100)
            {
                f.setSampleRate(rate);
                if (device.isFormatSupported(f))
                {
                    formats.push_back(fmt);
                    break;
                }
            }
        }
    }
}
#endif

//
// Sets the sample size and type in qformat for a given twkFormat.
//
// TODO_QT: The name is not optimal for Qt6, but could be fine. 
void
QTAudioRenderer::setSampleSizeAndType(Layout twkLayout,
                                      Format twkFormat,
                                      QAudioFormat &qformat) const
{
    qformat.setChannelCount(TwkAudio::channelsCount(twkLayout));

    switch (twkFormat)
    {
        case TwkAudio::Float32Format:
#if defined( RV_VFX_CY2023 )
            qformat.setSampleSize(32);
            qformat.setSampleType(QAudioFormat::Float);
#else
            qformat.setSampleFormat(QAudioFormat::Float);
#endif
          break;

        case TwkAudio::Int32Format:
#if defined( RV_VFX_CY2023 )
            qformat.setSampleSize(32);
            qformat.setSampleType(QAudioFormat::SignedInt);
#else
            qformat.setSampleFormat(QAudioFormat::Int32);
#endif
            break;

        case TwkAudio::Int24Format:
#if defined( RV_VFX_CY2023 )
            qformat.setSampleSize(24);
            qformat.setSampleType(QAudioFormat::SignedInt);
#else
            // Qt6 does not have a direct equivalent for 24-bit audio.
            // TODO_QT: Might have to handle this specially or not support it.
            qformat.setSampleFormat(QAudioFormat::Int32);
#endif
            break;

        case TwkAudio::Int16Format:
#if defined( RV_VFX_CY2023 )
            qformat.setSampleSize(16);
            qformat.setSampleType(QAudioFormat::SignedInt);
#else
            qformat.setSampleFormat(QAudioFormat::Int16);
#endif
            break;

        case TwkAudio::Int8Format:
#if defined( RV_VFX_CY2023 )
            qformat.setSampleSize(8);
            qformat.setSampleType(QAudioFormat::SignedInt);
#else
            // Qt6 does not have a direct equivalent for signed 8-bit audio.
            // TODO_QT: Might have to handle this specially or not support it.
            qformat.setSampleFormat(QAudioFormat::UInt8);
#endif
            break;

        default:
            cout << "AUDIO: format unknown" << endl;
            return;
    }
}

void
QTAudioRenderer::availableRates(const Device &d, Format format, RateVector &audiorates)
{
    QAudioFormat f;

    f.setChannelCount(TwkAudio::channelsCount(d.layout));
#if defined( RV_VFX_CY2023 )
    f.setCodec(QString(m_codec.c_str()));
    f.setByteOrder(QAudioFormat::LittleEndian);
#endif

    setSampleSizeAndType(d.layout, format, f);


#if defined( RV_VFX_CY2023 )
    const QAudioDeviceInfo &device  = m_deviceList[d.index];
    QList<int> rates                = device.supportedSampleRates();
    sort(rates.begin(), rates.end());
#else
    const QAudioDevice &device      = m_deviceList[d.index];
#endif

    audiorates.clear();

#if defined( RV_VFX_CY2023 )
    for (size_t i = 0; i < rates.size(); i++)
    {
        f.setSampleRate(rates[i]);
        if (device.isFormatSupported(f)) audiorates.push_back(rates[i]);
    }
#else
    // Qt6: doesn't provide a list of supported sample rates.
    // Qt6: Instead, we'll check a range of commong sample rates.
    // TODO_QT: Better way?
    std::vector<int> commonRates = { 8000, 11025, 1600, 22050, 3200, 44100, 4800, 88200, 96000, 192000 };
    for (int rate : commonRates)
    {
        if (rate >= device.minimumSampleRate() && rate <= device.maximumSampleRate())
        {
            f.setSampleRate(rate);
            if (device.isFormatSupported(f)) audiorates.push_back(rate);
        }
    }
#endif
}

//
// Check that the device supports the number
// of channels as specified by m_parameters.layout.
//
bool
#if defined( RV_VFX_CY2023 )
    QTAudioRenderer::supportsRequiredChannels(const QAudioDeviceInfo &device) const
#else
    QTAudioRenderer::supportsRequiredChannels(const QAudioDevice &device) const
#endif
{
#ifdef PLATFORM_LINUX
    if (AudioRenderer::debug)
    {
#if defined( RV_VFX_CY2023 )
        const QList<int> channels = device.supportedChannelCounts();
        for (QList<int>::const_iterator ci = channels.begin(); ci != channels.end(); ci++)
            TwkUtil::Log("AUDIO") << device.deviceName().toStdString() << " supports channel count = " << (*ci);
#else
        TwkUtil::Log("AUDIO") << device.description().toStdString()
                              << " supports minimum " << device.minimumChannelCount()
                              << " to maximum " << device.maximumChannelCount() << " channels";
#endif
    }

    // Its possible e.g. Linux that some devices hv no channel count info
    // so we assume its supported... hence we always return true.
    return true;
#else
#if defined( RV_VFX_CY2023 )
    const QList<int> channels = device.supportedChannelCounts();
#endif

    if (AudioRenderer::debug)
    {
#if defined( RV_VFX_CY2023 )
        for (QList<int>::const_iterator ci = channels.begin(); ci != channels.end(); ci++)
            TwkUtil::Log("Audio") << device.deviceName().toStdString() << " supports channel count = " << (*ci);
#else
        TwkUtil::Log("AUDIO") << device.description().toStdString()
                              << " supports minimum " << device.minimumChannelCount()
                              << " to maximum " << device.maximumChannelCount() << " channels";
#endif
    }

    //
    //  Now we can support driving whatever channels the device advertizes
    //  even if we have to fake it by upscaling the limited channels available
    //  in the input sources
    //

#if defined( RV_VFX_CY2023 )
    return (!channels.empty());
#else
    return (device.maximumChannelCount() > 0);
#endif

#endif
}

void
QTAudioRenderer::initDeviceList()
{
    if (m_deviceList.empty())
    { 
        // TODO_QT: This could be removed since it is for CentOS 6.5 bug and RV does not support it anymore.
        //          Should we check on Rocky linux if that is still the case?
        //          The whole point of the function seems to workaround that specific bug.

        //
        //  This appears to be a bug with Linux (on centos6.5).
        //  Basically the size returned by availbleDevices() is different
        //  the first time you call it compared to all subsequent times.
        //  So this logic between keeps calling availableDevices() until
        //  its size doesnt change; we limit this to five attempts.
        //
        int noOfAttempts = 5;
        int noOfDevices = 0;
        int prev_noOfDevices = 0;

        do
        {
            prev_noOfDevices = noOfDevices;
#if defined( RV_VFX_CY2023 )
            noOfDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).size();
#else
            noOfDevices = QMediaDevices::audioOutputs().size();
#endif
            --noOfAttempts;
        } while (prev_noOfDevices != noOfDevices && noOfAttempts);

#if defined( RV_VFX_CY2023 )
        m_deviceList << QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
#else
        m_deviceList << QMediaDevices::audioOutputs();
#endif
    }
}

//  The purpose of init() is to
//      1. Populate the AudioRenderer m_outputDevices();
//      2. Initialise the current Device, m_device based on current
//         preference device choice while finding the closest sample rate
//         allowed for that device.
//      3. Initialise the current QAudioFormat, m_format, based of current
//         preference sample size and sample rate.
//      4. Create QAudioThread.
//
void
QTAudioRenderer::init()
{
    const int channelCount  = TwkAudio::channelsCount(m_parameters.layout);  

    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "init()";

    //
    //  Init the device list
    //
    initDeviceList();

    if (m_parameters.device == "Default" || m_parameters.device.empty())
    {
#if defined( RV_VFX_CY2023 )
        const QAudioDeviceInfo &defaultDevice = QAudioDeviceInfo::defaultOutputDevice();
#else
        const QAudioDevice &defaultDevice = QMediaDevices::defaultAudioOutput();
#endif
        bool validDevice = false;

        for (size_t i = 0; i < m_deviceList.size(); ++i)
        {
#if defined( RV_VFX_CY2023 )
            if (m_deviceList[i].deviceName() == defaultDevice.deviceName())
#else
            if (m_deviceList[i].description() == defaultDevice.description())
#endif
            {
                validDevice = true;
                break;
            }
        }

        // Check that the defaultOutputDevice does support the required
        // number of channels.
        if (validDevice && supportsRequiredChannels(defaultDevice))
        {
#if defined( RV_VFX_CY2023 )
            m_parameters.device = defaultDevice.deviceName().toStdString();
#else
            m_parameters.device = defaultDevice.description().toStdString();
#endif
            if (AudioRenderer::debug)
            {
                TwkUtil::Log("AUDIO") << "Using default device=" << m_parameters.device;
            }

            QAudioFormat qformat;

            qformat.setSampleRate((int) m_parameters.rate);
            qformat.setChannelCount(channelCount);
#if defined( RV_VFX_CY2023 )
            qformat.setCodec(QString(m_codec.c_str()));
            qformat.setByteOrder(QAudioFormat::LittleEndian);
            //qformat.setByteOrder(QAudioFormat::BigEndian);
#endif

#if defined( RV_VFX_CY2023 )
            setSampleSizeAndType(m_parameters.layout, m_parameters.format, qformat);
#else
            setSampleSizeAndType(m_parameters.layout, m_parameters.format, qformat);
#endif
            //
            // If the default device does not support the format
            // defined by m_parameters; we change to the
            // preferredFormat() of the default device.
            if (!defaultDevice.isFormatSupported(qformat))
            {
                const QAudioFormat defaultFormat = defaultDevice.preferredFormat();
#if defined( RV_VFX_CY2023 )
                m_parameters.format = convertToTwkAudioFormat(sampleSize(defaultFormat),
                                                              sampleType(defaultFormat));
#else
                m_parameters.format = convertToTwkAudioFormat(sampleType(defaultFormat));
#endif
            }
        }
    }

    m_outputDevices.clear();
    for (size_t i = 0; i < m_deviceList.size(); ++i)
    {
#if defined( RV_VFX_CY2023 )
        const QAudioDeviceInfo &device = m_deviceList[i];
#else
        const QAudioDevice &device = m_deviceList[i];
#endif       

        if (device.isNull() || !supportsRequiredChannels(device)) continue;

        const QAudioFormat defaultFormat = device.preferredFormat();

#if defined( RV_VFX_CY2023 )
        std::string deviceName = device.deviceName().toStdString();
#else
        std::string deviceName = device.description().toStdString();
#endif

        // TODO_QT NOTE_QT: I would assume that below is not true anymore or fixed because QtMultimedia had a big
        //                  refactor in Qt6.

        // On Windows, a QAudioDevice can be listed twice with the same name
        // This is due to the fact that both Qt audio plugins, wasapi and WinMM, 
        // can return the exact same name for an audiio output device
        // https://bugreports.qt.io/browse/QTBUG-75781
        // Both will have different capabilities and RV references the output
        // audio device by name, so it is important that a unique name be used
        // to reference those distinct devices of the same name.
        while (findDeviceByName(deviceName)!=-1) 
        { 
#if defined( RV_VFX_CY2023 )
            deviceName+="_"+device.realm().toStdString(); 
#else
            deviceName+="_"+device.id().toStdString(); 
#endif
        } 

        Device d(deviceName);

        d.layout          = m_parameters.layout;
        d.defaultRate     = defaultFormat.sampleRate();
        d.latencyLow      = 0;
        d.latencyHigh     = 0;
        d.index           = i;


        if (m_parameters.device == "Default" || m_parameters.device.empty())
        {
            // If we got here it implies QAudioDeviceInfo::defaultOutputDevice()
            // returns a device with too few channels so we cannot use it
            // as our default device. Instead we pick the first device that
            // supports the channel count we need.
            m_parameters.device = d.name;
#if defined( RV_VFX_CY2023 )
            m_parameters.format = convertToTwkAudioFormat(sampleSize(defaultFormat),
                                                          sampleType(defaultFormat));
#else
            m_parameters.format = convertToTwkAudioFormat(sampleType(defaultFormat));
#endif
        }

        if (m_parameters.device == d.name)
        {
            d.isDefaultDevice = true;
            m_device = device;
        }
        else
        {
            d.isDefaultDevice = false;
        }

        if (AudioRenderer::debug)
        {
            cout << "AUDIO: device=" << d.name << " with channelsCount = " << TwkAudio::channelsCount(d.layout)
#if defined( RV_VFX_CY2023 )
                << " defaultSampleSize=" << (int) sampleSize(defaultFormat)
                << " defaultSampleType=" << (int) sampleType(defaultFormat)
                << " defaultSampleByteOrder=" << (int) defaultFormat.byteOrder()
#else
                << " defaultSampleFormat=" << (int) sampleType(defaultFormat)
                // Qt6: No custom sample size.
                // Qt6: No custom byte orde. Qt expect the order of the host.
#endif
                << " defaultrate=" << (int) d.defaultRate
                << " isDefaultDevice=" << (int) d.isDefaultDevice << endl;
        }

        m_outputDevices.push_back(d);
    }

    if (AudioRenderer::debug)
    {
#if defined( RV_VFX_CY2023 )
        cout << "AUDIO: init default device=" << m_device.deviceName().toStdString() << endl;
#else
        cout << "AUDIO: init default device=" << m_device.description().toStdString() << endl;    
#endif

        cout << "AUDIO: init m_parameters.device=" << m_parameters.device << endl;
        cout << "AUDIO: init m_parameters.format=" << (int) m_parameters.format
             << " (" << TwkAudio::formatString(m_parameters.format) << ")" << endl;
        cout << "AUDIO: init m_parameters.rate=" << (int) m_parameters.rate << endl;
        cout << "AUDIO: init m_parameters.layout=" << (int) m_parameters.layout
             << " (" << TwkAudio::channelsCount(m_parameters.layout) << " channels)" << endl;
        cout << "AUDIO: init m_parameters.framesPerBuffer=" << (int) m_parameters.framesPerBuffer << endl;
        cout << "AUDIO: init m_parameters.latency=" << m_parameters.latency << endl;
        cout << "AUDIO: init m_parameters.preRoll=" << (m_parameters.preRoll ? 1 : 0) << endl;
        cout << "AUDIO: init m_parameters.hardwareLock=" << (m_parameters.hardwareLock ? 1 : 0) << endl;
    }

    if (!m_outputDevices.empty())
    {
        size_t defaultDeviceIndex = 0;

        for (size_t i = 0; i < m_outputDevices.size(); ++i)
        {
            if (m_outputDevices[i].isDefaultDevice)
            {
                defaultDeviceIndex = i;
                break;
            }
        }

        const Device &defaultDevice = m_outputDevices[defaultDeviceIndex];

        if (!defaultDevice.isDefaultDevice)
        {
            if (m_parameters.device == "Default")
            {
                cout << "ERROR: audio default device is NOT available." << endl;
            }
            else
            {
                // The device that was picked and registered in the preferences
                // might no longer be available because its unplugged.
                cout << "WARNING: audio device '" << m_parameters.device <<
                        "' is unavailable: Using default." << endl;
                m_parameters.device = "Default";
                m_parameters.layout = TwkAudio::Stereo_2;
                init();
                return;
            }
        }

        DeviceState state;

        state.framesPerBuffer = m_parameters.framesPerBuffer;
        state.format = m_parameters.format;
        state.layout = defaultDevice.layout;
        state.device = defaultDevice.name;
        state.rate = defaultDevice.defaultRate;
        state.latency = m_parameters.latency;

        // Find the best matching sample rate for the
        // select device.
        if (m_parameters.rate != 0.0)
        {
            state.rate = m_parameters.rate;
        }
        else
        {
            m_parameters.rate = state.rate;
        }

        RateVector rates;

        availableRates(defaultDevice, state.format, rates);

        if (!rates.empty())
        {
            double nearRate = rates[0];

            for (size_t i = 1; i < rates.size(); i++)
            {
                if (fabs(rates[i] - state.rate) < fabs(nearRate - state.rate))
                {
                    nearRate = rates[i];
                }
            }

            state.rate = nearRate;
            m_parameters.rate = state.rate;
        }

        //
        //  Init the QAudioFormat to use in m_format
        //
        m_format.setSampleRate((int) state.rate);
        m_format.setChannelCount(TwkAudio::channelsCount(state.layout));
#if defined( RV_VFX_CY2023 )
        m_format.setCodec(QString(m_codec.c_str()));
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        //m_format.setByteOrder(QAudioFormat::BigEndian);
#endif
        // TODO_QT: Not sure about codec.
        // Qt6: Expect the endian of the host.
        setSampleSizeAndType(state.layout, state.format, m_format);

        if (!m_device.isFormatSupported(m_format))
        {
#if defined( RV_VFX_CY2023 )
            m_format = m_device.nearestFormat(m_format);
#else
            // NOTE_QT: Qt6: does not have the nearestFormat avaiable. Take the default prefered.
            m_format = m_device.preferredFormat();
#endif
            
            if (sampleType(m_format) == QAudioFormat::Unknown)
            {
                cout << "AUDIO: Default format not supported - trying to use nearest" << endl;
                setErrorCondition("Default format not supported - trying to use nearest");
            }
            else
            {
                if (AudioRenderer::debug)
                {
#if defined( RV_VFX_CY2023 )
                    cout << "AUDIO: nearest m_format.sampleType()=" << (int) sampleType(m_format) << endl;
                    cout << "AUDIO: nearest m_format.sampleSize()=" << (int) sampleSize(m_format) << endl;
#else
                    cout << "AUDIO: nearest m_format.sampleFormat()=" << (int) sampleType(m_format) << endl;
#endif
                    cout << "AUDIO: nearest m_format.sampleRate()=" << (int) m_format.sampleRate() << endl;
                    cout << "AUDIO: nearest m_format.channelCount()=" << (int) m_format.channelCount() << endl;
                }
            }

            state.rate = m_parameters.rate = m_format.sampleRate();
            state.layout = m_parameters.layout = TwkAudio::channelLayouts(m_format.channelCount()).front();
            state.format = m_parameters.format = getTwkAudioFormat();
        }

#ifdef DEBUG_QTAUDIO
        if (AudioRenderer::debug)
        {
#if defined( RV_VFX_CY2023 )
                    cout << "AUDIO: nearest m_format.sampleType()=" << (int) sampleType(m_format) << endl;
                    cout << "AUDIO: nearest m_format.sampleSize()=" << (int) sampleSize(m_format) << endl;
#else
                    cout << "AUDIO: nearest m_format.sampleFormat()=" << (int) sampleType(m_format) << endl;
#endif
            cout << "AUDIO: m_format.sampleRate()=" << (int) m_format.sampleRate() << endl;
            cout << "AUDIO: m_format.channelCount()=" << (int) m_format.channelCount() << endl;
        }
#endif

        setDeviceState(state);

        if (AudioRenderer::debug)
        {
            cout << "AUDIO: init: state.format =" << (int) state.format
                 << " (" << TwkAudio::formatString(state.format) << ")" << endl;
            cout << "AUDIO: init: state.rate =" << state.rate << endl;
            cout << "AUDIO: init: state.layout =" << (int) state.layout 
                 << " (" << TwkAudio::channelsCount(state.layout) << " channels)" << endl;
            cout << "AUDIO: init: state.framesPerBuffer =" << state.framesPerBuffer << endl;
        }

        // Create the audio renderer thread.
        // The renderer runs in this thread.
        //
        m_thread = new QTAudioThread(*this, m_parent);
        if (!m_thread)
        {
            setErrorCondition("Unable to create QThread for Platform Audio.");
        }
        else
        {
            m_thread->startMe();
        }
    }
    else
    {
        setErrorCondition("No devices available for Platform Audio.");
    }
}

void QTAudioRenderer::play()
{

    if (m_thread)
    {
        m_thread->emitStartAudio();
    }

    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "play";
    AudioRenderer::play();
}

void QTAudioRenderer::play(IPCore::Session* s)
{
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "play session";

    s->audioVarLock();
    s->setAudioTimeShift(numeric_limits<double>::max());
    s->setAudioStartSample(0);
    s->setAudioFirstPass(true);
    s->audioVarUnLock();

    AudioRenderer::play(s);

    if (m_thread) m_thread->emitPlay(s);
}


void QTAudioRenderer::stop()
{
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "stop";

    if (m_thread) m_thread->emitSuspendAndResetAudio();

    AudioRenderer::stop();
}

void QTAudioRenderer::stop(IPCore::Session* s)
{
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "stop session";


    AudioRenderer::stop(s);
    s->audioVarLock();
    s->setAudioTimeShift(numeric_limits<double>::max());
    s->audioVarUnLock();
}

void
QTAudioRenderer::shutdown()
{
    if (AudioRenderer::debug) TwkUtil::Log("AUDIO") << "shutdown";


    if (m_thread)
    {
        if (!m_parameters.holdOpen)
        {
            // NB: You only get here if you have the "Hold
            // audio device open" preference unchecked.
            // 
            m_thread->emitStopAudio();
        }
    }
}


} // IPCore
