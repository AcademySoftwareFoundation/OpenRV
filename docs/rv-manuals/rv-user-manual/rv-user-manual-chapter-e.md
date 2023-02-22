# E - Open RV Audio on Linux

### E.1 Overview

RV provides multiple audio modules so users can get the most audio functionality available given the constraints imposed by the Linux kernel version, the available audio frameworks, and the other audio applications in use.

Potential user issues with audio on Linux are choppy playback (clicking, dead spots, drop-outs), or high latency (it takes a breath's worth of time to start playing). Both of these issues are extremely annoying and it is even worse when they both occur. Ideally latency is nearly 0 for high quality audio and drop-outs never occur.

To make matters worse, there are two completely different audio drivers for Linux: OSS and ALSA. Only one of these can be present at a time (but both provide a \`\`compatibility'' API for the other). ALSA is the official Linux audio API and is shipped with almost all distributions. OSS is a cross platform API (for UNIX based machines).

The Linux desktop projects, KDE and Gnome, both have sound servers build on top of OSS/ALSA. These are called esd and arts. Both of them have become deprecated in recent releases. If either of these processes is running you may find it difficult to use audio with RV (or other commercial products).

Some distributions use a newer desktop sound server called PulseAudio . PulseAudio ships with most distributions and is on by default in RHEL, CentOS, and Fedora, and comes with Debian and Ubuntu. RV can use PulseAudio through the ALSA (Safe) module, however we strongly recommend against using PulseAudio. Though the PulseAudio server is designed to allow multiple applications to simultaneously play audio (and can run in real-time mode), we have found a decrease in stability with each new release of the module. Even for simple read-only API calls that query the state of the device. If possible for your workflow we suggest you remove PulseAudio, if it is not otherwise necessary.

#### E.1.1 How Open RV Handles Linux Audio

RV has two ALSA audio back ends for Linux: the old and safe versions. The old version is meant to run on distributions which shipped with ALSA versions 1.0.13 or earlier and which do not have PulseAudio installed. The safe version should work well with PulseAudio systems. Unless you know that your system is using PulseAudio you should try the ALSA Old back end to start with.

On some systems, RV will fail to load one or more of its audio modules. This can occur of the API used by the module is newer than the one installed on the system. For example neither CentOS 4.6 or Fedora Core 4 can load the ALSA safe module unless the ALSA client library is upgraded. Any Linux system which uses ALSA 1.0 or newer should function with the ALSA old back end.

### E.2 ALSA (Pre-1.0.14)

The ALSA (Old) audio back uses a subset of the ALSA API. Without modification, it will make hardware devices directly accessible. (In ALSA terms these are the hw:x,y devices). You can also add to the list of devices using an environment variable called RV_ALSA_EXTRA_DEVICES. The syntax is:

```
visiblename1@alsadevicename1|visiblename2@alsadevicename2|...
```

So for example to add the “dmix” device, set the variable to: dmix@dmix.

See the ALSA documentation for the .asoundrc file for more information about devices and how they can be created. Devices defined in the configuration files will not automatically show up in RV; you'll need to add them to the environment variable.

When using the hardware devices other programs will not have access to the audio.

### E.3 ALSA (Safe)

The term “Safe” here refers to the subset of the ALSA API deemed safe to use in the presence of PulseAudio. Programs that use features that PulseAudio cannot intercept will cause inconsistent audio availability or unstable output. Use of hardware devices in the presence of PulseAudio can prevent other applications from using the audio driver whether they be ALSA or OSS based.

The ALSA (Safe) audio back end uses a more modern version of the ALSA client API. If the version of ALSA client library on your system is newer than 1.0.13 this back end might work better on your system. PulseAudio system should definitely be using this audio module.

The ALSA safe back end can use the RV_ALSA_EXTRA_DEVICES variable (see above). However, user/system defined devices will typically show up in the device list automatically.

The safe back end does not give you direct access to the hardware devices by default. Typically you will see devices which look like:

The xxxx and n values correspond to the hardware device values used in the ALSA old back end. You can force the use of the hardware devices by adding them to the environment variable.

```
front:CARD=xxxx,DEV=n
```

For example, the first device on the first card would be hw:0,0 in ALSA parlance. So setting the value of RV_ALSA_EXTRA_DEVICES to:

First Hardware Device@hw:0,0

would add that to the list.

We recommend that you do not add hardware devices when using the safe ALSA module. The hardware devices will typically be accessible via the default device list under different names (like front:CARD=xxxx,DEV=n from above). Unlike the default list of devices, the hardware devices will shut out other software from using the audio even on systems using the PulseAudio sound server.

### E.4 Platform Audio

RV supports a cross platform audio module based on Qt audio (which on Linux is ALSA based). Note Platform Audio supports playback on multichannel audio devices.
