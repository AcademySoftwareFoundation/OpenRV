//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPBaseNodes__PrimaryConvertIPNode__h__
#define __IPBaseNodes__PrimaryConvertIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  PrimaryConvertIPNode
    //
    //  Handles primary colorspace conversions with
    //  optional illuminant adaptation
    //

    class PrimaryConvertIPNode : public IPNode
    {
    public:
        PrimaryConvertIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group = 0);

        virtual ~PrimaryConvertIPNode();
        virtual IPImage* evaluate(const Context&);
        bool active() const;

    private:
        IntProperty* m_activeProperty;

        // Chromaticities properties
        Vec2fProperty* m_inChromaticitiesRed;
        Vec2fProperty* m_inChromaticitiesGreen;
        Vec2fProperty* m_inChromaticitiesBlue;
        Vec2fProperty* m_inChromaticitiesWhite;

        Vec2fProperty* m_outChromaticitiesRed;
        Vec2fProperty* m_outChromaticitiesGreen;
        Vec2fProperty* m_outChromaticitiesBlue;
        Vec2fProperty* m_outChromaticitiesWhite;

        // Illuminant adaptation properties
        Vec2fProperty* m_inIlluminantWhite;
        Vec2fProperty* m_outIlluminantWhite;
        IntProperty* m_useBradfordTransform;
    };

} // namespace IPCore

#endif // __IPBaseNodes__PrimaryConvertIPNode__h__
