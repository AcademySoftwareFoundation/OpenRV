//*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkFB__TwkFBThreadPool__h__
#define __TwkFB__TwkFBThreadPool__h__

#include <TwkFB/dll_defs.h>
#include "IlmThreadPool.h"
#include <stddef.h> // for size_t

namespace TwkFB
{
    namespace ThreadPool
    {

        TWKFB_EXPORT void initialize();
        TWKFB_EXPORT void shutdown();
        TWKFB_EXPORT size_t getNumThreads();
        TWKFB_EXPORT void addTask(ILMTHREAD_NAMESPACE::Task* task);

    } // namespace ThreadPool
} // namespace TwkFB

#endif //__TwkFB__TwkFBThreadPool__h__
