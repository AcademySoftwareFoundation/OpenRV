//
//  Copyright (c) 2011 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __OutputVideoDevices__OutputVideoDeviceModule__h__
#define __OutputVideoDevices__OutputVideoDeviceModule__h__
#include <iostream>
#include <TwkApp/VideoModule.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GL.h>

namespace IPCore {
class IPGraph;
}


namespace OutputVideoDevices {
class OutputVideoDevice;

class OutputVideoDeviceModule : public TwkApp::VideoModule
{
  public:
    OutputVideoDeviceModule(NativeDisplayPtr);
    virtual ~OutputVideoDeviceModule();

    virtual std::string name() const;
    virtual void open();
    virtual void close();
    virtual bool isOpen() const;

    OutputVideoDevice* newDevice(const std::string& name, IPCore::IPGraph* graph);
};

} // OutputVideoDeviceDevices

#endif // __OutputVideoDevices__OutputVideoDeviceModule__h__
