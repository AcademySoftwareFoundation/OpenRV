//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/SourceIPNode.h>
#include <IPCore/IPGraph.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Iostream.h>

extern const char* SourceRGBA_glsl;

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    SourceIPNode::SourceIPNode(const std::string& name,
                               const NodeDefinition* def, IPGraph* g,
                               GroupIPNode* group,
                               const std::string mediaRepName, bool mediaActive)
        : IPNode(name, def, g, group)
    {
        setMaxInputs(0);
        m_imageComponent =
            createProperty<StringProperty>("request.imageComponent");
        m_eyeViews = createProperty<StringProperty>("request.stereoViews");
        m_updateAction = "new";

        m_mediaRepName =
            declareProperty<StringProperty>("media.repName", mediaRepName);
        m_mediaActive =
            declareProperty<IntProperty>("media.active", mediaActive ? 1 : 0);
    }

    SourceIPNode::~SourceIPNode() {}

    IPImage* SourceIPNode::evaluate(const Context& context)
    {
        IPImage* img = IPNode::evaluate(context);
        return img;
    }

    void SourceIPNode::prepareForWrite()
    {
        //
        //  Add information about the movie range, etc, just in case the
        //  movie file becomes inaccessible in when the RV file is read
        //  in. This makes it possible to read a partial RV file. These
        //  are all put in a "proxy" component.
        //

        ImageRangeInfo info = imageRangeInfo();
        ImageStructureInfo sinfo =
            imageStructureInfo(graph()->contextForFrame(info.start));

        declareProperty<Vec2iProperty>("proxy.range",
                                       Vec2i(info.start, info.end + 1));
        declareProperty<IntProperty>("proxy.inc", info.inc);
        declareProperty<Vec2iProperty>("proxy.size",
                                       Vec2i(sinfo.width, sinfo.height));
    }

    void SourceIPNode::writeCompleted()
    {
        //
        //  Remove the proxy properties
        //

        removeProperty(find("proxy.inc"));
        removeProperty(find("proxy.range"));
        removeProperty(find("proxy.size"));
    }

    void SourceIPNode::readCompleted(const std::string& typeName,
                                     unsigned int version)
    {
        m_updateAction = "session";
    }

    void SourceIPNode::addUserAttributes(FrameBuffer* fb)
    {
        if (Component* comp = component("attributes"))
        {
            const Component::Container& props = comp->properties();

            for (size_t i = 0; i < props.size(); i++)
            {
                Property* p = props[i];

                if (StringProperty* sp = dynamic_cast<StringProperty*>(p))
                {
                    if (sp->size() == 1)
                    {
                        fb->attribute<string>(sp->name()) = sp->front();
                    }
                }

                if (FloatProperty* fp = dynamic_cast<FloatProperty*>(p))
                {
                    if (fp->size() == 1)
                    {
                        fb->attribute<float>(p->name()) = (*fp)[0];
                    }
                    else if (fp->size() == 2)
                    {
                        fb->attribute<Vec2f>(p->name()) =
                            Vec2f((*fp)[0], (*fp)[1]);
                    }
                    else if (fp->size() == 3)
                    {
                        fb->attribute<Vec3f>(p->name()) =
                            Vec3f((*fp)[0], (*fp)[1], (*fp)[2]);
                    }
                    else if (fp->size() == 4)
                    {
                        fb->attribute<Vec4f>(p->name()) =
                            Vec4f((*fp)[0], (*fp)[1], (*fp)[2], (*fp)[3]);
                    }
                }

                if (IntProperty* ip = dynamic_cast<IntProperty*>(p))
                {
                    if (ip->size() == 1)
                    {
                        fb->attribute<int>(p->name()) = (*ip)[0];
                    }
                    else if (ip->size() == 2)
                    {
                        fb->attribute<Vec2i>(p->name()) =
                            Vec2f((*ip)[0], (*ip)[1]);
                    }
                    else if (ip->size() == 3)
                    {
                        fb->attribute<Vec3i>(p->name()) =
                            Vec3f((*ip)[0], (*ip)[1], (*ip)[2]);
                    }
                    else if (ip->size() == 4)
                    {
                        fb->attribute<Vec4i>(p->name()) =
                            Vec4f((*ip)[0], (*ip)[1], (*ip)[2], (*ip)[3]);
                    }
                }
            }
        }
    }

    string SourceIPNode::idFromAttributes()
    {
        ostringstream str;

        if (Component* comp = component("attributes"))
        {
            const Component::Container& props = comp->properties();

            for (size_t i = 0; i < props.size(); i++)
            {
                Property* p = props[i];

                if (StringProperty* sp = dynamic_cast<StringProperty*>(p))
                {
                    if (sp->size() == 1)
                    {
                        str << p->name() << "@" << sp->front() << "@";
                    }
                }

                if (FloatProperty* fp = dynamic_cast<FloatProperty*>(p))
                {
                    if (fp->size() == 1)
                    {
                        str << p->name() << "@" << fp->front() << "@";
                    }
                    else if (fp->size() == 2)
                    {
                        str << p->name() << "@" << (*fp)[0] << "@" << (*fp)[1]
                            << "@";
                    }
                    else if (fp->size() == 3)
                    {
                        str << p->name() << "@" << (*fp)[0] << "@" << (*fp)[1]
                            << "@" << (*fp)[2] << "@";
                    }
                    else if (fp->size() == 4)
                    {
                        str << p->name() << "@" << (*fp)[0] << "@" << (*fp)[1]
                            << "@" << (*fp)[2] << "@" << (*fp)[3] << "@";
                    }
                }

                if (IntProperty* ip = dynamic_cast<IntProperty*>(p))
                {
                    if (ip->size() == 1)
                    {
                        str << p->name() << "@" << (*ip)[0] << "@";
                    }
                    else if (ip->size() == 2)
                    {
                        str << p->name() << "@" << (*ip)[0] << "@" << (*ip)[1]
                            << "@";
                    }
                    else if (ip->size() == 3)
                    {
                        str << p->name() << "@" << (*ip)[0] << "@" << (*ip)[1]
                            << "@" << (*ip)[2] << "@";
                    }
                    else if (ip->size() == 4)
                    {
                        str << p->name() << "@" << (*ip)[0] << "@" << (*ip)[1]
                            << "@" << (*ip)[2] << "@" << (*ip)[2] << "@";
                    }
                }
            }
        }

        return str.str();
    }

    IPNode::ImageComponent
    SourceIPNode::selectComponentFromContext(const Context& context) const
    {
        //
        //  If the user has indicated a specific component start with that
        //

        if (!m_imageComponent->empty())
        {
            const StringProperty::container_type& data =
                m_imageComponent->valueContainer();
            const string type = data[0];
            const size_t size = data.size();

            if (type == "view" && size == 2)
            {
                return ImageComponent(ViewComponent, data[1]);
            }
            else if (type == "layer")
            {
                if (size == 2)
                {
                    return ImageComponent(LayerComponent, "", data[1]);
                }
                else if (size == 3)
                {
                    return ImageComponent(LayerComponent, data[1], data[2]);
                }
            }
            else if (type == "channel")
            {
                if (size == 2)
                {
                    return ImageComponent(ChannelComponent, "", "", data[1]);
                }
                else if (size == 3)
                {
                    return ImageComponent(ChannelComponent, "", data[1],
                                          data[2]);
                }
                else if (size == 4)
                {
                    return ImageComponent(ChannelComponent, data[1], data[2],
                                          data[3]);
                }
            }
        }

        return context.component;
    }

    IPNode::ImageComponent
    SourceIPNode::stereoComponent(const ImageComponent& component,
                                  size_t eye) const
    {
        size_t neyes = m_eyeViews->size();

        if (component.type == NoComponent && neyes >= 2 && eye < neyes)
        {
            return ImageComponent(ViewComponent, eyeView(eye));
        }
        else if (component.name.size() >= 1 && neyes >= 2 && eye < neyes)
        {
            ImageComponent c = component;
            c.name[0] = eyeView(eye);
            return c;
        }

        return ImageComponent();
    }

    string SourceIPNode::eyeView(size_t eye) const
    {
        if (m_eyeViews->size() >= eye)
            return (*m_eyeViews)[eye];
        else
            return "";
    }

    void SourceIPNode::mediaInfo(const Context& context,
                                 MediaInfoVector& infos) const
    {
        for (size_t i = 0; i < numMedia(); i++)
        {
            infos.push_back(MediaInfo(mediaName(i), mediaMovieInfo(i),
                                      const_cast<SourceIPNode*>(this)));
        }
    }

    const string& SourceIPNode::mediaRepName() const
    {
        return m_mediaRepName->front();
    }

    bool SourceIPNode::isMediaActive() const
    {
        return m_mediaActive->front() != 0;
    }

    void SourceIPNode::setMediaActive(bool state)
    {
        if (state != isMediaActive())
        {
            propertyWillChange(m_mediaActive);
            m_mediaActive->front() = state ? 1 : 0;
            propertyChanged(m_mediaActive);
        }
    }

    void SourceIPNode::updateStereoViews(const vector<string>& views,
                                         StringProperty* viewsP) const
    {
        //
        //  If we have left and right views, use them, if we have 2 or more
        //  views assume first is left, last is right.  If we have just one
        //  incoming view, assume that is left (right may be coming from another
        //  layer in the source).
        //
        //  This happens when you have EG left and right views in separate exr
        //  files each with multiView attr, but neither with "view" channels.
        //  See separateMultiView*.exr in tweaklib.
        //

        if (views.size() > 0 && viewsP)
        {
            if (std::find(views.begin(), views.end(), "left") != views.end()
                && std::find(views.begin(), views.end(), "right")
                       != views.end())
            {
                viewsP->resize(2);
                viewsP->front() = "left";
                viewsP->back() = "right";
            }
            else if (views.size() > 1)
            {
                viewsP->resize(2);
                viewsP->front() = views.front();
                viewsP->back() = views.back();
            }
            else
            {
                viewsP->resize(1);
                viewsP->front() = views.front();
            }
        }
    }

} // namespace IPCore
