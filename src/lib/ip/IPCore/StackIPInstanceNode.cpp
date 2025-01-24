//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/AdaptorIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/StackIPInstanceNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/ShaderUtil.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderFunction.h>
#include <TwkAudio/Audio.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Function.h>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;
    using namespace TwkAudio;
    using namespace std;

    StackIPInstanceNode::StackIPInstanceNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* g, GroupIPNode* group)
        : IPInstanceNode(name, def, g, group)
        , m_offset(0)
        , m_fit(false)
        , m_rangeInfoDirty(false)
        , m_activeAudioInputIndex(-2)
    {
        pthread_mutex_init(&m_lock, 0);

        m_useCutInfo = declareProperty<IntProperty>("timing.useCutInfo", 1);
        m_alignStartFrames =
            declareProperty<IntProperty>("timing.alignStartFrames", 0);
        m_outOfRangePolicyProp =
            declareProperty<StringProperty>("output.outOfRangePolicy", "hold");
        m_autoSize = declareProperty<IntProperty>("output.autoSize", 1);
        m_activeAudioInput =
            declareProperty<StringProperty>("output.activeAudioInput", ".all.");
        m_outputFPS = declareProperty<FloatProperty>("output.fps", 0.0f);

        m_outputSize = createProperty<IntProperty>("output", "size");
        m_outputSize->resize(2);
        m_outputSize->front() = 720;
        m_outputSize->back() = 480;

        setHasLinearTransform(m_fit);

        updateOutOfRangePolicy();
    }

    StackIPInstanceNode::~StackIPInstanceNode()
    {
        pthread_mutex_destroy(&m_lock);
    }

    void StackIPInstanceNode::updateOutOfRangePolicy()
    {
        string policy = m_outOfRangePolicyProp->front();

        if (policy == "blank")
            m_outOfRangePolicy = NoImageOutOfRange;
        else if (policy == "black")
            m_outOfRangePolicy = BlackOutOfRange;
        else if (policy == "hold")
            m_outOfRangePolicy = HoldOutOfRange;
        else
            m_outOfRangePolicy = NoImageOutOfRange;
    }

    void StackIPInstanceNode::updateActiveAudioInput()
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

    void StackIPInstanceNode::setInputs(const IPNodes& nodes)
    {
        IPInstanceNode::setInputs(nodes);

        if (!isDeleting())
        {
            computeRanges();
            propagateRangeChange();
            updateActiveAudioInput();
        }
    }

    void StackIPInstanceNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_useCutInfo || p == m_alignStartFrames
                || p == m_outOfRangePolicyProp || p == m_outputFPS)
            {
                lock();
                m_rangeInfoDirty = true;
                updateOutOfRangePolicy();
                unlock();
                propagateRangeChange();
                // computeRanges();

                if (group())
                    group()->propertyChanged(p);
            }

            if (p == m_autoSize || p == m_outputSize)
            {
                lock();
                m_structureInfoDirty = true;
                unlock();
                propagateImageStructureChange();
                // computeRanges();
            }

            if (p == m_activeAudioInput)
                updateActiveAudioInput();
        }

        IPInstanceNode::propertyChanged(p);
    }

    void StackIPInstanceNode::lazyUpdateRanges() const
    {
        lock();

        if (m_rangeInfoDirty || m_structureInfoDirty)
        {
            try
            {
                const_cast<StackIPInstanceNode*>(this)->computeRanges();
            }
            catch (...)
            {
                unlock();
                throw;
            }
        }

        unlock();
    }

    void StackIPInstanceNode::inputRangeChanged(int inputIndex,
                                                PropagateTarget target)
    {
        lock();
        m_rangeInfoDirty = true;
        unlock();
    }

    void StackIPInstanceNode::inputImageStructureChanged(int inputIndex,
                                                         PropagateTarget target)
    {
        lock();
        m_structureInfoDirty = true;
        unlock();
    }

    IPNode::ImageStructureInfo
    StackIPInstanceNode::imageStructureInfo(const Context& context) const
    {
        lazyUpdateRanges();
        return m_structureInfo;
    }

    void StackIPInstanceNode::computeRanges()
    {
        if (isDeleting())
            return;

        //
        //  NOTE: this function computes the range for this node based on
        //  the first input as the start. At the end we compute an offset
        //  from m_info so that the result range is actually 1 to whatever
        //

        m_rangeInfos.resize(inputs().size());
        m_structInfos.resize(inputs().size());

        const bool useCutInfo = m_useCutInfo->front();
        const bool alignStartFrames = m_alignStartFrames->front();
        int maxDuration = 0;

        for (int i = 0; i < inputs().size(); i++)
        {
            m_rangeInfos[i] = inputs()[i]->imageRangeInfo();
            m_structInfos[i] = inputs()[i]->imageStructureInfo(
                graph()->contextForFrame(m_rangeInfos[i].start));
            const ImageStructureInfo& sinfo = m_structInfos[i];

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

        if (m_outputFPS->front() == 0.0 && !m_rangeInfos.empty())
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

    int StackIPInstanceNode::inputFrame(size_t index, int frame,
                                        bool unconstrained)
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

    IPImage* StackIPInstanceNode::evaluate(const Context& context)
    {
        const Shader::Function* F = definition()->function();
        if (!isActive())
            return IPInstanceNode::evaluate(context);
        lazyUpdateRanges();

        IPNodes nodes = inputs();
        const int ninputs = nodes.size();

        if (ninputs != F->imageParameters().size())
        {
            return IPImage::newNoImage(this, "Missing Inputs");
        }

        int width = m_structureInfo.width;
        int height = m_structureInfo.height;
        int frame = context.frame;
        float aspect = float(width) / float(height);
        bool useCutInfo = m_useCutInfo->front();

        IPImage* root =
            new IPImage(this, IPImage::MergeRenderType, width, height, 1.0,
                        IPImage::IntermediateBuffer, IPImage::FloatDataType);

        if (ninputs == 0)
            return root;

        vector<Shader::Expression*> inExpressions;
        IPImageVector images;
        IPImageSet modifiedImages;

        try
        {
            IPImage* current = 0;

            //
            //  This loop over the inputs evaluates them and applies the
            //  out of range policy and fit to aspect transform
            //

            for (unsigned int i = 0; i < ninputs; i++)
            {
                int inFrame = inputFrame(i, frame, true);
                const ImageRangeInfo& info = m_rangeInfos[i];

                const bool outOfRange =
                    useCutInfo ? (inFrame < info.cutIn || inFrame > info.cutOut)
                               : (inFrame < info.start || inFrame > info.end);

                if (outOfRange
                    && (m_outOfRangePolicy == NoImageOutOfRange
                        || m_outOfRangePolicy == BlackOutOfRange))
                {
                    const ImageStructureInfo& sinfo = m_structInfos[i];

                    //
                    //  Because this fb is not marked as being "cached"
                    //  its going to be deleted when the IPImage is. The
                    //  image data passed in is either a blank or black
                    //  pixel with a display window of the usual image
                    //  size.
                    //
                    //  The pointer value of the static data is used as
                    //  the identifier.
                    //

                    if (m_outOfRangePolicy == NoImageOutOfRange)
                    {
                        current = IPImage::newBlankImage(this, sinfo.width,
                                                         sinfo.height);
                    }
                    else
                    {
                        current = IPImage::newBlackImage(this, sinfo.width,
                                                         sinfo.height);
                    }
                }
                else
                {
                    Context c = context;
                    c.fps = m_outputFPS->front();
                    c.frame = inputFrame(i, frame);

                    current = nodes[i]->evaluate(c);
                }

                if (!current)
                {
                    // continue;
                    IPNode* node = nodes[i];
                    TWK_THROW_STREAM(
                        EvaluationFailedExc,
                        "MergeIPInstanceNode evaluation failed on node "
                            << node->name());
                }

                if (m_fit)
                    current->fitToAspect(aspect);

                images.push_back(current);
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

        addFillerInputs(images);

        //
        //  If there are virtual images (like a stack, layout, etc) make
        //  them intermediate. modified images will appear in
        //  modifiedImages. MergeRenderType cannot interact with
        //  BlendRenderType children so convert them if necessary.
        //

        convertBlendRenderTypeToIntermediate(images, modifiedImages);

        //
        //  modifiedImages is futher changed when balancing resources if
        //  necessary.
        //

        balanceResourceUsage(F->isFilter() ? IPNode::filterAccumulate
                                           : IPNode::accumulate,
                             images, modifiedImages, 8, 8, 81);

        //
        //  This function will prepare the root IPImage node and images
        //  for MergeRenderType, the inExpressions vector will contain the
        //  args for the merge function. The modifiedImages are *not*
        //  touched.
        //

        assembleMergeExpressions(root, images, modifiedImages, F->isFilter(),
                                 inExpressions);

        //
        //  Bind final merge expression and add children
        //

        root->appendChildren(images);
        root->mergeExpr =
            bind(root, inExpressions, context); // IPInstanceNode::bind
        root->shaderExpr = Shader::newSourceRGBA(root);

        //
        //  Compute the resource usage of the finished IPImage
        //

        root->recordResourceUsage();

        return root;
    }

    IPImageID* StackIPInstanceNode::evaluateIdentifier(const Context& context)
    {
        lazyUpdateRanges();
        IPNodes nodes = inputs();
        int ninputs = inputs().size();
        int frame = context.frame;
        int width = m_structureInfo.width;
        int height = m_structureInfo.height;
        float aspect = float(width) / float(height);

        if (nodes.empty())
            return 0;

        IPImageID* root = new IPImageID();
        IPImageID* last = 0;

        const bool useCutInfo = m_useCutInfo->front();

        //
        //  Loop over the inputs starting at the top index
        //  If restricted() than quit after the first one
        //

        try
        {
            ostringstream idstr;

            for (size_t i = 0; i < ninputs; i++)
            {
                if (m_outOfRangePolicy == NoImageOutOfRange)
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

                Context c = context;
                c.frame = inputFrame(i, frame);

                IPImageID* imgid = nodes[i]->evaluateIdentifier(c);

                if (!imgid)
                {
                    // continue;
                    IPNode* node = nodes[i];
                    TWK_THROW_STREAM(EvaluationFailedExc,
                                     "NULL imgid in StackIPInstanceNode "
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

    void StackIPInstanceNode::testEvaluate(const Context& context,
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

    void StackIPInstanceNode::metaEvaluate(const Context& context,
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
            c.frame = inputFrame(i, c.frame);

            if (visitor.traverseChild(c, i, this, ins[i]))
            {
                ins[i]->metaEvaluate(c, visitor);
            }
        }

        visitor.leave(context, this);
    }

    IPNode::ImageRangeInfo StackIPInstanceNode::imageRangeInfo() const
    {
        lazyUpdateRanges();

        ImageRangeInfo info = m_info;
        info.start -= m_offset;
        info.end -= m_offset;
        info.cutIn -= m_offset;
        info.cutOut -= m_offset;
        return info;
    }

    size_t StackIPInstanceNode::audioFillBuffer(const AudioContext& context)
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

    void StackIPInstanceNode::mapInputToEvalFrames(size_t inputIndex,
                                                   const FrameVector& inframes,
                                                   FrameVector& outframes) const
    {
        lazyUpdateRanges();
        mapInputToEvalFramesInternal(inputIndex, inframes, outframes);
    }

    void StackIPInstanceNode::mapInputToEvalFramesInternal(
        size_t inputIndex, const FrameVector& inframes,
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

    void StackIPInstanceNode::readCompleted(const string& t, unsigned int v)
    {
        updateActiveAudioInput();
        updateOutOfRangePolicy();

        lock();
        m_rangeInfoDirty = true;
        m_structureInfoDirty = true;
        unlock();

        IPInstanceNode::readCompleted(t, v);
    }

    void
    StackIPInstanceNode::propagateFlushToInputs(const FlushContext& context)
    {
        Context c = context;

        for (int i = 0; i < inputs().size(); ++i)
        {
            c.frame = inputFrame(i, context.frame);
            inputs()[i]->propagateFlushToInputs(c);
        }
    }

} // namespace IPCore
