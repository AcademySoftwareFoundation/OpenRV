#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(rv_options)
INCLUDE(ProcessorCount)
PROCESSORCOUNT(_cpu_count)

# Switches
ADD_COMPILE_OPTIONS(
  -DTWK_NO_SGI_BYTE_ORDER
  -DTWK_LITTLE_ENDIAN
  -D__LITTLE_ENDIAN__
  -DTWK_NO_STD_MIN_MAX
  -D_WINDOWS
  -D_AMD64=1
  -D_WIN64=1
  -DWIN64=1
  -DWIN32=1
  -D_WIN32=1
  -D_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH=1
  -DGC_NOT_DLL
  -DTWK_USE_GLEW
  -DGLEW_STATIC
  -DIMPORT_GL32
  -DMINIZ_DLL=1
  -DZLIB_DLL=1
  -DZLIB_WINAPI=1
  -DOPENEXR_DLL=1
  -DIMATH_DLL=1
  -DPNG_USE_DLL=1
  -DCMS_DLL=1
  -DBOOST_ALL_NO_LIB=1
  -DBOOST_ALL_DYN_LINK=1
  -DOpenColorIO_SHARED=1
  -D_SCL_SECURE_NO_WARNINGS=1
  -J
  -D_CRT_SECURE_NO_DEPRECATE=1
  -DPLATFORM_OPENGL=1
  -DNOMINMAX
  -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
)

# Compiler options
ADD_COMPILE_OPTIONS(
  -bigobj
  -EHsc
  -favor:blend
  -fp:precise
  -FS
  -GR
  -Gy
  -nologo
  -Qfast_transcendentals
  -Zc:forScope
  -Zc:sizedDealloc-
  -Zi
)

# Increasing default stack size to 8MB which would be on par with Linux and most macOS versions. Visual Studio usually sets a 1MB default stack size which is
# quite lower than what's available on macOS or Linux
#
# This is mostly to get a pass on one of the Mu test that was failing on Windows. The Ackermann Mu test was failing on Windows. The test would pass with k=7 but
# fail with k=8 value (as designed for, see the ack.mu file). On Windows Visual Studio the default is 1 MB stack size whereas this is more around 8MB on macOS
# and Linux.
ADD_LINK_OPTIONS("/STACK:8388608")

# Enable parallel builds Note that in theory we should be able to specify just /MP here but when we do cmake sets /MP1 instead. So in order to parellize the
# build, we must set the number of processors.
ADD_DEFINITIONS(/MP${_cpu_count})
