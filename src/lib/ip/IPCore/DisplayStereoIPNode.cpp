//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/ShaderCommon.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;

    string DisplayStereoIPNode::m_defaultType;
    int DisplayStereoIPNode::m_swapScanlines = false;

    DisplayStereoIPNode::DisplayStereoIPNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* g, GroupIPNode* group)
        : StereoTransformIPNode(name, def, g, group)
    {
        setHasLinearTransform(true); // fits to aspect
        m_stereoType = declareProperty<StringProperty>(
            "stereo.type", m_defaultType == "" ? "off" : m_defaultType);
    }

    DisplayStereoIPNode::~DisplayStereoIPNode() {}

    void DisplayStereoIPNode::setStereoType(const string& type)
    {
        setProperty(m_stereoType, type);
    }

    string DisplayStereoIPNode::stereoType() const
    {
        return propertyValue(m_stereoType, "off");
    }

    namespace
    {

        void prepareForStereo(IPImage* img, bool isLeftEye, bool mirror,
                              bool vertical, Vec3f offset, Vec3f roffset,
                              Vec3f scale)
        {
            if (img->destination == IPImage::OutputTexture)
                return;

            if (offset != Vec3f(0.0f) || roffset != Vec3f(0.0f)
                || scale != Vec3f(1.0f))
            {
                float aspect = img->displayAspect();
                Mat44f T, S;

                if (isLeftEye)
                {
                    T.makeTranslation(offset
                                      * Vec3f(-aspect, vertical ? -1 : 1, 1));
                    S.makeScale(scale);
                }
                else
                {
                    Vec3f off = (offset + roffset) * Vec3f(aspect, 1, 1);
                    if (mirror)
                        off *= Vec3f(-1, 1, 1);
                    T.makeTranslation(off);
                    S.makeScale(
                        Vec3f(mirror ? -scale.x : scale.x, scale.y, scale.z));
                }

                img->transformMatrix = img->transformMatrix * S * T;
            }
        }

    } // namespace

    IPImage* DisplayStereoIPNode::evaluate(const Context& context)
    {
        int frame = context.frame;
        int ninputs = inputs().size();
        Vec3f offset = Vec3f(0.0f);
        Vec3f roffset = Vec3f(0.0f);
        Vec3f scale = Vec3f(1.0f, 1, 1);
        bool mirror = false;
        bool vertical = false;
        bool passthrough = context.stereo;
        bool oneEye = false;
        bool mono = true;

        string type = "mono";

        //
        //  Note that the stereoType here is irrelevant in the
        //  "passthrough" case.
        //
        if (m_stereoType && m_stereoType->size())
        {
            type = m_stereoType->front();
            oneEye = type == "left" || type == "right";
            mono = type == "off" || type == "mono";

            if (type == "pair")
            {
                scale = Vec3f(0.5f, 0.5f, 1);
                offset += Vec3f(0.5f, 0, 0);
            }
            else if (type == "mirror")
            {
                scale = Vec3f(-0.5f, 0.5f, 1.0);
                offset += Vec3f(0.5f, 0, 0);
                mirror = true;
            }
            else if (type == "hsqueezed")
            {
                scale = Vec3f(0.5f, 1, 1);
                offset += Vec3f(0.5f, 0, 0);
            }
            else if (type == "vsqueezed")
            {
                scale = Vec3f(1, 0.5f, 1);
                offset += Vec3f(0, 0.5f, 0);
                vertical = true;
            }
        }

        IPImage* root = NULL;
        Context scontext = context;

        IPImageVector images;
        IPImageSet modifiedImages;

        //
        //  If the incoming context is already in stereo mode than just
        //  continue with that eye. Otherwise we're in some emulation mode
        //  and need to build some shaders
        //

        if (!oneEye && !mono && !passthrough)
        {
            const size_t w = context.viewWidth;
            const size_t h = context.viewHeight;
            const float viewAspect = float(w) / float(h);
            const float parityX = context.viewXOrigin % 2 == 0 ? 0.0 : 1.0;
            float parityY = context.viewYOrigin % 2 == 0 ? 0.0 : 1.0;
            bool isMerge = type == "lumanaglyph" || type == "anaglyph"
                           || type == "scanline" || type == "checker";

            try
            {
                scontext.stereo = true;
                scontext.allowInteractiveResize = isMerge;
                scontext.stereoContext.active = true;

                //  Evaluate left Eye
                Context leftContext = scontext;
                leftContext.eye = 0;
                images.push_back(StereoTransformIPNode::evaluate(leftContext));
                prepareForStereo(images.back(), true, mirror, vertical, offset,
                                 roffset, scale);

                //  Evaluate right Eye
                Context rightContext = scontext;
                rightContext.eye = 1;
                images.push_back(StereoTransformIPNode::evaluate(rightContext));
                prepareForStereo(images.back(), false, mirror, vertical, offset,
                                 roffset, scale);
            }
            catch (std::exception&)
            {
                ThreadType thread = context.thread;
                TWK_CACHE_LOCK(context.cache, "thread=" << thread);
                context.cache.checkInAndDelete(images);
                TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
                throw;
            }

            //
            //  Each eye is a child of root, which is a merge by
            //  default. default operation if not specified is add of both
            //  eyes
            //
            //  For scanline and checker we need this to be happening in
            //  an image the size of the display image.
            //

            root = new IPImage(this,
                               isMerge ? IPImage::MergeRenderType
                                       : IPImage::BlendRenderType,
                               w, h, 1.0,
                               isMerge ? IPImage::IntermediateBuffer
                                       : IPImage::CurrentFrameBuffer,
                               IPImage::FloatDataType);

            const float aspect = root->displayAspect();

            if (isMerge)
            {
                images[0]->fitToAspect(aspect);
                images[1]->fitToAspect(aspect);

                convertBlendRenderTypeToIntermediate(images, modifiedImages);

                balanceResourceUsage(IPNode::accumulate, images, modifiedImages,
                                     8, 8, 81);

                Shader::ExpressionVector inExpressions;
                assembleMergeExpressions(root, images, modifiedImages, false,
                                         inExpressions);

                if (type == "anaglyph")
                    root->mergeExpr =
                        Shader::newStereoAnaglyph(root, inExpressions);
                else if (type == "lumanaglyph")
                    root->mergeExpr =
                        Shader::newStereoLumAnaglyph(root, inExpressions);
                else if (type == "checker")
                    root->mergeExpr = Shader::newStereoChecker(
                        root, parityX, parityY, inExpressions);
                else if (type == "scanline")
                {
                    if (DisplayStereoIPNode::m_swapScanlines)
                        parityY = 1 - parityY;
                    root->mergeExpr =
                        Shader::newStereoScanline(root, parityY, inExpressions);
                }

                root->shaderExpr = Shader::newSourceRGBA(root);
                root->appendChildren(images);
            }
            else
            {
                float daspect = float(w) / float(h);
                float iaspect = images[0]->displayAspect();

                if (daspect < iaspect)
                {
                    //
                    //  Insert another coordinate system in here so we can
                    //  fitToAspect on the result of whatever transform occured
                    //  with the left and right eyes.
                    //

                    IPImage* temp = new IPImage(
                        this, IPImage::BlendRenderType, images[0]->width,
                        images[0]->height, 1.0, IPImage::CurrentFrameBuffer);

                    temp->appendChildren(images);
                    root->appendChild(temp);
                    temp->fitToAspect(daspect);
                }
                else
                {
                    root->appendChildren(images);
                }
            }

            root->recordResourceUsage();
            return root;
        }
        else if (oneEye)
        {
            scontext.stereo = true;
            bool isLeft = type == "left";
            scontext.eye = isLeft ? 0 : 1;
            scontext.stereoContext.active = true;
            root = StereoTransformIPNode::evaluate(scontext);
            prepareForStereo(root, isLeft, mirror, vertical, offset, roffset,
                             scale);

            return root;
        }
        else //  mono || passthrough
        {
            if (scontext.stereo)
                scontext.stereoContext.active = true;
            root = StereoTransformIPNode::evaluate(scontext);
            //
            //  Note that we are in mono or passthrough mode and in either case
            //  we should be doing nothing to img matrix here, so do _not_ call
            //  prepareForStereo.
            //
            return root;
        }
    }

    IPImageID* DisplayStereoIPNode::evaluateIdentifier(const Context& context)
    {
        bool passthrough = context.stereo;
        bool oneEye = false;
        bool mono = true;

        string type = "mono";

        if (m_stereoType && m_stereoType->size())
        {
            type = m_stereoType->front();
            oneEye = type == "left" || type == "right";
            mono = type == "off" || type == "mono";
        }

        Context scontext = context;

        //
        //  If the incoming context is already in stereo mode than just
        //  continue with that eye. Otherwise we're in some emulation mode
        //  and need to build some shaders
        //

        if (!oneEye && !mono && !passthrough)
        {
            vector<IPImageID*> images;
            scontext.stereo = true;
            scontext.stereoContext.active = true;

            //  Evaluate left Eye
            Context leftContext = scontext;
            leftContext.eye = 0;
            images.push_back(
                StereoTransformIPNode::evaluateIdentifier(leftContext));

            //  Evaluate right Eye
            Context rightContext = scontext;
            rightContext.eye = 1;
            images.push_back(
                StereoTransformIPNode::evaluateIdentifier(rightContext));

            IPImageID* root = new IPImageID();

            root->appendChildren(images);

            return root;
        }
        else if (oneEye)
        {
            bool isLeft = scontext.eye == 0;
            scontext.eye = isLeft ? 0 : 1;
            scontext.stereo = true;
        }

        if (scontext.stereo)
        {
            scontext.stereoContext.active = true;
        }

        return StereoTransformIPNode::evaluateIdentifier(scontext);
    }

    void DisplayStereoIPNode::metaEvaluate(const Context& context,
                                           MetaEvalVisitor& visitor)
    {
        visitor.enter(context, this);

        IPNode* input = (inputs().size()) ? inputs()[0] : 0;
        if (!input)
        {
            visitor.leave(context, this);
            return;
        }

        bool passthrough = context.stereo;
        bool oneEye = false;
        bool mono = true;

        string type = "mono";

        if (StringProperty* sp = m_stereoType)
        {
            if (sp->size())
            {
                type = sp->front();
                oneEye = type == "left" || type == "right";
                mono = type == "off" || type == "mono";
            }
        }

        Context scontext = context;

        //
        //  If the incoming context is already in stereo mode than just
        //  continue with that eye. Otherwise we're in some emulation mode
        //  and need to build some shaders
        //

        if (!oneEye && !mono && !passthrough)
        {
            scontext.stereo = true;
            scontext.stereoContext.active = true;

            //  Evaluate left Eye
            Context leftContext = scontext;
            leftContext.eye = 0;
            if (visitor.traverseChild(leftContext, 0, this, input))
            {
                input->metaEvaluate(leftContext, visitor);
            }

            //  Evaluate right Eye
            Context rightContext = scontext;
            rightContext.eye = 1;
            if (visitor.traverseChild(rightContext, 1, this, input))
            {
                input->metaEvaluate(rightContext, visitor);
            }

            visitor.leave(context, this);
            return;
        }
        else if (oneEye)
        {
            bool isLeft = scontext.eye == 0;
            scontext.eye = isLeft ? 0 : 1;
            scontext.stereo = true;
        }

        if (scontext.stereo)
        {
            scontext.stereoContext.active = true;
        }

        if (visitor.traverseChild(scontext, 0, this, input))
        {
            input->metaEvaluate(scontext, visitor);
        }

        visitor.leave(context, this);
    }

    void
    DisplayStereoIPNode::propagateFlushToInputs(const FlushContext& context)
    {
        bool passthrough = context.stereo;
        bool oneEye = false;
        bool mono = true;

        string type = "mono";

        if (m_stereoType && m_stereoType->size())
        {
            type = m_stereoType->front();
            oneEye = type == "left" || type == "right";
            mono = type == "off" || type == "mono";
        }

        Context scontext = context;

        //
        //  If the incoming context is already in stereo mode than just
        //  continue with that eye. Otherwise we're in some emulation mode
        //  and need to include each eye.
        //

        if (!oneEye && !mono && !passthrough)
        {
            scontext.stereo = true;
            scontext.stereoContext.active = true;

            //  Evaluate left Eye
            Context leftContext = scontext;
            leftContext.eye = 0;
            StereoTransformIPNode::propagateFlushToInputs(leftContext);

            //  Evaluate right Eye
            Context rightContext = scontext;
            rightContext.eye = 1;
            StereoTransformIPNode::propagateFlushToInputs(rightContext);

            return;
        }
        else if (oneEye)
        {
            bool isLeft = scontext.eye == 0;
            scontext.eye = isLeft ? 0 : 1;
            scontext.stereo = true;
        }

        if (scontext.stereo)
        {
            scontext.stereoContext.active = true;
        }

        StereoTransformIPNode::propagateFlushToInputs(scontext);
    }

} // namespace IPCore
