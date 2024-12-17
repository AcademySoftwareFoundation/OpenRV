//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <ALSASafeAudioModule/ALSASafeAudioRenderer.h>
#include <RvApp/RvSession.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <stl_ext/string_algo.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <TwkAudio/Audio.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkMovie/Movie.h>
#include <iostream>
#include <sstream>

//
//  "Safe" ALSA is some unknown subset of the ALSA API that works
//  under more conditions than the "not safe". This API is refered to
//  by a PulseAudio developer here:
//
//          http://0pointer.de/blog/projects/guide-to-sound-apis.html
//
//  Here's the important excerpt:
//
//    You want to know more about the safe ALSA subset?
//
//    Here's a list of DOS and DONTS in the ALSA API if you care about
//    that you application stays future-proof and works fine with
//    non-hardware backends or backends for user-space sound drivers
//    such as Bluetooth and FireWire audio. Some of these
//    recommendations apply for people using the full ALSA API as
//    well, since some functionality should be considered obsolete for
//    all cases.
//
//    If your application's code does not follow these rules, you must
//    have a very good reason for that. Otherwise your code should
//    simply be considered broken!
//
//    DONTS:
//
//        * Do not use "async handlers", e.g. via
//          snd_async_add_pcm_handler() and friends. Asynchronous
//          handlers are implemented using POSIX signals, which is a
//          very questionable use of them, especially from libraries
//          and plugins. Even when you don't want to limit yourself to
//          the safe ALSA subset it is highly recommended not to use
//          this functionality. Read this for a longer explanation why
//          signals for audio IO are evil.
//
//        * Do not parse the ALSA configuration file yourself or with
//          any of the ALSA functions such as snd_config_xxx(). If you
//          need to enumerate audio devices use snd_device_name_hint()
//          (and related functions). That is the only API that also
//          supports enumerating non-hardware audio devices and audio
//          devices with drivers implemented in userspace.
//
//        * Do not parse any of the files from /proc/asound/. Those
//          files only include information about kernel sound drivers
//          -- user-space plugins are not listed there. Also, the set
//          of kernel devices might differ from the way they are
//          presented in user-space. (i.e. sub-devices are mapped in
//          different ways to actual user-space devices such as
//          surround51 an suchlike.
//
//        * Do not rely on stable device indexes from ALSA. Nowadays
//          they depend on the initialization order of the drivers
//          during boot-up time and are thus not stable.
//
//        * Do not use the snd_card_xxx() APIs. For enumerating use
//          snd_device_name_hint() (and related
//          functions). snd_card_xxx() is obsolete. It will only list
//          kernel hardware devices. User-space devices such as sound
//          servers, Bluetooth audio are not included. snd_card_load()
//          is completely obsolete in these days.
//
//        * Do not hard-code device strings, especially not hw:0 or
//          plughw:0 or even dmix -- these devices define no channel
//          mapping and are mapped to raw kernel devices. It is highly
//          recommended to use exclusively default as device
//          string. If specific channel mappings are required the
//          correct device strings should be front for stereo,
//          surround40 for Surround 4.0, surround41, surround51, and
//          so on. Unfortunately at this point ALSA does not define
//          standard device names with channel mappings for non-kernel
//          devices. This means default may only be used safely for
//          mono and stereo streams. You should probably prefix your
//          device string with plug: to make sure ALSA transparently
//          reformats/remaps/resamples your PCM stream for you if the
//          hardware/backend does not support your sampling parameters
//          natively.
//
//        * Do not assume that any particular sample type is supported
//          except the following ones: U8, S16_LE, S16_BE, S32_LE,
//          S32_BE, FLOAT_LE, FLOAT_BE, MU_LAW, A_LAW.
//
//        * Do not use snd_pcm_avail_update() for synchronization
//          purposes. It should be used exclusively to query the
//          amount of bytes that may be written/read right now. Do not
//          use snd_pcm_delay() to query the fill level of your
//          playback buffer. It should be used exclusively for
//          synchronisation purposes. Make sure you fully understand
//          the difference, and note that the two functions return
//          values that are not necessarily directly connected!
//
//        * Do not assume that the mixer controls always know dB
//          information.
//
//        * Do not assume that all devices support MMAP style buffer
//          access.
//
//        * Do not assume that the hardware pointer inside the
//          (possibly mmaped) playback buffer is the actual position
//          of the sample in the DAC. There might be an extra latency
//          involved.
//
//        * Do not try to recover with your own code from ALSA error
//          conditions such as buffer under-runs. Use
//          snd_pcm_recover() instead.
//
//        * Do not touch buffering/period metrics unless you have
//          specific latency needs. Develop defensively, handling
//          correctly the case when the backend cannot fulfill your
//          buffering metrics requests. Be aware that the buffering
//          metrics of the playback buffer only indirectly influence
//          the overall latency in many cases. i.e. setting the buffer
//          size to a fixed value might actually result in practical
//          latencies that are much higher.
//
//        * Do not assume that snd_pcm_rewind() is available and works
//          and to which degree.
//
//        * Do not assume that the time when a PCM stream can receive
//          new data is strictly dependant on the sampling and
//          buffering parameters and the resulting average
//          throughput. Always make sure to supply new audio data to
//          the device when it asks for it by signalling "writability"
//          on the fd. (And similarly for capturing)
//
//        * Do not use the "simple" interface snd_spcm_xxx().
//
//        * Do not use any of the functions marked as "obsolete".
//
//        * Do not use the timer, midi, rawmidi, hwdep subsystems.
//
//    DOS:
//
//        * Use snd_device_name_hint() for enumerating audio devices.
//
//        * Use snd_smixer_xx() instead of raw snd_ctl_xxx()
//
//        * For synchronization purposes use snd_pcm_delay().
//
//        * For checking buffer playback/capture fill level use
//          snd_pcm_update_avail().
//
//        * Use snd_pcm_recover() to recover from errors returned by any
//          of the ALSA functions.
//
//        * If possible use the largest buffer sizes the device
//          supports to maximize power saving and drop-out safety. Use
//          snd_pcm_rewind() if you need to react to user input
//          quickly.
//

#define USE_SAFE_ALSA 1

#include <alsa/asoundlib.h>

namespace IPCore
{
    using namespace std;
    using namespace stl_ext;
    using namespace TwkApp;
    using namespace TwkAudio;

    namespace
    {

        template <typename T> T toType(float a)
        {
            return T(double(a) * double(numeric_limits<T>::max()));
        }

    } // namespace

    static void alsaErrorHandler(const char* file, int line,
                                 const char* function, int err, const char* fmt,
                                 ...)
    {
        // don't print out ALSA asserts
    }

    static int snd_pcm_open_real_hard(snd_pcm_t** pcm, const char* name,
                                      snd_pcm_stream_t stream, int mode,
                                      size_t iterations = 1)
    {
        //
        //  Try REAL HARD to open the device. If it doesn't open right
        //  away. Wait a second and see if anything improves. There's
        //  probably a way to avoid this by waiting until the device is
        //  drained, but one might assume snd_pcm_drain() would do that
        //  wouldn't one?
        //
        //  It seems like the device is still playing back accumulated
        //  samples and cannot be opened (e.g., switching to an exclusive
        //  hardware device from default). If you wait for the device to
        //  drain, it will eventually work. This technique (or a
        //  variation) is applied by a number of other programs on the
        //  net.
        //

        int err = snd_pcm_open(pcm, name, stream, mode);

#if 0
    for (size_t i = 0; i < iterations && err == -EBUSY; i++)
    {
        usleep(10000);
        err = snd_pcm_open(pcm, name, stream, mode);
    }
#endif

        if (!err && *pcm)
            snd_pcm_nonblock(*pcm, 0); // 0 means block
        return err;
    }

    ALSASafeAudioRenderer::ALSASafeAudioRenderer(const RendererParameters& p)
        : AudioRenderer(p)
        , m_threadGroup(1, 2)
        , m_pcm(0)
        , m_audioThread(pthread_self())
        , m_audioThreadRunning(false)
    {
        snd_lib_error_set_handler(alsaErrorHandler);
        pthread_mutex_init(&m_runningLock, 0);

        if (m_parameters.rate == 0)
            m_parameters.rate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
        if (m_parameters.framesPerBuffer == 0)
            m_parameters.framesPerBuffer = 512;
        if (m_parameters.device == "")
            m_parameters.device = "default";

        createDeviceList();

        if (debug)
            outputParameters(m_parameters);

#ifndef PLATFORM_WINDOWS
        /*
        We used to initialize the device at this point, but if something
        goes wrong, it means we can't even change the device in the
        prefs, because audio is shutdown.  If we skip this step, we'll
        hold open the device after the first play().
        */

        if (m_parameters.holdOpen)
        {
            play();
            AudioRenderer::stop();
        }
#endif
    }

    ALSASafeAudioRenderer::~ALSASafeAudioRenderer()
    {
        AudioRenderer::stop();
        shutdown();
        pthread_mutex_destroy(&m_runningLock);
    }

    void ALSASafeAudioRenderer::createDeviceList()
    {
        m_outputDevices.clear();
        m_alsaDevices.clear();

#ifdef USE_SAFE_ALSA
        //
        //  use the snd_device_name_hint() function
        //

        void** deviceList = 0;
        snd_device_name_hint(-1, "pcm", (void***)&deviceList);

        //
        //  For whatever reason ("pulseaudio sucks" ?) the list of devices
        //  returned here is sometimes incomplete (missing the "pusle" device).
        //  But calling the same call again seems to work around the problem ...
        //

        snd_device_name_free_hint(deviceList);
        snd_device_name_hint(-1, "pcm", (void***)&deviceList);

        for (void** dv = deviceList; *dv; dv++)
        {
            char* namep = snd_device_name_get_hint(*dv, "NAME");
            char* descp = snd_device_name_get_hint(*dv, "DESC");
            const string name = namep;
            const string desc = descp;

            //
            //  we only do stereo right now
            //
            if (name == "null" || name.find("surround") != string::npos)
                continue;

            string::size_type n = desc.find('\n');
            const string desc0 = desc.substr(0, n);
            const string desc1 = desc.substr(n + 1, desc.size());

            free(namep);
            free(descp);

            Device d(name);
            d.layout = TwkAudio::UnknownLayout;
            d.defaultRate = 0;
            d.isDefaultDevice = false;
            d.index = m_outputDevices.size();

            ALSADevice ad;
            ad.plug = "plugIn";
            ad.name = name;
            ad.card = "";

            m_outputDevices.push_back(d);
            m_alsaDevices.push_back(ad);
        }

        snd_device_name_free_hint(deviceList);

#else

        for (int card = -1; snd_card_next(&card) >= 0 && card >= 0;)
        {
            //
            //  Find the card devices
            //

            snd_ctl_t* ctl = 0;
            char* cardname = 0;
            ostringstream hwcard;
            hwcard << "hw:" << card;

            snd_card_get_longname(card, &cardname);

            if (snd_ctl_open(&ctl, hwcard.str().c_str(), 0) == 0)
            {
                snd_pcm_info_t* info = 0;
                snd_pcm_info_alloca(&info);

                for (int device = -1;
                     snd_ctl_pcm_next_device(ctl, &device) >= 0 && device >= 0;)
                {
                    snd_pcm_info_set_device(info, device);
                    snd_pcm_info_set_subdevice(info, 0);
                    snd_pcm_info_set_stream(info, SND_PCM_STREAM_PLAYBACK);

                    if (snd_ctl_pcm_info(ctl, info) >= 0)
                    {
                        ostringstream fullname;

                        fullname << cardname << ": "
                                 << snd_pcm_info_get_name(info) << " ("
                                 << hwcard.str() << "," << device << ")";

                        ostringstream dname;
                        dname << hwcard.str() << "," << device;

                        Device d(fullname.str());
                        d.layout = TwkAudio::UnknownLayout;
                        d.defaultRate = 0;
                        d.isDefaultDevice = false;
                        d.index = m_outputDevices.size();

                        char* c;
                        snd_card_get_name(card, &c);

                        ALSADevice ad;
                        ad.plug = "hw";
                        ad.name = dname.str();
                        ad.card = c;

                        m_outputDevices.push_back(d);
                        m_alsaDevices.push_back(ad);
                    }
                }

                snd_ctl_close(ctl);
            }
        }

#endif

        //
        //  See if RV_ALSA_EXTRA_DEVICES is set and if so scarf the
        //  devices out of there too.
        //

        if (const char* xdevices = getenv("RV_ALSA_EXTRA_DEVICES"))
        {
            vector<string> tokens;
            stl_ext::tokenize(tokens, xdevices, "|");

            for (size_t i = 0; i < tokens.size(); i++)
            {
                vector<string> dtokens;
                stl_ext::tokenize(dtokens, tokens[i], "@");

                string name;
                string alsaDevice;

                if (dtokens.size() == 1)
                {
                    name = tokens[i];
                    alsaDevice = name;
                }
                else if (dtokens.size() == 2)
                {
                    name = dtokens[0];
                    alsaDevice = dtokens[1];
                }
                else
                {
                    cerr << "ERROR: RV_ALSA_EXTRA_DEVICES env variable -- "
                            "syntax error"
                         << endl;
                    break;
                }

                Device d(name);
                d.layout = TwkAudio::Stereo_2;
                d.defaultRate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
                d.latencyLow = 0;
                d.latencyHigh = 0;
                d.isDefaultDevice = false;
                d.index = m_outputDevices.size();

                ALSADevice ad;
                ad.plug = "plughw";
                ad.card = "PlugIn Module";
                ad.name = alsaDevice;

                m_outputDevices.push_back(d);
                m_alsaDevices.push_back(ad);
            }
        }

        //
        //  Add default
        //

        Device d("default");
        d.layout = TwkAudio::Stereo_2;
        d.defaultRate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
        d.latencyLow = 0;
        d.latencyHigh = 0;
        d.isDefaultDevice = true;
        d.index = m_outputDevices.size();

        ALSADevice ad;
        ad.plug = "plughw";
        ad.card = "PlugIn Module";
        ad.name = "default";

        m_outputDevices.push_back(d);
        m_alsaDevices.push_back(ad);

        //
        //  Copy the current state from the parameters
        //

        DeviceState state;
        state.device = m_parameters.device;
        state.format = m_parameters.format;
        state.rate = m_parameters.rate;
        state.latency = m_parameters.latency;
        state.layout = m_parameters.layout;
        state.framesPerBuffer = m_parameters.framesPerBuffer;
        setDeviceState(state);

        //
        //  Force allocation of memory now
        //

        int channelsCount = TwkAudio::channelsCount(state.layout);
        m_abuffer.reconfigure(state.framesPerBuffer, state.layout,
                              Time(state.rate), 0, 0);
        m_outBuffer.resize(m_abuffer.size() * channelsCount
                           * formatSizeInBytes(state.format));
    }

    void ALSASafeAudioRenderer::availableLayouts(const Device& d,
                                                 LayoutsVector& layouts)
    {
        layouts.clear();

        layouts.push_back(TwkAudio::Mono_1);
        layouts.push_back(TwkAudio::Stereo_2);
    }

    void ALSASafeAudioRenderer::availableFormats(const Device& d,
                                                 FormatVector& formats)
    {
        //
        //  The base class ensures that this function is not called when a
        //  device is opened.
        //

        formats.clear();
        static Format allformats[4] = {Float32Format, Int32Format, Int16Format,
                                       Int8Format};

        const ALSADevice& ad = m_alsaDevices[d.index];
        const string& deviceName = ad.name;

        for (size_t i = 0; i < 3; i++)
        {
            Format format = allformats[i];

            snd_pcm_t* pcm;
            int err;

            if ((err = snd_pcm_open_real_hard(&pcm, deviceName.c_str(),
                                              SND_PCM_STREAM_PLAYBACK,
                                              SND_PCM_NONBLOCK, 10))
                == 0)
            {
                snd_pcm_hw_params_t* params;
                snd_pcm_hw_params_alloca(&params);
                snd_pcm_hw_params_any(pcm, params);

                unsigned int rrate = (unsigned int)m_parameters.rate;

                snd_pcm_format_t alsaformat;

                switch (format)
                {
                case Float32Format:
                    alsaformat = SND_PCM_FORMAT_FLOAT_LE;
                    break;
                case Int32Format:
                    alsaformat = SND_PCM_FORMAT_S32_LE;
                    break;
                case Int16Format:
                    alsaformat = SND_PCM_FORMAT_S16_LE;
                    break;
                case Int8Format:
                    alsaformat = SND_PCM_FORMAT_S8;
                    break;
                }

                int dir = 0;
                snd_pcm_hw_params_set_access(pcm, params,
                                             SND_PCM_ACCESS_RW_INTERLEAVED);
                bool ok =
                    snd_pcm_hw_params_set_format(pcm, params, alsaformat) == 0;
                snd_pcm_hw_params_set_channels(
                    pcm, params, TwkAudio::channelsCount(m_parameters.layout));
                snd_pcm_hw_params_set_rate_near(pcm, params, &rrate, &dir);

                if (ok && rrate == m_parameters.rate)
                {
                    int periods = 2;
                    snd_pcm_uframes_t period_size = 8192;
                    snd_pcm_uframes_t buffer_size = 0;

                    snd_pcm_hw_params_set_periods(pcm, params, periods, 0);
                    snd_pcm_hw_params_set_buffer_size(
                        pcm, params, (period_size * periods) >> 2);

                    if (snd_pcm_hw_params(pcm, params) >= 0)
                    {
                        formats.push_back(format);
                    }
                }

                snd_pcm_close(pcm);
            }
            else if (err == -EBUSY)
            {
                //
                //  The device isn't available
                //

                break;
            }
        }
    }

    void ALSASafeAudioRenderer::availableRates(const Device& device,
                                               Format format, RateVector& rates)
    {
        //
        //  The base class ensures that this function is not called when a
        //  device is opened.
        //

        rates.clear();

        static double standardSampleRates[] = {
            8000.0,  9600.0,  11025.0,  12000.0, 16000.0,
            22050.0, 24000.0, 32000.0,  44100.0, 48000.0,
            88200.0, 96000.0, 192000.0, -1 /* negative terminated  list */
        };

        const ALSADevice& ad = m_alsaDevices[device.index];
        const string& deviceName = ad.name;

        for (size_t i = 0; standardSampleRates[i] > 0; i++)
        {
            double rate = standardSampleRates[i];
            snd_pcm_t* pcm;
            int err;

            if ((err = snd_pcm_open_real_hard(&pcm, deviceName.c_str(),
                                              SND_PCM_STREAM_PLAYBACK,
                                              SND_PCM_NONBLOCK, 10))
                == 0)
            {
                snd_pcm_hw_params_t* params;
                snd_pcm_hw_params_alloca(&params);
                snd_pcm_hw_params_any(pcm, params);

                unsigned int rrate = (unsigned int)rate;

                snd_pcm_format_t alsaformat;

                switch (m_parameters.format)
                {
                case Float32Format:
                    alsaformat = SND_PCM_FORMAT_FLOAT_LE;
                    break;
                case Int32Format:
                    alsaformat = SND_PCM_FORMAT_S32_LE;
                    break;
                case Int16Format:
                    alsaformat = SND_PCM_FORMAT_S16_LE;
                    break;
                case Int8Format:
                    alsaformat = SND_PCM_FORMAT_S8;
                    break;
                }

                //
                //  This doesn't work -- why does ALSA say it does?
                //

                int dir = 0;
                snd_pcm_hw_params_set_access(pcm, params,
                                             SND_PCM_ACCESS_RW_INTERLEAVED);
                snd_pcm_hw_params_set_format(pcm, params, alsaformat);
                snd_pcm_hw_params_set_channels(
                    pcm, params, TwkAudio::channelsCount(m_parameters.layout));
                snd_pcm_hw_params_set_rate_near(pcm, params, &rrate, &dir);

                if (rrate == rate)
                    rates.push_back((unsigned int)rate);

#if 0
            int periods = 2;
            snd_pcm_uframes_t period_size = 8192;
            snd_pcm_uframes_t buffer_size = 0;

            snd_pcm_hw_params_set_periods(pcm, params, periods, 0);
            snd_pcm_hw_params_set_buffer_size(pcm, params, (period_size * periods) >> 2);
            
            if (snd_pcm_hw_params(pcm, params) >= 0 && rrate == rate)
            {
                rates.push_back((unsigned int)rate);
            }
#endif

                snd_pcm_close(pcm);
            }
        }
    }

    static void trampoline(void* data)
    {
        ALSASafeAudioRenderer* a = (ALSASafeAudioRenderer*)data;
        a->threadMain();
    }

    void ALSASafeAudioRenderer::configureDevice()
    {
        int err;

        if (!m_pcm)
        {
            string alsaDeviceName = "default";
            int dindex = findDeviceByName(m_parameters.device);

            if (dindex >= 0 && dindex < m_alsaDevices.size())
            {
                const ALSADevice& ad = m_alsaDevices[dindex];
                alsaDeviceName = ad.name;
            }

            if ((err = snd_pcm_open_real_hard(&m_pcm, alsaDeviceName.c_str(),
                                              SND_PCM_STREAM_PLAYBACK,
                                              SND_PCM_NONBLOCK))
                < 0)
            {
                cerr << "WARNING: ALSA: Playback open error (" << err
                     << "): " << snd_strerror(err) << endl;

                if (alsaDeviceName != "default" && err != -16) /* busy */
                {
                    cerr << "WARNING: ALSA: trying default instead of "
                         << alsaDeviceName << endl;

                    if ((err = snd_pcm_open_real_hard(&m_pcm, "default",
                                                      SND_PCM_STREAM_PLAYBACK,
                                                      SND_PCM_NONBLOCK))
                        < 0)
                    {
                        cerr << "WARNING: ALSA: no luck with default either"
                             << endl;
                        setErrorCondition("Unable to open an audio device");
                    }
                    else
                    {
                        cerr << "WARNING: ALSA: using default device instead"
                             << endl;
                        m_deviceState.device = "default";
                        alsaDeviceName = "default";
                    }
                }
                else
                {
                    setErrorCondition("Unable to open an audio device");
                }
            }

            snd_pcm_hw_params_t* params;
            snd_pcm_hw_params_alloca(&params);
            snd_pcm_hw_params_any(m_pcm, params);

            snd_pcm_format_t alsaformat;
            size_t dataSize = 0;

            switch (m_parameters.format)
            {
            case Float32Format:
                alsaformat = SND_PCM_FORMAT_FLOAT_LE;
                dataSize = sizeof(float);
                break;
            default:
            case Int16Format:
                alsaformat = SND_PCM_FORMAT_S16_LE;
                dataSize = sizeof(short);
                break;
            case Int32Format:
                alsaformat = SND_PCM_FORMAT_S32_LE;
                dataSize = sizeof(signed int);
                break;
            case Int8Format:
                alsaformat = SND_PCM_FORMAT_S8;
                dataSize = sizeof(char);
                break;
            }

            unsigned int rrate = (unsigned int)m_parameters.rate;
            int dir = 0;

            if (snd_pcm_hw_params_set_access(m_pcm, params,
                                             SND_PCM_ACCESS_RW_INTERLEAVED)
                < 0)
            {
                cerr << "ERROR: ALSA: access (interleaved) failed - can't "
                        "continue"
                     << endl;
                outputParameters(m_parameters);
                snd_pcm_close(m_pcm);
                m_pcm = 0;
                setErrorCondition("unable to configure audio device");
            }

            if (snd_pcm_hw_params_set_format(m_pcm, params, alsaformat) < 0)
            {
                if (snd_pcm_hw_params_set_format(m_pcm, params,
                                                 SND_PCM_FORMAT_S16_LE)
                    >= 0)
                {
                    if (debug)
                        cerr << "WARNING: ALSA: format falling back to Int16"
                             << endl;
                    m_parameters.format = Int16Format;
                }
                else
                {
                    cerr << "ERROR: ALSA: format failed - can't continue"
                         << endl;
                    outputParameters(m_parameters);
                    snd_pcm_close(m_pcm);
                    m_pcm = 0;
                    setErrorCondition("unable to configure audio device");
                }
            }

            int channelsCount = TwkAudio::channelsCount(m_parameters.layout);

            if (snd_pcm_hw_params_set_channels(m_pcm, params, channelsCount)
                < 0)
            {
                cerr << "ERROR: ALSA: can't use " << channelsCount
                     << " channels -- can't continue" << endl;
                outputParameters(m_parameters);
                snd_pcm_close(m_pcm);
                m_pcm = 0;
                setErrorCondition("unable to configure audio device");
            }

            if (snd_pcm_hw_params_set_rate_near(m_pcm, params, &rrate, &dir)
                < 0)
            {
                cerr << "ERROR: ALSA: unable to set rate near " << rrate
                     << " -- can't continue" << endl;
                outputParameters(m_parameters);
                snd_pcm_close(m_pcm);
                m_pcm = 0;
                setErrorCondition("unable to configure audio device");
            }

            snd_pcm_hw_params_set_periods_integer(m_pcm, params);

            //
            //  Reasonably low latency
            //

            unsigned int periods = 2;
            m_periodSize = snd_pcm_uframes_t(double(512) / double(48000.0)
                                             * double(m_parameters.rate));
            m_bufferSize = m_periodSize * periods * channelsCount * dataSize;
            dir = 0;

            snd_pcm_hw_params_set_periods(m_pcm, params, periods, 0);
            snd_pcm_hw_params_set_period_size_near(m_pcm, params, &m_periodSize,
                                                   &dir);
            snd_pcm_hw_params_set_buffer_size_near(m_pcm, params,
                                                   &m_bufferSize);

            if ((err = snd_pcm_hw_params(m_pcm, params)) < 0)
            {
                cerr << "ERROR: ALSA: params failed: " << snd_strerror(err)
                     << endl;
                snd_pcm_close(m_pcm);
                m_pcm = 0;
                setErrorCondition("unable to configure audio device");
            }

            snd_pcm_hw_params_get_buffer_size(params, &m_bufferSize);
            snd_pcm_hw_params_get_period_size(params, &m_periodSize, &dir);
            periods = m_bufferSize / m_periodSize;

            //  LATENCY:
            //
            //      latency = periodsize * periods / (rate * bytes_per_frame)
            //
            //  is this even correct? Some sources claim there are still
            //  other latencies beyond this. I really don't want to add an
            //  extra latency UI.
            //

            DeviceState nstate;
            nstate.device = m_parameters.device;
            nstate.format = m_parameters.format;
            nstate.rate = rrate;
            nstate.layout = m_parameters.layout;
            nstate.latency =
                m_parameters.latency
                + m_periodSize * periods
                      / (nstate.rate * sizeof(short) * channelsCount);
            // THIS WILL BLOW IT UP IF ITS NOT 512
            // nstate.framesPerBuffer = 2048;
            nstate.framesPerBuffer = m_parameters.framesPerBuffer;
            setDeviceState(nstate);

            if (debug)
            {
                cout << "DEBUG: alsa buffer_size = " << m_bufferSize << endl
                     << "DEBUG: alsa period_size = " << m_periodSize << endl
                     << "DEBUG: alsa periods = " << periods << endl
                     << "DEBUG: alsa device latency = " << nstate.latency
                     << endl;
            }

            //
            //  Wait for the device to be ready
            //

            snd_pcm_wait(m_pcm, -1);
        }
    }

    void ALSASafeAudioRenderer::threadMain()
    {
        //
        //  Set the thread priority if asked to do so
        //

        const Rv::Options& opts = Rv::Options::sharedOptions();

        if (opts.schedulePolicy)
        {
            sched_param sp;
            memset(&sp, 0, sizeof(sched_param));
            sp.sched_priority = opts.displayPriority;

            unsigned int policy = SCHED_OTHER;

            if (!strcmp(opts.schedulePolicy, "SCHED_RR"))
                policy = SCHED_RR;
            else if (!strcmp(opts.schedulePolicy, "SCHED_FIFO"))
                policy = SCHED_FIFO;

            if (policy != SCHED_OTHER)
            {
                if (sched_setscheduler(0, policy, &sp))
                {
                    cout << "ERROR: can't set thread priority" << endl;
                }
            }
            else
            {
                if (setpriority(PRIO_PROCESS, 0, opts.displayPriority))
                {
                    cout << "ERROR: can't set thread priority" << endl;
                }
            }
        }

        int err;

        //
        //  NOTE: it's possible for this thread to continue on as a
        //  zombie (after the control thread tried to stop it).  In that
        //  case m_pcm will be null, so check for that whenever we're
        //  going to use it.  This should stop the "asset failed: pcm"
        //  crashes we get from alsa.
        //
        if (!m_pcm)
            return;

        m_threadGroup.lock(m_runningLock);
        m_audioThreadRunning = true;
        m_audioThread = pthread_self();
        m_threadGroup.unlock(m_runningLock);

        //
        //  Get the current state. (yeah, I know it was just set above)
        //

        const DeviceState& state = deviceState();

        //
        //  Setup accumulation buffer
        //

        int channelsCount = TwkAudio::channelsCount(state.layout);

        AudioBuffer buffer(state.framesPerBuffer, state.layout, state.rate);

        m_abuffer.reconfigure(state.framesPerBuffer, state.layout,
                              Time(state.rate));

        const size_t formatSize = formatSizeInBytes(m_parameters.format);
        m_outBuffer.resize(state.framesPerBuffer * channelsCount * formatSize);

        //
        //  Allocate and initialize the "status" object for this pcm stream
        //

        snd_pcm_status_t* status;
        snd_pcm_status_alloca(&status);
        if (!m_pcm)
            return;
        snd_pcm_status(m_pcm, status);

        if (!m_pcm)
            return;
        snd_pcm_state_t pcmState = snd_pcm_state(m_pcm);

        if (pcmState == SND_PCM_STATE_SETUP)
        {
            if (!m_pcm)
                return;
            if ((err = snd_pcm_prepare(m_pcm)) < 0)
            {
                cerr << "ERROR: ASLA: " << snd_strerror(err) << endl;
                return;
            }

            if (!m_pcm)
                return;
            pcmState = snd_pcm_state(m_pcm);
        }

        m_startSample = 0;

        for (size_t count = 0;
             pcmState != SND_PCM_STATE_DISCONNECTED
             && pcmState != SND_PCM_STATE_XRUN
             && (isPlaying() || (m_startSample % m_periodSize != 0)
                 || m_parameters.holdOpen);
             count++)
        {
            //
            //  Figure out the current latency by hook or by crook.
            //

            if (!m_pcm)
                return;
            snd_pcm_status(m_pcm, status);
            snd_pcm_sframes_t latencyFrames = snd_pcm_status_get_delay(status);

            if ((!latencyFrames || state.latency == 0) && count > 0)
            {
                //
                //  Update the latency using snd_pcm_delay because
                //  snd_pcm_status_get_delay() failed. This is common with
                //  the default device and/or dmix.
                //

                if (!m_pcm)
                    return;
                if (!snd_pcm_delay(m_pcm, &latencyFrames) && latencyFrames > 0)
                {
                    //
                    //  Only if we think this actually worked
                    //  (i.e. latencyFrames > 0)
                    //

                    m_deviceState.latency =
                        m_parameters.latency
                        + latencyFrames / double(state.rate);
                }
                else
                {
                    m_deviceState.latency = m_parameters.latency;
                }
            }

            //
            //  Call the evaluation function
            //

            buffer.reconfigure(state.framesPerBuffer, state.layout,
                               Time(state.rate),
                               samplesToTime(m_startSample, state.rate));

            buffer.zero();

            //
            //  Fetch the samples
            //

            audioFillBuffer(buffer);

            //
            //  Convert to the correct output format
            //

            unsigned char* out = &m_outBuffer.front();

            switch (m_parameters.format)
            {
            case Float32Format:
                memcpy(out, buffer.pointer(), buffer.sizeInBytes());
                break;

            case Int16Format:
                transform(buffer.pointer(),
                          buffer.pointer() + buffer.sizeInFloats(), (short*)out,
                          toType<short>);
                break;

            case Int32Format:
                transform(buffer.pointer(),
                          buffer.pointer() + buffer.sizeInFloats(),
                          (signed int*)out, toType<signed int>);
                break;

            case Int8Format:
                transform(buffer.pointer(),
                          buffer.pointer() + buffer.sizeInFloats(),
                          (signed char*)out, toType<signed char>);
                break;
            }

            //
            //  Write it. This may block waiting for the device. I'm not
            //  sure why we have to account for chopping up the output:
            //  sometimes ALSA just doesn't write all the samples and you
            //  have to call write again.
            //

            size_t total = 0;
            snd_pcm_sframes_t frames = 0;

            if (!m_pcm)
                return;
            pcmState = snd_pcm_state(m_pcm);
            ;

            //
            //  Write the samples.
            //
            //  NOTE: snd_pcm_writei() can BLOCK for an indefinite amount
            //  of time.
            //

            for (size_t wcount = 0; (pcmState == SND_PCM_STATE_RUNNING
                                     || pcmState == SND_PCM_STATE_PREPARED)
                                    && total < buffer.size() && frames >= 0
                                    && wcount < buffer.size();
                 wcount++)
            {
                if (!m_pcm)
                    return;
                if (debug)
                {
                    cerr << "INFO: pcm_writei: wcount " << wcount << ", count "
                         << count << ", total " << total << ", chan "
                         << channelsCount << ", formatSize " << formatSize
                         << ", bufferSize " << buffer.size() << endl;
                }
                frames = snd_pcm_writei(
                    m_pcm, out + (total * channelsCount * formatSize),
                    buffer.size() - total);
                if (debug)
                    cerr << "INFO: pcm_writei complete, frames " << frames
                         << endl;

                if (frames >= 0)
                {
                    total += frames;
                }
                else
                {
                    //
                    //  Possible underrun (in alsa parlance)
                    //

                    // too new to use

                    if (debug)
                    {
                        cerr << "ERROR: ASLA: write failed (" << frames
                             << "): " << snd_strerror(frames) << " -- ";

                        switch (frames)
                        {
                        case -EBADFD:
                            cerr << "bad pcm state";
                            break;
                        case -EPIPE:
                            cerr << "underrun occured";
                            break;
                        case -ESTRPIPE:
                            cerr << "suspend event occured";
                            break;
                        }

                        cerr << endl;
                    }

                    if (!m_pcm)
                        return;
#ifdef USE_SAFE_ALSA
                    frames = snd_pcm_recover(m_pcm, frames, 0);
#else
                    frames = snd_pcm_prepare(m_pcm);
#endif
                    break;
                }
                if (total < buffer.size())
                {
                    if (!m_pcm)
                        return;
                    pcmState = snd_pcm_state(m_pcm);
                }
            }

            m_startSample += total;
            if (!m_pcm)
                return;
            pcmState = snd_pcm_state(m_pcm);

            if (pcmState == SND_PCM_STATE_XRUN)
            {
                if (!m_pcm)
                    return;
                if (snd_pcm_prepare(m_pcm) < 0)
                    break;
                if (!m_pcm)
                    return;
                if ((pcmState = snd_pcm_state(m_pcm)) == SND_PCM_STATE_XRUN)
                    break;
            }
        }

        if (!m_pcm)
            return;
        if (pcmState == SND_PCM_STATE_RUNNING)
            snd_pcm_drop(m_pcm);

        m_threadGroup.lock(m_runningLock);
        m_audioThreadRunning = false;
        m_threadGroup.unlock(m_runningLock);

        if (debug)
            cout << "DEBUG: audio thread exiting" << endl;
    }

    void ALSASafeAudioRenderer::play(Session* s)
    {
        AudioRenderer::play(s);

        s->audioVarLock();
        s->setAudioTimeShift(numeric_limits<double>::max());
        s->setAudioStartSample(0);
        s->setAudioFirstPass(true);
        s->audioVarUnLock();

        s->audioConfigure();

        if (!isPlaying())
            play();
    }

    void ALSASafeAudioRenderer::play()
    {
        AudioRenderer::play();

        bool notconfigured = m_pcm == 0;
        configureDevice();

        //
        //  Dispatch the playback thread if its not running already. The
        //  last argument (async=false) indicates that we want to wait
        //  until the thread is running before the function returns.
        //

        if (notconfigured)
            m_threadGroup.dispatch(trampoline, this);
    }

    void ALSASafeAudioRenderer::stop(Session* s)
    {
        AudioRenderer::stop(s);
        s->setAudioTimeShift(numeric_limits<double>::max());
        if (!m_parameters.holdOpen)
            shutdown();
    }

    void ALSASafeAudioRenderer::shutdown()
    {
        //
        //  Shut down the hardware
        //

        const bool holdOpen = m_parameters.holdOpen;
        m_parameters.holdOpen = false;

        AudioRenderer::stop();

        m_threadGroup.lock(m_runningLock);
        bool isrunning = m_audioThreadRunning;
        m_threadGroup.unlock(m_runningLock);

        if (isrunning)
            m_threadGroup.control_wait(true, 1.0);

        if (m_pcm)
        {
            //
            //  Although we tried to stop the audio threads above, we
            //  may have failed (IE the control_wait may have timed out,
            //  so lock-bracket the m_pcm=0 statement, and don't close
            //  the audio device until after m_pcm has been zeroed.
            //
            snd_pcm_t* tmp = m_pcm;
            m_threadGroup.lock(m_runningLock);
            m_pcm = 0;
            m_threadGroup.unlock(m_runningLock);
            snd_pcm_close(tmp);
        }

        m_parameters.holdOpen = holdOpen;
    }

} // namespace IPCore
