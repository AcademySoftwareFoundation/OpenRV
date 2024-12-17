#ifndef __TwkExc__dll_defs__h__
#define __TwkExc__dll_defs__h__
//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifdef WIN32
#if defined(TWKEXC_BUILD)
#define TWKEXC_EXPORT __declspec(dllexport)
#else
#define TWKEXC_EXPORT __declspec(dllimport)
#endif
#else
#define TWKEXC_EXPORT
#endif

#endif
