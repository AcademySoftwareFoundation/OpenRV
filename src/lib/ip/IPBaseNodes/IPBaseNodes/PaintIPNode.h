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
#include <map>
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
        };

        class LocalText
            : public Paint::Text
            , public LocalCommand
        {
        public:
            LocalText() = default;
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
        using FrameMap = std::map<int, Components>;

        PaintIPNode(const std::string& name, const NodeDefinition* def,
                    IPGraph* graph, GroupIPNode* group);
        virtual ~PaintIPNode();

        IPImage* evaluate(const Context& context) override;

        void propertyChanged(const Property* proprety) override;
        void readCompleted(const std::string& typeName,
                           unsigned int version) override;

    protected:
        void compilePenComponent(Component*);
        void compileTextComponent(Component*);
        void compileFrame(Component*);
        void setPaintEffects();

    private:
        PenMap m_penStrokes;
        TextMap m_texts;
        LocalCommands m_commands;
        Paint::PushFrameBuffer m_pushFBO;
        Paint::PopFrameBuffer m_popFBO;
        FrameMap m_frameMap;
        Component* m_tag;
        std::mutex m_commandsMutex;
        PaintEffects m_paintEffects;
    };

} // namespace IPCore

#endif // __IPGraph__PaintIPNode__h__
