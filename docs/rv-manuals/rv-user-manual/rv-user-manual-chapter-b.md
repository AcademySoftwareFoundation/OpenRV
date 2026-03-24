# B - Stereo Setup

### B.1 Linux

This is taken from an NVIDIA README file. The portions pertaining to stereo modes are reproduced here:

The following driver options are supported by the NVIDIA X driver. They may be specified either in the Screen or Device sections of the X config file.

#### Option "Stereo" "integer"

Enable offering of quad-buffered stereo visuals on Quadro. Integer indicates the type of stereo glasses being used

| Value     | Equipment                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| --------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1         | DDC glasses. The sync signal is sent to the glasses via the DDC signal to the monitor. These usually involve a passthrough cable between the monitor and video card                                                                                                                                                                                                                                                                                                                           |
| 2         | "Blueline" glasses. These usually involve a passthrough cable between the monitor and video card. The glasses know which eye to display based on the length of a blue line visible at the bottom of the screen. When in this mode, the root window dimensions are one pixel shorter in the Y dimension than requested. This mode does not work with virtual root window sizes larger than the visible root window size (desktop panning).                                                     |
| 3         | Onboard stereo support. This is usually only found on professional cards. The glasses connect via a DIN connector on the back of the video card.                                                                                                                                                                                                                                                                                                                                              |
| 4         | TwinView clone mode stereo (aka "passive" stereo). On video cards that support TwinView, the left eye is displayed on the first display, and the right eye is displayed on the second display. This is normally used in conjuction with special projectors to produce 2 polarized images which are then viewed with polarized glasses. To use this stereo mode, you must also configure TwinView in clone mode with the same resolution, panning offset, and panning domains on each display. |

Stereo is only available on Quadro cards. Stereo options 1, 2, and 3 (aka "active" stereo) may be used with TwinView if all modes within each metamode have identical timing values. Please see Appendix J for suggestions on making sure the modes within your metamodes are identical. The identical modeline requirement is not necessary for Stereo option 4 ("passive" stereo). Currently, stereo operation may be "quirky" on the original Quadro (NV10) chip and left-right flipping may be erratic. We are trying to resolve this issue for a future release. Default: Stereo is not enabled.

UBB must be enabled when stereo is enabled (this is the default behavior).

Stereo options 1, 2, and 3 (aka "active" stereo) are not supported on digital flat panels.

#### Option "AllowDFPStereo" "boolean"

By default, the NVIDIA X driver performs a check which turns off active stereo (stereo options 1, 2, and 3) if the X screen is driving a DFP. The "AllowDFPStereo" option bypasses this check.

ENSURING IDENTICAL MODE TIMINGS

Some functionality, such as Active Stereo with TwinView, requires control over exactly what mode timings are used. There are several ways to accomplish that:

If you only want to make sure that both display devices use the same modes, you only need to make sure that both display devices use the same HorizSync and VertRefresh values when performing mode validation; this would be done by making sure the HorizSync and SecondMonitorHorizSync match, and that the VertRefresh and the SecondMonitorVertRefresh match.

A more explicit approach is to specify the modeline you wish to use (using one of the modeline generators available), and using a unique name. For example, if you wanted to use 1024x768 at 120 Hz on each monitor in TwinView with active stereo, you might add something like: # 1024x768 @ 120.00 Hz (GTF) hsync: 98.76 kHz; pclk: 139.05 MHz Modeline "1024x768_120" 139.05 1024 1104 1216 1408 768 769 772 823 -HSync +Vsync In the monitor section of your X config file, and then in the Screen section of your X config file, specify a MetaMode like this: Option "MetaModes" "1024x768_120, 1024x768_120"

#### Support for GLX in Xinerama

This driver supports GLX when Xinerama is enabled on similar GPUs. The Xinerama extension takes multiple physical X screens (possibly spanning multiple GPUs), and binds them into one logical X screen. This allows windows to be dragged between GPUs and to span across multiple GPUs. The NVIDIA driver supports hardware accelerated OpenGL rendering across all NVIDIA GPUs when Xinerama is enabled.

To configure Xinerama: configure multiple X screens (please refer to the XF86Config(5x) or xorg.conf(5x) manpages for details). The Xinerama extension can be enabled by adding the line

Option "Xinerama" "True"

to the "ServerFlags" section of your X config file.

Requirements:

It is recommended to use identical GPUs. Some combinations of non-identical, but similar, GPUs are supported. If a GPU is incompatible with the rest of a Xinerama desktop then no OpenGL rendering will appear on the screens driven by that GPU. Rendering will still appear normally on screens connected to other supported GPUs. In this situation the X log file will include a message of the form:

(WW) NVIDIA(2): The GPU driving screen 2 is incompatible with the rest of (WW) NVIDIA(2): the GPUs composing the desktop. OpenGL rendering will (WW) NVIDIA(2): be turned off on screen 2.

The NVIDIA X driver must be used for all X screens in the server.

Only the intersection of capabilities across all GPUs will be advertised.

X configuration options that affect GLX operation (e.g.: stereo, overlays) should be set consistently across all X screens in the X server.

### B.2 macOS and Windows

There are no special requirements (other than having a proper GPU that can produce stereo output). If the macOS or Windows graphical environment can provide RV with a stereo GL context, it will play back in stereo. If not, the console widget will pop up and you will see GL errors.

If you have trouble with the stereo on the Mac, you might have some luck on one of Apple's mailing lists.
