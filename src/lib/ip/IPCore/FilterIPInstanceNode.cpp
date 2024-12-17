//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/FilterIPInstanceNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderFunction.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderUtil.h>
#include <TwkMath/Function.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    FilterIPInstanceNode::FilterIPInstanceNode(const std::string& name,
                                               const NodeDefinition* def,
                                               IPGraph* g, GroupIPNode* group)
        : IPInstanceNode(name, def, g, group)
    {
        const Shader::Function* F = def->function();
        assert(F->isFilter());
    }

    FilterIPInstanceNode::~FilterIPInstanceNode() {}

    IPImage* FilterIPInstanceNode::evaluate(const Context& context)
    {
        const Shader::Function* F = definition()->function();
        if (!isActive())
            return IPInstanceNode::evaluate(context);

        IPNodes nodes = inputs();
        int ninputs = nodes.size();
        if (!ninputs)
            return IPImage::newNoImage(this, "No Input");

        Context c = context;

        //
        //  NOTE: evaluate() may throw but that's ok (nothing to clean up)
        //

        IPImage* current = nodes[0]->evaluate(c);

        if (!current)
        {
            TWK_THROW_STREAM(EvaluationFailedExc,
                             "FilterIPInstanceNode evaluation failed on node "
                                 << nodes[0]->name());
        }

        IPImageVector images(1);
        IPImageSet modifiedImages;
        Shader::ExpressionVector inExpressions;
        images[0] = current;

        convertBlendRenderTypeToIntermediate(images, modifiedImages);

        balanceResourceUsage(IPNode::filterAccumulate, images, modifiedImages,
                             8, 8, 81);

        IPImage* root = new IPImage(
            this, IPImage::MergeRenderType, current->width, current->height,
            1.0, IPImage::IntermediateBuffer, IPImage::FloatDataType);

        //
        //  Copy the transform over to our image and remove the
        //  existing one
        //

        root->transformMatrix = current->transformMatrix;
        images[0]->transformMatrix = Matrix();

        //
        //  Prep merge
        //

        assembleMergeExpressions(root, images, modifiedImages, true,
                                 inExpressions);

        //
        //  Assemble shaders and add the children
        //

        root->mergeExpr = bind(root, inExpressions, context);
        root->shaderExpr = Shader::newSourceRGBA(root);
        root->appendChild(images[0]);
        root->recordResourceUsage();

        return root;
    }

} // namespace IPCore
