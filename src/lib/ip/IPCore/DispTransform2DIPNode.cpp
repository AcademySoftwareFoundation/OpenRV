//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPCore/DispTransform2DIPNode.h>
#include <TwkMath/Function.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>
#include <iomanip>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;
    using namespace std;

    DispTransform2DIPNode::DispTransform2DIPNode(const std::string& name,
                                                 const NodeDefinition* def,
                                                 IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
    {
        setMaxInputs(1);
        setHasLinearTransform(true);
        m_translate =
            declareProperty<Vec2fProperty>("transform.translate", Vec2f(0, 0));
        m_scale =
            declareProperty<Vec2fProperty>("transform.scale", Vec2f(1, 1));

        updateTransformHash();
    }

    DispTransform2DIPNode::~DispTransform2DIPNode() {}

    void DispTransform2DIPNode::outputDisconnect(IPNode* node)
    {
        IPNode::outputDisconnect(node);
    }

    IPNode::Matrix DispTransform2DIPNode::localMatrix(const Context&) const
    {
        Mat44f M;
        Vec2f scale = propertyValue(m_scale, Vec2f(1, 1));
        Vec2f translate = propertyValue(m_translate, Vec2f(0, 0));

        if (translate != Vec2f(0, 0))
        {
            Mat44f T;
            T.makeTranslation(Vec3f(translate.x, translate.y, 0));
            M = T * M;
        }

        if (scale != Vec2f(1, 1))
        {
            Mat44f S;
            S.makeScale(Vec3f(scale.x, scale.y, 1));
            M = S * M;
        }

        return M;
    }

    IPImage* DispTransform2DIPNode::evaluate(const Context& context)
    {
        Matrix M = localMatrix(context);
        Context newContext = context;

        if (IPImage* root = IPNode::evaluate(newContext))
        {
            if (root->destination == IPImage::IntermediateBuffer
                && root->children)
            {
                IPImage* child = root->children;
                for (; child; child = child->next)
                {
                    child->transformMatrix = M * child->transformMatrix;
                }
            }
            else
                root->transformMatrix = M * root->transformMatrix;
            return root;
        }

        return IPImage::newNoImage(this, "No Input");
    }

    IPImageID* DispTransform2DIPNode::evaluateIdentifier(const Context& context)
    {
        Matrix M = localMatrix(context);
        Context newContext = context;
        return IPNode::evaluateIdentifier(newContext);
    }

    size_t DispTransform2DIPNode::m_transformHash = 1;

    void DispTransform2DIPNode::updateTransformHash()
    {
        Vec2f scale = propertyValue(m_scale, Vec2f(1, 1));
        Vec2f translate = propertyValue(m_translate, Vec2f(0, 0));

        double sx = scale.x;
        double sy = scale.y;
        double tx = translate.x;
        double ty = translate.y;

        m_transformHash = *((size_t*)&(sx));
        m_transformHash += *((size_t*)&(sy));
        m_transformHash += *((size_t*)&(tx));
        m_transformHash += *((size_t*)&(ty));
    }

    void DispTransform2DIPNode::readCompleted(const std::string& type,
                                              unsigned int version)
    {
        updateTransformHash();
        IPNode::readCompleted(type, version);
    }

    void DispTransform2DIPNode::propertyChanged(const Property* p)
    {
        if (p == m_translate || p == m_scale)
            updateTransformHash();

        IPNode::propertyChanged(p);
    }

} // namespace IPCore
