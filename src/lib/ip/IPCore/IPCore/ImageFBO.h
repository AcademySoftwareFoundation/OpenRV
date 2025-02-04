//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ImageFBO__h__
#define __IPCore__ImageFBO__h__
#include <TwkGLF/GLFBO.h>
#include <IPCore/IPImage.h>
#include <IPCore/IPNode.h>
#include <boost/thread.hpp>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLFence.h>

namespace IPCore
{

    //
    //  ImageFBO holds an internally managed fbo or texture used during
    //  multi-pass rendering. For example, intermediate comps are
    //  rendered into these and immediately used as a texture for
    //  another a parent comp.
    //
    //  expectedUseCount is the number of times the FBO is expected to
    //  be used (e.g. first created by an image from a node with
    //  multiple outputs). This information should be used to
    //  determine how precious the FBO is in low-memory situations.
    //

    class ImageFBO
    {
    public:
        typedef TwkGLF::GLFBO GLFBO;

        ImageFBO(GLFBO* f, bool a = true, GLenum t = 0, GLenum fmt = 0,
                 GLenum tg = 0)
            : GLfbo(f)
            , available(a)
            , type(t)
            , format(fmt)
            , target(tg)
            , expectedUseCount(0)
            , fullSerialNum(0)
            , belongToDevice(0)
        {
        }

        // this is the type of FBO that belongs to device, are marked as such,
        // and cannot be grabbed by client rendering code.
        ImageFBO(size_t w, size_t h, GLenum iformat, bool belongToDevice = true)
            : available(0)
            , target(0)
            , type(0)
            , format(iformat)
            , expectedUseCount(0)
            , fullSerialNum(0)
        {
            GLfbo = new GLFBO(w, h, iformat);
        }

        GLFBO* fbo() { return GLfbo; }

        bool available;
        GLenum target; // texture target
        GLenum type;   // for textures
        GLenum format;
        std::string identifier;
        size_t expectedUseCount;
        size_t fullSerialNum;
        bool belongToDevice;

    private:
        GLFBO* GLfbo;
    };

    class ImageFBOManager
    {
        // the manager also keeps track of how much memory has been allocated in
        // FBO form
    public:
        typedef TwkGLF::GLFBO GLFBO;
        typedef std::vector<ImageFBO*> ImageFBOVector;
        typedef std::set<const GLFBO*> FBOSet;
        typedef TwkGLF::GLFence GLFence;
        typedef std::map<const size_t, GLFence*> GLFenceTextureMap;
        typedef boost::mutex::scoped_lock ScopedLock;

        // typedef std::map<const GLFBO*,GLFence*>    GLFenceFBOMap;

        ImageFBOManager()
            : m_totalSizeInBytes(0)
        {
        }

        static void setIntermediateLogging(bool b) { m_imageFBOLog = b; }

        // management functions
        void insertFBOFence(const GLFBO*);
        void waitForFBOFence(const GLFBO*);
        void deleteFBOFence(const GLFBO*);
        void insertTextureFence(const size_t);
        void waitForTextureFence(const size_t);
        void clearAllFences();

        void gcImageFBOs(size_t fullSerialNum);
        void releaseImageFBO(const GLFBO*);
        void flushImageFBOs();

        size_t totalSizeInBytes() const { return m_totalSizeInBytes; }

        ImageFBO* newOutputOnlyImageFBO(GLenum internalFormat, size_t w,
                                        size_t h, size_t samples = 0);

        ImageFBO* newImageFBO(const IPImage*, size_t fullSerialNum,
                              const std::string& identifier);

        ImageFBO* newImageFBO(const GLFBO*, size_t fullSerialNum,
                              const std::string& identifier);

        ImageFBO* newImageFBO(size_t w, size_t h, GLenum target,
                              GLenum internalFormat, GLenum format, GLenum type,
                              size_t fullSerialNum,
                              const std::string& identifier = "",
                              size_t expectedUseCount = 1, size_t samples = 0);

        ImageFBO* findExistingImageFBO(GLuint textureID);
        ImageFBO* findExistingImageFBO(const std::string&, size_t);
        ImageFBO* findExistingPaintFBO(const GLFBO*, const std::string&, bool&,
                                       size_t&, size_t);

    private:
        ImageFBOVector m_outputImageFBOs;
        GLFenceTextureMap m_textureFenceMap;
        // GLFenceFBOMap             m_fboFenceMap;
        boost::mutex m_textureFenceMutex;
        FBOSet m_fboSet;
        boost::mutex m_fboFenceMutex;
        ImageFBOVector m_imageFBOs;
        size_t m_totalSizeInBytes;
        static bool m_imageFBOLog;
    };

} // namespace IPCore

#endif // __IPCore__ImageRenderer__h__
