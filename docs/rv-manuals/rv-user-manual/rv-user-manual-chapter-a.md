# A - Tuning Platform Audio for Linux

On Linux, RV's Platform Audio module is based on Qt's QAudioOutput, which is implemented against ALSA's api. As such, we have added several environment variables that allow us to better debug audio issues and which also allows users to tune the audio data IO performance between RV and their chosen playback audio hardware device. Note these environment variables are only supported on Linux.

The environment variables are as follows:

| Environment Variable              | Variable values                                                                                                                                                                                                                                                                                                                                                  |
| --------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| TWK_QTAUDIOOUTPUT_ENABLE_DEBUG    | 0 = No debugging msgs (default) <br> 1 = Standard debugging msgs ; displays the ALSA device hardware parameter values used to configure the audio device. It will also display errors like buffer underruns. <br> 2 = Verbose debugging msgs ; use this value for tracing crashes. Messages are displayed with each audio period write to the audio alsa device. |
| TWK_QTAUDIOOUTPUT_BUFFER_TIME     | value in microsecs e.g. 120000 for 120ms (default is 120000)                                                                                                                                                                                                                                                                                                     |
| TWK_QTAUDIOOUTPUT_PERIOD_TIME     | value in microsecs e.g. 20000 for 20ms (default is 20000)                                                                                                                                                                                                                                                                                                        |

Table K.1:

Platform Audio Environment Variables

```
# setenv TWK_QTAUDIOOUTPUT_ENABLE_DEBUG 1
# setenv RV_NO_CONSOLE_REDIRECT 1
# rv myclip.mov

Version 7.1.0, built on Aug 31 29 2016 at 03:05:11 (HEAD=166a76e). (L)
Copyright (c) 2016 Autodesk, Inc. All rights reserved.
INFO: myclip.mov
DEBUG: Number of available audio devices  3
DEBUG: Audio device =  "default"
DEBUG: Audio device actual =  "pulse"
DEBUG: ranges: pmin= 666 , pmax= 7281792 , bmin= 2000 , bmax= 21845334
DEBUG: used: buffer_frames= 5760 , period_frames= 960     # in sample count
DEBUG: used: buffer_size= 23040 , period_size= 3840       # in bytes
DEBUG: used: buffer_time= 120000 , period_time= 20000     # in microsecs
DEBUG: used: chunks= 6                                    # no of periods per buffer
DEBUG: used: max write periods/chunks=0                   # 0 implies max possile.
DEBUG: used: bytesAvailable= 23040                        # amount of free space in the audio buffer on device open().
```

The environment variables TWK_QTAUDIOOUTPUT_BUFFER_TIME and TWK_QTAUDIOOUTPUT_PERIOD_TIME can be used to set the buffer size and period size of the audio device. The audio buffer size is always an integral number of period sizes, in other words chose values such that TWK_QTAUDIOOUTPUT_BUFFER_TIME = N \* TWK_QTAUDIOOUTPUT_PERIOD_TIME; where N is an integer. Experimentally we have found N = 6 produced a measured audio - video sync < 10ms; while increasing or decreasing N from this value seemed to produce larger and increasingly worst av sync lag numbers.

It is worth remembering that TWK_QTAUDIOOUTPUT_BUFFER_TIME determines overall size of the audio buffer. The audio device will not start playing until the buffer is completely filled first. This means the buffer size can influence the lag at the beginning when play first starts.

So for a given TWK_QTAUDIOOUTPUT_BUFFER_TIME, TWK_QTAUDIOOUTPUT_PERIOD_TIME determines the number of period buffers within the overall buffer size and this influences the average av sync value and if too small <=3 leads to buffer underun errors and crackles in audio.

```
# setenv TWK_QTAUDIOOUTPUT_ENABLE_DEBUG 1
# setenv RV_NO_CONSOLE_REDIRECT 1
# setenv TWK_QTAUDIOOUTPUT_BUFFER_TIME 60000
# setenv TWK_QTAUDIOOUTPUT_PERIOD_TIME 20000
# rv myclip.mov

Version 7.1.0, built on Aug 31 29 2016 at 03:05:11 (HEAD=166a76e). (L)
Copyright (c) 2016 Autodesk, Inc. All rights reserved.
INFO: myclip.mov
DEBUG: Number of available audio devices  3
DEBUG: Audio device =  "default"
DEBUG: Audio device actual =  "pulse"
DEBUG: ranges: pmin= 666 , pmax= 7281792 , bmin= 2000 , bmax= 21845334
DEBUG: used: buffer_frames= 2880 , period_frames= 960     # in sample count
DEBUG: used: buffer_size= 11520 , period_size= 3840       # in bytes
DEBUG: used: buffer_time= 60000 , period_time= 20000      # in microsecs
DEBUG: used: chunks= 3                                    # no of periods per buffer
DEBUG: used: max write periods/chunks=0                   # 0 implies max possile.
DEBUG: used: bytesAvailable= 11520                        # amount of free space in the audio buffer on device open().
DEBUG: *** Buffer underrun: -32
DEBUG: *** Buffer underrun: -32
DEBUG: *** Buffer underrun: -32
DEBUG: *** Buffer underrun: -32
DEBUG: *** Buffer underrun: -32
```

The tuning process steps:

1. Launch RV and setup the File->Preferences->Audio tab settings as follows:
    1. Output Module: Platform Audio
    2. Output Device: Default
    3. Output Format: Stereo 32bit float 48000
    4. Enable 'Keep Audio device open when playing'
    5. Turn off 'Hardware Audio/Video Synchronization'
    6. Enable 'Scrubbing on by default'
2. Determine the smallest TWK_QTAUDIOOUTPUT_BUFFER_TIME and TWK_QTAUDIOOUTPUT_PERIOD_TIME settings before buffer underruns occur.
    1. Set Platform Audio debugging messaging on... TWK_QTAUDIOOUTPUT_ENABLE_DEBUG = 1.
    2. Set values for TWK_QTAUDIOOUTPUT_BUFFER_TIME and TWK_QTAUDIOOUTPUT_PERIOD_TIME.
    3. Try the following combinations of TWK_QTAUDIOOUTPUT_BUFFER_TIME / TWK_QTAUDIOOUTPUT_PERIOD_TIME i.e. 60000 / 10000 and 36000 / 6000.
    4. Launch RV with a 48Khz video clip and enable lookahead/region caching. Either cache option would do as long as you size the video cache appropriately so the entire clip is cached. Playback and observe for buffer underruns messages. Note you should also stress test running multiple RVs with the same config.
    5. If no underun errors occur; repeat the previous step, setting smaller values for TWK_QTAUDIOOUTPUT_BUFFER_TIME and TWK_QTAUDIOOUTPUT_PERIOD_TIME keeping in mind that the ratio of TWK_QTAUDIOOUTPUT_BUFFER_TIME/TWK_QTAUDIOOUTPUT_PERIOD_TIME must be greater than 3 and ideally around 6.
3. Once you have found the smallest value of TWK_QTAUDIOOUTPUT_BUFFER_TIME; use an av sync meter to measure the average av sync lag playing back RV's movieproc syncflash clip.
    1. With TWK_QTAUDIOOUTPUT_BUFFER_TIME fixed to the value determined in the previous step; increase and decrease TWK_QTAUDIOOUTPUT_PERIOD_TIME, bearing in mind TWK_QTAUDIOOUTPUT_BUFFER_TIME must be a integer multiple of TWK_QTAUDIOOUTPUT_PERIOD_TIME. Find the multiple with the lowest measure average av sync values (i.e. the average of at least 25 av sync measurements).

The table below provides the smallest values of TWK_QTAUDIOOUTPUT_BUFFER_TIME and TWK_QTAUDIOOUTPUT_PERIOD_TIME that prevents buffer underruns (i.e. audio corruption/static issues), minimizes the lag at play-start and average av sync. Note the default value for buffer time is 120000 and period time is 20000.

| Hardware class   | Audio Hardware                  | OS Machine Specs                                                                                    | BUFFER_TIME  | PERIOD_TIME  | Avrg AV sync |
| ---------------- | ------------------------------- | --------------------------------------------------------------------------------------------------- | ------------ | ------------ | ------------ |
| Gaming machine   | onboard intel HDA/realtek 7.1ch | Centos7.2, Asus Rampage Gene IV, iCore7 3.5Ghz, 32GB 2400Mhz RAM, SSD, Quadro K2200 (nv drv 352.55) | 36000        | 6000         | ~3ms         |
| HP workstation   | onboard intel HDA/realtek 2ch   | Centos6.6, HPZ820, Xeon 12core, 48GB RAM, SSD, Quadro K6000 (nv 352.63)                             | TDB          | TDB          | TDB          |
| Dell workstation | onboard intel HDA/realtek 2ch   | Centos7.2                                                                                           | TDB          | TDB          | TDB          |
| Any              | USB Soundblaster XiFi           | Centos7.2, Asus Rampage Gene IV, iCore7 3.5Ghz, 32GB 2400Mhz RAM, SSD, Quadro K2200 (nv drv 352.55) | 120000       | 20000        | TDB          |
|                  |                                 |                                                                                                     |              |              |              |

Table K.2:

Some tuned configurations for 24fps on a 60Hz monitor.

### K.1 Suggestions for resolving audio static issues with PulseAudio for Linux

In this Appendix, we outline some suggestions for configuring your linux distribution's pulseaudio to address the issue of audio static during playback when RV sees heavy and continuous use within the context of a production environment. While this issue is intermittent and hard to reproduce, it can occur with sufficient frequency to become a support burden. Please note the suggestions here should be validated by your systems/video engineering dept before you adopt them.

Settings for /etc/pulse/default.pa.

```
# For RHEL7 equivalent systems
#
# fragments=2 (prevents problems on kvm and teradici host)
# fixed_latency_range=1 (fixes static problem when system is heavily loaded or in swap)
#
load-module module-alsa-card device_id=PCH format=s16le rate=48000 fragments=2 fixed_latency_range=1
```

NB: Many thanks to JayHillard/ChrisMihaly@WDAS from sharing these settings with us.
