//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
//  StereoVideoFrame.cpp
//  Signal Generator
//

#include <BlackMagicDevices/StereoVideoFrame.h>
#include <stdexcept>
#include <cstring>

#define CompareREFIID(iid1, iid2) (memcmp(&iid1, &iid2, sizeof(REFIID)) == 0)

StereoVideoFrame::Provider::Provider(IDeckLinkMutableVideoFrame* parent,
                                     IDeckLinkMutableVideoFrame* right)
    : m_parentFrame(parent)
    , m_rightFrame(right)
    , m_refCount(1)
{
    if (m_parentFrame == nullptr)
    {
        throw std::invalid_argument("At least a left frame should be defined");
    }
    if (m_rightFrame != nullptr)
    {
        m_rightFrame->AddRef();
    }
}

StereoVideoFrame::Provider::~Provider()
{
    if (m_rightFrame != nullptr)
    {
        m_rightFrame->Release();
    }
}

HRESULT StereoVideoFrame::Provider::QueryInterface(REFIID iid, LPVOID* ppv)
{
    return (new StereoVideoFrame(m_parentFrame, m_rightFrame))
        ->QueryInterface(iid, ppv);
}

ULONG StereoVideoFrame::Provider::AddRef()
{
#ifdef PLATFORM_WINDOWS
    return _InterlockedIncrement((volatile long*)&m_refCount);
#else
    return ++m_refCount;
#endif
}

ULONG StereoVideoFrame::Provider::Release()
{
#ifdef PLATFORM_WINDOWS
    ULONG newRefValue = _InterlockedDecrement((volatile long*)&m_refCount);
    if (!newRefValue)
        delete this;
    return newRefValue;
#else
    ULONG refCount = --m_refCount;
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
#endif
}

StereoVideoFrame::StereoVideoFrame(IDeckLinkMutableVideoFrame* owner,
                                   IDeckLinkMutableVideoFrame* right)
    : m_frameLeft(owner)
    , m_frameRight(right)
    , m_refCount(1)
{
    if (m_frameLeft == nullptr)
    {
        throw std::invalid_argument("At minimum a left frame must be defined");
    }

    m_frameLeft->AddRef();
    if (m_frameRight != nullptr)
    {
        m_frameRight->AddRef();
    }
}

StereoVideoFrame::~StereoVideoFrame()
{
    if (m_frameLeft != nullptr)
    {
        m_frameLeft->Release();
    }
    if (m_frameRight != nullptr)
    {
        m_frameRight->Release();
    }
}

HRESULT StereoVideoFrame::QueryInterface(REFIID iid, LPVOID* ppv)
{
#ifdef PLATFORM_DARWIN
    CFUUIDBytes iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
#else
    REFIID iunknown = IID_IUnknown;
#endif

    if (CompareREFIID(iid, iunknown)
        || CompareREFIID(iid, IID_IDeckLinkVideoFrame3DExtensions))
    {
        *ppv = static_cast<IDeckLinkVideoFrame3DExtensions*>(this);
        AddRef();
        return S_OK;
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG StereoVideoFrame::AddRef()
{
#ifdef PLATFORM_WINDOWS
    return _InterlockedIncrement((volatile long*)&m_refCount);
#else
    return ++m_refCount;
#endif
}

ULONG StereoVideoFrame::Release()
{
#ifdef PLATFORM_WINDOWS
    ULONG newRefValue = _InterlockedDecrement((volatile long*)&m_refCount);
    if (!newRefValue)
        delete this;
    return newRefValue;
#else
    ULONG refCount = --m_refCount;
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
#endif
}

BMDVideo3DPackingFormat StereoVideoFrame::Get3DPackingFormat()
{
    return bmdVideo3DPackingLeftOnly;
}

HRESULT
StereoVideoFrame::GetFrameForRightEye(IDeckLinkVideoFrame** rightEyeFrame)
{
    if (m_frameRight != nullptr)
    {
        return m_frameRight->QueryInterface(
            IID_IDeckLinkVideoFrame, reinterpret_cast<void**>(rightEyeFrame));
    }
    return S_FALSE;
}
