#ifndef __TwkUtil__dll_defs__h__
#define __TwkUtil__dll_defs__h__
//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifdef WIN32
#if defined(TWKUTIL_BUILD)
#define TWKUTIL_EXPORT __declspec(dllexport)
#else
#define TWKUTIL_EXPORT __declspec(dllimport)
#endif
#else
#define TWKUTIL_EXPORT
#endif

#endif
