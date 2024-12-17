//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/SwitchIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/Exception.h>
#include <IPCore/AdaptorIPNode.h>
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

    SwitchIPNode::SwitchIPNode(const std::string& name,
                               const NodeDefinition* def, IPGraph* g,
                               GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_offset(0)
        , m_fit(false)
        , m_rangeInfoDirty(false)
        , m_activeInputIndex(0)
    {
        pthread_mutex_init(&m_lock, 0);

        m_outputFPS = declareProperty<FloatProperty>("output.fps", 0.0f);
        m_outputSize = declareProperty<IntProperty>("output.size");
        m_outputSize->resize(2);
        m_outputSize->front() = 720;
        m_outputSize->back() = 480;

        m_useCutInfo = declareProperty<IntProperty>("mode.useCutInfo", 1);
        m_autoEDL = declareProperty<IntProperty>("mode.autoEDL", 1);
        m_alignStartFrames =
            declareProperty<IntProperty>("mode.alignStartFrames", 0);
        m_autoSize = declareProperty<IntProperty>("output.autoSize", 1);
        m_activeInput = declareProperty<StringProperty>("output.input", "");
    }

    SwitchIPNode::~SwitchIPNode() { pthread_mutex_destroy(&m_lock); }

    void SwitchIPNode::updateSelectedInput()
    {
        m_activeInputIndex = 0;

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
                if (node->name() == m_activeInput->front())
                {
                    m_activeInputIndex = i;
                    break;
                }
            }
        }

        graph()->flushAudioCache();

        if (ins.size() > 0)
        {
            ins[m_activeInputIndex]->setMediaActive(true);
        }
    }

    void SwitchIPNode::setInputs(const IPNodes& nodes)
    {
        IPNode::setInputs(nodes);

        if (!isDeleting())
        {
            computeRanges();
            propagateRangeChange();
            updateSelectedInput();
        }
    }

    void SwitchIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_useCutInfo || p == m_alignStartFrames || p == m_outputFPS
                || p == m_autoEDL)
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

            if (p == m_activeInput)
            {
                updateSelectedInput();

                lock();
                m_rangeInfoDirty = true;
                unlock();
                propagateRangeChange();
                if (group())
                    group()->propertyChanged(p);
            }
        }

        IPNode::propertyChanged(p);
    }

    void SwitchIPNode::lazyUpdateRanges() const
    {
        lock();
        if (m_rangeInfoDirty || m_structureInfoDirty)
        {
            const_cast<SwitchIPNode*>(this)->computeRanges();
        }
        unlock();
    }

    void SwitchIPNode::inputRangeChanged(int inputIndex, PropagateTarget target)
    {
        lock();
        m_rangeInfoDirty = true;
        unlock();
    }

    void SwitchIPNode::inputImageStructureChanged(int inputIndex,
                                                  PropagateTarget target)
    {
        lock();
        m_structureInfoDirty = true;
        unlock();
    }

    void SwitchIPNode::mediaInfo(const Context& context,
                                 MediaInfoVector& infos) const
    {
        const IPNodes& ins = inputs();
        Context c = context;
        c.fps = m_outputFPS->front();
        size_t i = m_activeInputIndex;
        c.frame = inputFrame(i, context.frame);
        ins[i]->mediaInfo(c, infos);
    }

    IPNode::ImageStructureInfo
    SwitchIPNode::imageStructureInfo(const Context& context) const
    {
        lazyUpdateRanges();
        return m_structureInfo;
    }

    void SwitchIPNode::computeRanges()
    {
        if (isDeleting())
            return;

        size_t ninputs = inputs().size();
        if (ninputs == 0)
            return;

        //
        //  NOTE: this function computes the range for this node based on
        //  the first input as the start. At the end we compute an offset
        //  from m_info so that the result range is actually 1 to whatever
        //

        m_rangeInfos.resize(ninputs);

        const bool useCutInfo = m_useCutInfo->front();
        const bool alignStartFrames = m_alignStartFrames->front();
        const bool autoEDL = m_autoEDL->front();

        for (size_t i = 0; i < ninputs; i++)
        {
            m_rangeInfos[i] = inputs()[i]->imageRangeInfo();
        }

        // Do not update the effective range of the switch node if the active
        // input's media is not active yet
        if (!activeInput() || !activeInput()->isMediaActive())
        {
            return;
        }

        size_t i = m_activeInputIndex;
        ImageStructureInfo sinfo = inputs()[i]->imageStructureInfo(
            graph()->contextForFrame(m_rangeInfos[i].start));

        m_info = m_rangeInfos[i];
        m_structureInfo.width = sinfo.width;
        m_structureInfo.height = sinfo.height;
        m_structureInfo.pixelAspect = 1.0;

        if (useCutInfo)
        {
            m_info.start = m_info.cutIn;
            m_info.end = m_info.cutOut;
        }

        if (alignStartFrames)
        {
            int size = 0;

            if (useCutInfo)
            {
                size = m_rangeInfos[i].cutOut - m_rangeInfos[i].cutIn;
            }
            else
            {
                size = m_rangeInfos[i].end - m_rangeInfos[i].start;
            }

            m_info.end =
                max(m_info.end - m_info.start + 1, size) + m_info.start;
        }
        else
        {
            if (useCutInfo)
            {
                m_info.start = min(m_info.start, m_rangeInfos[i].cutIn);
                m_info.end = max(m_info.end, m_rangeInfos[i].cutOut);
            }
            else
            {
                m_info.start = min(m_info.start, m_rangeInfos[i].start);
                m_info.end = max(m_info.end, m_rangeInfos[i].end);
            }
        }

        m_info.cutIn = m_info.start;
        m_info.cutOut = m_info.end;
        m_offset = autoEDL ? m_info.start - 1 : 0;

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

        for (size_t i = 0; i < ninputs; i++)
        {
            FrameVector gout;
            gin[0] = useCutInfo ? m_rangeInfos[i].cutIn : m_rangeInfos[i].start;
            gin[1] = useCutInfo ? m_rangeInfos[i].cutOut : m_rangeInfos[i].end;
            mapInputToEvalFramesInternal(i, gin, gout);
            //
            //  m_info.start is already taken into account.
            //
            //  m_globalRanges[i] = FrameRange(gout[0] - m_info.start, gout[1] -
            //  m_info.start);
            m_globalRanges[i] = FrameRange(gout[0], gout[1]);
        }

        m_rangeInfoDirty = false;
        m_structureInfoDirty = false;
    }

    int SwitchIPNode::inputFrame(size_t index, int frame,
                                 bool unconstrained) const
    {
        if (m_rangeInfos.empty())
            return frame;

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

    IPImage* SwitchIPNode::evaluate(const Context& context)
    {
        lazyUpdateRanges();

        const IPNodes& nodes = inputs();
        const int ninputs = nodes.size();
        const int frame = context.frame;
        const int width = m_structureInfo.width;
        const int height = m_structureInfo.height;
        const float aspect = float(width) / float(height);

        IPImage* firstChild = 0;

        if (nodes.empty())
            return IPImage::newNoImage(this, "No Inputs");

        IPImage* root =
            new IPImage(this, IPImage::BlendRenderType, width, height);

        try
        {
            IPImage* lastChild = 0;

            Context c = context;
            c.fps = m_outputFPS->front();
            size_t i = m_activeInputIndex;
            c.frame = inputFrame(i, frame);

            IPImage* current = nodes[i]->evaluate(c);

            if (!current)
            {
                // continue;
                IPNode* node = nodes[i];
                TWK_THROW_STREAM(EvaluationFailedExc,
                                 "SwitchIPNode evaluation failed on node "
                                     << node->name());
            }

            //
            //  Fit large aspect ratios into our output aspect
            //

            float imgAspect = current->displayAspect();

            if (m_fit)
                current->fitToAspect(aspect);

            if (current)
            {
                if (firstChild)
                {
                    lastChild->append(current);
                    lastChild = current;
                }
                else
                {
                    firstChild = current;
                    lastChild = firstChild;
                }
            }
            else
            {
                // ?
            }
        }
        catch (std::exception&)
        {
            ThreadType thread = context.thread;

            TWK_CACHE_LOCK(context.cache, "thread=" << thread);
            context.cache.checkInAndDelete(firstChild);
            TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
            delete root;
            throw;
        }

        const char* comp = 0;

        root->children = firstChild;
        root->recordResourceUsage();

        return root;
    }

    IPImageID* SwitchIPNode::evaluateIdentifier(const Context& context)
    {
        lazyUpdateRanges();
        const IPNodes& nodes = inputs();
        const int ninputs = nodes.size();
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
        //  If restricted() than quit after the first one
        //

        try
        {
            ostringstream idstr;

            size_t i = m_activeInputIndex;

            Context c = context;
            c.frame = inputFrame(i, frame);

            IPImageID* imgid = nodes[i]->evaluateIdentifier(c);

            if (!imgid)
            {
                // continue;
                IPNode* node = nodes[i];
                TWK_THROW_STREAM(EvaluationFailedExc,
                                 "NULL imgid in SwitchIPNode "
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

            root->id = idstr.str();
        }
        catch (...)
        {
            delete root;
            throw;
        }

        return root;
    }

    void SwitchIPNode::testEvaluate(const Context& context,
                                    TestEvaluationResult& result)
    {
        lazyUpdateRanges();
        IPNodes ins = inputs();
        int ninputs = inputs().size();
        int frame = context.frame;
        IPImage* img = 0;

        if (!ninputs)
            return;

        size_t i = m_activeInputIndex;
        int f = frame;
        Context c = context;
        const ImageRangeInfo& info = m_rangeInfos[i];
        c.frame = inputFrame(i, frame);

        ins[i]->testEvaluate(context, result);
    }

    void SwitchIPNode::metaEvaluate(const Context& context,
                                    MetaEvalVisitor& visitor)
    {
        lazyUpdateRanges();
        visitor.enter(context, this);

        IPNodes ins = inputs();
        int ninputs = inputs().size();
        IPImage* img = 0;

        if (ninputs)
        {
            size_t i = m_activeInputIndex;

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

    IPNode::ImageRangeInfo SwitchIPNode::imageRangeInfo() const
    {
        lazyUpdateRanges();

        ImageRangeInfo info = m_info;
        info.start -= m_offset;
        info.end -= m_offset;
        info.cutIn -= m_offset;
        info.cutOut -= m_offset;
        return info;
    }

    size_t SwitchIPNode::audioFillBuffer(const AudioContext& context)
    {
        lazyUpdateRanges();

        if (inputs().empty())
            return 0;

        //
        // In case the switch sources start frames are aligned we need to offset
        // the start of our query context accordingly
        //

        size_t i = m_activeInputIndex;

        const long foffset = inputFrame(i, m_info.start - m_offset, true)
                             - m_rangeInfos[i].start;
        const Time soffset = double(foffset) / context.fps;
        Time contextStart =
            samplesToTime(context.buffer.startSample(), context.buffer.rate());

        //
        // Construct a temporary buffer from which we will fill the given
        // context
        //

        AudioBuffer audioBuffer(context.buffer.size(),
                                context.buffer.channels(),
                                context.buffer.rate(), soffset + contextStart);

        AudioContext subContext(audioBuffer, context.fps);

        size_t rval = inputs()[i]->audioFillBuffer(subContext);

        memcpy(context.buffer.pointer(), subContext.buffer.pointer(),
               rval * context.buffer.numChannels() * sizeof(float));

        return rval;
    }

    void SwitchIPNode::mapInputToEvalFrames(size_t inputIndex,
                                            const FrameVector& inframes,
                                            FrameVector& outframes) const
    {
        if (inputIndex != m_activeInputIndex)
            return;
        lazyUpdateRanges();
        mapInputToEvalFramesInternal(inputIndex, inframes, outframes);
    }

    void
    SwitchIPNode::mapInputToEvalFramesInternal(size_t inputIndex,
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

    void SwitchIPNode::readCompleted(const string& t, unsigned int v)
    {
        updateSelectedInput();
        IPNode::readCompleted(t, v);
    }

    void SwitchIPNode::propagateFlushToInputs(const FlushContext& context)
    {
        Context c = context;

        //
        //  Should the switch node flush *all* its inputs? Or just the
        //  active one?
        //

        for (int i = 0; i < inputs().size(); ++i)
        {
            c.frame = inputFrame(i, context.frame);
            inputs()[i]->propagateFlushToInputs(c);
        }
    }

    IPNode* SwitchIPNode::activeInput() const
    {
        return (m_activeInputIndex < inputs().size())
                   ? inputs()[m_activeInputIndex]
                   : nullptr;
    }

} // namespace IPCore
