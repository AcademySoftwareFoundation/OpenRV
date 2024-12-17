//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/SequenceIPNode.h>
#include <IPCore/Exception.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ShaderUtil.h>
#include <IPCore/GroupIPNode.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <iostream>
#include <algorithm>
#include <deque>

#if 0
#define DB(x)                                                         \
    {                                                                 \
        stringstream msg;                                             \
        msg << "INFO [" << this << "]: SequenceIPNode:" << x << endl; \
        cerr << msg.str();                                            \
    }
#else
#define DB(x)
#endif

namespace
{

    TwkAudio::SampleTime frameToSample(int frame, double fps,
                                       double audioSamplingRate)
    {
        return TwkAudio::timeToSamples(double(frame) / fps, audioSamplingRate);
    }

} // namespace

namespace IPCore
{
    using namespace TwkFB;
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkAudio;
    using namespace TwkMath;

    SequenceIPNode::SequenceIPNode(const std::string& name,
                                   const NodeDefinition* def, IPGraph* g,
                                   GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_updateHiddenData(false)
        , m_updateEDL(false)
        , m_clipCaching(false)
        , m_volatileInputs(false)
    {
        m_structInfo.width = 854;
        m_structInfo.height = 480;
        m_structInfo.pixelAspect = 1.0;

        m_edlSource = declareProperty<IntProperty>("edl.source");
        m_edlGlobalIn = declareProperty<IntProperty>("edl.frame");
        m_edlSourceIn = declareProperty<IntProperty>("edl.in");
        m_edlSourceOut = declareProperty<IntProperty>("edl.out");
        m_edl = component("edl");

        m_outputSize = declareProperty<IntProperty>("output.size");
        m_outputSize->resize(2);
        m_outputSize->front() = 720;
        m_outputSize->back() = 480;

        m_outputFPS = declareProperty<FloatProperty>("output.fps", 0.0f);
        m_interactiveSize =
            declareProperty<IntProperty>("output.interactiveSize", 1);
        m_autoEDL = declareProperty<IntProperty>("mode.autoEDL", 1);
        m_useCutInfo = declareProperty<IntProperty>("mode.useCutInfo", 1);
        m_autoSize = declareProperty<IntProperty>("output.autoSize", 1);

        // SequenceIPNode can optionally provide per-input blend modes. Those
        // are stored in a container that is indexed with the source indices,
        // just like any other per-input EDL information.
        m_inputsBlendingModes = declareProperty<StringProperty>(
            "composite.inputBlendModes", "", 0, false);
        // since it is per input, make sure the property contains is emptied at
        // creation time
        m_inputsBlendingModes->erase(0, 1);

        // By default, the output of this node support reverse-order blending.
        // this is mainly kept for backward compatibility reason.
        // See ImageRenderer::renderAllChildren for more details on this mode.
        m_supportReversedOrderBlending = declareProperty<IntProperty>(
            "mode.supportReversedOrderBlending", 1);

        setHasLinearTransform(true); // fits to aspect
    }

    SequenceIPNode::~SequenceIPNode() {}

    bool SequenceIPNode::interactiveSize(const Context& c) const
    {
        return m_interactiveSize->front() != 0 && graph()->viewNode() == group()
               && c.allowInteractiveResize && c.viewWidth != 0
               && c.viewHeight != 0;
    }

    int SequenceIPNode::indexAtFrame(int frame) const
    {
        lazyBuildState();
        if (!m_edlGlobalIn)
            return -1;

        //
        //  We are looking for the index of the source which contains (between
        //  its in and out points) the imagery that corresponds to the global
        //  scope search frame.
        //
        //  We will locate the index into the m_edlSource list based on the
        //  search for the closest frame boundary in the "global" m_edlGlobalIn
        //  frames list. All of the m_edl* lists are synchronized in that
        //  information at an index into any one of the lists corresponds to the
        //  same source boundary in each of the other lists. This means we can
        //  search the m_edlGlobalIn list by frame and get information from the
        //  other lists from those same discoveries.
        //
        //  The m_edlGlobalIn "global" list of frame boundaries is unique in
        //  that, though each list/vector is the same length, m_edlGlobalIn's
        //  final entry value is still valid and represents the final frame of
        //  the sequence. All of the other lists contain no meaningful
        //  information in the last entry or back of the vector.
        //
        //  Here is an example set of data from a session:
        //
        //    edl
        //    {
        //        int frame  = [  1  27 141 252 365 480 594 ]  => m_edlGlobalIn
        //        int source = [  0   1   2   3   4   5   0 ]  => m_edlSource
        //        int in     = [  0  26 140 251 364 479   0 ]  => m_edlSourceIn
        //        int out    = [ 25 139 250 363 478 592   0 ]  => m_edlSourceOut
        //    }
        //

        //
        //  Create a vector iterator pointing at the closest (equal to or
        //  greater than) m_edlGlobalIn "global" frame number
        //

        IntProperty::const_iterator globalFrameItr =
            lower_bound(m_edlGlobalIn->begin(), m_edlGlobalIn->end(), frame);

        //
        //  Since lower_bound returns an iterator pointing at the "global" frame
        //  which is equal to or greater than what we searched for, we assume
        //  that the index we want (the index to the source which includes the
        //  search frame) is the previous location.
        //

        int index = globalFrameItr - m_edlGlobalIn->begin() - 1;

        //
        //  Next we check if the location is too high or at the end of the
        //  range.
        //

        if (globalFrameItr >= m_edlGlobalIn->end()
            || frame == m_edlGlobalIn->back())
        {
            //
            // Minus 2 because the last valid index in all other lists/vectors
            // is one less than the last of m_edlGlobalIn (size - 1). Read the
            // above comments for more details.
            //

            index = m_edlGlobalIn->size() - 2;
        }

        //
        //  If the frame was found then we use that location/index
        //

        else if (*globalFrameItr == frame)
        {
            index = globalFrameItr - m_edlGlobalIn->begin();
        }

        //
        //  Finally if our asssumption was too low then we use the first index.
        //

        else if (index < 0)
        {
            index = 0;
        }

        //
        //  Basically we are saying that if we have inputs, then make sure the
        //  index we found does not exceed the valid range of indicies in our
        //  edl information.
        //

        if (inputs().size())
        {
            if (index >= m_edlSource->size() - 1
                || index >= m_edlSourceIn->size() - 1
                || index >= m_edlSourceOut->size() - 1
                || index >= m_edlGlobalIn->size() - 1 || index < 0)
            {
                TWK_THROW_STREAM(SequenceOutOfBoundsExc,
                                 "Out-of-bounds EDL data");
            }
        }

        return index;
    }

    int SequenceIPNode::indexAtSample(SampleTime seekSample, double sampleRate,
                                      double fps)
    {
        lazyBuildState();
        if (!m_edlGlobalIn)
            return -1;

        //
        //  We are trying to find the index of the Sequence input Source which
        //  contains the seekSample. This means that we find the closest input
        //  index with a Sequence "global" start sample less than the
        //  seekSample.
        //
        //  To accomplish this first we find the index of the Sequence input
        //  Source which contains the seekSample converted to a frame.
        //

        const int globalOffset = m_edlGlobalIn->front();
        int seekFrame = int(samplesToTime(seekSample, sampleRate) * fps + 0.49);
        int index = indexAtFrame(seekFrame + globalOffset);

        int inputFrame = (*m_edlGlobalIn)[index];
        SampleTime inputStart =
            frameToSample(inputFrame - globalOffset, fps, sampleRate);

        //
        //  Since audio samples and video frames don't line up precisely we
        //  might need to walk back a frame to actually find the index which
        //  contains the trasition between sources. Here we make sure the first
        //  audio sample of the input we are returning is less than the search
        //  sample.
        //

        while (seekSample < inputStart && index > 0)
        {
            index = indexAtFrame(--seekFrame + globalOffset);
            inputFrame = (*m_edlGlobalIn)[index];
            inputStart =
                frameToSample(inputFrame - globalOffset, fps, sampleRate);
        }

        return index;
    }

    SequenceIPNode::EvalPoint
    SequenceIPNode::evaluationPoint(int frame, int forceIndex) const
    {
        const IPNodes& ins = inputs();

        if (ins.size() > 0)
        {
            const int index =
                (forceIndex == -1) ? indexAtFrame(frame) : forceIndex;

            const int source = (*m_edlSource)[index];
            const int in = (*m_edlSourceIn)[index];
            const int out = (*m_edlSourceOut)[index];
            const int iframe = (*m_edlGlobalIn)[index];

            const int f0 = (frame - iframe) + in;

            if (forceIndex == -1)
            {
                const int f1 = min(f0, out);
                const int f = max(f1, in);

                return EvalPoint(source, f, index);
            }
            else
                return EvalPoint(source, f0, index);
        }
        else
        {
            return EvalPoint(-1, 0, -1);
        }
    }

    IPImage* SequenceIPNode::evaluate(const Context& context)
    {
        lazyBuildState();

        const bool interactive = interactiveSize(context);
        const float vw = interactive ? context.viewWidth : m_structInfo.width;
        const float vh = interactive ? context.viewHeight : m_structInfo.height;
        const float aspect = float(vw) / float(vh);
        const int frame = context.frame;
        const IPNodes& ins = inputs();

        if (ins.empty())
            return IPImage::newNoImage(this, "No Inputs");

        EvalPoint ep = evaluationPoint(frame);
        const int source = ep.sourceIndex;
        Context newContext = context;

        IPImageVector images(1);
        IPImageSet modifiedImages;
        Shader::ExpressionVector inExpressions;
        newContext.frame = ep.sourceFrame;

        if (source < 0 || source >= ins.size())
        {
            TWK_THROW_STREAM(SequenceOutOfBoundsExc,
                             "Bad Sequence EDL source number "
                                 << source << " is not in range [0,"
                                 << ins.size() - 1 << "]");
        }

        IPImage* root =
            new IPImage(this, IPImage::BlendRenderType, vw, vh, 1.0);

        // if we have per-input blending modes, now is the time to use them
        if (source >= 0 && source < m_inputsBlendingModes->size())
        {
            string blendModeString = (*m_inputsBlendingModes)[source];
            root->blendMode =
                IPImage::getBlendModeFromString(blendModeString.c_str());
        }

        // Make sure to disable reverse-order blending if required.
        // See ImageRenderer::renderAllChildren for more details on this mode.
        root->supportReversedOrderBlending =
            m_supportReversedOrderBlending->front() != 0;

        //
        //  If we pull multiple inputs simultaneously we'll need to
        //  check for caching errors and check in partially evaluated
        //  frames. Currently we don't have to worry about that
        //  because SequenceIPNode only evals one of its inputs.  (See
        //  StackIPNode)
        //

        IPImage* child = ins[ep.sourceIndex]->evaluate(newContext);

        //
        //  Uncrop the child into our output image: this is done by
        //  modifying its transform
        //

        child->fitToAspect(aspect);

        images[0] = child;

        convertBlendRenderTypeToIntermediate(images, modifiedImages);

        balanceResourceUsage(IPNode::filterAccumulate, images, modifiedImages,
                             8, 8, 81);

        //
        //  Put adaptive resize on child
        //

        // Shader::installAdaptiveBoxResizeRecursive(root);

        root->appendChild(child);

        if (m_clipCaching && context.thread == CacheEvalThread
            && context.frame == context.baseFrame && !context.cacheNode)
        {
            //
            //  Force eval point for "clip of display frame", only if frame is
            //  "outside clip".  This allows caching of framebuffers that are
            //  needed by the current view, but _will_ be needed if we extend
            //  the part of the current clip visible in the sequence.
            //

            EvalPoint dispP = evaluationPoint(graph()->cache().displayFrame());

            if (ep.sourceIndex != dispP.sourceIndex)
            {
                EvalPoint clipP = evaluationPoint(frame, dispP.inputIndex);

                if (clipP.sourceFrame >= m_rangeInfos[dispP.inputIndex].start
                    && clipP.sourceFrame <= m_rangeInfos[dispP.inputIndex].end)
                {
                    Context clipContext = context;
                    clipContext.frame = clipP.sourceFrame;

                    //  cerr << "clipCaching: evaluating source frame " <<
                    //  clipP.sourceFrame << endl;
                    root->appendChild(
                        ins[clipP.sourceIndex]->evaluate(clipContext));
                }
            }
        }

        //
        //  Compute the resource usage of the finished IPImage
        //

        root->recordResourceUsage();

        return root;
    }

    IPImageID* SequenceIPNode::evaluateIdentifier(const Context& context)
    {
        lazyBuildState();

        const IPNodes& ins = inputs();
        const bool interactive = interactiveSize(context);
        const float vw = interactive ? context.viewWidth : m_structInfo.width;
        const float vh = interactive ? context.viewHeight : m_structInfo.height;
        const float aspect = float(vw) / float(vh);
        const int frame = context.frame;

        if (ins.size() < 1)
            return IPNode::evaluateIdentifier(context);

        EvalPoint ep = evaluationPoint(frame);
        const int source = ep.sourceIndex;
        Context newContext = context;
        newContext.frame = ep.sourceFrame;

        if (source < 0 || source >= ins.size())
        {
            TWK_THROW_STREAM(SequenceOutOfBoundsExc,
                             "Bad Sequence EDL source number "
                                 << source << " is not in range [0,"
                                 << ins.size() - 1 << "] from frame " << frame);
        }

        IPImageID* root = ins[ep.sourceIndex]->evaluateIdentifier(newContext);

        if (m_clipCaching && context.thread == CacheEvalThread
            && context.frame == context.baseFrame && !context.cacheNode)
        {
            //
            //  Force eval point for "clip of display frame", only if frame is
            //  "outside clip".  This allows caching of framebuffers that are
            //  needed by the current view, but _will_ be needed if we extend
            //  the part of the current clip visible in the sequence.
            //

            EvalPoint dispP = evaluationPoint(graph()->cache().displayFrame());

            if (ep.sourceIndex != dispP.sourceIndex)
            {
                EvalPoint clipP = evaluationPoint(frame, dispP.inputIndex);

                if (clipP.sourceFrame >= m_rangeInfos[dispP.inputIndex].start
                    && clipP.sourceFrame <= m_rangeInfos[dispP.inputIndex].end)
                {
                    Context clipContext = context;
                    clipContext.frame = clipP.sourceFrame;

                    root->appendChild(
                        ins[clipP.sourceIndex]->evaluateIdentifier(
                            clipContext));
                }
            }
        }

        return root;
    }

    void SequenceIPNode::setInputs(const IPNodes& nodes)
    {
        LockGuard guard(m_mutex);

        m_updateHiddenData = true;
        int sizeDiff = nodes.size() - inputs().size();
        bool differs = false;

        //
        //  m_volatileInputs means you can't assume that the input
        //  range/fps is the same as last time. That means testing pointer
        //  equality is not a good enough test for append-only
        //

        if (nodes.size() > inputs().size() && !inputs().empty()
            && !m_volatileInputs)
        {
            for (size_t i = 0; i < inputs().size(); i++)
            {
                if (inputs()[i] != nodes[i])
                {
                    differs = true;
                    break;
                }
            }
        }
        else
        {
            differs = true;
        }

        //
        //  In order to preserve the EDL timing we must respect autoEDL.
        //
        //  Before calling the underlying setInputs(), update functions to
        //  compute the state. When the base setInputs() is called it will
        //  propagate a range and structure changed signal.
        //

        updateInputDataInternal(nodes);
        if (m_autoEDL->front())
            createDefaultEDLInternal(
                (differs || m_volatileInputs) ? 0 : sizeDiff, nodes);
        graph()->flushAudioCache();
        IPNode::setInputs(nodes);
        propagateRangeChange();
        propagateImageStructureChange();
    }

    void SequenceIPNode::inputChanged(int inputIndex)
    {
        //
        //  An input somewhere upstream changed.
        //
        //  Treat it just like inputRangeChanged() which is just like
        //  inputImageStructureChanged()
        //

        inputRangeChanged(inputIndex);
    }

    void SequenceIPNode::inputRangeChanged(int inputIndex,
                                           PropagateTarget target)
    {
        //
        //  This function should *only* set dirty flags, etc, but should never
        //  notify of anything outside (or inside) the graph
        //

        if (m_autoEDL->front() && !isDeleting())
        {
            //
            //  We're in "auto-edl" mode which means that the sequence
            //  node should be automatically recreating the default edl
            //  anytime something changes upstream
            //

            LockGuard guard(m_mutex);
            if (!m_updateHiddenData)
                m_updateHiddenData = true;
            if (!m_updateEDL)
                m_updateEDL = true;
        }
    }

    void SequenceIPNode::inputImageStructureChanged(int inputIndex,
                                                    PropagateTarget target)
    {
        //
        //  This function should *only* set dirty flags, etc, but should never
        //  notify of anything outside (or inside) the graph
        //

        if (!isDeleting())
        {
            //
            //  We're in "auto-edl" mode which means that the sequence
            //  node should be automatically recreating the default edl
            //  anytime something changes upstream
            //

            LockGuard guard(m_mutex);
            if (!m_updateHiddenData)
                m_updateHiddenData = true;
        }
    }

    void SequenceIPNode::updateInputData() const
    {
        if (isDeleting())
            return;
        updateInputDataInternal(inputs());
    }

    void SequenceIPNode::updateInputDataInternal(const IPNodes& ins) const
    {
        // Use the average duration of discovered sources for the unknown
        // ranges.
        std::deque<int> undiscoveredRangeInfosIdx;
        int totalStartEndRange = 0;
        int totalCutInCutOutRange = 0;

        m_rangeInfos.resize(ins.size());
        for (int i = 0; i < ins.size(); i++)
        {
            m_rangeInfos[i] = ins[i]->imageRangeInfo();

            if (m_rangeInfos[i].isUndiscovered)
            {
                undiscoveredRangeInfosIdx.push_back(i);
            }
            else
            {
                DB("Discoverd Source #" << i
                                        << " start: " << m_rangeInfos[i].start
                                        << ", end: " << m_rangeInfos[i].end);
                totalStartEndRange +=
                    (m_rangeInfos[i].end - m_rangeInfos[i].start);
                totalCutInCutOutRange +=
                    (m_rangeInfos[i].cutOut - m_rangeInfos[i].cutIn);
            }

            if (!i && m_outputFPS->front() == 0.0f
                && !m_rangeInfos[i].isUndiscovered)
            {
                m_outputFPS->front() = m_rangeInfos[i].fps;
                assert(m_outputFPS->front() != 1.0f);
            }

            ImageStructureInfo sinfo = ins[i]->imageStructureInfo(
                graph()->contextForFrame(m_rangeInfos[i].start));

            if (!i)
            {
                m_structInfo.width = sinfo.width;
                m_structInfo.height = sinfo.height;
                m_structInfo.pixelAspect = 1.0;
            }
            else
            {
                m_structInfo.width =
                    std::max(int(sinfo.width), m_structInfo.width);
                m_structInfo.height =
                    std::max(int(sinfo.height), m_structInfo.height);
                m_structInfo.pixelAspect = 1.0;
            }
        }

        const auto nbDiscoveredSources =
            ins.size() - undiscoveredRangeInfosIdx.size();
        DB("Discovered sources: " << nbDiscoveredSources);
        if ((nbDiscoveredSources >= kMinSourceDiscovered)
            && !undiscoveredRangeInfosIdx.empty())
        {
            const int averageStartEndRange =
                totalStartEndRange / nbDiscoveredSources;
            const int averageCutInCutOutRange =
                totalCutInCutOutRange / nbDiscoveredSources;
            DB("Average start-end range: " << averageStartEndRange);
            DB("Average cutIn-cutOut range: " << averageCutInCutOutRange);
            while (!undiscoveredRangeInfosIdx.empty())
            {
                const auto idx = undiscoveredRangeInfosIdx.front();
                m_rangeInfos[idx].end =
                    m_rangeInfos[idx].start + averageStartEndRange;
                m_rangeInfos[idx].cutOut =
                    m_rangeInfos[idx].cutIn + averageCutInCutOutRange;
                undiscoveredRangeInfosIdx.pop_front();
            }
        }

        if (m_autoSize->front())
        {
            m_outputSize->front() = m_structInfo.width;
            m_outputSize->back() = m_structInfo.height;
        }
        else
        {
            m_structInfo.width = m_outputSize->front();
            m_structInfo.height = m_outputSize->back();
        }

        m_updateHiddenData = false;
    }

    void SequenceIPNode::createDefaultEDL(int append) const
    {
        createDefaultEDLInternal(append, inputs());
    }

    void SequenceIPNode::createDefaultEDLInternal(int append,
                                                  const IPNodes& ins) const
    {

        m_updateEDL = false;
        if (isDeleting())
            return;
        if (m_updateHiddenData)
            updateInputData();
        // cout << this << " -- createDefaultEDL" << endl;

        bool useCutInfo = m_useCutInfo->front() != 0;

        if (ins.empty())
        {
            m_edl->resize(0);
        }
        else if (append)
        {
            int startIndex = m_edlGlobalIn->size() - 1;
            m_edl->resize(m_edlGlobalIn->size() + append);
            int accum = (*m_edlGlobalIn)[startIndex];
            int inindex = ins.size() - append;

            for (int i = startIndex; i < m_edlGlobalIn->size() - 1;
                 i++, inindex++)
            {
                const ImageRangeInfo& info = m_rangeInfos[inindex];

                const int start = useCutInfo ? info.cutIn : info.start;
                const int end = useCutInfo ? info.cutOut : info.end;

                (*m_edlGlobalIn)[i] = accum;
                (*m_edlSource)[i] = inindex;
                (*m_edlSourceIn)[i] = start;
                (*m_edlSourceOut)[i] = end;

                accum += (end - start + 1);
            }

            m_edlGlobalIn->back() = accum;
            m_edlSource->back() = 0;
            m_edlSourceIn->back() = 0;
            m_edlSourceOut->back() = 0;
        }
        else
        {
            m_edl->resize(ins.size() + 1);
            // int accum = ins[0]->imageRangeInfo().start;
            int accum = 1;

            for (int i = 0; i < ins.size(); i++)
            {
                const ImageRangeInfo& info = m_rangeInfos[i];

                const int start = useCutInfo ? info.cutIn : info.start;
                const int end = useCutInfo ? info.cutOut : info.end;

                (*m_edlGlobalIn)[i] = accum;
                (*m_edlSource)[i] = i;
                (*m_edlSourceIn)[i] = start;
                (*m_edlSourceOut)[i] = end;

                accum += (end - start + 1);
            }

            m_edlGlobalIn->back() = accum;
            m_edlSource->back() = 0;
            m_edlSourceIn->back() = 0;
            m_edlSourceOut->back() = 0;
        }
    }

    void SequenceIPNode::mediaInfo(const Context& context,
                                   MediaInfoVector& infos) const
    {
        const IPNodes& ins = inputs();
        if (ins.empty())
            return;
        EvalPoint ep = evaluationPoint(context.frame);
        const int source = ep.sourceIndex;

        if (source < 0 || source >= ins.size())
        {
            TWK_THROW_STREAM(SequenceOutOfBoundsExc,
                             "Bad Sequence EDL source number "
                                 << source << " is not in range [0,"
                                 << ins.size() - 1 << "]");
        }

        Context newContext = context;
        newContext.frame = ep.sourceFrame;
        ins[source]->mediaInfo(newContext, infos);
    }

    IPNode::ImageRangeInfo SequenceIPNode::imageRangeInfo() const
    {
        lazyBuildState();

        if (inputs().size() >= 1)
        {
            ImageRangeInfo info;
            info.inc = 1;
            info.fps = m_outputFPS->front();
            info.cutIn = info.start = m_edlGlobalIn->front();
            info.cutOut = info.end = m_edlGlobalIn->back() - 1;
            return info;
        }
        else
        {
            return IPNode::ImageRangeInfo(1, 1, 1, m_outputFPS->front(), 1);
        }
    }

    IPNode::ImageStructureInfo
    SequenceIPNode::imageStructureInfo(const Context& context) const
    {
        lazyBuildState();

        const bool interactive = interactiveSize(context);
        const float vw =
            interactive ? context.viewWidth : float(m_structInfo.width);
        const float vh =
            interactive ? context.viewHeight : float(m_structInfo.height);

        if (inputs().empty() || interactive)
        {
            return ImageStructureInfo(vw, vh);
        }
        else
        {
            const float aspect = vw / vh;
            EvalPoint ep = evaluationPoint(context.frame);
            Context context = graph()->contextForFrame(ep.sourceFrame);
            ImageStructureInfo ininfo =
                inputs()[ep.sourceIndex]->imageStructureInfo(context);

            //
            //  We may have an input that has no media "behind it" (IE a
            //  viewNode with no inputs of its own).  So check to be sure we
            //  have "real" info before we divide.
            //

            if (ininfo.height == 0)
                return m_structInfo;

            const float imgAspect = ininfo.width / ininfo.height;
        }

        return m_structInfo;
    }

    void SequenceIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_autoEDL || p == m_useCutInfo || p == m_outputFPS)
            {
                {
                    LockGuard guard(m_mutex);
                    m_updateEDL = true;
                }

                propagateRangeChange();

                graph()->flushAudioCache();

                if (group())
                    group()->propertyChanged(p);
            }
            else if (p == m_outputSize || p == m_autoSize)
            {
                {
                    LockGuard guard(m_mutex);
                    m_updateHiddenData = true;
                }

                const_cast<SequenceIPNode*>(this)
                    ->propagateImageStructureChange();
                if (group())
                    group()->propertyChanged(p);
            }
        }

        IPNode::propertyChanged(p);
    }

    size_t SequenceIPNode::audioFillBuffer(const AudioContext& context)
    {
        lazyBuildState();

        if (!m_edlGlobalIn)
        {
            cout << "WARNING: no edl frame property: bailing on audio" << endl;
            return 0;
        }
        else if (m_edlGlobalIn->size() == 0)
        {
            return context.buffer.size();
        }

        const double rate = context.buffer.rate();
        const int numChannels = context.buffer.numChannels();
        const size_t requestedSamples = context.buffer.size();
        const ChannelsVector channels = context.buffer.channels();
        float* outBuffer = context.buffer.pointer();

        //
        //  Measured in Sequence "global" scope;
        //
        //  globalOffset - First frame of the Sequence edl.
        //
        //  startSample  - First requested sample from the Sequence.
        //
        //  readSample   - Next Sequence sample we want to read.
        //

        const int globalOffset = m_edlGlobalIn->front();
        SampleTime startSample =
            timeToSamples(context.buffer.startTime(), rate);
        SampleTime readSample = startSample;

        //
        //  We will now attempt to read samples from input Sources until we read
        //  in the the total number of requestedSamples.
        //
        //  The only other way out of this loop is if we fail in reading from an
        //  input, or fail in calculating what to read.
        //

        while ((readSample - startSample + 1) < requestedSamples)
        {
            //
            //  Look up the sequence input index for the target read region
            //  (post- offset) as well as the source input index.
            //

            int index = indexAtSample(readSample, rate, context.fps);
            int sourceIndex = (*m_edlSource)[index];

            //
            //  Measured in Sequence "global" scope;
            //
            //  inputStart        - First sample expected from targeted Sequence
            //                      input. Value is un-offset from Sequence head
            //                      to reach the beginning of the targeted
            //                      input.
            //

            int inputFrame = (*m_edlGlobalIn)[index] - globalOffset;
            SampleTime inputStart =
                frameToSample(inputFrame, context.fps, rate);

            //
            //  Normalized sample delta (scopeless);
            //
            //  samplesIntoInput  - Offset to target sample in Sequence's input.
            //
            //  samplesInSource   - Total number of samples in source
            //

            SampleTime samplesIntoInput = readSample - inputStart;

            //
            //  Measured in Source level scope;
            //
            //  sourceStartSample - Sample corresponding to very first sample in
            //  the
            //                      input Source.
            //
            //  sourceCutInSample - Sample corresponding to first sample of
            //  cutIn
            //                      frame front boundary of the input Source.
            //
            //  samplesIntoSource - Offset to first rendered audio sample in
            //  Source.
            //
            //  sourceReadStart   - Target sample in Source (where to read
            //  from).
            //

            int firstFrame = (*m_edlSourceIn)[index];
            SampleTime sourceStartSample =
                frameToSample(firstFrame, context.fps, rate);
            SampleTime sourceCutInSample = frameToSample(
                m_rangeInfos[sourceIndex].start, context.fps, rate);
            int lastFrame = (*m_edlSourceOut)[index];
            int sourceDuration = m_rangeInfos[sourceIndex].end
                                 - m_rangeInfos[sourceIndex].start + 1;
            SampleTime samplesInSource =
                frameToSample(sourceDuration, context.fps, rate);
            SampleTime samplesIntoSource =
                sourceStartSample - sourceCutInSample;
            SampleTime sourceReadStart = samplesIntoInput + samplesIntoSource;

            //
            //  The number of samples to read from this input Source is the
            //  smaller of what we want and the number of samples until the next
            //  input Source. If there are no more input Sources then the
            //  numSamples will be the remainder of what we want.
            //

            SampleTime samplesStillWanted =
                SampleTime(requestedSamples) - (readSample - startSample);
            SampleTime samplesToNextInput =
                (index < m_edlGlobalIn->size() - 2)
                    ? samplesInSource - sourceReadStart + 1
                    : requestedSamples;
            SampleTime numSamples = min(samplesStillWanted, samplesToNextInput);

            if (numSamples > 0)
            {
                AudioBuffer audioBuffer(numSamples, channels, rate,
                                        samplesToTime(sourceReadStart, rate));

                AudioContext subContext(audioBuffer, context.fps);
                const size_t numRead =
                    inputs()[sourceIndex]->audioFillBuffer(subContext);

                //
                // If we got something back then copy it into the context buffer
                // and advance the read location.
                //

                if (numRead)
                {
                    //
                    //  We can only advance as far as the number of valid
                    //  samples we read. That is the smaller value of what we
                    //  wanted and we we got back from the input. These should
                    //  match since ultimately all inputs eventually reach a
                    //  FileSource which is required to return numSamples even
                    //  when there is an error. However, just in case they don't
                    //  match we only step forward the minimum.
                    //

                    size_t advance = min(SampleTime(numRead), numSamples);
                    memcpy(outBuffer, subContext.buffer.pointer(),
                           advance * numChannels * sizeof(float));
                    outBuffer += (advance * numChannels);
                    readSample += advance;
                }
                else
                {
                    cerr << "ERROR: no samples read from input node" << endl;
                    break;
                }
            }
            else
            {
                if (IPCore::AudioRenderer::debug)
                {
                    cerr
                        << "AUDIO: invalid request for sequence audio samples: "
                        << numSamples << endl;
                }
                break;
            }
        }

        return size_t(readSample - startSample);
    }

    namespace
    {

        void outputProp(const IntProperty* p)
        {
            cout << "ERROR: " << p->name() << " ->";

            for (size_t i = 0; i < p->size(); i++)
            {
                cout << " " << (*p)[i];
            }

            cout << endl;
        }

    } // namespace

    void SequenceIPNode::testEvaluate(const Context& context,
                                      TestEvaluationResult& result)
    {
        lazyBuildState();

        int frame = context.frame;
        const IPNodes& ins = inputs();

        if (ins.size() > 1)
        {
            EvalPoint ep = evaluationPoint(frame);
            const int source = ep.sourceIndex;

            if (source < 0 || source >= ins.size())
            {
                outputProp(m_edlSource);
                outputProp(m_edlGlobalIn);
                outputProp(m_edlSourceIn);
                outputProp(m_edlSourceOut);

                TWK_THROW_STREAM(SequenceOutOfBoundsExc,
                                 "Bad Sequence EDL source number "
                                     << source << " is not in range [0,"
                                     << ins.size() - 1 << "]");
            }

            Context newContext = context;
            newContext.frame = ep.sourceFrame;
            ins[source]->testEvaluate(newContext, result);
        }
        else if (ins.size() == 1)
        {
            ins[0]->testEvaluate(context, result);
        }
    }

    void SequenceIPNode::metaEvaluate(const Context& context,
                                      MetaEvalVisitor& visitor)
    {
        lazyBuildState();

        visitor.enter(context, this);
        const IPNodes& ins = inputs();

        if (ins.size() >= 1)
        {
            EvalPoint ep = evaluationPoint(context.frame);
            const int source = ep.sourceIndex;

            if (source < 0 || source >= ins.size())
            {
                TWK_THROW_STREAM(SequenceOutOfBoundsExc,
                                 "Bad Sequence EDL source number "
                                     << source << " is not in range [0,"
                                     << ins.size() - 1 << "]");
            }

            Context c = context;
            c.frame = ep.sourceFrame;

            if (visitor.traverseChild(c, source, this, ins[source]))
            {
                ins[source]->metaEvaluate(c, visitor);
            }
        }
        else if (ins.size() == 1)
        {
            if (visitor.traverseChild(context, 0, this, ins[0]))
            {
                ins[0]->metaEvaluate(context, visitor);
            }
        }

        visitor.leave(context, this);
    }

    void SequenceIPNode::mapInputToEvalFrames(size_t inputIndex,
                                              const FrameVector& inframes,
                                              FrameVector& outframes) const
    {
        lazyBuildState();

        const IntProperty::container_type& src = m_edlSource->valueContainer();
        const IntProperty::container_type& in = m_edlSourceIn->valueContainer();
        const IntProperty::container_type& out =
            m_edlSourceOut->valueContainer();
        const IntProperty::container_type& global =
            m_edlGlobalIn->valueContainer();

        if (m_autoEDL)
        {
            //
            //  In this case no need to search for it
            //

            const size_t i = inputIndex;

            for (size_t q = 0; q < inframes.size(); q++)
            {
                int frame = inframes[q];

                if (in[i] <= frame && out[i] >= frame)
                {
                    outframes.push_back(global[i] + frame - in[i]);
                }
                else
                {
                    // something is wrong here
                    outframes.push_back(global[i] + frame - in[i]);
                }
            }
        }
        else
        {
            //
            //  Have to search all possible cuts because the same input
            //  could be cut to more than once. Cache the cuts that
            //  include the src index up front so we don't have to keep
            //  searching. Loop over the inframes and seach all of the
            //  relevant cuts.
            //

            vector<int> srcIndices;

            for (size_t i = 0; i < src.size(); i++)
            {
                if (src[i] == inputIndex)
                {
                    srcIndices.push_back(i);
                }
            }

            for (size_t q = 0; q < inframes.size(); q++)
            {
                bool found = false;
                int frame = inframes[q];

                for (size_t i = 0; i < srcIndices.size(); i++)
                {
                    if (in[i] <= frame && out[i] >= frame)
                    {
                        outframes.push_back(global[i] + (frame - in[i]));
                        found = true;
                    }
                }

                if (!found)
                    outframes.push_back(frame);
            }
        }
    }

    void SequenceIPNode::lazyBuildState() const
    {
        LockGuard guard(m_mutex);

        if (m_changing)
            // we dont to do a build state in an event
            return;

        bool needBuildState =
            m_updateHiddenData || (m_updateEDL && m_autoEDL->front());
        if (!needBuildState)
            return;

        m_changing = true;
        m_changingSignal();

        if (m_updateHiddenData)
            updateInputData();

        //
        //  To avoid accidentally blowing away a pre-built EDL
        //  we should only create the default when in autoEDL mode
        //
        if (m_updateEDL && m_autoEDL->front())
            createDefaultEDL();

        m_changing = false;
        m_changedSignal();
    }

    void SequenceIPNode::propagateFlushToInputs(const FlushContext& context)
    {
        EvalPoint ep = evaluationPoint(context.frame);

        if (ep.sourceIndex >= 0 && ep.sourceIndex < inputs().size())
        {
            FlushContext newContext = context;
            newContext.frame = ep.sourceFrame;
            inputs()[ep.sourceIndex]->propagateFlushToInputs(newContext);
        }
    }

    bool SequenceIPNode::getSourceRange(int sourceIndex,
                                        ImageRangeInfo& rangeInfo,
                                        int& sourceOffset)
    {
        if (sourceIndex < 0 || sourceIndex >= m_rangeInfos.size()
            || sourceIndex >= m_edlGlobalIn->size())
            return false;

        rangeInfo = m_rangeInfos[sourceIndex];
        sourceOffset = (*m_edlGlobalIn)[sourceIndex];

        return true;
    }

    void SequenceIPNode::invalidate() { m_updateHiddenData = true; }

} // namespace IPCore
