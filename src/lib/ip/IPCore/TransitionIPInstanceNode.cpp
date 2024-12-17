//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/TransitionIPInstanceNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderFunction.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ShaderCommon.h>
#include <TwkAudio/Audio.h>
#include <TwkMath/Function.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;
    using namespace TwkAudio;

    TransitionIPInstanceNode::TransitionIPInstanceNode(
        const std::string& name, const NodeDefinition* def, IPGraph* graph,
        GroupIPNode* group)
        : IPInstanceNode(name, def, graph, group)
        , m_fit(true)
        , m_rangeInfoDirty(true)
        , m_outputFPS(0)
        , m_outputSize(0)
        , m_autoSize(0)
        , m_useCutInfo(0)
        , m_startFrame(0)
        , m_duration(0)
    {
        setHasLinearTransform(m_fit);
        m_outputFPS = declareProperty<FloatProperty>("output.fps", 0.0f);
        m_outputSize = declareProperty<IntProperty>("output.size");
        m_autoSize = declareProperty<IntProperty>("output.autoSize", 1);
        m_useCutInfo = declareProperty<IntProperty>("mode.useCutInfo", 1);
        m_startFrame = declareProperty<FloatProperty>("parameters.startFrame",
                                                      1, 0, false);
        m_duration = declareProperty<FloatProperty>("parameters.numFrames", 10,
                                                    0, false);

        m_outputSize->resize(2);
        m_outputSize->front() = 720;
        m_outputSize->back() = 480;
    }

    TransitionIPInstanceNode::~TransitionIPInstanceNode() {}

    void TransitionIPInstanceNode::setInputs(const IPNodes& nodes)
    {
        IPInstanceNode::setInputs(nodes);

        if (!isDeleting())
        {
            computeRanges();
            propagateRangeChange();
        }
    }

    void TransitionIPInstanceNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_useCutInfo || p == m_outputFPS)
            {
                lock();
                m_rangeInfoDirty = true;
                unlock();
                propagateRangeChange();

                if (group())
                    group()->propertyChanged(p);
            }

            if (p == m_autoSize || p == m_outputSize)
            {
                lock();
                m_structureInfoDirty = true;
                unlock();
                propagateImageStructureChange();
            }
        }

        IPInstanceNode::propertyChanged(p);
    }

    void TransitionIPInstanceNode::lazyUpdateRanges() const
    {
        lock();

        if (m_rangeInfoDirty || m_structureInfoDirty)
        {
            try
            {
                const_cast<TransitionIPInstanceNode*>(this)->computeRanges();
            }
            catch (...)
            {
                unlock();
                throw;
            }
        }

        unlock();
    }

    void TransitionIPInstanceNode::inputRangeChanged(int inputIndex,
                                                     PropagateTarget target)
    {
        lock();
        m_rangeInfoDirty = true;
        unlock();
    }

    void
    TransitionIPInstanceNode::inputImageStructureChanged(int inputIndex,
                                                         PropagateTarget target)
    {
        lock();
        m_structureInfoDirty = true;
        unlock();
    }

    IPNode::ImageStructureInfo
    TransitionIPInstanceNode::imageStructureInfo(const Context& context) const
    {
        lazyUpdateRanges();
        return m_structureInfo;
    }

    void TransitionIPInstanceNode::computeRanges()
    {
        if (isDeleting())
            return;

        //
        //  NOTE: this function computes the range for this node based on
        //  the first input as the start. At the end we compute an offset
        //  from m_info so that the result range is actually 1 to whatever
        //

        m_rangeInfos.resize(inputs().size());

        if (inputs().size() != 2)
            return;

        const bool useCutInfo = m_useCutInfo->front();
        int maxDuration = 0;
        int transitionDuration = m_duration->front();

        IPNode* in0 = inputs()[0];
        IPNode* in1 = inputs()[1];

        m_rangeInfos.resize(2);
        m_rangeInfos[0] = in0->imageRangeInfo();
        m_rangeInfos[1] = in1->imageRangeInfo();

        ImageStructureInfo sinfo0 = in0->imageStructureInfo(
            graph()->contextForFrame(m_rangeInfos[0].start));
        ImageStructureInfo sinfo1 = in1->imageStructureInfo(
            graph()->contextForFrame(m_rangeInfos[1].start));

        if (useCutInfo)
        {
            m_rangeInfos[0].start = m_rangeInfos[0].cutIn;
            m_rangeInfos[0].end = m_rangeInfos[0].cutOut;
            m_rangeInfos[1].start = m_rangeInfos[1].cutIn;
            m_rangeInfos[1].end = m_rangeInfos[1].cutOut;
        }

        const size_t len0 = m_rangeInfos[0].end - m_rangeInfos[0].start + 1;
        const size_t len1 = m_rangeInfos[1].end - m_rangeInfos[1].start + 1;

        size_t overlap = (m_startFrame->front() <= m_rangeInfos[0].end)
                             ? (1 + len0 - m_startFrame->front())
                             : 0;
        m_info.start = 1;
        m_info.end = len0 + len1 - overlap;

        const int w0 = sinfo0.width;
        const int h0 = sinfo0.height;
        const int w1 = sinfo1.width;
        const int h1 = sinfo1.height;

        m_structureInfo.pixelAspect = 1.0;
        m_structureInfo.width = max(w0, w1);
        m_structureInfo.height = max(h0, h1);

        m_info.cutIn = m_info.start;
        m_info.cutOut = m_info.end;

        if (m_outputFPS->front() == 0.0)
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

        //
        //  Range ends are _inclusive_
        //
        m_globalRanges[0] =
            FrameRange(1, m_startFrame->front() + m_duration->front() - 1);
        m_globalRanges[1] = FrameRange(m_startFrame->front(), m_info.end);

        m_rangeInfoDirty = false;
        m_structureInfoDirty = false;
    }

    int TransitionIPInstanceNode::inputFrame(size_t index, int frame,
                                             bool unconstrained)
    {
        const ImageRangeInfo& info = m_rangeInfos[index];
        const int tstart = m_startFrame->front();
        const int tduration = m_duration->front();

        const int fs = m_rangeInfos[index].start;
        const int fe = m_rangeInfos[index].end;
        const int len = fe - fs + 1;

        //
        //  Convert the input global frame to a local frame for given
        //  input index
        //

        if (frame < m_info.start)
            frame = m_info.start;
        if (frame > m_info.end)
            frame = m_info.end;

        if (index == 0)
        {
            if (frame - 1 >= len)
                return fe;
            if (frame <= 1)
                return fs;
            return fs + frame - 1;
        }
        else
        {
            if (frame >= tstart)
            {
                int offset = frame - tstart;
                if (offset >= len)
                    return fe;
                return fs + offset;
            }
        }
        return fs;
    }

    IPImage* TransitionIPInstanceNode::evaluate(const Context& context)
    {
        lazyUpdateRanges();
        if (!isActive())
            return IPInstanceNode::evaluate(context);

        const IPNodes nodes = inputs();
        const int ninputs = nodes.size();
        const int frame = context.frame;
        const int width = m_structureInfo.width;
        const int height = m_structureInfo.height;
        const float aspect = float(width) / float(height);

        if (ninputs == 0)
        {
            return IPImage::newNoImage(this, "No Inputs");
        }
        else if (ninputs == 1)
        {
            return IPImage::newNoImage(this, "Missing an Input");
        }

        IPImage* root =
            new IPImage(this, IPImage::MergeRenderType, width, height, 1.0,
                        IPImage::IntermediateBuffer, IPImage::FloatDataType);

        if (ninputs == 0)
            return root;

        const bool useCutInfo = m_useCutInfo->front();

        Shader::ExpressionVector inExpressions;
        IPImageSet modifiedImages;
        IPImageVector images;

        try
        {
            for (size_t i = 0; i < 2; i++)
            {
                Context c = context;
                c.fps = m_outputFPS->front();
                c.frame = inputFrame(i, frame);

                // cout << "i = " << i << ", frame = " << c.frame << endl;

                IPImage* current = nodes[i]->evaluate(c);
                if (ninputs == 1)
                    return current;

                if (!current)
                {
                    TWK_THROW_STREAM(
                        EvaluationFailedExc,
                        "TransitionIPInstanceNode evaluation failed on node "
                            << nodes[i]->name());
                }

                //
                //  Fit large aspect ratios into our output aspect
                //

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

        const Shader::Function* F = definition()->function();

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
        //  Assemble shaders
        //

        root->appendChildren(images);

        root->mergeExpr = bind(root, inExpressions, context);
        root->shaderExpr = Shader::newSourceRGBA(root);
        root->recordResourceUsage();
        return root;
    }

    IPImageID*
    TransitionIPInstanceNode::evaluateIdentifier(const Context& context)
    {
        lazyUpdateRanges();
        const IPNodes nodes = inputs();
        const int ninputs = inputs().size();
        const int frame = context.frame;
        const int width = m_structureInfo.width;
        const int height = m_structureInfo.height;
        const float aspect = float(width) / float(height);

        if (nodes.empty())
            return 0;

        IPImageID* root = new IPImageID();
        IPImageID* last = 0;

        //
        //  Loop over the inputs starting at the top index
        //

        try
        {
            ostringstream idstr;
            bool haveOneImage = false;

            for (size_t i = 0; i < ninputs; i++)
            {
                Context c = context;
                c.frame = inputFrame(i, frame);

                IPImageID* imgid = nodes[i]->evaluateIdentifier(c);

                if (!imgid)
                {
                    // continue;
                    IPNode* node = nodes[i];
                    TWK_THROW_STREAM(EvaluationFailedExc,
                                     "NULL imgid in TransitionIPInstanceNode "
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

    void TransitionIPInstanceNode::testEvaluate(const Context& context,
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

    void TransitionIPInstanceNode::metaEvaluate(const Context& context,
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
            int f = inputFrame(i, context.frame);

            Context c = context;
            c.frame = f;

            if (visitor.traverseChild(c, i, this, ins[i]))
            {
                ins[i]->metaEvaluate(c, visitor);
            }
        }

        visitor.leave(context, this);
    }

    IPNode::ImageRangeInfo TransitionIPInstanceNode::imageRangeInfo() const
    {
        lazyUpdateRanges();
        return m_info;
    }

    size_t
    TransitionIPInstanceNode::audioFillBuffer(const AudioContext& context)
    {
        lazyUpdateRanges();

        if (inputs().empty())
            return 0;

        const double rate = context.buffer.rate();
        const double fps = context.fps;
        const size_t startSample = context.buffer.startSample();
        const size_t nsamples = context.buffer.size();
        const double sampsPerFrame = rate / fps;
        const size_t ninputs = inputs().size();
        const double localFPS = outputFPSProperty()->front();

        //
        //  Sum up the audio from all of the stacked nodes
        //

        size_t rval = 0;

        AudioBuffer tempbuffer(context.buffer.size(), context.buffer.channels(),
                               context.buffer.rate(),
                               context.buffer.startTime());

        size_t gss = startSample;
        size_t gse = startSample + nsamples;

        for (size_t i = 0; i < ninputs; i++)
        {
            //
            //  For foffset and transition sample bounds, we must take account
            //  of the fact that the second input is shifted in time (unless
            //  startFrame is one).
            //

            const long foffset =
                inputFrame(i, m_info.start, true) - m_rangeInfos[i].start
                - ((i == 1) ? m_startFrame->front() - m_info.start : 0);

            size_t transitionStartSample = timeToSamples(
                ((i == 1) ? 0 : (m_startFrame->front() - 1)) / fps, rate);
            size_t transitionEndSample = timeToSamples(
                ((i == 1) ? m_duration->front()
                          : m_startFrame->front() - 1 + m_duration->front() + 1)
                    / fps,
                rate);

            const long soffset = foffset * sampsPerFrame;
            const long eoffset = soffset + nsamples;

            const size_t gstart =
                timeToSamples((m_globalRanges[i].first - 1) / fps, rate);
            const size_t gend = timeToSamples(
                (m_globalRanges[i].second - 1) / fps + 1.0 / fps, rate);

            //
            //  soffset may be negative, so don't let sample index explode.
            //
            size_t ss = (-soffset >= startSample) ? 0 : startSample + soffset;
            size_t se = startSample + eoffset;

            if (!(gss >= gend || gse < gstart))
            {
                AudioContext temp(tempbuffer, context.fps);

                temp.buffer.setStartTime(samplesToTime(ss, rate));
                temp.buffer.zero();

                rval = max(inputs()[i]->audioFillBuffer(temp), rval);

                //
                //  Silence samples in this buffer that come from audio outside
                //  the input's frame range.
                //

                if (gse >= gend)
                {
                    size_t n = gse - gend;
                    temp.buffer.zeroRegion(temp.buffer.size() - n, n);
                }

                if (gss < gstart)
                {
                    size_t n = gstart - gss;
                    temp.buffer.zeroRegion(0, n);
                }

                //
                //  Cross-fade audio during transition.
                //

                if (!(temp.buffer.startSample() > transitionEndSample
                      || temp.buffer.startSample() + temp.buffer.size()
                             < transitionStartSample))

                {
                    size_t sample = temp.buffer.startSample();

                    size_t startSampleOffset = 0;
                    size_t endSampleOffset = 0;

                    if (temp.buffer.startSample() < transitionStartSample)
                    {
                        startSampleOffset =
                            transitionStartSample - temp.buffer.startSample();
                    }
                    if (temp.buffer.startSample() + temp.buffer.size()
                        > transitionEndSample)
                    {
                        endSampleOffset = temp.buffer.startSample()
                                          + temp.buffer.size()
                                          - transitionEndSample;
                    }

                    float* f = temp.buffer.pointer()
                               + temp.buffer.numChannels() * startSampleOffset;
                    float* lim = temp.buffer.pointer()
                                 + temp.buffer.numChannels()
                                       * (temp.buffer.size() - endSampleOffset);
                    float denom =
                        float(transitionEndSample - transitionStartSample);
                    float factor = 1.0;

                    if (i == 0)
                    {
                        while (f < lim)
                        {
                            factor =
                                float(transitionEndSample - sample) / denom;

                            *f = factor * *f;
                            ++f;
                            *f = factor * *f;
                            ++f;

                            ++sample;
                        }
                    }
                    else
                    {
                        while (f < lim)
                        {
                            factor =
                                1.0
                                - float(transitionEndSample - sample) / denom;

                            *f = factor * *f;
                            ++f;
                            *f = factor * *f;
                            ++f;

                            ++sample;
                        }
                    }
                }

                transform(context.buffer.pointer(),
                          context.buffer.pointer()
                              + context.buffer.sizeInFloats(),
                          temp.buffer.pointer(), context.buffer.pointer(),
                          plus<float>());
            }
        }

        return rval;
    }

    void
    TransitionIPInstanceNode::mapInputToEvalFrames(size_t inputIndex,
                                                   const FrameVector& inframes,
                                                   FrameVector& outframes) const
    {
        lazyUpdateRanges();
        mapInputToEvalFramesInternal(inputIndex, inframes, outframes);
    }

    void TransitionIPInstanceNode::mapInputToEvalFramesInternal(
        size_t inputIndex, const FrameVector& inframes,
        FrameVector& outframes) const
    {
        const ImageRangeInfo& info = m_rangeInfos[inputIndex];
        const bool useCutInfo = m_useCutInfo->front();
        const int tstart = m_startFrame->front();
        const int tduration = m_duration->front();

        for (size_t i = 0; i < inframes.size(); i++)
        {
            int f = inframes[i];

            if (inputIndex == 0)
            {
                outframes.push_back(f - m_rangeInfos[0].start + 1);
            }
            else
            {
                outframes.push_back(f - m_rangeInfos[1].start + 1 + tstart);
            }
        }
    }

    void TransitionIPInstanceNode::propagateFlushToInputs(
        const FlushContext& context)
    {
        Context c = context;

        for (int i = 0; i < inputs().size(); ++i)
        {
            c.frame = inputFrame(i, context.frame);
            inputs()[i]->propagateFlushToInputs(c);
        }
    }

    void TransitionIPInstanceNode::readCompleted(const string& t,
                                                 unsigned int v)
    {
        lock();
        m_rangeInfoDirty = true;
        m_structureInfoDirty = true;
        unlock();

        IPInstanceNode::readCompleted(t, v);
    }

} // namespace IPCore
