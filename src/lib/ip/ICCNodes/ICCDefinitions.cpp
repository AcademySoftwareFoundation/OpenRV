//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <ICCNodes/ICCDefinitions.h>
#include <IPCore/NodeManager.h>
#include <ICCNodes/ICCIPNode.h>

namespace IPCore
{
    using namespace std;
    using namespace IPCore;

    typedef TwkContainer::StringProperty StringProperty;
    typedef TwkContainer::IntProperty IntProperty;

    void addICCNodeDefinitions(NodeManager* m)
    {
        NodeDefinition::ByteVector emptyIcon;

        {
            NodeDefinition* def = new NodeDefinition(
                "ICCDisplayTransform", 1, false, "icc", newIPNode<ICCIPNode>,
                "", "", emptyIcon, true);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "ICCLinearizeTransform", 1, false, "icc", newIPNode<ICCIPNode>,
                "", "", emptyIcon, true);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "ICCTransform", 1, false, "icc", newIPNode<ICCIPNode>, "", "",
                emptyIcon, true);

            m->addDefinition(def);
        }
    }

} // namespace IPCore
