#ifndef __TwkMath__dll_defs__h__
#define __TwkMath__dll_defs__h__
//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifdef WIN32
#if defined(TWKMATH_BUILD)
#define TWKMATH_EXPORT __declspec(dllexport)
#else
#define TWKMATH_EXPORT __declspec(dllimport)
#endif
#else
#define TWKMATH_EXPORT
#endif

#endif
