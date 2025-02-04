//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__AudioRenderer__h__
#define __IPCore__AudioRenderer__h__
#include <stl_ext/thread_group.h>
#include <TwkAudio/Audio.h>
#include <TwkUtil/Timer.h>
#include <TwkMath/Time.h>
#include <limits>
#include <set>

namespace IPCore
{
    class Session;

    /// AudioRenderer is an interface to an underlying audio API

    ///
    /// AudioRenderer is both a base class for an instance of an audio
    /// renderer as well as a static interface to multiple audio APIs. An
    /// audio API is called a module. Each module instantiates one
    /// renderer. At the time the renderer is created, the audio device
    /// parameters are handed to it. Its assumed that these cannot be
    /// changed once they are set. To change parameters the renderer needs
    /// to be reconstructed using the reset() call (or by changing the
    /// module used).
    ///
    /// This class needs to handle a number of differing audio APIs so
    /// some of its behavior will be due to least-common-denominator
    /// requirements.
    ///

    class AudioRenderer
    {
    public:
        //
        //  Types
        //

        typedef TwkAudio::Format Format;
        typedef TwkAudio::Channels Channels;
        typedef TwkAudio::Layout Layout;
        typedef std::vector<size_t> AudioFrameSizeVector;

        struct RendererParameters
        {
            RendererParameters()
            //
            //  On linux the follow default settings are
            //  most appropriate for alsa and provide the best
            //  render cycle/avsync performance. In particular,
            //  holdOpen is true and hardwareLock is false.
            //
#ifdef PLATFORM_LINUX
                : holdOpen(true)
                , hardwareLock(false)
                , preRoll(true)
                ,
#else
                : holdOpen(true)
                , hardwareLock(true)
                , preRoll(false)
                ,
#endif
                format(TwkAudio::Float32Format)
                , layout(TwkAudio::Stereo_2)
                , rate(TWEAK_AUDIO_DEFAULT_SAMPLE_RATE)
                , latency(0.0)
                , framesPerBuffer(512)
                ,

                device("")
            {
            }

            bool holdOpen;
            bool hardwareLock;
            bool preRoll;
            Format format;
            Layout layout;
            double rate;
            double latency; // This represents the average device latency/av
                            // sync lag
            size_t framesPerBuffer;
            std::string device;
            AudioFrameSizeVector frameSizes;
        };

        typedef AudioRenderer* (*ModuleCreateFunc)(const RendererParameters&);

        struct Module
        {
            Module(const std::string& n, const std::string& s,
                   ModuleCreateFunc F, bool isinternal = false)
                : name(n)
                , so(s)
                , func(F)
                , internal(isinternal)
            {
            }

            std::string name;
            std::string so;
            ModuleCreateFunc func;
            bool internal;
        };

        typedef stl_ext::thread_group ThreadGroup;
        typedef TwkMath::Time Time;
        typedef TwkUtil::Timer Timer;
        typedef std::set<Session*> Sessions;
        typedef std::vector<size_t> SampleRateVector;
        typedef std::vector<Module> ModuleVector;
        typedef std::vector<Channels> ChannelsVector;
        typedef std::vector<Layout> LayoutsVector;
        typedef std::vector<Format> FormatVector;
        typedef std::vector<size_t> RateVector;
        typedef TwkAudio::AudioBuffer AudioBuffer;
        typedef unsigned int UInt32;

        typedef Module (*ModuleInitializationFunc)();

        struct ModuleInitObject
        {
            std::string name;
            ModuleInitializationFunc func;
        };

        typedef std::vector<ModuleInitObject> ModuleInitVector;

        ///
        ///  Each module should add devices to the m_outputDevices vector in its
        ///  constructor. Don't provide rates or formats that cannot be selected
        ///  by the user or which are know to be software only. E.g., don't
        ///  bother giving the user a choice for float format if the device
        ///  really doesn't handle it and its just a software only format.
        ///

        struct Device
        {
            Device(const std::string& n)
                : name(n)
                , layout(TwkAudio::UnknownLayout)
                , isDefaultDevice(false)
                , defaultRate(0)
                , latencyLow(-1)
                , latencyHigh(-1)
                , index(0)
            {
            }

            std::string name;
            Layout layout;
            double defaultRate;
            double latencyLow;
            double latencyHigh;
            bool isDefaultDevice;
            size_t index;
        };

        typedef std::vector<Device> DeviceVector;

        ///
        ///  DeviceState is the *actual* state after opening the
        ///  device. Renderes should set this once the device has been
        ///  opened.
        ///

        struct DeviceState
        {
            std::string device;
            Format format;
            Layout layout;
            double rate;
            double latency;
            size_t framesPerBuffer;
            AudioFrameSizeVector frameSizes;
        };

        ///
        ///  Set this to true if you want diagnostic output
        ///

        static bool debug;
        static bool debugVerbose;
        static void setDebug(bool b);
        static void setDebugVerbose(bool b);

        ///
        /// If true dump audio to a raw file
        ///

        static bool dump;
        static void setDumpAudio(bool b);

        ///
        ///  The preferred audio module
        ///

        static std::string audioModule;

        ///
        ///  Add a function to be called to create a new Module object
        ///  during audio initialization.
        ///

        static void addModuleInitFunc(ModuleInitializationFunc F,
                                      std::string name);

        ///
        ///  Initialize needs to be called early to set up the possible
        ///  modules (perhaps finding them on disk, etc). This is currently
        ///  called by Rv:Application from its constructor.
        ///

        static void initialize();

        ///
        ///  Audio Modules are API implementations. The user can choose an
        ///  implementation -- which will correspond to an instance of an
        ///  AudioRenderer. These will not be available until after
        ///  initialize() is called.
        ///

        static const ModuleVector& modules() { return m_modules; }

        ///
        ///  Return the name of the current module as set by setModule()
        ///  below or the default module if it hasn't been called yet.
        ///

        static const std::string& currentModule();

        static size_t currentModuleIndex() { return m_moduleIndex; }

        ///
        ///  Current and default parameters. The default parameters
        ///  indicate the current settings.
        ///

        static const RendererParameters& defaultParameters()
        {
            return m_defaultParameters;
        }

        static void setDefaultParameters(const RendererParameters&);

        ///
        ///  Reset the audio module. This will delete the current renderer
        ///  and create a new one of the same type. Any pointers to
        ///  devices, etc, will become invalid after a call to this
        ///  function. The current default rate, format, and device are
        ///  used as the renderer values when the new render is created.
        ///

        static void reset();

        ///
        ///  Reset to the given params. This used when setting to an
        ///  internal module
        ///

        static void reset(const RendererParameters&);

        ///
        ///  Select a module or reset the current one.  This function calls
        ///  reset() after changing the current module.
        ///

        static void setModule(const std::string&);

        ///
        ///  Set module to the internal pre-frame module
        ///

        static void pushPerFrameModule(const RendererParameters&);
        static void popPerFrameModule();

        ///
        ///  The current module audio renderer.
        ///

        static AudioRenderer* renderer();

        ///
        ///  No audio completely disables audio
        ///

        static void setNoAudio(bool noaudio) { m_noaudio = noaudio; }

        static void setAudioNever(bool b) { m_disableAudio = b; }

        static bool audioDisabled() { return m_noaudio; }

        static bool audioDisabledAlways() { return m_disableAudio; }

        ///
        ///  Call before exit to delete the renderer and not create a new one
        ///

        static void cleanup();

        //----------------------------------------------------------------------

        virtual ~AudioRenderer();

        ///
        ///  Devices addressable by this renderer
        ///

        const DeviceVector& outputDevices() const { return m_outputDevices; }

        ///
        ///  Return the index of the named device or -1 if no match
        ///

        int findDeviceByName(const std::string&) const;

        ///
        /// Return a good default device. Will return a good match or the
        /// first device.
        ///

        int findDefaultDevice() const;

        ///
        ///  Current parameter settings. NOTE: you can't change these. You
        ///  need to set the default params and then reset() the audio or
        ///  set a new module.
        ///

        const RendererParameters& currentParameters() const
        {
            return m_parameters;
        }

        ///
        ///  Get possible formats and rates for a given device. These
        ///  probably will not work once the audio is being used. Calling
        ///  reset() before calling these will ensure that they operate
        ///  properly. (NOTE: once you call reset the device structs will
        ///  no longer be valid and you will need to get a new device
        ///  list).
        ///

        virtual void availableLayouts(const Device&, LayoutsVector&);
        virtual void availableFormats(const Device&, FormatVector&);
        virtual void availableRates(const Device&, Format, RateVector&);

        ///
        ///  play() should return almost immediately. The derived class should
        ///  use a separate worker thread for playing.
        ///  NOTE: play will throw if something goes wrong.
        ///

        virtual void play();
        virtual void play(Session*);

        ///
        ///  isPlaying() needs to return true only when the audio is
        ///  *actually* playing.
        ///

        virtual bool isPlaying() const;

        ///
        ///  stop() will cause the worker thread to wait until play.
        ///  NOTE: stop will throw if something goes wrong.
        ///  NOTE: stopOnTurnAround should be used to stop the audio
        ///  session play state but not stop the audio renderer in terms
        ///  of closing or suspending the audio device.
        ///

        virtual void stop();
        virtual void stop(Session*);
        virtual void stopOnTurnAround(Session*);

        ///
        ///  shutdown() is like stop, but it will also cause any hardware
        ///  connection to be completely let go. This may be meaningless
        ///  for some types of audio renderer.
        ///

        virtual void shutdown();
        virtual void shutdownOnLast();

        ///
        ///  The device state is the actual state of the device after it
        ///  has been opened.
        ///

        const DeviceState& deviceState() const { return m_deviceState; }

        ///
        ///  Will be false if an error condition exists preventing audio
        ///  playback
        ///

        bool isOK() const;

        ///
        /// Set the error condistion to true and store the msg and then throws
        ///

        virtual void setErrorCondition(const std::string& msg);

        ///
        /// calls setErrorCondition() with the existing error msg
        //

        void resetErrorCondition();

        ///
        /// Restore error condition
        ///

        void clearErrorCondition();

        ///
        ///  If !isOK() than errorString() should have a human readable
        ///  reason.
        ///

        std::string errorString() const;

        static void outputParameters(const RendererParameters&);

        ///
        /// Returns the hardware lock renderer param setting.
        ///

        bool hasHardwareLock() const { return m_parameters.hardwareLock; }

        ///
        /// Returns if the renderer has PerRoll.
        ///

        bool hasPreRoll() const { return m_parameters.preRoll; }

        TwkAudio::Time preRollDelay() const { return m_preRollDelay; }

        void setPreRollDelay(TwkAudio::Time t) { m_preRollDelay = t; }

        ///
        /// Utility Functions
        ///

        template <typename T> static T toType(float a)
        {
            if (a > 1.0f)
                a = 1.0f;
            else if (a < -1.0f)
                a = -1.0f;
            return T(double(a) * double(std::numeric_limits<T>::max()));
        }

        template <typename T> static T toUnsignedType(float a)
        {
            if (a > 1.0f)
                a = 1.0f;
            else if (a < -1.0f)
                a = -1.0f;
            return T((double(a) + 1.0) * 0.5
                     * double(std::numeric_limits<T>::max()));
        }

        static UInt32 toUInt32_SMPTE272M_20(float a, UInt32 adminBits)
        {
            //
            // adminBits should have these set:
            //
            // (U & 0x1) << 24 |    // user bit
            // (V & 0x1) << 23 |    // validity bit
            // (0x1 << 0) |         // AES block sync (Z bit)
            // (0x1 << 24) |        // AES channel status (C bit)
            //

            // NOTE: +-(2^23-1) == 4194303 So we're safe keeping this as float
            // +-(2^19-1) == 262143

            if (a > 1.0f)
                a = 1.0f;
            else if (a < -1.0f)
                a = -1.0f;

            return (UInt32(a * 262143.0f) << 3) | adminBits;
        }

        static UInt32 toUInt32_SMPTE299M_24(float a, UInt32 adminBits)
        {
            //
            // adminBits should have these set:
            //
            // (U & 0x1) << 29 |    // user bit
            // (V & 0x1) << 28 |    // validity bit
            // (0x1 << 3) |         // AES block sync (Z bit)
            // (0x1 << 30) |        // AES channel status (C bit)
            //

            if (a > 1.0f)
                a = 1.0f;
            else if (a < -1.0f)
                a = -1.0f;

            UInt32 sample = (UInt32(a * 4194303.0) & 0xffffff) << 4;
            sample |= adminBits;

            UInt32 p = sample;

            p = p ^ (p >> 16);
            p = p ^ (p >> 8);
            p = p ^ (p >> 4);
            p = p ^ (p >> 2);
            p = p ^ (p >> 1);
            p = p & 0x1;

            return sample | ((p & 0x1) << 31);
        }

        //
        //  You can use these to bind the adminBits upfront and pass the
        //  above funcs to std::transform
        //

        struct toUInt32_SMPTE272M_20_transformer
        {
            toUInt32_SMPTE272M_20_transformer(UInt32 adminBits = 0)
                : bits(adminBits)
                , count(0)
            {
            }

            UInt32 bits;
            size_t count;

            UInt32 operator()(float a)
            {
                UInt32 b = bits;
                if (count % 192 == 0)
                    b |= (1 << 0) | (1 << 25);
                count++;
                return toUInt32_SMPTE272M_20(a, b);
            }
        };

        struct toUInt32_SMPTE299M_24_transformer
        {
            toUInt32_SMPTE299M_24_transformer(UInt32 adminBits = 0)
                : bits(adminBits)
                , count(0)
            {
            }

            UInt32 bits;
            size_t count;

            UInt32 operator()(float a)
            {
                UInt32 b = bits;
                if (count % 192 == 0)
                    b |= (1 << 3) | (1 << 30);
                count++;
                return toUInt32_SMPTE299M_24(a, b);
            }
        };

        static void
        transformFloat32ToInt24(TwkAudio::AudioBuffer::BufferPointer inData,
                                char* outData, size_t dataCount,
                                bool isLittleEndian = true);

    protected:
        AudioRenderer(const RendererParameters&);
        void lockSessions();
        void unlockSessions();
        bool hasSessions();
        void getSessions(std::vector<Session*>&);
        bool addSession(Session*);
        bool deleteSession(Session*);

        void setDeviceState(const DeviceState&);

        //
        //  These functions synchronize with the session and call the
        //  audio evaluation to fetch audio from the audio cache.
        //

        void audioFillBuffer(AudioBuffer&);
        void audioFillBuffer(Session*, AudioBuffer&);

        void lockPlaying() const;
        void unlockPlaying() const;

        static void loadModule(Module&);

    protected:
        bool m_error;
        std::string m_errorString;
        mutable pthread_mutex_t m_errorLock;
        Sessions m_sessions;
        pthread_mutex_t m_sessionsLock;
        DeviceVector m_outputDevices;
        RendererParameters m_parameters;
        DeviceState m_deviceState;
        AudioBuffer m_abuffer;
        TwkAudio::Time m_lastSampleTime;
        TwkAudio::Time m_preRollDelay;

        static RendererParameters m_defaultParameters;
        static AudioRenderer* m_renderer;
        static bool m_noaudio;
        static bool m_disableAudio;
        static ModuleVector m_modules;
        static size_t m_moduleIndex;
        static size_t m_savedIndex;
        static ModuleInitVector m_moduleInitObjects;

    private:
        mutable pthread_mutex_t m_playLock;
        bool m_playing;
    };

}; // namespace IPCore

#endif // __IPCore__AudioRenderer__h__
