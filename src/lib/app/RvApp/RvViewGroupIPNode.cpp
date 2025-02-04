//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#include <RvApp/RvViewGroupIPNode.h>
#include <TwkDeploy/Deploy.h>

// RV third party optional customization
#if defined(RV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION)
extern IPCore::IPImage*
rvViewGroupThirdPartyCustomization(IPCore::IPNode* node,
                                   const IPCore::IPNode::Context& context);
#endif

namespace Rv
{

    IPImage* RvViewGroupIPNode::evaluate(const Context& context)
    {
#if defined(RV_VIEW_GROUP_THIRD_PARTY_CUSTOMIZATION)
        IPImage* img = rvViewGroupThirdPartyCustomization(this, context);
        if (img)
            return img;
#endif

        return ViewGroupIPNode::evaluate(context);
    }

    RvViewGroupIPNode::~RvViewGroupIPNode() {}

} // namespace Rv
