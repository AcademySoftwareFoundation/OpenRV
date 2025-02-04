//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__OverlayIPNode__h__
#define __IPGraph__OverlayIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/PaintCommand.h>
#include <map>
#include <vector>
#include <IPCore/IPGraph.h>

namespace IPCore
{

    class OverlayIPNode : public IPNode
    {
    public:
        //
        //  Types
        //

        //
        //  Paint:: commands + per-command state local to this node.
        //

        struct LocalRectangle : public Paint::Rectangle
        {
            int eye;
            bool active;
        };

        struct LocalText
        {
            int eye;
            bool active;
            int firstFrame;

            std::vector<Paint::Text> texts;

            Paint::Text& commandForFrame(int frame);
        };

        //
        //  Window components produce list of paint commands per component.
        //
        struct LocalWindow
        {
            int eye;
            int firstFrame;

            bool windowActive;
            bool textActive;
            bool outlineActive;

            std::string outlineBrush;

            //  target line width in "native pixels"
            float outlineWidth;

            float scalingWidth;

            //
            //  FrameCommands hold per-frame sets of commands.
            //
            struct FrameCommands
            {

                bool antialiased;

                // by default outline is drawn in gl as a polygon outline
                // (aliased)
                Paint::Quad plainOutline;
                // for antialiased case outline is implemented as four polylines
                std::vector<Paint::PolyLine> outline;

                std::vector<Paint::Quad> quads;

                // for antialaised case outline is drawn around the frame in the
                // same color
                // as the solid matts as a way to antialiase
                std::vector<Paint::PolyLine> quadsOutline;
            };

            std::vector<FrameCommands> frameCommands;

            FrameCommands& commandsForFrame(int frame);
        };

        typedef std::map<Component*, LocalText> TextMap;
        typedef std::map<Component*, LocalRectangle> RectangleMap;
        typedef std::map<Component*, LocalWindow> WindowMap;

        OverlayIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph* graph, GroupIPNode* group);
        virtual ~OverlayIPNode();

        virtual IPImage* evaluate(const Context&);

        virtual void propertyChanged(const Property*);
        virtual void readCompleted(const std::string&, unsigned int);

    protected:
        void compileWindowComponent(Component*);
        void compileRectangleComponent(Component*);
        void compileTextComponent(Component*);
        bool addMattePaintCommands(IPImage* head);

    private:
        TextMap m_texts;
        RectangleMap m_rectangles;
        WindowMap m_windows;

        bool m_hasSideMattes;
        float m_matteRatio;
        float m_heightVisible;
        float m_imageWidth;
        TwkMath::Vec2f m_centerPoint;

        Paint::Rectangle m_top;
        Paint::Rectangle m_bottom;
        Paint::Rectangle m_left;
        Paint::Rectangle m_right;

        Paint::ExecuteAllBefore m_executeAll;
    };
} // namespace IPCore

#endif // __IPGraph__OverlayIPNode__h__
