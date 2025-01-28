//*****************************************************************************
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#include <TwkUtil/sgcHopTools.h>

#include <TwkUtil/sgcHop.h>

void HOP_PROF_FUNCTION_PTR(const char* functionName,
                           void (*functionPtr)(void**, void*), void** params,
                           void* ret)
{
    HOP_PROF(functionName);
    functionPtr(params, ret);
}
