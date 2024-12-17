//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/IPProperty.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ResizeIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <TwkApp/VideoDevice.h>
#include <TwkMath/MatrixColor.h>
#include <TwkApp/VideoModule.h>
#include <IPCore/ShaderCommon.h>

#include <sstream>

namespace IPCore
{
    using namespace std;

    DisplayGroupIPNode::DisplayGroupIPNode(const std::string& name,
                                           const NodeDefinition* def,
                                           IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
        , m_physicalDevice(0)
        , m_outputDevice(0)
    {
        //
        //  NOTE: by default the DisplayGroupIPNode is not written to a
        //  session file. This can be changed by setting
        //  defaults.persistent to 1 in the node definition. Typically
        //  only output display groups (OutputDisplayGroup type) are
        //  written to files.
        //

        string stereoNodeType =
            def->stringValue("defaults.stereoType", "DisplayStereo");
        string pipelineType =
            def->stringValue("defaults.pipelineType", "PipelineGroup");
        string resizeType = def->stringValue("defaults.resizeType", "");
        int autoResize = def->intValue("defaults.autoResize", 1);
        bool writable = def->intValue("defaults.persistent", 0);

        PropertyInfo* outputOnly = new PropertyInfo(PropertyInfo::OutputOnly);
        PropertyInfo* notPersistent = new PropertyInfo(
            PropertyInfo::NotPersistent | PropertyInfo::OutputOnly);

        setWritable(writable);

        m_deviceName =
            declareProperty<StringProperty>("device.name", "", outputOnly);
        m_moduleName = declareProperty<StringProperty>("device.moduleName", "",
                                                       outputOnly);
        m_systemProfileURL = declareProperty<StringProperty>(
            "device.systemProfileURL", outputOnly);
        m_systemProfileType = declareProperty<StringProperty>(
            "device.systemProfileType", outputOnly);
        m_renderHashCount =
            declareProperty<IntProperty>("render.hashCount", 0, notPersistent);
        m_adaptor = newMemberNodeOfType<AdaptorIPNode>("Adaptor", "adaptor");
        m_displayPipelineNode = newMemberNodeOfType<PipelineGroupIPNode>(
            pipelineType, "colorPipeline");
        m_resizeNode = 0;
        m_stereoNode = 0;

        if (resizeType != "")
        {
            m_resizeNode =
                newMemberNodeOfType<ResizeIPNode>(resizeType, "resize");
            m_resizeNode->setProperty<IntProperty>("node.useContext",
                                                   autoResize);
            m_resizeNode->setInputs1(m_adaptor);
        }

        if (stereoNodeType != "")
        {
            m_stereoNode = newMemberNodeOfType<DisplayStereoIPNode>(
                stereoNodeType, "stereo");
            m_stereoNode->setInputs1(m_resizeNode ? (IPNode*)m_resizeNode
                                                  : (IPNode*)m_adaptor);
        }

        if (m_stereoNode)
        {
            m_displayPipelineNode->setInputs1(m_stereoNode);
        }
        else if (m_resizeNode)
        {
            m_displayPipelineNode->setInputs1(m_resizeNode);
        }
        else
        {
            m_displayPipelineNode->setInputs1(m_adaptor);
        }

        setRoot(m_displayPipelineNode);
    }

    DisplayGroupIPNode::~DisplayGroupIPNode()
    {
        if (graph())
            graph()->removeDisplayGroup(this);
    }

    void DisplayGroupIPNode::setOutputVideoDevice(const VideoDevice* d)
    {
        m_outputDevice = d;
    }

    void DisplayGroupIPNode::setPhysicalVideoDevice(const VideoDevice* d)
    {
        m_physicalDevice = d;
        setProperty<StringProperty>(m_deviceName, d ? d->name() : "");
        setProperty<StringProperty>(
            m_moduleName, d && d->module() ? d->module()->name() : "");
        setProperty<StringProperty>(m_systemProfileURL, "");
        setProperty<StringProperty>(m_systemProfileType, "");

        if (d)
        {
            TwkApp::VideoDevice::ColorProfile p = d->colorProfile();
            setProperty<StringProperty>("device.systemProfileURL", p.url);

            switch (p.type)
            {
            case TwkApp::VideoDevice::ICCProfile:
                setProperty<StringProperty>("device.systemProfileType", "ICC");
                break;
            default:
                break;
            }
        }
    }

    bool DisplayGroupIPNode::dualOutputMode() const
    {
        if (m_stereoNode)
        {
            string stype = m_stereoNode->stereoType();

            return stype == "hardware" || stype == "dual"
                   || (m_outputDevice && m_outputDevice->isDualStereo())
                   || (!m_outputDevice && m_physicalDevice
                       && m_physicalDevice->isDualStereo());
        }
        else
        {
            return false;
        }
    }

    void DisplayGroupIPNode::initNewContext(const Context& context,
                                            Context& newContext) const
    {
        const VideoDevice* d = imageDevice();

        if (d)
        {
            VideoDevice::Margins m = d->margins();
            newContext.viewWidth = d->internalWidth() - m.left - m.right;
            newContext.viewHeight = d->internalHeight() - m.top - m.bottom;
            newContext.deviceWidth = newContext.viewWidth;
            newContext.deviceHeight = newContext.viewHeight;

            //
            //  viewOrigin is the abs position of the view origin on the screen,
            //  but y coord from Qt is flipped so unflip here.
            //

            VideoDevice::Offset o = d->offset();
            newContext.viewXOrigin = o.x;
            newContext.viewYOrigin = d->internalHeight() - o.y - 1 - m.bottom;
        }

        newContext.allowInteractiveResize =
            newContext.viewWidth != 0 && newContext.viewHeight != 0;
        newContext.stereo = dualOutputMode();
    }

    IPImage* DisplayGroupIPNode::evaluate(const Context& context)
    {
        IPImage* root = 0;
        Context newContext = context;
        initNewContext(context, newContext);

        const VideoDevice* d = imageDevice();
        bool yuvConvert = false;
        bool flip = false;

        if (d)
        {
            flip = d->capabilities() & VideoDevice::FlippedImage;
            VideoDevice::InternalDataFormat f =
                d->dataFormatAtIndex(d->currentDataFormat()).iformat;
            yuvConvert = f >= VideoDevice::CbY0CrY1_8_422;
        }

        Matrix M;

        if (flip)
        {
            M.makeScale(TwkMath::Vec3f(1, -1, 1));
        }

        if (newContext.stereo)
        {
            Context newContext2 = newContext;
            //
            //  This is done here in the group so that *all* of the
            //  display color transforms are included in the output
            //

            IPImage* left = 0;
            IPImage* right = 0;

            try
            {
                newContext.eye = 0;
                left = m_root->evaluate(newContext);
                if (!yuvConvert)
                    left = insertIntermediateRendersForPaint(left, newContext);
                left->useBackground = true;

                newContext2.eye = 1;
                right = m_root->evaluate(newContext2);
                if (!yuvConvert)
                    right =
                        insertIntermediateRendersForPaint(right, newContext);
                right->useBackground = true;

                if (flip)
                    left->transformMatrix = M * left->transformMatrix;
                if (flip)
                    right->transformMatrix = M * right->transformMatrix;
            }
            catch (...)
            {
                ThreadType thread = newContext.thread;
                TWK_CACHE_LOCK(newContext.cache, "thread=" << thread);
                newContext.cache.checkInAndDelete(left);
                newContext.cache.checkInAndDelete(right);
                TWK_CACHE_UNLOCK(newContext.cache, "thread=" << thread);
                throw;
            }

            IPImage* leftEimage = new IPImage(
                this, imageDevice(), IPImage::ExternalRenderType,
                IPImage::CurrentFrameBuffer, IPImage::Rect2DSampler, false);

            IPImage* rightEimage = new IPImage(
                this, imageDevice(), IPImage::ExternalRenderType,
                IPImage::CurrentFrameBuffer, IPImage::Rect2DSampler, false);

            leftEimage->appendChild(left);
            rightEimage->appendChild(right);

            IPImage* leftTarget =
                new IPImage(this, imageDevice(), IPImage::BlendRenderType,
                            IPImage::LeftBuffer);

            IPImage* rightTarget =
                new IPImage(this, imageDevice(), IPImage::BlendRenderType,
                            IPImage::RightBuffer);

            if (yuvConvert)
            {
                IPImage* leftRgbImage = new IPImage(
                    this, imageDevice(), IPImage::BlendRenderType,
                    IPImage::IntermediateBuffer, IPImage::Rect2DSampler, false);

                IPImage* rightRgbImage = new IPImage(
                    this, imageDevice(), IPImage::BlendRenderType,
                    IPImage::IntermediateBuffer, IPImage::Rect2DSampler, false);

                leftRgbImage->appendChild(leftEimage);
                rightRgbImage->appendChild(rightEimage);

                const Matrix M = TwkMath::Rec709VideoRangeRGBToYUV10<float>();
                leftRgbImage->shaderExpr =
                    Shader::newColorClamp(Shader::newSourceRGBA(leftRgbImage));
                leftRgbImage->shaderExpr =
                    Shader::newColorMatrix(leftRgbImage->shaderExpr, M);
                leftRgbImage->useBackground = true;
                leftRgbImage->hashCount = propertyValue(m_renderHashCount, 0);

                rightRgbImage->shaderExpr =
                    Shader::newColorClamp(Shader::newSourceRGBA(rightRgbImage));
                rightRgbImage->shaderExpr =
                    Shader::newColorMatrix(rightRgbImage->shaderExpr, M);
                rightRgbImage->useBackground = true;
                rightRgbImage->hashCount = leftRgbImage->hashCount;

                leftEimage = leftRgbImage;
                rightEimage = rightRgbImage;

                leftRgbImage->tagMap["yuvconvert"] = "true";
                rightRgbImage->tagMap["yuvconvert"] = "true";
            }

            leftTarget->appendChild(leftEimage);
            leftTarget->shaderExpr = Shader::newSourceRGBA(leftTarget);
            rightTarget->appendChild(rightEimage);
            rightTarget->shaderExpr = Shader::newSourceRGBA(rightTarget);

            root = new IPImage(this, imageDevice(), IPImage::GroupType,
                               IPImage::NoBuffer);

            root->appendChild(leftTarget);
            root->appendChild(rightTarget);
        }
        else
        {

            try
            {
                IPImage* image = m_root->evaluate(newContext);
                if (!yuvConvert)
                    image =
                        insertIntermediateRendersForPaint(image, newContext);

                if (flip)
                    image->transformMatrix = M * image->transformMatrix;
                image->useBackground = true;

                IPImage* eimage = new IPImage(
                    this, imageDevice(), IPImage::ExternalRenderType,
                    IPImage::CurrentFrameBuffer, IPImage::Rect2DSampler, false);

                eimage->appendChild(image);

                if (yuvConvert)
                {
                    IPImage* rgbImage = new IPImage(
                        this, imageDevice(), IPImage::BlendRenderType,
                        IPImage::IntermediateBuffer, IPImage::Rect2DSampler,
                        false);

                    rgbImage->appendChild(eimage);

                    const Matrix M =
                        TwkMath::Rec709VideoRangeRGBToYUV10<float>();
                    rgbImage->shaderExpr =
                        Shader::newColorClamp(Shader::newSourceRGBA(rgbImage));
                    rgbImage->shaderExpr =
                        Shader::newColorMatrix(rgbImage->shaderExpr, M);
                    rgbImage->useBackground = true;
                    rgbImage->hashCount = propertyValue(m_renderHashCount, 0);
                    eimage = rgbImage;

                    rgbImage->tagMap["yuvconvert"] = "true";
                }

                root = new IPImage(this, imageDevice(), IPImage::GroupType,
                                   IPImage::MainBuffer, IPImage::Rect2DSampler,
                                   false);

                root->appendChild(eimage);
            }
            catch (...)
            {
                throw;
            }
        }

        root->recordResourceUsage();

        return root;
    }

    IPImageID* DisplayGroupIPNode::evaluateIdentifier(const Context& context)
    {
        IPImageID* root = 0;
        Context newContext = context;
        initNewContext(context, newContext);

        if (newContext.stereo)
        {
            //
            //  This is done here in the group so that *all* of the
            //  display color transforms are included in the output
            //
            vector<IPImageID*> images;

            try
            {
                newContext.eye = 0;
                images.push_back(m_root->evaluateIdentifier(newContext));

                newContext.eye = 1;
                images.push_back(m_root->evaluateIdentifier(newContext));
            }
            catch (...)
            {
                throw;
            }

            IPImageID* root = new IPImageID();
            root->appendChildren(images);

            return root;
        }
        else
        {
            try
            {
                IPImageID* image = m_root->evaluateIdentifier(newContext);

                root = new IPImageID();
                root->appendChild(image);
            }
            catch (...)
            {
                throw;
            }
        }

        return root;
    }

    void DisplayGroupIPNode::metaEvaluate(const Context& context,
                                          MetaEvalVisitor& visitor)
    {
        if (!m_root)
            return;

        visitor.enter(context, this);

        Context newContext = context;
        initNewContext(context, newContext);

        if (newContext.stereo)
        {
            //
            //  This is done here in the group so that *all* of the
            //  display color transforms are included in the output
            //

            try
            {
                //  Evaluate left Eye
                newContext.eye = 0;
                if (visitor.traverseChild(newContext, 0, this, m_root))
                {
                    m_root->metaEvaluate(newContext, visitor);
                }

                //  Evaluate right Eye
                newContext.eye = 1;
                if (visitor.traverseChild(newContext, 1, this, m_root))
                {
                    m_root->metaEvaluate(newContext, visitor);
                }

                visitor.leave(context, this);
            }
            catch (...)
            {
                throw;
            }

            return;
        }

        if (visitor.traverseChild(newContext, 0, this, m_root))
        {
            m_root->metaEvaluate(newContext, visitor);
        }

        visitor.leave(context, this);
    }

    IPNode::ImageStructureInfo
    DisplayGroupIPNode::imageStructureInfo(const Context& context) const
    {
        Context newContext = context;
        initNewContext(context, newContext);

        if (newContext.viewWidth != 0 && newContext.viewHeight != 0)
        {
            return ImageStructureInfo(newContext.viewWidth,
                                      newContext.viewHeight);
        }
        else
        {
            return GroupIPNode::imageStructureInfo(context);
        }
    }

    void DisplayGroupIPNode::mediaInfo(const Context& context,
                                       MediaInfoVector& infos) const
    {
        Context newContext = context;
        initNewContext(context, newContext);

        if (newContext.viewWidth != 0 && newContext.viewHeight != 0)
        {
            return;
        }
        else
        {
            GroupIPNode::mediaInfo(context, infos);
        }
    }

    void DisplayGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        //
        //  We shouldn't have more than one input
        //

        if (newInputs.size() > 1)
        {
            cout << "WARNING: DisplayGroupIPNode: more than one input" << endl;
        }

        if (newInputs.size() == 1)
        {
            m_adaptor->setGroupInputNode(newInputs.front());
        }
        else
        {
            m_adaptor->setGroupInputNode(0);
        }

        IPNode::setInputs(newInputs);
    }

    const TwkApp::VideoDevice* DisplayGroupIPNode::imageDevice() const
    {
        return m_outputDevice ? m_outputDevice : m_physicalDevice;
    }

    void DisplayGroupIPNode::incrementRenderHashCount()
    {
        setProperty(m_renderHashCount, propertyValue(m_renderHashCount, 0) + 1);
    }

} // namespace IPCore
