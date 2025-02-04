//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <boost/algorithm/string.hpp>

namespace IPCore
{
    using namespace std;
    using namespace TwkFB;

    ImageSourceIPNode::ImageSourceIPNode(const string& name,
                                         const NodeDefinition* def, IPGraph* g,
                                         GroupIPNode* group,
                                         const std::string mediaRepName)
        : SourceIPNode(name, def, g, group, mediaRepName)
    {
        PropertyInfo* einfo = new PropertyInfo(
            PropertyInfo::Persistent | PropertyInfo::ExcludeFromProfile
            | PropertyInfo::RequiresGraphEdit);

        PropertyInfo* minfo = new PropertyInfo(
            PropertyInfo::Persistent | PropertyInfo::ExcludeFromProfile);

        //
        //  This function sets up the property values for an image
        //  source. The RV image source is subset of what RV can handle
        //  from an external file (basically just EXR).
        //
        //  Image sources can have multiple views each of which have
        //  multiple layers. However, all views must have the same
        //  layers. Image sources cannot have layers within layers,
        //  orphaned channels, empty views, missing views, or other
        //  weirdnesses that EXR can have.
        //

        m_mediaName =
            declareProperty<StringProperty>("media.name", name, minfo);
        m_mediaMovie =
            declareProperty<StringProperty>("media.movie", name, minfo);
        m_mediaLocation =
            declareProperty<StringProperty>("media.location", "image", minfo);

        //
        //  .layers = ["diffuse" "specular" "ambient"]
        //  .views  = ["left" "right"]
        //  .stereo   = ["left" "right"]
        //

        m_width = declareProperty<IntProperty>("image.width", 640, minfo);
        m_height = declareProperty<IntProperty>("image.height", 480, minfo);
        m_uncropWidth =
            declareProperty<IntProperty>("image.uncropWidth", 640, minfo);
        m_uncropHeight =
            declareProperty<IntProperty>("image.uncropHeight", 480, minfo);
        m_uncropX = declareProperty<IntProperty>("image.uncropX", 0, minfo);
        m_uncropY = declareProperty<IntProperty>("image.uncropY", 0, minfo);
        m_pixelAspect =
            declareProperty<FloatProperty>("image.pixelAspect", 1.0, minfo);
        m_fps = declareProperty<FloatProperty>("image.fps", 0.0, minfo);
        m_start = declareProperty<IntProperty>("image.start", 1, einfo);
        m_end = declareProperty<IntProperty>("image.end", 1, einfo);
        m_cutIn = declareProperty<IntProperty>(
            "cut.in", -numeric_limits<int>::max(), einfo);
        m_cutOut = declareProperty<IntProperty>(
            "cut.out", numeric_limits<int>::max(), einfo);
        m_inc = declareProperty<IntProperty>("image.inc", 1, minfo);
        m_encoding =
            declareProperty<StringProperty>("image.encoding", "None", minfo);
        m_channels =
            declareProperty<StringProperty>("image.channels", "RGBA", minfo);
        m_bits = declareProperty<IntProperty>("image.bitsPerChannel", 0, minfo);
        m_float = declareProperty<IntProperty>("image.float", 0, minfo);
        m_defaultView =
            declareProperty<StringProperty>("image.defaultView", "", minfo);
        m_defaultLayer =
            declareProperty<StringProperty>("image.defaultLayer", "", minfo);

        m_layers = createProperty<StringProperty>("image.layers");
        m_layers->setInfo(minfo);

        m_views = createProperty<StringProperty>("image.views");
        m_views->setInfo(minfo);

        updateViewLayerInfo();

        updateStereoViews(m_mediaInfo.views, m_eyeViews);
    }

    void ImageSourceIPNode::set(const string& mediaName,
                                const MovieInfo& mediaInfo)
    {
        m_mediaInfo = mediaInfo;

        m_mediaName->front() = mediaName;
        m_mediaMovie->front() = mediaName;

        m_width->front() = m_mediaInfo.width;
        m_height->front() = m_mediaInfo.height;
        m_uncropWidth->front() = m_mediaInfo.uncropWidth;
        m_uncropHeight->front() = m_mediaInfo.uncropHeight;
        m_uncropX->front() = m_mediaInfo.uncropX;
        m_uncropY->front() = m_mediaInfo.uncropY;
        m_pixelAspect->front() = m_mediaInfo.pixelAspect;
        m_fps->front() = m_mediaInfo.fps;
        m_start->front() = m_mediaInfo.start;
        m_end->front() = m_mediaInfo.end;
        m_inc->front() = m_mediaInfo.inc;
        m_channels->front() = string("RGBA").substr(0, m_mediaInfo.numChannels);

        if (m_mediaInfo.views.size())
        {
            m_defaultView->front() = m_mediaInfo.views.front();
        }

        if (m_mediaInfo.layers.size())
        {
            m_defaultLayer->front() = m_mediaInfo.layers.front();
        }

        m_layers->resize(m_mediaInfo.layers.size());

        m_views->resize(m_mediaInfo.views.size());

        if (m_mediaInfo.layers.size() > 0)
        {
            std::copy(m_mediaInfo.layers.begin(), m_mediaInfo.layers.end(),
                      m_layers->StringProperty::valueContainer().begin());
        }

        if (m_mediaInfo.views.size() > 0)
        {
            std::copy(m_mediaInfo.views.begin(), m_mediaInfo.views.end(),
                      m_views->StringProperty::valueContainer().begin());
        }

        switch (m_mediaInfo.dataType)
        {
        default:
        case FrameBuffer::UCHAR:
            m_bits->front() = 8;
            m_float->front() = 0;
            break;
        case FrameBuffer::USHORT:
            m_bits->front() = 16;
            m_float->front() = 0;
            break;
        case FrameBuffer::HALF:
            m_bits->front() = 16;
            m_float->front() = 1;
            break;
        case FrameBuffer::FLOAT:
            m_bits->front() = 32;
            m_float->front() = 1;
            break;
        }

        updateViewLayerInfo();

        updateStereoViews(m_mediaInfo.views, m_eyeViews);

        propagateImageStructureChange();
        propagateRangeChange();
        propagateMediaChange();
    }

    void ImageSourceIPNode::readCompleted(const std::string& typeName,
                                          unsigned int version)
    {
        SourceIPNode::readCompleted(typeName, version);

        updateInfoFromProps();
        propagateMediaChange();
    }

    void ImageSourceIPNode::writeCompleted()
    {
        updateInfoFromProps();
        updateViewLayerInfo();

        if (m_mediaMovie->front() != m_mediaName->front())
        {
            m_mediaMovie->front() = m_mediaName->front();
        }
    }

    void ImageSourceIPNode::updateInfoFromProps()
    {
        m_mediaInfo.width = m_width->front();
        m_mediaInfo.height = m_height->front();
        m_mediaInfo.uncropHeight = m_uncropHeight->front();
        m_mediaInfo.uncropWidth = m_uncropWidth->front();
        m_mediaInfo.uncropX = m_uncropX->front();
        m_mediaInfo.uncropY = m_uncropY->front();
        m_mediaInfo.pixelAspect = m_pixelAspect->front();
        m_mediaInfo.numChannels = m_channels->front().size();

        size_t b = m_bits->front();
        bool f = m_float->front();

        if (b == 8 && !f)
            m_mediaInfo.dataType = FrameBuffer::UCHAR;
        else if (b == 16 && !f)
            m_mediaInfo.dataType = FrameBuffer::USHORT;
        else if (b == 32 && f)
            m_mediaInfo.dataType = FrameBuffer::FLOAT;
        else if (b == 16 && f)
            m_mediaInfo.dataType = FrameBuffer::HALF;
        else
            m_mediaInfo.dataType = FrameBuffer::UCHAR;

        m_mediaInfo.orientation = FrameBuffer::NATURAL;

        m_mediaInfo.views.resize(m_views->size());
        m_mediaInfo.layers.resize(m_layers->size());

        std::copy(m_views->StringProperty::valueContainer().begin(),
                  m_views->StringProperty::valueContainer().end(),
                  m_mediaInfo.views.begin());

        std::copy(m_layers->StringProperty::valueContainer().begin(),
                  m_layers->StringProperty::valueContainer().end(),
                  m_mediaInfo.layers.begin());

        m_mediaInfo.defaultView = m_defaultView->front();
        m_mediaInfo.video = true;
        m_mediaInfo.start = m_start->front();
        m_mediaInfo.end = m_end->front();
        m_mediaInfo.inc = m_inc->front();
        m_mediaInfo.fps = m_fps->front();
        m_mediaInfo.quality = 1.0;
        m_mediaInfo.audio = false;
        m_mediaInfo.audioSampleRate = 0;
        m_mediaInfo.slowRandomAccess = false;
    }

    void ImageSourceIPNode::updateViewLayerInfo()
    {
        //
        //  Copy over the simple view/layer to the general ViewInfoVector
        //  and ChannelInfoVector. The image source has a simple 1 level
        //  structure.
        //

        const size_t nlayers = m_mediaInfo.layers.size();
        const size_t nviews = m_mediaInfo.views.size();

        m_mediaInfo.viewInfos.resize(nviews == 0 ? 1 : nviews);

        static const char* chnames[] = {"R", "G", "B", "A"};

        if (nviews)
        {
            for (size_t i = 0; i < nviews; i++)
            {
                FBInfo::ViewInfo& vinfo = m_mediaInfo.viewInfos[i];
                vinfo.name = m_mediaInfo.views[i];
                vinfo.layers.resize(nlayers == 0 ? 1 : nlayers);

                for (size_t q = 0; q < nlayers; q++)
                {
                    FBInfo::LayerInfo& linfo = vinfo.layers[q];

                    size_t nchannels = m_mediaInfo.numChannels;
                    linfo.name = m_mediaInfo.layers[q];

#if 0
                linfo.channels.resize(nchannels);

                for (size_t j = 0; j < nchannels; j++)
                {
                    ostringstream str;
                    str << linfo.name << "." << vinfo.name << ".";
                    if (j < 4) { str << chnames[j]; } else { str << "C" << j; }
                    linfo.channels[j].name = str.str();
                    linfo.channels[j].type = m_mediaInfo.dataType;

                    m_mediaInfo.channelInfos.resize(m_mediaInfo.channelInfos.size() + 1);
                    m_mediaInfo.channelInfos.back().name = str.str();
                    m_mediaInfo.channelInfos.back().type = m_mediaInfo.dataType;
                }
#endif
                }

                if (!nlayers)
                {
#if 0
                size_t nchannels = m_mediaInfo.numChannels;
                vinfo.otherChannels.resize(nchannels);

                //
                //  Can't do channels yet because they aren't individually
                //  selectable
                //

                for (size_t j = 0; j < nchannels; j++)
                {
                    ostringstream str;
                    if (vinfo.name != "") str << vinfo.name << ".";
                    if (j < 4) { str << chnames[j]; } else { str << "C" << j; }
                    vinfo.otherChannels[j].name = str.str();
                    vinfo.otherChannels[j].type = m_mediaInfo.dataType;
                    
                    m_mediaInfo.channelInfos.resize(m_mediaInfo.channelInfos.size() + 1);
                    m_mediaInfo.channelInfos.back().name = str.str();
                    m_mediaInfo.channelInfos.back().type = m_mediaInfo.dataType;
                }
#endif
                }
            }
        }
        else if (nlayers)
        {
            FBInfo::ViewInfo& vinfo = m_mediaInfo.viewInfos[0];

            vinfo.name =
                m_mediaInfo.views.empty() ? string("") : m_mediaInfo.views[0];
            vinfo.layers.resize(nlayers == 0 ? 1 : nlayers);

            for (size_t q = 0; q < nlayers; q++)
            {
                FBInfo::LayerInfo& linfo = vinfo.layers[q];

                size_t nchannels = m_mediaInfo.numChannels;
                linfo.name = m_mediaInfo.layers[q];

#if 0
            linfo.channels.resize(nchannels);
            for (size_t j = 0; j < nchannels; j++)
            {
                ostringstream str;
                str << linfo.name << ".";
                if (vinfo.name != "") str << vinfo.name << ".";
                if (j < 4) { str << chnames[j]; } else { str << "C" << j; }
                linfo.channels[j].name = str.str();
                linfo.channels[j].type = m_mediaInfo.dataType;
                
                m_mediaInfo.channelInfos.resize(m_mediaInfo.channelInfos.size() + 1);
                m_mediaInfo.channelInfos.back().name = str.str();
                m_mediaInfo.channelInfos.back().type = m_mediaInfo.dataType;
            }
#endif
            }
        }
        else
        {
#if 0
        FBInfo::ViewInfo& vinfo = m_mediaInfo.viewInfos[0];

        size_t nchannels = m_mediaInfo.numChannels;
        vinfo.otherChannels.resize(nchannels);
        
        for (size_t j = 0; j < nchannels; j++)
        {
            ostringstream str;
            if (vinfo.name != "") str << vinfo.name << ".";
            if (j < 4) { str << chnames[j]; } else { str << "C" << j; }
            vinfo.otherChannels[j].name = str.str();
            vinfo.otherChannels[j].type = m_mediaInfo.dataType;
            
            m_mediaInfo.channelInfos.resize(m_mediaInfo.channelInfos.size() + 1);
            m_mediaInfo.channelInfos.back().name = str.str();
            m_mediaInfo.channelInfos.back().type = m_mediaInfo.dataType;
        }
#endif
        }
    }

    ImageSourceIPNode::~ImageSourceIPNode() {}

    ImageSourceIPNode::Property*
    ImageSourceIPNode::makePixels(const string& comp, const string& prop)
    {
        Property* p = 0;

        int w = m_width->front();
        int h = m_height->front();
        int numChannels = m_channels->front().size();
        int bits = m_bits->front();
        int fp = m_float->front();

        if (bits == 8 && !fp)
        {
            ByteProperty* bp = createProperty<ByteProperty>(comp, prop);
            bp->resize(w * h * numChannels);
            p = bp;

            char* c = (char*)p->rawData();

#if 0
        cout << "valid addresses = " 
             << (void*)c
             << " to "
             << (void*)(c + p->sizeofElement() * p->size())
             << endl;
#endif
        }
        else if (bits == 16 && !fp)
        {
            ShortProperty* shp = createProperty<ShortProperty>(comp, prop);
            shp->resize(w * h * numChannels);
            p = shp;
        }
        else if (bits == 16 && fp)
        {
            HalfProperty* hp = createProperty<HalfProperty>(comp, prop);
            hp->resize(w * h * numChannels);
            p = hp;
        }
        else if (bits == 32 && fp)
        {
            FloatProperty* fp = createProperty<FloatProperty>(comp, prop);
            fp->resize(w * h * numChannels);
            p = fp;
        }

        p->clearToDefaultValue();
        return p;
    }

    void ImageSourceIPNode::propertyChanged(const Property* p)
    {
        m_propMap[p]++;
        IPNode::propertyChanged(p);

        if (p == m_width || p == m_height || p == m_uncropWidth
            || p == m_uncropHeight || p == m_uncropX || p == m_uncropY
            || p == m_pixelAspect)
        {
            propagateImageStructureChange();
        }

        if (p == m_fps || p == m_start || p == m_end || p == m_cutIn
            || p == m_cutOut || p == m_inc)
        {
            propagateRangeChange();
        }
    }

    ImageSourceIPNode::Property*
    ImageSourceIPNode::findCreatePixels(int frame, const string& view,
                                        const string& layer)
    {
        const StringProperty::container_type& viewContainer =
            m_views->valueContainer();
        const StringProperty::container_type& layerContainer =
            m_layers->valueContainer();

        if (view != "-")
        {
            if (std::find(viewContainer.begin(), viewContainer.end(), view)
                == viewContainer.end())
            {
                TWK_THROW_EXC_STREAM("Bad view name: " << view);
            }
        }

        if (layer != "-")
        {
            if (std::find(layerContainer.begin(), layerContainer.end(), layer)
                == layerContainer.end())
            {
                TWK_THROW_EXC_STREAM("Bad layer name: " << layer);
            }
        }

        StringVector views;
        StringVector layers;

        if (view != "-")
            views.push_back(view);
        if (layer != "-")
            layers.push_back(layer);
        LayerTree tree;

        pixelPropNames(views, layers, frame, tree, false);
        const string& name = tree.front().front();

        if (Property* p = find("image", name))
            return p;
        return makePixels("image", name);
    }

    IPNode::ImageRangeInfo ImageSourceIPNode::imageRangeInfo() const
    {
        int in = m_cutIn->front();
        int out = m_cutOut->front();
        return ImageRangeInfo(
            m_start->front(), m_end->front(), m_inc->front(), m_fps->front(),
            (in != -numeric_limits<int>::max()) ? in : m_start->front(),
            (out != numeric_limits<int>::max()) ? out : m_end->front());
    }

    IPNode::ImageStructureInfo
    ImageSourceIPNode::imageStructureInfo(const Context& context) const
    {
        return ImageStructureInfo(m_uncropWidth->front(),
                                  m_uncropHeight->front(),
                                  m_pixelAspect->front());
    }

    void ImageSourceIPNode::pixelPropNamesViewLayer(
        const StringVector& suffixes1, const StringVector& suffixes2,
        const string& separator1, const string& separator2, int frame,
        LayerTree& layerTree, bool existingOnly)
    {
        //
        //  This function is a bit funky because it has to find alternate
        //  "stand-in" frames for non-existing pixels (if existingOnly is
        //  true).
        //

        int fs = m_start->front();
        int fe = m_end->front();

        if (frame < fs)
            frame = fs;
        if (frame > fe)
            frame = fe;
        Component* c = component("image");
        if (!c)
            return;

        layerTree.resize(suffixes1.size());

        //
        //  It's likely that layer names will contain ".", but this is illegal
        //  char for property names, so swap in "*".
        //
        StringVector cleanSuffixes2 = suffixes2;
        for (int i = 0; i < cleanSuffixes2.size(); ++i)
        {
            boost::algorithm::replace_all(cleanSuffixes2[i], ".", "*");
        }

        for (size_t i = 0; i < suffixes1.size(); i++)
        {
            StringVector& propNames = layerTree[i];

            for (size_t q = 0; q < cleanSuffixes2.size(); q++)
            {
                if (!existingOnly)
                {
                    ostringstream str;
                    str << frame << separator1 << suffixes1[i] << separator2
                        << cleanSuffixes2[q];

                    propNames.push_back(str.str());
                }
                else
                {
                    bool found = false;

                    for (int f = frame; !found && f >= fs; f--)
                    {
                        ostringstream str;

                        str << f << separator1 << suffixes1[i] << separator2
                            << cleanSuffixes2[q];

                        const string& name = str.str();

                        if (c->find(name))
                        {
                            propNames.push_back(name);
                            found = true;
                        }
                    }

                    for (int f = frame + 1; !found && f <= fe; f++)
                    {
                        ostringstream str;

                        str << f << separator1 << suffixes1[i] << separator2
                            << cleanSuffixes2[q];

                        const string& name = str.str();

                        if (c->find(name))
                        {
                            propNames.push_back(name);
                            found = true;
                        }
                    }
                }
            }
        }
    }

    void ImageSourceIPNode::pixelPropNames(const StringVector& views,
                                           const StringVector& layers,
                                           int frame, LayerTree& propNames,
                                           bool existingOnly)
    {
        if (views.empty() && layers.empty())
        {
            StringVector suffixes1(1);
            StringVector suffixes2(1);
            suffixes1.front() = defaultView();
            suffixes2.front() = defaultLayer();

            pixelPropNamesViewLayer(suffixes1, suffixes2,
                                    suffixes1.front() == "" ? "" : ":",
                                    suffixes2.front() == "" ? "" : ";", frame,
                                    propNames, existingOnly);
        }
        else if (views.empty())
        {
            StringVector suffixes1(1);
            suffixes1.front() = defaultView();

            pixelPropNamesViewLayer(suffixes1, layers,
                                    suffixes1.front() == "" ? "" : ":", ";",
                                    frame, propNames, existingOnly);
        }
        else if (layers.empty())
        {
            StringVector suffixes2(1);
            suffixes2.front() = defaultLayer();

            pixelPropNamesViewLayer(views, suffixes2, ":",
                                    suffixes2.front() == "" ? "" : ";", frame,
                                    propNames, existingOnly);
        }
        else
        {
            pixelPropNamesViewLayer(views, layers, ":", ";", frame, propNames,
                                    existingOnly);
        }
    }

    IPImage* ImageSourceIPNode::evaluate(const Context& context)
    {
        IPImage* head = 0;
        LayerTree propTree;
        StringVector cviews;
        StringVector clayers;

        ImageComponent selection = selectComponentFromContext(context);
        if (context.stereo)
        {
            ImageComponent newSelection =
                stereoComponent(selection, context.eye);

            if (newSelection.isValid())
                selection = newSelection;
        }

        switch (selection.type)
        {
        case ViewComponent:
        {
            cviews.push_back(selection.name[0]);
            break;
        }
        case LayerComponent:
        {
            if (selection.name.size() > 1)
            {
                if (selection.name[0] != "")
                    cviews.push_back(selection.name[0]);
                clayers.push_back(selection.name[1]);
            }
            else
                clayers.push_back(selection.name[0]);
            break;
        }
        case NoComponent:
        case ChannelComponent:
            break;
        }

        pixelPropNames(cviews, clayers, context.frame, propTree, true);

        if (!propTree.empty())
        {
            const StringVector& propNames = propTree[0];
            IPImage* head = 0;
            int eye = 3;

            if (m_eyeViews && context.stereo)
            {
                const string& name = propNames.front();

                for (int v = 0; v < m_eyeViews->size(); v++)
                {
                    const string& viewName = (*m_eyeViews)[v];
                    string::size_type index = name.find(viewName);
                    if (name.size() - index == viewName.size())
                    {
                        eye = v + 1;
                        break;
                    }
                }
            }

            for (size_t i = 0; i < propNames.size(); i++)
            {
                const string& name = propNames[i];

                if (Property* pixels = find("image", name))
                {
                    //
                    //  Found some pixels
                    //

                    FrameBuffer::DataType d;

                    switch (pixels->layoutTrait())
                    {
                    case Property::FloatLayout:
                        d = FrameBuffer::FLOAT;
                        break;
                    case Property::ShortLayout:
                        d = FrameBuffer::USHORT;
                        break;
                    case Property::HalfLayout:
                        d = FrameBuffer::HALF;
                        break;
                    case Property::ByteLayout:
                        d = FrameBuffer::UCHAR;
                        break;
                    default:
                        break;
                    }

                    size_t numChannels = m_channels->front().size();
                    StringVector chans(numChannels);
                    for (size_t c = 0; c < numChannels; c++)
                        chans[c] = m_channels->front().at(c);

                    FrameBuffer* fb = new FrameBuffer(
                        m_width->front(), m_height->front(), numChannels, d,
                        (unsigned char*)pixels->rawData(), &chans);

                    IPImage* img =
                        new IPImage(this, IPImage::BlendRenderType, fb);

                    fb->attribute<int>("Eye") = eye;

                    fb->idstream()
                        << IPNode::name() << "/" << m_mediaName->front() << "/"
                        << pixels->name();

                    //
                    //  The ID at this point (without the unique integer from
                    //  propMap) is a handy substring we can use to flush any
                    //  FBs previously generated by this Source.  We should
                    //  flush them because we know they are out of date (we just
                    //  generated a replacement FB, and otherwise they may not
                    //  get deleted unless the user does something to regenerate
                    //  the frame cache.
                    //

                    FBCache::IDSet ids;
                    ids.insert(fb->identifier());
                    graph()->cache().lock();
                    graph()->cache().flushIDSetSubstr(ids);
                    graph()->cache().unlock();

                    fb->idstream() << "/" << m_propMap[pixels];

                    if (m_uncropHeight->front() != m_height->front()
                        || m_uncropWidth->front() != m_width->front())
                    {
                        fb->setUncrop(m_uncropWidth->front(),
                                      m_uncropHeight->front(),
                                      m_uncropX->front(), m_uncropY->front());
                    }
                    addUserAttributes(img->fb);

                    ostringstream sourceName;
                    sourceName << IPNode::name() << ".0/" << "0/"
                               << pixels->name();
                    fb->attribute<string>("RVSource") = sourceName.str();
                    head = img;

                    return head;
                }
            }
        }

        return 0;
    }

    IPImageID* ImageSourceIPNode::evaluateIdentifier(const Context& context)
    {
        LayerTree propTree;
        Component* c = component("image");
        if (!c)
            return 0;

        StringVector layers;
        StringVector views;

        ImageComponent selection = selectComponentFromContext(context);
        if (context.stereo)
        {
            ImageComponent newSelection =
                stereoComponent(selection, context.eye);

            if (newSelection.isValid())
                selection = newSelection;
        }

        switch (selection.type)
        {
        case ViewComponent:
        {
            views.push_back(selection.name[0]);
            break;
        }
        case LayerComponent:
        {
            if (selection.name.size() > 1)
            {
                if (selection.name[0] != "")
                    views.push_back(selection.name[0]);
                layers.push_back(selection.name[1]);
            }
            else
                layers.push_back(selection.name[0]);
            break;
        }
        case NoComponent:
        case ChannelComponent:
            break;
        }

        pixelPropNames(views, layers, context.frame, propTree, true);

        IPImageID* root = 0;

        if (!propTree.empty())
        {
            const StringVector& propNames = propTree[0];

            for (size_t i = 0; i < propNames.size(); i++)
            {
                const string& name = propNames[i];

                if (Property* pixels = c->find(name))
                {
                    ostringstream str;
                    str << IPNode::name() << "/" << m_mediaName->front() << "/"
                        << pixels->name() << "/" << m_propMap[pixels]
                        << idFromAttributes();

                    IPImageID* id = new IPImageID(str.str());
                    root = id;
                    break;
                }
            }
        }

        return root;
    }

    size_t ImageSourceIPNode::numMedia() const { return 1; }

    size_t ImageSourceIPNode::mediaIndex(const std::string& name) const
    {
        return name == m_mediaName->front() ? 0 : size_t(-1);
    }

    const SourceIPNode::MovieInfo&
    ImageSourceIPNode::mediaMovieInfo(size_t index) const
    {
        return m_mediaInfo;
    }

    const string& ImageSourceIPNode::mediaName(size_t index) const
    {
        return m_mediaName->front();
    }

    void ImageSourceIPNode::insertPixels(const string& view,
                                         const string& layer, int frame, int x,
                                         int y, int w, int h,
                                         const void* pixels, size_t size)
    {
        Property* p = findCreatePixels(frame, view, layer);
        unsigned char* d = (unsigned char*)p->rawData();
        const size_t iw = m_mediaInfo.width;
        const size_t ih = m_mediaInfo.height;
        const size_t ch = m_mediaInfo.numChannels;
        const size_t esize = p->sizeofElement();
        const size_t pixelSize = esize * ch;
        const size_t iRowSize = iw * pixelSize;
        const size_t tRowSize = w * pixelSize;
        const size_t expectedSize = w * h * ch * esize;

        if (layer.find('/') != string::npos || layer.find('@') != string::npos
            || layer.find('#') != string::npos
            || layer.find('*') != string::npos
            || layer.find(':') != string::npos
            || layer.find(';') != string::npos)
        {
            cerr << "ERROR: Characters /, @, #, *, :, ; are illegal in layer "
                    "names"
                 << endl;
            TWK_THROW_STREAM(
                LayerOutOfBoundsExc,
                "Characters /, @, #, *, :, ; are illegal in layer names");
        }

        if (expectedSize != size)
        {
            TWK_THROW_STREAM(PixelBlockSizeMismatchExc,
                             "Received pixels of size " << size << ". Expected "
                                                        << expectedSize);
        }

        for (size_t i = y; i < (y + h); i++)
        {
            unsigned char* dst = d + iRowSize * i + x * pixelSize;
            unsigned char* src = (unsigned char*)pixels + tRowSize * (i - y);
            assert((dst + tRowSize - 1) < (d + iw * ih * ch * esize));
            memcpy(dst, src, tRowSize);
        }

        //
        //  propertyChanged() will end up cause the image identifier to
        //  change. This will eventually cause the graph to
        //  re-evaluate. However, if the graph has evaluated since the
        //  last time this function was called, there may be a key in the
        //  cache with the *current* identifier. The cache policy
        //  currently does not have a half-life on cache objects so we
        //  need to tell it to flush the old identifier. Otherwise we get
        //  a "leak" in that the cache simple never frees older versions
        //  of the image.
        //
        //
        //  Look above at evaluateIdentifier() to see which portions of
        //  the Context will actually be used. Flushing in the input
        //  direction is a bit strange: its easy to concoct a situation
        //  that makes this impossible. E.g, there's an EDL down stream
        //  and it maps the same frame to multiple times.
        //

        Context context = graph()->contextForFrame(frame);
        context.stereo = false; // (we'll explicitly set the view)

        if (view != "" && view != "-")
        {
            context.component = ImageComponent(ViewComponent, view);
        }
        else if (layer != "" && layer != "-")
        {
            context.component = ImageComponent(LayerComponent, layer);
        }

        propagateFlushToInputs(
            context); // flush any old cached versions of the image

        //
        //  Ok, now notify that the property changed. The identifier will
        //  also change by the time this returns.
        //

        propertyChanged(p);

        propagateMediaChange();
    }

    string ImageSourceIPNode::defaultView() const
    {
        if (m_defaultView)
            return m_defaultView->front();
        else
            return "";
    }

    string ImageSourceIPNode::defaultLayer() const
    {
        if (m_defaultLayer)
            return m_defaultLayer->front();
        else
            return "";
    }

    void ImageSourceIPNode::flushAllCaches(const FlushContext& context)
    {
        //
        //  We almost certainly got here from insertPixels() above. But go
        //  through the motions anyway.
        //

        try
        {
            IPImageID* id = evaluateIdentifier(context);
            IPNode::FlushFBs F(context);
            foreach_ip(id, F);
            delete id;
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: while evaluateIdentifier in "
                    "ImageSourceIPNode::flushAllCaches"
                 << endl
                 << "CAUGHT: " << exc.what() << endl;
        }
    }

} // namespace IPCore
