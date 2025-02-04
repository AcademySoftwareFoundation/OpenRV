//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <IPCore/ImageRenderer.h>
#include <IPCore/Exception.h>
#include <IPCore/IPNode.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/Application.h>
#include <TwkExc/TwkExcException.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/BasicGLProgram.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <TwkFB/FastMemcpy.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Plane.h>
#include <TwkMath/QuadAlgo.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Frustum.h>
#include <TwkMovie/Movie.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/ThreadName.h>
#include <assert.h>
#include <half.h>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <IPCore/ImageRenderer.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPNode.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderState.h>
#include <limits>
#include <stdexcept>
#include <algorithm>

#ifdef PLATFORM_DARWIN
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#include <OpenCL/cl_gl.h>
#include <OpenCL/cl_gl_ext.h>
#endif

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#endif

//
//  NOTE: (2006 or so) PBOs are slight slower on OS X, but are better
//  behaved. There is some very odd behavior with client_storage_apple when
//  calling glTexSubImage2D(), none of which occur with PBOs.
//
//  UPDATE: (2013) apple forgot to inform that calling glTexSubImage2D with
//  client storage results in a memcpy or other fun stuff. Their docs at
//  some point were updated to include that piece of information. So we're
//  using them again!
//

extern const char* Histogram48k_cl;
extern const char* Histogram32k_cl;
extern const char* Histogram16k_cl;

namespace IPCore
{
    using namespace std;
    using namespace boost;
    using namespace TwkMath;
    using namespace TwkMovie;
    using namespace TwkFB;
    using namespace TwkUtil;
    using namespace TwkGLF;

    static ENVVAR_BOOL(evUsePBOs, "RV_RENDERING_USE_PBOS", true);
    static ENVVAR_INT(evMaxConcurrentPBOs, "RV_RENDERING_MAX_CONCURRENT_PBOS",
                      10);

#define NOT_A_FRAME (std::numeric_limits<int>::min())
#define NOT_A_COORDINATE (GLuint(-1))
#define NOT_A_TARGET (GLuint(-1))
#define NOT_A_TEXTURE_UNIT (GLuint(-1))

    typedef TwkContainer::StringProperty StringProperty;
    typedef TwkContainer::IntProperty IntProperty;

    TWK_CLASS_NEW_DELETE(ImageRenderer::TextureDescription);
    TWK_CLASS_NEW_DELETE(ImageRenderer::ImagePassState);

    namespace
    {

        void uploadThreadTrampoline(ImageRenderer* renderer)
        {
            //
            // this is the 'busy wait' upload thread
            // we use CPU sync to notify upload to do its work
            //
            setThreadName("PBO Upload");

            renderer->uploadThreadDevice()->makeCurrent();

            while (true)
            {
                // wait for render's signal
                renderer->waitToUpload();

                if (renderer->stopUploadThread())
                    break;

                renderer->prefetch(renderer->uploadRoot());

                //
                // fence is inserted after all upload gl commands, before we
                // notify renderer to draw currently one gl fence is inserted
                // per frame (NOTE not per framebuffer) theorectically we could
                // insert one fence per upload (framebuffer), and it might give
                // us better performance. however in practice the gain is
                // probably not significant.
                //
                renderer->manager()->insertTextureFence(
                    renderer->uploadRootHash());

                // signal renderer to proceed
                renderer->notifyDraw();
            }

            glFinish();
        }

    } // namespace

    ImageRenderer::BGPattern ImageRenderer::defaultBGPattern =
        ImageRenderer::Solid0;

    //
    //  Device
    //

    ImageRenderer::Device::Device(const VideoDevice* d, const GLVideoDevice* g,
                                  const GLBindableVideoDevice* bd,
                                  size_t ringBufferSize, size_t nviews)
        : device(d)
        , glDevice(g)
        , glBindableDevice(bd)
    {
        setRingBufferSize(ringBufferSize, nviews);
    }

    void ImageRenderer::Device::clearFBOs()
    {
        for (size_t i = 0; i < fboRingBuffer.size(); i++)
        {
            FBOVector& views = fboRingBuffer[i].views;

            for (size_t q = 0; q < views.size(); q++)
            {
                delete views[q];
                views[q] = 0;
            }
        }
    }

    void ImageRenderer::Device::setRingBufferSize(size_t s, size_t nviews)
    {
        clearFBOs();
        fboRingBuffer.resize(s);

        for (size_t i = 0; i < s; i++)
        {
            fboRingBuffer[i].views.resize(nviews);

            for (size_t q = 0; q < nviews; q++)
            {
                fboRingBuffer[i].views[q] = 0;
            }
        }
    }

    void ImageRenderer::Device::allocateFBOs(size_t w, size_t h, GLenum iformat,
                                             size_t vindex)
    {
        for (size_t i = 0; i < fboRingBuffer.size(); i++)
        {
            FBOVector& views = fboRingBuffer[i].views;

            assert(views[vindex] == 0);
            ImageFBO* ifbo = new ImageFBO(w, h, iformat);
            views[vindex] = ifbo->fbo();
            TWK_GLDEBUG;
        }
    }

    bool ImageRenderer::m_noGPUCache = false;
    bool ImageRenderer::m_imageFBODebug = false;
    bool ImageRenderer::m_passDebug = false;
    bool ImageRenderer::m_queryInit = true;
    bool ImageRenderer::m_floatBuffers = true;
    bool ImageRenderer::m_textureFloat = true;
    bool ImageRenderer::m_pixelBuffers = true;
    bool ImageRenderer::m_halfFloat = true;
    bool ImageRenderer::m_rectTextures = true;
    bool ImageRenderer::m_useTextures = true;
    bool ImageRenderer::m_hasClientStorage = true;
    bool ImageRenderer::m_useClientStorage = false;
    bool ImageRenderer::m_hasThreadedUpload = false;
    bool ImageRenderer::m_useThreadedUpload = false;
    bool ImageRenderer::m_imaging = true;
    bool ImageRenderer::m_nonPowerOf2 = false;
    int ImageRenderer::m_maxW = 2048;
    int ImageRenderer::m_maxH = 2048;
    int ImageRenderer::m_maxImageUnits = 0;
    int ImageRenderer::m_maxTexCoords = 0;
    int ImageRenderer::m_maxVertexAttrs = 0;
    bool ImageRenderer::m_defaultAllowPBOs = true;
    bool ImageRenderer::m_fragmentProgram = true;
    bool ImageRenderer::m_ycbcrApple = false;
    bool ImageRenderer::m_reportGL = false;
    bool ImageRenderer::m_softwareGLRenderer = false;
    int ImageRenderer::m_ALUinsnLimit = 0;
    int ImageRenderer::m_tempLimit = 0;
    bool ImageRenderer::m_floatFormats = false;

    ImageRenderer::FBAcceptableTypes ImageRenderer::m_FBAcceptableTypes;
    ImageRenderer::GetProcAddressFunc ImageRenderer::m_procFunc = 0;

#ifdef PLATFORM_DARWIN
#ifndef CL_VERSION_1_2
#define clCreateFromGLTexture clCreateFromGLTexture2D
#endif

    static void printCLError(int err)
    {
        //
        // this func should be moved out to CL specific files in the future
        // when we do more CL stuff. As of now, the only use case of CL is
        // histogram and all the related code is in this file. When we start to
        // use CL more we should do what we did for GL and have a library that
        // deals with CL stuff.
        //
        switch (err)
        {
        case CL_SUCCESS:
            break;
        case CL_DEVICE_NOT_FOUND:
            fprintf(stderr, "Device not found.\n");
            break;
        case CL_DEVICE_NOT_AVAILABLE:
            fprintf(stderr, "Device not available\n");
            break;
        case CL_COMPILER_NOT_AVAILABLE:
            fprintf(stderr, "Compiler not available\n");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            fprintf(stderr, "Memory object allocation failure\n");
            break;
        case CL_OUT_OF_RESOURCES:
            fprintf(stderr, "Out of resources\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            fprintf(stderr, "Out of host memory\n");
            break;
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            fprintf(stderr, "Profiling information not available\n");
            break;
        case CL_MEM_COPY_OVERLAP:
            fprintf(stderr, "Memory copy overlap\n");
            break;
        case CL_IMAGE_FORMAT_MISMATCH:
            fprintf(stderr, "Image format mismatch\n");
            break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            fprintf(stderr, "Image format not supported\n");
            break;
        case CL_BUILD_PROGRAM_FAILURE:
            fprintf(stderr, "Program build failure\n");
            break;
        case CL_MAP_FAILURE:
            fprintf(stderr, "Map failure\n");
            break;
        case CL_INVALID_VALUE:
            fprintf(stderr, "Invalid value\n");
            break;
        case CL_INVALID_DEVICE_TYPE:
            fprintf(stderr, "Invalid device type\n");
            break;
        case CL_INVALID_PLATFORM:
            fprintf(stderr, "Invalid platform\n");
            break;
        case CL_INVALID_DEVICE:
            fprintf(stderr, "Invalid device\n");
            break;
        case CL_INVALID_CONTEXT:
            fprintf(stderr, "Invalid context\n");
            break;
        case CL_INVALID_QUEUE_PROPERTIES:
            fprintf(stderr, "Invalid queue properties\n");
            break;
        case CL_INVALID_COMMAND_QUEUE:
            fprintf(stderr, "Invalid command queue\n");
            break;
        case CL_INVALID_HOST_PTR:
            fprintf(stderr, "Invalid host pointer\n");
            break;
        case CL_INVALID_MEM_OBJECT:
            fprintf(stderr, "Invalid memory object\n");
            break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            fprintf(stderr, "Invalid image format descriptor\n");
            break;
        case CL_INVALID_IMAGE_SIZE:
            fprintf(stderr, "Invalid image size\n");
            break;
        case CL_INVALID_SAMPLER:
            fprintf(stderr, "Invalid sampler\n");
            break;
        case CL_INVALID_BINARY:
            fprintf(stderr, "Invalid binary\n");
            break;
        case CL_INVALID_BUILD_OPTIONS:
            fprintf(stderr, "Invalid build options\n");
            break;
        case CL_INVALID_PROGRAM:
            fprintf(stderr, "Invalid program\n");
            break;
        case CL_INVALID_PROGRAM_EXECUTABLE:
            fprintf(stderr, "Invalid program executable\n");
            break;
        case CL_INVALID_KERNEL_NAME:
            fprintf(stderr, "Invalid kernel name\n");
            break;
        case CL_INVALID_KERNEL_DEFINITION:
            fprintf(stderr, "Invalid kernel definition\n");
            break;
        case CL_INVALID_KERNEL:
            fprintf(stderr, "Invalid kernel\n");
            break;
        case CL_INVALID_ARG_INDEX:
            fprintf(stderr, "Invalid argument index\n");
            break;
        case CL_INVALID_ARG_VALUE:
            fprintf(stderr, "Invalid argument value\n");
            break;
        case CL_INVALID_ARG_SIZE:
            fprintf(stderr, "Invalid argument size\n");
            break;
        case CL_INVALID_KERNEL_ARGS:
            fprintf(stderr, "Invalid kernel arguments\n");
            break;
        case CL_INVALID_WORK_DIMENSION:
            fprintf(stderr, "Invalid work dimension\n");
            break;
        case CL_INVALID_WORK_GROUP_SIZE:
            fprintf(stderr, "Invalid work group size\n");
            break;
        case CL_INVALID_WORK_ITEM_SIZE:
            fprintf(stderr, "Invalid work item size\n");
            break;
        case CL_INVALID_GLOBAL_OFFSET:
            fprintf(stderr, "Invalid global offset\n");
            break;
        case CL_INVALID_EVENT_WAIT_LIST:
            fprintf(stderr, "Invalid event wait list\n");
            break;
        case CL_INVALID_EVENT:
            fprintf(stderr, "Invalid event\n");
            break;
        case CL_INVALID_OPERATION:
            fprintf(stderr, "Invalid operation\n");
            break;
        case CL_INVALID_GL_OBJECT:
            fprintf(stderr, "Invalid OpenGL object\n");
            break;
        case CL_INVALID_BUFFER_SIZE:
            fprintf(stderr, "Invalid buffer size\n");
            break;
        case CL_INVALID_MIP_LEVEL:
            fprintf(stderr, "Invalid mip-map level\n");
            break;
        default:
            fprintf(stderr, "Unknown\n");
        }
    }
#endif

    ImageRenderer::ImageRenderer(const string& name)
        : m_uploadThreadPrefetch(false)
        , m_uploadThreadDevice(0)
        , m_setGLContext(false)
        ,
#ifdef PLATFORM_DARWIN
        m_setCLContext(false)
        ,
#endif
        m_stopUploadThread(false)
        , m_drawReady(false)
        , m_uploadReady(true)
        , m_name(name)
        , m_filter(GL_LINEAR)
        , m_defaultDeviceFBORingBufferSize(1)
        , m_deviceFBORingBufferIndex(0)
        , m_mainRenderSerialNumber(0)
        , m_fullRenderSerialNumber(0)
        , m_programCache(new Shader::ProgramCache)
        , m_rootContext(0)
    {
        m_bgpattern = defaultBGPattern;

        // cout << "INFO: output ring buffer size is " <<
        // m_defaultDeviceFBORingBufferSize << endl;
        m_maxmem = SystemInfo::maxVRAM();
        m_hasThreadedUpload = getenv("TWK_ALLOW_THREADED_UPLOAD") != NULL;
        m_glState = new GLState();

        if (const char* p = getenv("TWK_IPCORE_RINGBUFFER_SIZE"))
        {
            m_defaultDeviceFBORingBufferSize = atoi(p);
            cout << "INFO: ringbuffer size is "
                 << m_defaultDeviceFBORingBufferSize << endl;
        }

        initProfilingState(0);
    }

    ImageRenderer::~ImageRenderer()
    {
#ifdef PLATFORM_DARWIN
        if (m_setCLContext)
        {
            for (size_t i = 0; i < m_clContext.clProgram.kernels.size(); ++i)
            {
                clReleaseKernel(m_clContext.clProgram.kernels[i]);
            }
            clReleaseProgram(m_clContext.clProgram.program);

            clReleaseCommandQueue(m_clContext.commandQueue);
            clReleaseContext(m_clContext.clContext);
        }
#endif

        // dual copy
        m_stopUploadThread = true;
        notifyUpload();

        if (m_uploadThread.get_id() != Thread::id())
        {
            m_uploadThread.join();
        }

        clearState();

        // clean up
        delete m_programCache;

        m_controlDevice.clearFBOs();
        m_outputDevice.clearFBOs();
        delete m_glState;
    }

    //----------------------------------------------------------------------
    //
    //  Dual copy engine threaded upload
    //

    void ImageRenderer::notifyDraw()
    {
        setDrawReady(true);
        m_drawCond.notify_one();
    }

    void ImageRenderer::waitToUpload()
    {
        ScopedLock lock(m_uploadMutex);
        while (!m_uploadReady)
            m_uploadCond.wait(lock);
        m_uploadReady = false;
    }

    void ImageRenderer::waitToDraw()
    {
        ScopedLock lock(m_drawMutex);
        while (!m_drawReady)
            m_drawCond.wait(lock);
        m_drawReady = false;
    }

    void ImageRenderer::setDrawReady(bool v)
    {
        ScopedLock lock(m_drawMutex);
        m_drawReady = v;
    }

    void ImageRenderer::setUploadReady(bool v)
    {
        ScopedLock lock(m_uploadMutex);
        m_uploadReady = v;
    }

    void ImageRenderer::notifyUpload()
    {
        setUploadReady(true);
        m_uploadCond.notify_one();
    }

    //----------------------------------------------------------------------
    //
    //  Miscellaneous: utilities, public funcs, etc.
    //

    void ImageRenderer::setFiltering(int f) { m_filter = f; }

    std::string ImageRenderer::logicalImageHash(const IPImage* img) const
    {
        ostringstream o;
        if (img->fb)
            o << img->fbHash();
        else
            o << img->renderIDHash();
        o << "pa" << img->displayPixelAspect();
        if (img->isCropped)
        {
            o << "crop" << img->cropStartX << " " << img->cropStartY << " "
              << img->cropEndX << " " << img->cropEndY << endl;
        }
        return o.str();
    }

    const ImageRenderer::VideoDevice* ImageRenderer::currentDevice() const
    {
        return m_rootContext ? m_rootContext->device : m_controlDevice.device;
    }

    void ImageRenderer::initProfilingState(TwkUtil::Timer* t)
    {
        m_profilingState.profilingTimer = t;
        m_profilingState.fenceWaitTime = 0.0;
        m_profilingState.uploadPlaneTime = 0.0;
    }

    void ImageRenderer::setIntermediateLogging(bool b)
    {
        ImageFBOManager::setIntermediateLogging(b);
    }

    ImageRenderer::FastPath
    ImageRenderer::findFastPath(const FrameBuffer* fb) const
    {
        // NOTE this is probably no longer relevant because most cards nowadays
        // are fast either RGBA or BGRA
        //
        //  This function should be expanded to handle other types of
        //  fast path not just this BGRA exception.
        //
        FastPath fastPath;
        fastPath.format8x4 = GL_RGBA;

#ifdef TWK_BGRA_FAST_PATH
        if (fb)
        {
            //
            //  Can only correct BGRA in the fragment program
            //

            if (fb->dataType() == FrameBuffer::UCHAR && fb->numChannels() == 4)
            {
                fastPath.format8x4 = GL_BGRA;
            }
        }
#endif
        return fastPath;
    }

    ImageRenderer::Box2f ImageRenderer::viewport() const
    {
        const VideoDevice* d = currentDevice();
        const Margins& margins = d->margins();
        const float tw = d->width();
        const float th = d->height();
        const float gw = tw - margins.left - margins.right;
        const float gh = th - margins.bottom - margins.top;

        return Box2f(Vec2f(margins.left, margins.bottom),
                     Vec2f(margins.left + gw, margins.bottom + gh));
    }

    void ImageRenderer::clearState()
    {
        clearRenderedImages();

        // clear state will unbind the FBO currently bound
        m_glState->clearState();
        m_imageFBOManager.flushImageFBOs();
        flushProgramCache();
    }

    //----------------------------------------------------------------------
    //
    //  GL/Card related
    //

    void ImageRenderer::createGLContexts()
    {
        m_setGLContext = true;
        delete m_uploadThreadDevice;
        m_uploadThreadDevice =
            controlDevice().glDevice->newSharedContextWorkerDevice();
        controlDevice().glDevice->makeCurrent();
    }

    void ImageRenderer::queryGLIntoContainer(IPNode* node)
    {
        const string glver = TwkGLF::safeGLGetString(GL_VERSION);
        TWK_GLDEBUG;
        const string glslver =
            TwkGLF::safeGLGetString(GL_SHADING_LANGUAGE_VERSION);
        TWK_GLDEBUG;
        const string glven = TwkGLF::safeGLGetString(GL_VENDOR);
        TWK_GLDEBUG;
        const string glren = TwkGLF::safeGLGetString(GL_RENDERER);
        TWK_GLDEBUG;
        const bool inParallels = glren.find("Parallels") != string::npos;

        vector<string> tokens;
        algorithm::split(tokens, glver, is_any_of(". "), token_compress_on);
        int glMajor = 0, glMinor = 0;
        if (tokens.size() >= 2)
        {
            glMajor = atoi(tokens[0].c_str());
            glMinor = atoi(tokens[1].c_str());
        }
        else
        {
            cerr << "ERROR: Could not retrieve OpenGL version. Make sure you "
                    "have installed the Nvidia drivers."
                 << endl;
        }

        vector<string> tokens2;
        algorithm::split(tokens2, glslver, is_any_of(". "), token_compress_on);
        int glslMajor = 0, glslMinor = 0;
        if (tokens2.size() >= 2)
        {
            glslMajor = atoi(tokens2[0].c_str());
            glslMinor = atoi(tokens2[1].c_str());
        }

        string strExtension = TwkGLF::safeGLGetString(GL_EXTENSIONS);
        vector<string> extensions;
        algorithm::split(extensions, strExtension, is_any_of(" "),
                         token_compress_on);

        GLint maxt = 0;
        GLint iunits = 0, tcoords = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxt);
        TWK_GLDEBUG;
        GLint maxva;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxva);
        TWK_GLDEBUG;
        GLint maxtu;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxtu);
        TWK_GLDEBUG;
        GLint maxdb;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxdb);
        TWK_GLDEBUG;
        GLint maxsamp;
        glGetIntegerv(GL_MAX_SAMPLES, &maxsamp);
        TWK_GLDEBUG;
#ifdef GL_MAX_RECTANGLE_TEXTURE_SIZE
        GLint maxrtsize;
        glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &maxrtsize);
        TWK_GLDEBUG;
#endif
        GLint max3d;
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3d);
        TWK_GLDEBUG;

#ifdef GL_MAX_TEXTURE_BUFFER_SIZE
        // parallels doesn't do this for some reason -- its an error
        GLint maxtb = 0;
        if (!inParallels)
            glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxtb);
        TWK_GLDEBUG;
#endif

        node->declareProperty<StringProperty>("opengl.GL_VERSION", glver, 0,
                                              true);
        node->declareProperty<IntProperty>("opengl.majorVersion", glMajor, 0,
                                           true);
        node->declareProperty<IntProperty>("opengl.minorVersion", glMinor, 0,
                                           true);
        node->declareProperty<StringProperty>(
            "opengl.GL_SHADING_LANGUAGE_VERSION", glslver, 0, true);
        node->declareProperty<StringProperty>("opengl.GL_VENDOR", glven, 0,
                                              true);
        node->declareProperty<StringProperty>("opengl.GL_RENDERER", glren, 0,
                                              true);
        node->declareProperty<IntProperty>("opengl.GL_MAX_TEXTURE_SIZE", maxt,
                                           0, true);
#ifdef GL_MAX_TEXTURE_BUFFER_SIZE
        node->declareProperty<IntProperty>("opengl.GL_MAX_TEXTURE_BUFFER_SIZE",
                                           maxtb, 0, true);
#endif
        node->declareProperty<IntProperty>("opengl.GL_MAX_TEXTURE_IMAGE_UNITS",
                                           iunits, 0, true);
        node->declareProperty<IntProperty>("opengl.GL_MAX_TEXTURE_COORDS",
                                           tcoords, 0, true);
        node->declareProperty<IntProperty>("opengl.GL_MAX_TEXTURE_UNITS", maxtu,
                                           0, true);
        node->declareProperty<IntProperty>("opengl.GL_MAX_VERTEX_ATTRIBS",
                                           maxva, 0, true);
        node->declareProperty<IntProperty>("opengl.GL_MAX_DRAW_BUFFERS", maxdb,
                                           0, true);
        node->declareProperty<IntProperty>("opengl.GL_MAX_SAMPLES", maxsamp, 0,
                                           true);
#ifdef GL_MAX_RECTANGLE_TEXTURE_SIZE
        node->declareProperty<IntProperty>(
            "opengl.GL_MAX_RECTANGLE_TEXTURE_SIZE", maxrtsize, 0, true);
#endif
        node->declareProperty<IntProperty>("opengl.GL_MAX_3D_TEXTURE_SIZE",
                                           max3d, 0, true);

        StringProperty* sp =
            node->createProperty<StringProperty>("opengl.GL_EXTENSIONS");
        sp->valueContainer() = extensions;

        node->declareProperty<IntProperty>("opengl.glsl.majorVersion",
                                           glslMajor, 0, true);
        node->declareProperty<IntProperty>("opengl.glsl.minorVersion",
                                           glslMinor, 0, true);
    }

    void ImageRenderer::queryGL()
    {
        if (!m_queryInit)
            return;

#ifdef TWK_USE_GLEW
        //
        //  Follow the insane path leading up to this function!
        //  Glew will *only* use wgl so I had to add an argument to its
        //  init function to take a "procedure to look up gl
        //  symbols". This is normally wglProcAddress() or something, but
        //  when we're using Mesa we need to give it
        //  OSMesaGetProcAddress() instead. BUT WAIT! you can't do that!
        //  because Mesa HAS to live in a DLL if you want to swap it in
        //  for GL. So you have to make another function which calls the
        //  ProcAddressFunc. But wait! you can't include osmesa.h here! So
        //  all we do is have a variable that's set from somewhere else
        //  that has to have the right function. If it 0, glew will use
        //  its normal path. Another round of DLL nightmare.
        //

        GLenum err = glewInit(m_procFunc);

        if (GLEW_OK != err)
        {
            TWK_THROW_STREAM(RenderFailedExc,
                             "glewInit FAILED: " << glewGetErrorString(err));
        }
#else
        TWK_INIT_GL_EXTENSIONS;
#endif

        // TWK_GLDEBUG;

        const string glver = TwkGLF::safeGLGetString(GL_VERSION);
        TWK_GLDEBUG;
        const string glslver =
            TwkGLF::safeGLGetString(GL_SHADING_LANGUAGE_VERSION);
        TWK_GLDEBUG;
        const string glven = TwkGLF::safeGLGetString(GL_VENDOR);
        TWK_GLDEBUG;
        const string glren = TwkGLF::safeGLGetString(GL_RENDERER);
        TWK_GLDEBUG;

        vector<string> tokens;
        stl_ext::tokenize(tokens, glren);
        m_softwareGLRenderer = (tokens.size() && tokens[0] == "Mesa");

        vector<string> tokens2;
        stl_ext::tokenize(tokens2, glslver, ".");
        size_t glslMajor = 0, glslMinor = 0;
        if (tokens2.size() >= 2)
        {
            glslMajor = atoi(tokens2[0].c_str());
            glslMinor = atoi(tokens2[1].c_str());
        }

        GLboolean b[10];
        m_queryInit = false;
        string strExtension = TwkGLF::safeGLGetString(GL_EXTENSIONS);
        // TWK_GLDEBUG;

        m_rectTextures = TWK_GL_SUPPORTS("GL_ARB_texture_rectangle");
        m_textureFloat = TWK_GL_SUPPORTS("GL_ARB_texture_float")
                         || TWK_GL_SUPPORTS("GL_APPLE_float_pixels")
                         || TWK_GL_SUPPORTS("GL_ATI_texture_float")
                         || m_softwareGLRenderer;
        m_halfFloat = TWK_GL_SUPPORTS("GL_ARB_half_float_pixel")
                      || TWK_GL_SUPPORTS("GL_APPLE_float_pixels")
                      || TWK_GL_SUPPORTS("GL_ATI_texture_float");
        m_floatBuffers = TWK_GL_SUPPORTS("GL_NV_float_buffer")
                         || TWK_GL_SUPPORTS("GL_APPLE_float_pixels")
                         || TWK_GL_SUPPORTS("GL_ARB_color_buffer_float")
                         || TWK_GL_SUPPORTS("GL_ATI_pixel_format_float")
                         || TWK_GL_SUPPORTS("WGL_ATI_pixel_format_float")
                         || m_softwareGLRenderer;
        m_pixelBuffers = TWK_GL_SUPPORTS("GL_ARB_pixel_buffer_object");
        m_hasClientStorage = TWK_GL_SUPPORTS("GL_APPLE_client_storage");
        m_imaging = TWK_GL_SUPPORTS("GL_ARB_imaging");
        m_fragmentProgram = TWK_GL_SUPPORTS("GL_ARB_fragment_program");
        m_nonPowerOf2 = TWK_GL_SUPPORTS("GL_ARB_texture_non_power_of_two");
        m_ycbcrApple = TWK_GL_SUPPORTS("GL_APPLE_ycbcr_422");

        if (m_fragmentProgram)
        {
            GLint ui;
            GLint tl;

            glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
                              GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB, &ui);
            TWK_GLDEBUG;

            glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
                              GL_MAX_PROGRAM_TEMPORARIES_ARB, &tl);
            TWK_GLDEBUG;

            m_ALUinsnLimit = ui;
            m_tempLimit = tl;
        }

        GLint maxt = 0;
        GLint iunits = 0, tcoords = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxt);
        TWK_GLDEBUG;

        if (maxt > 0)
        {
            IPCore::Application::setOptionValue<int>("maxTextureSize", maxt);
        }

        if (m_fragmentProgram)
        {
            glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &iunits);
            TWK_GLDEBUG;
            glGetIntegerv(GL_MAX_TEXTURE_COORDS, &tcoords);
            TWK_GLDEBUG;
        }

        GLint maxva;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxva);

        m_maxImageUnits = iunits;
        m_maxTexCoords = tcoords;
        m_maxVertexAttrs = maxva;
        m_maxH = maxt;
        m_maxW = maxt;

        if (m_reportGL)
        {
            cout << "INFO: GL version            = " << glver << endl;
            cout << "INFO: GLSL version          = " << glslver << endl;
            cout << "INFO: GL vendor             = " << glven << endl;
            cout << "INFO: GL renderer           = " << glren << endl;
            cout << "INFO: Max texture size      = " << maxt << endl;
            cout << "INFO: Max ALU insns         = " << m_ALUinsnLimit << endl;
            cout << "INFO: Max TEMP vars         = " << m_tempLimit << endl;
            cout << "INFO: half float            = " << m_halfFloat << endl;
            cout << "INFO: rectangle textures    = " << m_rectTextures << endl;
            cout << "INFO: non-power-of-two tex  = " << m_nonPowerOf2 << endl;
            cout << "INFO: floating buffers      = " << m_floatBuffers << endl;
            cout << "INFO: texture float         = " << m_textureFloat << endl;
            cout << "INFO: client storage        = " << m_hasClientStorage
                 << endl;
            cout << "INFO: imaging               = " << m_imaging << endl;
            cout << "INFO: fragment program      = " << m_fragmentProgram
                 << endl;
            cout << "INFO: pixel buffer object   = " << m_pixelBuffers << endl;
            cout << "INFO: apple yuv 422         = " << m_ycbcrApple << endl;
            cout << "INFO: Max tex image units   = " << m_maxImageUnits << endl;
            cout << "INFO: Max tex coords        = " << m_maxTexCoords << endl;
            cout << "INFO: Max vert attrs        = " << m_maxVertexAttrs
                 << endl;
            cout << "INFO: all GL extensions     = " << strExtension << endl;
        }

        if (!m_rectTextures)
        {
            cerr << "WARNING: GPU lacks rectangle textures. " << endl;
            m_useTextures = false;
        }

        if (!m_halfFloat)
        {
            cerr << "WARNING: GPU cannot handle 16 bit float images natively"
                 << endl;
        }

        if (!m_textureFloat)
        {
            cerr
                << "WARNING: GPU does not handle floating point images natively"
                << endl;
        }

        if (!m_floatBuffers)
        {
            // cerr << "WARNING: GPU does not handle floating point frame
            // buffer" << endl;
        }

        if (!m_fragmentProgram)
        {
            cerr << "WARNING: GPU lacks fragment program -- many features are "
                    "disabled"
                 << endl;
        }
        else if (m_ALUinsnLimit < 32)
        {
            cerr << "WARNING: GPU only supports " << m_ALUinsnLimit
                 << " ALU instructions. Some features may fail if used "
                    "simultaneously"
                 << endl;
        }

        if (!m_useTextures)
        {
            cerr << "WARNING: falling back to basic pixel drawing" << endl;
            cerr << "WARNING: color corrections may be slow" << endl;
        }

        SystemInfo::setMaxColorBitDepth(m_textureFloat ? 32 : 8);
        // SystemInfo::setMaxColorBitDepth(32);

        //
        //  Indicate the acceptable image formats to the Format node.
        //  NOTE: this switch statement falls through (no breaks) on
        //  purpose.
        //

        m_floatFormats = m_textureFloat;

        if (SystemInfo::maxColorBitDepth() == 32)
        {
            if (m_textureFloat)
                m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::FLOAT);
            if (m_textureFloat)
                m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::HALF);
            m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::USHORT);
            m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::UCHAR);
        }
        else if (SystemInfo::maxColorBitDepth() == 16)
        {
            if (m_textureFloat)
                m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::HALF);
            m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::USHORT);
            m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::UCHAR);
        }
        else
        {
            m_FBAcceptableTypes.insert(TwkFB::FrameBuffer::UCHAR);
            m_floatFormats = false;
        }

        if (!m_defaultAllowPBOs || !evUsePBOs.getValue())
        {
            m_pixelBuffers = false;
        }
    }

//----------------------------------------------------------------------
//
//  OpenCL
//
#ifdef PLATFORM_DARWIN

    void ImageRenderer::createCLContexts()
    {
        m_setCLContext = true;

        // there is only one CL context for each ImageRenderer instance
        CGLContextObj currentGLContext = CGLGetCurrentContext();
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(currentGLContext);

        cl_context_properties props[] = {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
            (cl_context_properties)kCGLShareGroup, 0};
        m_clContext.clContext = clCreateContext(props, 0, 0, NULL, 0, 0);

        cl_int ret = clGetPlatformIDs(1, &m_clContext.platformID,
                                      &m_clContext.platformNo);
        printCLError(ret);

        size_t dSize;
        ret = clGetContextInfo(m_clContext.clContext, CL_CONTEXT_DEVICES, 0,
                               NULL, &dSize);
        printCLError(ret);

        size_t dNo = dSize / sizeof(cl_device_id);
        cl_device_id devices[dNo];
        m_clContext.deviceNo = dNo;

        ret = clGetContextInfo(m_clContext.clContext, CL_CONTEXT_DEVICES, dSize,
                               devices, &dSize);

        for (size_t i = 0; i < dNo; i++)
        {
            cl_device_type deviceType;
            ret = clGetDeviceInfo(devices[i], CL_DEVICE_TYPE,
                                  sizeof(cl_device_type), &deviceType, &dSize);
            printCLError(ret);

            if (deviceType == CL_DEVICE_TYPE_GPU)
            {
                m_clContext.deviceID = devices[i];
                m_clContext.commandQueue = clCreateCommandQueue(
                    m_clContext.clContext, m_clContext.deviceID,
                    CL_QUEUE_PROFILING_ENABLE, &ret);
                printCLError(ret);

                // query opencl version
                char v[128];
                ret = clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, 128,
                                      (void*)v, NULL);
                string v2(v);
                if (v2.find("1.0") != string::npos)
                    m_clContext.version = CLContext::OpenCL1_0;
                else if (v2.find("1.1") != string::npos)
                    m_clContext.version = CLContext::OpenCL1_1;
                else
                    m_clContext.version = CLContext::OpenCL1_2;
                printCLError(ret);

                return;
            }
        }
    }

    void ImageRenderer::compileCLHistogram()
    {

        cl_int ret;
        // query card abilities: local memory size and workgroup size
        cl_ulong localMemSize;
        ret = clGetDeviceInfo(m_clContext.deviceID, CL_DEVICE_LOCAL_MEM_SIZE,
                              sizeof(cl_ulong), &localMemSize, NULL);

        const char* histoSrc = NULL;
        if (localMemSize <= 16 * 1024)
            histoSrc = Histogram16k_cl;
        else if (localMemSize <= 32 * 1024)
            histoSrc = Histogram32k_cl;
        else
            histoSrc = Histogram48k_cl;

        m_clContext.clProgram.program = clCreateProgramWithSource(
            m_clContext.clContext, 1, (const char**)&histoSrc, NULL, &ret);
        printCLError(ret);

        ret = clBuildProgram(m_clContext.clProgram.program, 1,
                             &m_clContext.deviceID, NULL, NULL, NULL);
        printCLError(ret);

#ifdef NDEBUG
#else
        size_t len;
        clGetProgramBuildInfo(m_clContext.clProgram.program,
                              m_clContext.deviceID, CL_PROGRAM_BUILD_LOG, 0,
                              NULL, &len);
        vector<char> buffer(len);
        clGetProgramBuildInfo(m_clContext.clProgram.program,
                              m_clContext.deviceID, CL_PROGRAM_BUILD_LOG, len,
                              &buffer[0], NULL);
#endif

        m_clContext.clProgram.kernels.push_back(clCreateKernel(
            m_clContext.clProgram.program, "histogram256_float4", &ret));
        printCLError(ret);

        m_clContext.clProgram.kernels.push_back(clCreateKernel(
            m_clContext.clProgram.program, "mergeHistograms256_float4", &ret));
        printCLError(ret);

        size_t workGroupSize;
        clGetKernelWorkGroupInfo(
            m_clContext.clProgram.kernels[0], m_clContext.deviceID,
            CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, NULL);
        m_clContext.workGroupSize = workGroupSize;
    }

    void ImageRenderer::compileCLPrograms()
    {
        //
        // compile needed CL Programs for this render pass ahead of rendering
        //
        try
        {
            if (CLContextNotSet())
            {
                createCLContexts();
                compileCLHistogram();
            }
        }
        catch (...)
        {
            cout << "OpenCL Initialization Failed" << endl;
            throw;
        }
    }

    void ImageRenderer::executeCLKernel(
        const CLContext& context, const cl_kernel& kernel,
        const size_t globalThreads[3], const size_t localThreads[3],
        const vector<pair<size_t, const void*>>& args) const
    {
        cl_int err;
        for (size_t i = 0; i < args.size(); i++)
        {
            err = clSetKernelArg(kernel, i, args[i].first, args[i].second);
            printCLError(err);
        }
        // clFinish(context.commandQueue);

        cl_event event; // opencl event if used, needs to be released to avoid
                        // mem leak
        err =
            clEnqueueNDRangeKernel(context.commandQueue, kernel, 2, NULL,
                                   globalThreads, localThreads, 0, NULL, NULL);
        printCLError(err);

#if 0
    clFinish(context.commandQueue);
    clReleaseEvent(event);
    printCLError(err);
    
    cl_ulong time_start, time_end;
    clGetEventProfilingInfo(event,
                            CL_PROFILING_COMMAND_START,
                            sizeof(time_start),
                            &time_start,
                            NULL);
    clGetEventProfilingInfo(event,
                            CL_PROFILING_COMMAND_END,
                            sizeof(time_end),
                            &time_end,
                            NULL);
    cout << "kernel milliseconds: " << ((time_end - time_start) / 1000000.0)) << "ms" << endl;
    clFlush(context.commandQueue);
#endif
    }

#define HISTOGRAM_BIN_NO 256

    void ImageRenderer::histogramOCL(cl_mem& image, cl_mem& hist,
                                     const size_t w, const size_t h) const
    {
        //
        // NOTE that the current cl code was optimized / tested on a couple of
        // macs one old AMD MAC and one mid 2012 Nvidia Mac. It was
        // surprisingly, a magnitude faster on the older AMD than on the Nvidia
        // possibly due to the unoptimized cl path from Nvidia
        //

        // group histogram
        size_t g = 256;
        size_t g2 = 1;
        if (m_clContext.workGroupSize < 512)
            g2 = 1;
        else if (m_clContext.workGroupSize < 1024)
            g2 = 2;
        else if (m_clContext.workGroupSize < 2048)
            g2 = 4;
        else
            g2 = 8;

        size_t wg = w + g - w % g;
        size_t hg = h + g2 - h % g2;
        size_t localThreads[3] = {g, g2, 1};
        size_t globalThreads[3] = {wg, g2, 1};
        size_t workGroupNo = wg / g;

        cl_int err;
        cl_mem subHist =
            clCreateBuffer(m_clContext.clContext, CL_MEM_READ_WRITE,
                           3 * HISTOGRAM_BIN_NO * workGroupNo * sizeof(uint),
                           NULL, &err); // 3 channels
        printCLError(err);

        vector<pair<size_t, const void*>> args;
        const size_t v = hg / g2;
        args.push_back(make_pair(sizeof(image), (void*)&image));
        args.push_back(make_pair(sizeof(subHist), (void*)&subHist));
        args.push_back(make_pair(sizeof(cl_uint), (void*)&v));
        args.push_back(make_pair(sizeof(cl_uint), (void*)&w));
        args.push_back(make_pair(sizeof(cl_uint), (void*)&h));

        executeCLKernel(m_clContext, m_clContext.clProgram.kernels[0],
                        globalThreads, localThreads, args);

        // merge
        size_t localThreads2[3] = {HISTOGRAM_BIN_NO, 1, 1};
        size_t globalThreads2[3] = {HISTOGRAM_BIN_NO, 1, 1};

        vector<pair<size_t, const void*>> args2;
        args2.push_back(make_pair(sizeof(cl_mem), (void*)&subHist));
        args2.push_back(make_pair(sizeof(cl_mem), (void*)&hist));
        args2.push_back(make_pair(sizeof(cl_uint), (void*)&workGroupNo));
        float imgSizef = w * h;
        args2.push_back(make_pair(sizeof(cl_float), (void*)&imgSizef));

        executeCLKernel(m_clContext, m_clContext.clProgram.kernels[1],
                        globalThreads2, localThreads2, args2);

        // TODO reuse this buffer
        clReleaseMemObject(subHist);
    }

    void ImageRenderer::computeHistogram(const ConstFBOVector& childrenFBO,
                                         const GLFBO* resultFBO) const
    {
        assert(childrenFBO.size() == 1);
        const GLFBO* fbo = childrenFBO[0];

        assert(fbo->hasColorAttachment());
        cl_int err;
#ifdef CL_VERSION_1_2
        cl_mem clImage = clCreateFromGLTexture(
            m_clContext.clContext, CL_MEM_READ_ONLY, GL_TEXTURE_RECTANGLE_ARB,
            0, fbo->colorID(0), &err);
#else
        cl_mem clImage = clCreateFromGLTexture2D(
            m_clContext.clContext, CL_MEM_READ_ONLY, GL_TEXTURE_RECTANGLE_ARB,
            0, fbo->colorID(0), &err);
#endif
        printCLError(err);
        err = clEnqueueAcquireGLObjects(m_clContext.commandQueue, 1, &clImage,
                                        0, 0, 0);
        printCLError(err);

        assert(resultFBO->hasColorAttachment());
#ifdef CL_VERSION_1_2
        cl_mem histoOut = clCreateFromGLTexture(
            m_clContext.clContext, CL_MEM_WRITE_ONLY, GL_TEXTURE_RECTANGLE_ARB,
            0, resultFBO->colorID(0), &err);
#else
        cl_mem histoOut = clCreateFromGLTexture2D(
            m_clContext.clContext, CL_MEM_WRITE_ONLY, GL_TEXTURE_RECTANGLE_ARB,
            0, resultFBO->colorID(0), &err);
#endif
        printCLError(err);
        err = clEnqueueAcquireGLObjects(m_clContext.commandQueue, 1, &histoOut,
                                        0, 0, 0);
        printCLError(err);

        histogramOCL(clImage, histoOut, fbo->width(), fbo->height());

        err = clEnqueueReleaseGLObjects(m_clContext.commandQueue, 1, &clImage,
                                        0, 0, 0);
        printCLError(err);
        err = clEnqueueAcquireGLObjects(m_clContext.commandQueue, 1, &histoOut,
                                        0, 0, 0);
        printCLError(err);
        clFlush(m_clContext.commandQueue);

        clReleaseMemObject(clImage);
        clReleaseMemObject(histoOut);
    }
#endif

    //----------------------------------------------------------------------
    //
    //  Core functions
    //

    void
    ImageRenderer::setupContextFromDevice(InternalRenderContext& context) const
    {
        const VideoDevice* device = context.device;

        if (m_controlDevice.device == device)
        {
            context.targetFBO = m_controlDevice.glDevice->defaultFBO();
        }
        else if (m_outputDevice.device == device)
        {
            size_t index =
                context.image->destination == IPImage::RightBuffer ? 1 : 0;
            context.targetFBO =
                m_outputDevice.fboRingBuffer[m_deviceFBORingBufferIndex]
                    .views[index];
        }
        else
        {
            return;
        }

        context.targetFBO->bind();

        GLPushAttrib attr1(GL_ALL_ATTRIB_BITS);
#ifdef PLATFORM_DARWIN
        GLPushClientAttrib attr2(GL_CLIENT_ALL_ATTRIB_BITS);
#endif

        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        setupBlendMode(context);
    }

    void ImageRenderer::setControlDevice(const VideoDevice* d)
    {
        const size_t ringBufferSize = m_defaultDeviceFBORingBufferSize;
        const size_t nviews = 2;

        if (d)
        {
            Device device(d, dynamic_cast<const GLVideoDevice*>(d), 0,
                          ringBufferSize, nviews);
            device.glDevice->makeCurrent();
            m_controlDevice.clearFBOs();
            m_controlDevice = device;
            setOutputDevice(m_outputDevice.device ? m_outputDevice.device : d);
        }
        else
        {
            setOutputDevice(0);
            m_controlDevice = Device();
        }
    }

    void ImageRenderer::setOutputDevice(const VideoDevice* d)
    {
        //
        //  Do nothing if the incoming device is ALREADY the output device.
        //
        if (d == m_outputDevice.device)
            return;

        if (!m_controlDevice.device)
            return;

        //
        //  This function is doing some complicated stuff.
        //
        //  The thing to realize, at least in the case of the glDevice, is
        //  that there is another context involved. The output glDevice
        //  *should* have shared textures and renderbuffers with the
        //  controlDevice (when it was created). Alas, FBOs are *not*
        //  shared between contexts. So the answer is to make a texture in
        //  the control FBO and use it in the output FBO as well. So if
        //  you render into the control it instantly shows up in the
        //  output context's FBO.
        //
        //  ALSO, you cannot blit between contexts. So you have to have an
        //  intermediate buffer to begin with. Its complicated but works.
        //
        //
        //  This can be cleared because we only create these fences for a
        //  unique device pair (controller and output).
        //

        if (m_outputDevice.glDevice)
            m_outputDevice.glDevice->makeCurrent();

        if (d)
            m_imageFBOManager.clearAllFences();
        TWK_GLDEBUG;

        m_deviceFBORingBufferIndex = 0;

        const TwkGLF::GLVideoDevice* gld =
            d ? dynamic_cast<const TwkGLF::GLVideoDevice*>(d) : 0;
        const TwkGLF::GLBindableVideoDevice* glbd =
            d ? dynamic_cast<const TwkGLF::GLBindableVideoDevice*>(d) : 0;

        const bool asyncReadBack =
            d && d->capabilities() & VideoDevice::ASyncReadBack;
        const bool multipleOutputs = d && d != m_controlDevice.device;
        const bool dualOut = glbd && glbd->isDualStereo();
        const size_t ringBufferSize = asyncReadBack
                                          ? d->asyncMaxMappedBuffers()
                                          : m_defaultDeviceFBORingBufferSize;
        const size_t nviews = 2;

        if ((ringBufferSize > m_defaultDeviceFBORingBufferSize)
            || (ringBufferSize == m_defaultDeviceFBORingBufferSize
                && m_controlDevice.ringBufferSize()
                       != m_defaultDeviceFBORingBufferSize))
        {
            m_controlDevice.setRingBufferSize(ringBufferSize, nviews);
        }

        //
        //  Create the device
        //

        Device device(d, gld, glbd, ringBufferSize, nviews);

        if (m_outputDevice.device != d
            && m_outputDevice.device != m_controlDevice.device)
        {
            if (m_outputDevice.glBindableDevice)
            {
                //  Possibly undo the bind() on this device from when we created
                //  it.
                //
                if (m_outputDevice.glBindableDevice)
                    m_outputDevice.glBindableDevice->unbind();

                m_controlDevice.glDevice->defaultFBO()->unbind();
            }

            m_outputDevice.clearFBOs();
        }

        if (device.glDevice)
        {
            device.glDevice->makeCurrent();

            //
            //  Initilaize a bunch of low level shaders
            //

            TwkGLF::textureRectGLProgram();
            TwkGLF::defaultGLProgram();
            TwkGLF::softPaintOldReplaceGLProgram();
            TwkGLF::paintOldReplaceGLProgram();
            TwkGLF::paintEraseGLProgram();
            TwkGLF::softPaintEraseGLProgram();
            TwkGLF::paintScaleGLProgram();
            TwkGLF::softPaintScaleGLProgram();
            TwkGLF::paintCloneGLProgram();
            TwkGLF::softPaintCloneGLProgram();
            TwkGLF::paintReplaceGLProgram();
            TwkGLF::softPaintReplaceGLProgram();
        }

        m_outputDevice = device;

        if (d && multipleOutputs)
        {
            const size_t w = d->internalWidth();
            const size_t h = d->internalHeight();

            m_controlDevice.glDevice->makeCurrent();
            TWK_GLDEBUG;

            //
            // The control FBO resolution comes from the *output* device
            //

            const VideoDevice::DataFormat& df =
                d->dataFormatAtIndex(d->currentDataFormat());
            GLenum iformat = TwkGLF::internalFormatFromDataFormat(df.iformat);
            TwkGLF::GLenumPair tformat =
                TwkGLF::textureFormatFromDataFormat(df.iformat);
            GLuint ttarget = m_outputDevice.device->capabilities()
                                     & VideoDevice::NormalizedCoordinates
                                 ? GL_TEXTURE_2D
                                 : GL_TEXTURE_RECTANGLE_ARB;

            if (m_outputDevice.glDevice || m_outputDevice.glBindableDevice)
            {
                if (m_outputDevice.glDevice)
                    m_outputDevice.glDevice->makeCurrent();
                m_outputDevice.clearFBOs();

                if (m_outputDevice.glBindableDevice)
                {
                    try
                    {
                        m_outputDevice.glBindableDevice->bind(
                            m_controlDevice.glDevice);
                        TWK_GLDEBUG;
                    }
                    catch (std::exception& exc)
                    {
                        throw;
                    }
                }

                m_outputDevice.allocateFBOs(w, h, iformat, 0);

                //
                //  Hook up shared texture with control device internal FBO
                //

                for (size_t i = 0; i < ringBufferSize; i++)
                {
                    GLFBO* fbo = m_outputDevice.fboRingBuffer[i].views[0];
                    fbo->bind();
                    fbo->newColorTexture(ttarget, tformat.first,
                                         tformat.second);
                    glClear(GL_COLOR_BUFFER_BIT);
                    TWK_GLDEBUG;
                    fbo->unbind();
                }

                m_controlDevice.glDevice->makeCurrent();

                if (dualOut)
                {
                    m_outputDevice.allocateFBOs(w, h, iformat, 1);

                    for (size_t i = 0; i < ringBufferSize; i++)
                    {
                        GLFBO* fbo = m_outputDevice.fboRingBuffer[i].views[1];
                        fbo->bind();
                        fbo->newColorTexture(ttarget, tformat.first,
                                             tformat.second);
                        glClear(GL_COLOR_BUFFER_BIT);
                        TWK_GLDEBUG;
                        fbo->unbind();
                    }
                }

                m_controlDevice.glDevice->makeCurrent();
                TWK_GLDEBUG;
            }
        }
        else
        {
            m_controlDevice.glDevice->makeCurrent();
            m_controlDevice.clearFBOs();

            if (m_outputDevice.glDevice)
            {
                m_outputDevice.glDevice->makeCurrent();
                m_outputDevice.clearFBOs();
            }
        }
    }

    struct AccumulateRenderInfo
    {
        AccumulateRenderInfo()
            : ncount(0)
        {
        }

        size_t ncount;

        void operator()(const IPImage* i)
        {
            if (i->renderType != IPImage::GroupType
                && i->destination != IPImage::MainBuffer
                && i->destination != IPImage::LeftBuffer
                && i->destination != IPImage::RightBuffer
                && i->destination != IPImage::NoBuffer)
            {
                ncount++;
            }
        }
    };

    void ImageRenderer::prepareTextureDescriptionsForUpload(const IPImage* img)
    {
        m_texturesToUpload.clear();
        assembleTextureDescriptionsForUpload(img);
    }

    void ImageRenderer::assembleTextureDescriptionsForUpload(const IPImage* img)
    {
        //
        // go through the whole tree
        // assemble all texture descriptions needed for upload
        // if new textures / buffers are needed then generate them now
        // before the upload thread starts uploading
        //
        for (const IPImage* i = img->children; i; i = i->next)
        {
            assembleTextureDescriptionsForUpload(i);
        }

        //
        //  traverse all framebuffers: source and aux (luts, etc.)
        //
        if (img->fb)
        {
            for (const FrameBuffer* p = img->fb; p; p = p->nextPlane())
            {
                TextureDescription::TextureMatch match;
                TextureDescription* tex = getTexture(p, match);
                if (match != TextureDescription::ExactMatch || !tex->uploaded)
                {
                    m_texturesToUpload[p->identifier()] = tex;
                }
            }
        }

        for (size_t q = 0; q < img->auxFBs.size(); q++)
        {
            TextureDescription::TextureMatch match;
            TextureDescription* tex = getTexture(img->auxFBs[q], match);
            if (match != TextureDescription::ExactMatch || !tex->uploaded)
            {
                m_texturesToUpload[img->auxFBs[q]->identifier()] = tex;
            }
        }

        for (size_t q = 0; q < img->auxMergeFBs.size(); q++)
        {
            TextureDescription::TextureMatch match;
            TextureDescription* tex = getTexture(img->auxMergeFBs[q], match);
            if (match != TextureDescription::ExactMatch || !tex->uploaded)
            {
                m_texturesToUpload[img->auxMergeFBs[q]->identifier()] = tex;
            }
        }
    }

    void ImageRenderer::setupUploadThread(IPImage* uploadRoot)
    {
        m_uploadRoot = uploadRoot;
        m_uploadRootHash = uploadRoot->renderIDHash();

        if (GLContextNotSet())
        {
            createGLContexts();

            // upload thread initialization
            m_uploadThread =
                Thread(uploadThreadTrampoline, this); // func and data
        }

        notifyUpload();
    }

    void ImageRenderer::renderBegin(const InternalRenderContext& context)
    {
        markReusableImageFBOs(context.image);
        clearRenderedImages();
    }

    void ImageRenderer::renderEnd(const InternalRenderContext& context)
    {
        m_imageFBOManager.gcImageFBOs(context.fullSerialNum);
        m_glState->clearState();
        clearImagePassStates();
    }

    void ImageRenderer::render(int frame, IPImage* root,
                               const AuxRender* auxRender,
                               const AuxAudio* auxAudio, IPImage* uploadRoot)
    {
#if defined(HOP_ENABLED)
        std::string hopMsg = std::string("ImageRenderer::render(frame=")
                             + std::to_string(frame) + std::string(")");
        HOP_PROF_DYN_NAME(hopMsg.c_str());
#endif

        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::render - Init");

            m_fullRenderSerialNumber++;
            freeOldTextures();

            if (useThreadedUpload())
            {
                if (uploadRoot == NULL)
                {
                    uploadRoot = root;
                }
                m_uploadThreadPrefetch = (root == uploadRoot) ? false : true;

                //
                // traverse our IPTree, and create gl textures/buffers for all
                // texture uploads the upload thread will need
                // the reason for this is we want all the gen/deletion of
                // textures/buffers to be handled by the main thread
                //
                if (m_uploadThreadPrefetch)
                {
                    //
                    // Load the textures for the next frame
                    // in case prefetch didnt fetch the current frame, fetch it
                    // now. This only happens for the first frame, or when the
                    // last render was N and this current render isn't N+1 (say
                    // N+2) in which case the prefetch went to a waste.
                    // Regardless we want to give prirority to make sure
                    // everything is uploaded for the current frame. This is
                    // overhead in most cases though
                    //
                    prepareTextureDescriptionsForUpload(root);
                    prefetch(root);
                }

                prepareTextureDescriptionsForUpload(uploadRoot);
                setupUploadThread(uploadRoot);
            }
            else
            {
                //
                // In case prefetch is on, in most cases textures should be
                // uploaded for this frame already with the exception of first
                // frame, or when the last render was N and this current render
                // isn't N+1 (say N+2) in which case the prefetch went to a
                // waste. In prefetch most cases this is a overhead
                //
                //

                prepareTextureDescriptionsForUpload(root);
                prefetch(root);
            }
            HOP_CALL(glFinish();)
        }

        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::render - clear BG");

            const GLFBO* controlFBO = m_controlDevice.glDevice->defaultFBO();
            if (!root)
            {
                clearBackground(controlFBO);
                return;
            }

            HOP_CALL(glFinish();)
        }

        AccumulateRenderInfo accum;
        foreach_ip(root, accum);
        m_imageCount = accum.ncount;

        InternalRenderContext context;
        context.image = root;
        context.frame = frame;
        context.numImages = m_imageCount;
        context.targetFBO = 0;
        context.fullSerialNum = m_fullRenderSerialNumber;
        context.mainSerialNum = m_mainRenderSerialNumber;

        if (root->children && !root->children->next)
        {
            if (root->destination == IPImage::OutputTexture
                || root->destination == IPImage::DataBuffer)
            {
                // nothing
            }
            else
            {
                m_contentAspect = root->children->displayAspect();
            }
        }
        else
        {
            m_contentAspect = root->displayAspect();
        }

        renderBegin(context);

        if (useThreadedUpload())
        {
            if (!m_uploadThreadPrefetch)
            {
                waitToDraw();
            }

            m_imageFBOManager.waitForTextureFence(root->renderIDHash());
        }

        assembleImagePassStates(root);

        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::render - Render Outputs");

            renderOutputs(frame, root, auxRender, auxAudio);

            HOP_CALL(glFinish();)
        }

        renderEnd(context);
    }

    void ImageRenderer::renderOutputs(int frame, const IPImage* root,
                                      const AuxRender* auxRenderer,
                                      const AuxAudio* auxAudio)
    {
        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::renderOutputs - makeCurrent");

            m_controlDevice.glDevice->makeCurrent();

            HOP_CALL(glFinish();)
        }

        const size_t riSize = m_controlDevice.fboRingBuffer.size();
        const size_t ri = m_deviceFBORingBufferIndex;
        const bool multipleOutputs = hasMultipleOutputs();
        const bool internalBuffer =
            m_controlDevice.fboRingBuffer[0].views[0] != 0;
        const GLFBO* controlFBO = m_controlDevice.glDevice->defaultFBO();
        const GLFBO* outputFBO =
            m_outputDevice.glDevice ? m_outputDevice.glDevice->defaultFBO() : 0;
        const bool dualStereo = m_outputDevice.glBindableDevice
                                && m_outputDevice.glBindableDevice->isStereo();
        const bool willBlock =
            multipleOutputs && m_outputDevice.device->willBlockOnTransfer();
        TWK_GLDEBUG;

        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::renderOutputs - bind FBO");

            controlFBO->bind();

            HOP_CALL(glFinish();)
        }

        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::renderOutputs - renderMain");

            renderMain(controlFBO, 0, 0, auxRenderer, frame, root);

            HOP_CALL(glFinish();)
        }

        if (multipleOutputs && !willBlock)
        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::renderOutputs - multipleOutputs render");

            m_outputDevice.device->beginTransfer();

            if (m_outputDevice.glDevice)
            {
                if (m_outputDevice.device->audioOutputEnabled())
                {
                    if (auxAudio && auxAudio->isAvailable())
                    {
                        size_t aindex =
                            m_outputDevice.device->currentAudioFrameSizeIndex();
                        size_t asize;
                        void* data =
                            auxAudio->audioForFrame(frame, aindex, asize);

                        if (data)
                        {
                            m_outputDevice.device->transferAudio(data, asize);
                        }
                        else
                        {
                            m_outputDevice.device->transferAudio(0, 0);
                        }
                    }
                    else
                    {
                        m_outputDevice.device->transferAudio(0, 0);
                    }
                }
                m_outputDevice.glDevice->makeCurrent();
                m_outputDevice.fboRingBuffer[ri].views[0]->copyTo(
                    outputFBO); // uses blit

                m_controlDevice.glDevice->makeCurrent();
                m_controlDevice.glDevice->redraw();
                controlFBO->bind();
            }
            else if (m_outputDevice.glBindableDevice)
            {
                if (m_outputDevice.glBindableDevice->readyForTransfer())
                {
                    if (m_outputDevice.device->audioOutputEnabled())
                    {
                        if (auxAudio && auxAudio->isAvailable())
                        {
                            size_t aindex = m_outputDevice.device
                                                ->currentAudioFrameSizeIndex();
                            size_t asize;
                            void* data =
                                auxAudio->audioForFrame(frame, aindex, asize);

                            if (data)
                            {
                                m_outputDevice.device->transferAudio(data,
                                                                     asize);
                            }
                            else
                            {
                                m_outputDevice.device->transferAudio(0, 0);
                            }
                        }
                        else
                        {
                            m_outputDevice.device->transferAudio(0, 0);
                        }
                    }
                    if (dualStereo)
                    {
                        // was riNext
                        GLFBO* fbo0 = m_outputDevice.fboRingBuffer[ri].views[0];
                        GLFBO* fbo1 = m_outputDevice.fboRingBuffer[ri].views[1];

                        m_imageFBOManager.waitForFBOFence(fbo0);
                        m_imageFBOManager.waitForFBOFence(fbo1);

                        const GLFBO *leftFBO = fbo0, *rightFBO = fbo1;

                        if (m_outputDevice.device->swapStereoEyes())
                        {
                            leftFBO = fbo1;
                            rightFBO = fbo0;
                        }

                        m_outputDevice.glBindableDevice->transfer2(leftFBO,
                                                                   rightFBO);
                    }
                    else
                    {
                        GLFBO* fbo0 = m_outputDevice.fboRingBuffer[ri].views[0];

                        m_imageFBOManager.waitForFBOFence(fbo0);

                        m_outputDevice.glBindableDevice->transfer(fbo0);
                    }

                    m_controlDevice.glDevice->makeCurrent();
                    controlFBO->bind();
                }
            }

            m_outputDevice.device->endTransfer();

            m_deviceFBORingBufferIndex =
                (m_deviceFBORingBufferIndex + 1) % riSize;

            HOP_CALL(glFinish();)
        }

        m_controlDevice.glDevice->makeCurrent();
    }

    void ImageRenderer::renderMain(const GLFBO* target, const GLFBO* targetL,
                                   const GLFBO* targetR,
                                   const AuxRender* auxRenderer, int frame,
                                   const IPImage* root)
    {
        m_mainRenderSerialNumber++;

        InternalRenderContext context;
        context.image = root;
        context.frame = frame;
        context.numImages = m_imageCount;
        context.blendMode = IPImage::Replace;
        context.targetFBO = target;
        context.targetLFBO = targetL;
        context.targetRFBO = targetR;
        context.device = root->device;
        context.auxRenderer = auxRenderer;
        context.fullSerialNum = m_fullRenderSerialNumber;
        context.mainSerialNum = m_mainRenderSerialNumber;

        renderRecursive(context);
    }

    void ImageRenderer::renderCurrentImage(InternalRenderContext& baseContext,
                                           const GLFBO* fbo)
    {
        const IPImage* root = baseContext.image;
        InternalRenderContext context;

        //
        //  Restore parent context
        //

        context = baseContext;
        context.image = root;
        context.imageFBO = fbo;
        context.numImages = m_imageCount;

        setupBlendMode(context);

        renderImage(context);
        if (!context.norender)
            renderPaint(context.image, context.targetFBO);
    }

    void ImageRenderer::renderAllChildren(InternalRenderContext& context)
    {
        //
        //  Render the current image's children recursively
        //

        // Note on reverse-order blending : Reversing the order of inputs when
        // blending has a huge impact; The first input becomes the top one.
        // For blend modes like 'Add' this has no impact, but for others it
        // does. This mode was implemented when the StackIPNode/SequenceIPNode
        // did not properly support multi-blend-modes, ie.: all inputs would
        // have the same blend mode.
        //
        // In order to support per-input blending modes, a new flag
        // (supportReversedOrderBlending) was created to allow an IPNode to
        // instruct the ImageRenderer that a given IPImage does not support
        // reversing the order of input blending.

        bool reverse = false;
        bool dontBlendFirst = false;
        const IPImage* root = context.image;

        //
        //  for merge context reverse is always false
        //

        if (!context.mergeContext && root->supportReversedOrderBlending
            && root->renderType != IPImage::GroupType)
        {
            switch (context.blendMode)
            {
            case IPImage::Over:
            case IPImage::Replace:
                reverse = true;
                break;
            case IPImage::Difference:
            case IPImage::ReverseDifference:
                dontBlendFirst = context.doBlend ? false : true;
                reverse = true;
                break;
            default:
                reverse = false;
            }
        }

        //
        //  Since we rely on the traversal order to assign texture coordinates
        //  we cannot allow reversing in merged images. That's fine anyway
        //  because the merge shader determines order of operations. See
        //  Shader.cpp bind2() function comments on how coordinates are (not)
        //  passed to the shader.
        //
        //  The way that the texture coordinates are assumed to derived from
        //  the order is the trickiest part of the rendering code.
        //

        if (reverse)
        {
            //
            //  Since IPImage is a singly linked list and we don't know how
            //  many children there are a temporary buffer is required to
            //  reverse the children for iteration.
            //

            vector<const IPImage*> children;

            for (const IPImage* i = root->children; i; i = i->next)
            {
                children.push_back(i);
            }

            std::reverse(children.begin(), children.end());

            for (size_t i = 0; i < children.size(); i++)
            {
                InternalRenderContext childContext = context;
                childContext.image = children[i];
                if (children[i]->device)
                    childContext.device = children[i]->device;

                if (dontBlendFirst && i == 0 && !childContext.doBlend)
                {
                    childContext.blendMode = IPImage::Replace;
                    context.doBlend =
                        true; // set to true, because the first 'no blend' has
                              // already happened. we want to blend for the rest
                              // of the rendering pass
                }
                else
                {
                    setupBlendMode(childContext);
                }

                renderRecursive(childContext);
            }
        }
        else
        {
            for (const IPImage* i = root->children; i; i = i->next)
            {
                InternalRenderContext childContext = context;
                childContext.image = i;
                if (i->device)
                    childContext.device = i->device;

                setupBlendMode(childContext);

                renderRecursive(childContext);
            }
        }
    }

    void ImageRenderer::renderIntermediate(InternalRenderContext& baseContext,
                                           const GLFBO* incomingFBO)
    {
        HOP_CALL(glFinish();)
        HOP_PROF_FUNC();

        //
        //  renderIntermediate starts a new InternalRenderContext and
        //  either reuses an existing compatible FBO as the target or
        //  creates a new one.
        //
        //  This is the main entry point for the GPU cache. Either this
        //  function finds an existing rendered FBO or it doesn't. If it
        //  finds it, rendering becomes recording only.
        //

        InternalRenderContext context = baseContext;
        const IPImage* root = baseContext.image;
        IPImage::RenderType renderType = root->renderType;
        const GLFBO* fbo = incomingFBO;

        if (!fbo)
        {
            HOP_CALL(glFinish();)
            HOP_PROF("renderIntermediate - findExistingImageFBO");

            if (ImageFBO* ifbo = findExistingImageFBO(root))
            {
                //
                //  Found an FBO with the exact same pixels we'd end up
                //  renderering if we continue. Just mark it used and stop
                //  rendering down this branch of the image tree.
                //
                //  Further children should also not be rendered
                //  regardless of whether or not they have cached existing
                //  renders. However, they need to be recorded by the
                //  renderer as if they were rendered.
                //
                //
                //  Note we should call findExistingImageFBO on root
                //  even if baseContext.norender is true
                //  this is because if findExistingImageFBO finds a match
                //  it will mark this fbo unavailable for this whole render pass
                //  which is a good thing because we don't want others to
                //  grab it and modify its content, in case we can reuse it more
                //  times
                //
                fbo = ifbo->fbo();
                context.norender = true;
            }
            else if (!baseContext.norender)
            {
                //
                //  Note if baseContext.norender is true, yet we did not find
                //  a fbo for this image, it means we do not need to render the
                //  children. Furthermore we should not call newImageFBO because
                //  it will return a imageFBO with this image's identifier (see
                //  newImageFBO in ImageFBO.cpp) even though we will not be
                //  rendering anything into the imageFBO's fbo this means the
                //  imageFBO will appear to be a match for this image, but has
                //  no valid content of this image
                //
                string renderID;
                {
                    HOP_CALL(glFinish();)
                    HOP_PROF("renderIntermediate - imageToFBOIdentifier");

                    renderID = imageToFBOIdentifier(root);

                    HOP_CALL(glFinish();)
                }
                {
                    HOP_CALL(glFinish();)
                    HOP_PROF("renderIntermediate - newImageFBO");

                    fbo = m_imageFBOManager
                              .newImageFBO(root, m_fullRenderSerialNumber,
                                           renderID)
                              ->fbo();

                    HOP_CALL(glFinish();)
                }

                context.norender = false;
            }
            HOP_CALL(glFinish();)
        }

        if (baseContext.norender)
            context.norender = true;

        context.mergeContext =
            renderType == IPImage::MergeRenderType ? &context : 0;
        context.activeImages.clear();

        context.targetFBO = fbo;
        context.targetLFBO = baseContext.targetLFBO;
        context.targetRFBO = baseContext.targetRFBO;
        context.image = root;
        context.frame = baseContext.frame;
        context.numImages = baseContext.numImages;
        context.device = baseContext.device;
        context.blendMode = baseContext.blendMode;
        context.fullSerialNum = baseContext.fullSerialNum;
        context.mainSerialNum = baseContext.mainSerialNum;
        context.auxRenderer = baseContext.auxRenderer;

        if (fbo)
        {
            {
                HOP_CALL(glFinish();)
                HOP_PROF("renderIntermediate - bind FBO");

                fbo->bind();

                HOP_CALL(glFinish();)
            }

            if (!context.norender)
            {
                if (root->useBackground)
                {
                    HOP_CALL(glFinish();)
                    HOP_PROF("renderIntermediate - clearBackground");

                    clearBackground(fbo);

                    HOP_CALL(glFinish();)
                }
                else
                {
                    HOP_CALL(glFinish();)
                    HOP_PROF("renderIntermediate - clearBackgroundToBlack");

                    clearBackgroundToBlack(fbo);

                    HOP_CALL(glFinish();)
                }
            }
        }

        {
            HOP_CALL(glFinish();)
            HOP_PROF("renderIntermediate - renderAllChildren");

            renderAllChildren(context);

            HOP_CALL(glFinish();)
        }

        //
        //  If this is a MergeRenderType all member images should be in
        //  flight or on the card by this time. So now the merge shader
        //  needs to be executed.
        //

        if (renderType == IPImage::MergeRenderType)
        {
            context.mergeRender = true; // tell ImageRenderer its a merge
            context.numImages = m_imageCount;
            context.imageFBO = fbo;
            context.mergeContext = 0; // don't use baseContext.mergeContext
                                      // here this is covered when
                                      // renderInternal is called later to
                                      // rasterize the result

            if (!context.norender && fbo)
            {
                HOP_CALL(glFinish();)
                HOP_PROF("renderIntermediate - clearBackgroundToBlack 2");

                clearBackgroundToBlack(fbo);

                HOP_CALL(glFinish();)
            }

            renderImage(context);

            context.imageFBO = NULL;
        }

        if (m_imageFBODebug && fbo)
        {
            //
            //  DEBUG WRITE THIS OUT AS AN EXR FILE
            //

            TwkFB::FrameBuffer fb(fbo->width(), fbo->height(), 4,
                                  TwkFB::FrameBuffer::HALF);
            vector<const TwkFB::FrameBuffer*> fbs(1);
            fbs.front() = &fb;

            glReadPixels(0, 0, fbo->width(), fbo->height(), GL_RGBA,
                         GL_HALF_FLOAT_ARB, fb.pixels<GLvoid>());

            ostringstream file;
            file << root->node->name() << "_" << root->imageNum << "_"
                 << root->graphID() << "." << m_mainRenderSerialNumber
                 << ".exr";
            TwkFB::GenericIO::writeImages(fbs, file.str(),
                                          TwkFB::FrameBufferIO::WriteRequest());
        }

        //
        //  Restore projection and FBO target
        //

        if (fbo)
        {

            fbo->unbind();

            if (baseContext.targetFBO)
                baseContext.targetFBO->bind();
        }

        {
            HOP_CALL(glFinish();)
            HOP_PROF("renderIntermediate - renderCurrentImage");

            renderCurrentImage(baseContext, fbo);

            HOP_CALL(glFinish();)
        }

        //
        //  If this was a TemporaryBuffer make it available immediately.
        //

        if (root->destination == IPImage::TemporaryBuffer
            && !baseContext.norender)
        {
            m_imageFBOManager.releaseImageFBO(fbo);
        }

        HOP_CALL(glFinish();)
    }

    void ImageRenderer::renderDataBuffer(InternalRenderContext& baseContext)
    {

#ifdef PLATFORM_DARWIN
        compileCLPrograms();
#endif

        //
        //  starts a new InternalRenderContext and
        //  either reuses an existing compatible FBO as the target or
        //  creates a new one.
        //

        InternalRenderContext context = baseContext;
        const GLFBO* fbo = NULL;
        const IPImage* root = baseContext.image;
        IPImage::RenderType renderType = root->renderType;

        // should have children and each child should be intermediate
        assert(root->renderType != IPImage::MergeRenderType);
        assert(root->children);

        for (IPImage* child = root->children; child; child = child->next)
        {
            assert(child->destination == IPImage::IntermediateBuffer);
        }

        if (!baseContext.norender)
        {
            // the framebuffer that contains the result of the computation
            if (ImageFBO* ifbo = findExistingImageFBO(root))
            {
                fbo = ifbo->fbo();
                context.norender = true;
            }
            else
            {
                string renderID = imageToFBOIdentifier(root);
                fbo = m_imageFBOManager
                          .newImageFBO(root, m_fullRenderSerialNumber, renderID)
                          ->fbo();
                context.norender = false;
            }
        }

        if (baseContext.norender)
            context.norender = true;

        context.mergeContext =
            renderType == IPImage::MergeRenderType ? &context : 0;
        context.targetFBO = baseContext.targetFBO;
        context.targetLFBO = baseContext.targetLFBO;
        context.targetRFBO = baseContext.targetRFBO;
        context.image = root;
        context.frame = baseContext.frame;
        context.numImages = baseContext.numImages;
        context.device = baseContext.device;
        context.blendMode = baseContext.blendMode;
        context.fullSerialNum = baseContext.fullSerialNum;
        context.mainSerialNum = baseContext.mainSerialNum;
        context.auxRenderer = baseContext.auxRenderer;

        if (fbo)
        {
            fbo->bind();
            if (!context.norender)
            {
                clearBackgroundToBlack(fbo);
            }
        }

        renderAllChildren(context);

        // run its opencl operation, write result to fbo
#ifdef PLATFORM_DARWIN
        try
        {
            if (!baseContext.norender) // && root->isHistogram)
            {
                ConstFBOVector childrenFBO;
                for (IPImage* child = root->children; child;
                     child = child->next)
                {
                    ImageFBO* ifbo = findExistingImageFBO(child);
                    assert(ifbo);
                    childrenFBO.push_back(ifbo->fbo());
                }
                computeHistogram(childrenFBO, fbo);
            }
        }
        catch (...)
        {
            cout << "OpenCL Initialization Failed" << endl;
            throw;
        }
#endif

        if (fbo)
        {
            fbo->unbind();

            if (baseContext.targetFBO)
                baseContext.targetFBO->bind();
        }

        renderCurrentImage(baseContext, fbo);
    }

    void
    ImageRenderer::renderNonIntermediate(InternalRenderContext& baseContext)
    {
        InternalRenderContext context = baseContext;

        // first render children recursively
        renderAllChildren(context);

        // then render current image
        renderCurrentImage(baseContext);
    }

    void ImageRenderer::renderExternal(InternalRenderContext& context)
    {
        const IPImage* image = context.image;
        const VideoDevice* device = context.device;
        const bool controller = device == m_controlDevice.device;
        const GLFBO* fbo = context.targetFBO;
        const AuxRender* auxRender = context.auxRenderer;
        IPImage::RenderDestination dest = image->destination;
        const bool left =
            dest == IPImage::MainBuffer || dest == IPImage::LeftBuffer;
        const bool right =
            dest == IPImage::MainBuffer || dest == IPImage::RightBuffer;

        context.norender = false;

        renderAllChildren(context);

        context.device = image->device;
        setupContextFromDevice(context);

        fbo->bind();

        if (auxRender)
        {
            m_rootContext = &context;
            GLState::FixedFunctionPipeline FFP(m_glState);
            FFP.setViewport(0, 0, fbo->width(), fbo->height());
            auxRender->render(VideoDevice::IndependentDisplayMode, left, right,
                              controller, !controller);
            m_rootContext = 0;
        }
    }

    void ImageRenderer::renderRootBuffer(InternalRenderContext& context)
    {
        //
        //  MainBuffer, LeftBuffer, or RightBuffer. These have to be
        //  treated a bit differently.
        //

        context.device = context.image->device;
        setupContextFromDevice(context);

        const VideoDevice* device = context.device;
        const bool controller = device == m_controlDevice.device;
        const GLFBO* fbo = context.targetFBO
                               ? context.targetFBO
                               : m_controlDevice.glDevice->defaultFBO();
        const AuxRender* auxRender = context.auxRenderer;
        IPImage::RenderDestination dest = context.image->destination;
        const bool left =
            dest == IPImage::MainBuffer || dest == IPImage::LeftBuffer;
        const bool right =
            dest == IPImage::MainBuffer || dest == IPImage::RightBuffer;

        //
        //  Don't touch this buffer yet if the device isn't done yet. This
        //  can happen in presentation mode with two monitors when the
        //  monitors don't have the same refresh rate. Eventually the
        //  clearBackground() call will stomp on the FBO that has yet to
        //  be fully flushed out to the other context.
        //
        //  This can be mitigated either by making the ringBufferSize > 1
        //  in which case the device has more than one frame to catch up,
        //  or waiting for the sync to complete before continuing.
        //
        //  NOTE: I still think its possible to get stomped on -- you can
        //  tell if that's happen by setting m_reportGL (-debug gpu in RV)
        //  which will cause some debug code to clear to blue. If you see
        //  blue flashing on the pres device that's the problem.
        //

        if (!controller && m_defaultDeviceFBORingBufferSize == 1 && device)
        {
            device->blockUntilSyncComplete();
        }

        if (!controller && fbo->state() == GLFBO::FenceInserted)
        {
            //
            //  If device was busy on last render we have to clear the
            //  fence
            //

            fbo->waitForFence();
        }

        if (controller && dest == IPImage::LeftBuffer)
        {
            glDrawBuffer(GL_BACK_LEFT);
        }
        else if (controller && dest == IPImage::RightBuffer)
        {
            glDrawBuffer(GL_BACK_RIGHT);
        }
        else
        {
            clearBackground(fbo);

            if (m_reportGL && !controller)
            {
                glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
                TWK_GLDEBUG;
                glClear(GL_COLOR_BUFFER_BIT);
                TWK_GLDEBUG;
            }
        }

        renderAllChildren(context);

        if (controller
            && (dest == IPImage::LeftBuffer || dest == IPImage::RightBuffer))
        {
            renderCurrentImage(context);
        }

        //
        //  For synchronizing "external" devices like SDI
        //

        if (!controller)
            m_imageFBOManager.insertFBOFence(fbo);
    }

    void ImageRenderer::renderRecursive(InternalRenderContext& baseContext)
    {
        HOP_CALL(glFinish();)
        HOP_PROF_FUNC();

        const IPImage* root = baseContext.image;

        if (root->isIntermediateRender())
        {
            //    IPImage::IntermediateBuffer:
            //    IPImage::TemporaryBuffer:
            //    If the image needs to be generated at render time into
            //    an IntermediateBuffer then allocate the buffer. The
            //    context is split here (differs from what was passed
            //    in) and reset to ground state.
            //

            //    IPImage::OutputTexture
            //    This is a "side-effect" of the rendering process. The
            //    image is not used in creation of the content imagery
            //
            renderIntermediate(baseContext);
        }
        else if (root->isExternalRender())
        {
            //
            //    Auxillary (user) render
            //
            renderExternal(baseContext);
        }
        else if (root->isRootRender())
        {
            //
            //    These are the root buffers (windows system, or
            //    external device).
            //
            renderRootBuffer(baseContext);
        }
        else if (root->isNoBuffer())
        {
            //
            //    Just ignore this one and continue
            //
            renderAllChildren(baseContext);
        }
        else if (root->isDataBuffer())
        {
            //
            //    root is an IPImage with a cpu or ocl operation on it
            //    which writes the result of the computation into the fbo of
            //    this image
            //
            renderDataBuffer(baseContext);
        }
        else
        {
            renderNonIntermediate(baseContext);
        }

        HOP_CALL(glFinish();)
    }

    void ImageRenderer::renderImage(InternalRenderContext& context)
    {
        const IPImage* i = context.image;

        if (i->ignore)
            return;

        TwkFB::FrameBuffer* fb = i->fb;
        if (fb && fb->width() == 0)
            return;

        //
        //  Stereo Eyes
        //

        TWK_GLDEBUG;

        if (!context.norender)
        {
            //
            //  Framebuffer modes
            //

            switch (context.blendMode)
            {
            case IPImage::UnspecifiedBlendMode:
            case IPImage::Replace:
                glDisable(GL_BLEND);
                break;
            case IPImage::Over:
            case IPImage::Dissolve:
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                                    GL_ONE_MINUS_SRC_ALPHA);
                break;
            case IPImage::Add:
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE);
                break;
            case IPImage::Difference:
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_SUBTRACT);
                glBlendFunc(GL_ONE, GL_ONE);
                break;
            case IPImage::ReverseDifference:
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                glBlendFunc(GL_ONE, GL_ONE);
                break;
            }
        }

        try
        {
            renderInternal(context);

            if (context.mergeContext)
            {
                context.mergeContext->activeImages.push_back(i);

                // cout << "MERGE MEMBER: " << i->graphID() << endl
                //       << " ---> " << context.mergeContext->image->graphID()
                //       << endl;
            }
        }
        catch (...)
        {
            cout << "WARNING: render failed while rendering " << i->graphID()
                 << endl;
            throw;
        }

        glBlendEquation(GL_FUNC_ADD);
        glDisable(GL_BLEND);
    }

    //----------------------------------------------------------------------
    //
    //  GL Draw related
    //
    void ImageRenderer::clearBackgroundToBlack(const GLFBO* target)
    {
        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::clearBackgroundToBlack - target->bind");

            target->bind();

            HOP_CALL(glFinish();)
        }
        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::clearBackgroundToBlack - glClearColor");

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            TWK_GLDEBUG;

            HOP_CALL(glFinish();)
        }
        {
            HOP_CALL(glFinish();)
            HOP_PROF("ImageRenderer::clearBackgroundToBlack - glClear");

            glClear(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;

            HOP_CALL(glFinish();)
        }
        TWK_GLDEBUG;
    }

    void ImageRenderer::clearBackground(const GLFBO* target)
    {
        target->bind();
        if (m_bgpattern < Checker)
            m_glState->useGLProgram(defaultGLProgram());
        else if (m_bgpattern == Checker)
            m_glState->useGLProgram(checkerBGGLProgram());
        else
            m_glState->useGLProgram(crosshatchBGGLProgram());

        GLPipeline* glPipeline = m_glState->activeGLPipeline();
        glPipeline->setViewport(0, 0, GLuint(target->width()),
                                GLuint(target->height()));

        float grey = 0.0;
        switch (m_bgpattern)
        {
        case Solid0:
            break;
        case Solid18:
            grey = .18f;
            break;
        case Solid50:
            grey = .50f;
            break;
        case Solid100:
            grey = 1.0f;
            break;
        case Checker:
            grey = .22f;
            break;
        case CrossHatch:
            grey = .18f;
            break;
        }

        glClearColor(grey, grey, grey, 0.0f);
        TWK_GLDEBUG;
        glClear(GL_COLOR_BUFFER_BIT);
        TWK_GLDEBUG;
        TWK_GLDEBUG;
        //
        //  If you're seeing a GL error after glClear() on the mac only it
        //  might be because there's a Cocoa bug which causes the default
        //  frame buffer to appear inconstitent in GL surfaces before they
        //  are shown for the first time.
        //
        //  This is supposedly an issue when using the 10.6 and 10.7 SDKs
        //

        if (m_bgpattern >= Checker)
        {
            Mat44f identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
            glPipeline->setModelview(identity);
            glPipeline->setProjection(identity);

            float data[] = {-1.0,
                            -1.0,
                            0,
                            0,
                            1.0,
                            -1.0,
                            static_cast<float>(target->width()),
                            0,
                            1.0,
                            1.0,
                            static_cast<float>(target->width()),
                            static_cast<float>(target->height()),
                            -1.0,
                            1.0,
                            0,
                            static_cast<float>(target->height())};

            PrimitiveData databuffer(data, NULL, GL_QUADS, 4, 1,
                                     sizeof(float) * 16);

            vector<VertexAttribute> attributeInfo;
            attributeInfo.push_back(
                VertexAttribute("in_Position", GL_FLOAT, 2, 0,
                                4 * sizeof(float))); // vertex

            attributeInfo.push_back(
                VertexAttribute("in_TexCoord0", GL_FLOAT, 2, 2 * sizeof(float),
                                4 * sizeof(float))); // texture

            RenderPrimitives renderprimitives(m_glState->activeGLProgram(),
                                              databuffer, attributeInfo,
                                              m_glState->vboList());
            renderprimitives.setupAndRender();

            TWK_GLDEBUG;
        }
    }

    void ImageRenderer::setupBlendMode(InternalRenderContext& context) const
    {
        const IPImage* image = context.image;

        if (image->blendMode != IPImage::UnspecifiedBlendMode)
        {
            context.blendMode = image->blendMode;
        }
    }

    //----------------------------------------------------------------------
    //
    //  Image FBO management
    //

    void ImageRenderer::markReusableImageFBOs(const IPImage* root)
    {
        //
        // this is called at the beginning of a frame (see render() func)
        // the point is if there are any ImageFBO we can reuse for this frame,
        // we will mark it as unavailable (so that the renderer cannot grab it
        // and use it)
        //
        for (IPImage* child = root->children; child; child = child->next)
        {
            markReusableImageFBOs(child);
        }

        if (root->isDataBuffer() || root->isIntermediateRender())
        {
            string renderID = imageToFBOIdentifier(root);
            // if found one, findExistingImageFBO will mark it unavailable and
            // update serialNum
            m_imageFBOManager.findExistingImageFBO(renderID,
                                                   m_fullRenderSerialNumber);
        }
    }

    string ImageRenderer::imageToFBOIdentifier(const IPImage* image) const
    {
        //
        // the content of a ImageFBO is determined by the IPImage
        // as well as global settings of RV/Crank: such as filter mode,
        // background mode, etc.
        //
        ostringstream o;
        o << image->renderIDHash();
        o << "ft" << m_filter;
        o << "bg" << m_bgpattern;
        return o.str();
    }

    ImageFBO* ImageRenderer::findExistingImageFBO(const IPImage* image)
    {
        // this func will return 0 if no match is found
        if (m_noGPUCache)
            return 0;
        string renderID = imageToFBOIdentifier(image);
        return m_imageFBOManager.findExistingImageFBO(renderID,
                                                      m_fullRenderSerialNumber);
    }

    ImageFBO* ImageRenderer::findExistingImageFBO(GLuint textureID)
    {
        return m_imageFBOManager.findExistingImageFBO(textureID);
    }

    ImageFBO* ImageRenderer::newOutputOnlyImageFBO(GLenum format,
                                                   size_t samples)
    {
        return m_imageFBOManager.newOutputOnlyImageFBO(
            format, m_outputDevice.device->width(),
            m_outputDevice.device->height(), samples);
    }

    //----------------------------------------------------------------------
    //
    //  Prefetch
    //
    void ImageRenderer::prefetch(const IPImage* img)
    {
        // this func uploads framebuffers to card
        if (!queryRectTextures())
        {
            throw RendererNotSupportedExc();
        }

        // is this needed ? YES this is needed, otherwise fonts wont render
        // right with FTGL
        GLPushAttrib attr1(GL_ALL_ATTRIB_BITS);
        GLPushClientAttrib attr2(GL_CLIENT_ALL_ATTRIB_BITS);
        TWK_GLDEBUG;

        prefetchRecursive(img);
    }

    void ImageRenderer::prefetchInternal(const IPImage* img)
    {
        TwkFB::FrameBuffer* fb = img ? img->fb : 0;

        if (!img || (fb && fb->width() == 0))
            return;

#if defined(HOP_ENABLED)
        std::string hopMsg = std::string("ImageRenderer::prefetchInternal()");
        if (fb)
        {
            hopMsg += std::string(" - width=") + std::to_string(fb->width())
                      + std::string(", height=") + std::to_string(fb->height());
        }
        HOP_PROF_DYN_NAME(hopMsg.c_str());
#endif

        //
        //  upload all Framebuffers: source and aux (luts, etc.)
        //  the renderer assembles all the textures to be uploaded into
        //  'm_texturesToUpload'
        //
        if (fb)
        {
            for (const FrameBuffer* p = fb; p; p = p->nextPlane())
            {
                FBToTextureMap::iterator it;
                it = m_texturesToUpload.find(p->identifier());
                if (it != m_texturesToUpload.end())
                {
                    uploadPlane(p, it->second, m_filter);
                    m_texturesToUpload.erase(it->first);
                }
            }
        }

        for (size_t q = 0; q < img->auxFBs.size(); q++)
        {
            FBToTextureMap::iterator it;
            it = m_texturesToUpload.find(img->auxFBs[q]->identifier());
            if (it != m_texturesToUpload.end())
            {
                uploadPlane(img->auxFBs[q], it->second, m_filter);
                m_texturesToUpload.erase(it->first);
            }
        }

        for (size_t q = 0; q < img->auxMergeFBs.size(); q++)
        {
            FBToTextureMap::iterator it;
            it = m_texturesToUpload.find(img->auxMergeFBs[q]->identifier());
            if (it != m_texturesToUpload.end())
            {
                uploadPlane(img->auxMergeFBs[q], it->second, m_filter);
                m_texturesToUpload.erase(it->first);
            }
        }

        HOP_CALL(glFinish();)
    }

    void ImageRenderer::prefetchRecursive(const IPImage* root)
    {
        for (const IPImage* i = root->children; i; i = i->next)
        {
            prefetchRecursive(i);
        }

        //
        //  The order here has to match that in renderRecursive, and
        //  renderRecursive does not check for i->fb so we cannot either.
        //  If we do, the order of the hash/identifer compares is
        //  different and we end up over-uploading.
        //

        if (root->destination != IPImage::NoBuffer
            && root->destination != IPImage::MainBuffer
            && root->destination != IPImage::LeftBuffer
            && root->destination != IPImage::RightBuffer
            && root->renderType != IPImage::GroupType)
        {
            prefetchInternal(root);
        }
    }

    //----------------------------------------------------------------------
    //
    //  Render frame
    //

    void ImageRenderer::renderInternal(const InternalRenderContext& context)
    {
        const GLFBO* imageFBO = context.imageFBO;
        int frame = context.frame;
        const IPImage* img = context.image;
        bool norender = context.norender;

        assert(img->renderType != IPImage::GroupType);
        assert(img->destination != IPImage::NoBuffer);

        TwkFB::FrameBuffer* fb = img ? img->fb : 0;

        if (!img || (fb && fb->width() == 0))
            return;

        GLPushAttrib attr1(GL_ALL_ATTRIB_BITS);
        TWK_GLDEBUG;

        //
        //  Assign (and possibly upload) all image planes and LUTs
        //
        LogicalImage* limage =
            m_imagePassStates[img->renderIDHash()]->primaryImage();
        limage->imageNum = img->imageNum;

        assignImage(limage, img, imageFBO, norender, context.mergeContext,
                    context.fullSerialNum);

        assignAuxImages(img);

        assignMemberImages(img, context.activeImages);

        //
        //  If a context has mergeContext defined that means it should not
        //  be rendered directly (it will be part of some ancestor's
        //  merge)
        //

        if (!context.mergeContext && !norender
            && img->destination != IPImage::DataBuffer)
        {
            const Shader::Program* prog = 0;

            Shader::Expression* expr =
                context.mergeRender ? img->mergeExpr : img->shaderExpr;

            if ((!limage->isvirtual || context.mergeRender) && expr)
            {
                try
                {
                    prog = m_programCache->select(expr);
                    m_glState->useGLProgram(prog);
                }
                catch (std::exception& e)
                {
                    cerr << "ERROR: failed to compile/select GL program: "
                         << e.what() << endl;
                    return;
                }

                markProgramRequirements(img, context, prog);
                assignTextureUnits(img, context.mergeRender);
                prog->bind(expr, m_unitAssignments, img->viewport.size());
            }
            else
            {
                assignTextureUnits(img, context.mergeRender);
            }

            //
            //  Draw. How should drawPaint be handled here?
            //

            if (context.mergeRender)
            {
                drawMerge(context);
            }
            else
            {
                drawImage(context);
            }
        }

        recordTransforms(context);

        // go back to use the Default Program
        m_glState->useGLProgram(defaultGLProgram());
    }

    void
    ImageRenderer::markProgramRequirements(const IPImage* img,
                                           const InternalRenderContext& context,
                                           const Shader::Program* program)
    {
        typedef Shader::Program::GraphIDSet GraphIDSet;

        const GraphIDSet& outputSTSet = program->outputSTSet();
        const GraphIDSet& outputSizeSet = program->outputSizeSet();

        if (size_t n = context.activeImages.size())
        {
            for (size_t i = 0; i < n; i++)
            {
                const IPImage* activeImage = context.activeImages[i];
                LogicalImage* image =
                    m_imagePassStates[activeImage->renderIDHash()]
                        ->primaryImage();
                string graphid = image->ipToGraphIDs[activeImage];
                image->outputST = outputSTSet.count(graphid) > 0;
                image->outputSize = outputSizeSet.count(graphid) > 0;
            }
        }
        else
        {
            ImagePassState* istate = m_imagePassStates[img->renderIDHash()];
            LogicalImage* image = istate->primaryImage();
            string graphid = image->ipToGraphIDs[img];
            image->outputST = outputSTSet.count(graphid) > 0;
            image->outputSize = outputSizeSet.count(graphid) > 0;
        }
    }

    void ImageRenderer::activateTextures(LogicalImageVector& images)
    {
        // this is where the filter mode is communicated to GL
        for (size_t i = 0; i < images.size(); i++)
        {
            LogicalImage* aimage = &images[i];
            int filterMode;
            // For textures, like LUTs, which...
            // - have no source string,
            // - are not rendered and
            // - arent a primary texture.
            // We always set the filter mode to GL_LINEAR.
            if (aimage->source.empty() && !aimage->render && !aimage->isPrimary)
            {
                filterMode = GL_LINEAR;
            }
            else
            {
                filterMode = m_filter;
            }

#if 0
        cout << "**** activateTextures:" << aimage->source
            << " idhash=" << aimage->idhash 
            << " fbhash=" << aimage->fbhash 
            << " render=" << (int) aimage->render 
            << " isPrimary=" << (int) aimage->isPrimary 
            << endl;
#endif

            for (size_t q = 0; q < aimage->planes.size(); q++)
            {
                ImagePlane& plane = aimage->planes[q];

                size_t id = plane.tile->id;
                // wait until upload thread is done with this pbo
                TwkUtil::Timer* timer = m_profilingState.profilingTimer;
                double start = (timer) ? timer->elapsed() : 0.0;
                if (timer)
                    m_profilingState.fenceWaitTime += timer->elapsed() - start;

                glActiveTextureARB(GL_TEXTURE0_ARB + plane.textureUnit);
                TWK_GLDEBUG;
                glEnable(plane.tile->target);
                TWK_GLDEBUG;
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                TWK_GLDEBUG;
                glBindTexture(plane.tile->target, id);
                TWK_GLDEBUG;
                glTexParameteri(plane.tile->target, GL_TEXTURE_MIN_FILTER,
                                filterMode);
                glTexParameteri(plane.tile->target, GL_TEXTURE_MAG_FILTER,
                                filterMode);
            }
        }
    }

    void ImageRenderer::computeMergeMatrix(const InternalRenderContext& context,
                                           const LogicalImage* logicalImage,
                                           const IPImage* ipImage,
                                           Mat44f& mergeMatrix) const
    {
        //
        //  This code reprojects the image coordinates into texture
        //  coordinates on another image.
        //
        //  First, transform the image to screen space coords with the X
        //  dimension going from [-aspect, aspect]
        //
        //  Then apply the inverse of the image matrix -- since the image
        //  may be flipped or flopped use its orientation matrix to correct
        //  that.
        //
        //  Finally, project the normalized image into the merge
        //  space. The merge space is different from each of the merge
        //  children -- the texture coords are
        //

        const IPImage* mimage = context.image;
        const float iw = mimage->width;
        const float ih = mimage->height;
        const float maspect = iw / ih;

        Matrix S0, T1, T0, S1, S2;

        float w = logicalImage->width;
        float h = logicalImage->height;

        const float aspect = w / h;
        const float ascale = maspect / aspect;

        T1.makeTranslation(Vec3f(-w / 2.0, -h / 2.0, 0.f));
        S0.makeScale(Vec3f(1.0f / h * ascale, 1.0f / h, 1.0f));
        S1.makeScale(Vec3f(h, h, 1.0f));
        T0.makeTranslation(Vec3f(w / 2.0, h / 2.0, 0.f));

        Matrix M = ipImage->transformMatrix.inverted();
        mergeMatrix = T0 * S1 * M * S0 * T1;
    }

    void ImageRenderer::computeDataBufferMergeMatrix(
        const InternalRenderContext& context, const IPImage* ipImage,
        Mat44f& mergeMatrix) const
    {
        assert(ipImage->destination == IPImage::DataBuffer);

        const IPImage* mimage = context.image;
        const float iw = mimage->width;
        const float ih = mimage->height;
        const float maspect = iw / ih;

        Matrix S0, T1, T0, S1, S2;

        const float ascale = 1.0;

        T1.makeTranslation(Vec3f(-iw / 2.0, -ih / 2.0, 0.f));
        S0.makeScale(Vec3f(1.0f / ih * ascale, 1.0f / ih, 1.0f));
        S1.makeScale(Vec3f(ih, ih, 1.0f));
        T0.makeTranslation(Vec3f(iw / 2.0, ih / 2.0, 0.f));

        Matrix M = ipImage->transformMatrix.inverted();
        mergeMatrix = T0 * S1 * M * S0 * T1;
    }

    void ImageRenderer::recordTransforms(const InternalRenderContext& context)
    {
        //
        // this func records info which will eventually be used by the UI
        //
        const IPImage* img = context.image;
        ImagePassState* istate = m_imagePassStates[img->renderIDHash()];
        LogicalImage* image = istate->primaryImage();

        image->devices.insert((VideoDevice*)context.device);

        if (context.mergeRender)
            return; // do not need to record these

        m_renderedImages.resize(m_renderedImages.size() + 1);
        RenderedImage& s = m_renderedImages.back();

        s.isVirtual = image->isvirtual;
        s.render = image->render;
        s.touched = image->touched;
        s.source = image->source;
        s.serialNum = image->serialNum;
        s.imageNum = image->imageNum;
        s.textureID =
            (image->planes.size()) ? image->planes.front().tile->id : 0;
        s.node = image->node;
        s.tagMap = image->tagMap;
        s.index = img->fb ? img->fbHash() : img->renderIDHash();
        s.imageBox = Box2f(Vec2f(0, 0), Vec2f(1.0, 1.0));
        s.stencilBox = Box2f(Vec2f(image->stencilMin.x, image->stencilMin.y),
                             Vec2f(image->stencilMax.x, image->stencilMax.y));
        s.modelMatrix = img->imageMatrix;
        s.globalMatrix = img->modelViewMatrixGlobal;
        s.projectionMatrix = img->projectionMatrixGlobal;
        s.orientationMatrix = img->orientationMatrix;
        s.placementMatrix = img->placementMatrix;
        s.textureMatrix = Matrix();
        s.width = image->width;
        s.height = image->height;
        s.uncropWidth = image->uncropWidth;
        s.uncropHeight = image->uncropHeight;
        s.uncropX = image->uncropX;
        s.uncropY = image->uncropY;
        s.planar = image->planes.size() > 1;
        s.numChannels = s.planar ? image->planes.size()
                                 : (image->planes.size()
                                        ? image->planes.front().tile->channels
                                        : 4);
        s.pixelAspect = image->pixelAspect;
        s.initPixelAspect = image->initPixelAspect;
        s.device = (VideoDevice*)context.device;

        GLuint channelType = image->planes.empty()
                                 ? GL_FLOAT
                                 : image->planes.front().tile->channelType;

        switch (channelType)
        {
#ifdef GL_FLOAT_R16_NV
        case GL_FLOAT_R16_NV:
#endif
        case GL_HALF_FLOAT_ARB:
            s.bitDepth = 16;
            s.floatingPoint = true;
            break;
        case GL_FLOAT:
            s.bitDepth = 32;
            s.floatingPoint = true;
            break;
#ifdef GL_UNSIGNED_SHORT_8_8_APPLE
        case GL_UNSIGNED_SHORT_8_8_APPLE:
        case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
#endif
        case GL_UNSIGNED_SHORT:
            s.bitDepth = 16;
            s.floatingPoint = false;
            break;
        case GL_UNSIGNED_INT_8_8_8_8_REV:
        case GL_UNSIGNED_BYTE:
            s.bitDepth = 8;
            s.floatingPoint = false;
            break;
        case GL_UNSIGNED_INT_10_10_10_2:
        case GL_UNSIGNED_INT_2_10_10_10_REV:
            s.bitDepth = 10;
            s.floatingPoint = false;
            break;
        }
    }

    void ImageRenderer::drawImage(const InternalRenderContext& context)
    {
        const IPImage* img = context.image;
        ImagePassState* istate = m_imagePassStates[img->renderIDHash()];
        LogicalImage* image = istate->primaryImage();

        // in this case (for instance audio texture) we need to call
        // activatePrimaryTexture which uploads the texture data from pbo to
        // texture
        if (img->destination == IPImage::OutputTexture
            || img->destination == IPImage::DataBuffer)
        {
            activateTextures(istate->images);
        }

        if (image->planes.empty() || !image->render || image->isvirtual)
            return;

        const bool merge = context.mergeRender;
        image->touched = true;

        glColor4f(1.0, 1.0, 1.0, 1.0);
        TWK_GLDEBUG; // IS THIS NEEDED?

        GLPipeline* glPipeline = m_glState->activeGLPipeline();

        const IPImage::Matrix& PM = img->projectionMatrix;
        const IPImage::Matrix& MV = img->modelViewMatrix;
        const IPImage::Box2& viewport = img->viewport;

        glPipeline->setProjection(PM);
        glPipeline->setModelview(MV);
        glPipeline->setViewport(viewport.min.x, viewport.min.y,
                                viewport.max.x - viewport.min.x,
                                viewport.max.y - viewport.min.y);

        vector<float> data(16);
        ImagePlane& plane = image->planes[0];
        TextureDescription* d = plane.tile;

        computeImageGeometry(img, plane);

        //
        //  Output the image rect with all of the tex coordinates
        //

        activateTextures(istate->images);

        data[0] = plane.x0;
        data[1] = plane.y0;
        data[2] = plane.originX;
        data[3] = plane.originY;
        data[4] = plane.x1;
        data[5] = plane.y0;
        data[6] = plane.endX;
        data[7] = plane.originY;
        data[8] = plane.x1;
        data[9] = plane.y1;
        data[10] = plane.endX;
        data[11] = plane.endY;
        data[12] = plane.x0;
        data[13] = plane.y1;
        data[14] = plane.originX;
        data[15] = plane.endY;

        if (img->textureMatrix != Mat33f())
        {
            Vec3f v1(data[2], data[3], 1.0f);
            Vec3f res1 = img->textureMatrix * v1;
            data[2] = res1.x;
            data[3] = res1.y;

            Vec3f v2(data[6], data[7], 1.0f);
            Vec3f res2 = img->textureMatrix * v2;
            data[6] = res2.x;
            data[7] = res2.y;

            Vec3f v3(data[10], data[11], 1.0f);
            Vec3f res3 = img->textureMatrix * v3;
            data[10] = res3.x;
            data[11] = res3.y;

            Vec3f v4(data[14], data[15], 1.0f);
            Vec3f res4 = img->textureMatrix * v4;
            data[14] = res4.x;
            data[15] = res4.y;
        }

        ostringstream texcoordname;
        texcoordname << "in_TexCoord" << image->ipToGraphIDs[img];

        PrimitiveData databuffer(&data.front(), NULL, GL_QUADS, 4, 1,
                                 sizeof(float) * 16);

        vector<VertexAttribute> attributeInfo;

        attributeInfo.push_back(VertexAttribute("in_Position", GL_FLOAT, 2, 0,
                                                4 * sizeof(float))); // vertex

        attributeInfo.push_back(VertexAttribute(texcoordname.str(), GL_FLOAT, 2,
                                                2 * sizeof(float),
                                                4 * sizeof(float))); // texture

        RenderPrimitives renderprimitives(m_glState->activeGLProgram(),
                                          databuffer, attributeInfo,
                                          m_glState->vboList());

        renderprimitives.setupAndRender();
        TWK_GLDEBUG;

        glActiveTextureARB(GL_TEXTURE0_ARB);
    }

    void ImageRenderer::assignMemberImages(
        const IPImage* img,
        const ImageRenderer::ActiveImageVector& activeImages)
    {
        ImagePassState* istate = m_imagePassStates[img->renderIDHash()];

        size_t n = activeImages.size();
        istate->memberImages.resize(n);

        for (size_t i = 0; i < n; i++)
        {
            istate->memberImages[i] = activeImages[i];
        }
    }

    void ImageRenderer::drawMerge(const InternalRenderContext& context)
    {
        const IPImage* img = context.image;
        ImagePassState* istate = m_imagePassStates[img->renderIDHash()];
        const LogicalImage* image = istate->primaryImage();
        const float w = image->width;
        const float h = image->height;
        const float a = w / h;
        const float x0 = -a / 2.0f;
        const float x1 = a / 2.0f;
        const float y0 = -0.5;
        const float y1 = 0.5;

        glColor4f(1.0, 1.0, 1.0, 1.0);
        TWK_GLDEBUG; // NEEDED?

        assert(img->children != NULL);

        //
        //  Set uniforms
        //

        GLPipeline* glPipeline = m_glState->activeGLPipeline();
        const IPImage::Matrix& PM = img->children->projectionMatrix;
        const IPImage::Box2& viewport = img->children->viewport;

        glPipeline->setProjection(PM);
        glPipeline->setModelview(Mat44f());
        glPipeline->setViewport(viewport.min.x, viewport.min.y,
                                viewport.max.x - viewport.min.x,
                                viewport.max.y - viewport.min.y);

        //  Active member image textures
        for (size_t i = 0; i < istate->memberImages.size(); i++)
        {
            ImagePassState* mstate =
                m_imagePassStates[istate->memberImages[i]->renderIDHash()];
            activateTextures(mstate->images);
        }

        activateTextures(istate->mergeImages);

        vector<float> data;
        // vertex
        data.push_back(x0);
        data.push_back(y0);
        data.push_back(x1);
        data.push_back(y0);
        data.push_back(x1);
        data.push_back(y1);
        data.push_back(x0);
        data.push_back(y1);

        vector<VertexAttribute> attributeInfo;

        attributeInfo.push_back(
            VertexAttribute("in_Position", GL_FLOAT, 2, 0,
                            2 * sizeof(float))); // first all vertices

        size_t count = 0;

        for (size_t j = 0; j < istate->memberImages.size(); j++)
        {
            const IPImage* img2 = istate->memberImages[j];
            ImagePassState* mstate = m_imagePassStates[img2->renderIDHash()];
            LogicalImage* image = mstate->primaryImage();

            if (image->coordinateSet == NOT_A_COORDINATE)
                continue;

            ostringstream texcoordstr;
            texcoordstr << "in_TexCoord" << image->ipToGraphIDs[img2];
            string tname = texcoordstr.str(); // windows

            attributeInfo.push_back(VertexAttribute(
                tname, GL_FLOAT, 2, (8 + 8 * count) * sizeof(float),
                2 * sizeof(float))); // texture

            if (img2->destination == IPImage::DataBuffer)
            {
                Mat44f mergeMatrix;
                computeDataBufferMergeMatrix(context, img2, mergeMatrix);
                Vec4f t = mergeMatrix * Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);

                t = mergeMatrix * Vec4f(w, 0.0f, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);

                t = mergeMatrix * Vec4f(w, h, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);

                t = mergeMatrix * Vec4f(0.0f, h, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);
            }
            else
            {
                Mat44f mergeMatrix;
                computeMergeMatrix(context, image, img2, mergeMatrix);
                Vec4f t = mergeMatrix * Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);

                t = mergeMatrix * Vec4f(image->width, 0.0f, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);

                t = mergeMatrix
                    * Vec4f(image->width, image->height, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);

                t = mergeMatrix * Vec4f(0.0f, image->height, 0.0f, 1.0f);
                data.push_back(t.x);
                data.push_back(t.y);
            }
            count++;
        }

        PrimitiveData databuffer(
            &(data[0]), NULL, GL_QUADS, 4, 1,
            sizeof(float)
                * (8 + 8 * count)); // 8 for vertex, 8 per texcoord set

        RenderPrimitives renderprimitives(glPipeline->glProgram(), databuffer,
                                          attributeInfo, m_glState->vboList());

        renderprimitives.setupAndRender();

        glActiveTextureARB(GL_TEXTURE0_ARB);
        TWK_GLDEBUG;
    }

    //----------------------------------------------------------------------
    //
    //  Assign texture units
    //

    void ImageRenderer::assignTextureUnits(const IPImage* img, bool mergeRender)
    {
        m_unitAssignments.clear();

        ImagePassState* istate = m_imagePassStates[img->renderIDHash()];

        if (!mergeRender)
        {
            size_t tunit = 0;
            size_t stunit = 0;
            for (size_t i = 0; i < istate->images.size(); ++i)
            {
                assignTextureUnits2(img, &istate->images[i], tunit, stunit);
            }
        }
        else
        {
            //
            // There could be cases where some of these logical images
            // correspond to the same buffer on the card, in that case we want
            // all of them to share the same texture unit. Here we build a map
            // from 'texturehash' to texture unit
            //
            size_t cunit = 0;
            map<string, size_t> framebufferToTextureUnit;
            map<string, size_t>::iterator it;

            for (size_t i = 0; i < istate->memberImages.size(); i++)
            {
                const IPImage* img2 = istate->memberImages[i];
                LogicalImageVector& images =
                    m_imagePassStates[img2->renderIDHash()]->images;
                for (size_t j = 0; j < images.size(); j++)
                {
                    LogicalImage* image = &images[j];
                    if (image->texturehash.empty())
                    {
                        if (!image->fbhash.empty())
                        {
                            image->texturehash = image->fbhash;
                            cerr << "ERROR: Texture has an empty texturehash. "
                                    "Using fbhash."
                                 << endl;
                        }
                        else
                        {
                            cerr << "ERROR: Texture has an empty texturehash "
                                    "and fbhash."
                                 << endl;
                        }
                    }
                    it = framebufferToTextureUnit.find(image->texturehash);
                    if (it != framebufferToTextureUnit.end())
                        continue;

                    framebufferToTextureUnit[image->texturehash] = cunit;
                    size_t totalPlanes = max(size_t(1), image->planes.size());
                    cunit += totalPlanes;
                }
            }

            for (size_t i = 0; i < img->auxMergeFBs.size(); ++i)
            {
                LogicalImage* image = &istate->mergeImages[i];
                if (image->texturehash.empty())
                {
                    if (!image->fbhash.empty())
                    {
                        image->texturehash = image->fbhash;
                        cerr << "ERROR: Texture has an empty texturehash. "
                                "Using fbhash."
                             << endl;
                    }
                    else
                    {
                        cerr << "ERROR: Texture has an empty texturehash and "
                                "fbhash."
                             << endl;
                    }
                }
                it = framebufferToTextureUnit.find(image->texturehash);
                if (it != framebufferToTextureUnit.end())
                    continue;

                framebufferToTextureUnit[image->texturehash] = cunit;
                size_t totalPlanes = image->planes.size();
                cunit += totalPlanes;
            }

            // now ready to assign
            size_t stunit = 0;
            for (size_t i = 0; i < istate->memberImages.size(); i++)
            {
                const IPImage* img2 = istate->memberImages[i];
                LogicalImageVector& images =
                    m_imagePassStates[img2->renderIDHash()]->images;
                for (size_t j = 0; j < images.size(); j++)
                {
                    LogicalImage* image = &images[j];
                    size_t tunit = framebufferToTextureUnit[image->texturehash];
                    bool assigned =
                        assignTextureUnits2(img2, image, tunit, stunit);
                    if (assigned)
                        stunit++;
                }
            }

            for (size_t i = 0; i < img->auxMergeFBs.size(); i++)
            {
                LogicalImage* image = &istate->mergeImages[i];
                size_t tunit = framebufferToTextureUnit[image->texturehash];
                bool assigned = assignTextureUnits2(img, image, tunit, stunit);
                if (assigned)
                    stunit++;
            }
        }
    }

    bool ImageRenderer::assignTextureUnits2(const IPImage* img,
                                            LogicalImage* image, size_t& tunit,
                                            size_t stunit)
    {
        if (image->planes.empty() && !image->outputST)
            return false;

        size_t startIndex = m_unitAssignments.size();

        size_t totalPlanes = max(size_t(1), image->planes.size());
        m_unitAssignments.resize(totalPlanes + startIndex);

        size_t uindex = startIndex;
        image->coordinateSet = stunit;

        if (image->planes.empty())
        {
            ImageAndCoordinateUnit& a = m_unitAssignments[uindex];

            a.textureUnit = NOT_A_TEXTURE_UNIT;
            a.coordinateSet = image->coordinateSet;
            a.textureTarget = NOT_A_TARGET;
            a.plane = 0;
            a.idhash = image->fbhash; // not using idhash here!
            a.graphID = img->graphID();
            a.hasST = true;
            a.width = image->width;
            a.height = image->height;
            a.uncropWidth = image->uncropWidth;
            a.uncropHeight = image->uncropHeight;
            a.uncropX = 0;
            a.uncropY = 0;
            a.outputSize = image->outputSize;
        }
        else
        {
            for (size_t q = 0; q < image->planes.size(); q++, tunit++, uindex++)
            {
                ImagePlane& plane = image->planes[q];
                plane.textureUnit = tunit;
                ImageAndCoordinateUnit& a = m_unitAssignments[uindex];

                if (!image->isPrimary)
                    image->coordinateSet =
                        NOT_A_COORDINATE; // aux images have no coordinates

                a.textureUnit = plane.textureUnit;
                a.coordinateSet = image->coordinateSet;
                a.textureTarget = plane.tile->target;
                a.plane = q;
                a.idhash = image->fbhash; // not using idhash here!
                a.width = plane.width;
                a.height = plane.height;
                a.uncropWidth = plane.tile->uncropWidth;
                a.uncropHeight = plane.tile->uncropHeight;
                a.uncropX = plane.tile->uncropX;
                a.uncropY = plane.tile->uncropY;

                a.graphID = image->isPrimary ? img->graphID() : "";
                a.hasST = image->isPrimary ? true : false;
                a.outputSize = image->isPrimary && image->outputSize && q == 0;
            }
        }

        return image->isPrimary ? true : false;
    }

    //----------------------------------------------------------------------
    //
    //  Functions that operate ImagePassState
    //

    void ImageRenderer::clearImagePassStates()
    {
        ImagePassStateMap::iterator it;
        for (it = m_imagePassStates.begin(); it != m_imagePassStates.end();
             it++)
        {
            delete it->second;
        }
        m_imagePassStates.clear();
    }

    void ImageRenderer::assembleImagePassStates(const IPImage* root)
    {
        //
        // go through all images of this render
        // assemble the needed texturedescription
        //
        for (const IPImage* i = root->children; i; i = i->next)
        {
            assembleImagePassStates(i);
        }

        HashValue renderIDHash = root->renderIDHash();
        if (!m_imagePassStates.count(renderIDHash))
        {
            m_imagePassStates[renderIDHash] = new ImagePassState();
        }

        ImagePassState* ips = m_imagePassStates[renderIDHash];

        size_t totalImage = 1 + root->auxFBs.size();
        size_t totalMergeImage = root->auxMergeFBs.size();
        ips->images.resize(totalImage);
        ips->mergeImages.resize(totalMergeImage);

        assemblePrimaryLogicalImage(root, ips->primaryImage());

        for (size_t i = 0; i < root->auxFBs.size(); ++i)
        {
            assembleAuxLogicalImage(root->auxFBs[i], &ips->images[i + 1]);
        }

        for (size_t i = 0; i < root->auxMergeFBs.size(); ++i)
        {
            assembleAuxLogicalImage(root->auxMergeFBs[i], &ips->mergeImages[i]);
        }
    }

    void ImageRenderer::assembleAuxLogicalImage(const FrameBuffer* fb,
                                                LogicalImage* i)
    {
        i->isPrimary = false;

        i->planes.resize(1);

        // find the right texture description
        FBToTextureMap::iterator it = m_uploadedTextures.find(fb->identifier());
        if (it != m_uploadedTextures.end())
        {
            i->planes[0].tile = it->second;
        }
        else
        {
            TWK_THROW_STREAM(RenderFailedExc,
                             "Cannot find texture description for auxFB"
                                 << endl);
        }
    }

    void ImageRenderer::assemblePrimaryLogicalImage(const IPImage* img,
                                                    LogicalImage* i)
    {
        i->pixelAspect = img->displayPixelAspect();
        i->initPixelAspect = img->initPixelAspect;

        if (!img->fb)
        {
            i->planes.resize(1);
            if (i->planes[0].tile)
            {
                TextureDescription emptyTexDescrip;
                (*(i->planes[0].tile)) = emptyTexDescrip;
            }
            else
            {
                i->planes[0].tile = new TextureDescription();
            }

            i->planes[0].ownTile = true;
            return;
        }

        size_t numPlanes = img->fb->numPlanes();
        i->planes.resize(numPlanes);

        size_t ip = 0;
        for (const FrameBuffer* p = img->fb; p; p = p->nextPlane(), ip++)
        {
            // find the right texture description
            FBToTextureMap::iterator it =
                m_uploadedTextures.find(p->identifier());
            if (it != m_uploadedTextures.end())
            {
                i->planes[ip].tile = it->second;
            }
            else
            {
                TWK_THROW_STREAM(RenderFailedExc,
                                 "Cannot find texture description for auxFB"
                                     << endl);
            }
        }
    }

    ImageRenderer::TextureDescription*
    ImageRenderer::getTexture(const FrameBuffer* fb,
                              TextureDescription::TextureMatch& match)
    {
        //
        // find the best match available
        // if none exists, look for a compatible one
        // if none exists, create a new one
        //
        const string fbhash = fb->identifier();

        FBToTextureMap::iterator it;
        it = m_uploadedTextures.find(fbhash);
        if (it != m_uploadedTextures.end())
        {
            //
            // if same fbhash exist then this texture is already on the card.
            //
            it->second->age = 0;
            match = TextureDescription::ExactMatch;
            return it->second;
        }

        //
        // find one that matches our formats
        //
        for (it = m_uploadedTextures.begin(); it != m_uploadedTextures.end();
             it++)
        {
            TextureDescription* tex = it->second;

            if (tex->age > 1 && compatible(fb, tex))
            {
                // tex->age == 1 means this was used last frame
                // we try to keep those around for another frame or two
                tex->age = 0;
                tex->uploaded = false;
                m_uploadedTextures[fbhash] = tex;
                m_uploadedTextures.erase(it->first);
                match = TextureDescription::Compatible;
                return tex;
            }
        }

        // create a new one
        TextureDescription* tex = new TextureDescription();
        tex->age = 0;
        initializeTexture(fb, tex);
        m_uploadedTextures[fbhash] = tex;
        match = TextureDescription::NewIncompatible;
        return tex;
    }

    //----------------------------------------------------------------------
    //
    //  Image management
    //

    void ImageRenderer::freeOldTextures()
    {
        vector<string> needsDelete;
        for (FBToTextureMap::iterator it = m_uploadedTextures.begin();
             it != m_uploadedTextures.end(); it++)
        {
            TextureDescription* tex = it->second;
            if (tex->age > 10)
            {
                needsDelete.push_back(it->first);
                continue;
            }
            tex->age++;
        }

        for (size_t i = 0; i < needsDelete.size(); i++)
        {
            FBToTextureMap::iterator it =
                m_uploadedTextures.find(needsDelete[i]);

            if (it != m_uploadedTextures.end())
            {
                m_glState->deleteGLTexture(it->second->id);
                if (it->second->bufferId)
                    glDeleteBuffers(1, &it->second->bufferId);
                delete it->second;
                m_uploadedTextures.erase(it->first);
            }
        }
    }

    void ImageRenderer::freeUploadedTextures()
    {
        for (FBToTextureMap::iterator it = m_uploadedTextures.begin();
             it != m_uploadedTextures.end(); it++)
        {
            m_glState->deleteGLTexture(it->second->id);
            if (it->second->bufferId)
                glDeleteBuffers(1, &it->second->bufferId);
            delete it->second;
        }
        m_uploadedTextures.clear();
    }

    bool ImageRenderer::compatible(const FrameBuffer* fb,
                                   const TextureDescription* tex) const
    {
        //
        //  Check for basic geometry
        //
        if (fb->height() != tex->height || fb->width() != tex->width
            || fb->depth() != tex->depth || fb->numChannels() != tex->channels
            || fb->pixelSize() != tex->pixelSize
            || fb->uncropWidth() != tex->uncropWidth
            || fb->uncropHeight() != tex->uncropHeight
            || fb->uncropX() != tex->uncropX || fb->uncropY() != tex->uncropY)
        {
            return false;
        }

        //
        //  Check further
        //
        TextureDescription b;
        initializeTextureFormat(fb, &b);

        if (tex->channelType != b.channelType
            || tex->internalFormat != b.internalFormat
            || tex->format != b.format || tex->alignment != b.alignment)
        {
            return false;
        }

        return true;
    }

    void ImageRenderer::initializePlane(const IPImage* img,
                                        const GLFBO* imageFBO,
                                        ImagePlane& plane) const
    {
        plane.width = imageFBO->width();
        plane.height = imageFBO->height();

        //
        //  Initialize texture sizes and locations
        //

        TextureDescription* d = plane.tile;

        d->id = imageFBO->colorID(0);
        d->bufferId = 0;
        d->width = plane.width;
        d->height = plane.height;
        d->depth = 0;

        d->alignment = 0;
        d->channelType = GL_HALF_FLOAT_ARB;
        d->format = GL_RGBA;
        d->internalFormat = GL_RGBA16F_ARB;
        d->channels = 4;
        d->pixelSize = 0;
        d->target = imageFBO->colorTarget(0);

        d->uncropWidth = plane.width;
        d->uncropHeight = plane.height;
        d->uncropX = 0;
        d->uncropY = 0;
    }

    void ImageRenderer::computeImageGeometry(const IPImage* img,
                                             ImagePlane& plane) const
    {
        if (img->fb)
        {
            computePlaneGeometry(img, img->fb, plane);
            return;
        }

        const int iw = plane.width;
        const int ih = plane.height;
        float ia = float(iw) / float(ih);

        plane.originX = 0;
        plane.originY = 0;
        plane.endX = iw;
        plane.endY = ih;

        if (img->isCropped)
        {
            plane.originX = img->cropStartX;
            plane.originY = img->cropStartY;
            plane.endX = img->cropEndX;
            plane.endY = img->cropEndY;
            ia = (float)(plane.endX - plane.originX)
                 / (float)(plane.endY - plane.originY);
        }

        plane.x0 = 0;
        plane.x1 = ia;
        plane.y0 = 0;
        plane.y1 = 1;
    }

    void ImageRenderer::computePlaneGeometry(const IPImage* img,
                                             const FrameBuffer* fb,
                                             ImagePlane& plane) const
    {
        const float ia = 1.0; // fix this with the uncrop matrix
        double ma = ((float)img->width / img->height);

        TextureDescription* d = plane.tile;
        const bool needsUncrop =
            d->width != d->uncropWidth || d->height != d->uncropHeight;

        plane.endX = d->width;
        plane.endY = d->height;
        plane.originX = 0;
        plane.originY = 0;

        if (img->isCropped)
        {
            plane.originX = img->cropStartX;
            plane.originY = img->cropStartY;
            plane.endX = img->cropEndX;
            plane.endY = img->cropEndY;

            ma *= (float)(img->cropEndX - img->cropStartX) * (float)img->height
                  / (float)(img->cropEndY - img->cropStartY)
                  / (float)img->width;
        }

        Mat33d M(ma, 0, 0, 0, 1, 0, 0, 0, 1);

        const float x0 = 0;
        const float x1 = ia;

        const float y0 = 0;
        const float y1 = 1.0;

        Vec2f min1 = M * Vec2d(x0, y0);
        Vec2f max1 = M * Vec2d(x1, y1);

        plane.x0 = min1.x;
        plane.x1 = max1.x;
        plane.y0 = min1.y;
        plane.y1 = max1.y;

        if (needsUncrop)
        {
            // use width, height, uncrop width, height, crop origin to figure
            // out the size of the whole data quad
            float tx0, ty0, tx1, ty1;
            ty0 = plane.y0
                  - (fb->height() + fb->uncropY() - fb->uncropHeight())
                        * (plane.y1 - plane.y0) / fb->uncropHeight();
            ty1 = plane.y1
                  - fb->uncropY() * (plane.y1 - plane.y0) / fb->uncropHeight();
            tx0 = plane.x0
                  + fb->uncropX() * (plane.x1 - plane.x0) / fb->uncropWidth();
            tx1 = plane.x1
                  + (fb->width() + fb->uncropX() - fb->uncropWidth())
                        * (plane.x1 - plane.x0) / fb->uncropWidth();

            plane.x0 = tx0;
            plane.y0 = ty0;
            plane.x1 = tx1;
            plane.y1 = ty1;
        }
    }

    void ImageRenderer::initializeTextureFormat(const FrameBuffer* fb,
                                                TextureDescription* d) const
    {
        d->width = fb->width();
        d->height = fb->height();
        d->depth = fb->depth();
        d->channels = fb->numChannels();
        d->pixelSize = fb->pixelSize();
        d->swapBytes = false;
        d->uncropWidth = fb->uncropWidth();
        d->uncropHeight = fb->uncropHeight();
        d->uncropX = fb->uncropX();
        d->uncropY = fb->uncropY();

        if (fb->coordinateType() == FrameBuffer::PixelCoordinates)
        {
            d->target = (fb->coordinateType() == FrameBuffer::PixelCoordinates)
                            ? GL_TEXTURE_RECTANGLE
                            : GL_TEXTURE_2D;
        }
        else
        {
            if (d->depth > 1)
            {
                d->target = GL_TEXTURE_3D;
            }
            else if (d->height > 1)
            {
                d->target = GL_TEXTURE_2D;
            }
            else
            {
                d->target = GL_TEXTURE_1D;
            }
        }

        //
        //  Channel data type
        //

        ImageRenderer::FastPath fastPath = findFastPath(fb);

        switch (fb->dataType())
        {
        case FrameBuffer::UCHAR:
            d->channelType = GL_UNSIGNED_BYTE;
            break;
        case FrameBuffer::USHORT:
            d->channelType = GL_UNSIGNED_SHORT;
            break;
        case FrameBuffer::HALF:
            d->channelType = GL_HALF_FLOAT_ARB;
            break;
        case FrameBuffer::FLOAT:
            d->channelType = GL_FLOAT;
            break;
        default:
            break;
        }

        for (size_t a = 1, ss = fb->scanlinePaddedSize(); a <= 8; a <<= 1)
        {
            if (ss % a == 0)
                d->alignment = a;
        }

        switch (d->channels)
        {
        case 0:
            //
            //    This can happen if a file is "barely" read with
            //    errors.
            //

            d->format = 0;
            d->internalFormat = 0;
            d->channelType = 0;
            return;

        case 1:
            //
            //    Handle some of the PACKED cases here.
            //

            switch (fb->dataType())
            {
            case FrameBuffer::PACKED_R10_G10_B10_X2:
                d->format = GL_RGBA;
                d->channelType = GL_UNSIGNED_INT_10_10_10_2;
                d->internalFormat = GL_RGB10;
                break;
            case FrameBuffer::PACKED_X2_B10_G10_R10:
                d->format = GL_RGBA;
                d->channelType = GL_UNSIGNED_INT_2_10_10_10_REV;
                d->internalFormat = GL_RGB10;
                break;
            case FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
            case FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
                //
                //  Treat these as two channel. An appropriate shader
                //  is required to render them. In the past we used
                //  the special apple extension, but it turns out that
                //  the color is "undefined" for that texture type
                //  (which basically renders it useless)
                //
                d->format = GL_RGBA;
                d->internalFormat = GL_RGBA8;
                d->format = GL_LUMINANCE_ALPHA;
                d->internalFormat = GL_RGBA8;
                d->channels = 4;
                d->channelType = GL_UNSIGNED_BYTE;
                d->width /= 2;
                break;
            default:
                d->format = GL_LUMINANCE;
                switch (d->channelType)
                {
                case GL_HALF_FLOAT_ARB:
                    if (m_softwareGLRenderer)
                        d->internalFormat = GL_LUMINANCE32F_ARB;
                    else
                        d->internalFormat = GL_LUMINANCE16F_ARB;
                    break;
                case GL_FLOAT:
                    d->internalFormat = GL_LUMINANCE32F_ARB;
                    break;
                case GL_UNSIGNED_SHORT:
                    d->internalFormat = GL_LUMINANCE16;
                    break;
                default:
                    d->internalFormat = GL_LUMINANCE8;
                    break;
                }
                break;
            }
            break;

        case 2:
            d->format = GL_LUMINANCE_ALPHA;

            switch (d->channelType)
            {
            case GL_HALF_FLOAT_ARB:
                if (m_softwareGLRenderer)
                    d->internalFormat = GL_LUMINANCE_ALPHA32F_ARB;
                else
                    d->internalFormat = GL_LUMINANCE_ALPHA16F_ARB;
                break;
            case GL_FLOAT:
                d->internalFormat = GL_LUMINANCE_ALPHA32F_ARB;
                break;
            case GL_UNSIGNED_SHORT:
                d->internalFormat = GL_LUMINANCE16_ALPHA16;
                break;
            default:
                d->internalFormat = GL_LUMINANCE8_ALPHA8;
                break;
            }
            break;

        case 3:
            d->format = GL_RGB;

            switch (d->channelType)
            {
            case GL_HALF_FLOAT_ARB:
                if (m_softwareGLRenderer)
                    d->internalFormat = GL_RGBA32F_ARB;
                else
                    d->internalFormat = GL_RGB16F_ARB;
                break;
            case GL_FLOAT:
                d->internalFormat = GL_RGB32F_ARB;
                break;
            case GL_UNSIGNED_SHORT:
                d->internalFormat = GL_RGB16;
                break;
            default:
                d->internalFormat = 3;
                break;
            }

            break;

        case 4:
        {
#if defined(TWK_BIG_ENDIAN) || defined(__BIG_ENDIAN__)
            bool backwards =
                fb->channelName(0) == "A" && fb->channelName(1) == "R";
#else
            bool backwards = false;
#endif

            d->format = GL_RGBA;
            d->internalFormat = GL_RGBA8;

            switch (d->channelType)
            {
            case GL_HALF_FLOAT_ARB:
                if (m_softwareGLRenderer)
                    d->internalFormat = GL_RGBA32F_ARB;
                else
                    d->internalFormat = GL_RGBA16F_ARB;
                break;
            case GL_FLOAT:
                d->internalFormat = GL_RGBA32F_ARB;
                break;
            case GL_UNSIGNED_SHORT:
                d->internalFormat = GL_RGBA16;
                break;
            case GL_UNSIGNED_BYTE:
                if (backwards)
                {
                    d->format = GL_BGRA;
                    d->channelType = GL_UNSIGNED_INT_8_8_8_8_REV;
                }
                else
                {
                    d->format = fastPath.format8x4;
                    d->channelType = GL_UNSIGNED_BYTE;
                }
                break;
            }

            if (fb->depth() > 1)
                d->internalFormat = GL_RGBA;
        }

        break;
        }

        assert(d->format);
    }

    void ImageRenderer::initializeTexture(const FrameBuffer* fb,
                                          TextureDescription* d) const
    {
        HOP_PROF_FUNC();

        initializeTextureFormat(fb, d);

        const size_t totalBytes =
            d->width * d->height * d->depth * d->pixelSize;
        d->id = m_glState->createGLTexture(totalBytes);

        // Note: The minimum size restriction is to prevent an NVIDIA driver
        // issue The problem is that once a PBO has been used for a transfer <
        // 128KB it becomes slow when used for larger transfers.
        // --
        // Note: The upper limit restriction set on the number of concurrent
        // PBOs is to prevent an explosion of PBOs allocated when the user
        // switches to a large layout say with over 20 clips. These extra PBOs
        // can really stress a system and also make the default layout slow to
        // appear due to the time it takes to allocate all those extra PBOs.
        const bool usePBO =
            m_pixelBuffers
            && (d->channels != 3 || d->channelType != GL_UNSIGNED_SHORT)
            && !useAppleClientStorage() && fb->scanlinePixelPadding() == 0
            && totalBytes > 128 * 1024
            && m_uploadedTextures.size() < evMaxConcurrentPBOs.getValue();
        if (usePBO)
        {
            d->pPBOToGPU =
                std::make_shared<TwkGLF::GLPixelBufferObjectFromPool>(
                    TwkGLF::GLPixelBufferObject::TO_GPU, totalBytes);
        }
    }

    void ImageRenderer::assignAuxImages(const IPImage* img)
    {
        ImagePassState* istate = m_imagePassStates[img->renderIDHash()];

        assert(istate->images.size() - 1
               == img->auxFBs.size()); // first in images is primary

        for (size_t i = 1; i < istate->images.size(); ++i)
        {
            assignAuxImage(&istate->images[i], img->auxFBs[i - 1]);
        }

        for (size_t i = 0; i < istate->mergeImages.size(); ++i)
        {
            assignAuxImage(&istate->mergeImages[i], img->auxMergeFBs[i]);
        }
    }

    void ImageRenderer::assignAuxImage(LogicalImage* i, const FrameBuffer* fb)
    {
        i->fbhash = fb->identifier();
        i->texturehash = i->fbhash;
    }

    void ImageRenderer::assignImage(LogicalImage* i, const IPImage* img,
                                    const GLFBO* imageFBO, bool norender,
                                    bool isMergeContext, const size_t serialNum)
    {
        const FrameBuffer* fb = img->fb;

        //  By the time this func is called, all textures must be uploaded

        //
        //  NOTE: not always recording imageFBOs in norender mode (because they
        //  might be gone). If this is important we need to figure out a way to
        //  record more information or have a different method of doing
        //  per-pixel lookup of input images
        //

        i->render = img->destination != IPImage::OutputTexture
                    && img->destination != IPImage::DataBuffer
                    && !img->isBlank() && !img->isNoImage();
        i->isresident = imageFBO ? true : false;
        i->isvirtual = img->fb ? false : !i->isresident;
        i->node = img->node;
        i->tagMap = img->tagMap;
        i->pixelAspect = img->displayPixelAspect();
        i->initPixelAspect = img->initPixelAspect;
        i->touched = false;
        i->outputST = false;
        i->outputSize = false;
        i->fbhash = fb ? fb->identifier() : img->graphID();
        i->texturehash = i->fbhash;
        i->serialNum = serialNum;
        i->ipToGraphIDs[img] = img->graphID();

        //
        //  The idhash is used for
        //

        const string idhash = logicalImageHash(img);

        if (norender || isMergeContext)
        {
            i->touched = true;
            i->render = true;
        }

        if (img->stencilBox.isEmpty())
        {
            i->stencilMin = Vec2f(0, 0);
            i->stencilMax = Vec2f(1, 1);
        }
        else
        {
            i->stencilMin = img->stencilBox.min;
            i->stencilMax = img->stencilBox.max;
        }

        if (fb)
        {
            i->width = fb->width();
            i->height = fb->height();
            i->uncropWidth = fb->uncropWidth();
            i->uncropHeight = fb->uncropHeight();
            i->uncropX = fb->uncropX();
            i->uncropY = fb->uncropY();
            i->depth = fb->depth();

            //
            //  Texture from memory -> GPU case
            //

            if (fb->hasAttribute("RVSource"))
            {
                i->source = fb->attribute<string>("RVSource");
            }
        }
        else
        {
            //
            //  Virtual or retained image. Use the LogicalImage to keep
            //  track of it on the card.
            //

            i->source = (img->node) ? img->node->name() : "";
            i->pixelAspect = img->displayPixelAspect();
            i->initPixelAspect = img->initPixelAspect;
            i->idhash = idhash;
            i->width = img->width;
            i->height = img->height;
            i->uncropWidth = img->width;
            i->uncropHeight = img->height;
            i->uncropX = 0;
            i->uncropY = 0;
            i->depth = 0;
            i->coordinateSet = NOT_A_COORDINATE;

            if (imageFBO)
            {
                i->texturehash = imageFBO->identifier();

                i->planes.resize(1);
                initializePlane(img, imageFBO, i->planes.front());
            }
        }
    }

    //----------------------------------------------------------------------
    //
    //  Upload plane
    //

    void ImageRenderer::upload3DTexture(const FrameBuffer* fb,
                                        GLuint pixelInterpolation,
                                        TextureDescription* tex)
    {
        const int iw = tex->width;
        const int ih = tex->height;
        const int id = tex->depth;
        const size_t totalBytes = iw * ih * id * tex->pixelSize;

        // glEnable(tex->target);
        tex->id = m_glState->createGLTexture(totalBytes);
        TWK_GLDEBUG;
        if (tex->target != GL_TEXTURE_3D)
        {
            cerr << "ERROR: Binding a non-3D texture in 3D target" << endl;
        }
        glBindTexture(tex->target, tex->id);
        TWK_GLDEBUG;

        GLPushClientAttrib attr1(GL_CLIENT_ALL_ATTRIB_BITS);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, tex->height);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexParameterf(tex->target, GL_TEXTURE_PRIORITY, 1.0);
        TWK_GLDEBUG;
        // won't work without the next two lines
        glTexParameteri(tex->target, GL_TEXTURE_MIN_FILTER, pixelInterpolation);
        glTexParameteri(tex->target, GL_TEXTURE_MAG_FILTER, pixelInterpolation);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

#if defined(USE_APPLE_STORAGE)
        if (useAppleClientStorage())
        {
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_STORAGE_HINT_APPLE,
                            GL_STORAGE_CACHED_APPLE);
        }
#endif

        TWK_GLDEBUG;

        const unsigned char* p = fb->pixels<unsigned char>();

        glTexImage3D(tex->target,         // target
                     0,                   // level
                     tex->internalFormat, // internal format
                     iw, ih, id,          // width + height + depth
                     0,                   // border
                     tex->format,         // data format
                     tex->channelType,    // data type
                     p);                  // data

        TWK_GLDEBUG;

        tex->uploaded = true;
    }

    void ImageRenderer::upload2DTexture(const FrameBuffer* fb,
                                        GLuint pixelInterpolation,
                                        TextureDescription* tex)
    {
        if (tex->depth > 1)
        {
            TWK_THROW_STREAM(
                RenderFailedExc,
                "Texture target is conflicting with its dimensions");
        }

        const int iw = tex->width;
        const int ih = tex->height;
        const size_t totalBytes = iw * ih * tex->pixelSize;

        tex->id = m_glState->createGLTexture(totalBytes);
        TWK_GLDEBUG;
        glBindTexture(tex->target, tex->id);
        TWK_GLDEBUG;

        glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, tex->height);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexParameteri(tex->target, GL_TEXTURE_MIN_FILTER, pixelInterpolation);
        glTexParameteri(tex->target, GL_TEXTURE_MAG_FILTER, pixelInterpolation);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        TWK_GLDEBUG;

        const unsigned char* p = fb->pixels<unsigned char>();

        glTexImage2D(tex->target,         // target
                     0,                   // level
                     tex->internalFormat, // internal format
                     iw, ih,              // width + height
                     0,                   // border
                     tex->format,         // data format
                     tex->channelType,    // data type
                     p);                  // data

        TWK_GLDEBUG;

        tex->uploaded = true;
    }

    void ImageRenderer::upload1DTexture(const FrameBuffer* fb,
                                        GLuint pixelInterpolation,
                                        TextureDescription* tex)
    {
        if (tex->height > 1 || tex->depth > 1)
        {
            TWK_THROW_STREAM(
                RenderFailedExc,
                "Texture target is conflicting with its dimensions");
        }

        const int iw = tex->width;
        const size_t totalBytes = iw * tex->pixelSize;

        tex->id = m_glState->createGLTexture(totalBytes);
        TWK_GLDEBUG;
        glBindTexture(tex->target, tex->id);
        TWK_GLDEBUG;

        glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexParameteri(tex->target, GL_TEXTURE_MIN_FILTER, pixelInterpolation);
        glTexParameteri(tex->target, GL_TEXTURE_MAG_FILTER, pixelInterpolation);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        TWK_GLDEBUG;

        const unsigned char* p = fb->pixels<unsigned char>();

        glTexImage1D(tex->target,         // target
                     0,                   // level
                     tex->internalFormat, // internal format
                     iw,                  // width
                     0,                   // border
                     tex->format,         // data format
                     tex->channelType,    // data type
                     p);                  // data

        TWK_GLDEBUG;

        tex->uploaded = true;
    }

    class ProfilerGuard
    {
    public:
        ProfilerGuard(ImageRenderer::ProfilingState& profilingState)
            : m_profilingState(profilingState)
            , m_profTimer(profilingState.profilingTimer)
            , m_startTime(0.0)
        {
            m_startTime = elapsed();
        }

        ~ProfilerGuard()
        {
            m_profilingState.uploadPlaneTime += elapsed() - m_startTime;
        }

        ProfilerGuard(ProfilerGuard const&) = delete;
        ProfilerGuard& operator=(ProfilerGuard const&) = delete;

    protected:
        double elapsed() const
        {
            return m_profTimer ? m_profTimer->elapsed() : 0.0;
        }

    private:
        ImageRenderer::ProfilingState& m_profilingState;
        TwkUtil::Timer* m_profTimer;
        double m_startTime;
    };

    void ImageRenderer::uploadPlane(const FrameBuffer* fb,
                                    TextureDescription* d, GLuint filter)
    {
        HOP_CALL(glFinish();)

#if defined(HOP_ENABLED)
        std::string hopMsg = std::string("ImageRenderer::uploadPlane()");
        if (d && d->format && fb && fb->allocSize() != 0)
        {
            hopMsg += std::string(" - width=") + std::to_string(d->width)
                      + std::string(", height=") + std::to_string(d->height)
                      + std::string(", pixelSize=")
                      + std::to_string(d->pixelSize);
        }
        HOP_PROF_DYN_NAME(hopMsg.c_str());
#endif

        ProfilerGuard guard(m_profilingState);

        if (fb->coordinateType() == FrameBuffer::NormalizedCoordinates)
        {
            GLuint pixelInterpolation = GL_LINEAR;
            if (const TwkFB::FBAttribute* a =
                    fb->findAttribute("texture_pixel_interpolation"))
            {
                if (const TwkFB::TypedFBAttribute<bool>* ta =
                        dynamic_cast<const TwkFB::TypedFBAttribute<bool>*>(a))
                {
                    pixelInterpolation = ta->value() ? GL_LINEAR : GL_NEAREST;
                }
            }
            switch (d->target)
            {
            case GL_TEXTURE_1D:
                upload1DTexture(fb, pixelInterpolation, d);
                return;
            case GL_TEXTURE_2D:
                upload2DTexture(fb, pixelInterpolation, d);
                return;
            case GL_TEXTURE_3D:
                upload3DTexture(fb, pixelInterpolation, d);
                return;
            }
            TWK_THROW_STREAM(RenderFailedExc, "Texture target is unsupported");
        }
        else
        {
            switch (d->target)
            {
            // Normal case
            case GL_TEXTURE_RECTANGLE:
                break;
            default:
                TWK_THROW_STREAM(RenderFailedExc,
                                 "Texture target is unsupported");
                break;
            }
        }

        if (!d->format || fb->allocSize() == 0)
            return;

        const int iw = d->width;
        const int ih = d->height;
        const size_t totalBytes = iw * ih * d->pixelSize;

        GLPushClientAttrib attr1(GL_CLIENT_ALL_ATTRIB_BITS);
        glEnable(d->target);
        TWK_GLDEBUG;

        //
        //  TODO: WE NEED TO CHECK ON THIS -- IS IT STILL TRUE?
        //
        //  Only use PBOs if we have a single tile (currently) This could
        //  be up to 8k or more. On some drivers on linux, it appears that
        //  3 channel images will mess with the window system. So just
        //  don't use PBOs for those.
        //
        //  Looks like 16 bit int images don't work either -- they're
        //  drawing garbage all over the place in the windowing system
        //  on linux and windows
        //

        // We should support non-contiguous pixel data in the PBO path.
        const bool contiguousData =
            (fb->scanlineSize() / fb->pixelSize()) == fb->width();

        const bool usePBO =
            m_pixelBuffers
            && (d->channels != 3 || d->channelType != GL_UNSIGNED_SHORT)
            && !useAppleClientStorage() && fb->scanlinePixelPadding() == 0
            && d->pPBOToGPU && d->pPBOToGPU->getSize() >= totalBytes
            && contiguousData;

        bool updateOnly = d->uploaded ? true : false;

        const unsigned char* p = fb->pixels<unsigned char>();

        if (usePBO)
        {
            void* b = nullptr;
            {
                HOP_PROF("ImageRenderer::uploadPlane() - usePBO - glMapBuffer");

                b = d->pPBOToGPU->map();
            }

            if (!b)
            {
                string estring = TwkGLF::errorString(glGetError());

                cerr << "ERROR: glMapBuffer: " << estring << endl;

                TWK_THROW_STREAM(RenderFailedExc,
                                 "glMapBuffer FAILED" << estring);
            }

            FastMemcpy_MP(b, p, totalBytes);

            d->pPBOToGPU->unmap();
            d->pPBOToGPU->bind();

            {
                HOP_PROF(
                    "ImageRenderer::uploadPlane() - usePBO - glBindTexture");

                glBindTexture(d->target, d->id);
                TWK_GLDEBUG;
                glPixelStorei(GL_UNPACK_ROW_LENGTH,
                              d->width + fb->scanlinePixelPadding());
                TWK_GLDEBUG;
                glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, d->height);
                TWK_GLDEBUG;
                glPixelStorei(GL_UNPACK_ALIGNMENT, d->alignment);
                TWK_GLDEBUG;
                glPixelStorei(GL_UNPACK_SWAP_BYTES, d->swapBytes ? 1 : 0);
                TWK_GLDEBUG;

                glTexParameterf(d->target, GL_TEXTURE_PRIORITY, 1.0);
                TWK_GLDEBUG;
                glTexParameteri(d->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(d->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(d->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                TWK_GLDEBUG;
            }

            if (updateOnly)
            {
                HOP_PROF("ImageRenderer::uploadPlane() - usePBO - updateOnly - "
                         "glTexSubImage2D");

                glTexSubImage2D(d->target,      // target
                                0,              // level
                                0, 0,           // x, y, offsets
                                iw, ih,         // w, h
                                d->format,      // data format
                                d->channelType, // data type
                                NULL);          // pbo to texture

                TWK_GLDEBUG;
                HOP_CALL(glFinish();)
            }
            else
            {
                HOP_PROF("ImageRenderer::uploadPlane() - usePBO - !updateOnly "
                         "- glTexImage2D");

                glTexImage2D(d->target,         // target
                             0,                 // level
                             d->internalFormat, // internal format
                             iw, ih,            // w, h
                             0,                 // border
                             d->format,         // data format
                             d->channelType,    // data type
                             NULL);             // data
                TWK_GLDEBUG;
                HOP_CALL(glFinish();)
            }

            d->pPBOToGPU->unbind();
            d->uploaded = true;
        }
        else
        {
            HOP_PROF("ImageRenderer::uploadPlane() - !usePBO");

            glBindTexture(d->target, d->id);
            TWK_GLDEBUG;
            glPixelStorei(GL_UNPACK_ROW_LENGTH,
                          fb->scanlineSize() / fb->pixelSize());
            TWK_GLDEBUG;
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, d->height);
            TWK_GLDEBUG;
            glPixelStorei(GL_UNPACK_ALIGNMENT, d->alignment);
            TWK_GLDEBUG;
            glPixelStorei(GL_UNPACK_SWAP_BYTES, d->swapBytes ? 1 : 0);
            TWK_GLDEBUG;

#if defined(USE_APPLE_STORAGE)

            if (useAppleClientStorage())
            {
                glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

                glTexParameteri(d->target, GL_TEXTURE_STORAGE_HINT_APPLE,
                                GL_STORAGE_CACHED_APPLE);
                TWK_GLDEBUG;
            }
#endif

            // glTexParameteri(d->target, GL_TEXTURE_MIN_FILTER, filter);
            // glTexParameteri(d->target, GL_TEXTURE_MAG_FILTER, filter);
            glTexParameteri(d->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(d->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(d->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            TWK_GLDEBUG;

            if (updateOnly && !useAppleClientStorage())
            {
                //
                //  NOTE: when using client storage do not "update"
                //  the buffer contents. That will just cause a memcpy
                //  in the driver. Instead redefine it with
                //  glTexImage2D which will release the previous
                //  client storage and start a new one
                //

                glTexSubImage2D(d->target,      // target
                                0,              // level
                                0, 0,           // x, y, offsets
                                iw, ih,         // w, h
                                d->format,      // data format
                                d->channelType, // data type
                                p);             // data

                TWK_GLDEBUG;
            }
            else
            {
                glTexImage2D(d->target,         // target
                             0,                 // level
                             d->internalFormat, // internal format
                             iw, ih,            // w, h
                             0,                 // border
                             d->format,         // data format
                             d->channelType,    // data type
                             p);                // data

                TWK_GLDEBUG;
            }

            d->uploaded = true;
        }
        HOP_CALL(glFinish();)
    }

    //----------------------------------------------------------------------
    //
    //  Paint
    //
    bool ImageRenderer::imageHasEraseCommands(const IPImage* root) const
    {
        // check out if there is a erase stroke. if there is one, we need to
        // have the result of ovelay commands as the 'initial render' otherwise
        // proceed
        bool hasErase = false;
        for (size_t i = 0; i < root->commands.size(); ++i)
        {
            if (root->commands[i]->getType() == Paint::Command::PolyLine
                && ((Paint::PolyLine*)root->commands[i])->mode
                       == Paint::PolyLine::EraseMode)
                hasErase = true;
        }
        return hasErase;
    }

    void ImageRenderer::renderPaint(const IPImage* root, const GLFBO* fbo)
    {
        //
        // if this image has overlay commands, such as matts, these commands
        // will be rendered first and they are considered part of the actual
        // rendering they should be not 'erasable' or 'clearable' by the
        // annotation tools after we render these, we move on to render
        // annotation
        //

        //
        // Note there are several cases
        // i. render all commands from scratch (playback or cache turned off)
        //    a. no overlay commands, or has overlay commands but have no erase
        //    commands.
        //       we just need to render all commands, no special magic
        //    b. has overlay commands and has erase commands. this means, we
        //    have to first render all overlay
        //       on top of source. as this as the 'initial render' to be passed
        //       onto erase commands. after all overlay commands are rendered,
        //       continue rendering the annotation commands.
        // ii. render only 'new' or 'updated' commands (meaning we found cached
        // paint fbo)
        //     a. no overlay commands, or has overlay commands but no erase
        //     commands.
        //        no magic needed just render the 'new' or 'updated' ones on top
        //        of the cached paint fbo
        //     b. has overlay and erase commands. this means we found a cached
        //     paint fbo that contains the rendering up
        //        to the last command. but we also need to get the content of
        //        the 'initial render (source + overlay)' so we make sure to get
        //        the initial render, pass it onto erase commands. 'overlayfbo'
        //        is used to hold the content of 'initial render'
        //

        if (root->commands.empty())
            return;

        const string prenderID = imageToFBOIdentifier(root);
        // these two are for pingpong
        const GLFBO* tempfbo1 =
            m_imageFBOManager
                .newImageFBO(fbo, m_fullRenderSerialNumber, prenderID)
                ->fbo();
        const GLFBO* tempfbo2 =
            m_imageFBOManager
                .newImageFBO(fbo, m_fullRenderSerialNumber, prenderID)
                ->fbo();

        assert(fbo);

        //////////////////////////////caching////////////////////////////////
        //
        //  only cache up to the second last command
        //  add spaces, esp after curCmdNum, since we want to parse this string
        //  later. add wxh, since we never want to reuse an fbo for paint that
        //  has different dimensions
        //
        //  NOTE: paintCmdNo must be added LAST since findExisting will only
        //  compare up to that point.
        //

        size_t lastCmdNum = 0;
        const size_t curCmdNum = root->commands.size();
        bool foundCachedFBO = false;
        ImageFBO* cachedFBO = NULL;
        size_t startCmd = 0;

        if (root->commands.size() > 1)
        {
            //
            // if there is only one command, we don't need to look for cached,
            // because the cached will be identical to current fbo, because we
            // only cache up to the second last command
            //

            ostringstream newRenderID;
            newRenderID << root->renderIDWithPartialPaint() << " " << m_filter
                        << " " << m_bgpattern << " " << fbo->width() << "x"
                        << fbo->height() << " paintCmdNo" << curCmdNum - 1;

            cachedFBO = m_imageFBOManager.findExistingPaintFBO(
                fbo, newRenderID.str(), foundCachedFBO, lastCmdNum,
                m_fullRenderSerialNumber);
            assert(lastCmdNum <= curCmdNum);

            if (foundCachedFBO)
            {
                startCmd = lastCmdNum;
                cachedFBO->fbo()->copyTo(tempfbo1);
            }
            else
            {
                cachedFBO = m_imageFBOManager.newImageFBO(
                    fbo, m_fullRenderSerialNumber, newRenderID.str());
                fbo->copyTo(tempfbo1);
            }
        }
        else
            fbo->copyTo(tempfbo1);

        ////////////////////////render commands////////////////////////////////
        // see if there is any 'ExecuteAllBefore' command. If so execute all
        // overlay commands first
        int overlayLoc = -1;
        for (size_t i = 0; i < root->commands.size(); ++i)
        {
            if (root->commands[i]->getType()
                == Paint::Command::ExecuteAllBefore)
            {
                overlayLoc = i;
                break;
            }
        }

        ///////////////////////////stencil//////////////////////////////////////
        bool hasStencil = !root->stencilBox.isEmpty();
        Vec4f stencil = Vec4f(0.0f, 0.0f, 1.0f, 1.0f);
        if (hasStencil)
        {
            FrameBuffer* fb = root->fb;
            const float pa = fb->pixelAspectRatio();
            const float iw = fb->width();
            const float ih = fb->height();
            const float uw = fb->uncropWidth();
            const float uh = fb->uncropHeight();
            const float ux = fb->uncropX();
            const float uy = fb->uncropY();

            const float wmin = ux * pa;
            const float hmin = uh - uy - ih;
            const float wmax = (ux + iw) * pa;
            const float hmax = uh - uy;

            float xmin = (wmin + (wmax - wmin) * root->stencilBox.min.x);
            float ymin = (hmin + (hmax - hmin) * root->stencilBox.min.y);
            float xmax = (wmin + (wmax - wmin) * root->stencilBox.max.x);
            float ymax = (hmin + (hmax - hmin) * root->stencilBox.max.y);

            stencil = Vec4f(xmin, ymin, xmax, ymax);
        }

        PaintContext paintContext;
        paintContext.glState = m_glState;
        paintContext.initialRender = fbo;
        paintContext.tempRender1 = tempfbo1;
        paintContext.tempRender2 = tempfbo2;
        paintContext.cachedfbo = cachedFBO ? cachedFBO->fbo() : NULL;
        paintContext.image = root;
        paintContext.hasStencil = hasStencil;
        paintContext.stencilBox = stencil;
        paintContext.lastCommand = root->commands.back();

        fbo->unbind();

        if (overlayLoc >= 0)
        {
            if (startCmd <= overlayLoc)
            {
                for (size_t i = startCmd; i <= overlayLoc; ++i)
                {
                    paintContext.commands.push_back(root->commands[i]);
                }
                // render overlay commands
                paintContext.updateCache = false;
                Paint::renderPaintCommands(paintContext);

                if (paintContext.commandExecuted % 2 == 0)
                {
                    const GLFBO* t = tempfbo1;
                    paintContext.tempRender1 = paintContext.tempRender2;
                    paintContext.tempRender2 = t;
                }

                // at this point, fbo contains the rendering of source + overlay
                // commands
                paintContext.commands.clear();
                for (size_t i = overlayLoc + 1; i < root->commands.size(); ++i)
                {
                    paintContext.commands.push_back(root->commands[i]);
                }
                // now render annotations
                paintContext.updateCache = true;
                Paint::renderPaintCommands(paintContext);
            }
            else
            {
                paintContext.updateCache = false;

                // this is the cached case and the cache contains all the
                // overlay commands already
                if (imageHasEraseCommands(root))
                {
                    //
                    // the erase strokes need to know what the initial render
                    // looks like initial render is source + overlay commands
                    //

                    paintContext.initialRender = 0;
                    paintContext.tempRender1 = fbo;
                    paintContext.tempRender2 = tempfbo2;
                    for (size_t i = 0; i <= overlayLoc; ++i)
                    {
                        paintContext.commands.push_back(root->commands[i]);
                    }
                    Paint::renderPaintCommands(paintContext);

                    if (paintContext.commandExecuted % 2 == 0)
                    {
                        tempfbo2->copyTo(fbo);
                    }
                    paintContext.initialRender =
                        fbo; // holds the source+overlay commands render
                    paintContext.tempRender1 = tempfbo1;
                    paintContext.tempRender2 = tempfbo2;
                    paintContext.commands.clear();
                }

                for (size_t i = startCmd; i < root->commands.size(); ++i)
                {
                    paintContext.commands.push_back(root->commands[i]);
                }
                Paint::renderPaintCommands(paintContext);
            }
        }
        else
        {
            // no overlay commands
            paintContext.commands.clear();
            for (size_t i = startCmd; i < root->commands.size(); ++i)
            {
                paintContext.commands.push_back(root->commands[i]);
            }
            Paint::renderPaintCommands(paintContext);
        }

        if (!paintContext.cacheUpdated && cachedFBO)
            cachedFBO->identifier = "";

        /////////////////// clean up /////////////////////////////////////
        //
        //  NOTE: we do not release the cachedFBO, so others cannot grab it
        //

        m_imageFBOManager.releaseImageFBO(tempfbo1);
        m_imageFBOManager.releaseImageFBO(tempfbo2);

        fbo->bind();

        m_glState->useGLProgram(defaultGLProgram());
    }

    void ImageRenderer::unlinkNode(IPCore::IPNode* node)
    {
        for (auto& currRenderedImage : m_renderedImages)
        {
            if (currRenderedImage.node == node)
                currRenderedImage.node = nullptr;
        }
    }

} // End namespace IPCore
