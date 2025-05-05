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
            std::pair<float, float> computeText(const std::string&,
                                                std::string&, float space,
                                                std::string origin) const;
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
            explicit CommandContext(TwkMath::Mat44f pid, TwkMath::Mat44f m,
                                    const GLFBO* i, const GLFBO* t,
                                    const GLFBO* c, GLState*& g, bool hasSten,
                                    TwkMath::Vec4f sten = TwkMath::Vec4f(0.0))
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
            typedef TwkMath::Vec4f Vec4;
            typedef TwkMath::Vec3f Vec3;
            typedef TwkMath::Vec2f Vec2;
            typedef TwkMath::Box2f Box2;
            typedef TwkMath::Mat44f Matrix;
            typedef TwkMath::Col4f Color;
            typedef TwkPaint::Path Path;

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
            bool deleteAfterRender;

            Color color;

            virtual void execute(CommandContext&) const = 0;
            virtual void hash(std::ostream&) const = 0;
            [[nodiscard]] virtual size_t getType() const = 0;
            [[nodiscard]] virtual Command* clone() const = 0;

            Command()
                : offset(0)
                , version(3)
                , deleteAfterRender(false)
                , color(Color(0.0))
            {
            }

            Command(const Command& rhs) = default;

            virtual ~Command() = default;
        };

        class PolyLine : public Command
        {
        public:
            typedef unsigned int HashValue;

            enum Mode
            {
                OverMode,
                EraseMode,
                ScaleMode,
                GradientScaleMode,
                CloneMode,
                TessellateMode // each triangle can have its own color
            };

            explicit PolyLine(const Vec2* v = nullptr, size_t n = 0,
                              float w = 0, Color c = Color(0.0), bool o = false)
                : npoints(n)
                , width(w)
                , smoothingWidth(1.0)
                , ownPoints(o)
                , debug(0)
                , join(Path::RoundJoin)
                , cap(Path::SquareCap)
                , mode(OverMode)
                , widths(nullptr)
                , splat(false)
                , built(false)
                , idhash(0)
            {
                color = c;

                if (ownPoints)
                {
                    points = new Vec2[n];
                    memcpy((void*)points, (void*)v, n * sizeof(Vec2));
                }
                else
                {
                    points = v;
                }
            }

            PolyLine(const PolyLine& rhs)
                : Command(rhs)
                , width(rhs.width)
                , brush(rhs.brush)
                , debug(rhs.debug)
                , join(rhs.join)
                , cap(rhs.cap)
                , mode(rhs.mode)
                , splat(rhs.splat)
                , smoothingWidth(rhs.smoothingWidth)
                , built(rhs.built)
            {
                if (rhs.npoints > 0)
                {
                    npoints = rhs.npoints;
                    points = new Vec2[npoints];
                    memcpy((void*)points, (void*)rhs.points,
                           npoints * sizeof(Vec2));
                }
                else
                {
                    npoints = 0;
                    points = nullptr;
                }

                if (rhs.width != 0.0f)
                {
                    widths = new float[npoints];
                    memcpy((void*)widths, (void*)rhs.widths,
                           npoints * sizeof(float));
                }
                else
                {
                    widths = nullptr;
                }

                ownPoints = true;
            }

            virtual ~PolyLine()
            {
                if (ownPoints)
                {
                    delete[] points;
                    points = nullptr;
                    delete[] widths;
                    widths = nullptr;
                }
            }

            const Vec2* points;
            size_t npoints;
            float width;
            float smoothingWidth;
            bool splat;
            const float* widths;
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

            Command* clone() const override { return new PolyLine(*this); }
        };

        class Text : public Command
        {
        public:
            explicit Text(const std::string& /*str*/ = "",
                          const std::string& /*fnt*/ = "", float ptsze = 1.0,
                          Color c = Color(1, 1, 1, 1))
                : ptsize(ptsze)
                , scale(1.0)
                , rotation(0.0)
                , spacing(1.0)
                , pos(0, 0)
            {
                color = c;
            }

            Text(const Text& rhs) = default;

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

            [[nodiscard]] Command* clone() const override
            {
                return new Text(*this);
            }

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

            [[nodiscard]] Command* clone() const override
            {
                return new PushFrameBuffer(*this);
            }
        };

        class PopFrameBuffer : public Command
        {
        public:
            PopFrameBuffer();
            void execute(CommandContext& context) const override;
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;

            [[nodiscard]] Command* clone() const override
            {
                return new PopFrameBuffer(*this);
            }
        };

        class Rectangle : public Command
        {
        public:
            explicit Rectangle(float h = 0, float w = 0,
                               Color c = Color(0, 0, 0, 1.0))
                : height(h)
                , width(w)
                , pos(0, 0)
            {
                color = c;
            }

            Rectangle(const Rectangle& rhs) = default;

            Vec2 pos;
            float height;
            float width;

            void execute(CommandContext&) const override;
            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;

            [[nodiscard]] Rectangle* clone() const override
            {
                return new Rectangle(*this);
            }
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

            Quad(const Quad& rhs)
                : drawMode(rhs.drawMode)
            {
                points[0] = rhs.points[0];
                points[1] = rhs.points[1];
                points[2] = rhs.points[2];
                points[3] = rhs.points[3];
            }

            Vec2 points[4];
            DrawMode drawMode;

            void execute(CommandContext&) const override;
            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;

            [[nodiscard]] Quad* clone() const override
            {
                return new Quad(*this);
            }
        };

        class ExecuteAllBefore : public Command
        {
        public:
            ExecuteAllBefore() = default;

            void execute(CommandContext&) const override;
            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;

            [[nodiscard]] Command* clone() const override
            {
                return new ExecuteAllBefore(*this);
            }
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
        };

        void renderPaintCommands(PaintContext&);

    } // namespace Paint

} // namespace IPCore

#endif // __IPCore__PaintCommand__h__
