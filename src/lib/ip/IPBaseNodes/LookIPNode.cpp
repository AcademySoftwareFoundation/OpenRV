//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/LookIPNode.h>
#include <IPCore/Exception.h>
#include <TwkMath/Function.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    LookIPNode::LookIPNode(const std::string& name, const NodeDefinition* def,
                           IPGraph* g, GroupIPNode* group)
        : LUTIPNode(name, def, g, group)
    {
    }

    LookIPNode::~LookIPNode() {}

    IPImage* LookIPNode::evaluate(const Context& context)
    {
        IPImage* head = IPNode::evaluate(context);

        if (!head)
            return IPImage::newNoImage(this, "No Input");

        //
        //  If input to this node is a blend, prepare img for shaderExpr mods,
        //  etc.
        //
        convertBlendRenderTypeToIntermediate(head);

        addLUTPipeline(context, head);

        return head;
    }

} // namespace IPCore
