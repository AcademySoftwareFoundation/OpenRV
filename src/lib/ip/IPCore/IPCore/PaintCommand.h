//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPCore__PaintCommand__h__
#define __IPCore__PaintCommand__h__
#include <TwkMath/Mat44.h>
#include <TwkMath/Color.h>
#include <TwkMath/Box.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkPaint/Path.h>
#include <IPCore/IPImage.h>
#include <vector>

namespace IPCore
{

    namespace Paint
    {

        typedef TwkGLF::GLFBO GLFBO;
        typedef TwkGLF::GLState GLState;
        typedef TwkGLF::PrimitiveData PrimitiveData;
        typedef TwkGLF::VertexAttribute VertexAttribute;

        class Brush
        {
        public:
            typedef TwkFB::FrameBuffer FrameBuffer;

            Brush(const std::string& name, FrameBuffer* fb, bool radial)
                : m_fb(fb)
                , m_name(name)
                , m_radial(radial)
                , m_id(-1)
            {
            }

            const FrameBuffer* fb() const { return m_fb; }

            unsigned int id() const { return m_id; }

            const std::string& name() const { return m_name; }

        private:
            FrameBuffer* m_fb;
            std::string m_name;
            bool m_radial;
            unsigned int m_id;
            friend class DeclareBrushes;
        };

        class Font
        {
        public:
            Font(const std::string& name)
                : m_name(name)
                , m_initialized(false)
            {
            }

            const std::string& name() const { return m_name; }

            void init() const;

            bool initialized() const { return m_initialized; }

            void setSize(unsigned int s) const;
            void setColor(float, float, float, float) const;
            std::pair<float, float> computeText(const std::string&, std::string&, float space, std::string origin) const;
            float globalAscenderHeight() const;
            float globalDescenderHeight() const;

        private:
            std::string m_name;
            mutable bool m_initialized;
            friend class DeclareFonts;
        };

        class CommandContext
        {
        public:
            explicit CommandContext(TwkMath::Mat44f pid, TwkMath::Mat44f m, const GLFBO* i, const GLFBO* t, const GLFBO* c, GLState*& g,
                                    bool hasSten, TwkMath::Vec4f sten = TwkMath::Vec4f(0.0))
            {
                hasStencil = hasSten;
                stencilBox = sten;
                projMatrix = pid;
                modelviewMatrix = m;
                initialRender = i;
                currentTexture = t;
                currentRender = c;
                glState = g;
            }

            ~CommandContext() {}

            bool hasStencil;
            TwkMath::Vec4f stencilBox;

            TwkMath::Mat44f projMatrix;
            TwkMath::Mat44f modelviewMatrix;
            const GLFBO* initialRender;
            const GLFBO* currentTexture;
            const GLFBO* currentRender;
            GLState* glState;
        };

        class Command
        {
        public:
            using Vec4 = TwkMath::Vec4f;
            using Vec3 = TwkMath::Vec3f;
            using Vec2 = TwkMath::Vec2f;
            using Box2 = TwkMath::Box2f;
            using Matrix = TwkMath::Mat44f;
            using Color = TwkMath::Col4f;
            using Path = TwkPaint::Path;

            enum CommandType
            {
                PolyLine,
                Text,
                PushFrameBuffer,
                PopFrameBuffer,
                Rectangle,
                Quad,
                ExecuteAllBefore
            };

            float offset;
            unsigned int version;
            Color color;

            virtual void execute(CommandContext& context) const = 0;
            virtual void hash(std::ostream& ostream) const = 0;
            [[nodiscard]] virtual size_t getType() const = 0;

            Command()
                : offset(0)
                , version(3)
                , color(Color(0.0))
            {
            }

            Command(const Command& command) = default;

            virtual ~Command() = default;
        };

        class PolyLine : public Command
        {
        public:
            using HashValue = unsigned int;

            enum Mode
            {
                OverMode,
                EraseMode,
                ScaleMode,
                GradientScaleMode,
                CloneMode,
                TessellateMode // each triangle can have its own color
            };

            explicit PolyLine(const Vec2* vector2d = nullptr, size_t npoints = 0, float width = 0, Color color = Color(0.0),
                              bool ownPoints = false)
                : npoints(npoints)
                , width(width)
                , smoothingWidth(1.0)
                , ownPoints(ownPoints)
                , debug(0)
                , join(Path::RoundJoin)
                , cap(Path::SquareCap)
                , mode(OverMode)
                , splat(false)
                , built(false)
                , idhash(0)
            {
                color = color;

                if (ownPoints)
                {
                    points.assign(vector2d->begin(), vector2d->end());
                }
                else
                {
                    points = {};
                }
            }

            PolyLine(const PolyLine& polyLine)
                : Command(polyLine)
                , width(polyLine.width)
                , brush(polyLine.brush)
                , debug(polyLine.debug)
                , join(polyLine.join)
                , cap(polyLine.cap)
                , mode(polyLine.mode)
                , splat(polyLine.splat)
                , smoothingWidth(polyLine.smoothingWidth)
                , built(polyLine.built)
            {
                if (polyLine.npoints > 0)
                {
                    npoints = polyLine.npoints;
                    points.assign(polyLine.points.begin(), polyLine.points.end());
                }
                else
                {
                    npoints = 0;
                    points = {};
                }

                if (polyLine.width != 0.0f)
                {
                    widths.assign(polyLine.widths.begin(), polyLine.widths.end());
                }
                else
                {
                    widths = {};
                }

                ownPoints = true;
            }

            virtual ~PolyLine() = default;

            std::vector<Vec2> points{};
            size_t npoints;
            float width;
            float smoothingWidth;
            bool splat;
            std::vector<float> widths{};
            std::string brush;
            TwkPaint::Path::JoinStyle join;
            TwkPaint::Path::CapStyle cap;
            Mode mode;
            int debug;
            bool ownPoints;

            mutable HashValue idhash;

            // the following 4 are computed / setup right after the geometry is
            // computed they only need to be recomputed if the geometry has
            // changed
            mutable PrimitiveData primitives;
            mutable std::vector<float> primitiveData;
            mutable std::vector<unsigned int> primitiveIndices;
            mutable std::vector<VertexAttribute> primitiveAttributes;

            //
            mutable Path path;
            mutable bool built;

            void execute(CommandContext& context) const override;
            void executeOldOverMode(CommandContext& context) const;

            void hash(std::ostream& ostream) const override;
            virtual HashValue hashValue() const;

            void build() const;

            size_t getType() const override;
        };

        class Text : public Command
        {
        public:
            explicit Text(const std::string& /*str*/ = "", const std::string& /*fnt*/ = "", float ptsze = 1.0,
                          Color color = Color(1, 1, 1, 1))
                : ptsize(ptsze)
                , scale(1.0)
                , rotation(0.0)
                , spacing(1.0)
                , pos(0, 0)
            {
                color = color;
            }

            Text(const Text& text) = default;

            Vec2 pos;
            std::string text;
            std::string font;
            std::string origin;
            float ptsize;
            float scale;
            float rotation;
            float spacing;

            void execute(CommandContext& context) const override;
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;

            static void setup(CommandContext&);
            static void cleanup(CommandContext&);
        };

        class PushFrameBuffer : public Command
        {
        public:
            PushFrameBuffer();
            void execute(CommandContext& context) const override;
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class PopFrameBuffer : public Command
        {
        public:
            PopFrameBuffer();
            void execute(CommandContext& context) const override;
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class Rectangle : public Command
        {
        public:
            explicit Rectangle(float height = 0, float width = 0, Color color = Color(0, 0, 0, 1.0))
                : height(height)
                , width(width)
                , pos(0, 0)
            {
                color = color;
            }

            Rectangle(const Rectangle& rectangle) = default;

            Vec2 pos;
            float height;
            float width;

            void execute(CommandContext&) const override;
            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class Quad : public Command
        {
        public:
            enum DrawMode
            {
                SolidMode,
                OutlineMode
            };

            Quad() = default;

            Quad(const Quad& quad)
                : drawMode(quad.drawMode)
            {
                points[0] = quad.points[0];
                points[1] = quad.points[1];
                points[2] = quad.points[2];
                points[3] = quad.points[3];
            }

            Vec2 points[4];
            DrawMode drawMode;

            void execute(CommandContext&) const override;
            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class ExecuteAllBefore : public Command
        {
        public:
            ExecuteAllBefore() = default;

            void execute(CommandContext& context) const override;
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

        struct PaintContext
        {
            PaintContext()
                : initialRender(0)
                , tempRender1(0)
                , tempRender2(0)
                , cachedfbo(0)
                , image(0)
                , hasStencil(false)
                , updateCache(true)
                , cacheUpdated(false)
                , lastCommand(0)
                , commandExecuted(0)
                , totalCommandsInCache(0)
            {
            }

            const GLFBO* initialRender;
            const GLFBO* tempRender1;
            const GLFBO* tempRender2;
            const GLFBO* cachedfbo;
            const IPImage* image;
            bool hasStencil;
            TwkMath::Vec4f stencilBox;
            bool updateCache;
            bool cacheUpdated;
            const Paint::Command* lastCommand;
            IPImage::PaintCommands commands;
            GLState* glState;
            size_t commandExecuted;
            size_t totalCommandsInCache;
        };

        void renderPaintCommands(PaintContext&);

    } // namespace Paint

} // namespace IPCore

#endif // __IPCore__PaintCommand__h__
