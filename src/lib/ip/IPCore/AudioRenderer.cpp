//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/Exception.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/Session.h>
#include <IPCore/Application.h>

#ifdef PLATFORM_WINDOWS
#include <VersionHelpers.h>
#endif

#include <IPCore/PerFrameAudioRenderer.h>
#include <TwkUtil/File.h>
#include <TwkUtil/Log.h>
#include <iostream>
#include <set>
#include <stl_ext/string_algo.h>
#include <fstream>

#ifndef WIN32
#include <dlfcn.h>
#endif

namespace IPCore
{
    using namespace std;
    using namespace TwkAudio;
    using namespace TwkUtil;

    AudioRenderer* AudioRenderer::m_renderer = 0;
    bool AudioRenderer::m_noaudio = false;
    bool AudioRenderer::m_disableAudio = false;
    size_t AudioRenderer::m_moduleIndex = 0;
    size_t AudioRenderer::m_savedIndex = 0;
    AudioRenderer::ModuleVector AudioRenderer::m_modules;
    bool AudioRenderer::debug = false;
    bool AudioRenderer::debugVerbose = false;
    bool AudioRenderer::dump = false;
    AudioRenderer::RendererParameters AudioRenderer::m_defaultParameters;
    string AudioRenderer::audioModule;
    AudioRenderer::ModuleInitVector AudioRenderer::m_moduleInitObjects;

    namespace
    {
        ofstream* audioDumpFile = 0;
    }

    //
    //  The compiled in modules
    //

    static AudioRenderer*
    createPerFrameAudio(const AudioRenderer::RendererParameters& p)
    {
        return new PerFrameAudioRenderer(p);
    }

    static AudioRenderer*
    createNothng(const AudioRenderer::RendererParameters& p)
    {
        TWK_THROW_EXC_STREAM("No audio module available");
    }

    void AudioRenderer::addModuleInitFunc(ModuleInitializationFunc F,
                                          std::string name)
    {
        ModuleInitObject initObj;

        initObj.name = name;
        initObj.func = F;

        m_moduleInitObjects.push_back(initObj);
    }

    namespace
    {

        void addModuleIfAllowed(AudioRenderer::ModuleVector& modules,
                                set<string>& badMods, string name,
                                string plugName,
                                AudioRenderer::ModuleCreateFunc F)
        {
            if (!badMods.count(name))
            {
                modules.push_back(AudioRenderer::Module(name, plugName, F));
            }
        }

#ifdef PLATFORM_WINDOWS
        //------------------------------------------------------------------------------
        // Note: GetVersion() is deprecated and IsWindows10OrGreater() always
        // return the windows version corresponding to the manifest of the app.
        // This is not a bug but rather a Microsoft design to defeat version
        // check. Here is a back door to truly retrieve the running windows
        // version.
        //
        void GetTrueWindowsVersion(OSVERSIONINFOEX* pOSversion)
        {
            // Function pointer to driver function
            NTSTATUS(WINAPI * pRtlGetVersion)(
                PRTL_OSVERSIONINFOW lpVersionInformation) = NULL;

            // load the System-DLL
            HINSTANCE hNTdllDll = LoadLibrary("ntdll.dll");

            // successfully loaded?
            if (hNTdllDll != NULL)
            {
                // get the function pointer to RtlGetVersion
                pRtlGetVersion =
                    (NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(
                        hNTdllDll, "RtlGetVersion");

                // if successfull then read the function
                if (pRtlGetVersion != NULL)
                {
                    pRtlGetVersion((PRTL_OSVERSIONINFOW)pOSversion);
                }

                // free the library
                FreeLibrary(hNTdllDll);
            } // if (hNTdllDll != NULL)

            // if function failed, use fallback to old version
            if (pRtlGetVersion == NULL)
            {
                GetVersionEx((OSVERSIONINFO*)pOSversion);
            }
        }

        //------------------------------------------------------------------------------
        //
        bool IsReallyWindows10OrGreater()
        {
            OSVERSIONINFOEX osvi;
            ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

            GetTrueWindowsVersion(&osvi);

            return osvi.dwMajorVersion >= 10;
        }
#endif // #ifdef PLATFORM_WINDOWS

    }; // namespace

    void AudioRenderer::setDebug(bool b) { AudioRenderer::debug = b; }

    void AudioRenderer::setDebugVerbose(bool b)
    {
        AudioRenderer::debugVerbose = b;
    }

    void AudioRenderer::setDumpAudio(bool b) { AudioRenderer::dump = b; }

    void AudioRenderer::initialize()
    {
        //
        //  If disabled from commandline, don't even initialize
        //
        if (m_disableAudio)
            return;

        set<string> badMods;

        const char* badModuleVar = getenv("TWK_AUDIO_MODULE_NO_INIT");
        if (badModuleVar)
        {
            vector<string> badModsVec;
            stl_ext::tokenize(badModsVec, badModuleVar, ",");
            badMods = set<string>(badModsVec.begin(), badModsVec.end());
        }

#ifdef PLATFORM_DARWIN

#ifdef HAVE_COREAUDIO
        addModuleIfAllowed(m_modules, badMods, "Core Audio (HAL)",
                           "libCoreAudioHALModule", NULL);
#endif
#endif

#ifdef PLATFORM_LINUX
        addModuleIfAllowed(m_modules, badMods, "ALSA (Pre-1.0.14)",
                           "libALSAAudioModule", NULL);
        addModuleIfAllowed(m_modules, badMods, "ALSA (Safe)",
                           "libALSASafeAudioModule", NULL);
#endif

        for (size_t i = 0; i < m_moduleInitObjects.size(); i++)
        {
            ModuleInitObject o = m_moduleInitObjects[i];

            if (!badMods.count(o.name))
            {
                if (o.name == "Platform Audio")
                {
                    // Make Platform Audio the default audio module; i.e. first
                    // entry.
                    m_modules.insert(m_modules.begin(), o.func());
                }
                else
                {
                    m_modules.push_back(o.func());
                }
            }
        }

        if (const char* modules = getenv("RV_AUDIO_MODULES"))
        {
            //
            //  If the variables RV_AUDIO_MODULES is set and it has the
            //  form:
            //
            //      Name0=moduleObject0:Name1=moduleObject1:Name2=moduleObject3
            //
            //  then load them into the module list name and so. NOTE: the
            //  moduleObject in the above cases should omit the extension
            //  (.so, .dylib, .dll).
            //

            vector<string> buffer;
            stl_ext::tokenize(buffer, modules, ":");

            for (size_t i = 0; i < buffer.size(); i++)
            {
                vector<string> parts;
                stl_ext::tokenize(parts, buffer[i], "=");

                if (parts.size() == 2)
                {
                    addModuleIfAllowed(m_modules, badMods, parts[0], parts[1],
                                       NULL);
                }
            }
        }

        m_moduleIndex = 0;

        //
        //  internal modules *must* be last in the list. These will not
        //  appear to user.
        //

        if (!badMods.count("Per-Frame"))
        {
            m_modules.push_back(
                Module("Per-Frame", "", createPerFrameAudio, true));
        }

        //
        //  Find the "default" module if there is one
        //

        for (size_t i = 0; i < m_modules.size(); i++)
        {
            if (m_modules[i].name == audioModule)
                m_moduleIndex = i;
        }
    }

    void AudioRenderer::outputParameters(const RendererParameters& p)
    {
        cout << "DEBUG: -parameters-" << endl
             << "DEBUG:    holdOpen = " << p.holdOpen << endl
             << "DEBUG:    hardwareLock = " << p.hardwareLock << endl
             << "DEBUG:    preRoll = " << p.preRoll << endl
             << "DEBUG:    format = " << p.format << endl
             << "DEBUG:    rate = " << p.rate << endl
             << "DEBUG:    layout = " << (int)p.layout << endl
             << "DEBUG:    latency = " << p.latency << endl
             << "DEBUG:    framesPerBuffer = " << p.framesPerBuffer << endl
             << "DEBUG:    device = " << p.device << endl;
    }

    void AudioRenderer::setDefaultParameters(const RendererParameters& p)
    {
        m_defaultParameters = p;
    }

    static bool operator==(const AudioRenderer::Module& m, const string& s)
    {
        return m.name == s;
    }

    void AudioRenderer::loadModule(Module& module)
    {
#ifndef PLATFORM_WINDOWS
        string soname = module.so;
#ifdef PLATFORM_LINUX
        soname += ".so";
#endif

#ifdef PLATFORM_DARWIN
        soname += ".dylib";
#endif

        if (const char* audiopath = getenv("LD_LIBRARY_PATH"))
        {
            vector<string> files = findInPath(soname, audiopath);

            if (files.size())
            {
                string file = files.front();

                cout << "INFO: loading audio module " << file << endl;

                if (void* handle = dlopen(file.c_str(), RTLD_LAZY))
                {
                    ModuleCreateFunc F =
                        (ModuleCreateFunc)dlsym(handle, "CreateAudioModule");

                    if (!F)
                    {
                        dlclose(handle);

                        cerr << "ERROR: ignoring audio module " << file
                             << ": missing CreateAudioModule function: "
                             << dlerror() << endl;
                    }

                    module.func = F;
                }
                else
                {
                    cerr << "ERROR: opening audio module " << file << ": "
                         << dlerror() << endl;
                }
            }
        }
#endif //  PLATFORM_WINDOWS

        if (!module.func)
            module.func = createNothng;
    }

    void AudioRenderer::reset(const RendererParameters& params)
    {
        if (m_disableAudio)
        {
            m_noaudio = true;
            if (m_renderer)
                delete m_renderer;
            m_renderer = 0;
            return;
        }

        // Do not stop the player when we set the audio renderer
        // first time. We want to avoid to stop the player when
        // we receiceve a media with sound in progressive source
        // loading mode.
        if (m_renderer)
            App()->stopAll();

        if (m_renderer)
        {
            m_renderer->stop();
            m_renderer->shutdown();
            delete m_renderer;
            m_renderer = 0;
        }

        try
        {
            Module& module = m_modules[m_moduleIndex];
            if (!module.func)
                loadModule(module);

            ModuleCreateFunc F = m_modules[m_moduleIndex].func;
            m_renderer = F(params);
            m_noaudio = false;
        }
        catch (exception& exc)
        {
            cerr << "WARNING: continuing without audio" << endl;
            m_renderer = 0;
            m_noaudio = true;
        }

        App()->resumeAll();

        if (!m_renderer)
        {
            TWK_THROW_EXC_STREAM("No audio module available");
        }
    }

    void AudioRenderer::reset()
    {
        try
        {
            reset(m_defaultParameters);
        }
        catch (exception& exc)
        {
            m_disableAudio = true;
            cerr << "EXCEPTION: " << exc.what() << endl;
        }
    }

    void AudioRenderer::setModule(const std::string& m)
    {
        ModuleVector::iterator i = find(m_modules.begin(), m_modules.end(), m);

        if (i != m_modules.end())
        {
            size_t index = i - m_modules.begin();

            if (m_moduleIndex != index)
            {
                m_moduleIndex = index;
                reset();
            }
        }
    }

    void AudioRenderer::pushPerFrameModule(const RendererParameters& p)
    {
        ModuleVector::iterator i =
            find(m_modules.begin(), m_modules.end(), "Per-Frame");

        if (i != m_modules.end())
        {
            size_t index = i - m_modules.begin();

            if (m_moduleIndex != index)
            {
                m_savedIndex = m_moduleIndex;
                m_moduleIndex = index;
                reset(p);
            }
        }
    }

    void AudioRenderer::popPerFrameModule()
    {
        m_moduleIndex = m_savedIndex;
        reset();
    }

    const std::string& AudioRenderer::currentModule()
    {
        return m_modules[m_moduleIndex].name;
    }

    void AudioRenderer::transformFloat32ToInt24(
        TwkAudio::AudioBuffer::BufferPointer inData, char* outData,
        size_t dataCount, bool isLittleEndian)
    {
        char* out24 = (char*)outData;
        const double dscale24 = (double)0x7FFFFF; // 8388607;
        int x;
        char* px = (char*)&x;

        for (size_t i = 0; i < dataCount; ++i)
        {
            float v = (*inData++);
            if (v > 1.0f)
            {
                x = 8388607;
            }
            else if (v < -1.0f)
            {
                x = -8388608;
            }
            else
            {
                x = int(double(v) * dscale24);
            }

            if (isLittleEndian)
            {
                (*out24++) = px[0];
                (*out24++) = px[1];
                (*out24++) = px[2];
            }
            else
            {
                (*out24++) = px[2];
                (*out24++) = px[1];
                (*out24++) = px[0];
            }
        }
    }

    AudioRenderer::AudioRenderer(const RendererParameters& params)
        : m_playing(false)
        , m_error(false)
        , m_parameters(params)
        , m_preRollDelay(0)
    {
        pthread_mutex_init(&m_sessionsLock, 0);
        pthread_mutex_init(&m_errorLock, 0);
        pthread_mutex_init(&m_playLock, 0);
    }

    AudioRenderer::~AudioRenderer()
    {
        pthread_mutex_destroy(&m_sessionsLock);
        pthread_mutex_destroy(&m_errorLock);
        pthread_mutex_destroy(&m_playLock);
    }

    void AudioRenderer::cleanup()
    {
        delete m_renderer;
        m_renderer = NULL;
    }

    void AudioRenderer::setDeviceState(const DeviceState& state)
    {
        m_deviceState = state;
    }

    int AudioRenderer::findDeviceByName(const std::string& name) const
    {
        for (size_t i = 0; i < m_outputDevices.size(); i++)
        {
            if (m_outputDevices[i].name == name)
                return i;
        }

        if (name == "")
            return 0;
        return -1;
    }

    int AudioRenderer::findDefaultDevice() const
    {
        for (size_t i = 0; i < m_outputDevices.size(); i++)
        {
            if (m_outputDevices[i].isDefaultDevice)
                return i;
        }

        return 0;
    }

    void AudioRenderer::lockPlaying() const { pthread_mutex_lock(&m_playLock); }

    void AudioRenderer::unlockPlaying() const
    {
        pthread_mutex_unlock(&m_playLock);
    }

    void AudioRenderer::availableLayouts(const Device&, LayoutsVector&) {}

    void AudioRenderer::availableFormats(const Device&, FormatVector&) {}

    void AudioRenderer::availableRates(const Device&, Format, RateVector&) {}

    void AudioRenderer::play()
    {
        lockPlaying();
        m_playing = true;
        m_lastSampleTime = 0.0;
        unlockPlaying();
    }

    void AudioRenderer::stop()
    {
        lockPlaying();
        m_playing = false;
        unlockPlaying();
    }

    void AudioRenderer::shutdown() {}

    bool AudioRenderer::isPlaying() const
    {
        lockPlaying();
        bool b = m_playing;
        unlockPlaying();
        return b;
    }

    void AudioRenderer::lockSessions() { pthread_mutex_lock(&m_sessionsLock); }

    void AudioRenderer::unlockSessions()
    {
        pthread_mutex_unlock(&m_sessionsLock);
    }

    void AudioRenderer::getSessions(vector<Session*>& temp)
    {
        lockSessions();
        temp.resize(m_sessions.size());
        copy(m_sessions.begin(), m_sessions.end(), temp.begin());
        unlockSessions();
    }

    bool AudioRenderer::addSession(Session* s)
    {
        lockSessions();
        bool first = m_sessions.empty();
        if (s)
            m_sessions.insert(s);
        m_lastSampleTime = 0.0;
        unlockSessions();

        return first;
    }

    bool AudioRenderer::deleteSession(Session* s)
    {
        lockSessions();
        if (s)
            m_sessions.erase(s);
        bool empty = m_sessions.empty();
        unlockSessions();

        return empty;
    }

    void AudioRenderer::play(Session* s)
    {
        if (addSession(s))
            play();
    }

    void AudioRenderer::stop(Session* s)
    {
        if (deleteSession(s))
        {
            stop();

            if (!m_parameters.holdOpen)
                shutdown();
        }
    }

    //
    //  For the case when we have preRoll enabled,
    //  we do not want to stop the audio renderer's
    //  device in terms of closing or suspending it
    //  during turn-around as this will introduce
    //  a delay in playback as the frames turn-around.
    //  So we just delete the session and reset audio
    //  time shift.
    void AudioRenderer::stopOnTurnAround(Session* s)
    {
        if (AudioRenderer::debug)
            TwkUtil::Log("AUDIO") << "stopOnTurnAround";
        if (m_parameters.preRoll)
        {
            deleteSession(s);
            if (s)
            {
                s->audioVarLock();
                s->setAudioTimeShift(numeric_limits<double>::max());
                s->audioVarUnLock();
            }

            // We are looping back with the audio device
            // already playing... so set the preRollDelay to zero.
            setPreRollDelay(0);
        }
        else
        {
            stop(s);
        }
    }

    void AudioRenderer::shutdownOnLast()
    {
        lockSessions();
        if (m_sessions.empty())
            shutdown();
        unlockSessions();
    }

    bool AudioRenderer::hasSessions()
    {
        lockSessions();
        bool b = m_sessions.empty();
        unlockSessions();
        return b;
    }

    AudioRenderer* AudioRenderer::renderer()
    {
        if (m_noaudio)
            return 0;
        if (!m_renderer)
            reset();
        return m_renderer;
    }

    std::string AudioRenderer::errorString() const
    {
        pthread_mutex_lock(&m_errorLock);
        string rval = m_errorString;
        pthread_mutex_unlock(&m_errorLock);
        return rval;
    }

    void AudioRenderer::clearErrorCondition()
    {
        pthread_mutex_lock(&m_errorLock);
        m_error = false;
        m_errorString = "";
        pthread_mutex_unlock(&m_errorLock);
    }

    bool AudioRenderer::isOK() const
    {
        pthread_mutex_lock(&m_errorLock);
        bool ok = !m_error;
        pthread_mutex_unlock(&m_errorLock);
        return ok;
    }

    void AudioRenderer::resetErrorCondition()
    {
        setErrorCondition(errorString());
    }

    void AudioRenderer::setErrorCondition(const std::string& msg)
    {
        pthread_mutex_lock(&m_errorLock);
        m_errorString = msg;
        m_error = true;
        pthread_mutex_unlock(&m_errorLock);

        if (AudioRenderer::debug)
            cerr << "ERROR: " << msg << endl;
        throw AudioFailedExc();
    }

    //----------------------------------------------------------------------

    void AudioRenderer::audioFillBuffer(AudioBuffer& outBuffer)
    {
        const DeviceState& state = deviceState();

        if (AudioRenderer::dump && !audioDumpFile)
        {
            audioDumpFile =
                new ofstream("audio-dump", std::ios::binary | std::ios::out);
        }

#if 0
    //
    //  Create a sine wave and do nothing else
    //

    {
        const Time s = outBuffer.startTime();
        float* out = outBuffer.pointer();

        for (size_t i = 0; i < outBuffer.size(); i++)
        {
            Time t = samplesToTime(i, state.rate) + s;
            out[i*2] = float(sin(t * 440.0 * 4.0) * .1);
            out[i*2+1] = float(sin(t * 440.0 * 4.0) * .1);
        }

        return;
    }
#endif

        if (m_playing)
        {
            //
            //  Loop over the sessions that are currently playing and
            //  evaluate audio for each.
            //

            lockSessions();

            for (Sessions::iterator i = m_sessions.begin();
                 i != m_sessions.end(); ++i)
            {
                audioFillBuffer(*i, outBuffer);
            }

            unlockSessions();

            //
            //  Do a quick fade in if we just started playing.
            //
            //  Note that CoreAudio seems to never reset the sampleTime
            //  hence this stuff with m_lastSampleTime.  Not sure about
            //  linux and windows.

            if (0.0 == m_lastSampleTime)
            {
                //  cerr << "resetting sampleTime" << endl;
                m_lastSampleTime = outBuffer.startTime();
            }

#if 0
        const Time fadeIn = 0.1;

        //  cerr << "AudioRenderer::audioFillBuffer startTime " << outBuffer.startTime() << endl;
        if ((outBuffer.startTime() - m_lastSampleTime) < fadeIn)
        {
            /*
            cerr << "AudioRenderer::audioFillBuffer fading " << 
                ((outBuffer.startTime() - m_lastSampleTime)/fadeIn)*
                ((outBuffer.startTime() - m_lastSampleTime)/fadeIn)
                << endl;
            */
            float*       out  = outBuffer.pointer();
            const float* end  = outBuffer.pointer() + outBuffer.sizeInFloats();
            const size_t ch   = outBuffer.channels();
            const size_t ss   = outBuffer.startSample();
            const size_t size = outBuffer.size();

            for (size_t i = 0; i < size; i++)
            {
                for (size_t q=0; q < ch; q++, out++)
                {
                    const float fact = (samplesToTime(ss + i, state.rate) - m_lastSampleTime) / fadeIn;
                    //
                    //  Exponential ramp up seems to work better
                    //  than linear.
                    //
                    *out *= fact*fact;
                }
            }
        }
#endif
        }
        else
        {
            //
            //  Output silence. Presumably we're in hold open mode.
            //

            outBuffer.zero();
        }

        if (AudioRenderer::dump)
        {
            audioDumpFile->write(
                reinterpret_cast<const char*>(outBuffer.pointer()),
                outBuffer.sizeInBytes());
        }
    }

    void AudioRenderer::audioFillBuffer(Session* s, AudioBuffer& outBuffer)
    {
        float* out = outBuffer.pointer();
        const Time startTime = outBuffer.startTime();
        const size_t startSample = outBuffer.startSample();
        const size_t numSamples = outBuffer.size();
        const DeviceState& state = deviceState();
        const int numChannels = TwkAudio::channelsCount(state.layout);

        s->audioVarLock();

        const bool backwards = s->inc() < 0;
        const double loopDuration = s->audioLoopDuration();
        const double elapsed0 = s->audioElapsedTime();
        const bool looping = loopDuration != 0.0;
        const size_t loopSamples = timeToSamples(loopDuration, state.rate);
        const size_t elapsedSamples = timeToSamples(elapsed0, state.rate);

        if (!s->audioFirstPass() && loopDuration != 0.0
            && elapsed0 > loopDuration)
        {
            if (s->audioLoopCount() <= 0)
            {
                outBuffer.zero();
                s->audioVarUnLock();
                return;
            }
            else
            {
                s->setAudioFirstPass(true);
            }
        }

        if (s->audioFirstPass())
        {
            size_t initialSample =
                timeToSamples(s->shift() / s->fps(), state.rate);
            s->setAudioInitialSample(initialSample);
            s->setAudioStartSample(initialSample);

            s->setAudioFirstPass(false);
            s->setAudioElapsedTime(0.0);

            // m_deviceState.framesPerBuffer = numSamples;

            if (AudioRenderer::debug)
            {
                TwkUtil::Log("AUDIO") << "audio frame size is " << numSamples;
            }

            //
            //  This needs to be recorded, because it seems that CoreAudio
            //  may not actually reset the sample time when you "stop" the
            //  hardware. I think it only really resets it when the
            //  hardware is completely empty. So if you start playing
            //  *before* the hardware is empty, it will just continue
            //  without reseting the mSampleTime. To get around this in a
            //  robust way we'll stash the offset as if it never
            //  resets. (This will happen if RV automatically wraps
            //  around). For back ends which don't have this problem,
            //  there's no harm done.
            //

            s->setAudioSampleShift(m_parameters.hardwareLock ? startTime : 0);

            if (!m_parameters.hardwareLock)
            {
                s->audioPlayTimer().stop();
                s->audioPlayTimer().start();
            }
        }

        //
        //  NOTE: this is a hack to get around the fact that some
        //  devices (whether or not they really have this problem)
        //  are unable to report anything useful in the timeInfo
        //  structure. So if the dac time is 0 we use a regular CPU timer
        //  to try and create the sync -- this is obviously not *really*
        //  synced, but it does seem like the only alternative.
        //

        if (!s->isScrubbingAudio())
        {
            if (m_parameters.hardwareLock)
            {
                // We have to account for preRollDelay() in the hardwareLock
                // case because the timer in elapsedPlaySecondsRaw() includes
                // the preRollDelay while the elapsed time computed from
                // (startTime - s->audioSampleShift()) does not. NB: if
                // m_parameters.preRoll is false preRollDelay() is zero.
                s->setAudioTimeShift(startTime - s->audioSampleShift()
                                     - s->elapsedPlaySecondsRaw()
                                     + preRollDelay() - state.latency);
            }
            else
            {
                s->setAudioTimeShift(s->audioPlayTimer().elapsed()
                                     - s->elapsedPlaySecondsRaw()
                                     - state.latency);
            }
        }

        s->audioVarUnLock();

        if (AudioRenderer::debugVerbose)
        {
            TwkUtil::Log("AUDIO")
                << "AudioRenderer audio time shift set to "
                << s->audioTimeShift() << " " << startTime << " "
                << s->audioSampleShift() << " " << s->elapsedPlaySecondsRaw()
                << " " << preRollDelay() << " ";
        }

        size_t n;
        size_t start = s->audioStartSample();

        if (backwards)
        {
            size_t boff =
                s->audioStartSample() - s->audioInitialSample() + numSamples;

            if (s->audioInitialSample() < boff)
            {
                start = 0;
            }
            else
            {
                start = s->audioInitialSample() - boff;
            }
        }

        try
        {
            m_abuffer.reconfigure(numSamples, state.layout, Time(state.rate),
                                  samplesToTime(start, state.rate), 0);

            m_abuffer.zero();
            IPNode::AudioContext context(m_abuffer, s->fps());
            n = s->graph().audioFillBuffer(context);
#if 0
        //
        //  Create a sine wave and do nothing else (debugging)
        //

        {
            const Time s = m_abuffer.startTime();
            float* out = m_abuffer.pointer();
            
            for (size_t i = 0; i < m_abuffer.size(); i++)
            {
                const Time t = samplesToTime(i, state.rate) + s;
                out[i*2] = sin(t * 440.0 * 4.0) * .01;
                out[i*2+1] = sin(t * 440.0 * 4.0) * .01;
            }

            n = numSamples;
        }
#endif
        }
        catch (std::exception& exc)
        {
            cout << "WARNING: audio exception: " << exc.what() << endl;
            n = 0;
        }

        if (n != numSamples)
        {
            //
            //  Zero any left over
            //
            // cerr << "Zeroing n sample=" << numSamples - n << endl;
            float* pout = m_abuffer.pointer() + n * numChannels;
            memset(pout, 0, (numSamples - n) * numChannels * sizeof(float));
        }

        if (looping)
        {
            const size_t n = (loopSamples - elapsedSamples);

            if (n < numSamples)
            {
                // cerr << "Zeroing (looping) n sample=" << numSamples - n <<
                // endl;
                float* pout = m_abuffer.pointer() + n * numChannels;
                memset(pout, 0, (numSamples - n) * numChannels * sizeof(float));
            }
        }

        s->audioVarLock();
        s->accumulateAudioStartSample(numSamples);
        s->accumulateAudioElapsedTime(double(n) / state.rate);
        if (looping)
            s->setAudioLoopCount(s->audioLoopCount() - 1);
        s->audioVarUnLock();

        if (backwards)
            m_abuffer.reverse();

        float* in0 = m_abuffer.pointer();
        float* in1 = in0 + m_abuffer.sizeInFloats();

        transform(in0, in1, out, out, plus<float>());
    }

    //----------------------------------------------------------------------
    //
    //  Utility Functions
    //

} // namespace IPCore
