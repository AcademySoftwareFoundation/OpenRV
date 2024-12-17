//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__FormatIPNode__h__
#define __IPGraph__FormatIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFBAux/FBAux.h>
#include <TwkMovie/Movie.h>
#include <algorithm>
#include <set>

namespace IPCore
{

    //
    //  class FormatIPNode
    //
    //  Converts inputs into formats that can be used by the ImageRenderer
    //

    class FormatIPNode : public IPNode
    {
    public:
        typedef TwkFBAux::Interpolation Interpolation;
        typedef TwkFB::FrameBuffer::DataType DataType;

        struct Params
        {
            bool floatOk;
            float scale;
            int xfit;
            int yfit;
            int xresize;
            int yresize;
            int maxBitDepth;
            DataType bestIntegral;
            DataType bestFloat;
            bool crop;
            int cropX0;
            int cropY0;
            int cropX1;
            int cropY1;
            int leftCrop;
            int rightCrop;
            int topCrop;
            int bottomCrop;
            bool uncrop;
            int uncropWidth;
            int uncropHeight;
            int uncropX;
            int uncropY;
            Interpolation method;
        };

        FormatIPNode(const std::string& name, const NodeDefinition* def,
                     IPGraph*, GroupIPNode* group = 0);
        virtual ~FormatIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);

        Params params() const;

        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void propertyChanged(const Property*);

        void setFitResolution(int w, int h);
        void setResizeResolution(int w, int h);
        void setCrop(int x0, int y0, int x1, int y1);
        void setUncrop(int w, int h, int x, int y);

        static int defaultBitDepth;
        static bool defaultAllowFP;
        static std::string defaultResampleMethod;

    protected:
        virtual void outputDisconnect(IPNode*);

    private:
        IntProperty* m_xfit;
        IntProperty* m_yfit;
        IntProperty* m_xresize;
        IntProperty* m_yresize;
        FloatProperty* m_scale;
        StringProperty* m_resampleMethod;
        IntProperty* m_maxBitDepth;
        IntProperty* m_allowFloatingPoint;
        IntProperty* m_cropActive;
        IntProperty* m_xmin;
        IntProperty* m_xmax;
        IntProperty* m_ymin;
        IntProperty* m_ymax;
        IntProperty* m_leftCrop;
        IntProperty* m_rightCrop;
        IntProperty* m_topCrop;
        IntProperty* m_bottomCrop;
        IntProperty* m_cropManip;
        IntProperty* m_uncropActive;
        IntProperty* m_uncropWidth;
        IntProperty* m_uncropHeight;
        IntProperty* m_uncropX;
        IntProperty* m_uncropY;
    };

} // namespace IPCore

#endif // __IPGraph__FormatIPNode__h__
