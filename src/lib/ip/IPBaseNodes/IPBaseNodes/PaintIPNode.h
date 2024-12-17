//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__PaintIPNode__h__
#define __IPGraph__PaintIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>
#include <IPCore/PaintCommand.h>
#include <map>

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

    class PaintIPNode : public IPNode
    {
    public:
        //
        //  Types
        //

        //
        //  Paint:: commands + per-command state local to this node.
        //

        struct LocalPolyLine : public Paint::PolyLine
        {
            int eye;
        };

        struct LocalText : public Paint::Text
        {
            int eye;
        };

        typedef TwkMath::Vec4f Vec4;
        typedef TwkMath::Vec3f Vec3;
        typedef TwkMath::Vec2f Vec2;
        typedef TwkMath::Box2f Box2;
        typedef TwkMath::Mat44f Matrix;
        typedef TwkMath::Col4f Color;
        typedef std::vector<Vec2> PointVector;
        typedef std::vector<std::string> FrameVector;
        typedef std::map<Component*, LocalPolyLine> PenMap;
        typedef std::map<Component*, LocalText> TextMap;
        typedef std::map<int, Components> FrameMap;

        PaintIPNode(const std::string& name, const NodeDefinition* def,
                    IPGraph* graph, GroupIPNode* group);
        virtual ~PaintIPNode();

        virtual IPImage* evaluate(const Context&);

        virtual void propertyChanged(const Property*);
        virtual void readCompleted(const std::string&, unsigned int);

    protected:
        void compilePenComponent(Component*);
        void compileTextComponent(Component*);
        void compileFrame(Component*);

    private:
        PenMap m_penStrokes;
        TextMap m_texts;
        Paint::PushFrameBuffer m_pushFBO;
        Paint::PopFrameBuffer m_popFBO;
        FrameMap m_frameMap;
        Component* m_tag;
    };

} // namespace IPCore

#endif // __IPGraph__PaintIPNode__h__
