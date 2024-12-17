//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkApp/VideoModule.h>

namespace TwkApp
{
    using namespace std;

    VideoModule::VideoModule(NativeDisplayPtr) {}

    VideoModule::~VideoModule() {}

    std::string VideoModule::name() const { return "UNNAMED"; }

    string VideoModule::SDKIdentifier() const { return ""; }

    string VideoModule::SDKInfo() const { return ""; }

    void VideoModule::open() {}

    void VideoModule::close() {}

    bool VideoModule::isOpen() const { return false; }

    const VideoModule::VideoDevices& VideoModule::devices() const
    {
        return m_devices;
    }

    VideoDevice* VideoModule::deviceFromPosition(int x, int y) const
    {
        return 0;
    }

} // namespace TwkApp
