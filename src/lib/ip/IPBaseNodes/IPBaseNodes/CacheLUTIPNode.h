//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__CacheLUTIPNode__cpp__
#define __IPGraph__CacheLUTIPNode__cpp__
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    /// Provides a 3D and channel CacheLUT LUT (right before display, per
    /// source)

    ///
    /// CacheLUTIPNode like ColorIPNode handles two LUTs -- a 3D LUT and a
    /// channel LUT. This LUT is evaluated in software only. Its intended
    /// to be used just before the CacheIPNode
    ///

    class CacheLUTIPNode : public LUTIPNode
    {
    public:
        //
        //  Constructors
        //

        CacheLUTIPNode(const std::string& name, const NodeDefinition* def,
                       IPGraph*, GroupIPNode* group = 0);
        virtual ~CacheLUTIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void propertyChanged(const Property*);

        bool isActive() const;
    };

} // namespace IPCore

#endif // __IPGraph__CacheLUTIPNode__cpp__
