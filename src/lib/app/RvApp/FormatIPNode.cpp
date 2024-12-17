//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <RvApp/FormatIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPProperty.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <TwkFBAux/FBAux.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Iostream.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/NodeDefinition.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkFB;
    using namespace std;
    using namespace TwkMath;

    int FormatIPNode::defaultBitDepth = 0; // any depth
    bool FormatIPNode::defaultAllowFP = true;
    string FormatIPNode::defaultResampleMethod = "area";

    IPNode::ImageStructureInfo outResFromParams(float w, float h,
                                                FormatIPNode::Params& s)
    {
        float aspect;
        float fx = w;
        float fy = h;

        //
        //  Make sure that we don't wind up dividing by zero and having
        //  undefined or NaN values in the resulting ImageStructureInfo
        //
        //  In particular if either of the incoming dimensions are 0 at
        //  this point, then try to preemptively use the fit parameters
        //  to create the aspect if they are both valid
        //
        //  Otherwise we set aspect to 1.0 and use the following checks
        //  to construct the ImageStructureInfo
        //

        if (fx == 0 || fy == 0)
        {
            aspect = (s.xfit && s.yfit) ? float(s.xfit) / float(s.yfit) : 1.0;
        }
        else
            aspect = fx / fy;

        if (s.xresize && !s.yresize)
        {
            fx = s.xresize;
            fy = fx / aspect;
        }
        else if (s.yresize && !s.xresize)
        {
            fy = s.yresize;
            fx = fy * aspect;
        }
        else if (s.xresize && s.yresize)
        {
            fx = s.xresize;
            fy = s.yresize;
        }
        else if (s.xfit && !s.yfit)
        {
            fx = s.xfit;
            fy = fx / aspect;
        }
        else if (s.yfit && !s.xfit)
        {
            fy = s.yfit;
            fx = fy * aspect;
        }
        else if (s.xfit && s.yfit)
        {
            // the goal here is to make the output match the user specified
            // s.xfit and s.yfit at least in one dimension
            // but also make sure the output dimension does NOT exceed
            // either s.xfit or s.yfit
            float oaspect = float(s.xfit) / float(s.yfit);
            if (oaspect >= aspect)
            {
                fy = s.yfit;
                fx = fy * aspect;
            }
            else
            {
                fx = s.xfit;
                fy = fx / aspect;
            }
        }
        else if (s.scale != 1.0)
        {
            fx *= s.scale;
            fy *= s.scale;
        }

        return IPNode::ImageStructureInfo(int(fx + 0.49f), int(fy + 0.49f),
                                          1.0);
    }

    FormatIPNode::FormatIPNode(const std::string& name,
                               const NodeDefinition* def, IPGraph* g,
                               GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_xmin(0)
        , m_xmax(0)
        , m_ymin(0)
        , m_ymax(0)
        , m_leftCrop(0)
        , m_rightCrop(0)
        , m_topCrop(0)
        , m_bottomCrop(0)
        , m_cropManip(0)
    {
        setMaxInputs(1);
        m_xfit = declareProperty<IntProperty>("geometry.xfit", 0);
        m_yfit = declareProperty<IntProperty>("geometry.yfit", 0);
        m_xresize = declareProperty<IntProperty>("geometry.xresize", 0);
        m_yresize = declareProperty<IntProperty>("geometry.yresize", 0);
        m_scale = declareProperty<FloatProperty>("geometry.scale", 1.0f);
        m_resampleMethod = declareProperty<StringProperty>(
            "geometry.resampleMethod", defaultResampleMethod);
        m_maxBitDepth =
            declareProperty<IntProperty>("color.maxBitDepth", defaultBitDepth);
        m_allowFloatingPoint = declareProperty<IntProperty>(
            "color.allowFloatingPoint", defaultAllowFP ? 1 : 0);
        m_cropActive = declareProperty<IntProperty>("crop.active", 0);
        m_uncropActive = declareProperty<IntProperty>("uncrop.active", 0);
        m_uncropWidth = declareProperty<IntProperty>("uncrop.width", 0);
        m_uncropHeight = declareProperty<IntProperty>("uncrop.height", 0);
        m_uncropX = declareProperty<IntProperty>("uncrop.x", 0);
        m_uncropY = declareProperty<IntProperty>("uncrop.y", 0);

        if (def->intValue("defaults.cropLRBT", 0) == 0)
        {
            m_xmin = declareProperty<IntProperty>("crop.xmin", 0);
            m_ymin = declareProperty<IntProperty>("crop.ymin", 0);
            m_xmax = declareProperty<IntProperty>("crop.xmax", 0);
            m_ymax = declareProperty<IntProperty>("crop.ymax", 0);
        }
        else
        {
            m_leftCrop = declareProperty<IntProperty>("crop.left", 0);
            m_rightCrop = declareProperty<IntProperty>("crop.right", 0);
            m_topCrop = declareProperty<IntProperty>("crop.top", 0);
            m_bottomCrop = declareProperty<IntProperty>("crop.bottom", 0);

            PropertyInfo* noSave =
                new PropertyInfo(PropertyInfo::NotPersistent);
            m_cropManip = declareProperty<IntProperty>("crop.manip", 0, noSave);
        }
    }

    FormatIPNode::~FormatIPNode() {}

    void FormatIPNode::setCrop(int x0, int y0, int x1, int y1)
    {
        if (m_xmin)
        {
            setProperty(m_xmin, x0);
            setProperty(m_ymin, y0);
            setProperty(m_xmax, x1);
            setProperty(m_ymax, y1);
        }
        else
        {
            setProperty(m_leftCrop, x0);
            setProperty(m_topCrop, y0);
            setProperty(m_rightCrop, x1);
            setProperty(m_bottomCrop, y1);
        }

        setProperty(m_cropActive, 1);
    }

    void FormatIPNode::setUncrop(int w, int h, int x, int y)
    {
        setProperty(m_uncropWidth, w);
        setProperty(m_uncropHeight, h);
        setProperty(m_uncropX, x);
        setProperty(m_uncropY, y);
        setProperty(m_uncropActive, 1);
    }

    void FormatIPNode::setFitResolution(int w, int h)
    {
        setProperty(m_xfit, w);
        setProperty(m_yfit, h);
    }

    void FormatIPNode::setResizeResolution(int w, int h)
    {
        setProperty(m_xresize, w);
        setProperty(m_yresize, h);
    }

    void FormatIPNode::outputDisconnect(IPNode* node)
    {
        IPNode::outputDisconnect(node);
    }

    static FrameBuffer* resizeImage(FrameBuffer* in, float sx, float sy,
                                    FrameBuffer::DataType outType)
    {
        //
        //  sx here is really representing outWidth/inWidth (both integers), but
        //  due to imprecise representation of that rational number as a float,
        //  we _must_ round here to make sure we recover the correct outWidth
        //  and not outWidth-1.
        //
        int ow = int(0.49f + float(in->width()) * sx);
        int oh = int(0.49f + float(in->height()) * sy);

        if (ow != in->width() || oh != in->height()
            || outType != in->dataType())
        {
            /*
            cout << "resize from " << in->width() << ", " << in->height()
                 << " to " << ow << ", " << oh << endl;
            */

            FrameBuffer* fb =
                new FrameBuffer(ow, oh, in->numChannels(), outType, 0,
                                &in->channelNames(), in->orientation());
            if (fb->isRootPlane())
                fb->idstream() << in->identifier();
            if (in->nextPlane())
                fb->appendPlane(resizeImage(in->nextPlane(), sx, sy, outType));
            return fb;
        }
        else
        {
            return in;
        }
    }

    FormatIPNode::Params FormatIPNode::params() const
    {
        Params s;

        s.floatOk = true;
        s.scale = 1.0;
        s.xfit = 0;
        s.yfit = 0;
        s.xresize = 0.0;
        s.yresize = 0.0;
        s.maxBitDepth = 32;
        s.bestIntegral = FrameBuffer::USHORT;
        s.bestFloat = FrameBuffer::FLOAT;
        s.crop = false;
        s.cropX0 = 0;
        s.cropY0 = 0;
        s.cropX1 = 0;
        s.cropY1 = 0;
        s.leftCrop = 0;
        s.rightCrop = 0;
        s.topCrop = 0;
        s.bottomCrop = 0;
        s.uncrop = false;
        s.uncropWidth = 0;
        s.uncropHeight = 0;
        s.uncropX = 0;
        s.uncropY = 0;
        s.method = TwkFBAux::AreaInterpolation;

        if (const StringProperty* sp = m_resampleMethod)
        {
            if (sp->size())
            {
                if (sp->front() == "linear")
                    s.method = TwkFBAux::LinearInterpolation;
            }
        }

        if (const IntProperty* bdepth = m_maxBitDepth)
        {
            s.maxBitDepth = bdepth->front();

            if (s.maxBitDepth >= 8)
                s.bestIntegral = FrameBuffer::UCHAR;
            if (s.maxBitDepth >= 16)
                s.bestIntegral = FrameBuffer::USHORT;
            if (s.maxBitDepth >= 32)
                s.bestIntegral = FrameBuffer::UINT;
        }

        if (const IntProperty* allowFloat = m_allowFloatingPoint)
        {
            s.floatOk = allowFloat->front() == 0 ? false : true;
            s.floatOk = ImageRenderer::hasFloatFormats() ? s.floatOk : false;

            if (s.floatOk)
            {
                if (s.maxBitDepth >= 16)
                    s.bestFloat = FrameBuffer::HALF;
                if (s.maxBitDepth >= 32)
                    s.bestFloat = FrameBuffer::FLOAT;
            }
            else
            {
                s.bestFloat = s.bestIntegral;
            }
        }

        if (const FloatProperty* scaleP = m_scale)
        {
            s.scale = scaleP->front();
        }

        if (const IntProperty* xfit = m_xfit)
        {
            s.xfit = xfit->front();
        }

        if (const IntProperty* yfit = m_yfit)
        {
            s.yfit = yfit->front();
        }

        if (const IntProperty* xresize = m_xresize)
        {
            s.xresize = xresize->front();
        }

        if (const IntProperty* yresize = m_yresize)
        {
            s.yresize = yresize->front();
        }

        s.crop =
            propertyValue(m_cropActive, 0) && !propertyValue(m_cropManip, 0);

        if (const IntProperty* x0 = m_xmin)
        {
            s.cropX0 = x0->front();
        }

        if (m_leftCrop)
            s.leftCrop = m_leftCrop->front();
        if (m_rightCrop)
            s.rightCrop = m_rightCrop->front();
        if (m_topCrop)
            s.topCrop = m_topCrop->front();
        if (m_bottomCrop)
            s.bottomCrop = m_bottomCrop->front();

        if (const IntProperty* x1 = m_xmax)
        {
            s.cropX1 = x1->front();
        }

        if (const IntProperty* y0 = m_ymin)
        {
            s.cropY0 = y0->front();
        }

        if (const IntProperty* y1 = m_ymax)
        {
            s.cropY1 = y1->front();
        }

        if (const IntProperty* a = m_uncropActive)
        {
            s.uncrop = a->front() != 0;
        }

        if (const IntProperty* w = m_uncropWidth)
        {
            s.uncropWidth = w->front();
        }

        if (const IntProperty* h = m_uncropHeight)
        {
            s.uncropHeight = h->front();
        }

        if (const IntProperty* x = m_uncropX)
        {
            s.uncropX = x->front();
        }

        if (const IntProperty* y = m_uncropY)
        {
            s.uncropY = y->front();
        }

        return s;
    }

    IPImage* FormatIPNode::evaluate(const Context& context)
    {
        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        IPImage* img = head;

        if (!head)
            return 0;

        Params s = params();

        if (FrameBuffer* in = img->fb)
        {
            FrameBuffer::DataType outType;
            FrameBuffer::DataType inType = in->dataType();

            int originalWidth = in->width();
            int originalHeight = in->height();

            if (inType == FrameBuffer::PACKED_R10_G10_B10_X2
                || inType == FrameBuffer::PACKED_X2_B10_G10_R10
                || inType == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8
                || inType == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8)
            {
                outType = inType;
            }
            else
            {
                if (inType >= FrameBuffer::HALF)
                {
                    outType = (inType > s.bestFloat) ? s.bestFloat : inType;
                }
                else
                {
                    outType =
                        (inType > s.bestIntegral) ? s.bestIntegral : inType;
                }

                if (ImageRenderer::fbAcceptableTypes().count(outType) == 0)
                {
                    outType = FrameBuffer::UCHAR;
                }
            }

            if (s.scale == 1.0f && s.xfit == 0 && s.yfit == 0 && s.xresize == 0
                && s.yresize == 0)
            {
                if (s.crop)
                {
                    FrameBuffer* cfb = 0;

                    try
                    {
                        bool sameType = (in->dataType() == outType);
                        if (m_xmin)
                        {
                            cfb = crop(in, s.cropX0, s.cropY0, s.cropX1,
                                       s.cropY1);
                        }
                        else
                        {
                            cfb = crop(in, s.leftCrop, s.bottomCrop,
                                       in->width() - s.rightCrop - 1,
                                       in->height() - s.topCrop - 1);
                        }

                        cfb->setIdentifier(in->identifier());
                        in->copyAttributesTo(cfb);
                        cfb->setPixelAspectRatio(in->pixelAspectRatio());
                        img->fb = cfb;
                        delete in;
                        in = cfb;

                        // Adjust the img header's width and height accordingly
                        // since we've just replaced its underlying FrameBuffer
                        img->width = cfb->width();
                        img->height = cfb->height();

                        //
                        //  Crop may have changed in's dataType.  If we had
                        //  previously matched inType to outType, make sure we
                        //  do so again.
                        //
                        if (sameType)
                            outType = in->dataType();
                    }
                    catch (...)
                    {
                        // just ignore bad params here
                    }
                }

                if (in->dataType() != outType
                    && in->dataType() != FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8
                    && in->dataType() != FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8)
                {
                    FrameBuffer* fb = 0;

                    if (in->isYRYBYPlanar() || in->isYRYBY())
                    {
                        fb = copyConvertYRYBYtoYUV(in, outType);
                    }
                    else
                    {
                        fb = copyConvert(in, outType);
                    }

                    in->copyAttributesTo(fb);
                    fb->setPixelAspectRatio(in->pixelAspectRatio());
                    img->fb = fb;
                    img->fb->setIdentifier(in->identifier());
                    delete in;
                }
                else
                {
                    // mR - 10/28/07
                    // make sure our identifier is set
                    img->fb->setIdentifier(in->identifier());
                }
            }
            else
            {
                //
                //  scale and/or fit
                //

                if (s.crop)
                {
                    FrameBuffer* cfb = 0;

                    try
                    {
                        bool sameType = (in->dataType() == outType);
                        if (m_xmin)
                        {
                            cfb = crop(in, s.cropX0, s.cropY0, s.cropX1,
                                       s.cropY1);
                        }
                        else
                        {
                            cfb = crop(in, s.leftCrop, s.bottomCrop,
                                       in->width() - s.rightCrop - 1,
                                       in->height() - s.topCrop - 1);
                        }
                        cfb->setIdentifier(in->identifier());
                        in->copyAttributesTo(cfb);
                        cfb->setPixelAspectRatio(in->pixelAspectRatio());
                        img->fb = cfb;
                        delete in;
                        in = cfb;

                        // Adjust the img header's width and height accordingly
                        // since we've just replaced its underlying FrameBuffer
                        img->width = cfb->width();
                        img->height = cfb->height();

                        //
                        //  Crop may have changed in's dataType.  If we had
                        //  previously matched inType to outType, make sure we
                        //  do so again.
                        //
                        if (sameType)
                            outType = in->dataType();
                    }
                    catch (...)
                    {
                        // ignore bad crop parameters
                    }
                }

                ImageStructureInfo ii;
                float scalex = 1.0;
                float scaley = 1.0;

                if (s.uncrop)
                {
                    //
                    //  Don't totally understand this.  It loooks
                    //  like the image will be scaled appropriately
                    //  for the uncrop later by
                    //  CompositeRenderer::initializePlane(), so if
                    //  we do it here we get a double scale.
                    //
                    //  ii = outResFromParams(s.uncropWidth, s.uncropHeight, s);
                    //  scale = float(ii.width) / float(s.uncropWidth);
                    //

                    //                     cout << "s.uncrop = " <<
                    //                     s.uncropWidth << ", " <<
                    //                     s.uncropHeight
                    //                          << ", scale = " << scale <<
                    //                          endl;
                    //                     cout << "fit = " << s.xfit << ", " <<
                    //                     s.yfit << endl;
                }
                else if (in->width() && in->height())
                {
                    ii = outResFromParams(in->width(), in->height(), s);
                    scalex = float(ii.width) / float(in->width());
                    scaley = float(ii.height) / float(in->height());
                }

                if (inType == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8
                    || inType == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8)
                {
                    FrameBuffer* fb0 = convertToLinearRGB709(in);
                    FrameBuffer* fb1 =
                        resizeImage(fb0, scalex, scaley, FrameBuffer::UCHAR);

                    if (fb0 != fb1)
                    {
                        TwkFBAux::resize(fb0, fb1);
                        img->fb = fb1;
                        delete fb0;
                    }
                    else
                    {
                        img->fb = fb0;
                    }

                    // Adjust the img header's width and height accordingly
                    // since we've just replaced its underlying FrameBuffer
                    img->width = img->fb->width();
                    img->height = img->fb->height();
                }
                else if (inType == outType)
                {
                    FrameBuffer* fb = resizeImage(in, scalex, scaley, outType);

                    if (fb != in)
                    {
                        TwkFBAux::resize(in, fb);
                    }

                    img->fb = fb;

                    // Avoid the off by one error
                    img->width = int(0.49f + (float(img->width)) * scalex);
                    img->height = int(0.49f + (float(img->height)) * scaley);
                }
                else
                {
                    FrameBuffer* fb0 = resizeImage(in, scalex, scaley, outType);
                    FrameBuffer* fb1 = 0;

                    if (fb0 != in)
                    {
                        TwkFBAux::resize(in, fb0);

                        if (in->isYRYBYPlanar() || in->isYRYBY())
                        {
                            fb1 = copyConvertYRYBYtoYUV(fb0, outType);
                        }
                        else
                        {
                            fb1 = copyConvert(fb0, outType);
                        }

                        img->fb = fb1;
                        delete fb0;
                    }
                    else
                    {
                        img->fb = in;
                    }

                    // Adjust the img header's width and height accordingly
                    // since we've just replaced its underlying FrameBuffer
                    img->width = img->fb->width();
                    img->height = img->fb->height();
                }
                img->fb->setUncrop(int(scalex * in->uncropWidth() + 0.49f),
                                   int(scaley * in->uncropHeight() + 0.49f),
                                   int(scalex * in->uncropX() + 0.49f),
                                   int(scaley * in->uncropY() + 0.49f));

                img->fb->setUncropActive(in->uncrop());
                in->copyAttributesTo(img->fb);
                img->fb->setIdentifier(in->identifier());
                img->fb->setPixelAspectRatio(in->pixelAspectRatio());
                if (in != img->fb)
                    delete in;
            }

            if (s.uncrop)
            {
                img->fb->setUncrop(s.uncropWidth, s.uncropHeight, s.uncropX,
                                   s.uncropY);
                img->fb->setUncropActive(in->uncrop());
                img->width = s.uncropWidth;
                img->height = s.uncropHeight;
            }

            if (s.scale != 1.0f || s.bestFloat != FrameBuffer::FLOAT
                || s.bestIntegral != FrameBuffer::USHORT || s.xfit || s.yfit
                || s.xresize || s.yresize || s.crop || s.uncrop)
            {
                //
                //  NOTE: you can just get rid of xfit and yfit here and
                //  use a single scale value: you don't in principle know
                //  the resolution here that the xfit and yfit are
                //  relative to, so you have to include them in the hash
                //

                img->fb->idstream()
                    << ":fmt/s" << s.scale << ":f" << s.bestFloat << ":i"
                    << s.bestIntegral << ":r" << s.method << ":xf" << s.xfit
                    << ":yf" << s.yfit << ":xr" << s.xresize << ":yr"
                    << s.yresize;

                if (s.crop)
                {
                    img->fb->idstream() << ":c"
                                        << "+" << s.cropX0 << "+" << s.cropY0
                                        << "+" << s.cropX1 << "+" << s.cropY1;
                }

                if (s.uncrop)
                {
                    img->fb->idstream()
                        << ":u"
                        << "+" << s.uncropWidth << "+" << s.uncropHeight << "+"
                        << s.uncropX << "+" << s.uncropY;
                }
            }
        }

        return head;
    }

    IPImageID* FormatIPNode::evaluateIdentifier(const Context& context)
    {
        IPImageID* imgid = IPNode::evaluateIdentifier(context);
        float scale = 1.0f;
        int depth = 0;

        Params p = params();

        IPImageID* i = imgid;
        ostringstream str;

        if (p.scale != 1.0f || p.bestFloat != FrameBuffer::FLOAT
            || p.bestIntegral != FrameBuffer::USHORT || p.xfit || p.yfit
            || p.xresize || p.yresize || p.crop || p.uncrop)
        {
            //
            //  NOTE: you can just get rid of xfit and yfit here and
            //  use a single scale value: you don't in principle know
            //  the resolution here that the xfit and yfit are
            //  relative to, so you have to include them in the hash
            //

            str << i->id;

            str << ":fmt/s" << p.scale << ":f" << p.bestFloat << ":i"
                << p.bestIntegral << ":r" << p.method << ":xf" << p.xfit
                << ":yf" << p.yfit << ":xr" << p.xresize << ":yr" << p.yresize;

            if (p.crop)
            {
                str << ":c"
                    << "+" << p.cropX0 << "+" << p.cropY0 << "+" << p.cropX1
                    << "+" << p.cropY1;
            }

            if (p.uncrop)
            {
                str << ":u"
                    << "+" << p.uncropWidth << "+" << p.uncropHeight << "+"
                    << p.uncropX << "+" << p.uncropY;
            }

            i->id = str.str();
        }

        return imgid;
    }

    IPNode::ImageStructureInfo
    FormatIPNode::imageStructureInfo(const Context& context) const
    {
        if (!inputs().empty())
        {
            Params p = params();
            ImageStructureInfo i =
                inputs().front()->imageStructureInfo(context);

            if (p.crop)
            {
                i.width = p.cropX1 - p.cropX0 + 1;
                i.height = p.cropY1 - p.cropY0 + 1;
            }

            ImageStructureInfo n = outResFromParams(i.width, i.height, p);

            if (p.uncrop)
            {
                n.width = p.uncropWidth;
                n.height = p.uncropHeight;
            }

            n.pixelAspect = i.pixelAspect;
            n.orientation = i.orientation;
            return n;
        }

        return ImageStructureInfo();
    }

    void FormatIPNode::propertyChanged(const Property* p)
    {
        //
        //  All properties of this node affect caching of all frames of the
        //  corresponding source.  so if the props change, flush all frames from
        //  this source.
        //
        if (group())
            group()->flushIDsOfGroup();

        IPNode::propertyChanged(p);
        propagateImageStructureChange();
    }

} // namespace IPCore
