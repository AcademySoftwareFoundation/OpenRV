//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkApp__VideoModule__h__
#define __TwkApp__VideoModule__h__
#include <iostream>
#include <string>
#include <vector>

namespace TwkApp
{
    class VideoDevice;

    //
    //  VideoModule
    //
    //  This is basically a factory for VideoDevice objects.
    //

    class VideoModule
    {
    public:
        typedef std::vector<VideoDevice*> VideoDevices;
        typedef std::vector<std::string> StringVector;
        typedef void* NativeDisplayPtr;

        VideoModule(NativeDisplayPtr p = NULL);
        virtual ~VideoModule();

        virtual std::string name() const;

        //
        //  SDKIdentifier: report a version string for any underlying SDK
        //  if there is any. For example "ACME SDK 12.2.1 2015-06-30". This
        //  should use whatever version macros are available from the SDK
        //  to automatically report -- don't hardcode this stuff.
        //
        //  SDKInfo: additional info like "Recommended ACME driver: 12.2"
        //
        //  Either one of these can return "" which is the default
        //

        virtual std::string SDKIdentifier() const;
        virtual std::string SDKInfo() const;

        //
        //  Before calling anything below this comment, you need to call
        //  open() on the module. Its expected that open() will fill
        //  m_devices with whatever devices are available from this
        //  module.
        //

        virtual void open();
        virtual void close();
        virtual bool isOpen() const;
        virtual const VideoDevices& devices() const;

        //
        //  Find the device at the absolute position. This is used by
        //  DesktopVideoModules when multiple screens are treated as one
        //  desktop.
        //

        virtual VideoDevice* deviceFromPosition(int x, int y) const;

    protected:
        VideoDevices m_devices;
    };

} // namespace TwkApp

#endif // __TwkApp__VideoModule__h__
