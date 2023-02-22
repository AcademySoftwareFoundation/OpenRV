# J - Supported Multichannel Audio Layouts

Multichannel audio devices are supported by RV audio output module choice “Platform Audio”. The “Platform Audio” choice is available on all RV platforms ie. OSX, Linux and Windows.

On OSX, you might need to enable the multichannel (e.g. 5.1) capability of your audio device using the OSX utility “Audio Midi Setup”.

The list of possible channel layouts that RV recognises is listed in the table below.

Table J.1:
|  |  |
| ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Channel Layout** | **Layout and Speaker Description** <br> **FL = Front Left** <br> **FR = Front Right** <br> **FC = Front Center** <br> **LF = Lower Frequency/Subwoofer** <br> **BL = Back Left** <br> **BR = Back Right** <br> **SL = Side Left** <br> **SR = Side Right** <br> **BC = Back Center** <br> **FLC = Front Left of Center** <br> **FRC = Front Right of Center** <br> **LH = Left Height** <br> **RH = Right Height** |
| Mono               | FC                                                                                                                                                                                                                                                                                                                                                             |
| Stereo             | FL:FR                                                                                                                                                                                                                                                                                                                                                          |
| 3.1                | FL:FR:LF                                                                                                                                                                                                                                                                                                                                                       |
| Quadrophonic       | FL:FR:BL:BR                                                                                                                                                                                                                                                                                                                                                    |
| 5.1                | FL:FR:FC:FL:SL:SR                                                                                                                                                                                                                                                                                                                                              |
| 5.1 (Back)         | FL:FR:FC:FL:BL:BR                                                                                                                                                                                                                                                                                                                                              |
| 5.1 (Swap)         | FL:FR:BL:BR:FC:FL                                                                                                                                                                                                                                                                                                                                              |
| 5.1 (AC3)          | FL:FC:FR:SL:SR:LF                                                                                                                                                                                                                                                                                                                                              |
| 5.1 (DTS)          | FC:FL:FR:SL:SR:LF                                                                                                                                                                                                                                                                                                                                              |
| 5.1 (AIFF)         | FL:BL:FC:FR:BR:LF                                                                                                                                                                                                                                                                                                                                              |
| 6.1                | FL:FR:FC:LF:BL:BR:BC                                                                                                                                                                                                                                                                                                                                           |
| 7.1 (SDDS)         | FL:FR:FC:LF:SL:SR:FLC:FRC                                                                                                                                                                                                                                                                                                                                      |
| 7.1                | FL:FR:FC:LF:SL:SR:BL:BR                                                                                                                                                                                                                                                                                                                                        |
| 7.1 (Back)         | FL:FR:FC:LF:BL:BR:SL:SR                                                                                                                                                                                                                                                                                                                                        |
| 9.1                | FL:FR:FC:LF:BL:BR:SL:SR:LH:RH                                                                                                                                                                                                                                                                                                                                  |
| 16                 |                                                                                                                                                                                                                                                                                                                                                                |


Supported Multichannel Layouts

Note that RV will mix down, mix up or reorder channels for any given media to match the intended output device channel layout format.

For example, playing back 5.1 media to a stereo audio device will see the 5.1 audio channels mixed downed to two channels.

Similarly, playing back stereo media to a 5.1 device will see the media's stereo FL and FR content mixed up to the 5.1 device's FL, FR and FC only.

For the case where the media and device have the same channel count and speaker types but different layout e.g. for 5.1 media and 5.1 (AC3) device, the media's channel layout is reordered to match the device channel layout when RV reads the media.

For the case where the media and device have the same channel count but non-matching channel/speaker types, the channel layout of media is passed to the device as is; for example 5.1 (Back) media and 5.1 device.

The audio channel layout for any given media can be determined from RV's image info tool.