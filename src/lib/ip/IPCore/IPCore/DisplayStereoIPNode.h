//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__DisplayStereoIPNode__h__
#define __IPCore__DisplayStereoIPNode__h__
#include <iostream>
#include <IPCore/StereoTransformIPNode.h>

namespace IPCore
{

    class DisplayStereoIPNode : public StereoTransformIPNode
    {
    public:
        DisplayStereoIPNode(const std::string& name, const NodeDefinition* def,
                            IPGraph*, GroupIPNode* group = 0);
        virtual ~DisplayStereoIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context& context);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual void propagateFlushToInputs(const FlushContext&);

        static void setDefaultType(const std::string& t) { m_defaultType = t; }

        std::string stereoType() const;
        void setStereoType(const std::string& type);

        static bool swapScanlines() { return m_swapScanlines; }

        static void setSwapScanlines(bool s) { m_swapScanlines = s; }

    private:
        static std::string m_defaultType;
        static int m_swapScanlines;
        StringProperty* m_stereoType;
    };

} // namespace IPCore

#endif // __IPCore__DisplayStereoIPNode__h__
