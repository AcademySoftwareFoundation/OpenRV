//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/CacheLUTIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;

    CacheLUTIPNode::CacheLUTIPNode(const std::string& name,
                                   const NodeDefinition* def, IPGraph* g,
                                   GroupIPNode* group)
        : LUTIPNode(name, def, g, group)
    {
        m_useHalfLUTProp = false;
        m_floatOutLUT = true;
        m_usePowerOf2 = false;
        setMaxInputs(1);
    }

    CacheLUTIPNode::~CacheLUTIPNode() {}

    static void applyLUT(const float* inp, float* outp, int channels,
                         int nelements, void* inlut)
    {
        FrameBuffer* lut = (FrameBuffer*)inlut;

        if (lut->hasAttribute("inMatrix"))
        {
            IPImage::Matrix M = lut->attribute<IPImage::Matrix>("inMatrix");
            if (M != IPImage::Matrix())
                linearColorTransform(inp, outp, channels, nelements, &M);
        }
        else if (lut->hasAttribute("matrix"))
        {
            IPImage::Matrix M = lut->attribute<IPImage::Matrix>("matrix");
            if (M != IPImage::Matrix())
                linearColorTransform(inp, outp, channels, nelements, &M);
        }

        //
        //  prelut or channel LUT
        //

        FrameBuffer* lut2d = 0;

        if (lut->depth() > 1)
            lut2d = lut->nextPlane();
        else
            lut2d = lut;

        if (lut2d)
        {
            try
            {
                channelLUTTransform(inp, outp, channels, nelements, lut2d);
            }
            catch (...)
            {
                cout << "ERROR: caught exception from channelLUTTransform"
                     << endl;
            }

            if (lut2d->hasAttribute("scale"))
            {
                float scale = lut->attribute<float>("scale");
                float offset = lut->attribute<float>("offset");

                IPImage::Matrix M(scale, 0, 0, offset, 0, scale, 0, offset, 0,
                                  0, scale, offset, 0, 0, 0, 1);

                if (M != IPImage::Matrix())
                    linearColorTransform(inp, outp, channels, nelements, &M);
            }
        }

        if (lut->depth() > 1)
        {
            pixel3DLUTTransform(inp, outp, channels, nelements, lut);

            if (lut->hasAttribute("scale"))
            {
                float scale = lut->attribute<float>("scale");
                float offset = lut->attribute<float>("offset");

                IPImage::Matrix M(scale, 0, 0, offset, 0, scale, 0, offset, 0,
                                  0, scale, offset, 0, 0, 0, 1);

                if (M != IPImage::Matrix())
                    linearColorTransform(inp, outp, channels, nelements, &M);
            }
        }

        if (lut->hasAttribute("outMatrix"))
        {
            IPImage::Matrix M = lut->attribute<IPImage::Matrix>("outMatrix");
            if (M != IPImage::Matrix())
                linearColorTransform(inp, outp, channels, nelements, &M);
        }
    }

    struct ApplyLUTs
    {
        ApplyLUTs(FrameBuffer* preLUT, FrameBuffer* LUT)
            : _preLUT(preLUT)
            , _LUT(LUT)
        {
        }

        FrameBuffer* _preLUT;
        FrameBuffer* _LUT;

        void operator()(IPImage* i)
        {
            if (FrameBuffer* fb = i->fb)
            {
                if (fb->isPlanar() || fb->isYRYBY() || fb->isYUV()
                    || fb->dataType()
                           > TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10)
                {
                    FrameBuffer* nfb = convertToLinearRGB709(fb);
                    delete fb;
                    fb = nfb;
                }

                applyTransform(fb, fb, applyLUT, _LUT);

                fb->idstream() << ":CL";
                if (_preLUT)
                    fb->idstream() << "+" << _preLUT->identifier();
                if (_LUT)
                    fb->idstream() << "+" << _LUT->identifier();

                i->fb = fb;
            }
        }
    };

    struct AddLUTIDs
    {
        AddLUTIDs(FrameBuffer* preLUT, FrameBuffer* LUT)
            : _preLUT(preLUT)
            , _LUT(LUT)
        {
        }

        FrameBuffer* _preLUT;
        FrameBuffer* _LUT;

        void operator()(IPImageID* i)
        {
            ostringstream str;
            str << i->id;
            str << ":CL";
            if (_preLUT)
                str << "+" << _preLUT->identifier();
            if (_LUT)
                str << "+" << _LUT->identifier();
            i->id = str.str();
        }
    };

    IPImage* CacheLUTIPNode::evaluate(const Context& context)
    {
        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");

        if (lutActive() && m_lutfb)
        {
            ApplyLUTs F(m_prelut, m_lutfb);
            foreach_ip(head, F);
        }

        return head;
    }

    IPImageID* CacheLUTIPNode::evaluateIdentifier(const Context& context)
    {
        IPImageID* imgid = IPNode::evaluateIdentifier(context);

        if (lutActive())
        {
            AddLUTIDs F(m_prelut, m_lutfb);
            foreach_ip(imgid, F);
        }

        return imgid;
    }

    void CacheLUTIPNode::propertyChanged(const Property* p)
    {
        //
        //  All properties of this node affect caching of all frames of the
        //  corresponding source.  so if the props change, flush all frames from
        //  this source.
        //
        if (group())
            group()->flushIDsOfGroup();

        LUTIPNode::propertyChanged(p);
    }

} // namespace IPCore
