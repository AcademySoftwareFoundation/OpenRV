//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/SessionIPNode.h>
#include <IPCore/ImageRenderer.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    SessionIPNode::SessionIPNode(const std::string& name, const NodeDefinition* def, IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setWritable(false); // special case for this one in the writer
        declareProperty<IntProperty>("matte.show", 0);
        declareProperty<FloatProperty>("matte.aspect", 1.33f);
        declareProperty<FloatProperty>("matte.opacity", 0.66f);
        declareProperty<FloatProperty>("matte.heightVisible", -1.0);
        declareProperty<Vec2fProperty>("matte.centerPoint", Vec2f(0.0, 0.0));
        // When 0 (default), the matte (and other overlay geometry such as
        // HUD rectangles/text/windows) does not rotate with the image when
        // the user applies an arbitrary rotation via
        // #RVTransform2D.transform.rotate. The matte still follows any
        // user-applied scale and translate so that pan/zoom review still
        // works normally. Setting this to 1 restores the legacy behavior
        // where all overlays rotate with the image.
        declareProperty<IntProperty>("matte.rotateWithImage", 0);
        declareProperty<IntProperty>("paintEffects.hold", 0);
        declareProperty<IntProperty>("paintEffects.ghost", 0);
        declareProperty<IntProperty>("paintEffects.ghostBefore", 5);
        declareProperty<IntProperty>("paintEffects.ghostAfter", 5);
        setMaxInputs(0);

        ImageRenderer::queryGLIntoContainer(this);
    }

    SessionIPNode::~SessionIPNode()
    {
        //
        //  The GroupIPNode will delete the root
        //
    }

} // namespace IPCore
