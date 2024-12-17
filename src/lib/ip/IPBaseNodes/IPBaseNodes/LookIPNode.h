//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__LookIPNode__h__
#define __IPGraph__LookIPNode__h__
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    /// Provides a 3D and channel Look LUT (right before display, per source)

    ///
    /// LookIPNode like ColorIPNode handles two LUTs -- a 3D LUT and a
    /// channel LUT.
    ///

    class LookIPNode : public LUTIPNode
    {
    public:
        //
        //  Constructors
        //

        LookIPNode(const std::string& name, const NodeDefinition* def,
                   IPGraph* graph, GroupIPNode* group = 0);

        virtual ~LookIPNode();

        virtual IPImage* evaluate(const Context&);
    };

} // namespace IPCore

#endif // __IPGraph__LookIPNode__h__
