//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPGraph__PaintIPNode__h__
#define __IPGraph__PaintIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/PaintCommand.h>
#include <TwkMath/Color.h>
#include <TwkPaint/Smoother.h>
#include <TwkPaint/StampPath.h>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace IPCore
{

    //
    //  PaintIPNode
    //
    //  Looks for components like type:NNNN where type is one of:
    //
    //      text
    //          Vec4 color
    //          float size
    //          Vec2 point
    //          float angle
    //          string text
    //

    struct PaintEffects
    {
        int hold = 0;
        int ghost = 0;
        int ghostBefore = 5;
        int ghostAfter = 5;
    };

    class PaintIPNode : public IPNode
    {
    public:
        //
        //  Types
        //

        //
        //  Paint:: commands + per-command state local to this node.
        //

        class LocalCommand
        {
        public:
            LocalCommand() = default;
            virtual ~LocalCommand() = default;

            int startFrame{};
            int duration{};
            int eye{-1};

            bool ghostOn{};
            TwkMath::Col4f ghostColor{Color(0.0)};
        };

        class LocalPolyLine
            : public Paint::PolyLine
            , public LocalCommand
        {
        public:
            LocalPolyLine() = default;

            // ── Ribbon brush state ────────────────────────────────────────────
            // Physics-based input smoother — persistent across compilePenComponent()
            // calls so that velocity/acceleration state accumulates correctly during
            // live drawing. Reset to nullptr at the start of each new stroke.
            std::unique_ptr<TwkPaint::SmoothInterpolate2D> inputSmoother;

            // Number of raw input points already fed through the smoother.
            size_t rawPointsSmoothed{0};

            // ── Stamp brush state ─────────────────────────────────────────────
            // Stamp placer — persistent across compilePenComponent() calls so that
            // arc-length state accumulates correctly during live drawing.
            // nullptr for ribbon brushes.
            std::unique_ptr<TwkPaint::StampPath> stampPlacer;

            // Accumulated stamp placements for the current stroke.
            std::vector<TwkPaint::StampInstance> stampInstances;
        };

        // Protocol string constants for Text.1 font fields.
        // These are the canonical wire values shared with the Python schema —
        // use these instead of bare string literals on both sides of the comparison.
        struct FontWeight
        {
            static constexpr const char* Normal = "normal";
            static constexpr const char* Bold = "bold";
        };

        struct FontStyle
        {
            static constexpr const char* Normal = "normal";
            static constexpr const char* Italic = "italic";
        };

        struct TextDecoration
        {
            static constexpr const char* None = "none";
            static constexpr const char* Underline = "underline";
        };

        class LocalText
            : public Paint::Text
            , public LocalCommand
        {
        public:
            LocalText() = default;

            // QPainter renderer — fontSize is in pixels
            std::string fontFamily;
            float fontSize{24.0f};
            std::string fontWeight{FontWeight::Normal};
            std::string fontStyle{FontStyle::Normal};
            std::string textDecoration{TextDecoration::None};

            void hash(std::ostream& o) const override;
        };

        /// Axis-aligned rectangle
        class LocalRect
            : public Paint::ShapeRect
            , public LocalCommand
        {
        public:
            LocalRect() = default;
        };

        /// Axis-aligned ellipse
        class LocalEllipse
            : public Paint::ShapeEllipse
            , public LocalCommand
        {
        public:
            LocalEllipse() = default;
        };

        /// Arrow with shaft and arrowhead at endPos
        class LocalArrow
            : public Paint::ShapeArrow
            , public LocalCommand
        {
        public:
            LocalArrow() = default;
        };

        /// Straight line with no arrowhead
        class LocalLine
            : public Paint::ShapeLine
            , public LocalCommand
        {
        public:
            LocalLine() = default;
        };

        using Vec4 = TwkMath::Vec4f;
        using Vec3 = TwkMath::Vec3f;
        using Vec2 = TwkMath::Vec2f;
        using Box2 = TwkMath::Box2f;
        using Matrix = TwkMath::Mat44f;
        using Color = TwkMath::Col4f;
        using PointVector = std::vector<Vec2>;
        using FrameVector = std::vector<std::string>;
        using LocalCommands = std::vector<LocalCommand*>;
        using PenMap = std::map<Component*, LocalPolyLine>;
        using TextMap = std::map<Component*, LocalText>;
        using RectMap = std::map<Component*, LocalRect>;
        using EllipseMap = std::map<Component*, LocalEllipse>;
        using ArrowMap = std::map<Component*, LocalArrow>;
        using LineMap = std::map<Component*, LocalLine>;
        using FrameMap = std::map<int, Components>;

        PaintIPNode(const std::string& name, const NodeDefinition* def, IPGraph* graph, GroupIPNode* group);
        virtual ~PaintIPNode();

        IPImage* evaluate(const Context& context) override;

        void propertyChanged(const Property* proprety) override;
        void readCompleted(const std::string& typeName, unsigned int version) override;

    protected:
        void compilePenComponent(Component*);
        void compileTextComponent(Component*);
        void compileRectComponent(Component*);
        void compileEllipseComponent(Component*);
        void compileArrowComponent(Component*);
        void compileLineComponent(Component*);
        void compileFrame(Component*);
        void setPaintEffects();

    private:
        PenMap m_penStrokes;
        TextMap m_texts;
        RectMap m_rects;
        EllipseMap m_ellipses;
        ArrowMap m_arrows;
        LineMap m_lines;
        LocalCommands m_commands;
        Paint::PushFrameBuffer m_pushFBO;
        Paint::PopFrameBuffer m_popFBO;
        FrameMap m_frameMap;
        Component* m_tag;
        std::mutex m_commandsMutex;
        PaintEffects m_paintEffects;
        // TODO: add per-node GL texture cache for QPainter text to avoid per-frame rasterization
    };

} // namespace IPCore

#endif // __IPGraph__PaintIPNode__h__
