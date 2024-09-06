//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <iostream>
#include <TwkApp/VideoModule.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GL.h>
#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglew.h>
#endif

namespace AJADevices
{

  class AJAModule : public TwkApp::VideoModule
  {
   public:
    enum class OperationMode
    {
      ProMode,
      SimpleMode
    };

    AJAModule( NativeDisplayPtr, unsigned int app4CC, OperationMode );
    virtual ~AJAModule();

    virtual std::string name() const;
    virtual std::string SDKIdentifier() const;
    virtual std::string SDKInfo() const;
    virtual void open();
    virtual void close();
    virtual bool isOpen() const;

   private:
    OperationMode m_mode{ OperationMode::ProMode };
    unsigned int m_appID{ 0 };
  };

}  // namespace AJADevices
