//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ColorIPInstanceNode.h>
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

    ColorIPInstanceNode::ColorIPInstanceNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* g, GroupIPNode* group)
        : IPInstanceNode(name, def, g, group)
    {
        const Shader::Function* F = def->function();
    }

    ColorIPInstanceNode::~ColorIPInstanceNode() {}

    IPImage* ColorIPInstanceNode::evaluate(const Context& context)
    {
        const Shader::Function* F = definition()->function();

        IPNodes nodes = inputs();
        int ninputs = nodes.size();
        if (!ninputs)
            return IPImage::newNoImage(this, "No Input");
        if (!isActive())
            return IPInstanceNode::evaluate(context);

        Context c = context;
        IPImage* current = nodes[0]->evaluate(c);

        if (!current)
        {
            TWK_THROW_STREAM(EvaluationFailedExc,
                             "ColorIPInstanceNode evaluation failed on node "
                                 << nodes[0]->name());
        }

        if (IntProperty* active = property<IntProperty>("color", "active"))
        {
            if (active->front() == 0)
                return current;
        }

        convertBlendRenderTypeToIntermediate(current);

        if (current->shaderExpr)
        {
            current->shaderExpr = bind(current, current->shaderExpr, context);
        }
        else if (current->mergeExpr)
        {
            current->mergeExpr = bind(current, current->mergeExpr, context);
        }

        current->recordResourceUsage();
        return current;
    }

} // namespace IPCore
