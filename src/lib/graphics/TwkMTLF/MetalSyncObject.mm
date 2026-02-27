//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#import <TwkMTLF/MetalSyncObject.h>

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <cassert>
#include <stdexcept>
#include <limits>
#include <dispatch/dispatch.h>

namespace TwkMTLF
{

// ---------------------------------------------------------------------------
//  Construction / destruction
// ---------------------------------------------------------------------------

MetalSyncObject::MetalSyncObject(id<MTLDevice> device)
    : m_fenceSet(false)
    , m_signalValue(0)
    , m_event(nil)
    , m_listener(nil)
{
    @autoreleasepool
    {
        if (!device)
        {
            throw std::invalid_argument("TwkMTLF::MetalSyncObject: device is nil");
        }

        m_event = [device newSharedEvent];
        if (!m_event)
        {
            throw std::runtime_error("TwkMTLF::MetalSyncObject: "
                                     "newSharedEvent returned nil.");
        }
        m_event.label = @"TwkMTLF::MetalSyncObject";

        // Create a listener on a dedicated serial dispatch queue.
        dispatch_queue_t q =
            dispatch_queue_create("com.autodesk.twkmtlf.sync",
                                  DISPATCH_QUEUE_SERIAL);
        m_listener = [[MTLSharedEventListener alloc] initWithDispatchQueue:q];
        // q is a local; ARC releases it automatically when it goes out of scope.
    }
}

MetalSyncObject::MetalSyncObject()
    : m_fenceSet(false)
    , m_signalValue(0)
    , m_event(nil)
    , m_listener(nil)
{
    // Invalid / uninitialised object.
}

MetalSyncObject::MetalSyncObject(MetalSyncObject&& rhs) noexcept
    : m_fenceSet(rhs.m_fenceSet)
    , m_signalValue(rhs.m_signalValue)
    , m_event(rhs.m_event)
    , m_listener(rhs.m_listener)
{
    rhs.m_fenceSet    = false;
    rhs.m_signalValue = 0;
    rhs.m_event       = nil;
    rhs.m_listener    = nil;
}

MetalSyncObject& MetalSyncObject::operator=(MetalSyncObject&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_fenceSet    = rhs.m_fenceSet;
        m_signalValue = rhs.m_signalValue;
        m_event       = rhs.m_event;
        m_listener    = rhs.m_listener;

        rhs.m_fenceSet    = false;
        rhs.m_signalValue = 0;
        rhs.m_event       = nil;
        rhs.m_listener    = nil;
    }
    return *this;
}

MetalSyncObject::~MetalSyncObject()
{
    m_listener = nil;
    m_event    = nil;
}

// ---------------------------------------------------------------------------
//  isValid
// ---------------------------------------------------------------------------

bool MetalSyncObject::isValid() const
{
    return m_event != nil;
}

// ---------------------------------------------------------------------------
//  GLSyncObject-compatible API
// ---------------------------------------------------------------------------

void MetalSyncObject::setFence(id<MTLCommandBuffer> commandBuffer)
{
    assert(!m_fenceSet && "MetalSyncObject::setFence called while fence already set");
    assert(m_event != nil);
    assert(commandBuffer != nil);

    m_signalValue++;
    [commandBuffer encodeSignalEvent:m_event value:m_signalValue];
    m_fenceSet = true;
}

void MetalSyncObject::removeFence()
{
    // Reset state so the object can be reused.  Does not affect already-
    // submitted GPU work; the GPU will still signal at m_signalValue, but
    // subsequent calls to waitFence() will be no-ops until setFence() is
    // called again.
    m_fenceSet = false;
}

void MetalSyncObject::waitFence() const
{
    if (!m_fenceSet)
        return;

    assert(m_event != nil);

    // Block this thread using a semaphore that will be signalled by the
    // MTLSharedEventListener when the GPU reaches m_signalValue.
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    const uint64_t targetValue = m_signalValue;

    [m_event notifyListener:m_listener
                    atValue:targetValue
                      block:^(id<MTLSharedEvent> /*event*/, uint64_t /*value*/) {
                          dispatch_semaphore_signal(sem);
                      }];

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    // sem is a local; ARC releases it automatically.

    m_fenceSet = false;
}

bool MetalSyncObject::testFence() const
{
    if (!m_fenceSet)
        return true;  // nothing to test

    assert(m_event != nil);

    bool signaled = (m_event.signaledValue >= m_signalValue);
    if (signaled)
    {
        m_fenceSet = false;
    }
    return signaled;
}

bool MetalSyncObject::tryTestFence() const
{
    if (!m_fenceSet)
        return true;
    return testFence();
}

void MetalSyncObject::wait(id<MTLCommandBuffer> commandBuffer)
{
    setFence(commandBuffer);
    [commandBuffer commit];
    waitFence();
}

// ---------------------------------------------------------------------------
//  Low-level signal / wait with explicit values
// ---------------------------------------------------------------------------

void MetalSyncObject::signal(id<MTLCommandBuffer> commandBuffer,
                              uint64_t signalValue)
{
    assert(m_event != nil);
    assert(commandBuffer != nil);
    [commandBuffer encodeSignalEvent:m_event value:signalValue];
}

bool MetalSyncObject::wait(uint64_t waitValue, uint64_t timeoutMs) const
{
    assert(m_event != nil);

    if (m_event.signaledValue >= waitValue)
        return true;

    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    [m_event notifyListener:m_listener
                    atValue:waitValue
                      block:^(id<MTLSharedEvent> /*event*/, uint64_t /*value*/) {
                          dispatch_semaphore_signal(sem);
                      }];

    dispatch_time_t timeout = (timeoutMs == 0)
        ? DISPATCH_TIME_FOREVER
        : dispatch_time(DISPATCH_TIME_NOW,
                        static_cast<int64_t>(timeoutMs) * 1000000LL);

    long result = dispatch_semaphore_wait(sem, timeout);
    // sem is a local; ARC releases it automatically.

    return result == 0; // 0 == success, non-zero == timeout
}

} // namespace TwkMTLF
