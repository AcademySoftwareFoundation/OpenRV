//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IPBaseNodes/PaintIPNode.h>
#include <IPCore/BrushTextureManager.h>
#include <IPCore/PaintCommand.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <TwkGLF/GL.h>
#include <TwkGLText/TwkGLText.h>
#include <exception>
#include <mutex>
#include <stl_ext/string_algo.h>
#include <cstdlib>
#include <IPCore/ShaderCommon.h>
#include <string>
#include <tuple>

namespace
{
    using namespace IPCore;
    using PerFramePaintCommands = std::map<int, PaintIPNode::LocalCommands>;

    int getStartFrame(const TwkContainer::Component& component)
    {
        std::stringstream stringStream;
        stringStream << component.name();

        std::string property;
        std::vector<std::string> properties{};

        while (std::getline(stringStream, property, ':'))
        {
            properties.push_back(property);
        }

        if (properties.size() > 2)
        {
            try
            {
                return std::stoi(properties[2]);
            }
            catch (const std::exception& e)
            {
                std::cout << "ERROR: Unable to get the frame from the pen component: " << e.what() << "'\n";
            }
        }

        return 0;
    }

    // Calculates the opacity based on the distance from the annotated frame to
    // make the ghosted annotation more visible closer to the frame and less
    // visible further away
    float getGhostOpacity(const int frame, const int startFrame, const int duration)
    {
        constexpr float minOpacity = 0.075;
        float ghostOpacity = 1.0;

        if (frame > startFrame)
            ghostOpacity = static_cast<float>(duration) / static_cast<float>(frame - startFrame) + minOpacity;
        else if (frame < startFrame)
            ghostOpacity = static_cast<float>(duration) / static_cast<float>(startFrame - frame) + minOpacity;

        // Clamp to [0, 1]: GL blend factors are clamped by hardware, but
        // QPainter QColor alpha must stay in [0, 255] or it wraps to near-zero.
        return std::min(1.0f, ghostOpacity);
    }

    // Loop over all commands and separate them in 3 containers:
    // 1. Commands that are visible on the current frame (based only on their
    // visibility range settings)
    // 2. Commands that end *before* the current frame
    // 3. Commands that start *after* the current frame
    auto separateCommandsByFrameGroup(const PaintIPNode::LocalCommands& commands, const int frame, const size_t eye)
    {
        PaintIPNode::LocalCommands currentFrameCommands; // visible commands, excluding hold and ghost
        PerFramePaintCommands beforeCommands;
        PerFramePaintCommands afterCommands;

        for (auto* localCommand : commands)
        {
            // Skip invisible annotations
            if (localCommand->eye != 2 && localCommand->eye != eye)
            {
                continue;
            }

            // Don't add polylines with 0 points (stamp strokes use stampInstances instead)
            if (auto* localPolyLine = dynamic_cast<PaintIPNode::LocalPolyLine*>(localCommand))
            {
                if (localPolyLine != nullptr && localPolyLine->npoints <= 0 && localPolyLine->stampInstances.empty())
                {
                    continue;
                }
            }

            const int startFrame = localCommand->startFrame;
            const int endFrame = startFrame + localCommand->duration - 1;

            if (frame >= startFrame && frame <= endFrame) // Command is visible on the current frame
            {
                currentFrameCommands.push_back(localCommand);
            }
            else
            {
                if (endFrame < frame) // Command ends before the current frame
                {
                    const int absDelta = frame - endFrame;
                    beforeCommands[absDelta].push_back(localCommand);
                }
                else
                { // Command starts after the current frame
                    const int absDelta = startFrame - frame;
                    afterCommands[absDelta].push_back(localCommand);
                }
            }
        }

        return std::make_tuple(currentFrameCommands, beforeCommands, afterCommands);
    }

    void addBeforeCommands(PaintIPNode::LocalCommands* allCommands, PaintIPNode::LocalCommands* currentFrameCommands,
                           const PerFramePaintCommands& beforeCommands, const PaintEffects& paintEffects, const int frame)
    {
        int levelIndex = 1;
        bool isHoldedCommandsInFirstLevel = false;

        bool isNewAnnotation = std::any_of(currentFrameCommands->begin(), currentFrameCommands->end(),
                                           [&frame](const auto* command) { return command->startFrame == frame; });

        for (const auto& beforeCommand : beforeCommands)
        {
            int ghostLevel = levelIndex - (isHoldedCommandsInFirstLevel ? 1 : 0);

            for (auto* command : beforeCommand.second)
            {
                // Keep held annotations on the current frame only if there is
                // no new annotation to add
                if (levelIndex == 1 && paintEffects.hold != 0 && !isNewAnnotation)
                {
                    isHoldedCommandsInFirstLevel = true;
                    command->ghostOn = true;

                    if (auto* polyLine = dynamic_cast<PaintIPNode::LocalPolyLine*>(command))
                    {
                        command->ghostColor = polyLine->color;
                    }
                    else if (auto* text = dynamic_cast<PaintIPNode::LocalText*>(command))
                    {
                        command->ghostColor = text->color;
                    }
                    else if (auto* rect = dynamic_cast<PaintIPNode::LocalRect*>(command))
                    {
                        command->ghostColor = rect->borderColor;
                    }
                    else if (auto* ellipse = dynamic_cast<PaintIPNode::LocalEllipse*>(command))
                    {
                        command->ghostColor = ellipse->borderColor;
                    }
                    else if (auto* arrow = dynamic_cast<PaintIPNode::LocalArrow*>(command))
                    {
                        command->ghostColor = arrow->borderColor;
                    }
                    else if (auto* line = dynamic_cast<PaintIPNode::LocalLine*>(command))
                    {
                        command->ghostColor = line->borderColor;
                    }

                    currentFrameCommands->push_back(command);
                }

                if (paintEffects.ghost != 0 && paintEffects.ghostBefore >= ghostLevel)
                {
                    command->ghostOn = true;
                    command->ghostColor = PaintIPNode::Color(1.0, 0.0, 0.0, 1.0); // Ghosted "Before" commands
                                                                                  // are drawn in green
                    command->ghostColor[3] = getGhostOpacity(frame, command->startFrame, command->duration);
                    allCommands->push_back(command);
                }
            }
            levelIndex++;
        }
    }

    void addAfterCommands(PaintIPNode::LocalCommands* allCommands, const PerFramePaintCommands& afterCommands,
                          const PaintEffects& paintEffects, const int frame)
    {
        int levelIndex = 1;

        for (const auto& afterCommand : afterCommands)
        {
            for (auto* command : afterCommand.second)
            {
                if (paintEffects.ghost != 0 && paintEffects.ghostAfter >= levelIndex)
                {
                    command->ghostOn = true;
                    command->ghostColor = PaintIPNode::Color(0.0, 1.0, 0.0,
                                                             1.0); // Ghosted "After" commands are drawn in red
                    command->ghostColor[3] = getGhostOpacity(frame, command->startFrame, command->duration);
                    allCommands->push_back(command);
                }
            }
            levelIndex++;
        }
    }

    void addVisibleCommands(PaintIPNode::LocalCommands* allCommands, const PaintIPNode::LocalCommands& currentFrameCommands)
    {
        for (auto* command : currentFrameCommands)
        {
            command->ghostOn = false;
            allCommands->push_back(command);
        }
    }

    PaintIPNode::LocalCommands generateVisibleCommands(const PaintIPNode::LocalCommands& commands, const int frame, const size_t eye,
                                                       const PaintEffects& paintEffects)
    {
        PaintIPNode::LocalCommands allCommands;          // visible commands, including hold and ghost
        PaintIPNode::LocalCommands currentFrameCommands; // visible commands, excluding hold and ghost
        PerFramePaintCommands beforeCommands;
        PerFramePaintCommands afterCommands;

        std::tie(currentFrameCommands, beforeCommands, afterCommands) = separateCommandsByFrameGroup(commands, frame, eye);

        addBeforeCommands(&allCommands, &currentFrameCommands, beforeCommands, paintEffects, frame);
        addAfterCommands(&allCommands, afterCommands, paintEffects, frame);
        addVisibleCommands(&allCommands, currentFrameCommands);

        return allCommands;
    }

    // Read a property's first value, falling back to defaultVal when absent or
    // empty.  T must be constructible from PropT::value_type — e.g. Col4f from
    // Vec4f, or int from int.  Scalar identity and cross-type construction both
    // work because T(expr) is the functional-cast form of the constructor call.
    template <typename PropT, typename T> T readProp(const TwkContainer::Component* c, const char* name, T defaultVal)
    {
        const PropT* p = c->property<PropT>(name);
        return (p && p->size()) ? T(p->front()) : std::move(defaultVal);
    }

    // Look up or insert an entry in a Component*-keyed map.  On first
    // insertion the new entry is also appended to commands so the render
    // loop picks it up without a full recompile.
    template <typename MapT, typename CmdsT> typename MapT::mapped_type& insertOrGet(MapT& map, TwkContainer::Component* c, CmdsT& commands)
    {
        const size_t prevSize = map.size();
        auto& entry = map[c];
        if (prevSize != map.size())
            commands.push_back(&entry);
        return entry;
    }

} // namespace

namespace IPCore
{
    using namespace std;
    using namespace stl_ext;
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkPaint;
    using namespace TwkGLText;

    PaintIPNode::PaintIPNode(const std::string& name, const NodeDefinition* def, IPGraph* g, GroupIPNode* group)
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
        const size_t prevNumStrokes = m_penStrokes.size();
        LocalPolyLine& p = m_penStrokes[c];

        // Add newly created strokes to the commands vector
        if (prevNumStrokes != m_penStrokes.size())
        {
            m_commands.push_back(&p);
        }

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

        const IntProperty* startFrameP = c->property<IntProperty>("startFrame");
        const IntProperty* durationP = c->property<IntProperty>("duration");

        const float width = widthP && widthP->size() ? widthP->front() : 0.01f;
        const Vec4f color = colorP && colorP->size() ? colorP->front() : Vec4f(1.0, 1.0, 1.0, 1.0);
        const string brush = brushP && brushP->size() ? brushP->front() : string("circle");
        const unsigned int join = joinP && joinP->size() ? joinP->front() : Path::RoundJoin;
        const unsigned int cap = capP && capP->size() ? capP->front() : Path::SquareCap;
        const int debug = debugP && debugP->size() ? debugP->front() : 0;
        const unsigned int mode = modeP && modeP->size() ? modeP->front() : Paint::PolyLine::OverMode;
        const unsigned int splat = splatP && splatP->size() ? splatP->front() : 0;
        const unsigned int version = versionP && versionP->size() ? versionP->front() : protocolVersion();
        const int eye = eyeP && eyeP->size() ? eyeP->front() : 2;

        const int startFrame = (startFrameP != nullptr && startFrameP->size() != 0) ? startFrameP->front() : getStartFrame(*c);
        const int duration = (durationP != nullptr && durationP->size() != 0) ? durationP->front() : 1;

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
        p.startFrame = startFrame;
        p.duration = duration;

        // Classify the brush type once — drives all downstream data paths.
        const bool isStampBrush = Paint::BrushTextureManager::instance().get(brush).isStamp;

        // Per-point widths are present when widthP has one entry per point.
        // Allow widthP to lag pointsP by one: the Mu layer inserts the point and
        // width in the same compound state change, but the IPGraph may re-evaluate
        // after the point insert before the width insert lands, leaving widthP
        // exactly one entry short. Clamp the index when that happens.
        const bool hasPerPointWidths = widthP && widthP->size() > 0 && pointsP && widthP->size() >= pointsP->size() - 1;

        if (pointsP && pointsP->size())
        {
            const auto* rawPts = static_cast<const Vec2f*>(pointsP->rawData());
            const size_t rawCount = pointsP->size();

            if (isStampBrush)
            {
                // ── Stamp path: feed raw points through smoother → StampPlacer ─
                if (!p.inputSmoother)
                {
                    p.inputSmoother = std::make_unique<TwkPaint::SmoothInterpolate2D>();
                    p.rawPointsSmoothed = 0;
                    p.stampPlacer = nullptr;
                    p.stampInstances.clear();
                }

                const size_t widthsCount = hasPerPointWidths ? widthP->size() : 0;

                for (size_t i = p.rawPointsSmoothed; i < rawCount; ++i)
                {
                    p.inputSmoother->add_point(rawPts[i]);

                    const size_t wi = (hasPerPointWidths && widthsCount > 0) ? std::min(i, widthsCount - 1) : static_cast<size_t>(-1);
                    const float w = (wi != static_cast<size_t>(-1)) ? static_cast<const float*>(widthP->rawData())[wi] : p.width;

                    TwkMath::Vec2f out;
                    while (p.inputSmoother->interpolate(out))
                    {
                        // Create placer lazily so p.width has its final value.
                        if (!p.stampPlacer)
                        {
                            TwkPaint::BrushParams params;
                            params.radius = p.width * 0.5f;
                            params.opacity = p.color[3];
                            p.stampPlacer = std::make_unique<TwkPaint::StampPath>(params);
                        }
                        p.stampPlacer->add_point(out, w * 0.5f);
                        TwkPaint::StampInstance s;
                        while (p.stampPlacer->next(s))
                            p.stampInstances.push_back(s);
                    }
                }

                p.rawPointsSmoothed = rawCount;
                // npoints mirrors stampInstances so hash() changes as stamps accumulate,
                // invalidating the IPGraph render cache mid-stroke.
                p.npoints = p.stampInstances.size();
            }
            else
            {
                // ── Ribbon path: assign raw points directly (input smoother was introduced
                // (for stamp brushes, let's leave ribbon brushes as they were)
                // The smoother densifies points ~6× which causes excessive opacity
                // accumulation from overlapping quads when rendering ribbon brushes.
                p.points.assign(rawPts, rawPts + rawCount);
                p.npoints = rawCount;

                if (widthP && widthP->size() == rawCount)
                    p.widths.assign(static_cast<const float*>(widthP->rawData()), static_cast<const float*>(widthP->rawData()) + rawCount);
                else
                    p.widths.clear();
            }
        }
        else
        {
            p.points.clear();
            p.widths.clear();
            p.stampInstances.clear();
            p.npoints = 0;
        }

        p.built = false;
    }

    void PaintIPNode::LocalText::hash(std::ostream& o) const
    {
        Paint::Text::hash(o);
        o << fontFamily << fontSize << fontWeight << fontStyle << textDecoration;
    }

    void PaintIPNode::compileTextComponent(Component* c)
    {
        LocalText& p = insertOrGet(m_texts, c, m_commands);

        // ptsize and scale are retained for session-file compatibility but are no
        // longer used for rendering (FTGL removed; all text renders via QPainter).
        p.ptsize = readProp<FloatProperty>(c, "size", 0.01f) * 100.0f * 100.0f;
        p.scale = readProp<FloatProperty>(c, "scale", 1.0f) / 80.0f / 10.0f;
        p.rotation = readProp<FloatProperty>(c, "rotation", 0.0f);
        p.spacing = readProp<FloatProperty>(c, "spacing", 1.0f);
        p.pos = readProp<Vec2fProperty>(c, "position", Vec2f(0.0f, 0.0f));
        p.color = readProp<Vec4fProperty>(c, "color", Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
        p.font = readProp<StringProperty>(c, "font", string(""));
        p.text = readProp<StringProperty>(c, "text", string(""));
        p.origin = readProp<StringProperty>(c, "origin", string(""));
        p.eye = readProp<IntProperty>(c, "eye", 2);
        p.startFrame = readProp<IntProperty>(c, "startFrame", 0);
        p.duration = readProp<IntProperty>(c, "duration", 0);

        p.fontFamily = readProp<StringProperty>(c, "fontFamily", string(""));
        p.fontSize = readProp<FloatProperty>(c, "fontSize", 24.0f);
        p.fontWeight = readProp<StringProperty>(c, "fontWeight", string("normal"));
        p.fontStyle = readProp<StringProperty>(c, "fontStyle", string("normal"));
        p.textDecoration = readProp<StringProperty>(c, "textDecoration", string("none"));
    }

    void PaintIPNode::compileRectComponent(Component* c)
    {
        LocalRect& r = insertOrGet(m_rects, c, m_commands);
        r.min = readProp<Vec2fProperty>(c, "min", Vec2f(0.0f, 0.0f));
        r.max = readProp<Vec2fProperty>(c, "max", Vec2f(0.1f, 0.1f));
        r.innerColor = readProp<Vec4fProperty>(c, "innerColor", Col4f(1.0f, 1.0f, 1.0f, 0.0f));
        r.borderColor = readProp<Vec4fProperty>(c, "borderColor", Col4f(1.0f, 1.0f, 1.0f, 1.0f));
        r.borderWidth = readProp<FloatProperty>(c, "borderWidth", 0.002f);
        r.eye = readProp<IntProperty>(c, "eye", 2);
        r.startFrame = readProp<IntProperty>(c, "startFrame", getStartFrame(*c));
        r.duration = readProp<IntProperty>(c, "duration", 1);
    }

    void PaintIPNode::compileEllipseComponent(Component* c)
    {
        LocalEllipse& e = insertOrGet(m_ellipses, c, m_commands);
        e.min = readProp<Vec2fProperty>(c, "min", Vec2f(0.0f, 0.0f));
        e.max = readProp<Vec2fProperty>(c, "max", Vec2f(0.1f, 0.1f));
        e.innerColor = readProp<Vec4fProperty>(c, "innerColor", Col4f(1.0f, 1.0f, 1.0f, 0.0f));
        e.borderColor = readProp<Vec4fProperty>(c, "borderColor", Col4f(1.0f, 1.0f, 1.0f, 1.0f));
        e.borderWidth = readProp<FloatProperty>(c, "borderWidth", 0.002f);
        e.eye = readProp<IntProperty>(c, "eye", 2);
        e.startFrame = readProp<IntProperty>(c, "startFrame", getStartFrame(*c));
        e.duration = readProp<IntProperty>(c, "duration", 1);
    }

    void PaintIPNode::compileArrowComponent(Component* c)
    {
        LocalArrow& a = insertOrGet(m_arrows, c, m_commands);
        a.startPos = readProp<Vec2fProperty>(c, "startPos", Vec2f(0.0f, 0.0f));
        a.endPos = readProp<Vec2fProperty>(c, "endPos", Vec2f(0.1f, 0.0f));
        a.innerColor = readProp<Vec4fProperty>(c, "innerColor", Col4f(1.0f, 1.0f, 1.0f, 1.0f));
        a.borderColor = readProp<Vec4fProperty>(c, "borderColor", Col4f(1.0f, 1.0f, 1.0f, 1.0f));
        a.thickness = readProp<FloatProperty>(c, "thickness", 0.005f);
        a.borderWidth = readProp<FloatProperty>(c, "borderWidth", 0.001f);
        a.eye = readProp<IntProperty>(c, "eye", 2);
        a.startFrame = readProp<IntProperty>(c, "startFrame", getStartFrame(*c));
        a.duration = readProp<IntProperty>(c, "duration", 1);
    }

    void PaintIPNode::compileLineComponent(Component* c)
    {
        LocalLine& l = insertOrGet(m_lines, c, m_commands);
        l.startPos = readProp<Vec2fProperty>(c, "startPos", Vec2f(0.0f, 0.0f));
        l.endPos = readProp<Vec2fProperty>(c, "endPos", Vec2f(0.1f, 0.0f));
        l.borderColor = readProp<Vec4fProperty>(c, "borderColor", Col4f(1.0f, 1.0f, 1.0f, 1.0f));
        l.borderWidth = readProp<FloatProperty>(c, "borderWidth", 0.002f);
        l.eye = readProp<IntProperty>(c, "eye", 2);
        l.startFrame = readProp<IntProperty>(c, "startFrame", getStartFrame(*c));
        l.duration = readProp<IntProperty>(c, "duration", 1);
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

    void PaintIPNode::setPaintEffects()
    {
        IPNode* sessionNode = graph()->sessionNode();
        if (sessionNode != nullptr)
        {
            const IntProperty* holdProperty = sessionNode->property<IntProperty>("paintEffects", "hold");
            const IntProperty* ghostProperty = sessionNode->property<IntProperty>("paintEffects", "ghost");
            const IntProperty* ghostBeforeProperty = sessionNode->property<IntProperty>("paintEffects", "ghostBefore");
            const IntProperty* ghostAfterProperty = sessionNode->property<IntProperty>("paintEffects", "ghostAfter");

            if (holdProperty != nullptr && holdProperty->size() != 0)
            {
                m_paintEffects.hold = holdProperty->front();
            }

            if (ghostProperty != nullptr && ghostProperty->size() != 0)
            {
                m_paintEffects.ghost = ghostProperty->front();
            }

            if (ghostBeforeProperty != nullptr && ghostBeforeProperty->size() != 0)
            {
                m_paintEffects.ghostBefore = ghostBeforeProperty->front();
            }

            if (ghostAfterProperty != nullptr && ghostAfterProperty->size() != 0)
            {
                m_paintEffects.ghostAfter = ghostAfterProperty->front();
            }
        }
    }

    void PaintIPNode::propertyChanged(const Property* p)
    {
        if (const Component* c = componentOf(p))
        {
            std::lock_guard<std::mutex> commandsGuard{m_commandsMutex};

            const string& cname = c->name();

            // "pen:" is 4 chars — component name must have at least 5 chars total
            if (cname.size() > 4 && cname.substr(0, 4) == "pen:")
                compilePenComponent((Component*)c);

            // "text:" and "rect:" and "line:" are 5 chars
            if (cname.size() > 5)
            {
                const string s5 = cname.substr(0, 5);
                if (s5 == "text:")
                    compileTextComponent((Component*)c);
                else if (s5 == "rect:")
                    compileRectComponent((Component*)c);
                else if (s5 == "line:")
                    compileLineComponent((Component*)c);
            }

            // "frame:" and "arrow:" are 6 chars
            if (cname.size() > 6)
            {
                const string s6 = cname.substr(0, 6);
                if (s6 == "frame:")
                    compileFrame((Component*)c);
                else if (s6 == "arrow:")
                    compileArrowComponent((Component*)c);
            }

            // "ellipse:" is 8 chars
            if (cname.size() > 8 && cname.substr(0, 8) == "ellipse:")
                compileEllipseComponent((Component*)c);
        }

        IPNode::propertyChanged(p);
    }

    static void fixV1Coordinates(Vec2fProperty* p, const IPNode::ImageStructureInfo& info)
    {
        const float aspect = float(info.width) / float(info.height) / 2.0;
        const Mat44f& O = info.orientation;
        Mat33f M(O(0, 0), O(0, 1), O(0, 2), O(1, 0), O(1, 1), O(1, 2), O(2, 0), O(2, 1), O(2, 2));

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

    void PaintIPNode::readCompleted(const string& typeName, unsigned int version)
    {
        Components& comps = components();

        if (version == 1)
        {
            //
            //  Need to transform the cooridinates to version 2 style
            //

            ImageStructureInfo info = imageStructureInfo(graph()->contextForFrame(graph()->cache().inFrame()));

            for (size_t i = 0; i < comps.size(); i++)
            {
                Component* c = comps[i];

                if (c->name().size() > 4)
                {
                    string s = c->name().substr(0, 4);

                    if (s == "pen:")
                    {
                        if (Vec2fProperty* p = c->property<Vec2fProperty>("points"))
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
                        if (Vec2fProperty* p = c->property<Vec2fProperty>("position"))
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
            const string& cname = c->name();

            // "pen:" is 4 chars
            if (cname.size() > 4 && cname.substr(0, 4) == "pen:")
                compilePenComponent(c);

            // "text:", "rect:", "line:" are 5 chars
            if (cname.size() > 5)
            {
                const string s5 = cname.substr(0, 5);
                if (s5 == "text:")
                    compileTextComponent(c);
                else if (s5 == "rect:")
                    compileRectComponent(c);
                else if (s5 == "line:")
                    compileLineComponent(c);
            }

            // "frame:", "arrow:" are 6 chars
            if (cname.size() > 6)
            {
                const string s6 = cname.substr(0, 6);
                if (s6 == "frame:")
                    compileFrame(c);
                else if (s6 == "arrow:")
                    compileArrowComponent(c);
            }

            // "ellipse:" is 8 chars
            if (cname.size() > 8 && cname.substr(0, 8) == "ellipse:")
                compileEllipseComponent(c);
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

        if (context.thread == IPNode::CacheEvalThread)
        {
            showPaint = false; // We don't want to compute annotations when only
                               // evaluating for caching
        }

        if (showPaint)
        {
            std::lock_guard<std::mutex> commandsGuard{m_commandsMutex};

            // generateVisibleCommands should only get the commands that are in
            // the order stack (e.g. m_frameMap)
            LocalCommands frameCommands{};
            for (const auto& frame : m_frameMap)
            {
                const Components& frameComponents = frame.second;
                for (const auto& frameComponent : frameComponents)
                {
                    if (m_penStrokes.count(frameComponent) > 0)
                    {
                        frameCommands.push_back(&m_penStrokes[frameComponent]);
                    }
                    else if (m_texts.count(frameComponent) > 0)
                    {
                        frameCommands.push_back(&m_texts[frameComponent]);
                    }
                    else if (m_rects.count(frameComponent) > 0)
                    {
                        frameCommands.push_back(&m_rects[frameComponent]);
                    }
                    else if (m_ellipses.count(frameComponent) > 0)
                    {
                        frameCommands.push_back(&m_ellipses[frameComponent]);
                    }
                    else if (m_arrows.count(frameComponent) > 0)
                    {
                        frameCommands.push_back(&m_arrows[frameComponent]);
                    }
                    else if (m_lines.count(frameComponent) > 0)
                    {
                        frameCommands.push_back(&m_lines[frameComponent]);
                    }
                }
            }

            setPaintEffects();

            LocalCommands visibleCommands = generateVisibleCommands(frameCommands, frame, context.eye, m_paintEffects);

            for (auto* visibleCommand : visibleCommands)
            {
                if (auto* polyLine = dynamic_cast<PaintIPNode::LocalPolyLine*>(visibleCommand))
                {
                    head->commands.push_back(polyLine);
                }
                else if (auto* localText = dynamic_cast<PaintIPNode::LocalText*>(visibleCommand))
                {
                    head->commands.push_back(localText);
                }
                else if (auto* localRect = dynamic_cast<PaintIPNode::LocalRect*>(visibleCommand))
                {
                    head->commands.push_back(localRect);
                }
                else if (auto* localEllipse = dynamic_cast<PaintIPNode::LocalEllipse*>(visibleCommand))
                {
                    head->commands.push_back(localEllipse);
                }
                else if (auto* localArrow = dynamic_cast<PaintIPNode::LocalArrow*>(visibleCommand))
                {
                    head->commands.push_back(localArrow);
                }
                else if (auto* localLine = dynamic_cast<PaintIPNode::LocalLine*>(visibleCommand))
                {
                    head->commands.push_back(localLine);
                }
            }
        }
        if (m_tag = component("tag"))
        {
            const Component::Container& props = m_tag->properties();

            for (size_t i = 0; i < props.size(); i++)
            {
                if (const StringProperty* sp = dynamic_cast<StringProperty*>(props[i]))
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
