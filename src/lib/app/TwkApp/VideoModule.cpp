//
//  Copyright (c) 2011 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <TwkApp/VideoModule.h>

namespace TwkApp {
using namespace std;

VideoModule::VideoModule(NativeDisplayPtr)
{
}

VideoModule::~VideoModule()
{
}

std::string 
VideoModule::name() const
{
    return "UNNAMED";
}

string
VideoModule::SDKIdentifier() const
{
    return "";
}

string
VideoModule::SDKInfo() const
{
    return "";
}

void
VideoModule::open()
{
}

void
VideoModule::close()
{
}

bool
VideoModule::isOpen() const
{
    return false;
}

const VideoModule::VideoDevices&
VideoModule::devices() const
{
    return m_devices;
}

VideoDevice* 
VideoModule::deviceFromPosition(int x, int y) const
{
    return 0;
}

VideoModuleProxy::VideoModuleProxy(const string& name, const string& file)
    : VideoModule(),
      m_name(name),
      m_file(file),
      m_module(0)
{
}

VideoModuleProxy::~VideoModuleProxy()
{
}

std::string 
VideoModuleProxy::name() const
{
    if (m_module) return m_module->name();
    return m_name;
}

void
VideoModuleProxy::open()
{
    //
    // load the plugin and call its 
    //

    m_module->open();
}

void
VideoModuleProxy::close()
{
    m_module->close();
}

bool
VideoModuleProxy::isOpen() const
{
    return m_module->isOpen();
}

const VideoModule::VideoDevices&
VideoModuleProxy::devices() const
{
    return m_module->devices();
}


} // TwkApp
