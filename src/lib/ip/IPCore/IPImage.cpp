//******************************************************************************
// Copyright (c) 2006 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <IPCore/IPImage.h>
#include <IPCore/PaintCommand.h>
#include <IPCore/ShaderExpression.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/IPNode.h>
#include <TwkFB/Operations.h>
#include <TwkApp/VideoDevice.h>
#include <TwkMath/Frustum.h>
#include <iostream>
#include <stl_ext/replace_alloc.h>
#include <sstream>
#include <boost/functional/hash.hpp>

namespace IPCore {
using namespace TwkFB;
using namespace std;
using namespace TwkMath;

namespace {
unsigned char blankImage[4] = {0, 0, 0, 0};
unsigned char blackImage[4] = {0, 0, 0, 1};
}

TWK_CLASS_NEW_DELETE(IPImage);

const char* IPImage::textureIDTagName() { return "_textureID"; }
const char* IPImage::waveformTagValue() { return "_waveform"; }

void
IPImage::init()
{
    fb                           = 0;
    noIntermediate               = false;
    next                         = 0;
    children                     = 0;
    info                         = 0;
    dataType                     = NoInternalDataType;
    pixelAspect                  = 0.0;      // ignore if 0
    initPixelAspect              = 0.0;
    minMax                       = false;
    blendMode                    = UnspecifiedBlendMode;
    renderType                   = BlendRenderType;
    ignore                       = false;
    missing                      = false;
    invalid                      = false;
    useBackground                = false;
    destination                  = CurrentFrameBuffer;
    shaderExpr                   = 0;
    mergeExpr                    = 0;
    branchCount                  = 0;
    m_coordID                    = size_t(-1);
    device                       = 0;
    unpremulted                  = false;
    m_renderIDHash               = 0;
    imageNum                     = -1;
    samplerType                  = Rect2DSampler;
    hashCount                    = 0;
    isHistogram                  = false;
    isCropped                    = false;
    cropStartX                   = 0;
    cropStartY                   = 0;
    cropEndX                     = 0;
    cropEndY                     = 0;
    supportReversedOrderBlending = true;
    width                        = 0;
    height                       = 0;
}

IPImage::IPImage(const IPNode* n) : node(n) { init(); }

IPImage::IPImage(const IPNode* n,
                 const VideoDevice* d,
                 RenderType type,
                 RenderDestination dest,
                 SamplerType stype,
                 bool useDeviceMargins) : node(n)
{
    VideoDevice::Resolution res = d ? d->resolution() : VideoDevice::Resolution(1,1,1.0,1.0);
    VideoDevice::Margins m = d && useDeviceMargins ? d->margins() : VideoDevice::Margins();

    init();
    fb          = 0;
    pixelAspect = res.pixelAspect;
    initPixelAspect = res.pixelAspect;
    width       = d->internalWidth()  - m.left - m.right;
    height      = d->internalHeight() - m.top - m.bottom;
    if (pixelAspect > 0.0 && pixelAspect < 1.0) height /= pixelAspect;
    if (pixelAspect > 1.0) width *= pixelAspect;
    
    destination = dest;
    renderType  = type;
    branchCount = n->outputs().size();
    device      = d;
    samplerType = stype;
}

IPImage::IPImage(const IPNode* n,
                 RenderType type,
                 FrameBuffer* srcFB,
                 RenderDestination dest,
                 SamplerType stype) : node(n)
{
    init();
    fb          = srcFB;
    pixelAspect = fb->pixelAspectRatio();
    initPixelAspect = fb->pixelAspectRatio();
    width       = fb->uncrop()? fb->uncropWidth() : fb->width();
    height      = fb->uncrop()? fb->uncropHeight() : fb->height();
    destination = dest;
    renderType  = type;
    branchCount = n->outputs().size();
    samplerType = stype;

    if (pixelAspect < 1.0) height /= pixelAspect;
    if (pixelAspect > 1.0) width *= pixelAspect;

    recordResourceUsage();
}

IPImage::IPImage(const IPNode* n, 
                 RenderType type,
                 size_t w,
                 size_t h,
                 float pa,
                 RenderDestination dest,
                 InternalDataType dtype,
                 SamplerType stype) : node(n)
{
    init();
    width       = w;
    height      = h;
    dataType    = dtype;
    pixelAspect = pa;
    initPixelAspect = pa;
    renderType  = type;
    destination = dest;
    samplerType = stype;
}


IPImage::~IPImage()
{
    //cout << "delete ipimage " << this << endl;
    assert(this != (IPImage*)0xdeadc0de);
    assert(next != (IPImage*)0xdeadc0de); // this was already deleted
    assert(children != (IPImage*)0xdeadc0de); // this was already deleted
    delete children;
    delete next;
    children = (IPImage*)0xdeadc0de;
    next = (IPImage*)0xdeadc0de;
    clear();
    fb = (FrameBuffer*)0xdeadc0de;
    commands.clear();
}


namespace {

FrameBuffer* 
newSpecialFB(unsigned char* pixels, size_t w, size_t h)
{
    FrameBuffer* fb = new FrameBuffer(1, 1, 4,
                                      FrameBuffer::UCHAR,
                                      pixels,
                                      0, 
                                      FrameBuffer::NATURAL,
                                      false);

    fb->setUncrop(w, h, 0, 0);
    fb->idstream() << fb->pixels<void>();
    return fb;
}

}

FrameBuffer* 
IPImage::newBlankFrameBuffer(size_t w, size_t h)
{
    FrameBuffer* fb = newSpecialFB(blankImage, w, h);
    fb->newAttribute<string>("Type", "Blank");
    fb->idstream() << "(BLANK)";
    return fb;
}

FrameBuffer* 
IPImage::newBlackFrameBuffer(size_t w, size_t h)
{
    FrameBuffer* fb = newSpecialFB(blackImage, w, h);
    fb->newAttribute<string>("Type", "Black");
    fb->idstream() << "(BLACK)";
    return fb;
}

FrameBuffer*
IPImage::newNoImageFrameBuffer(size_t w, size_t h)
{
    FrameBuffer* fb = newBlankFrameBuffer(w, h);
    fb->attribute<string>("Type") = "NoImage";
    fb->idstream() << "(NOIMAGE)";
    return fb;
}

FrameBuffer*
IPImage::newErrorFrameBuffer(size_t w, size_t h)
{
    FrameBuffer* fb = newBlankFrameBuffer(w, h);
    fb->idstream() << "(ERROR)";
    return fb;
}

FrameBuffer*
IPImage::newNoImageFrameBufferWithAttrs(IPNode* node, 
                                        size_t w,
                                        size_t h,
                                        const string& message)
{
    FrameBuffer* fb = newNoImageFrameBuffer(w, h);
    if (node) fb->newAttribute<string>("Node", node->uiName());
    if (message != "") fb->newAttribute<string>("Message", message);
    return fb;
}

IPImage*
IPImage::newBlankImage(IPNode* node, size_t w, size_t h)
{
    IPImage* image = new IPImage(node, IPImage::BlendRenderType, newBlankFrameBuffer(w, h));
    image->shaderExpr = Shader::newSourceRGBA(image);
    return image;
}


IPImage*
IPImage::newBlackImage(IPNode* node, size_t w, size_t h)
{
    IPImage* image = new IPImage(node, IPImage::BlendRenderType, newBlackFrameBuffer(w, h));
    image->shaderExpr = Shader::newSourceRGBA(image);
    return image;
}

void 
IPImage::setErrorState(IPNode* node, const string& message, const string& type)
{
    if (fb)
    {
        if (node) fb->newAttribute<string>("Node", node->uiName());
        fb->attribute<string>("Type") = "Error";
        fb->newAttribute<string>("Message", message);
    }
}

IPImage*
IPImage::newErrorImage(IPNode* node, const string& message)
{
    IPImage* image = new IPImage(node, IPImage::BlendRenderType, newErrorFrameBuffer());
    image->shaderExpr = Shader::newSourceRGBA(image);
    image->setErrorState(node);
    return image;
}

IPImage*
IPImage::newNoImage(IPNode* node, const string& message)
{
    FrameBuffer* fb = newNoImageFrameBufferWithAttrs(node, 1280, 720, message);
    IPImage* image = new IPImage(node, IPImage::BlendRenderType, fb);
    image->shaderExpr = Shader::newSourceRGBA(image);
    return image;
}

IPImageID*
IPImage::newBlankImageID(size_t w, size_t h)
{
    FrameBuffer* fb = newBlankFrameBuffer(w, h);
    IPImageID* id = new IPImageID(fb->identifier());
    delete fb;
    return id;
}

IPImageID*
IPImage::newBlackImageID(size_t w, size_t h)
{
    FrameBuffer* fb = newBlackFrameBuffer(w, h);
    IPImageID* id = new IPImageID(fb->identifier());
    delete fb;
    return id;
}

IPImageID*
IPImage::newNoImageID()
{
    FrameBuffer* fb = newNoImageFrameBuffer();
    IPImageID* id = new IPImageID(fb->identifier());
    delete fb;
    return id;
}

IPImageID*
IPImage::newErrorImageID()
{
    FrameBuffer* fb = newErrorFrameBuffer();
    IPImageID* id = new IPImageID(fb->identifier());
    delete fb;
    return id;
}

IPImage::BlendMode
IPImage::getBlendModeFromString( const char* blendModeString )
{
    IPImage::BlendMode blendMode = IPImage::Over;

    if      (!strcmp(blendModeString, "over"))         blendMode = IPImage::Over;
    else if (!strcmp(blendModeString, "add"))          blendMode = IPImage::Add;
    else if (!strcmp(blendModeString, "difference"))   blendMode = IPImage::Difference;
    else if (!strcmp(blendModeString, "-difference"))  blendMode = IPImage::ReverseDifference;
    else if (!strcmp(blendModeString, "dissolve"))     blendMode = IPImage::Dissolve;
    else if (!strcmp(blendModeString, "replace"))      blendMode = IPImage::Replace;
    else if (!strcmp(blendModeString, "topmost"))      blendMode = IPImage::Replace;

    return blendMode;
}

bool 
IPImage::isType(const std::string& type) const
{
    return fb && 
           fb->hasAttribute("Type") &&
           fb->attribute<string>("Type") == type;
}

bool IPImage::isBlank() const { return isType("Blank"); } 
bool IPImage::isBlack() const { return isType("Black"); } 
bool IPImage::isNoImage() const { return isType("NoImage"); } 
bool IPImage::isError() const { return isType("Error"); }

bool 
IPImage::isIntermediateRender() const
{
    return destination == IntermediateBuffer || 
           destination == TemporaryBuffer || 
           destination == OutputTexture;
}

bool 
IPImage::isRootRender() const
{
    return destination == LeftBuffer || 
           destination == RightBuffer || 
           destination == MainBuffer;
}

bool 
IPImage::isExternalRender() const
{
    return renderType == ExternalRenderType;
}

bool 
IPImage::isNoBuffer() const
{
    return destination == NoBuffer;
}
    
bool
IPImage::isDataBuffer() const
{
    return destination == DataBuffer;
}

size_t
IPImage::allocSize() const
{
    size_t size = 0;

    for (const IPImage* i = this; i; i = i->next) 
    {
        size += i->fb->allocSize();
    }

    return size;
}

size_t
IPImage::totalImageSize() const
{
    size_t size = 0;

    for (const IPImage* i = this; i; i = i->next) 
    {
        size += i->fb->totalImageSize();
    }

    return size;
}

void
IPImage::clear()
{
    if (fb && !fb->inCache())
    {
       if (!fb->hasStaticRef() || fb->staticUnRef()) 
       {
           delete fb;  
       }
    }

    for (size_t i = 0; i < auxFBs.size(); i++)
    {
        const FrameBuffer* fb1 = auxFBs[i];
        if (fb1 && !fb1->inCache())
        {
            if (!fb1->hasStaticRef() || fb1->staticUnRef()) 
            {
                delete fb1;
            }
        } 
    }

    for (size_t i = 0; i < auxMergeFBs.size(); i++)
    {
        const FrameBuffer* fb1 = auxMergeFBs[i];
        if (fb1 && !fb1->inCache())
        {
            if (!fb1->hasStaticRef() || fb1->staticUnRef()) 
            {
                delete fb1;
            }
        } 
    }

    //
    //  The GLSL shaders are all owned by the IPImage. 
    // 
    //  Paint Commands are currently owned by the nodes that put them
    //  there.
    //

    transformMatrix.makeIdentity();
    stencilBox.makeEmpty();

    fb          = 0;
    dataType    = NoInternalDataType;
    node        = 0;
    info        = 0;
    pixelAspect = 1.0;
    missing     = false;
    invalid     = false;

    auxFBs.clear();
    auxMergeFBs.clear();
    
    delete shaderExpr;
    delete mergeExpr;
    shaderExpr = 0;
    mergeExpr = 0;

    m_renderID             = "";
    m_renderIDWithPartialPaint = "";
    m_renderIDHash         = 0;
}

void
IPImage::append(IPImage* img)
{
    IPImage* i;
    for (i = this; i->next; i = i->next);
    i->next = img;
}

void
IPImage::appendChild(IPImage* img)
{
    if (children) children->append(img);
    else children = img;
}

void 
IPImage::appendChildren(const std::vector<IPImage*>& images)
{
    for (size_t i = 0; i < images.size(); i++) appendChild(images[i]);
}

size_t
IPImage::numImages() const
{
    size_t n = 0;
    for (const IPImage* i = this; i; i = i->next, n++);
    return n;
}

size_t
IPImage::numChildren() const
{
    if (children) return children->numImages();
    else return 0;
}

float 
IPImage::displayPixelAspect() const
{
    if (fb) return pixelAspect ? pixelAspect : fb->pixelAspectRatio();
    else return pixelAspect ? pixelAspect : 1.0;
}

int
IPImage::displayWidth() const
{
    if (destination == IntermediateBuffer ||
        destination == OutputTexture      ||
        destination == DataBuffer         ||
        destination == CurrentFrameBuffer ||
        destination == TemporaryBuffer)
    {
        return width;
    }
    else if (children)
    {
        return children->displayWidth();
    }
    else
    {
        return 720;
    }
}

int
IPImage::displayHeight() const
{
    if (destination == IntermediateBuffer ||
        destination == OutputTexture      ||
        destination == DataBuffer         ||
        destination == CurrentFrameBuffer ||
        destination == TemporaryBuffer)
    {
        return height;
    }
    else if (children)
    {
        return children->displayHeight();
    }
    else
    {
        return 480;
    }
}

void
IPImage::recordResourceUsage()
{
    if (mergeExpr)
    {
        resourceUsage = mergeExpr->computeResourceUsageRecursive();
    }
    else if (fb)
    {
        resourceUsage.set(fb->numPlanes(), fb->numPlanes(), 1);
    }
    else if (renderType == BlendRenderType)
    {
        resourceUsage.clear();

        for (IPImage* i = children; i; i = i->next)
        {
            resourceUsage.accumulate(i->resourceUsage);
        }
    }
    else
    {
        resourceUsage = ResourceUsage();
    }
}

IPImage::Matrix 
IPImage::fitToAspectMatrix(float aspect) const
{
    Matrix M;
    float imgAspect = displayAspect();
    
    if (imgAspect > aspect)
    {
        float s = aspect / imgAspect;
        M.makeScale(TwkMath::Vec3f(s, s, 1.0));
    }

    return M;
}

void 
IPImage::fitToAspect(float aspect)
{
    //
    //  Note, we don't want to use *= here, since that implies applying the
    //  "fit" matrix first, and we want to apply the fit matrix to the result
    //  of the original transformMatrix.  This doesn't often matter, but will
    //  mess up stereo offset in the stereo case.
    //
    transformMatrix = fitToAspectMatrix(aspect) * transformMatrix;
}

static void
hashMatrix (ostream& o, const IPImage::Matrix& M)
{
    o << "[" << M(0,0) << "," << M(0,1) << "," << M(0,2) << "," << M(0,3)
      << "," << M(1,0) << "," << M(1,1) << "," << M(1,2) << "," << M(1,3)
      << "," << M(2,0) << "," << M(2,1) << "," << M(2,2) << "," << M(2,3)
      << "," << M(3,0) << "," << M(3,1) << "," << M(3,2) << "," << M(3,3)
      << "]";
}
    
static void
hashMatrix33 (ostream& o, const IPImage::Matrix33& M)
{
    o << "[" << M(0,0) << "," << M(0,1) << "," << M(0,2)
    << "," << M(1,0) << "," << M(1,1) << "," << M(1,2)
    << "," << M(2,0) << "," << M(2,1) << "," << M(2,2)
    << "]";
}

static void
hashBox (ostream& o, const IPImage::Box2& box)
{
    o << "[[" << box.min.x << "," << box.min.y 
      << "][" << box.max.x << "," << box.max.y 
      << "]]";
}

Mat44f 
IPImage::computeOrientationMatrix(const FrameBuffer* fb) const
{
    Mat44f T;

    if (fb)
    {
        if (fb->orientation() == FrameBuffer::TOPLEFT ||
            fb->orientation() == FrameBuffer::TOPRIGHT)
        {
            Mat44f S;
            S.makeScale(Vec3f(1, -1, 1));
            T *= S;
        }
    
        if (fb->orientation() == FrameBuffer::TOPRIGHT ||
            fb->orientation() == FrameBuffer::BOTTOMRIGHT)
        {
            Mat44f S;
            S.makeScale(Vec3f(-1, 1, 1));
            T *= S;
        }
    }

    return T;
}

void
IPImage::computeMatrices(const VideoDevice* controlDevice, const VideoDevice* outputDevice)
{
    InternalGLMatricesContext c;
    c.outputWidth   = controlDevice->internalWidth();
    c.outputHeight  = controlDevice->internalHeight();
    c.controlDevice = controlDevice;
    c.outputDevice  = outputDevice;
    c.parentMatrix  = Matrix();

    //
    //  Compute projection and viewport
    //

    const Margins margins    = controlDevice->margins();
    const float   hmargin    = margins.left + margins.right;
    const float   vmargin    = margins.bottom + margins.top;
    const float   gw         = c.outputWidth - hmargin;
    const float   gh         = c.outputHeight - vmargin;
    const float   viewAspect = gw / gh;

    Frustumf f;
    f.window(-viewAspect / 2.0f,  viewAspect / 2.0f, -0.5, 0.5, -1, 1, true);
    projectionMatrix = f.matrix();

    viewport = Box2f(Vec2f(margins.left, margins.bottom),
                     Vec2f(margins.left + gw, margins.bottom + gh));

    c.projectionMatrix = projectionMatrix;
    c.viewport         = viewport;

    //
    //  "Global" versions of these matrices do not treat images rendered to
    //  intermediate buffers differently, so they are truly global.
    //

    c.projectionMatrixGlobal = c.projectionMatrix;
    c.parentMatrixGlobal     = c.parentMatrix;

    parentMatrix = c.parentMatrix;
    
    //
    //  Record the "treeOrder" in imageNum field.  This is the "depth first"
    //  traversal order of each image in the tree.  This is like the render
    //  order, but more stable (in particular order of children will not
    //  change).
    //
    size_t imageNumCounter = 0;
    c.imageNumCounter = &imageNumCounter;

    computeMatricesRecursive(c);
}

void
IPImage::computeMatricesRecursive(const InternalGLMatricesContext& baseContext)
{
    //
    //  Recurse on siblings first
    //

    if (next) next->computeMatricesRecursive(baseContext);

    //
    //  OutputTexture doesn't need any transforms
    //

    //if (destination == OutputTexture) return;

    //
    //  Compute the matrix used when rendering this image.
    //
    //  NOTE: this is not the same as a global matrix. This matrix is
    //  relative to an ancestor that is an IntermediateBuffer not the final
    //  frame buffer.
    //

    Mat44f currentMatrix = baseContext.parentMatrix * transformMatrix;
    Mat44f currentMatrixGlobal = baseContext.parentMatrixGlobal * transformMatrix;

    // if (fb)
    // {
    //     cout << "currentMatrixGlobal = " << fb->identifier() << endl
    //          << currentMatrixGlobal << endl;
    // }

    if (!isNoBuffer()) 
    {
        float imageAspect = 1.0f;
        if (width!=0 && height!=0)
        {
            imageAspect = (float)width / height;
            if (isCropped && cropStartY!=cropEndY)
            {
                imageAspect *= (float)(cropEndX - cropStartX) * (float)height
                            / (float)((cropEndY - cropStartY) * width);
            }
        }

        Mat44f T;
        T.makeTranslation(Vec3f(-imageAspect / 2.0f, -0.5f, 0.f));

        projectionMatrix  = baseContext.projectionMatrix;
        viewport          = baseContext.viewport;
        placementMatrix   = T;
        orientationMatrix = computeOrientationMatrix(fb);
        imageMatrix       = currentMatrix;
        modelViewMatrix   = imageMatrix * placementMatrix;
        
        projectionMatrixGlobal = baseContext.projectionMatrixGlobal;
        modelViewMatrixGlobal  = currentMatrixGlobal * placementMatrix;
        
        parentMatrix = currentMatrixGlobal;
    }

    if (children)
    {
        //
        //  Make a copy of the incoming context which we'll modify before
        //  passing it on to any children
        //

        InternalGLMatricesContext context = baseContext;

        if (isIntermediateRender())
        {
            //
            //  The children are in a new (unrelated) contect to the
            //  one this image is rendered into. Reset the width and
            //  height and leave everything else (current coord
            //  system) in the ground state.
            //

            context.outputWidth  = width;
            context.outputHeight = height;
            context.parentMatrix = Matrix();

            //
            //  Compute projection and viewport
            //

            float aspect = displayAspect();

            Frustumf f;
            f.window(-aspect / 2.0f,  aspect / 2.0f, -0.5, 0.5, -1, 1, true);

            context.projectionMatrix = f.matrix();
            context.viewport = Box2f(Vec2f(0,0), Vec2f(width, height));

            //  
            //  leave global projection matrix alone, so it reflects the final
            //  projection to the screen (root), not an intermediate projection
            //  to a buffer.
            //
            context.parentMatrixGlobal = currentMatrixGlobal;
        }
        else 
        {
            if (isRootRender() || isExternalRender())
            {
                //
                //  In this case the root device dimensions need to
                //  become the context dimensions
                //
                //  In ExternalRenderType we need to use the margins
                //  

                Margins margins;

                if (context.controlDevice == device)
                {
                    margins              = isExternalRender() ? context.controlDevice->margins() : Margins();
                    context.outputWidth  = context.controlDevice->internalWidth();
                    context.outputHeight = context.controlDevice->internalHeight();
                }
                else if (context.outputDevice == device)
                {
                    margins              = isExternalRender() ? context.outputDevice->margins() : Margins();
                    context.outputWidth  = context.outputDevice->internalWidth();
                    context.outputHeight = context.outputDevice->internalHeight();
                }

                //
                //  Compute projection and viewport
                //

                const float hmargin    = margins.left + margins.right;
                const float vmargin    = margins.bottom + margins.top;
                const float gw         = context.outputWidth - hmargin;
                const float gh         = context.outputHeight - vmargin;
                const float viewAspect = gw / gh;

                Frustumf f;
                f.window(-viewAspect / 2.0f,  viewAspect / 2.0f, -0.5, 0.5, -1, 1, true);
                projectionMatrix = f.matrix();

                viewport = Box2f(Vec2f(margins.left, margins.bottom),
                                 Vec2f(margins.left + gw, margins.bottom + gh));
                
                context.projectionMatrix = projectionMatrix;
                context.viewport         = viewport;

                currentMatrix = Matrix();

                //  
                //  "Global" matrices react to the root case just like "non
                //  global" matrices.
                //

                context.projectionMatrixGlobal = projectionMatrix;
                currentMatrixGlobal = currentMatrix;
            }
            
            context.parentMatrix = currentMatrix;
            context.parentMatrixGlobal = currentMatrixGlobal;
        }

        //
        //  Recursively visit children
        //
        
        children->computeMatricesRecursive(context);
    }

    imageNum = (*baseContext.imageNumCounter)++;
}

void
IPImage::computeGraphIDs()
{
    if (m_graphID == "") 
    {
        size_t coordNum = 0;
        computeGraphIDRecursive(0, 0, coordNum);
    }
}

void
IPImage::computeGraphIDRecursive(const IPImage* parent,
                                 size_t index,
                                 size_t& coordNum) const
{
    //
    //  The graphID is unique for a given render pass in the IPImage tree
    //  topology. Since there can be multiple passes in the root IPImage
    //  tree, there can be multiple images with the same graphID in a
    //  single root IPImage tree. For example this IPImage tree:
    //
    //            +-A*--E
    //            |
    //      Root*-+    +-C
    //            |    |
    //            +-B*-+
    //                 |
    //                 +-D--F
    //
    //  where IPImage nodes with a '*' indicate an intermediate merge
    //  pass. There are three passes. One at Root, A, and B. The graphIDs
    //  in this case would be:
    //
    //      Root = (no graphID)
    //         A = 0
    //         B = 1
    //         C = 0
    //         D = 2
    //         E = 0
    //         F = 1
    //
    //  These values represent the topological ordering relative to
    //  the root image in the pass.
    //

    size_t i = 0;

    if (fb)
    {
    }
    else if (destination == IntermediateBuffer && 
             (renderType == MergeRenderType || renderType == RecordOnlyType))
    {
        //
        //  Children of this image are numbered because they all
        //  contribute as a single render pass
        //

        size_t newCoordNum = 0;

        for (IPImage* child = children; child; child = child->next, i++)
        {
            child->computeGraphIDRecursive(this, i, newCoordNum);
        }
    }
    else if (renderType == BlendRenderType ||
             renderType == GroupType)
    {
        //
        //  Children of this image are possibly independent passes so
        //  each one is a "root"
        //

        for (IPImage* child = children; child; child = child->next, i++)
        {
            size_t newCoordNum = 0;
            child->computeGraphIDRecursive(this, i, newCoordNum);
        }
    }
    else
    {
        for (IPImage* child = children; child; child = child->next, i++)
        {
            child->computeGraphIDRecursive(this, i, coordNum);
        }
    }

    m_coordID = coordNum;
    coordNum++;

    //
    //  Stash the m_coordID as a pre-baked name too
    //

    ostringstream gid;
    gid << m_coordID;
    m_graphID = gid.str();
}

const string&
IPImage::graphID() const
{
    return m_graphID;
}

void
IPImage::renderIDHashRecursive(ostream& o)
{
    o << renderID();
}

IPImage::HashValue
IPImage::renderIDHash() const
{
    if (renderIDNeedsCompute()) computeRenderIDs();
    return m_renderIDHash;
}

IPImage::HashValue
IPImage::fbHash() const
{
    if (renderIDNeedsCompute()) computeRenderIDs();
    return m_fbHash;
}

const string&
IPImage::renderID() const
{
    if (renderIDNeedsCompute()) computeRenderIDs();
    return m_renderID;
}

// hash up to second last command (not last command)
// this is used in ImageRenderer for paint FBO caching
// in case there is one or less command, no command hash will be included
// in renderIDWithPartialPaint
const string&
IPImage::renderIDWithPartialPaint() const
{
    if (renderIDNeedsCompute()) computeRenderIDs();
    return m_renderIDWithPartialPaint;
}

void
IPImage::computeRenderIDs() const
{
    //
    //  Compute and cache all hash values associated with the IPImage
    //

    ostringstream o;

    o << hashCount << "{";
    if (pixelAspect != 0.0) o << "p!" << pixelAspect;
    if (!stencilBox.isEmpty()) hashBox(o, stencilBox);
    o << "bm" << blendMode;
    
    if (mergeExpr) 
    { 
        o << ";(";
        mergeExpr->outputHash(o);
        o << ")";
    }
    
    if (shaderExpr) 
    {
        o << ";(";
        shaderExpr->outputHash(o);
        o << ")";
    }
    
    if (isCropped)
    {
        o << "crop{" << cropStartX << "_" << cropStartY << "_"
          << cropEndX << "_" << cropEndY << "}";
    }
    
    for (IPImage* child = children; child; child = child->next)
    {
        child->renderIDHashRecursive(o);
        hashMatrix(o, child->transformMatrix);
        if (child->textureMatrix != Mat33f())
        {
            hashMatrix33(o, child->textureMatrix);
        }
    }

    o << "}";

    if (fb)
    {
        o << "_" << fb->identifier();
    }

    m_renderIDWithPartialPaint = o.str();

    //
    //  XXX Hash commands into renderID, but set aside ID prior to last paint
    //  command for "partial paint" ID.  BUT, need parentMatrix in partial
    //  paint ID regardless of number of paint commands.
    //

    if (commands.size() == 1)
    {
        ostringstream o2(o.str(), ostringstream::ate);
        hashMatrix(o2, parentMatrix);
        m_renderIDWithPartialPaint = o2.str();
        commands[0]->hash(o);
    }
    else
    {
        for (size_t i = 0; i < commands.size(); i++)
        {
            commands[i]->hash(o);
            if (i == commands.size() - 2)
            {
                //  Start stream pointer at END of initializing string.
                //
                ostringstream o2(o.str(), ostringstream::ate);
                hashMatrix(o2, parentMatrix);
                m_renderIDWithPartialPaint = o2.str();
            }
        }
    }

    m_renderID = o.str();

    //
    //  The "index" used by the UI is the hash value and that was set to 32
    //  bits long ago. So here we're forcing the hash to be 32 bits to
    //  maintain compatibility. In the future it would be nice to have a 64
    //  bit hash as well. 
    //

    boost::hash<string> string_hash;
    size_t v = string_hash(m_renderID); 

    if (sizeof(size_t) == 8)
    {
        m_renderIDHash = (v & 0xffffffff) ^ ((v >> 32) & 0xffffffff);
    }
    else
    {
        m_renderIDHash = v;
    }
    
    boost::hash<string> string_hash2;
    size_t v2 = string_hash2(fb ? fb->identifier() : "");
    
    if (sizeof(size_t) == 8)
    {
        m_fbHash = (v2 & 0xffffffff) ^ ((v2 >> 32) & 0xffffffff);
    }
    else
    {
        m_fbHash = v2;
    }
    
}

namespace {
    
using namespace Shader;
    
void
assembleFrameBuffersFromExpr(const Expression* expr, bool isMerge = false)
{
    const ArgumentVector& boundArgs   = expr->arguments();
    const size_t          nargs       = boundArgs.size();
    
    // push fbs in order from leave to root
    for (size_t i = 0; i < nargs; ++i)
    {
        if (BoundExpression* be = dynamic_cast<BoundExpression*>(boundArgs[i]))
        {
            const Expression* expr2 = static_cast<const Expression*>(be->valuePointer());
            assembleFrameBuffersFromExpr(expr2);
        }
        if (BoundSampler* bsampler = dynamic_cast<BoundSampler*>(boundArgs[i]))
        {
            const FrameBuffer* fb = bsampler->value().fb;
            const IPImage* image = bsampler->value().image;
            if (fb && !image) // only get the aux ones, not the source ones
            {
                // Increment ref count of fb if it is ref counted
                if (fb->hasStaticRef())
                {                    
                    fb->staticRef();
                }

                if (isMerge)
                {
                    expr->image()->auxMergeFBs.push_back(fb);
                }
                else
                {
                    expr->image()->auxFBs.push_back(fb);
                }
            }
        }
    }
}
    
}
    
void
IPImage::assembleAuxFrameBuffers()
{
    for (IPImage* child = children; child; child = child->next)
    {
        child->assembleAuxFrameBuffers();
    }
    
    if (shaderExpr)
    {
        assembleFrameBuffersFromExpr(shaderExpr);
    }
    if (mergeExpr)
    {
        assembleFrameBuffersFromExpr(mergeExpr, true);
    }
}
 
void
IPImage::computeRenderIDRecursive()
{
    for (IPImage* child = children; child; child = child->next)
    {
        child->computeRenderIDRecursive();
    }
    
    computeRenderIDs();
}

static void
printTreeRecursive(ostream& o, const IPImage* image, int level)
{
    if (!image) return;
    for (size_t i = 0; i < level; i++) o << "  ";

    switch (image->destination)
    {
      case IPImage::IntermediateBuffer: o << "[*|"; break;
      case IPImage::TemporaryBuffer: o << "[+|"; break;
      case IPImage::OutputTexture: o << "[T|"; break;
      case IPImage::DataBuffer: o << "[D|"; break;
      case IPImage::LeftBuffer: o << "[L|"; break;
      case IPImage::RightBuffer: o << "[R|"; break;
      case IPImage::MainBuffer: o << "[M|"; break;
      default:
      case IPImage::CurrentFrameBuffer: o << "[ |"; break;
    }

    switch (image->renderType)
    {
      default:
      case IPImage::GroupType: o << "G|"; break;
      case IPImage::BlendRenderType: o << "B|"; break;
      case IPImage::MergeRenderType: o << "M|"; break;
      case IPImage::NoRenderType: o << "-|"; break;
      case IPImage::RecordOnlyType: o << "R|"; break;
      case IPImage::ExternalRenderType: o << "E|"; break;
    }

    switch (image->blendMode)
    {
      case IPImage::UnspecifiedBlendMode: o << "u|"; break;
      case IPImage::Replace: o << "r|"; break;
      case IPImage::Over: o << "o|"; break;
      case IPImage::Add: o << "a|"; break;
      case IPImage::Difference: o << "d|"; break;
      case IPImage::ReverseDifference: o << "i|"; break;
      case IPImage::Dissolve: o << "x|"; break;
    }

    switch (image->dataType)
    {
      case IPImage::NoInternalDataType: o << "?ty"; break;
      case IPImage::HalfDataType: o << "16f"; break;
      case IPImage::FloatDataType: o << "32f"; break;
      case IPImage::UInt8DataType: o << "08u"; break;
      case IPImage::UInt16DataType: o << "16u"; break;
      case IPImage::UInt10A2DataType: o << "10u"; break;
      case IPImage::UInt10A2RevDataType: o << "10u"; break;
    }
    if (image->shaderExpr) o << "|exp";
    else                   o << "|?ex";

    o << "|" << image->width << "x" << image->height;

    if (image->fb) o << "|" << image->fb->width() << "x" << image->fb->height();

    o << "|" << image->resourceUsage.fetches
      << "," << image->resourceUsage.buffers
      << "," << image->resourceUsage.coords;

    if (image->pixelAspect != 1.0) o << "|Pa";
    if (image->transformMatrix != IPImage::Matrix()) o << "|Tf";

    o << "]     " << image->graphID();

    if (image->fb) 
    {
        o << " " << image->fb->identifier() << " (" << image->fb << ")";
    }
    else 
    {
        o << " (" << image->node->name() 
          << ":" << image->node->protocol()
          << ")";
    }

    o << " " << image;

    if (level == 1)
    {
        if (image->device)
        {
            o << " {" << image->device->name() << "}";
        }
    } 

    for (IPImage::TagMap::const_iterator i = image->tagMap.begin(); i != image->tagMap.end(); ++i)
    {
        o << " " << i->first << ":" << i->second;
    }

    o << endl;

    printTreeRecursive(o, image->children, level + 1);
    printTreeRecursive(o, image->next, level);
}


static void 
printIDTreeRecursive(ostream& o, const IPImageID* image, int level)
{
    if (!image) return;
    for (size_t i = 0; i < level; i++) o << "  ";

    o << image->id << endl;

    printIDTreeRecursive(o, image->children, level + 1);
    printIDTreeRecursive(o, image->next, level);
}

void
IPImageID::append(IPImageID* img)
{
    IPImageID* i;
    for (i = this; i->next; i = i->next);
    i->next = img;
}

void 
IPImageID::appendChild(IPImageID* img)
{
    if (children) children->append(img);
    else children = img;
}

void 
IPImageID::appendChildren(const std::vector<IPImageID*>& images)
{
    for (size_t i = 0; i < images.size(); i++) appendChild(images[i]);
}

void
printTree(ostream& o, const IPImage* i)
{
    printTreeRecursive(o, i, 0);
}

void
printIDTree(ostream& o, const IPImageID* i)
{
    printIDTreeRecursive(o, i, 0);
}

void printTreeStdout(const IPImage* i) { printTree(cout, i); }

} // Rv
