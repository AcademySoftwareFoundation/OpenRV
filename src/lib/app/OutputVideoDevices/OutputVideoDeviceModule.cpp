//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <OutputVideoDevices/OutputVideoDeviceModule.h>
#include <OutputVideoDevices/OutputVideoDevice.h>
#include <TwkExc/Exception.h>
#include <TwkGLF/GLFBO.h>
#ifdef PLATFORM_DARWIN
#include <TwkGLF/GL.h>
#endif
#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#endif
#include <sstream>

namespace OutputVideoDevices {
using namespace std;

OutputVideoDeviceModule::OutputVideoDeviceModule(NativeDisplayPtr p)
    : VideoModule()
{
    open();
}

OutputVideoDeviceModule::~OutputVideoDeviceModule()
{
    close();
}

string
OutputVideoDeviceModule::name() const
{
    return "Output";
}

void
OutputVideoDeviceModule::open()
{
    if (isOpen()) return;

    //m_devices.push_back(new OutputVideoDeviceModule(this, "foo"));
}

void
OutputVideoDeviceModule::close()
{
    for (size_t i = 0; i < m_devices.size(); i++)
    {
        m_devices[i]->close();
        delete m_devices[i];
    }
    m_devices.clear();
}

bool
OutputVideoDeviceModule::isOpen() const
{
    return !m_devices.empty();
}

OutputVideoDevice*
OutputVideoDeviceModule::newDevice(const string& name, IPCore::IPGraph* graph)
{
    OutputVideoDevice* d = new OutputVideoDevice(this, name, graph);
    m_devices.push_back(d);
    return d;
}

} // OutputVideoDevices
