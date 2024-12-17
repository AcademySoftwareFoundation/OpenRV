//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
#include <IPBaseNodes/OverlayIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <algorithm>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    OverlayIPNode::OverlayIPNode(const string& name, const NodeDefinition* def,
                                 IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_hasSideMattes(false)
        , m_matteRatio(0)
        , m_heightVisible(0)
        , m_imageWidth(0)
        , m_centerPoint(Vec2f(0, 0))
    {
        setMaxInputs(1);

        declareProperty<IntProperty>("overlay.nextRectId", 0);
        declareProperty<IntProperty>("overlay.nextTextId", 0);
        declareProperty<IntProperty>("overlay.show", 1);
    }

    OverlayIPNode::~OverlayIPNode() {}

    void OverlayIPNode::compileRectangleComponent(Component* c)
    {
        LocalRectangle& p = m_rectangles[c];
        const FloatProperty* heightP = c->property<FloatProperty>("height");
        const FloatProperty* widthP = c->property<FloatProperty>("width");
        const Vec4fProperty* colorP = c->property<Vec4fProperty>("color");
        const Vec2fProperty* posP = c->property<Vec2fProperty>("position");
        const IntProperty* eyeP = c->property<IntProperty>("eye");
        const IntProperty* actP = c->property<IntProperty>("active");

        const float height =
            heightP && heightP->size() ? heightP->front() : 1.0f;
        const float width = widthP && widthP->size() ? widthP->front() : 1.0f;
        const Vec4f color = colorP && colorP->size()
                                ? colorP->front()
                                : Vec4f(0.0, 0.0, 0.0, 1.0);
        const Vec2f pos =
            posP && posP->size() ? posP->front() : Vec2f(0.0, 0.0);
        const int eye = eyeP && eyeP->size() ? eyeP->front() : 2;
        const bool active = actP && actP->size() ? (actP->front() != 0) : true;

        p.height = height;
        p.width = width;
        p.color = color;
        p.pos = pos;
        p.eye = eye;
        p.active = active;
    }

    void OverlayIPNode::compileTextComponent(Component* c)
    {
        LocalText& lt = m_texts[c];
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
        const IntProperty* actP = c->property<IntProperty>("active");
        const Vec2fProperty* psP = c->property<Vec2fProperty>("pixelScale");
        const IntProperty* ffP = c->property<IntProperty>("firstFrame");

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
        const int debug = debugP && debugP->size() ? debugP->front() : 0;
        const int eye = eyeP && eyeP->size() ? eyeP->front() : 2;
        const int ff = ffP && ffP->size() ? ffP->front() : 1;
        const Vec2f ps = psP && psP->size() ? psP->front() : Vec2f(0.0, 0.0);

        const bool active = actP && actP->size() ? (actP->front() != 0) : true;

        float aspect = (ps == Vec2f(0.0, 0.0)) ? 0.0 : ps.x / ps.y;

        int frameCount = (textP) ? textP->size() : 0;

        lt.texts.resize(frameCount);
        lt.eye = eye;
        lt.active = active;
        lt.firstFrame = ff;

        if (frameCount < 1)
            lt.active = false;

        for (int i = 0; i < lt.texts.size(); ++i)
        {
            Paint::Text& p = lt.texts[i];

            p.ptsize = size * 100.0 * 100.0;
            p.scale = 1.0 / 80.0 / 10.0 * scale;
            p.spacing = space;
            p.color = color;
            p.font = font;
            p.origin = origin;
            p.rotation = rot;
            p.text = (*textP)[i];

            if (aspect == 0.0)
            {
                p.pos = pos;
            }
            else
            {
                p.pos.x = aspect * (pos.x - ps.x / 2.0) / ps.x;
                p.pos.y = (pos.y - ps.y / 2.0) / ps.y;
            }
        }
    }

    Paint::Text& OverlayIPNode::LocalText::commandForFrame(int frame)
    {
        int index = frame - firstFrame;
        index = min(max(0, index), int(texts.size() - 1));

        return texts[index];
    }

    namespace
    {

        static void initializeOutline(Paint::PolyLine& outline,
                                      const Vec2f* points, size_t npoints,
                                      TwkPaint::Color oc, float outlineWidth,
                                      const string& brush)
        {
            // these polyline owns the memory of their own points
            outline.points = new Vec2f[npoints];
            memcpy((void*)outline.points, (void*)points,
                   npoints * sizeof(Vec2f));
            outline.ownPoints = true;

            outline.npoints = npoints;
            outline.color = oc;
            outline.width = outlineWidth;
            outline.brush = brush;
        }

        void assembleFrameCommands(
            OverlayIPNode::LocalWindow::FrameCommands& fc, int index, float ia,
            const Vec2f& ps, const Vec4f& oc, const Vec4f& wc,
            const FloatProperty* ULxP, const FloatProperty* ULyP,
            const FloatProperty* URxP, const FloatProperty* URyP,
            const FloatProperty* LLxP, const FloatProperty* LLyP,
            const FloatProperty* LRxP, const FloatProperty* LRyP,
            bool antialias, const string& brush)
        {

            fc.antialiased = antialias;

            //
            //  Outline paint command.
            //

            Vec2f UL =
                Vec2f(index < ULxP->size() ? (*ULxP)[index] : ULxP->front(),
                      index < ULyP->size() ? (*ULyP)[index] : ULyP->front());

            Vec2f UR =
                Vec2f(index < URxP->size() ? (*URxP)[index] : URxP->front(),
                      index < URyP->size() ? (*URyP)[index] : URyP->front());

            Vec2f LL =
                Vec2f(index < LLxP->size() ? (*LLxP)[index] : LLxP->front(),
                      index < LLyP->size() ? (*LLyP)[index] : LLyP->front());

            Vec2f LR =
                Vec2f(index < LRxP->size() ? (*LRxP)[index] : LRxP->front(),
                      index < LRyP->size() ? (*LRyP)[index] : LRyP->front());

            Vec2f ULorig = UL;
            Vec2f URorig = UR;
            Vec2f LLorig = LL;
            Vec2f LRorig = LR;

            if (ia == 0.0)
            //
            //  Use pixel scale to compute image aspect and normalize
            //  coordinates.
            //
            {
                ia = ps.x / ps.y;

                UL.x = ia * (UL.x - ps.x / 2.0) / ps.x;
                UR.x = ia * (UR.x - ps.x / 2.0) / ps.x;
                LL.x = ia * (LL.x - ps.x / 2.0) / ps.x;
                LR.x = ia * (LR.x - ps.x / 2.0) / ps.x;

                UL.y = (UL.y - ps.y / 2.0) / ps.y;
                UR.y = (UR.y - ps.y / 2.0) / ps.y;
                LL.y = (LL.y - ps.y / 2.0) / ps.y;
                LR.y = (LR.y - ps.y / 2.0) / ps.y;
            }

            if (!antialias)
            {
                fc.plainOutline.drawMode = Paint::Quad::OutlineMode;
                fc.plainOutline.color = oc;
                fc.plainOutline.points[0] = UL;
                fc.plainOutline.points[1] = UR;
                fc.plainOutline.points[2] = LR;
                fc.plainOutline.points[3] = LL;
            }

            else
            {
                // colored outline
                float outlineWidth = 0.003;

                fc.outline.resize(4);

                Vec2f top[2] = {UL, UR};
                Vec2f left[2] = {UL, LL};
                Vec2f right[2] = {UR, LR};
                Vec2f bottom[2] = {LL, LR};

                initializeOutline(fc.outline[0], top, 2, oc, outlineWidth,
                                  brush);

                initializeOutline(fc.outline[1], left, 2, oc, outlineWidth,
                                  brush);

                initializeOutline(fc.outline[2], right, 2, oc, outlineWidth,
                                  brush);

                initializeOutline(fc.outline[3], bottom, 2, oc, outlineWidth,
                                  brush);

                // this is the solid matt outline
                fc.quadsOutline.resize(4);

                initializeOutline(fc.quadsOutline[0], top, 2, wc, outlineWidth,
                                  brush);

                initializeOutline(fc.quadsOutline[1], left, 2, wc, outlineWidth,
                                  brush);

                initializeOutline(fc.quadsOutline[2], right, 2, wc,
                                  outlineWidth, brush);

                initializeOutline(fc.quadsOutline[3], bottom, 2, wc,
                                  outlineWidth, brush);
            }

            //
            //  Solid matting commands
            //

            fc.quads.resize(8);

            for (int i = 0; i < fc.quads.size(); ++i)
            {
                fc.quads[i].color = wc;
                fc.quads[i].drawMode = Paint::Quad::SolidMode;
            }

            //
            //  Corners
            //

            float xMin = -0.5 * ia;
            float xMax = 0.5 * ia;
            float yMin = -0.5;
            float yMax = 0.5;

            fc.quads[0].points[0] = Vec2f(xMin, yMax);
            fc.quads[0].points[1] = Vec2f(UL.x, yMax);
            fc.quads[0].points[2] = Vec2f(UL.x, UL.y);
            fc.quads[0].points[3] = Vec2f(xMin, UL.y);

            fc.quads[1].points[0] = Vec2f(UR.x, yMax);
            fc.quads[1].points[1] = Vec2f(xMax, yMax);
            fc.quads[1].points[2] = Vec2f(xMax, UR.y);
            fc.quads[1].points[3] = Vec2f(UR.x, UR.y);

            fc.quads[2].points[0] = Vec2f(LR.x, LR.y);
            fc.quads[2].points[1] = Vec2f(xMax, LR.y);
            fc.quads[2].points[2] = Vec2f(xMax, yMin);
            fc.quads[2].points[3] = Vec2f(LR.x, yMin);

            fc.quads[3].points[0] = Vec2f(xMin, LL.y);
            fc.quads[3].points[1] = Vec2f(LL.x, LL.y);
            fc.quads[3].points[2] = Vec2f(LL.x, yMin);
            fc.quads[3].points[3] = Vec2f(xMin, yMin);

            //
            //  Sides
            //

            //  Top
            fc.quads[4].points[0] = Vec2f(UL.x, yMax);
            fc.quads[4].points[1] = Vec2f(UR.x, yMax);
            fc.quads[4].points[2] = Vec2f(UR.x, UR.y);
            fc.quads[4].points[3] = Vec2f(UL.x, UL.y);

            //  Bottom
            fc.quads[5].points[0] = Vec2f(LL.x, LL.y);
            fc.quads[5].points[1] = Vec2f(LR.x, LR.y);
            fc.quads[5].points[2] = Vec2f(LR.x, yMin);
            fc.quads[5].points[3] = Vec2f(LL.x, yMin);

            //  Left
            fc.quads[6].points[0] = Vec2f(xMin, UL.y);
            fc.quads[6].points[1] = Vec2f(UL.x, UL.y);
            fc.quads[6].points[2] = Vec2f(LL.x, LL.y);
            fc.quads[6].points[3] = Vec2f(xMin, LL.y);

            //  Right
            fc.quads[7].points[0] = Vec2f(UR.x, UR.y);
            fc.quads[7].points[1] = Vec2f(xMax, UR.y);
            fc.quads[7].points[2] = Vec2f(xMax, LR.y);
            fc.quads[7].points[3] = Vec2f(LR.x, LR.y);
        }

    }; //  namespace

    void OverlayIPNode::compileWindowComponent(Component* c)
    {
        //  XXX locking ?

        LocalWindow& w = m_windows[c];

        const IntProperty* eyeP = c->property<IntProperty>("eye");
        const IntProperty* windowActiveP =
            c->property<IntProperty>("windowActive");
        const IntProperty* outlineActiveP =
            c->property<IntProperty>("outlineActive");
        const FloatProperty* outlineWidthP =
            c->property<FloatProperty>("outlineWidth");
        const Vec4fProperty* outlineColorP =
            c->property<Vec4fProperty>("outlineColor");
        const StringProperty* outlineBrushP =
            c->property<StringProperty>("outlineBrush");
        const Vec4fProperty* windowColorP =
            c->property<Vec4fProperty>("windowColor");
        const FloatProperty* imageAspectP =
            c->property<FloatProperty>("imageAspect");
        const Vec2fProperty* pixelScaleP =
            c->property<Vec2fProperty>("pixelScale");

        const IntProperty* firstFrameP = c->property<IntProperty>("firstFrame");

        const FloatProperty* windowULxP =
            c->property<FloatProperty>("windowULx");
        const FloatProperty* windowULyP =
            c->property<FloatProperty>("windowULy");

        const FloatProperty* windowURxP =
            c->property<FloatProperty>("windowURx");
        const FloatProperty* windowURyP =
            c->property<FloatProperty>("windowURy");

        const FloatProperty* windowLLxP =
            c->property<FloatProperty>("windowLLx");
        const FloatProperty* windowLLyP =
            c->property<FloatProperty>("windowLLy");

        const FloatProperty* windowLRxP =
            c->property<FloatProperty>("windowLRx");
        const FloatProperty* windowLRyP =
            c->property<FloatProperty>("windowLRy");

        const IntProperty* antialias = c->property<IntProperty>("antialias");

        //
        //  Make sure we have what we need, and if we return early all active
        //  flags are false:
        //

        w.windowActive = w.outlineActive = false;

        if (!windowULxP || !windowULyP || !windowURxP || !windowURyP
            || !windowLLxP || !windowLLyP || !windowLRxP || !windowLRyP)
        {
            return;
        }

        int minSize = min(windowULxP->size(),
                          min(windowULyP->size(),
                              min(windowURxP->size(),
                                  min(windowURyP->size(),
                                      min(windowLLxP->size(),
                                          min(windowLLyP->size(),
                                              min(windowLRxP->size(),
                                                  windowLRyP->size())))))));

        int maxSize = max(windowULxP->size(),
                          max(windowULyP->size(),
                              max(windowURxP->size(),
                                  max(windowURyP->size(),
                                      max(windowLLxP->size(),
                                          max(windowLLyP->size(),
                                              max(windowLRxP->size(),
                                                  windowLRyP->size())))))));

        if (minSize < 1)
            return;

        if (maxSize > 1 && (!firstFrameP || firstFrameP->size() < 1))
            return;

        float ia = (imageAspectP && imageAspectP->size())
                       ? imageAspectP->front()
                       : 0.0;
        Vec2f ps = (pixelScaleP && pixelScaleP->size()) ? pixelScaleP->front()
                                                        : Vec2f(0.0, 0.0);

        if (ia == 0.0 && ps == Vec2f(0.0, 0.0))
            return;

        //
        //  These have sensible defaults
        //

        w.windowActive = (windowActiveP && windowActiveP->size())
                             ? (windowActiveP->front() != 0)
                             : false;
        w.outlineActive = (outlineActiveP && outlineActiveP->size())
                              ? (outlineActiveP->front() != 0)
                              : false;
        w.outlineWidth = (outlineWidthP && outlineWidthP->size())
                             ? outlineWidthP->front()
                             : 3.0;
        w.outlineBrush = (outlineBrushP && outlineBrushP->size())
                             ? outlineBrushP->front()
                             : "gauss";
        w.firstFrame =
            (firstFrameP && firstFrameP->size()) ? firstFrameP->front() : 1;
        w.eye = (eyeP && eyeP->size()) ? eyeP->front() : 2;

        const Vec4f wc = (windowColorP && windowColorP->size())
                             ? windowColorP->front()
                             : Vec4f(0.0, 0.0, 0.0, 1.0);
        const Vec4f oc = (outlineColorP && outlineColorP->size())
                             ? outlineColorP->front()
                             : Vec4f(0.4, 0.4, 1.0, 1.0);

        w.scalingWidth = (ia == 0.0) ? ps.x : 0.0;

        //
        //  Don't do anything if there's nothing to do
        //

        if (!w.windowActive && !w.outlineActive)
            return;

        //
        //  We have a set of paint commands per-frame, unless none of the
        //  vertices are animated, in which case we use one set of commands for
        //  all frames.
        //

        w.frameCommands.resize(maxSize);

        bool doAntialias =
            (antialias && antialias->size()) ? antialias->front() : false;
        for (int i = 0; i < w.frameCommands.size(); ++i)
        {
            LocalWindow::FrameCommands& fc = w.frameCommands[i];

            assembleFrameCommands(fc, i, ia, ps, oc, wc, windowULxP, windowULyP,
                                  windowURxP, windowURyP, windowLLxP,
                                  windowLLyP, windowLRxP, windowLRyP,
                                  doAntialias, w.outlineBrush);
        }
    }

    OverlayIPNode::LocalWindow::FrameCommands&
    OverlayIPNode::LocalWindow::commandsForFrame(int frame)
    {
        int index = frame - firstFrame;
        index = min(max(0, index), int(frameCommands.size() - 1));

        return frameCommands[index];
    }

    void OverlayIPNode::propertyChanged(const Property* p)
    {
        if (const Component* c = componentOf(p))
        {
            if (c->name().size() > 5)
            {
                string s = c->name().substr(0, 5);
                if (s == "rect:")
                    compileRectangleComponent((Component*)c);
                if (s == "text:")
                    compileTextComponent((Component*)c);

                if (c->name().size() > 7 && c->name().substr(0, 7) == "window:")
                {
                    compileWindowComponent((Component*)c);
                }
            }
        }

        IPNode::propertyChanged(p);
    }

    void OverlayIPNode::readCompleted(const string& typeName,
                                      unsigned int version)
    {
        Components& comps = components();

        for (size_t i = 0; i < comps.size(); i++)
        {
            Component* c = comps[i];

            if (c->name().size() > 5)
            {
                string s = c->name().substr(0, 5);
                if (s == "rect:")
                    compileRectangleComponent((Component*)c);
                if (s == "text:")
                    compileTextComponent((Component*)c);

                if (c->name().size() > 7 && c->name().substr(0, 7) == "window:")
                {
                    compileWindowComponent((Component*)c);
                }
            }
        }

        IPNode::readCompleted(typeName, version);
    }

    IPImage* OverlayIPNode::evaluate(const Context& context)
    {
        IPImage* head = 0;

        if (inputs().empty() || !(head = IPNode::evaluate(context)))
        {
            return IPImage::newNoImage(this);
        }

        static int mattesOnTop = -1;
        if (mattesOnTop == -1)
        {
            if (getenv("TWK_MATTES_ON_TOP"))
                mattesOnTop = 1;
            else
                mattesOnTop = 0;
        }
        if (!mattesOnTop)
            addMattePaintCommands(head);

        IntProperty* showP = property<IntProperty>("overlay", "show");
        bool showOverlay = !showP || showP->empty() || showP->front() == 1;

        if (showOverlay)
        {
            Components& comps = components();

            size_t s = comps.size();

            for (size_t q = 0; q < s; q++)
            {
                Component* c = comps[q];
                int numProps = c->properties().size();

                if (m_rectangles.count(c) >= 1)
                {
                    if (numProps == 0)
                    {
                        m_rectangles.erase(c);
                    }
                    else
                    {
                        LocalRectangle& r = m_rectangles[c];

                        if (r.active && (r.eye == 2 || (r.eye == context.eye)))
                        {
                            head->commands.push_back(&r);
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

                        if (t.active && (t.eye == 2 || (t.eye == context.eye)))
                        {
                            head->commands.push_back(
                                &t.commandForFrame(context.frame));
                        }
                    }
                }
                else if (m_windows.count(c) >= 1)
                {
                    if (numProps == 0)
                    {
                        m_windows.erase(c);
                    }
                    else
                    {
                        LocalWindow& w = m_windows[c];

                        if (w.eye == 2 || (w.eye == context.eye))
                        {
                            LocalWindow::FrameCommands& fc =
                                w.commandsForFrame(context.frame);

                            if (w.windowActive)
                            {
                                for (int i = 0; i < fc.quads.size(); ++i)
                                {
                                    head->commands.push_back(&fc.quads[i]);
                                }
                                if (fc.antialiased)
                                {
                                    float s = (w.scalingWidth != 0.0)
                                                  ? 1.0 / w.scalingWidth
                                                  : 1.0 / head->width;

                                    for (int i = 0; i < fc.quadsOutline.size();
                                         ++i)
                                    {
                                        float oldWidth =
                                            fc.quadsOutline[i].width;
                                        fc.quadsOutline[i].width =
                                            s * w.outlineWidth;

                                        if (fc.quadsOutline[i].width
                                            != oldWidth)
                                            fc.quadsOutline[i].built = false;

                                        head->commands.push_back(
                                            &fc.quadsOutline[i]);
                                    }
                                }
                            }

                            if (w.outlineActive)
                            {
                                if (fc.antialiased)
                                {
                                    float s = (w.scalingWidth != 0.0)
                                                  ? 1.0 / w.scalingWidth
                                                  : 1.0 / head->width;

                                    for (int i = 0; i < fc.outline.size(); ++i)
                                    {
                                        float oldWidth = fc.outline[i].width;
                                        fc.outline[i].width =
                                            s * w.outlineWidth;

                                        if (fc.outline[i].width != oldWidth)
                                            fc.outline[i].built = false;

                                        head->commands.push_back(
                                            &fc.outline[i]);
                                    }
                                }
                                else
                                {
                                    head->commands.push_back(&fc.plainOutline);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (mattesOnTop)
            addMattePaintCommands(head);

        if (!head->commands.empty())
            head->commands.push_back(&m_executeAll);

        return head;
    }

    bool OverlayIPNode::addMattePaintCommands(IPImage* head)
    {

        //
        // Look first for local matte settings then for shared matte settings
        //

        IPNode* sessionNode = graph()->sessionNode();
        bool drawMatte =
            sessionNode->property<IntProperty>("matte", "show")->front();
        IntProperty* showMatte = property<IntProperty>("matte", "show");
        if (showMatte && showMatte->size())
            drawMatte = showMatte->front();
        if (drawMatte)
        {
            //
            // aspect: The aspect ratio of the matte
            // opacity: The opacity of the matte (0.0 - 1.0)
            // heightVisible: Fraction of the image that is visible (0.0 - 1.0).
            //     Negative values mean ignore.
            // centerPoint: The center of the matte as well as the visbile
            // section
            //     of the unmatted image.
            //

            float matteOpacity =
                sessionNode->property<FloatProperty>("matte", "opacity")
                    ->front();
            FloatProperty* fp = property<FloatProperty>("matte", "opacity");
            if (fp && fp->size())
                matteOpacity = fp->front();
            if (matteOpacity <= 0)
                matteOpacity = 0.66;
            m_top.color = m_bottom.color = m_left.color = m_right.color =
                Vec4f(0.0, 0.0, 0.0, matteOpacity);

            bool refresh = false;

            float matteRatio =
                sessionNode->property<FloatProperty>("matte", "aspect")
                    ->front();
            fp = property<FloatProperty>("matte", "aspect");
            if (fp && fp->size())
                matteRatio = fp->front();
            if (matteRatio <= 0)
                matteRatio = 1.33;
            refresh |= (m_matteRatio != matteRatio);

            float heightVisible =
                sessionNode->property<FloatProperty>("matte", "heightVisible")
                    ->front();
            fp = property<FloatProperty>("matte", "heightVisible");
            if (fp && fp->size())
                heightVisible = fp->front();
            refresh |= (m_heightVisible != heightVisible);

            Vec2f centerPoint =
                sessionNode->property<Vec2fProperty>("matte", "centerPoint")
                    ->front();
            Vec2fProperty* vp = property<Vec2fProperty>("matte", "centerPoint");
            if (vp && vp->size())
                centerPoint = vp->front();
            refresh |= (m_centerPoint != centerPoint);

            int headWidth = head->width;
            int headHeight = head->height;

            //
            //  Note we ignore the incoming image's pixel aspect here, since
            //  displayWidth/Height already incorporates this info.
            //

            float imageWidth = float(headWidth) / float(headHeight);
            refresh |= (m_imageWidth != imageWidth);

            //
            //  If we don't need to recalculate geometry, then add the cached
            //  rects
            //

            if (!refresh)
            {
                head->commands.push_back(&m_top);
                head->commands.push_back(&m_bottom);
                if (m_hasSideMattes)
                {
                    head->commands.push_back(&m_left);
                    head->commands.push_back(&m_right);
                }
                return drawMatte;
            }

            //
            // If the heightVisible is negative calculate what it should be to
            // entirely cover the width of the image
            //

            m_hasSideMattes = true;
            if (heightVisible <= 0)
            {
                heightVisible = min(imageWidth / matteRatio, 1.0f);
                centerPoint.x = 0;
                m_hasSideMattes = false;
            }
            float widthVisible = matteRatio * heightVisible;

            //
            // leftMatteWidth       widthVisible      rightMatteWidth
            //  ____/\_____  ___________/\____________  ____/\_____
            // /           \/                         \/           \
        // ------------------------[0.5]------------------------
            // |                                                   |\
        // |                 topMatte (m_top)                  | }
            // topMatteHeight
            // |(leftSideOfImage,topMatteVPosition)                |/
            // *---------------------------------------------------|
            // |           |                          |            |\
        // |           |                          |            | \
        // | leftMatte |      Visible Image       | rightMatte |  \
        // |  (m_left) |                          | (m_right)  |   }
            // heightVisible |           |                          | |  / | |
            // |            | /
            // |(leftSideOfImage,sideMattesVPosition)
            // |(rightMatteHPosition,sideMattesVPosition)
            // *--------------------------------------*------------|
            // |                                                   |\
        // |              bottomMatte (m_bottom)               | }
            // bottomMatteHeight
            // |(leftSideOfImage,-0.5)                             |/
            // *----------------------[-0.5]------------------------
            // \________________________  _________________________/
            //                          \/
            //                      imageWidth
            //

            float topMatteHeight = 0.5 - centerPoint.y - (heightVisible / 2.0);
            float topMatteVPosition = 0.5 - topMatteHeight;
            float bottomMatteHeight = 1.0 - topMatteHeight - heightVisible;
            float leftSideOfImage = -0.5 * imageWidth;

            m_top.height = topMatteHeight;
            m_top.width = imageWidth;
            m_top.pos = Vec2f(leftSideOfImage, topMatteVPosition);
            head->commands.push_back(&m_top);

            m_bottom.height = bottomMatteHeight;
            m_bottom.width = imageWidth;
            m_bottom.pos = Vec2f(leftSideOfImage, -0.5);
            head->commands.push_back(&m_bottom);

            if (m_hasSideMattes)
            {
                float sideMattesVPosition = -0.5 + bottomMatteHeight;
                float leftMatteWidth = max(
                    (((imageWidth - widthVisible) / 2.0) + centerPoint.x), 0.0);
                float rightMatteWidth = max(
                    (((imageWidth - widthVisible) / 2.0) - centerPoint.x), 0.0);
                float rightMatteHPosition =
                    (imageWidth / 2.0) - rightMatteWidth;

                m_left.height = heightVisible;
                m_left.width = leftMatteWidth;
                m_left.pos = Vec2f(leftSideOfImage, sideMattesVPosition);
                head->commands.push_back(&m_left);

                m_right.height = heightVisible;
                m_right.width = rightMatteWidth;
                m_right.pos = Vec2f(rightMatteHPosition, sideMattesVPosition);
                head->commands.push_back(&m_right);
            }

            //
            //  Store the new cached settings
            //

            m_matteRatio = matteRatio;
            m_heightVisible = heightVisible;
            m_centerPoint = centerPoint;
            m_imageWidth = imageWidth;
        }

        return drawMatte;
    }

} // namespace IPCore
