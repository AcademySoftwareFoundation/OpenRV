//
//  Copyright (c) 2011 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GL.h>

namespace TwkGLF {
using namespace std;
using namespace TwkApp;

GLVideoDevice::GLVideoDevice(VideoModule* m, const string& name, unsigned int capabilities)
    : VideoDevice(m, name, capabilities),
      m_textContext(TwkGLText::GLtext::newContext()),
      m_textContextOwner(true),
      m_fbo(0)
{
}

GLVideoDevice::~GLVideoDevice()
{
    if (m_textContextOwner)
    {
        TwkGLText::GLtext::deleteContext(m_textContext);
    }

    m_textContext = (void*)0xdeadc0de;
    m_textContextOwner = false;
    delete m_fbo;
}

GLVideoDevice*
GLVideoDevice::newSharedContextWorkerDevice() const
{
    return 0;
}

void
GLVideoDevice::setTextContext(TwkGLText::Context c, bool shared)
{
    if (m_textContext && m_textContextOwner)
    {
        TwkGLText::GLtext::deleteContext(m_textContext);
    }

    m_textContext = c;
    m_textContextOwner = !shared;
    TwkGLText::GLtext::setContext(c);
}

void
GLVideoDevice::makeCurrent() const
{
    TwkGLText::GLtext::setContext(m_textContext);
}

void
GLVideoDevice::makeCurrent(TwkFB::FrameBuffer* target) const
{
    makeCurrent();
}

void 
GLVideoDevice::clearCaches() const
{
    makeCurrent();
    TwkGLText::GLtext::clear();
}

void GLVideoDevice::bind() const { }
void GLVideoDevice::unbind() const {}
void GLVideoDevice::redraw() const { }
void GLVideoDevice::redrawImmediately() const { }


VideoDevice::Resolution 
GLVideoDevice::resolution() const
{
    return Resolution(width(), height(), 1.0, 1.0);
}

VideoDevice::Offset 
GLVideoDevice::offset() const
{
    return Offset(0, 0);
}

VideoDevice::Timing 
GLVideoDevice::timing() const
{
    return Timing(0.0);
}

VideoDevice::VideoFormat
GLVideoDevice::format() const
{
    return VideoFormat(width(),
                       height(),
                       1.0,
                       1.0,
                       0.0,
                       hardwareIdentification());
}

void GLVideoDevice::open(const StringVector& args) { } 
void GLVideoDevice::close() { } 
bool GLVideoDevice::isOpen() const { return true; }
bool GLVideoDevice::isQuadBuffer() const { return false; }
bool GLVideoDevice::sharesWith(const GLVideoDevice*) const { return true; }

GLFBO*
GLVideoDevice::defaultFBO() 
{
    if (!m_fbo) m_fbo = new GLFBO(this);
    return m_fbo;
}

const GLFBO*
GLVideoDevice::defaultFBO() const
{
    if (!m_fbo) m_fbo = new GLFBO(this);
    return m_fbo;
}

//----------------------------------------------------------------------

GLBindableVideoDevice::GLBindableVideoDevice(VideoModule* m, const std::string& name, unsigned int capabilities)
    : VideoDevice(m, name, capabilities) {}
GLBindableVideoDevice::~GLBindableVideoDevice() {}

bool GLBindableVideoDevice::willBlockOnTransfer() const { return false; }
void GLBindableVideoDevice::unbind() const { }
void GLBindableVideoDevice::bind(const GLVideoDevice*) const { }
void GLBindableVideoDevice::bind2(const GLVideoDevice*, const GLVideoDevice*) const { }
void GLBindableVideoDevice::transfer(const GLFBO*) const { }
void GLBindableVideoDevice::transfer2(const GLFBO*, const GLFBO*) const { }
bool GLBindableVideoDevice::readyForTransfer() const { return true; }

VideoDevice::Resolution 
GLBindableVideoDevice::resolution() const
{
    return Resolution(width(), height(), 1.0, 1.0);
}

VideoDevice::Offset 
GLBindableVideoDevice::offset() const
{
    return Offset(0, 0);
}

VideoDevice::Timing 
GLBindableVideoDevice::timing() const
{
    return Timing(0.0);
}

VideoDevice::VideoFormat 
GLBindableVideoDevice::format() const
{
    return VideoFormat(width(),
                       height(),
                       1.0,
                       1.0,
                       0.0,
                       hardwareIdentification());
}

void GLBindableVideoDevice::open(const StringVector&) { } 
void GLBindableVideoDevice::close() { } 
bool GLBindableVideoDevice::isOpen() const { return true; }

GLenum 
internalFormatFromDataFormat(VideoDevice::InternalDataFormat f)
{
    switch (f)
    {
      case VideoDevice::RGB8:    return GL_RGB8;
      case VideoDevice::RGBA8:   
      case VideoDevice::BGRA8:   return GL_RGBA8;
      case VideoDevice::RGB16:   return GL_RGB16;
      case VideoDevice::RGBA16:  return GL_RGBA16;
      case VideoDevice::RGB10X2Rev: return GL_RGB10_A2;
      case VideoDevice::RGB10X2: return GL_RGB10_A2;
      case VideoDevice::RGB16F:  return GL_RGB16F_ARB;
      case VideoDevice::RGBA16F: return GL_RGBA16F_ARB;
      case VideoDevice::RGB32F:  return GL_RGB32F_ARB;
      case VideoDevice::RGBA32F: return GL_RGBA32F_ARB;

      case VideoDevice::CbY0CrY1_8_422: return GL_RGB8;
      case VideoDevice::Y0CbY1Cr_8_422: return GL_RGB8;
      case VideoDevice::Y1CbY0Cr_8_422: return GL_RGB8;
      case VideoDevice::YCrCb_AJA_10_422: return GL_RGB10_A2;
      case VideoDevice::YCrCb_BM_10_422: return GL_RGB10_A2;
      case VideoDevice::YCbCr_P216_16_422: return GL_RGB10_A2;
          
      default:
          return GL_RGBA16F_ARB;
          //case VideoDevice::YCrCb8_444:
          //case VideoDevice::YCrCb8_422:
          //case VideoDevice::YCrCb10_444:
          //case VideoDevice::YCrCbA10_444:
          //case VideoDevice::YCrCb10_422:
    }
}

GLenumPair
textureFormatFromDataFormat(VideoDevice::InternalDataFormat f)
{
    switch (f)
    {
      case VideoDevice::RGB8:    return GLenumPair(GL_RGB, GL_UNSIGNED_BYTE);
      case VideoDevice::RGBA8:   return GLenumPair(GL_RGBA, GL_UNSIGNED_BYTE);
      case VideoDevice::BGRA8:   return GLenumPair(GL_BGRA, GL_UNSIGNED_BYTE);
      case VideoDevice::RGB16:   return GLenumPair(GL_RGB, GL_UNSIGNED_SHORT);
      case VideoDevice::RGBA16:  return GLenumPair(GL_RGBA, GL_UNSIGNED_SHORT);
      case VideoDevice::RGB10X2: return GLenumPair(GL_RGBA, GL_UNSIGNED_INT_10_10_10_2);
      case VideoDevice::RGB10X2Rev: return GLenumPair(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
      case VideoDevice::RGB16F:  return GLenumPair(GL_RGB, GL_HALF_FLOAT_ARB);
      case VideoDevice::RGBA16F: return GLenumPair(GL_RGBA, GL_HALF_FLOAT_ARB);
      case VideoDevice::RGB32F:  return GLenumPair(GL_RGB, GL_FLOAT);
      case VideoDevice::RGBA32F: return GLenumPair(GL_RGBA, GL_FLOAT);

      case VideoDevice::CbY0CrY1_8_422:
      case VideoDevice::Y0CbY1Cr_8_422:
      case VideoDevice::Y1CbY0Cr_8_422: return GLenumPair(GL_RGB, GL_UNSIGNED_BYTE);
      case VideoDevice::YCrCb_AJA_10_422:
      case VideoDevice::YCrCb_BM_10_422: 
      case VideoDevice::YCbCr_P216_16_422: return GLenumPair(GL_RGBA, GL_UNSIGNED_INT_10_10_10_2);
          
      default:
          return GLenumPair(GL_RGBA, GL_HALF_FLOAT_ARB);
          //case VideoDevice::YCrCb8_444:
          //case VideoDevice::YCrCb8_422:
          //case VideoDevice::YCrCb10_444:
          //case VideoDevice::YCrCbA10_444:
          //case VideoDevice::YCrCb10_422:
    }
}

size_t
pixelSizeFromTextureFormat(GLenum format, GLenum type)
{
    // given the format such as GL_RGB, and data type such as GL_FLOAT
    // compute the pixelSize
    size_t size = 1;
    switch (format)
    {
        case GL_RGB:
            size *= 3; break;
        case GL_RGBA:
        case GL_BGRA:
            size *= 4; break;
        case GL_LUMINANCE_ALPHA:
            size *= 2; break;
        case GL_LUMINANCE:
        default: break;
    }
    switch (type)
    {
        case GL_UNSIGNED_INT_10_10_10_2:
        case GL_UNSIGNED_INT_2_10_10_10_REV:
            size = 4; break;
        case GL_FLOAT:
            size *= 4; break;
        case GL_UNSIGNED_SHORT:
        case GL_HALF_FLOAT_ARB:
            size *= 2; break;
        case GL_UNSIGNED_BYTE:   
        case GL_UNSIGNED_INT_8_8_8_8_REV:
        default: break;
    }
    return size;
}

    
} // TwkGLF
