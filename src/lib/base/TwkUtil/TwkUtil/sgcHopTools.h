//*****************************************************************************
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#ifndef SGC_HOP_TOOLS_H
#define SGC_HOP_TOOLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HOP_ENABLED
// Macro to add code only when HOP is enabled (eg.: HOP_CALL( glFinish(); ) )
#define HOP_CALL(x) x
#else
#define HOP_CALL(x)
#endif

    // Function to enable HOP before calling the function pointer.
    // This can be useful to add some HOP traces in C code.
    void HOP_PROF_FUNCTION_PTR(const char* functionName,
                               void (*functionPtr)(void**, void*),
                               void** params, void* ret);

#ifdef __cplusplus
}
#endif

#endif // SGC_HOP_TOOLS_H
