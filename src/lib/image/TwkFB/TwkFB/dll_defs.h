//*****************************************************************************/
// Copyright (c) 2022 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#pragma once

#ifdef WIN32
#if defined(TWKFB_BUILD)
#define TWKFB_EXPORT __declspec(dllexport)
#else
#define TWKFB_EXPORT __declspec(dllimport)
#endif
#else
#define TWKFB_EXPORT
#endif
