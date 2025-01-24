//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__FileSpaceLinearizeIPNode__h__
#define __IPGraph__FileSpaceLinearizeIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    //
    //  FileSpaceLinearizeIPNode
    //
    //  Convert from file to (linear) working space. Don't use this node
    //  in new applications. This is for RV backwards compatibility.
    //

    class FileSpaceLinearizeIPNode : public LUTIPNode
    {
    public:
        FileSpaceLinearizeIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* graph,
                                 GroupIPNode* group = 0);

        virtual ~FileSpaceLinearizeIPNode();

        virtual IPImage* evaluate(const Context&);

        void setLogLin(int);
        void setSRGB(int);
        void setRec709(int);
        void setFileGamma(float);

    private:
        void evaluateOne(IPImage* img, const Context& context);

    private:
        IntProperty* m_colorAlphaType;
        IntProperty* m_colorLogType;
        IntProperty* m_cineonWhite;
        IntProperty* m_cineonBlack;
        IntProperty* m_cineonBreakPoint;
        IntProperty* m_colorYUV;
        IntProperty* m_colorsRGB2Linear;
        IntProperty* m_colorRec7092Linear;
        FloatProperty* m_colorFileGamma;
        IntProperty* m_colorActive;
        IntProperty* m_colorIgnoreChromaticities;
        IntProperty* m_CDLactive;
        Vec3fProperty* m_CDLslope;
        Vec3fProperty* m_CDLoffset;
        Vec3fProperty* m_CDLpower;
        FloatProperty* m_CDLsaturation;
        IntProperty* m_CDLnoclamp;
    };

} // namespace IPCore

#endif // __IPGraph__FileSpaceLinearizeIPNode__h__
