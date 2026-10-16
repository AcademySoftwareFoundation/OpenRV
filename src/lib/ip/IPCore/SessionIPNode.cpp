//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/SessionIPNode.h>
#include <IPCore/ImageRenderer.h>
#if defined(PLATFORM_DARWIN)
#include <TwkGLF/GL.h>
#endif

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
        declareProperty<IntProperty>("paintEffects.hold", 0);
        declareProperty<IntProperty>("paintEffects.ghost", 0);
        declareProperty<IntProperty>("paintEffects.ghostBefore", 5);
        declareProperty<IntProperty>("paintEffects.ghostAfter", 5);
        setMaxInputs(0);

#if defined(PLATFORM_DARWIN)
        // Defer until Session::queryAndStoreGLInfo() can make the control
        // device's GL context current (Metal hybrid path at session construction).
        if (!TwkGLF::safeGLGetString(GL_VERSION).empty())
            ImageRenderer::queryGLIntoContainer(this);
#else
        ImageRenderer::queryGLIntoContainer(this);
#endif
    }

    SessionIPNode::~SessionIPNode()
    {
        //
        //  The GroupIPNode will delete the root
        //
    }

} // namespace IPCore
