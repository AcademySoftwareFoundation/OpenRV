//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__IPImage__h__
#define __IPCore__IPImage__h__
#include <IPCore/ShaderExpression.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Color.h>
#include <TwkMath/Box.h>
#include <TwkMovie/Movie.h>
#include <TwkApp/VideoDevice.h>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <sstream>

namespace TwkApp
{
    class VideoDevice;
}

namespace IPCore
{
    class IPNode;
    class IPImageID;

    namespace Paint
    {
        class Command;
    }

    //
    //  struct IPImage
    //
    //  IPImage holds zero, one, or two Shader::Expressions and represents both
    //  a coordinate system and possibly a render pass. I.e. some IPImages
    //  result in actual pixel buffers during rendering and others are merely
    //  used to determine where "virtual" image exists in space during a render
    //  pass.
    //
    //  IPImages form a tree. Each IPImage can have multiple children.  An
    //  IPImage tree is the result of IPNode::evaluate. The IPNode DAG produces
    //  a IPImage tree at each node. An IPImage tree may or may not resemble
    //  the IPNode DAG which produced it.
    //
    //  IMPORTANT NOTE: these things are created and destroyed frequently by
    //  graph evaluation threads and the main thread. On Windows, which has a
    //  completely serialized allocator, this can be come a serious problem. So
    //  these objects are allocated by the "alternate" allocator defined in
    //  stl_ext/replace_alloc.h. On Windows and Linux this is nedmalloc which
    //  excels at multithreaded allocation. On the Mac the allocator is good
    //  enough so we use the system one. (At least as of the writing of this
    //  comment).
    //
    //  NOTE ON DELETION: if you put an fb in an IPImage which is checked into
    //  the cache, you have to let the cache delete the IPImage. Its ok, to use
    //  the cache to delete it even if the fbs are NOT checked in. Something is
    //  obviously wrong with the way these classes interact.
    //
    //  Additional FBs used by the IPImage's shader program can be added
    //  into auxFBs. For FBs used by a merge expr not already on one of
    //  the input images use mergeFBs.
    //
    //  For compositing, each image indicates how it will be composited against
    //  the existing framebuffer. For example an over of two images would have
    //  the first image using Atop and the second using Over resulting in a
    //  slap comp.
    //
    //  If hardware stereo is indicated, than each image will have a left and
    //  right target buffer.
    //
    //  The "next" field means "next sibling".
    //
    //  IDENTIFIERS: there are two identifiers used by IPImage, the
    //  graphID nad the renderID. The graphID is used by the
    //  Shader::Program and is automatically generated.
    //
    //  The renderID is unique for a given graph topology + set of all
    //  parameter values. So e.g. changing any parameter in the graph
    //  leading up to a IPImage's creation should result in a new value
    //  for the renderID. If the pixels are a function of time/frame, then
    //  time needs to be incorporated as a parameter somewhere (e.g. a
    //  shader parameter or is part of an incoming fb hash). There can be
    //  no hidden parameters for the renderID to work.
    //
    //  The renderID is automatically generated.
    //
    //  TAGS: any node can add tags to an IPImage. These are (name, value)
    //  pairs of type (string, string). The renderer has functions to
    //  retrieve information about rendered images (or OutputTexture
    //  images) by searching tags. The tags are maintained by the renderer
    //  in the LogicalImage structure so that the UI can search them
    //  without having access to the IPImages that created them.
    //
    //  In particular, the tag IPImage::textureIDTagName() is used in
    //  conjunction with IPImage::OutputTexture images to find the texure
    //  ID of the render for that image. For example the audio waveform is
    //  created as an FB passed as an IPImage::OutputTexture to the
    //  renderer. It has a tag indicating it is the "_waveform".
    //

    class IPImage
    {
    public:
        typedef Shader::Expression Expression;
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef TwkMath::Vec4f Vec4;
        typedef TwkMath::Vec3f Vec3;
        typedef TwkMath::Vec2f Vec2;
        typedef TwkMath::Box2f Box2;
        typedef TwkMath::Mat33f Matrix33;
        typedef TwkMath::Mat44f Matrix;
        typedef TwkMovie::MovieInfo MovieInfo;
        typedef TwkMath::Col4f Color;
        typedef std::stringstream ShaderStream;
        typedef stl_ext::replacement_allocator<const FrameBuffer*> FBAlloc;
        typedef std::vector<const FrameBuffer*, FBAlloc> FBVector;
        typedef std::vector<const Paint::Command*> PaintCommands;
        typedef std::vector<IPImage*> IPImageVector;
        typedef std::map<std::string, std::string> TagMap;
        typedef Shader::Function::ResourceUsage ResourceUsage;
        typedef TwkApp::VideoDevice VideoDevice;
        typedef TwkApp::VideoDevice::Margins Margins;
        typedef unsigned int HashValue;

        //
        //  NOTE: if you add/change a RenderType and/or RenderDestination
        //  enum value you probably have to change the logic which
        //  computes the graphID. That can be found in
        //  computeGraphIDRecursive().
        //

        enum RenderType
        {
            BlendRenderType,    // use hardware blending
            MergeRenderType,    // children merged to FBO
            RecordOnlyType,     // record image transforms only
            GroupType,          // no rendering children are all roots
            ExternalRenderType, // external (user) render
            NoRenderType
        };

        enum RenderDestination
        {
            CurrentFrameBuffer, // some other buffer (no buffer for this image)
            IntermediateBuffer, // cached with decay
            TemporaryBuffer,    // never cached
            LeftBuffer,         // device left
            RightBuffer,        // device right
            MainBuffer,         // main window buffer
            NoBuffer,           // not rendered
            OutputTexture, // upload texture for UI use. requires textureID tag
            DataBuffer // eithr a opencl node or cpu node will write its result
                       // into this buffer
        };

        enum BlendMode
        {
            UnspecifiedBlendMode,
            Replace,
            Over,
            Add,
            Difference,
            ReverseDifference,
            Dissolve
        };

        enum InternalDataType
        {
            NoInternalDataType,
            HalfDataType,
            FloatDataType,
            UInt8DataType,
            UInt16DataType,
            UInt10A2DataType,
            UInt10A2RevDataType
        };

        enum SamplerType
        {
            Rect2DSampler,
            NDC2DSampler
        };

        static void* operator new(size_t s);
        static void operator delete(void* p, size_t s);

        //
        //  Common internal tag names and values. The renderer uses some
        //  of these when queried for a particular image.
        //

        static const char* textureIDTagName();
        static const char* waveformTagValue();

        //
        //  Constructors
        //

        IPImage(const IPNode*); // default

        IPImage(const IPNode*, const VideoDevice* device, RenderType type,
                RenderDestination dest, SamplerType stype = Rect2DSampler,
                bool useDeviceMargins = true);

        IPImage(const IPNode*, // for a source or from cache
                RenderType type, FrameBuffer* fb,
                RenderDestination dest = CurrentFrameBuffer,
                SamplerType stype = Rect2DSampler);

        IPImage(const IPNode*, RenderType type, size_t width = 0,
                size_t height = 0, float pixelAspect = 1.0f,
                RenderDestination dest = CurrentFrameBuffer,
                InternalDataType dataType = NoInternalDataType,
                SamplerType stype = Rect2DSampler);

        ~IPImage();

        //
        //  These are constructors for special images
        //

        static FrameBuffer* newBlankFrameBuffer(size_t w = 1280,
                                                size_t h = 720);
        static FrameBuffer* newBlackFrameBuffer(size_t w = 1280,
                                                size_t h = 720);
        static FrameBuffer* newNoImageFrameBuffer(size_t w = 1280,
                                                  size_t h = 720);
        static FrameBuffer* newErrorFrameBuffer(size_t w = 1280,
                                                size_t h = 720);
        static FrameBuffer*
        newNoImageFrameBufferWithAttrs(IPNode*, size_t w, size_t h,
                                       const std::string& message);

        static IPImage* newBlankImage(IPNode*, size_t w, size_t h);
        static IPImage* newBlackImage(IPNode*, size_t w, size_t h);
        static IPImage* newNoImage(IPNode*, const std::string& message = "");
        static IPImage* newErrorImage(IPNode*,
                                      const std::string& errorMessage = "");

        static IPImageID* newBlankImageID(size_t w, size_t h);
        static IPImageID* newBlackImageID(size_t w, size_t h);
        static IPImageID* newNoImageID();
        static IPImageID* newErrorImageID();

        static BlendMode getBlendModeFromString(const char* blendModeString);

        //
        //  These will only return true if the image was made using the
        //  above special image constructors.
        //

        bool isType(const std::string&) const;
        bool isBlank() const;
        bool isBlack() const;
        bool isNoImage() const;
        bool isError() const;

        bool isIntermediateRender() const;
        bool isRootRender() const;
        bool isExternalRender() const;
        bool isDataBuffer() const;
        bool isNoBuffer() const;

        //
        //  Set an existing image to be an Error. This assumes an FB
        //  exists on the image
        //

        void setErrorState(IPNode*, const std::string& message = "",
                           const std::string& type = "Error");

        void setHistogram(bool h) { isHistogram = h; }

        //
        //  IPImage children
        //

        void append(IPImage*);
        void appendChild(IPImage*);
        void appendChildren(const std::vector<IPImage*>&);

        size_t coordID() const { return m_coordID; }

        const std::string& graphID() const;
        const std::string& renderID() const;
        const std::string& renderIDWithPartialPaint() const;
        HashValue fbHash() const;
        HashValue renderIDHash() const;

        size_t allocSize() const;
        size_t totalImageSize() const;

        size_t numImages() const;
        size_t numChildren() const;

        int displayWidth() const;
        int displayHeight() const;

        float displayAspect() const
        {
            return float(displayWidth()) / float(displayHeight());
        }

        float displayPixelAspect() const;

        void computeGraphIDs();

        void assembleAuxFrameBuffers();

        void init();

        //
        //  Some common operations
        //

        void fitToAspect(float aspectToFit); // sets the transform on this
        Matrix fitToAspectMatrix(float aspectToFit) const;
        void recordResourceUsage();

        struct InternalGLMatricesContext
        {
            InternalGLMatricesContext()
                : outputWidth(0)
                , outputHeight(0)
                , controlDevice(NULL)
                , outputDevice(NULL)
            {
            }

            Matrix parentMatrix;
            Matrix
                parentMatrixGlobal; //  includes intermediate buffer transitions
            size_t outputWidth;
            size_t outputHeight;
            Matrix projectionMatrix;
            Matrix projectionMatrixGlobal; //  includes intermediate buffer
                                           //  transitions
            Box2 viewport;
            const VideoDevice* controlDevice;
            const VideoDevice* outputDevice;
            //
            //  imageNum has nothing to do with matrices, but we use the same
            //  recursion to store it on IPImages.
            //
            size_t* imageNumCounter;
        };

        void populateTransformAsNeeded();
        void computeMatrices(const VideoDevice* controlDevice,
                             const VideoDevice* outputDevice);
        void
        computeMatricesRecursive(const InternalGLMatricesContext& baseContext);
        TwkMath::Mat44f computeOrientationMatrix(const FrameBuffer* fb) const;

        void computeRenderIDRecursive();

        //
        //  These determine how the IPImage will be treated by the
        //  renderer.
        //

        RenderType renderType;
        BlendMode blendMode;
        RenderDestination destination;
        SamplerType samplerType;
        size_t branchCount;
        const VideoDevice* device;

        //
        //  Children are stored as linked lists
        //

        IPImage* children; // pointer to first child
        IPImage* next;     // pointer to next sibling

        const IPNode* node; // node that created the IPImage
        TagMap tagMap;      // information maintained by renderer

        FrameBuffer* fb;     // image pixels
        bool noIntermediate; // true --> do not render this
                             // node as an intermediate.

        //
        // these two are used by texture upload only
        //

        mutable FBVector auxFBs;      // LUTs, etc of a shader expr
        mutable FBVector auxMergeFBs; // LUTs, etc of a merge shader expr

        int width;             // should reflect fb state
        int height;            //  if there is one
        int imageNum;          // order in which this image is rendered
        float pixelAspect;     // pixel aspect ratio
        float initPixelAspect; // Original aspect ratio (no Lens)
        InternalDataType dataType;
        int missingLocalFrame; // if missing is true this is meaningful

        //
        //  Geometric State
        //

        const MovieInfo* info;  // original source info
        Matrix transformMatrix; // image geometry transform (rotation, etc)
        Matrix modelViewMatrix; // modelView matrix
        Matrix modelViewMatrixGlobal;  // modelView matrix (including
                                       // intermediate buffer transitions)
        Matrix projectionMatrix;       // perspective matrix
        Matrix projectionMatrixGlobal; // perspective matrix (including
                                       // intermediate buffer transitions)
        Matrix imageMatrix;
        Matrix orientationMatrix; // NOTE that the orientation matrix actually
                                  // holds the orientation which is needed by
                                  // the UI for example but its not part of the
                                  // transform computation in rendering because
                                  // the source shaders takes care of sampling
                                  // w.r.t. orientation
        Matrix placementMatrix;
        Matrix parentMatrix; // w.r.t. current intermediate

        Matrix33 textureMatrix;

        Box2 viewport;   // viewport
        Box2 stencilBox; // box in image space

        Expression* shaderExpr; // Shader bindings
        Expression* mergeExpr;  // Shader bindings
        PaintCommands commands; // Paint geometry to render

        ResourceUsage resourceUsage;

        bool unpremulted : 1;   // image has unpremulted alpha
        bool minMax : 1;        // normalize color space flag
        bool ignore : 1;        // true == don't render
        bool missing : 1;       // image represents a missing image
        bool invalid : 1;       // image represents an out-of-range image
        bool useBackground : 1; // draws background for this image
        bool isHistogram : 1;

        size_t hashCount;

        //
        // isCropped crop* is a way to notify the renderer which part of this
        // image should be rendered. pixels outside of the crop window is not
        // rendered. this is different from uncropped Framebuffers, because in
        // case of uncropped (data window) only the data window will be rendered
        // but the 'fit' is determined by the whole image size, NOT the data
        // window. In case of cropped, the render will fit the cropped region
        // into the target, effectively render as if there was no pixels outside
        // of the crop Note this could be achieved by a transform, but it was
        // implemented this way to avoid using transformMatrix because
        // transformMatrix is already overloaded and 'incorrect' in many cases.
        // We should first clean out the transformMatrix / sampling before we
        // change the implementation here.
        //
        bool isCropped;
        size_t cropStartX;
        size_t cropStartY;
        size_t cropEndX;
        size_t cropEndY;

        std::string source;

        bool supportReversedOrderBlending;

    private:
        void clear();

        bool renderIDNeedsCompute() const { return m_renderID == ""; }

        void renderIDHashRecursive(std::ostream&);
        void computeGraphIDRecursive(const IPImage*, size_t, size_t&) const;
        void computeRenderIDs() const;

        mutable HashValue m_renderIDHash; // 32 bit crc
        mutable HashValue m_fbHash;       // 32 bit crc
        mutable std::string m_renderID;
        mutable std::string m_renderIDWithPartialPaint;
        mutable std::string m_graphID;
        mutable size_t m_coordID;
    };

    typedef std::vector<IPImage*> IPImageFrameVector;
    typedef std::vector<IPImage*> IPImageVector;
    typedef std::set<IPImage*> IPImageSet;

    //
    //  IPImageID wraps a single string which is usually the same string
    //  used by TwkFB::FrameBuffer as the cache key. The keys are used to
    //  locate items in the cache. The CacheIPNode will use this struct to
    //  generate a new IPImage struct of the same topology.
    //
    //  The next field means "next sibling" since the struct holds a list
    //  of children as a linked list.
    //

    class IPImageID
    {
    public:
        typedef std::string ID;

        IPImageID(const ID& id_ = "", IPImageID* next_ = 0,
                  IPImageID* children_ = 0)
            : id(id_)
            , next(next_)
            , children(children_)
            , noIntermediate(false)
        {
        }

        ~IPImageID()
        {
            delete children;
            delete next;
        }

        void append(IPImageID*);
        void appendChild(IPImageID*);
        void appendChildren(const std::vector<IPImageID*>&);

        ID id;
        IPImageID* children;
        IPImageID* next;
        bool noIntermediate; // true --> do not render this node as an
                             // intermediate.
    };

    //
    //  This iterates over a tree of either IPImage or IPImageID. The callable
    //  type should take an IPImage*. The callable can be modified by this
    //  function to accumulate state.
    //

    template <class Image, class Callable>
    void foreach_ip(Image* root, Callable& callable)
    {
        if (!root)
            return;
        foreach_ip<Image, Callable>(root->next, callable);
        foreach_ip<Image, Callable>(root->children, callable);
        callable(root);
    }

    //
    //  This is used to transform one IPImage or IPImageID tree into a new
    //  one. An example is the CacheIPNode which onverts an IPImageID tree
    //  into an IPImage tree by looking up fbs in the cache from the image
    //  identifiers. This function is annoying to call because you *must*
    //  use the notation supplying all of the types like e.g.:
    //
    //      transform_ip<IPImage,IPImage,Callable>(root, F)
    //
    //  This is becaue the compiler can't handle the inference required by
    //  the return type (Rtype).
    //

    template <class Rtype, class Image, class Callable>
    Rtype* transform_ip(Image* root, Callable& callable)
    {
        if (!root)
            return 0;

        Rtype* newRoot = callable(root);
        newRoot->next =
            transform_ip<Rtype, Image, Callable>(root->next, callable);
        newRoot->children =
            transform_ip<Rtype, Image, Callable>(root->children, callable);
        return newRoot;
    }

    //
    //  For debugging: passing this to foreach_ip will print the IPImage tree
    //  out
    //

    void printTree(std::ostream&, const IPImage*);
    void printTreeStdout(const IPImage*);

    void printIDTree(std::ostream&, const IPImageID*);

    //
    //  Some operations
    //

    void recordResourceUsage(IPImage* image); // includes children

} // namespace IPCore

#endif // __IPCore__IPImage__h__
