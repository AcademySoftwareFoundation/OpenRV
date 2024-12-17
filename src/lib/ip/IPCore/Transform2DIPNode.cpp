//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/Transform2DIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderUtil.h>
#include <IPCore/NodeDefinition.h>
#include <TwkContainer/Properties.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <IPBaseNodes/SwitchIPNode.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>
#include <cmath>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;
    using namespace std;

    Transform2DIPNode::Transform2DIPNode(const std::string& name,
                                         const NodeDefinition* def, IPGraph* g,
                                         GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_adaptiveResampling(false)
        , m_visibleBox(0)
        , m_leftVisibleBox(0)
        , m_rightVisibleBox(0)
        , m_topVisibleBox(0)
        , m_bottomVisibleBox(0)
    {
        setMaxInputs(1);
        setHasLinearTransform(true);

        const bool newStyleVisibleBox =
            def->intValue("defaults.newStyleVisibleBox", 0) != 0;

        IntProperty* ip;

        m_flip = declareProperty<IntProperty>("transform.flip", 0);
        m_flop = declareProperty<IntProperty>("transform.flop", 0);
        m_rotate = declareProperty<FloatProperty>("transform.rotate", 0.0f);
        m_translate =
            declareProperty<Vec2fProperty>("transform.translate", Vec2f(0, 0));
        m_scale =
            declareProperty<Vec2fProperty>("transform.scale", Vec2f(1, 1));
        m_active = declareProperty<IntProperty>("transform.active", 1);

        if (newStyleVisibleBox)
        {
            m_leftVisibleBox =
                declareProperty<FloatProperty>("visibleBox.left", 0.0f);
            m_rightVisibleBox =
                declareProperty<FloatProperty>("visibleBox.right", 1.0f);
            m_bottomVisibleBox =
                declareProperty<FloatProperty>("visibleBox.bottom", 0.0f);
            m_topVisibleBox =
                declareProperty<FloatProperty>("visibleBox.top", 1.0f);
        }
        else
        {
            m_visibleBox = createProperty<FloatProperty>("stencil.visibleBox");
            m_visibleBox->resize(4);
            (*m_visibleBox)[0] = 0;
            (*m_visibleBox)[1] = 1;
            (*m_visibleBox)[2] = 0;
            (*m_visibleBox)[3] = 1;
        }

        m_tag = createComponent("tag");
    }

    Transform2DIPNode::~Transform2DIPNode() {}

    void Transform2DIPNode::setFlip(bool b) { setProperty(m_flip, b ? 1 : 0); }

    void Transform2DIPNode::setFlop(bool b) { setProperty(m_flop, b ? 1 : 0); }

    IPNode::Matrix Transform2DIPNode::localMatrix(const Context& context) const
    {
        //
        //  The order here is important. We want to do all scaling
        //  followed by rotation followed by translation.
        //

        Matrix M;

        if (propertyValue(m_flip, 0) == 1)
        {
            Mat44f S;
            S.makeScale(Vec3f(1, -1, 1));
            M *= S;
        }

        if (propertyValue(m_flop, 0) == 1)
        {
            Mat44f S;
            S.makeScale(Vec3f(-1, 1, 1));
            M *= S;
        }

        Vec2f scale = propertyValue(m_scale, Vec2f(1, 1));
        float rot = propertyValue(m_rotate, 0.0f);
        Vec2f translate = propertyValue(m_translate, Vec2f(0, 0));

        if (scale != Vec2f(1, 1))
        {
            Mat44f S;
            S.makeScale(Vec3f(scale.x, scale.y, 1));
            M = S * M;
        }

        if (rot != 0.0f)
        {
            Mat44f R;
            R.makeRotation(Vec3f(0, 0, -1), degToRad(rot));
            M = R * M;
        }

        if (translate != Vec2f(0, 0))
        {
            Mat44f T;
            T.makeTranslation(Vec3f(translate.x, translate.y, 0));
            M = T * M;
        }

        return M;
    }

    IPImage* Transform2DIPNode::evaluate(const Context& context)
    {
        if (m_active && !m_active->front())
        {
            IPImage* head = IPNode::evaluate(context);
            if (!head)
                return IPImage::newNoImage(this, "No Input");
            return head;
        }

        Matrix M = localMatrix(context);

        //
        //  Transform2DIPNode still handles the visible box for wipes,
        //  etc.
        //

        float x0 = 0.0f, y0 = 0.0f, x1 = 1.0f, y1 = 1.0f;

        if (FloatProperty* fp = m_visibleBox)
        {
            if (fp->size() == 4)
            {
                x0 = (*fp)[0];
                x1 = (*fp)[1];
                y0 = (*fp)[2];
                y1 = (*fp)[3];
            }
        }
        else if (m_leftVisibleBox)
        {
            x0 = propertyValue(m_leftVisibleBox, 0.0f);
            x1 = propertyValue(m_rightVisibleBox, 1.0f);
            y0 = propertyValue(m_bottomVisibleBox, 0.0f);
            y1 = propertyValue(m_topVisibleBox, 1.0f);
        }

        const bool stencilBox =
            x0 > 0.0f || x1 < 1.0f || y0 > 0.0f || y1 < 1.0f;

        //
        //  Create a new context with our matrix in there
        //

        Context newContext = context;
        IPImage* root = IPNode::evaluate(newContext);
        if (!root)
            return IPImage::newNoImage(this, "No Input");

        //
        //  Any tags that the UI has created are added to the image
        //

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
                        root->tagMap[sp->name()] = sp->front();
                    }
                }
            }
        }

        if (root->destination == IPImage::IntermediateBuffer)
        {
            for (IPImage* child = root->children; child; child = child->next)
            {
                if (root->children->width >= root->width
                    || root->children->height >= root->height)
                {
                    child->transformMatrix = M * child->transformMatrix;
                }
            }
        }
        else
        {
            root->transformMatrix = M * root->transformMatrix;
        }

        if (stencilBox)
        {
            root->stencilBox.min = Vec2f(x0, y0);
            root->stencilBox.max = Vec2f(x1, y1);

            // Propagate the stencil to the source group if separated by a
            // switch node (MMR)
            if (dynamic_cast<const SwitchIPNode*>(root->node))
            {
                for (IPImage* child = root->children; child;
                     child = child->next)
                {
                    child->stencilBox.min = root->stencilBox.min;
                    child->stencilBox.max = root->stencilBox.max;
                }
            }
        }
        else
        {
            root->stencilBox.makeEmpty();
        }

        return root;
    }

    IPImageID* Transform2DIPNode::evaluateIdentifier(const Context& context)
    {
        if (m_active && !m_active->front())
        {
            return IPNode::evaluateIdentifier(context);
        }

        Matrix M = localMatrix(context);
        Context newContext = context;
        return IPNode::evaluateIdentifier(newContext);
    }

} // namespace IPCore
