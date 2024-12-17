//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/SourceStereoIPNode.h>
#include <TwkMath/Function.h>

namespace IPCore
{

    using namespace std;
    using namespace TwkMath;

    SourceStereoIPNode::SourceStereoIPNode(const std::string& name,
                                           const NodeDefinition* def,
                                           IPGraph* graph, GroupIPNode* group)
        : StereoTransformIPNode(name, def, graph, group)
    {
        m_useAttrs = true;
    }

    SourceStereoIPNode::~SourceStereoIPNode() {}

    namespace
    {

        void prepareForStereo(IPImage* img, bool isLeftEye, Mat44f& Mr,
                              float offset, float roffset)
        {
            if (!isLeftEye)
            {
                img->transformMatrix = Mr * img->transformMatrix;
            }

            if (offset != 0.0f || roffset != 0.0f)
            {
                float aspect = img->displayAspect();
                Mat44f T;

                if (isLeftEye)
                {
                    T.makeTranslation(Vec3f(-offset * aspect, 0, 0));
                }
                else
                {
                    const float off = offset + roffset;
                    T.makeTranslation(Vec3f(off * aspect, 0, 0));
                }

                img->transformMatrix = img->transformMatrix * T;
            }
        }

        //
        //  Apply StereoContext contents to IPImage matrices.
        //

        void applyStereoContext(bool isLeft, IPNode::StereoContext& sc,
                                IPImage* img)
        {
            if (!sc.active)
                return;

            Mat44f Mr;

            //
            //  The order here is important. We want to do all scaling
            //  followed by rotation followed by translation.
            //

            if (sc.flip)
            {
                Mat44f S;
                S.makeScale(Vec3f(1, -1, 1));
                Mr *= S;
            }

            if (sc.flop)
            {
                Mat44f S;
                S.makeScale(Vec3f(-1, 1, 1));
                Mr *= S;
            }

            if (sc.rrotate != 0.0)
            {
                Mat44f R;
                R.makeRotation(Vec3f(0, 0, -1), degToRad(sc.rrotate));
                Mr = R * Mr;
            }

            if (sc.rtranslate != Vec2f(0.0, 0.0))
            {
                Mat44f T;
                T.makeTranslation(Vec3f(sc.rtranslate.x, sc.rtranslate.y, 0.0));
                Mr = T * Mr;
            }

            prepareForStereo(img, isLeft, Mr, sc.offset, sc.roffset);
        }

    } // namespace

    void SourceStereoIPNode::contextFromStereoContext(Context& c)
    {
        //
        //  If swapping, we change nothing but the eye.  IE offsets, etc all
        //  stay the same, we just ask for different images in the eyes.
        //

        if (c.stereoContext.active && c.stereoContext.swap)
        {
            if (c.eye == 0)
                c.eye = 1;
            else if (c.eye == 1)
                c.eye = 0;
        }
    }

    IPImage* SourceStereoIPNode::evaluate(const Context& context)
    {
        if (!context.stereo)
            return IPNode::evaluate(context);

        IPImage* root = StereoTransformIPNode::evaluate(context);

        if (!root)
            root = IPImage::newNoImage(this, "No Input");

        applyStereoContext(context.eye == 0, context.stereoContext, root);

        return root;
    }

} // namespace IPCore
