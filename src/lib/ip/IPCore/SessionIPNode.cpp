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

    SessionIPNode::SessionIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* graph,
                                 GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setWritable(false); // special case for this one in the writer
        declareProperty<IntProperty>("matte.show", 0);
        declareProperty<FloatProperty>("matte.aspect", 1.33f);
        declareProperty<FloatProperty>("matte.opacity", 0.66f);
        declareProperty<FloatProperty>("matte.heightVisible", -1.0);
        declareProperty<Vec2fProperty>("matte.centerPoint", Vec2f(0.0, 0.0));
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
