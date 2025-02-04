//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/TextureOutputGroupIPNode.h>
#include <TwkMath/Mat44.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/IPGraph.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    TextureOutputGroupIPNode::TextureOutputGroupIPNode(
        const std::string& name, const NodeDefinition* def, IPGraph* graph,
        GroupIPNode* group)
        : DisplayGroupIPNode(name, def, graph, group)
    {
        setWritable(false);
        setMetaSearchable(false);
        setUnconstrainedInputs(true); // can connect to group members

        m_active = declareProperty<IntProperty>("output.active", 0);
        m_ndcCoords = declareProperty<IntProperty>("output.ndcCoordinates", 1);
        m_width = declareProperty<IntProperty>("output.width", 0);
        m_height = declareProperty<IntProperty>("output.height", 0);
        m_dataType =
            declareProperty<StringProperty>("output.dataType", "uint8");
        m_pixelAspect =
            declareProperty<FloatProperty>("output.pixelAspect", 1.0f);
        m_tag = declareProperty<StringProperty>("output.tag", "");
        m_outFrame = declareProperty<IntProperty>("output.frame", 1);
        m_flip = declareProperty<IntProperty>("output.flip", 0);
        m_flop = declareProperty<IntProperty>("output.flop", 0);
    }

    TextureOutputGroupIPNode::~TextureOutputGroupIPNode()
    {
        //
        //  Dereference any frame buffer from this node in the per-node cache.
        //
        graph()->cache().flushPerNodeCache(this);
    }

    bool TextureOutputGroupIPNode::isActive() const
    {
        return propertyValue(m_active, 1);
    }

    void TextureOutputGroupIPNode::setFlip(bool b)
    {
        setProperty(m_flip, b ? 1 : 0);
    }

    void TextureOutputGroupIPNode::setFlop(bool b)
    {
        setProperty(m_flop, b ? 1 : 0);
    }

    void TextureOutputGroupIPNode::setTag(const string& tag)
    {
        setProperty(m_tag, tag);
    }

    void TextureOutputGroupIPNode::setFrame(int f)
    {
        bool pushItem = (isActive() && f != frame());

        setProperty(m_outFrame, f);

        if (pushItem)
        {
            graph()->cache().pushCachableOutputItem(name());
            graph()->redispatchCachingThread();
        }
    }

    void TextureOutputGroupIPNode::setActive(bool b)
    {
        bool pushItem = (b && !isActive());

        setProperty(m_active, b ? 1 : 0);

        if (pushItem)
        {
            graph()->cache().pushCachableOutputItem(name());
            graph()->redispatchCachingThread();
        }
    }

    void TextureOutputGroupIPNode::setGeometry(int w, int h, const string& dt)
    {
        setProperty(m_width, w);
        setProperty(m_height, h);
        setProperty(m_dataType, dt);
    }

    void TextureOutputGroupIPNode::initContext(Context& context) const
    {
        int w = propertyValue(m_width, 1280);
        int h = propertyValue(m_height, 720);

        //
        //  Don't allow a 0 area texture
        //

        if (!w)
            w = h;
        else if (!h)
            h = w;
        if (!w && !h)
        {
            w = 128;
            h = 128;
        }

        context.viewWidth = w;
        context.viewHeight = h;
        context.deviceWidth = context.viewWidth;
        context.deviceHeight = context.viewHeight;
        context.viewXOrigin = 0;
        context.viewYOrigin = 0;
        context.frame = propertyValue(m_outFrame, 0);
        context.cacheNode = this;
    }

    IPImage* TextureOutputGroupIPNode::evaluate(const Context& context)
    {
        //
        //  If the tag has not been set, or has been re-set to "", do nothing
        //
        if (tag() == "" || !isActive())
            return 0;

        //
        //  Texture nodes use the per-node cache, so in general we have to be
        //  careful to not allow the FBs used by these nodes to be referenced by
        //  the FrameCache.  There are several cases:
        //
        //  (1) If this is not a caching thread,
        //
        //  (1a)    and there is nothing in the perNode cache, return nothing
        //          (display nothing)
        //
        //  (1b)    and there is something in the perNode cache, proceed (the
        //          CacheIPNode may swap in out-of-date images).
        //
        //  (2) If this _is_ a caching thread,
        //
        //  (2a)    and the context.cacheNode is set to this node (because the
        //          caching threads is explicitly caching this node) evaluate as
        //          usual.
        //
        //  (2b)    and the context.cacheNode is _not_ set (because the cache
        //          thread is just caching frames), return nothing

        if (context.thread != CacheEvalThread
            && !graph()->cache().perNodeCacheContents(this))
            return 0;
        else if (context.thread == CacheEvalThread && context.cacheNode != this)
            return 0;

        Context c = context;
        initContext(c);

        IPImage* root = 0;

        IPImage::InternalDataType dataType = IPImage::UInt8DataType;
        IPImage::SamplerType stype = IPImage::Rect2DSampler;
        const std::string& tname = m_dataType->front();

        if (tname == "uint16")
            dataType = IPImage::UInt16DataType;
        else if (tname == "uint10rev")
            dataType = IPImage::UInt10A2RevDataType;
        else if (tname == "uint10")
            dataType = IPImage::UInt10A2DataType;
        else if (tname == "float")
            dataType = IPImage::FloatDataType;
        else if (tname == "half")
            dataType = IPImage::HalfDataType;

        if (m_ndcCoords->front() != 0)
            stype = IPImage::NDC2DSampler;

        const float yscale = propertyValue(m_flip, 0) == 0 ? 1.0f : -1.0f;
        const float xscale = propertyValue(m_flop, 0) == 0 ? 1.0f : -1.0f;

        try
        {
            float aspect = float(c.viewWidth) / float(c.viewHeight);
            Mat44f S;
            S.makeScale(Vec3f(xscale, yscale, 1));

            IPImage* image = m_root->evaluate(c);

            root = new IPImage(this, IPImage::BlendRenderType, c.viewWidth,
                               c.viewHeight, m_pixelAspect->front(),
                               IPImage::OutputTexture, dataType, stype);

            image->fitToAspect(aspect);
            image->transformMatrix = S * image->transformMatrix;

            root->tagMap[IPImage::textureIDTagName()] = m_tag->front();
            root->appendChild(image);
        }
        catch (...)
        {
            throw;
        }

        return root;
    }

    IPImageID*
    TextureOutputGroupIPNode::evaluateIdentifier(const Context& context)
    {
        //
        //  If the tag has not been set, or has been re-set to "", do nothing
        //
        if (tag() == "" || !isActive())
            return 0;

        //
        //  Texture nodes use the per-node cache, so in general we have to be
        //  careful to not allow the FBs used by these nodes to be referenced by
        //  the FrameCache.  There are several cases:
        //
        //  (1) If this is not a caching thread,
        //
        //  (1a)    and there is nothing in the perNode cache, return nothing
        //          (display nothing)
        //
        //  (1b)    and there is something in the perNode cache, proceed (the
        //          CacheIPNode may swap in out-of-date images).
        //
        //  (2) If this _is_ a caching thread,
        //
        //  (2a)    and the context.cacheNode is set to this node (because the
        //          caching threads is explicitly caching this node) evaluate as
        //          usual.
        //
        //  (2b)    and the context.cacheNode is _not_ set (because the cache
        //          thread is just caching frames), return nothing

        if (context.thread != CacheEvalThread
            && !graph()->cache().perNodeCacheContents(this))
            return 0;
        else if (context.thread == CacheEvalThread && context.cacheNode != this)
            return 0;

        Context c = context;
        initContext(c);

        const float yscale = propertyValue(m_flip, 0) == 0 ? 1.0f : -1.0f;
        const float xscale = propertyValue(m_flop, 0) == 0 ? 1.0f : -1.0f;
        const float aspect = float(c.viewWidth) / float(c.viewHeight);

        Mat44f S;
        S.makeScale(Vec3f(xscale, yscale, 1));

        return GroupIPNode::evaluateIdentifier(c);
    }

    bool TextureOutputGroupIPNode::cached() const
    {
        return (graph()->cache().perNodeCacheContents(this) != 0);
    }

} // namespace IPCore
