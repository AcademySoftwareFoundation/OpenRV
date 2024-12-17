//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/StackIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/Exception.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <TwkAudio/Audio.h>
#include <TwkMath/Function.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;
    using namespace TwkAudio;
    using namespace std;

// NOTE: SUBOPTIMAL: the value here MUST be smaller or equal to the value of
// MAX_TEXTURE_PER_SHADER in ShaderCommon.cpp Currently there are built in
// versions of over, difference, etc. that takes anywhere between 2 and
// MAX_TEXTURE_PER_BLEND inputs.
#define MAX_TEXTURE_PER_BLEND 4

    string StackIPNode::m_defaultCompType("over");

    namespace
    {
        bool useMerge = getenv("TWK_STACKIPNODE_USE_MERGE") != 0;
        bool useMergeForReplace =
            (getenv("TWK_STACKIPNODE_USE_MERGE_FOR_REPLACE") == 0
             || getenv("TWK_STACKIPNODE_USE_MERGE_FOR_REPLACE")[0] != '0');
    } // namespace

    StackIPNode::StackIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_offset(0)
        , m_fit(true)
        , m_rangeInfoDirty(false)
        , m_activeAudioInputIndex(-2)
    {
        pthread_mutex_init(&m_lock, 0);
        setHasLinearTransform(m_fit);

        m_outputFPS = declareProperty<FloatProperty>("output.fps", 0.0f);
        m_outputSize = createProperty<IntProperty>("output.size");
        m_outputSize->resize(2);
        m_outputSize->front() = 720;
        m_outputSize->back() = 480;

        m_useCutInfo = declareProperty<IntProperty>("mode.useCutInfo", 1);
        m_alignStartFrames =
            declareProperty<IntProperty>("mode.alignStartFrames", 0);
        m_autoSize = declareProperty<IntProperty>("output.autoSize", 1);
        m_activeAudioInput =
            declareProperty<StringProperty>("output.chosenAudioInput", ".all.");
        m_strictFrameRanges =
            declareProperty<IntProperty>("mode.strictFrameRanges", 0);
        m_interactiveSize =
            declareProperty<IntProperty>("output.interactiveSize", 0);
        m_supportReversedOrderBlending = declareProperty<IntProperty>(
            "mode.supportReversedOrderBlending", 1);

        if (m_defaultCompType == "layer")
        {
            m_defaultCompType = "topmost";
            if (this->name() == "defaultStack_stack")
            {
                m_strictFrameRanges->front() = 1;
                m_activeAudioInput->front() = ".topmost.";
            }
        }

        m_compMode = declareProperty<StringProperty>(
            "composite.type", (this->name() == "defaultStack_stack"
                               || this->name() == "defaultLayout_stack")
                                  ? m_defaultCompType
                                  : "over");
    }

    StackIPNode::~StackIPNode() { pthread_mutex_destroy(&m_lock); }

    bool StackIPNode::interactiveSize(const Context& c) const
    {
        return m_interactiveSize->front() != 0 && graph()->viewNode() == group()
               && c.allowInteractiveResize && c.viewWidth != 0
               && c.viewHeight != 0;
    }

    void StackIPNode::updateChosenAudioInput()
    {
        m_activeAudioInputIndex = -2;

        if (m_activeAudioInput->front() == ".topmost.")
            m_activeAudioInputIndex = -3;
        else if (m_activeAudioInput->front() == ".all.")
            m_activeAudioInputIndex = -2;
        else if (m_activeAudioInput->front() == ".first.")
            m_activeAudioInputIndex = -1;
        else
        {
            const IPNodes& ins = inputs();

            for (int i = 0; i < ins.size(); ++i)
            {
                const IPNode* node = ins[i];
                while (node->inputs().size())
                    node = node->inputs()[0];

                if (const AdaptorIPNode* n =
                        dynamic_cast<const AdaptorIPNode*>(node))
                {
                    node = n->groupInputNode();
                    if (node->name() == m_activeAudioInput->front())
                    {
                        m_activeAudioInputIndex = i;
                        break;
                    }
                }
            }
        }
        graph()->flushAudioCache();
    }

    void StackIPNode::setInputs(const IPNodes& nodes)
    {
        IPNode::setInputs(nodes);

        if (!isDeleting())
        {
            computeRanges();
            propagateRangeChange();

            updateChosenAudioInput();
        }
    }

    void StackIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_useCutInfo || p == m_alignStartFrames
                || p == m_strictFrameRanges || p == m_outputFPS)
            {
                lock();
                m_rangeInfoDirty = true;
                unlock();
                propagateRangeChange();
                invalidate();
                graph()->flushAudioCache();

                if (group())
                    group()->propertyChanged(p);
            }

            if (p == m_autoSize || p == m_outputSize)
            {
                lock();
                m_structureInfoDirty = true;
                unlock();
                propagateImageStructureChange();
                invalidate();
            }

            if (p == m_activeAudioInput)
                updateChosenAudioInput();
        }

        IPNode::propertyChanged(p);
    }

    void StackIPNode::lazyUpdateRanges() const
    {
        lock();
        if (m_rangeInfoDirty || m_structureInfoDirty)
        {
            const_cast<StackIPNode*>(this)->computeRanges();
        }
        unlock();
    }

    void StackIPNode::inputRangeChanged(int inputIndex, PropagateTarget target)
    {
        lock();
        m_rangeInfoDirty = true;
        unlock();
    }

    void StackIPNode::inputImageStructureChanged(int inputIndex,
                                                 PropagateTarget target)
    {
        lock();
        m_structureInfoDirty = true;
        unlock();
    }

    IPNode::ImageStructureInfo
    StackIPNode::imageStructureInfo(const Context& context) const
    {
        if (interactiveSize(context))
        {
            return ImageStructureInfo(context.viewWidth, context.viewHeight);
        }
        else
        {
            lazyUpdateRanges();
            return m_structureInfo;
        }
    }

    void StackIPNode::invalidate()
    {
        lock();
        m_rangeInfoDirty = true;
        unlock();

        propagateRangeChange();
    }

    void StackIPNode::computeRanges()
    {
        if (isDeleting())
            return;

        // cout << this << " computeRanges()" << endl;
        //
        //   NOTE: this function computes the range for this node based on
        //   the first input as the start. At the end we compute an offset
        //   from m_info so that the result range is actually 1 to whatever
        //

        m_rangeInfos.resize(inputs().size());

        const bool useCutInfo = m_useCutInfo->front();
        const bool alignStartFrames = m_alignStartFrames->front();
        int maxDuration = 0;

        for (int i = 0; i < inputs().size(); i++)
        {
            m_rangeInfos[i] = inputs()[i]->imageRangeInfo();
            ImageStructureInfo sinfo = inputs()[i]->imageStructureInfo(
                graph()->contextForFrame(m_rangeInfos[i].start));

            if (!i)
            {
                m_info = m_rangeInfos[i];
                m_structureInfo.width = sinfo.width;
                m_structureInfo.height = sinfo.height;
                m_structureInfo.pixelAspect = 1.0;

                if (useCutInfo)
                {
                    m_info.start = m_info.cutIn;
                    m_info.end = m_info.cutOut;
                }
                maxDuration = m_info.end - m_info.start + 1;
            }
            else
            {
                m_structureInfo.width =
                    max(m_structureInfo.width, int(sinfo.width));
                m_structureInfo.height =
                    max(m_structureInfo.height, int(sinfo.height));

                int duration = 0;

                if (useCutInfo)
                {
                    m_info.start = min(m_info.start, m_rangeInfos[i].cutIn);
                    m_info.end = max(m_info.end, m_rangeInfos[i].cutOut);
                    duration =
                        m_rangeInfos[i].cutOut - m_rangeInfos[i].cutIn + 1;
                }
                else
                {
                    m_info.start = min(m_info.start, m_rangeInfos[i].start);
                    m_info.end = max(m_info.end, m_rangeInfos[i].end);
                    duration = m_rangeInfos[i].end - m_rangeInfos[i].start + 1;
                }
                maxDuration = max(maxDuration, duration);
            }
        }

        if (alignStartFrames)
            m_info.end = m_info.start + maxDuration - 1;

        m_info.cutIn = m_info.start;
        m_info.cutOut = m_info.end;
        m_offset = m_info.start - 1;

        if (m_outputFPS->front() == 0.0 && !m_rangeInfos.empty()
            && !m_rangeInfos[0].isUndiscovered)
        {
            m_outputFPS->front() = m_rangeInfos[0].fps;
        }

        if (m_autoSize->front())
        {
            m_outputSize->front() = m_structureInfo.width;
            m_outputSize->back() = m_structureInfo.height;
        }
        else
        {
            m_structureInfo.width = m_outputSize->front();
            m_structureInfo.height = m_outputSize->back();
        }

        m_info.fps = m_outputFPS->front();

        //
        //  Compute the cut in/out of each input in global frames and
        //  cache it so we can mask audio outside the range.
        //

        FrameVector gin(2);
        m_globalRanges.resize(m_rangeInfos.size());

        for (size_t i = 0; i < inputs().size(); i++)
        {
            FrameVector gout;
            gin[0] = useCutInfo ? m_rangeInfos[i].cutIn : m_rangeInfos[i].start;
            gin[1] = useCutInfo ? m_rangeInfos[i].cutOut : m_rangeInfos[i].end;
            mapInputToEvalFramesInternal(i, gin, gout);
            //
            //  m_info.start is already taken into account.
            //
            //  m_globalRanges[i] = FrameRange(max(0, gout[0] - m_info.start),
            //  max(0, gout[1] - m_info.start));
            m_globalRanges[i] = FrameRange(gout[0], gout[1]);
        }

        m_rangeInfoDirty = false;
        m_structureInfoDirty = false;
    }

    int StackIPNode::inputFrame(size_t index, int frame, bool unconstrained)
    {
        const ImageRangeInfo& info = m_rangeInfos[index];
        const bool useCutInfo = m_useCutInfo->front();
        const bool alignStartFrames = m_alignStartFrames->front();

        frame += m_offset;

        if (!unconstrained)
        {
            if (frame < m_info.start)
                frame = m_info.start;
            if (frame > m_info.end)
                frame = m_info.end;
        }

        if (alignStartFrames)
        {
            if (useCutInfo)
            {
                int offset = frame - m_info.start;
                int f = info.cutIn + offset;
                if (!unconstrained && f > info.cutOut)
                    f = info.cutOut;
                return f;
            }
            else
            {
                int offset = frame - m_info.start;
                int f = info.start + offset;
                if (!unconstrained && f > info.end)
                    f = info.end;
                return f;
            }
        }
        else if (!unconstrained)
        {
            if (useCutInfo)
            {
                if (frame < info.cutIn)
                    return info.cutIn;
                if (frame > info.cutOut)
                    return info.cutOut;
            }
            else
            {
                if (frame < info.start)
                    return info.start;
                if (frame > info.end)
                    return info.end;
            }
        }

        return frame;
    }

    //
    // This is the more complex case where we need to alter the IPImage tree
    // Every time we reach MAX_TEXTURE_PER_BLEND children of root, we insert an
    // IPImage that will (1) be the parent of a few input IPImages (2) hold the
    // evaluation results of the merge expression of the input IPImages If the
    // IPImage tree was like so before this process (assuming
    // MAX_TEXTURE_PER_BLEND is 4):
    //
    //           |----- B
    //           |----- C
    //           |----- D
    //  root <---|----- E
    //           |----- F
    //           |----- G
    //
    // after this insertion process it will become
    //
    //                    |----- B
    //           |--K<----|----- C
    //           |        |----- D
    //  root <---|        |----- E
    //           |----- F
    //           |----- G
    //
    // and the mega expr will be over3(over4(B, C, D, E), F, G)
    // where over(B, C, D, E) will be "rendered" into newly inserted IPImage K
    //

    IPImage* StackIPNode::collapseInputs(const Context& context,
                                         IPImage::BlendMode mode,
                                         IPImageVector& images,
                                         IPImageSet& modifiedImages)
    {
        size_t n = images.size();

        if (n <= MAX_TEXTURE_PER_BLEND)
        {
            const bool interactive = interactiveSize(context);
            const float vw =
                interactive ? context.viewWidth : m_structureInfo.width;
            const float vh =
                interactive ? context.viewHeight : m_structureInfo.height;

            IPImage* root = new IPImage(this, IPImage::MergeRenderType, vw, vh,
                                        1.0, IPImage::TemporaryBuffer);

            Shader::ExpressionVector inExpressions;
            balanceResourceUsage(IPNode::accumulate, images, modifiedImages, 8,
                                 8, 81);
            assembleMergeExpressions(root, images, modifiedImages, false,
                                     inExpressions);

            root->appendChildren(images);
            root->mergeExpr = newBlend(root, inExpressions, mode);
            root->shaderExpr = Shader::newSourceRGBA(root);
            root->recordResourceUsage();

            return root;
        }
        else
        {
            IPImageVector newImages;

            for (size_t q = 0; q < n; q += MAX_TEXTURE_PER_BLEND)
            {
                IPImageVector subImages;

                for (size_t i = 0; i < MAX_TEXTURE_PER_BLEND && (q + i) < n;
                     i++)
                {
                    subImages.push_back(images[q + i]);
                }

                newImages.push_back(
                    collapseInputs(context, mode, subImages, modifiedImages));
            }

            return collapseInputs(context, mode, newImages, modifiedImages);
        }
    }

    IPImage* StackIPNode::evaluate(const Context& context)
    {
        lazyUpdateRanges();

        const IPNodes& nodes = inputs();
        const bool interactive = interactiveSize(context);
        const float vw =
            interactive ? context.viewWidth : m_structureInfo.width;
        const float vh =
            interactive ? context.viewHeight : m_structureInfo.height;
        const float aspect = float(vw) / float(vh);

        const char* comp = "";

        if (StringProperty* sp = m_compMode)
        {
            if (sp->size())
                comp = sp->front().c_str();
        }

        const IPImage::BlendMode mode = IPImage::getBlendModeFromString(comp);

        const bool topmostOnly = !strcmp(comp, "topmost");

        const bool localUseMerge =
            useMerge
            || (mode == IPImage::Replace && !topmostOnly && useMergeForReplace);

        IPImage* root = 0;

        if (localUseMerge)
        {
            root = new IPImage(this, IPImage::MergeRenderType, vw, vh, 1.0,
                               IPImage::IntermediateBuffer);
        }
        else
        {
            root = new IPImage(this, IPImage::BlendRenderType, vw, vh, 1.0);
        }

        // Make sure to disable reverse-order blending if required.
        // See ImageRenderer::renderAllChildren for more details on this mode.
        root->supportReversedOrderBlending =
            m_supportReversedOrderBlending->front() != 0;

        if (nodes.empty())
            return root;

        int ninputs = nodes.size();
        int frame = context.frame;

        const bool strictFrameRanges = m_strictFrameRanges->front();
        const bool useCutInfo = m_useCutInfo->front();

        IPImageVector images;
        IPImageSet modifiedImages;

        try
        {
            bool haveOneImage = false;

            for (unsigned int i = 0; i < ninputs; i++)
            {
                if (strictFrameRanges)
                {
                    int inF = inputFrame(i, frame, true);
                    const ImageRangeInfo& info = m_rangeInfos[i];

                    if (useCutInfo)
                    {
                        if (inF < info.cutIn || inF > info.cutOut)
                            continue;
                    }
                    else
                    {
                        if (inF < info.start || inF > info.end)
                            continue;
                    }
                }

                if (topmostOnly)
                {
                    if (haveOneImage)
                        break;
                    haveOneImage = true;
                }

                Context c = context;
                c.fps = m_outputFPS->front();
                c.frame = inputFrame(i, frame);

                IPImage* current = nodes[i]->evaluate(c);

                if (!current)
                {
                    // continue;
                    IPNode* node = nodes[i];
                    TWK_THROW_STREAM(EvaluationFailedExc,
                                     "StackIPNode evaluation failed on node "
                                         << node->name());
                }
                else if (current->isNoImage() || current->isBlank())
                {
                    delete current;
                }
                else
                {
                    //
                    //  Fit large aspect ratios into our output aspect
                    //

                    if (m_fit)
                        current->fitToAspect(aspect);

                    images.push_back(current);
                }
            }
        }
        catch (std::exception&)
        {
            ThreadType thread = context.thread;

            TWK_CACHE_LOCK(context.cache, "thread=" << thread);
            context.cache.checkInAndDelete(images);
            TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
            delete root;
            throw;
        }

        if (localUseMerge)
        {
            convertBlendRenderTypeToIntermediate(images, modifiedImages);

            for (size_t i = 0; i < images.size(); i++)
            {
                IPImage* image = images[i];

                if (!image->stencilBox.isEmpty())
                {
                    //
                    //  Wipe crop box shader.   If we are "replacing" then we
                    //  need to indicate the parts of the image outside the box
                    //  with alpha=-1.
                    //

                    if (mode == IPImage::Replace)
                    {
                        image->shaderExpr = Shader::newStencilBoxNegAlpha(
                            image, image->shaderExpr);
                    }
                    else
                    {
                        image->shaderExpr =
                            Shader::newStencilBox(image, image->shaderExpr);
                    }

                    size_t newWidth = image->width;
                    size_t newHeight = image->height;
                    IPImage* newCurrent = new IPImage(
                        this, IPImage::BlendRenderType, newWidth, newHeight,
                        1.0, IPImage::IntermediateBuffer);
                    newCurrent->children = image;
                    newCurrent->shaderExpr = Shader::newSourceRGBA(newCurrent);
                    images[i] = newCurrent;
                }
            }

            if (ninputs == 1)
            {
                Shader::ExpressionVector inExpressions;
                balanceResourceUsage(IPNode::accumulate, images, modifiedImages,
                                     8, 8, 81);
                assembleMergeExpressions(root, images, modifiedImages, false,
                                         inExpressions);

                root->mergeExpr = inExpressions.front();
                root->shaderExpr = Shader::newSourceRGBA(root);
                root->appendChildren(images);
            }
            else
            {
                delete root;
                root = collapseInputs(context, mode, images, modifiedImages);
                root->destination = IPImage::IntermediateBuffer;
            }
        }
        else
        {
            for (size_t i = 0; i < images.size(); i++)
            {
                IPImage* image = images[i];

                if (!image->stencilBox.isEmpty())
                {
                    convertBlendRenderTypeToIntermediate(image);
                    image->shaderExpr =
                        Shader::newStencilBox(image, image->shaderExpr);

                    size_t newWidth = image->width;
                    size_t newHeight = image->height;
                    IPImage* newCurrent = new IPImage(
                        this, IPImage::BlendRenderType, newWidth, newHeight,
                        1.0, IPImage::IntermediateBuffer);
                    newCurrent->children = image;
                    newCurrent->shaderExpr = Shader::newSourceRGBA(newCurrent);
                    images[i] = newCurrent;
                }
            }
            root->appendChildren(images);
        }

        root->blendMode = mode;
        root->recordResourceUsage();

        return root;
    }

    IPImageID* StackIPNode::evaluateIdentifier(const Context& context)
    {
        lazyUpdateRanges();
        const IPNodes& nodes = inputs();
        const int ninputs = inputs().size();
        const int frame = context.frame;
        const bool interactive = interactiveSize(context);
        const float vw =
            interactive ? context.viewWidth : m_structureInfo.width;
        const float vh =
            interactive ? context.viewHeight : m_structureInfo.height;
        const float aspect = float(vw) / float(vh);

        if (nodes.empty())
            return 0;

        IPImageID* root = new IPImageID();
        IPImageID* last = 0;

        const char* comp = "";

        if (StringProperty* sp = m_compMode)
        {
            if (sp->size())
                comp = sp->front().c_str();
        }

        const bool topmostOnly = (!strcmp(comp, "topmost"));
        const bool strictFrameRanges = m_strictFrameRanges->front();
        const bool useCutInfo = m_useCutInfo->front();

        //
        //  Loop over the inputs starting at the top index
        //  If restricted() than quit after the first one
        //

        try
        {
            ostringstream idstr;
            bool haveOneImage = false;

            for (size_t i = 0; i < ninputs; i++)
            {
                if (strictFrameRanges)
                {
                    int inF = inputFrame(i, frame, true);
                    const ImageRangeInfo& info = m_rangeInfos[i];

                    if (useCutInfo)
                    {
                        if (inF < info.cutIn || inF > info.cutOut)
                            continue;
                    }
                    else
                    {
                        if (inF < info.start || inF > info.end)
                            continue;
                    }
                }
                if (topmostOnly)
                {
                    if (haveOneImage)
                        break;
                    haveOneImage = true;
                }

                Context c = context;
                c.frame = inputFrame(i, frame);

                IPImageID* imgid = nodes[i]->evaluateIdentifier(c);

                if (!imgid)
                {
                    // continue;
                    IPNode* node = nodes[i];
                    TWK_THROW_STREAM(EvaluationFailedExc,
                                     "NULL imgid in StackIPNode "
                                         << name() << " failed on input "
                                         << node->name());
                }

                if (i)
                    idstr << "+";
                idstr << imgid->id;

                if (root->children)
                    last->next = imgid;
                else
                    root->children = imgid;

                last = imgid;
            }

            root->id = idstr.str();
        }
        catch (...)
        {
            delete root;
            throw;
        }

        return root;
    }

    void StackIPNode::testEvaluate(const Context& context,
                                   TestEvaluationResult& result)
    {
        lazyUpdateRanges();
        IPNodes ins = inputs();
        int ninputs = inputs().size();
        int frame = context.frame;
        IPImage* img = 0;

        for (unsigned int i = 0; i < ninputs; i++)
        {
            int f = frame;
            Context c = context;
            const ImageRangeInfo& info = m_rangeInfos[i];
            c.frame = inputFrame(i, frame);

            ins[i]->testEvaluate(context, result);
        }
    }

    void StackIPNode::metaEvaluate(const Context& context,
                                   MetaEvalVisitor& visitor)
    {
        lazyUpdateRanges();
        visitor.enter(context, this);

        IPNodes ins = inputs();
        int ninputs = inputs().size();
        IPImage* img = 0;

        for (unsigned int i = 0; i < ninputs; i++)
        {
            const ImageRangeInfo& info = m_rangeInfos[i];
            Context c = context;
            c.frame = inputFrame(i, context.frame);

            if (visitor.traverseChild(c, i, this, ins[i]))
            {
                ins[i]->metaEvaluate(c, visitor);
            }
        }

        visitor.leave(context, this);
    }

    IPNode::ImageRangeInfo StackIPNode::imageRangeInfo() const
    {
        lazyUpdateRanges();

        ImageRangeInfo info = m_info;
        info.start -= m_offset;
        info.end -= m_offset;
        info.cutIn -= m_offset;
        info.cutOut -= m_offset;
        return info;
    }

    size_t StackIPNode::audioFillBuffer(const AudioContext& context)
    {
        lazyUpdateRanges();

        if (inputs().empty())
            return 0;

        //
        //  Sum up the audio from all of the stacked nodes
        //

        size_t rval = 0;

        AudioBuffer audioBuffer(
            context.buffer.size(), context.buffer.channels(),
            context.buffer.rate(), context.buffer.startTime());

        for (size_t i = 0; i < inputs().size(); i++)
        {
            //
            //  If this is not the named input or .topmost./.first. then keep
            //  searching
            //

            if (m_activeAudioInputIndex >= 0 && m_activeAudioInputIndex != i)
                continue;

            const long foffset = inputFrame(i, m_info.start - m_offset, true)
                                 - m_rangeInfos[i].start;
            const Time soffset = double(foffset) / context.fps;
            Time contextStart = samplesToTime(context.buffer.startSample(),
                                              context.buffer.rate());

            AudioContext subContext(audioBuffer, context.fps);
            subContext.buffer.setStartTime(soffset + contextStart);
            subContext.buffer.zero();

            rval = max(inputs()[i]->audioFillBuffer(subContext), rval);

            transform(context.buffer.pointer(),
                      context.buffer.pointer() + context.buffer.sizeInFloats(),
                      subContext.buffer.pointer(), context.buffer.pointer(),
                      plus<float>());

            //
            //  If this was the .first. input to have audio then we are done.
            //  Otherwise we need to keep filling
            //

            if (m_activeAudioInputIndex == -3 && rval > 0)
                break;

            //
            //  Either the .topmost. had some audio or it didn't
            //

            if (m_activeAudioInputIndex == -1)
                break;
        }

        return rval;
    }

    void StackIPNode::mapInputToEvalFrames(size_t inputIndex,
                                           const FrameVector& inframes,
                                           FrameVector& outframes) const
    {
        lazyUpdateRanges();
        mapInputToEvalFramesInternal(inputIndex, inframes, outframes);
    }

    void StackIPNode::mapInputToEvalFramesInternal(size_t inputIndex,
                                                   const FrameVector& inframes,
                                                   FrameVector& outframes) const
    {
        const ImageRangeInfo& info = m_rangeInfos[inputIndex];
        const bool useCutInfo = m_useCutInfo->front();
        const bool alignStartFrames = m_alignStartFrames->front();

        if (alignStartFrames)
        {
            for (size_t i = 0; i < inframes.size(); i++)
            {
                int start = (useCutInfo) ? info.cutIn : info.start;
                outframes.push_back((inframes[i] - start + m_info.start)
                                    - m_offset);
            }
        }
        else
        {
            for (size_t i = 0; i < inframes.size(); i++)
            {
                outframes.push_back(inframes[i] - m_offset);
            }
        }
    }

    void StackIPNode::readCompleted(const string& typeName,
                                    unsigned int version)
    {
        updateChosenAudioInput();

        IPNode::readCompleted(typeName, version);
    }

    void StackIPNode::propagateFlushToInputs(const FlushContext& context)
    {
        Context c = context;

        for (int i = 0; i < inputs().size(); ++i)
        {
            c.frame = inputFrame(i, context.frame);
            inputs()[i]->propagateFlushToInputs(c);
        }
    }

} // namespace IPCore
