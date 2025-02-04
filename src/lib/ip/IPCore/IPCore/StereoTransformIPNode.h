//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__StereoTransformIPNode__h__
#define __IPCore__StereoTransformIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <algorithm>

namespace IPCore
{

    //
    //  class StereoTransformIPNode
    //
    //  Converts inputs into formats that can be used by the
    //  ImageRenderer. This is a base class for the DisplayStereoTransformIPNode
    //  and the SourceStereoTransformIPNode.
    //

    class StereoTransformIPNode : public IPNode
    {
    public:
        StereoTransformIPNode(const std::string& name,
                              const NodeDefinition* def, IPGraph* graph,
                              GroupIPNode* group = 0);

        virtual ~StereoTransformIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual void propagateFlushToInputs(const FlushContext&);

    protected:
        //
        //  Set up StereoContext object from node's property values.
        //
        void prepareStereoContext(StereoContext& scontext);

        //
        //  Make adjustments in evaluation Context driven by Context's
        //  StereoContext member.
        //
        virtual void contextFromStereoContext(Context&) {}

        bool m_useAttrs;

    protected:
        IntProperty* m_stereoSwap;
        FloatProperty* m_stereoRelativeOffset;
        FloatProperty* m_stereoRightOffset;
        IntProperty* m_rightTransformFlip;
        IntProperty* m_rightTransformFlop;
        FloatProperty* m_rightTransformRotate;
        Vec2fProperty* m_rightTransformTranslate;
        static std::string m_defaultType;
    };

} // namespace IPCore

#endif // __IPCore__StereoTransformIPNode__h__
