#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
ADD_SUBDIRECTORY(TwkAudio)
ADD_SUBDIRECTORY(QTAudioRenderer)
IF(RV_TARGET_LINUX)
  ADD_SUBDIRECTORY(ALSASafeAudioModule)
  ADD_SUBDIRECTORY(ALSAAudioModule)
ENDIF()
