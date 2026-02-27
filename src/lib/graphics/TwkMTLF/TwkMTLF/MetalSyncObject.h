//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkMTLF__MetalSyncObject__h__
#define __TwkMTLF__MetalSyncObject__h__

#include <cstdint>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

namespace TwkMTLF
{

    //
    //  MetalSyncObject  —  Metal equivalent of TwkGLF::GLSyncObject / GLFence
    //
    //  Metal provides two synchronisation primitives:
    //
    //    * MTLEvent / MTLSharedEvent — for GPU↔GPU or GPU↔CPU signalling across
    //      command buffers or even across processes (shared events).
    //
    //    * MTLFence — lightweight intra-encoder/intra-pass resource tracking.
    //
    //  This class wraps MTLSharedEvent so it can be signalled from the GPU
    //  (via [encoder signalEvent:value:]) and waited on from the CPU
    //  (via [event notifyListener:atValue:block:]).
    //
    //  Typical usage (mirrors GLSyncObject::wait()):
    //
    //    MetalSyncObject sync(device);
    //    // ... encode GPU work into commandBuffer ...
    //    sync.signal(commandBuffer);   // GPU will signal when it reaches this point
    //    commandBuffer.commit();
    //    sync.wait();                  // CPU blocks until GPU signals
    //
    //  setFence() / waitFence() / testFence() names are provided as compatibility
    //  shims for call-sites ported from GLSyncObject.
    //

    class MetalSyncObject
    {
    public:
#ifdef __OBJC__
        explicit MetalSyncObject(id<MTLDevice> device);
#endif

        MetalSyncObject(); // creates an invalid (uninitialised) object

        MetalSyncObject(MetalSyncObject&& rhs) noexcept;
        MetalSyncObject& operator=(MetalSyncObject&& rhs) noexcept;

        ~MetalSyncObject();

        // Non-copyable
        MetalSyncObject(const MetalSyncObject&) = delete;
        MetalSyncObject& operator=(const MetalSyncObject&) = delete;

        // -------------------------------------------------------------------------
        //  GLSyncObject-compatible API
        // -------------------------------------------------------------------------

        //  Insert a signal command into commandBuffer at the current point in the
        //  command stream.  Increments the internal signal value.
        //  Equivalent to glFenceSync / glSetFenceAPPLE.
#ifdef __OBJC__
        void setFence(id<MTLCommandBuffer> commandBuffer);
#endif

        //  Remove (reset) the fence — resets internal bookkeeping so the object
        //  can be reused.
        void removeFence();

        //  Block the CPU until the previously signalled value is reached.
        //  Equivalent to glClientWaitSync / glFinishFenceAPPLE.
        void waitFence() const;

        //  Non-blocking test — returns true if the GPU has already passed the fence.
        bool testFence() const;

        //  tryTestFence: if fence is set, test it; otherwise return true.
        bool tryTestFence() const;

        //  Convenience: setFence() + waitFence() combined.
        //  NOTE: because setFence requires a commandBuffer, this overload takes one.
#ifdef __OBJC__
        void wait(id<MTLCommandBuffer> commandBuffer);
#endif

        // -------------------------------------------------------------------------
        //  Lower-level signal / wait with explicit values
        //
        //  The MTLSharedEvent counts monotonically.  Each signal() call records
        //  a new target value.  wait() blocks until the event value >= that target.
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        //  Encode a signal command into commandBuffer at value signalValue.
        void signal(id<MTLCommandBuffer> commandBuffer, uint64_t signalValue);

        //  Block the CPU until sharedEvent.signaledValue >= waitValue.
        //  timeoutMs == 0 means infinite timeout.
        bool wait(uint64_t waitValue, uint64_t timeoutMs = 0) const;

        id<MTLSharedEvent> sharedEvent() const { return m_event; }
#else
        void* sharedEvent() const { return m_event_opaque; }
#endif

        bool isValid() const;

    private:
        mutable bool m_fenceSet;
        uint64_t m_signalValue; // current target value

#ifdef __OBJC__
        id<MTLSharedEvent> m_event;
        MTLSharedEventListener* m_listener;
#else
        void* m_event_opaque;
        void* m_listener_opaque;
#endif
    };

} // namespace TwkMTLF

#endif // __TwkMTLF__MetalSyncObject__h__
