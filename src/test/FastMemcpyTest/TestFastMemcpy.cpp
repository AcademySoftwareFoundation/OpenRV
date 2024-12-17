//*****************************************************************************/
//
// Filename: FastMemcpy.h
//
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#include <cstdio>
#include <stdint.h>

#include <TwkUtil/Timer.h>
#include <TwkFB/TwkFBThreadPool.h>
#include <TwkFB/FastMemcpy.h>

void TestFastMemcpy()
{
    using namespace TwkUtil;

    printf("Test TestPxlMemcpy\n");

    TwkFB::ThreadPool::initialize();

    const size_t bufferSize = 7680 * 4320 * 4 * 2;
    const size_t tryCount = 100;

    uint8_t* srcBuffer = new uint8_t[bufferSize];
    uint8_t* interBuffer = new uint8_t[bufferSize];
    uint8_t* dstBuffer = new uint8_t[bufferSize];

    Timer timer(true);
    for (size_t i = 0; i < tryCount; i++)
        FastMemcpy(dstBuffer, srcBuffer, bufferSize);
    printf("PxlMemcpy() for 8k frame:%f sec/frame\n",
           timer.elapsed() / tryCount);

    Timer timer1(true);
    for (size_t i = 0; i < tryCount; i++)
        FastMemcpy_MP(dstBuffer, srcBuffer, bufferSize);
    printf("PxlMemcpy_MP() for 8k frame:%f sec/frame\n",
           timer1.elapsed() / tryCount);

    delete[] srcBuffer;
    delete[] interBuffer;
    delete[] dstBuffer;

    TwkFB::ThreadPool::shutdown();
}
