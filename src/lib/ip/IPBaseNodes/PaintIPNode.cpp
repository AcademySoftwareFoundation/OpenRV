//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/PaintIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <TwkGLText/TwkGLText.h>
#include <stl_ext/string_algo.h>
#include <cstdlib>
#include <IPCore/ShaderCommon.h>

namespace IPCore
{
    using namespace std;
    using namespace stl_ext;
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkPaint;
    using namespace TwkGLText;

    PaintIPNode::PaintIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
    {
        //
        //  This is version 2 because we changed the coordinate system to
        //  one that's consistant regardless of the image orientation.
        //

        setMaxInputs(1);

        declareProperty<IntProperty>("paint.nextId", 0);
        declareProperty<IntProperty>("paint.nextAnnotationId", 0);
        declareProperty<IntProperty>("paint.show", 1);
        declareProperty<StringProperty>("paint.exclude");
        declareProperty<StringProperty>("paint.include");

        m_tag = createComponent("tag");
    }

    PaintIPNode::~PaintIPNode() {}

    void PaintIPNode::compilePenComponent(Component* c)
    {
        LocalPolyLine& p = m_penStrokes[c];
        const FloatProperty* widthP = c->property<FloatProperty>("width");
        const Vec2fProperty* pointsP = c->property<Vec2fProperty>("points");
        const Vec4fProperty* colorP = c->property<Vec4fProperty>("color");
        const StringProperty* brushP = c->property<StringProperty>("brush");
        const IntProperty* joinP = c->property<IntProperty>("join");
        const IntProperty* capP = c->property<IntProperty>("cap");
        const IntProperty* debugP = c->property<IntProperty>("debug");
        const IntProperty* modeP = c->property<IntProperty>("mode");
        const IntProperty* splatP = c->property<IntProperty>("splat");
        const IntProperty* versionP = c->property<IntProperty>("version");
        const IntProperty* eyeP = c->property<IntProperty>("eye");

        const float width = widthP && widthP->size() ? widthP->front() : 0.01f;
        const Vec4f color = colorP && colorP->size()
                                ? colorP->front()
                                : Vec4f(1.0, 1.0, 1.0, 1.0);
        const string brush =
            brushP && brushP->size() ? brushP->front() : string("circle");
        const unsigned int join =
            joinP && joinP->size() ? joinP->front() : Path::RoundJoin;
        const unsigned int cap =
            capP && capP->size() ? capP->front() : Path::SquareCap;
        const int debug = debugP && debugP->size() ? debugP->front() : 0;
        const unsigned int mode =
            modeP && modeP->size() ? modeP->front() : Paint::PolyLine::OverMode;
        const unsigned int splat =
            splatP && splatP->size() ? splatP->front() : 0;
        const unsigned int version = versionP && versionP->size()
                                         ? versionP->front()
                                         : protocolVersion();
        const int eye = eyeP && eyeP->size() ? eyeP->front() : 2;

        p.width = width;
        p.color = color;
        p.brush = brush;
        p.debug = debug;
        p.join = (Path::JoinStyle)join;
        p.cap = (Path::CapStyle)cap;
        p.mode = (Paint::PolyLine::Mode)mode;
        p.splat = splat ? true : false;
        p.smoothingWidth = p.splat ? 0.25 : 1.0;
        p.version = version;
        p.eye = eye;

        if (widthP && pointsP && widthP->size() == pointsP->size()
            && widthP->size() > 1)
        {
            p.widths = (const float*)widthP->rawData();
        }

        if (pointsP && pointsP->size())
        {
            p.points = (const Vec2f*)pointsP->rawData();
            p.npoints = pointsP->size();
        }
        else
        {
            p.points = 0;
            p.npoints = 0;
        }

        p.built = false;
    }

    void PaintIPNode::compileTextComponent(Component* c)
    {
        LocalText& p = m_texts[c];
        const FloatProperty* sizeP = c->property<FloatProperty>("size");
        const FloatProperty* scaleP = c->property<FloatProperty>("scale");
        const FloatProperty* rotP = c->property<FloatProperty>("rotation");
        const FloatProperty* spaceP = c->property<FloatProperty>("spacing");
        const Vec2fProperty* posP = c->property<Vec2fProperty>("position");
        const Vec4fProperty* colorP = c->property<Vec4fProperty>("color");
        const StringProperty* fontP = c->property<StringProperty>("font");
        const StringProperty* textP = c->property<StringProperty>("text");
        const StringProperty* originP = c->property<StringProperty>("origin");
        const IntProperty* debugP = c->property<IntProperty>("debug");
        const IntProperty* eyeP = c->property<IntProperty>("eye");

        const float size = sizeP && sizeP->size() ? sizeP->front() : 0.01f;
        const float scale = scaleP && scaleP->size() ? scaleP->front() : 1.0f;
        const float rot = rotP && rotP->size() ? rotP->front() : 0.0f;
        const float space = spaceP && spaceP->size() ? spaceP->front() : 1.0f;
        const Vec4f color = colorP && colorP->size()
                                ? colorP->front()
                                : Vec4f(1.0, 1.0, 1.0, 1.0);
        const Vec2f pos =
            posP && posP->size() ? posP->front() : Vec2f(0.0, 0.0);
        const string font =
            fontP && fontP->size() ? fontP->front() : string("");
        const string origin =
            originP && originP->size() ? originP->front() : string("");
        const string text =
            textP && textP->size() ? textP->front() : string("");
        const int debug = debugP && debugP->size() ? debugP->front() : 0;
        const int eye = eyeP && eyeP->size() ? eyeP->front() : 2;

        p.ptsize = size * 100.0 * 100.0;
        p.scale = 1.0 / 80.0 / 10.0 * scale;
        p.pos = pos;
        p.spacing = space;
        p.color = color;
        p.font = font;
        p.text = text;
        p.origin = origin;
        p.rotation = rot;
        p.eye = eye;
    }

    void PaintIPNode::compileFrame(Component* comp)
    {
        const StringProperty* orderP = comp->property<StringProperty>("order");
        size_t s = orderP->size();

        const string frameName = comp->name().substr(6);
        int frame = atoi(frameName.c_str());

        Components& fcomps = m_frameMap[frame];
        fcomps.clear();

        for (size_t i = 0; i < s; i++)
        {
            const string& c = (*orderP)[i];
            if (Component* fc = component(c))
            {
                fcomps.push_back(fc);
            }
        }
    }

    void PaintIPNode::propertyChanged(const Property* p)
    {
        if (const Component* c = componentOf(p))
        {
            if (c->name().size() > 4)
            {
                string s = c->name().substr(0, 4);
                if (s == "pen:")
                    compilePenComponent((Component*)c);
            }

            if (c->name().size() > 5)
            {
                string s = c->name().substr(0, 5);
                if (s == "text:")
                    compileTextComponent((Component*)c);
            }

            if (c->name().size() > 6)
            {
                string s = c->name().substr(0, 6);
                if (s == "frame:")
                    compileFrame((Component*)c);
            }
        }

        IPNode::propertyChanged(p);
    }

    static void fixV1Coordinates(Vec2fProperty* p,
                                 const IPNode::ImageStructureInfo& info)
    {
        const float aspect = float(info.width) / float(info.height) / 2.0;
        const Mat44f& O = info.orientation;
        Mat33f M(O(0, 0), O(0, 1), O(0, 2), O(1, 0), O(1, 1), O(1, 2), O(2, 0),
                 O(2, 1), O(2, 2));

        //
        //  DPX orientation was modified so it has to be treated uniquely
        //

        // const bool skip = info.proxy.hasAttribute("DPX/Version") &&
        // info.dataType != FrameBuffer::PACKED_R10_G10_B10_X2;

        // if (skip) return;

        for (Vec2f *v = p->data(), *e = p->data() + p->size(); v != e; v++)
        {
            (*v) = ((*v) - Vec2f(aspect, .5)) * M;
        }
    }

    void PaintIPNode::readCompleted(const string& typeName,
                                    unsigned int version)
    {
        Components& comps = components();

        if (version == 1)
        {
            //
            //  Need to transform the cooridinates to version 2 style
            //

            ImageStructureInfo info = imageStructureInfo(
                graph()->contextForFrame(graph()->cache().inFrame()));

            for (size_t i = 0; i < comps.size(); i++)
            {
                Component* c = comps[i];

                if (c->name().size() > 4)
                {
                    string s = c->name().substr(0, 4);

                    if (s == "pen:")
                    {
                        if (Vec2fProperty* p =
                                c->property<Vec2fProperty>("points"))
                        {
                            fixV1Coordinates(p, info);
                        }

                        //
                        //  Transfer old version number information to
                        //  paint stroke so its rendered correct by RV > 4
                        //

                        ostringstream name;
                        name << c->name() << ".version";
                        declareProperty<IntProperty>(name.str(), version);
                    }
                }

                if (c->name().size() > 5)
                {
                    string s = c->name().substr(0, 5);

                    if (s == "text:")
                    {
                        if (Vec2fProperty* p =
                                c->property<Vec2fProperty>("position"))
                        {
                            fixV1Coordinates(p, info);
                        }
                    }
                }
            }
        }
        else if (version == 2)
        {
            //
            //  In paint version 2 (and 1) files, the "replace" pen
            //  strokes were rendered using blending which cannot be
            //  replicated using shaders. So these are marked as such. New
            //  paint strokes will have the new shader based rendering
            //  which does not rely on blending.
            //

            for (size_t i = 0; i < comps.size(); i++)
            {
                Component* c = comps[i];

                if (c->name().size() > 4)
                {
                    string s = c->name().substr(0, 4);

                    if (s == "pen:")
                    {
                        ostringstream name;
                        name << c->name() << ".version";
                        declareProperty<IntProperty>(name.str(), version);
                    }
                }
            }
        }

        for (size_t i = 0; i < comps.size(); i++)
        {
            Component* c = comps[i];

            if (c->name().size() > 4)
            {
                string s = c->name().substr(0, 4);
                if (s == "pen:")
                    compilePenComponent(c);
            }

            if (c->name().size() > 5)
            {
                string s = c->name().substr(0, 5);
                if (s == "text:")
                    compileTextComponent(c);
            }

            if (c->name().size() > 6)
            {
                string s = c->name().substr(0, 6);
                if (s == "frame:")
                    compileFrame(c);
            }
        }

        IPNode::readCompleted(typeName, version);
    }

    IPImage* PaintIPNode::evaluate(const Context& context)
    {
        IPImage* head = 0;

        if (inputs().empty() || !(head = IPNode::evaluate(context)))
        {
            return IPImage::newNoImage(this);
        }

        int frame = context.frame;

        IntProperty* showP = property<IntProperty>("paint", "show");
        StringProperty* incP = property<StringProperty>("paint", "include");
        StringProperty* excP = property<StringProperty>("paint", "exclude");

        bool showPaint = !showP || showP->empty() || showP->front() == 1;

        if (showPaint)
        {
            if (m_frameMap.count(frame) != 0)
            {
                Components& comps = m_frameMap[frame];
                size_t s = comps.size();

                for (size_t q = 0; q < s; q++)
                {
                    Component* c = comps[q];
                    int numProps = c->properties().size();

                    if (m_penStrokes.count(c) >= 1)
                    {
                        if (numProps == 0)
                        {
                            m_penStrokes.erase(c);
                        }
                        else
                        {
                            LocalPolyLine& pl = m_penStrokes[c];

                            if (pl.eye == 2 || (pl.eye == context.eye))
                            {
                                head->commands.push_back(&pl);
                            }
                        }
                    }
                    else if (m_texts.count(c) >= 1)
                    {
                        if (numProps == 0)
                        {
                            m_texts.erase(c);
                        }
                        else
                        {
                            LocalText& t = m_texts[c];

                            if (t.eye == 2 || (t.eye == context.eye))
                            {
                                head->commands.push_back(&t);
                            }
                        }
                    }
                }
            }
        }

        if (m_tag = component("tag"))
        {
            const Component::Container& props = m_tag->properties();

            for (size_t i = 0; i < props.size(); i++)
            {
                if (const StringProperty* sp =
                        dynamic_cast<StringProperty*>(props[i]))
                {
                    if (sp->size())
                    {
                        head->tagMap[sp->name()] = sp->front();
                    }
                }
            }
        }

        return head;
    }

} // namespace IPCore
