//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__SourceStereoIPNode__h__
#define __IPGraph__SourceStereoIPNode__h__
#include <iostream>
#include <IPCore/StereoTransformIPNode.h>

namespace IPCore
{

    class SourceStereoIPNode : public StereoTransformIPNode
    {
    public:
        SourceStereoIPNode(const std::string& name, const NodeDefinition* def,
                           IPGraph*, GroupIPNode* group = 0);

        virtual ~SourceStereoIPNode();
        virtual IPImage* evaluate(const Context&);

    protected:
        virtual void contextFromStereoContext(Context&);
    };

} // namespace IPCore

#endif // __IPGraph__SourceStereoIPNode__h__
