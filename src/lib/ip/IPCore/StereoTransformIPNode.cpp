//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/StereoTransformIPNode.h>
// #include <TwkMath/Function.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Attribute.h>
#include <TwkFB/Operations.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;
    using namespace std;

    StereoTransformIPNode::StereoTransformIPNode(const std::string& name,
                                                 const NodeDefinition* def,
                                                 IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_useAttrs(false)
    {
        setMaxInputs(1);
        m_stereoSwap = declareProperty<IntProperty>("stereo.swap", 0);
        m_stereoRelativeOffset =
            declareProperty<FloatProperty>("stereo.relativeOffset", 0.0f);
        m_stereoRightOffset =
            declareProperty<FloatProperty>("stereo.rightOffset", 0.0f);
        m_rightTransformFlip =
            declareProperty<IntProperty>("rightTransform.flip", 0);
        m_rightTransformFlop =
            declareProperty<IntProperty>("rightTransform.flop", 0);
        m_rightTransformRotate =
            declareProperty<FloatProperty>("rightTransform.rotate", 0.0f);
        m_rightTransformTranslate = declareProperty<Vec2fProperty>(
            "rightTransform.translate", Vec2f(0.0));
    }

    StereoTransformIPNode::~StereoTransformIPNode() {}

    //
    //  Modify StereoContext according to property values.
    //

    void StereoTransformIPNode::prepareStereoContext(StereoContext& scontext)
    {
        if (!scontext.active)
            return;

        if (IntProperty* sp = m_stereoSwap)
        {
            if (sp->size())
                scontext.swap = (scontext.swap != bool(sp->front()));
        }

        if (FloatProperty* fp = m_stereoRelativeOffset)
        {
            if (fp->size())
                scontext.offset += fp->front();
        }

        if (FloatProperty* fp = m_stereoRightOffset)
        {
            if (fp->size())
                scontext.roffset += fp->front();
        }

        if (IntProperty* flip = m_rightTransformFlip)
        {
            if (flip->front())
                scontext.flip = !scontext.flip;
        }

        if (IntProperty* flop = m_rightTransformFlop)
        {
            if (flop->front())
                scontext.flop = !scontext.flop;
        }

        if (FloatProperty* rot = m_rightTransformRotate)
        {
            if (rot->size() > 0)
                scontext.rrotate += rot->front();
        }

        if (Vec2fProperty* vp = m_rightTransformTranslate)
        {
            if (vp->size())
                scontext.rtranslate += vp->front();
        }
    }

    IPImage* StereoTransformIPNode::evaluate(const Context& context)
    {
        if (!context.stereo)
            return IPNode::evaluate(context);

        prepareStereoContext(context.stereoContext);

        Context scontext = context;

        contextFromStereoContext(scontext);

        //
        //  Evaluate children
        //

        IPImage* root = IPNode::evaluate(scontext);

        if (!root)
            root = IPImage::newNoImage(this, "No Input");

        return root;
    }

    IPImageID* StereoTransformIPNode::evaluateIdentifier(const Context& context)
    {
        if (!context.stereo)
            return IPNode::evaluateIdentifier(context);

        Context scontext = context;

        contextFromStereoContext(scontext);

        return IPNode::evaluateIdentifier(scontext);
    }

    void StereoTransformIPNode::metaEvaluate(const Context& context,
                                             MetaEvalVisitor& visitor)
    {
        visitor.enter(context, this);

        IPNode* input = (inputs().size()) ? inputs()[0] : 0;
        if (input)
        {
            Context scontext = context;

            contextFromStereoContext(scontext);

            if (visitor.traverseChild(scontext, 0, this, input))
            {
                input->metaEvaluate(scontext, visitor);
            }
        }

        visitor.leave(context, this);
    }

    void
    StereoTransformIPNode::propagateFlushToInputs(const FlushContext& context)
    {
        if (!context.stereo)
        {
            IPNode::propagateFlushToInputs(context);
            return;
        }

        Context scontext = context;

        contextFromStereoContext(scontext);

        IPNode::propagateFlushToInputs(scontext);
    }

} // namespace IPCore
