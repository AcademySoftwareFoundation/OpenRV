//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ImageRenderer__h__
#define __IPCore__ImageRenderer__h__
#include <boost/thread.hpp>
#include <IPCore/IPNode.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/PaintCommand.h>
#include <IPCore/ImageFBO.h>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/GLPixelBufferObjectPool.h>
#include <TwkMath/Box.h>
#include <TwkMath/Color.h>
#include <TwkMovie/Movie.h>
#include <TwkUtil/Timer.h>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <boost/thread/condition_variable.hpp>

#ifdef PLATFORM_DARWIN
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl.h>
#endif

#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
// #ifndef PLATFORM_DARWIN
#define TWK_BGRA_FAST_PATH
// #endif
#endif

namespace IPCore
{
    class IPNode;
    class PaintCommand;

    namespace Shader
    {
        class ProgramCache;
    }

    //
    //  class ImageRenderer
    //
    //  Abstract base class for an Image Renderer (for RV). This class is
    //  responsible for taking an IP image -- including all of its
    //  parameters (gamma, luts, etc) and rendering pixels to the screeen
    //  using those parameters.
    //

    //
    //  The main GPU memory consumption of each renderer comes from
    //  textures and buffers (fbo, vbo).
    //  ImageFBO manager keeps track of how much memory is currently in use for
    //  FBOs GL State keeps track of how much memory is currently in use for
    //  textures In the future GL State could track vbo memory too.
    //

    //
    //  Threaded Upload
    //  Two modes in threaded upload: (1) prefetch fetches current frame
    //  render waits until upload is done to proceed. this mode isn't that
    //  useful (2) prefetch fetches what is predicted as the next frame (which
    //  could be wrong at times) upload and render goes simultaneously. render
    //  thread will wait for upload thread to return until it starts the next
    //  frame. Note return doesn't mean textures are uploaded the texture fences
    //  are what actually tell us whether upload is fully finished
    //

    //
    //  Description of what renderer does, for each frame
    //  ImageRenderer::render() is the main entry point
    //  i.    renderer does some book keeping (increase serial number, clean up
    //  old unused textures, etc.) ii.   prepare all the textures that need to
    //  be uploaded (could be for current or next frame) iii.  upload textures
    //  (could be for this or next frame, could be serial or threaded) iv. mark
    //  any ImageFBO we can reuse (hold on to them, make sure the renderer won't
    //  try to use them) v.    start fresh
    //        -- if necessary, wait for upload to finish before draw
    //        -- assemble ImagePassState for each IPImage: match IPImage to all
    //        the textures that have been uploaded
    //        -- draw whatever needs to be drawn (in some cases, nothing needs
    //        to be drawn)d end
    //  vi.   cleanup
    //

    class ImageRenderer
    {
    public:
        typedef std::vector<const IPImage*> DisplayIPNodeStack;
        typedef IPImage::Matrix Matrix;
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef std::vector<const FrameBuffer*> FBVector;
        typedef TwkFB::FrameBuffer::Orientation Orientation;
        typedef FrameBuffer::DataType FBDataType;
        typedef std::set<FBDataType> FBAcceptableTypes;
        typedef TwkApp::VideoDevice VideoDevice;
        typedef TwkGLF::GLVideoDevice GLVideoDevice;
        typedef TwkGLF::GLBindableVideoDevice GLBindableVideoDevice;
        typedef TwkGLF::GLFBO GLFBO;
        typedef TwkGLF::FBOVector FBOVector;
        typedef TwkGLF::ConstFBOVector ConstFBOVector;
        typedef TwkMath::Vec2f Vec2f;
        typedef TwkMath::Box2f Box2f;
        typedef TwkMath::Vec3f Vec3f;
        typedef TwkMath::Vec4f Vec4f;
        typedef TwkMath::Mat44f Mat44f;
        typedef TwkMath::Mat33f Mat33f;
        typedef TwkUtil::Timer Timer;
        typedef VideoDevice::Margins Margins;
        typedef VideoDevice::DisplayMode DisplayMode;
        typedef TwkGLF::GLState GLState;
        typedef std::vector<const IPImage*> ActiveImageVector;
        typedef Paint::PaintContext PaintContext;
        typedef std::vector<ImageFBO*> ImageFBOVector;
        typedef IPImage::PaintCommands PaintCommands;
        typedef IPImage::HashValue HashValue;
        typedef Shader::Program::ImageAndCoordinateUnit ImageAndCoordinateUnit;
        typedef Shader::Program::TextureUnitAssignments TextureUnitAssignments;
        typedef std::map<const IPImage*, std::string> GraphIDMap;
        typedef std::set<const VideoDevice*> DeviceSet;
        typedef boost::thread Thread;
        typedef boost::mutex Mutex;
        typedef boost::condition_variable ConditionalVariable;
        typedef boost::unique_lock<Mutex> ScopedLock;

        typedef void (*VoidFunc)();
        typedef VoidFunc (*GetProcAddressFunc)(const char*);

        enum BGPattern
        {
            Solid0,
            Solid18,
            Solid50,
            Solid100,
            Checker,
            CrossHatch
        };

        struct ProfilingState
        {
            Timer* profilingTimer;
            double fenceWaitTime;
            double uploadPlaneTime;
        };

        struct FastPath
        {
            GLuint format8x4;
        };

        //
        // TextureDescription contains all info needed for upload
        // and corresponds to the framebuffer raw data
        //
        struct TextureDescription
        {
            enum TextureMatch
            {
                ExactMatch,
                Compatible,
                NewIncompatible
            };

            TextureDescription()
                : uploaded(false)
                , age(-1)
            {
            }

            void clean();

            static void* operator new(size_t s);
            static void operator delete(void* p, size_t s);

            GLuint id;       // from glGenTextures
            GLuint bufferId; // from glGenBuffers (PBOs)
            bool uploaded;
            int age; // used to determine when to delete old ones

            GLuint target; // e.g. GL_TEXTURE_RECTANGLE_ARB
            size_t width;  // 1D texture (i.e. width with height=depth=0)
            size_t height; // 2D texture (i.e. width + height with depth=0)
            size_t depth;  // 3D texture (i.e. width + height + depth)
            size_t channels;
            GLuint channelType;
            GLuint internalFormat;
            GLuint format;
            GLuint alignment;
            size_t pixelSize; // in byte
            bool swapBytes;

            int uncropWidth; // display window size for uncropped
            int uncropHeight;
            int uncropX; // position of the data window (can be negative) for
                         // uncropped
            int uncropY;

            std::shared_ptr<TwkGLF::GLPixelBufferObjectFromPool> pPBOToGPU;
        };

        typedef stl_ext::replacement_allocator<
            std::pair<const std::string, TextureDescription*>>
            TextureDescriptionAlloc;
        typedef std::map<const std::string, TextureDescription*,
                         std::less<const std::string>, TextureDescriptionAlloc>
            FBToTextureMap;

        //
        // image plane contains info about how the texture will be rendered
        // i.e. quad boundary, uncrop, etc.
        //
        struct ImagePlane
        {
            ImagePlane()
                : tile(0)
                , ownTile(false)
            {
            }

            ~ImagePlane()
            {
                if (ownTile)
                    delete tile;
            }

            float x0; // draw quad bounds
            float y0; //
            float x1; //
            float y1; //
            int originX;
            int originY;
            int endX;
            int endY;

            int width;
            int height;
            GLuint textureUnit;

            TextureDescription* tile;
            bool ownTile;
        };

        typedef stl_ext::replacement_allocator<ImagePlane> ImagePlaneAlloc;
        typedef std::vector<ImagePlane, ImagePlaneAlloc> ImagePlanes;

        // we do not cache these
        struct LogicalImage
        {
            LogicalImage()
                : width(0)
                , height(0)
                , depth(0)
                , uncropWidth(0)
                , uncropHeight(0)
                , uncropX(0)
                , uncropY(0)
                , pixelAspect(1.0)
                , initPixelAspect(1.0)
                , isvirtual(true)
                , isresident(false)
                , node(0)
                , Yw(0.0)
                , stencilMin(0.0)
                , stencilMax(0.0)
                , render(false)
                , touched(false)
                , outputST(false)
                , outputSize(false)
                , coordinateSet(GLuint(-1))
                , imageNum(0)
                , isPrimary(true)
            {
            }

            DeviceSet
                devices; // maintains all the devices that rendered this image
            size_t serialNum;
            int width;
            int height;
            int depth;
            int uncropWidth;
            int uncropHeight;
            int uncropX;
            int uncropY;
            float pixelAspect;
            float initPixelAspect;
            std::string idhash; // used to identify a logical image
            std::string fbhash; // fb->identifier or ""
            std::string
                texturehash; // fbhash or a GLFBO id
                             // or img->graphID for non resident
                             // this is needed to correctly assign texture units
            std::string source;
            bool isvirtual;  // has no pixels (ever)
            bool isresident; // pixels already on card
            bool outputST;
            bool outputSize;
            const IPNode* node;
            IPImage::TagMap tagMap;
            Vec3f Yw;
            Vec2f stencilMin;
            Vec2f stencilMax;
            GLuint coordinateSet;
            ImagePlanes planes;
            bool render;
            mutable GraphIDMap ipToGraphIDs;
            mutable bool touched;
            size_t imageNum;
            bool isPrimary;
        };

        typedef stl_ext::replacement_allocator<LogicalImage> LogicalImageAlloc;
        typedef std::vector<LogicalImage, LogicalImageAlloc> LogicalImageVector;

        //
        // we do not cache these. they are assembled per render by tracing
        // expressions prefetch do not need these structs
        //
        // there is precisely one ImagePassState for each IPImage
        //
        struct ImagePassState
        {
            LogicalImage* primaryImage()
            {
                if (images.empty())
                    return 0;
                return &images[0];
            }

            static void* operator new(size_t s);
            static void operator delete(void* p, size_t s);

            LogicalImageVector images; // there is at least the primary image
            LogicalImageVector mergeImages;
            ActiveImageVector memberImages;
        };

        typedef stl_ext::replacement_allocator<
            std::pair<const HashValue, ImagePassState*>>
            ImagePassStateAlloc;
        typedef std::map<HashValue, ImagePassState*, std::less<HashValue>,
                         ImagePassStateAlloc>
            ImagePassStateMap;

        //
        // these are used to record render related info for each IPImage
        // that are needed by the UI
        //
        struct RenderedImage
        {
            RenderedImage(const std::string& name = "")
                : source(name)
                , node(0)
                , device(0)
                , serialNum(0)
            {
            }

            std::string source;
            IPImage::TagMap tagMap;
            int index;
            Box2f imageBox;
            Box2f stencilBox;
            Matrix modelMatrix;       // affine matrix
            Matrix globalMatrix;      // affine matrix
            Matrix projectionMatrix;  // projective
            Matrix textureMatrix;     // (unused?)
            Matrix orientationMatrix; // scale only
            Matrix placementMatrix;   // scale + translate
            int width;                // render structure
            int height;
            int uncropWidth;
            int uncropHeight;
            int uncropX;
            int uncropY;
            float pixelAspect;
            float initPixelAspect;
            int bitDepth;
            bool floatingPoint;
            int numChannels;
            bool planar;
            const IPNode* node;
            const VideoDevice* device;
            size_t serialNum;
            size_t imageNum;
            GLuint textureID;
            bool isVirtual;
            bool touched;
            bool render;
        };

        typedef stl_ext::replacement_allocator<RenderedImage>
            RenderedImageAlloc;
        typedef std::vector<RenderedImage, RenderedImageAlloc>
            RenderedImagesVector;

        //
        // the reason this is a struct is for future development where
        // we might add more members to this struct, as we need them
        //
        struct GLTextureObject
        {
            GLTextureObject(GLuint id)
                : textureID(id)
            {
            }

            GLTextureObject()
                : textureID(0)
            {
            }

            GLuint textureID;
        };

        //
        //  A Device is a VideoDevice plus a collection of FBOs used for
        //  offscreen rendering. If the device is a GLVideoDevice or a
        //  GLBindableVideoDevice its casted pointer is cached.
        //
        //  A GLVideoDevice can be rendered to directly. Unless some
        //  multi-pass rendering is occuring, no internal FBOs are
        //  required.
        //
        //  A GLBindableVideoDevice can take one or two rendered FBOs and
        //  transfer the data directly using its transfer() and
        //  transfer2() functions. These will be the internal FBOs for the
        //  device.
        //
        //  A non-GL VideoDevice requires that final pixels are rendered
        //  to internal FBOs like the GLBindableVideoDevice, but a final
        //  transfer is made to main memory -- NOTE: not working yet. This
        //  would be e.g. sdi card, etc.
        //
        //  The fbo field represents the device *as is* -- so for a GL
        //  widget this is just a wrapper around the underlying system
        //  FBO.
        //
        //  The internalFBO is an off screen buffer (or 2 if its stereo)
        //  used during rendering to the device. In some cases, the
        //  internalFBO can have a different resolution than the device
        //  itself.
        //

        struct DeviceFBOGroup
        {
            FBOVector views;
        };

        typedef std::vector<DeviceFBOGroup> DeviceFBORingBuffer;

        struct Device
        {
            Device(const VideoDevice* d = 0, const GLVideoDevice* g = 0,
                   const GLBindableVideoDevice* bd = 0,
                   size_t ringBufferSize = 1, size_t nviews = 1);

            const VideoDevice* device;
            const GLVideoDevice* glDevice;
            const GLBindableVideoDevice* glBindableDevice;
            DeviceFBORingBuffer fboRingBuffer;

            void setRingBufferSize(size_t, size_t);

            size_t ringBufferSize() const { return fboRingBuffer.size(); }

            void clearFBOs();
            void allocateFBOs(size_t w, size_t h, GLenum iformat, size_t view);
        };

        //
        //  AuxRender is intended to be derived from and passed to the
        //  main render function. This is how "user" rendering occurs.
        //

        class AuxRender
        {
        public:
            AuxRender() {}

            virtual ~AuxRender() {}

            virtual void render(VideoDevice::DisplayMode, bool leftEye,
                                bool rightEye, bool forController,
                                bool forOuput) const = 0;
        };

        //
        //  AuxAudio is allows the video output device to pull audio data.
        //  The data size and format is determined by the AudioFormat
        //  parameters of the VideoDevice
        //

        class AuxAudio
        {
        public:
            AuxAudio() {}

            virtual ~AuxAudio() {}

            virtual void* audioForFrame(int frame, size_t seqindex,
                                        size_t& n) const = 0;
            virtual bool isAvailable() const = 0;
        };

        //
        //  Internal Render Context
        //
        //  This holds the current rendering state. Because the graph
        //  being rendered can branch, these can also branch. Any
        //  information that could be replicated along branches needs to
        //  go in here.
        //

        struct InternalRenderContext
        {
            InternalRenderContext()
                : frame(-std::numeric_limits<int>::max())
                , targetFBO(0)
                , targetLFBO(0)
                , targetRFBO(0)
                , image(0)
                , numImages(0)
                , imageFBO(0)
                , mergeRender(false)
                , norender(false)
                , doBlend(false)
                , mergeContext(0)
                , device(0)
                , fullSerialNum(0)
                , mainSerialNum(0)
                , auxRenderer(0)
                , blendMode(IPImage::UnspecifiedBlendMode)
            {
            }

            int frame;
            const GLFBO* targetFBO;  // current render target
            const GLFBO* targetLFBO; // left eye target (from device, may be 0)
            const GLFBO* targetRFBO; // right eye target (from device, may be 0)
            bool mergeRender;        // use the IPImage::mergeExpr instead of
                                     // IPImage::shaderExpr
            const IPImage* image;    // current image being rendered
            const GLFBO* imageFBO;   // if virtual this may contain image pixels
            const VideoDevice* device; // current device (from image tree)

            size_t fullSerialNum;
            size_t mainSerialNum;
            size_t numImages;
            bool norender; // don't actually render, just record
            bool doBlend;
            IPImage::BlendMode blendMode; // blending state
            const AuxRender* auxRenderer;
            InternalRenderContext* mergeContext;
            ActiveImageVector activeImages;
        };

        typedef std::vector<InternalRenderContext*> ContextStack;

        struct InternalGLMatricesContext
        {
            InternalGLMatricesContext()
                : targetFBOWidth(0)
                , targetFBOHeight(0)
            {
            }

            Matrix baseImageMatrix;
            Margins margins;
            size_t targetFBOWidth;
            size_t targetFBOHeight;
        };

#ifdef PLATFORM_DARWIN
        struct CLProgram
        {
            cl_program program;
            std::vector<cl_kernel> kernels;
        };

        struct CLContext
        {
            enum
            {
                OpenCL1_0,
                OpenCL1_1,
                OpenCL1_2
            };

            cl_context clContext;
            cl_platform_id platformID;
            cl_uint platformNo;
            cl_device_id deviceID;
            cl_uint deviceNo;
            cl_command_queue commandQueue;
            CLProgram clProgram;
            size_t workGroupSize;
            size_t version; // OpenCL version
        };
#endif

        //----------------------------------------------------------------------

        //
        //  Constructors
        //

        ImageRenderer(const std::string& name);
        ~ImageRenderer();

        const std::string& name() const { return m_name; }

        //
        //  Device management
        //

        void setControlDevice(const VideoDevice*);
        void setOutputDevice(const VideoDevice*);

        bool hasMultipleOutputs() const
        {
            return m_controlDevice.device != m_outputDevice.device
                   && m_outputDevice.device != 0;
        }

        const Device& controlDevice() const { return m_controlDevice; }

        const Device& outputDevice() const { return m_outputDevice; }

        //
        //  Main render entry. render will eventually call the
        //  renderInternal() function of the derived class to do the heavy
        //  lifting.
        //

        void render(int frame, IPImage* root, const AuxRender* aux = 0,
                    const AuxAudio* auxAudio = 0, IPImage* uploadRoot = 0);

        void prepareTextureDescriptionsForUpload(const IPImage* img);
        void prefetch(const IPImage*);

        ImageFBO* newOutputOnlyImageFBO(GLenum internalFormat,
                                        size_t samples = 0);

        void releaseImageFBO(const GLFBO* fbo)
        {
            m_imageFBOManager.releaseImageFBO(fbo);
        }

        void flushImageFBOs() { m_imageFBOManager.flushImageFBOs(); }

        ImageFBOManager* manager() { return &m_imageFBOManager; }

        //
        //  General static
        //

        static void queryGL();
        static void queryGLIntoContainer(IPNode*);

        static bool queryGLFinished() { return !m_queryInit; }

        static bool queryFloatBuffers() { return m_floatBuffers; }

        static bool queryPixelBuffers() { return m_pixelBuffers; }

        static bool queryImaging() { return m_imaging; }

        static bool queryRectTextures() { return m_rectTextures; }

        static void defaultAllowPBOs(bool b) { m_defaultAllowPBOs = b; }

        static bool getDefaultAllowPBOs() { return m_defaultAllowPBOs; }

        static void setDrawPixelsOnly(bool b) { m_drawPixelsOnly = b; }

        static void setAltGetProcAddress(GetProcAddressFunc f)
        {
            m_procFunc = f;
        }

        static bool queryClientStorage() { return m_hasClientStorage; }

        static void setUseAppleClientStorage(bool b) { m_useClientStorage = b; }

        static bool useAppleClientStorage()
        {
            return m_useClientStorage && m_hasClientStorage;
        }

        static bool queryThreadedUpload() { return m_hasThreadedUpload; }

        static void setAllowThreadedUpload(bool b) { m_hasThreadedUpload = b; }

        static void setUseThreadedUpload(bool b) { m_useThreadedUpload = b; }

        static bool useThreadedUpload()
        {
            return m_useThreadedUpload && m_hasThreadedUpload;
        }

        static void reportGL(bool b) { m_reportGL = b; }

        static bool reportGL() { return m_reportGL; }

        static void setPBOs(bool b) { m_pixelBuffers = b; }

        static bool hasFloatFormats() { return m_floatFormats; }

        static const FBAcceptableTypes& fbAcceptableTypes()
        {
            return m_FBAcceptableTypes;
        }

        static BGPattern defaultBGPattern;

        static void setDefaultBGPattern(BGPattern p) { defaultBGPattern = p; }

        static void setPassDebug(bool b) { m_passDebug = b; }

        static void setNoGPUCache(bool b) { m_noGPUCache = b; }

        static void setOutputIntermediateDebug(bool b) { m_imageFBODebug = b; }

        static void setIntermediateLogging(bool b);

        //
        //  Image FBO related
        //

        ImageFBO* findExistingImageFBO(GLuint textureID);
        ImageFBO* findExistingImageFBO(const IPImage* image);

        //
        //  Miscellaneous
        //

        Box2f viewport() const;
        const VideoDevice* currentDevice() const;
        void clearState();
        void sync();
        void initProfilingState(Timer* t);

        void setBGPattern(BGPattern p) { m_bgpattern = p; }

        BGPattern bgPattern() const { return m_bgpattern; }

        void setFiltering(int f);

        int filterType() const { return m_filter; }

        const RenderedImagesVector* renderedImages() const
        {
            return &m_renderedImages;
        }

        void clearRenderedImages() { m_renderedImages.clear(); }

        GLState* getGLState() const { return m_glState; }

        void setMaxMem(size_t m) { m_maxmem = m; }

        bool supported() const { return queryRectTextures(); }

        std::string nextBestRenderer() const { return "Direct"; }

        float contentAspect() { return m_contentAspect; }

        const ProfilingState& profilingState() { return m_profilingState; }

        void flushProgramCache() { m_programCache->flush(); }

        size_t fullRenderSerialNum() { return m_fullRenderSerialNumber; }

        //
        // threaded upload
        //

        const IPImage* uploadRoot() const { return m_uploadRoot; }

        HashValue uploadRootHash() const { return m_uploadRootHash; }

        const bool stopUploadThread() const { return m_stopUploadThread; }

        const GLVideoDevice* uploadThreadDevice() const
        {
            return m_uploadThreadDevice;
        }

        ConditionalVariable& drawCond() { return m_drawCond; }

        Mutex& drawMutex() { return m_drawMutex; }

        const bool drawReady() const { return m_drawReady; }

        ConditionalVariable& uploadCond() { return m_uploadCond; }

        Mutex& uploadMutex() { return m_uploadMutex; }

        const bool uploadReady() const { return m_uploadReady; }

        void waitToUpload();
        void waitToDraw();
        void notifyDraw();
        void setDrawReady(bool v);
        void setUploadReady(bool v);
        void notifyUpload();
        void freeUploadedTextures();

        // resets node ptr on all rendered image links to this node
        void unlinkNode(IPCore::IPNode* node);

    private:
        //
        // OPENCL
        //
#ifdef PLATFORM_DARWIN
        void createCLContexts();
        void compileCLPrograms();
        void compileCLHistogram();
        void executeCLKernel(
            const CLContext& context, const cl_kernel& kernel,
            const size_t globalThreads[3], const size_t localThreads[3],
            const std::vector<std::pair<size_t, const void*>>& args) const;

        const bool CLContextNotSet() const { return !m_setCLContext; }
#endif

        //
        //  Internal rendering functions
        //

        void renderImage(InternalRenderContext&);
        void renderPaint(const IPImage*, const GLFBO*);

        void renderExternal(InternalRenderContext&);
        void renderRootBuffer(InternalRenderContext&);
        void renderRecursive(InternalRenderContext&);
        void renderCurrentImage(InternalRenderContext&,
                                const GLFBO* fbo = NULL);

        void renderAllChildren(InternalRenderContext&);
        void renderIntermediate(InternalRenderContext&,
                                const GLFBO* fbo = NULL);

        void renderNonIntermediate(InternalRenderContext&);
        void renderDataBuffer(InternalRenderContext&);

        void renderOutputs(int frame, const IPImage*, const AuxRender*,
                           const AuxAudio*);

        void renderMain(const GLFBO* Ftarget, const GLFBO* Ltarget,
                        const GLFBO* Rtarget, const AuxRender* auxRenderer,
                        int frame, const IPImage*);

        void renderBegin(const InternalRenderContext&);
        void renderEnd(const InternalRenderContext&);
        void renderInternal(const InternalRenderContext&);

        void assignTextureUnits(const IPImage*, bool);
        bool assignTextureUnits2(const IPImage*, LogicalImage*, size_t&,
                                 size_t);

        void activateTextures(LogicalImageVector&);

        void markProgramRequirements(const IPImage*,
                                     const InternalRenderContext&,
                                     const Shader::Program*);

        void recordTransforms(const InternalRenderContext&);

        void drawImage(const InternalRenderContext&);

        void drawMerge(const InternalRenderContext&);

        void computeMergeMatrix(const InternalRenderContext&,
                                const LogicalImage*, const IPImage*,
                                Mat44f&) const;

        void computeDataBufferMergeMatrix(const InternalRenderContext&,
                                          const IPImage*, Mat44f&) const;

        //
        // Prefetch
        //

        void prefetchInternal(const IPImage*);
        void prefetchRecursive(const IPImage*);

        //
        //  Functions that operate on ImagePassStates
        //

        void assignMemberImages(const IPImage*,
                                const ImageRenderer::ActiveImageVector&);

        void assembleAuxLogicalImage(const FrameBuffer*, LogicalImage*);

        void assemblePrimaryLogicalImage(const IPImage*, LogicalImage*);

        //
        //  Functions related to textures
        //

        TextureDescription* getTexture(const FrameBuffer*,
                                       TextureDescription::TextureMatch&);

        //  Same fb identifier different pixel aspect means a new upload
        std::string logicalImageHash(const IPImage*) const;

        void freeOldTextures();

        void uploadAuxImage(LogicalImage*, const FrameBuffer*);

        void uploadImage(LogicalImage*, const IPImage*);

        void assignImage(LogicalImage*, const IPImage*, const GLFBO*, bool,
                         bool, size_t);

        void assignAuxImage(LogicalImage* i, const FrameBuffer* fb);

        void assignAuxImages(const IPImage*);

        void computeImageGeometry(const IPImage* img, ImagePlane& plane) const;

        void computePlaneGeometry(const IPImage* img, const FrameBuffer* fb,
                                  ImagePlane& plane) const;

        void initializeTexture(const FrameBuffer* fb,
                               TextureDescription* tex) const;

        void initializeTextureFormat(const FrameBuffer* fb,
                                     TextureDescription* tex) const;

        void initializePlane(const IPImage* img, const GLFBO*,
                             ImagePlane&) const;

        void initializeFBPlane(const IPImage* img, const FrameBuffer*,
                               ImagePlane&, float, bool,
                               bool generateTexture = false) const;

        bool compatible(const FrameBuffer* fb,
                        const TextureDescription* tex) const;

        //
        //  Functions that operate on Image Plane
        //

        void uploadPlane(const FrameBuffer*, TextureDescription*,
                         GLuint filter);

        //
        //  Functions that operate on Texture
        //

        void upload3DTexture(const FrameBuffer*, GLuint pixelInterpolation,
                             TextureDescription*);
        void upload2DTexture(const FrameBuffer*, GLuint pixelInterpolation,
                             TextureDescription*);
        void upload1DTexture(const FrameBuffer*, GLuint pixelInterpolation,
                             TextureDescription*);

        //
        // Miscellaneous
        //

        //
        // if any imageFBO matches any IPImage of the current render, make it as
        // unavailable this way we can prevent others from grabbing this
        // imageFBO for this frame
        //
        void markReusableImageFBOs(const IPImage*);

        void assembleImagePassStates(const IPImage* root);
        void clearImagePassStates();

        void setupContextFromDevice(InternalRenderContext&) const;
        void setupBlendMode(InternalRenderContext&) const;

        void clearBackgroundToBlack(const GLFBO* target);
        void clearBackground(const GLFBO*);

        void setupUploadThread(IPImage* uploadRoot);
        void assembleTextureDescriptionsForUpload(const IPImage* img);

        FastPath findFastPath(const FrameBuffer*) const;
        std::string imageToFBOIdentifier(const IPImage* image) const;
        bool imageHasEraseCommands(const IPImage* image) const;

        void createGLContexts();

        const bool GLContextNotSet() const { return !m_setGLContext; }

        //
        // OPENCL
        //

#ifdef PLATFORM_DARWIN
        void computeHistogram(const ConstFBOVector& childrenFBO,
                              const GLFBO* resultFBO) const;
        void histogramOCL(cl_mem&, cl_mem&, const size_t, const size_t) const;
#endif

    private:
        RenderedImagesVector m_renderedImages;
        FBToTextureMap
            m_texturesToUpload; // render and upload thread will never
                                // read/write this simultaneously
        FBToTextureMap m_uploadedTextures; // only read/write from render thread
        ImagePassStateMap m_imagePassStates;
        ImageFBOManager m_imageFBOManager;
        bool m_uploadThreadPrefetch;
        bool m_setGLContext;
        GLVideoDevice* m_uploadThreadDevice;
        Thread m_uploadThread;
        bool m_stopUploadThread;

#ifdef PLATFORM_DARWIN
        bool m_setCLContext;
        CLContext m_clContext;
#endif

        ConditionalVariable m_drawCond;
        Mutex m_drawMutex;
        bool m_drawReady;
        ConditionalVariable m_uploadCond;
        Mutex m_uploadMutex;
        bool m_uploadReady;

        std::string m_name;
        InternalRenderContext* m_rootContext;
        Device m_controlDevice;
        Device m_outputDevice;
        size_t m_defaultDeviceFBORingBufferSize;
        size_t m_deviceFBORingBufferIndex;

        size_t m_fullRenderSerialNumber;
        size_t m_mainRenderSerialNumber;

        IPImage* m_uploadRoot;
        HashValue m_uploadRootHash;

        size_t m_maxmem;
        int m_filter;

        BGPattern m_bgpattern;

        float m_contentAspect;
        size_t m_imageCount;

        Matrix m_displayXformMatrix;
        bool m_shaderOverride;

        GLState* m_glState;

        Shader::ProgramCache* m_programCache;
        ProfilingState m_profilingState;

        TextureUnitAssignments m_unitAssignments;

        static bool m_noGPUCache;
        static bool m_imageFBODebug;
        static bool m_passDebug;
        static bool m_queryInit;
        static bool m_floatBuffers;
        static bool m_pixelBuffers;
        static bool m_textureFloat;
        static bool m_halfFloat;
        static bool m_rectTextures;
        static bool m_useTextures;
        static bool m_hasClientStorage;
        static bool m_useClientStorage;
        static bool m_hasThreadedUpload;
        static bool m_useThreadedUpload;
        static bool m_imaging;
        static bool m_fragmentProgram;
        static bool m_drawPixelsOnly;
        static bool m_defaultAllowPBOs;
        static bool m_ycbcrApple;
        static bool m_reportGL;
        static bool m_nonPowerOf2;
        static bool m_softwareGLRenderer;
        static int m_ALUinsnLimit;
        static int m_tempLimit;
        static int m_maxW;
        static int m_maxH;
        static int m_maxImageUnits;
        static int m_maxTexCoords;
        static int m_maxVertexAttrs;
        static bool m_floatFormats;
        static GetProcAddressFunc m_procFunc;
        static FBAcceptableTypes m_FBAcceptableTypes;
    };

} // namespace IPCore

#endif // __IPCore__ImageRenderer__h__
