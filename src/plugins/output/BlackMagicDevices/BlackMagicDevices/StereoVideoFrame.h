//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//  StereoVideoFrame.h
//  Signal Generator
//

#pragma once

#include <DeckLinkAPI.h>

#ifdef PLATFORM_WINDOWS
typedef __int32 int32_t;
#endif

#include <boost/thread.hpp>
#include <atomic>

/*
 * An example class which may be used to output a frame or pair of frames to
 * a 3D capable output.
 *
 * This class implements the IDeckLinkVideoFrame3DExtensions interface which can
 * be used to operate on the left frame following the BMD provider pattern
 *
 * The Provider class manages the relationship between the video frame and
 * the 3D extensions, and is associated with the video frame using
 * SetInterfaceProvider().
 *
 * Access to the right frame through the IDeckLinkVideoFrame3DExtensions
 * interface:
 *
 * IDeckLinkVideoFrame3DExtensions *threeDimensionalFrame;
 * result = leftEyeFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions,
 * reinterpret_cast<void**>(&threeDimensionalFrame);
 * result = threeDimensionalFrame->GetFrameForRightEye(&rightEyeFrame);
 *
 * After which IDeckLinkVideoFrame operations are performed directly
 * on the rightEyeFrame object.
 */

class StereoVideoFrame : public IDeckLinkVideoFrame3DExtensions
{
public:
    using ScopedLock = boost::mutex::scoped_lock;
    using Mutex = boost::mutex;
    using Condition = boost::condition_variable;

    virtual ~StereoVideoFrame();

    StereoVideoFrame(const StereoVideoFrame&) = delete;
    StereoVideoFrame& operator=(const StereoVideoFrame&) = delete;
    StereoVideoFrame(StereoVideoFrame&&) = delete;
    StereoVideoFrame& operator=(StereoVideoFrame&&) = delete;

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDeckLinkVideoFrame3DExtensions methods
    BMDVideo3DPackingFormat STDMETHODCALLTYPE Get3DPackingFormat() override;
    HRESULT STDMETHODCALLTYPE
    GetFrameForRightEye(/* out */ IDeckLinkVideoFrame** rightEyeFrame) override;

    class Provider : public IUnknown
    {
    public:
        Provider(IDeckLinkMutableVideoFrame* parent,
                 IDeckLinkMutableVideoFrame* right);
        virtual ~Provider();

        Provider(const Provider&) = delete;
        Provider& operator=(const Provider&) = delete;
        Provider(Provider&&) = delete;
        Provider& operator=(Provider&&) = delete;

        // IUnknown methods
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
                                                 LPVOID* ppv) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        IDeckLinkVideoFrame* GetLeftFrame() const { return m_parentFrame; }

    private:
        IDeckLinkMutableVideoFrame* m_parentFrame;
        IDeckLinkMutableVideoFrame* m_rightFrame;
        std::atomic<ULONG> m_refCount;
    };

private:
    friend class Provider;

    StereoVideoFrame(IDeckLinkMutableVideoFrame* owner,
                     IDeckLinkMutableVideoFrame* right);

    IDeckLinkMutableVideoFrame* m_frameLeft;
    IDeckLinkMutableVideoFrame* m_frameRight;
    std::atomic<ULONG> m_refCount;
};
