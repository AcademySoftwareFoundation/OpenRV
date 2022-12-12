//
//  Copyright (c) 2010 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//

#include <RvApp/RvViewGroupIPNode.h>
#include <TwkDeploy/Deploy.h>

namespace Rv {


IPImage* 
RvViewGroupIPNode::evaluate(const Context& context)
{
    if (TWK_DEPLOY_GET_LICENSE_STATE() < 1)
    {
        return IPImage::newNoImage((inputs().size()) ? inputs()[0] : this, "Unlicensed");
    }
    else
    {
        return ViewGroupIPNode::evaluate(context);
    }
}

RvViewGroupIPNode::~RvViewGroupIPNode()
{
}

} // Rv
