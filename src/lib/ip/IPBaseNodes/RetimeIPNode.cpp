//******************************************************************************
// Copyright (c) 2010 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/RetimeIPNode.h>
#include <IPBaseNodes/RetimeGroupIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <TwkAudio/Audio.h>
#include <TwkFB/FrameBuffer.h>
#include <iostream>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkAudio;
    using namespace TwkMath;
    using namespace TwkFB;
    using namespace boost;
    using namespace std;

    RetimeIPNode::RetimeIPNode(const std::string& name,
                               const NodeDefinition* def, IPGraph* g,
                               GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_vscale(0)
        , m_voffset(0)
        , m_ascale(0)
        , m_aoffset(0)
        , m_outputFrame(0)
        , m_inputFrame(0)
        , m_warpActive(0)
        , m_warpKeyFrames(0)
        , m_warpKeyRates(0)
        , m_warpDataValid(false)
        , m_explicitActive(0)
        , m_explicitFirstOutputFrame(0)
        , m_explicitFirstInputFrame(0)
        , m_explicitInputFrames(0)
        , m_explicitDataValid(false)
        , m_grainDuration(0.022)
        , m_grainEnvelope(0.006)
    {
        m_vscale = declareProperty<FloatProperty>("visual.scale", 1.0);
        m_voffset = declareProperty<FloatProperty>("visual.offset", 0.0);
        m_ascale = declareProperty<FloatProperty>("audio.scale", 1.0);
        m_aoffset = declareProperty<FloatProperty>("audio.offset", 0.0);
        m_fps = declareProperty<FloatProperty>("output.fps", 0.0);

        //
        //  Time warp properties (key arrays are size 0 by default)
        //

        m_warpActive = declareProperty<IntProperty>("warp.active", false);
        m_warpStyle = declareProperty<IntProperty>("warp.style", 0);
        m_warpKeyFrames = declareProperty<IntProperty>("warp.keyFrames");
        m_warpKeyRates = declareProperty<FloatProperty>("warp.keyRates");

        //
        //  Explicit frame remapping array.
        //
        //  * Each integer in the array is the input frame.
        //  * Each index in the array designates the output frame (startFrame +
        //  index).
        //  * The number of frames in the output will = the length of the array.
        //
        //  If the "explicit.active" flag is true it takes precidence over other
        //  "retiming" styles.
        //

        m_explicitActive =
            declareProperty<IntProperty>("explicit.active", false);
        m_explicitFirstOutputFrame =
            declareProperty<IntProperty>("explicit.firstOutputFrame", 1);
        m_explicitInputFrames =
            declareProperty<IntProperty>("explicit.inputFrames");

        setMaxInputs(1);
    }

    RetimeIPNode::~RetimeIPNode() {}

    bool RetimeIPNode::testInputs(const IPNodes& inputs,
                                  std::ostringstream& msg) const
    {
        if (inputs.size() > 1)
        {
            string un = uiName();
            string n = name();

            if (un == n)
                msg << n;
            else
                msg << un << " (" << n << ")";
            msg << " cannot have more than one input" << endl;
            return false;
        }

        return IPNode::testInputs(inputs, msg);
    }

    void RetimeIPNode::inputChanged(int index)
    {
        //
        //  Without knowing what's upstream treat this like inputRangeChanged()
        //  to cover this case
        //

        inputRangeChanged(index);
    }

    void RetimeIPNode::inputRangeChanged(int index, PropagateTarget target)
    {
        {
            ScopedLock lock(m_nodeMutex);
            m_warpDataValid = false;
            m_explicitDataValid = false;
        }

        //
        //  Force the input range fields to become up-to-date
        //

        imageRangeInfo();
    }

    void RetimeIPNode::setInputs(const IPNodes& nodes)
    {
        if (nodes.size() && nodes != inputs())
        {
            // Should this be calling imageRangeInfo() on this to update the
            // m_inputInfo in the case of non-linear retiming?
            m_inputInfo = nodes.front()->imageRangeInfo();
            //
            //  Only reset the fps if we've never set it, or we're replacing
            //  existing inputs
            //
            if (inputs().size() || m_fps->front() == 0.0)
            {
                m_fps->front() = m_inputInfo.fps;
                m_fpsDetected = true;
            }
        }

        IPNode::setInputs(nodes);
        propagateRangeChange();
        propagateImageStructureChange();
    }

    //
    //  Regenerate explicit reverse mapping data if necessary.  Called only when
    //  "explicit" is active.
    //

    void RetimeIPNode::updateExplicitData() const
    {
        //
        //  Lock other threads out while we check validity of and possibly
        //  update the data.
        //
        ScopedLock lock(m_nodeMutex);

        if (!m_explicitDataValid)
        {
            m_explicitInToOut.resize(0);

            if (!explicitPropertiesOK())
                return;

            //
            //  Find output Frame Range
            //

            int minInputFrame = (*m_explicitInputFrames)[0];
            int maxInputFrame = (*m_explicitInputFrames)[0];

            for (int i = 0; i < m_explicitInputFrames->size(); ++i)
            {
                int f = (*m_explicitInputFrames)[i];
                if (f < minInputFrame)
                    minInputFrame = f;
                if (f > maxInputFrame)
                    maxInputFrame = f;
            }

            m_explicitFirstInputFrame = minInputFrame;

            //
            //  A given input frame may be mapped to many output frames, but we
            //  want this mapping to provide the _smallest_ output frame a given
            //  inputframe is mapped to, so minimize as we go.
            //

            m_explicitInToOut.resize(maxInputFrame - minInputFrame + 1,
                                     numeric_limits<int>::max());

            for (int i = 0; i < m_explicitInputFrames->size(); ++i)
            {
                int inIndex = (*m_explicitInputFrames)[i] - minInputFrame;
                ;
                int outF = i + m_explicitFirstOutputFrame->front();

                if (m_explicitInToOut[inIndex] > outF)
                    m_explicitInToOut[inIndex] = outF;
            }

            //
            //  Fill in any gaps in in-to-out map
            //

            int outFrame = m_explicitFirstOutputFrame->front();
            for (int i = 0; i < m_explicitInToOut.size(); ++i)
            {
                if (m_explicitInToOut[i] != numeric_limits<int>::max())
                    outFrame = m_explicitInToOut[i];
                else
                    m_explicitInToOut[i] = outFrame;
            }

            /*
            cerr << "explicitInToOut: " << endl;
            for (int i = 0; i < m_explicitInToOut.size(); ++i)
            {
                cerr << "    " << (minInputFrame+i) << " -> " <<
            m_explicitInToOut[i] << endl;
            }
            */

            m_explicitDataValid = true;
        }
    }

    //
    //  Regenerate timewarp data if necessary.  Called only when warp is active.
    //

    void RetimeIPNode::updateWarpData() const
    {
        //
        //  Lock other threads out while we check validity of and possubply
        //  update the warp data.
        //
        ScopedLock lock(m_nodeMutex);

        if (!m_warpDataValid)
        {
            //
            //  Check warp keys
            //
            if (m_warpKeyFrames->size() != m_warpKeyRates->size())
            {
                cerr << "WARNING: warp key numbers don't match, skipping warp."
                     << endl;
                return;
            }
            int warpKeyIndex = -1;
            if (m_warpKeyFrames->size())
            {
                warpKeyIndex = 0;
            }

            m_warpedInToOut.resize(0);
            m_warpedOutToIn.resize(0);

            //
            //  The TimeSteps are used to move forward in time, using the
            //  condition that neither the input nor output frame can changed in
            //  an amount of time that is _less_ than both TimeSteps.
            //
            //  Note that which step is larger can change as we animate the rate
            //  multiplier.  At each iteration, we add the smaller of the two
            //  timesteps to both Gaps.
            //
            float inputTimeStep = 1.0 / m_inputInfo.fps;
            float outputTimeStep = 1.0 / m_fps->front();

            //
            //  InputGap and OutputGap track the amount of time that has passed
            //  since the corresponding sequence last had a frame added.  When
            //  the Gap > TimeStep - eps, we add a frame.
            //
            float inputGap = 0.0;
            float outputGap = 0.0;
            float EPSILON = 0.0001;

            //
            //  Always map first input frame to first output frame, and vs/vs
            //
            m_warpedInToOut.push_back(0);
            m_warpedOutToIn.push_back(0);

            /*
            cerr << "start " << m_inputInfo.start << " end " << m_inputInfo.end
                    << " in fps " << m_inputInfo.fps << " target fps " <<
            m_fps->front() << endl;
            */

            while (true)
            {
                //
                //  Find true outputTimeStep
                //

                int inputFrame = m_warpedInToOut.size() - 1;
                int outputFrame = m_warpedOutToIn.size() - 1;
                float scale = 1.0;

                //
                //  Find pair of rate scales to interpolate.
                //
                if (warpKeyIndex >= 0)
                {
                    IntProperty& frames = *m_warpKeyFrames;
                    FloatProperty& rates = *m_warpKeyRates;

                    float aScale, bScale;
                    int aFrame, bFrame;

                    //
                    //  make sure our lower bound is still greatest lower bound
                    //
                    int nextKeyIndex = warpKeyIndex;
                    while (nextKeyIndex < frames.size()
                           && inputFrame
                                  >= frames[nextKeyIndex] - m_inputInfo.start)
                    {
                        ++nextKeyIndex;
                    }

                    if (nextKeyIndex == frames.size())
                    //
                    //  We ran off the end of the keys array
                    //
                    {
                        warpKeyIndex = frames.size() - 1;
                        aScale = bScale = rates[warpKeyIndex];
                        aFrame = bFrame =
                            frames[warpKeyIndex] - m_inputInfo.start;
                    }
                    else
                    {
                        if (warpKeyIndex != nextKeyIndex)
                            warpKeyIndex = nextKeyIndex - 1;
                        //  cerr << "warpKeyIndex " << warpKeyIndex << "
                        //  nextKeyIndex " << nextKeyIndex << endl;

                        aScale = rates[warpKeyIndex];
                        aFrame = frames[warpKeyIndex] - m_inputInfo.start;

                        bScale = (warpKeyIndex < frames.size() - 1)
                                     ? rates[warpKeyIndex + 1]
                                     : aScale;
                        bFrame =
                            (warpKeyIndex < frames.size() - 1)
                                ? frames[warpKeyIndex + 1] - m_inputInfo.start
                                : aFrame;
                    }

                    if (m_warpStyle->front() == 0)
                    {
                        //
                        //  Linear interpolation
                        //
                        if (inputFrame < aFrame)
                            scale = aScale;
                        else if (inputFrame >= bFrame)
                            scale = bScale;
                        else
                        {
                            float t = float(inputFrame - aFrame)
                                      / float(bFrame - aFrame);
                            scale = (1.0 - t) * aScale + t * bScale;
                        }
                    }
                    else
                    {
                        //
                        //  Constant "interpolation"
                        //
                        if (inputFrame < bFrame)
                            scale = aScale;
                        else if (inputFrame >= bFrame)
                            scale = bScale;
                    }
                    //  cerr << "scale " << scale << " aFrame " << aFrame << "
                    //  aScale " << aScale << " bFrame " << bFrame << " bScale "
                    //  << bScale << endl;
                }

                outputTimeStep = scale * 1.0 / m_fps->front();

                //
                //  Advance both gaps by the smaller of the two steps.
                //

                float step = outputTimeStep;
                if (inputTimeStep < outputTimeStep)
                    step = inputTimeStep;

                inputGap += step;
                outputGap += step;

                //
                //  Check to see if the gap is big enough to require a new
                //  frame in either input or output sequence.
                //
                bool needInputFrame = (inputGap > (inputTimeStep - EPSILON));
                bool needOutputFrame = (outputGap > (outputTimeStep - EPSILON));

                //
                //  Stop mapping process if we're about to run off the end of
                //  the input sequence.
                //
                if (needInputFrame
                    && (inputFrame + 1)
                           > (m_inputInfo.end - m_inputInfo.start + EPSILON))
                    break;

                //
                //  Add new frames as appropriate, and reset gap.  the
                //  "targetFrame for each mapping is 1 + the "current" frame in
                //  the other mapping if we are adding a frame to that mapping
                //  too, otherwise we "hold" the last frame we mapped to.
                //

                float oldInputGap = inputGap;
                float oldOutputGap = outputGap;

                if (needInputFrame)
                {
                    int targetOutput = (needOutputFrame)
                                           ? (outputFrame + 1)
                                           : m_warpedInToOut.back();
                    m_warpedInToOut.push_back(targetOutput);
                    inputGap = 0.0;
                }
                if (needOutputFrame)
                {
                    int targetInput = (needInputFrame) ? (inputFrame + 1)
                                                       : m_warpedOutToIn.back();
                    m_warpedOutToIn.push_back(targetInput);
                    outputGap = 0.0;
                }

                /*
                cerr << "step " << step << " (in fps " << 1.0/inputTimeStep <<
                ", out fps " << 1.0/outputTimeStep << ")" << endl;

                cerr << "    input:  f " << inputFrame  << " gap " <<
                oldInputGap  << " -> " << inputGap; if (needInputFrame)  cerr <<
                ", frame "  << m_warpedInToOut.size()-1 << " -> " <<
                m_warpedInToOut.back(); cerr << endl; cerr << "    output: f "
                << outputFrame << " gap " << oldOutputGap << " -> " <<
                outputGap; if (needOutputFrame) cerr << ", frame " <<
                m_warpedOutToIn.size()-1 << " -> " << m_warpedOutToIn.back();
                cerr << endl;
                */
            }
            m_warpDataValid = true;
        }
    }

    //
    //  retimedFrame(f) returns 'the frame that was retimed to frame f',
    //  IE maps the output to the input of the retime node.
    //
    //  If we are scaling down (decreasing length, scale < 1.0), this is a
    //  one-to-many mapping, so this function returns the first of
    //  possibly several input frames that the output frame is mapped
    //  from.
    //

    int RetimeIPNode::retimedFrame(int frame) const
    {
        if (m_explicitActive->front())
        {
            if (!explicitPropertiesOK())
                return 1;

            int index = frame - m_explicitFirstOutputFrame->front();

            if (index < 0)
                index = 0;
            if (index >= m_explicitInputFrames->size())
                index = m_explicitInputFrames->size() - 1;

            return (*m_explicitInputFrames)[index];
        }

        if (m_warpActive->front())
        {
            updateWarpData();

            if (frame < 1)
                frame = 1;
            if (frame > m_warpedOutToIn.size())
                frame = m_warpedOutToIn.size();

            return (m_inputInfo.start + m_warpedOutToIn[frame - 1]);
        }

        float scale = m_vscale->front();
        float offset = m_voffset->front();
        float fps = m_fps->front();

        if (fps != m_inputInfo.fps && m_inputInfo.fps != 0.0)
        {
            //
            //  FPS is stored in session file with only 6 sig figs so
            //  abort retimes that result from tiny fps differences,
            //  otherwise an fps=23.9762001038 movie will be retimed
            //  upon reload from session file.
            //
            float fpsScale = fps / m_inputInfo.fps;
            if (fabs(fpsScale - 1.0) > 0.00001)
            {
                scale *= fpsScale;
            }
        }

        if (scale == 0.0)
            scale = 1.0;
        const float fframe = float(frame - m_inputInfo.start) / scale;
        //
        //  Note, don't round here, because we want boundaries between
        //  frames to stay in the same place, not centers of frames.
        //
        int inFrame = int(fframe) + m_inputInfo.start - offset;
        return inFrame;
    }

    //
    //  invRetimedFrame(f) returns 'the frame that frame f is retimed to',
    //  IE maps the input to the output of the retime node.
    //
    //  If we are scaling up (increasing length, scale > 1.0), this is a
    //  one-to-many mapping, so this function returns the first of
    //  possibly several output frames that the input frame is mapped
    //  to.
    //

    int RetimeIPNode::invRetimedFrame0(int frame) const
    {
        float scale = m_vscale->front();
        float offset = m_voffset->front();
        float fps = m_fps->front();

        if (fps != m_inputInfo.fps && m_inputInfo.fps != 0.0)
        {
            //
            //  FPS is stored in session file with only 6 sig figs so
            //  abort retimes that result from tiny fps differences,
            //  otherwise an fps=23.9762001038 movie will be retimed
            //  upon reload from session file.
            //
            float fpsScale = fps / m_inputInfo.fps;
            if (fabs(fpsScale - 1.0) > 0.00001)
            {
                scale *= fpsScale;
            }
        }

        const float fframe = float(frame - m_inputInfo.start + offset) * scale;

        //
        // We're trying to match retime's input. The rounding makes this
        // more accurate.
        //
        int outFrame = m_inputInfo.start + int(fframe);
        return outFrame;
    }

    int RetimeIPNode::invRetimedFrame(int frame) const
    {
        if (m_explicitActive->front())
        {
            updateExplicitData();

            if (m_explicitInToOut.empty())
                return m_explicitFirstOutputFrame->front();

            int index = frame - m_explicitFirstInputFrame;

            if (index < 0)
                index = 0;
            if (index >= m_explicitInToOut.size())
                index = m_explicitInToOut.size() - 1;

            return m_explicitInToOut[index];
        }

        if (m_warpActive->front())
        {
            updateWarpData();

            if (frame < m_inputInfo.start)
                frame = m_inputInfo.start;
            if (frame > m_inputInfo.end)
                frame = m_inputInfo.end;

            return (1 + m_warpedInToOut[frame - m_inputInfo.start]);
        }

        int f = invRetimedFrame0(frame);
        int fi = retimedFrame(f);

        if (frame != fi)
        {
            int f0 = invRetimedFrame0(frame - 1);
            int f1 = invRetimedFrame0(frame + 1);

            if (f0 > f1)
                std::swap(f0, f1);

            for (int g = f0; g <= f1; g++)
            {
                if (retimedFrame(g) == frame)
                    return g;
            }
        }

        return f;
    }

    void RetimeIPNode::mapInputToEvalFrames(size_t index,
                                            const FrameVector& inframes,
                                            FrameVector& outframes) const
    {
        if (size_t s = inframes.size())
        {
            for (size_t q = 0; q < s; q++)
            {
                outframes.push_back(invRetimedFrame(inframes[q]));
            }
        }
    }

    IPImage* RetimeIPNode::evaluate(const Context& context)
    {
        IPImage* img = 0;

        if (inputs().empty())
        {
            return IPImage::newNoImage(this);
        }
        else
        {
            IPNode::Context newContext = context;
            newContext.frame = retimedFrame(context.frame);
            img = IPNode::evaluate(newContext);
        }
        //
        //  Only take ownership of this image if we are part of a
        //  RetimeGroup, not an ancillary node of a SequenceGroup, for
        //  example.
        //
        if (img && group() && dynamic_cast<const RetimeGroupIPNode*>(group()))
            img->node = this;

        return img;
    }

    IPNode::ImageStructureInfo
    RetimeIPNode::imageStructureInfo(const Context& context) const
    {
        if (inputs().size())
        {
            int newFrame = retimedFrame(context.frame);
            return inputs().front()->imageStructureInfo(
                graph()->contextForFrame(newFrame));
        }

        return ImageStructureInfo();
    }

    void RetimeIPNode::metaEvaluate(const Context& context,
                                    MetaEvalVisitor& visitor)
    {
        Context c = context;
        c.frame = retimedFrame(context.frame);
        IPNode::metaEvaluate(c, visitor);
    }

    void RetimeIPNode::testEvaluate(const Context& context,
                                    TestEvaluationResult& result)
    {
        Context newContext = context;
        newContext.frame = retimedFrame(context.frame);
        IPNode::testEvaluate(newContext, result);
    }

    void RetimeIPNode::flushAllCaches(const FlushContext& context)
    {
        FlushContext newContext = context;
        newContext.frame = retimedFrame(context.frame);
        IPNode::flushAllCaches(newContext);
    }

    void RetimeIPNode::propagateFlushToInputs(const FlushContext& context)
    {
        FlushContext newContext = context;
        newContext.frame = retimedFrame(context.frame);
        IPNode::propagateFlushToInputs(newContext);
    }

    void RetimeIPNode::propagateFlushToOutputs(const FlushContext& context)
    {
        FlushContext newContext = context;
        newContext.frame = invRetimedFrame(context.frame);
        IPNode::propagateFlushToOutputs(newContext);
    }

    IPImageID* RetimeIPNode::evaluateIdentifier(const Context& context)
    {
        Context newContext = context;
        newContext.frame = retimedFrame(context.frame);
        return IPNode::evaluateIdentifier(newContext);
    }

    IPNode::ImageRangeInfo RetimeIPNode::imageRangeInfo() const
    {
        ImageRangeInfo info = IPNode::imageRangeInfo();
        m_inputInfo = info;

        //
        //  If we _still_ haven't set the fps yet try again. Worst case info.fps
        //  is zero and we try again later.
        //

        if (m_fps->front() == 0.0)
        {
            m_fps->front() = info.fps;
            m_fpsDetected = true;
        }

        //
        //  Explict mapping overrides any rangeInfo that comes from the input,
        //  since evaluation of the retime node's input will always just ask for
        //  specific frames.
        //

        if (m_explicitActive->front())
        {
            // NOTE: This is updating m_inputInfo which is required in the case
            // of nonlinear retime. Should this be happening in
            // updateExplicitData()?

            info.start = info.cutIn = m_explicitFirstOutputFrame->front();
            info.end = info.cutOut =
                info.start + m_explicitInputFrames->size() - 1;
            //  cerr << "rinfo s " << info.start << " i " << info.cutIn << " o "
            //  << info.cutOut << " e " << info.end << endl;

            //  Keep incoming FPS

            return info;
        }

        int istart = invRetimedFrame(info.start);
        int iend = invRetimedFrame(info.end);
        int iin = invRetimedFrame(info.cutIn);
        int iout = invRetimedFrame(info.cutOut);

        if (info.cutIn == info.start)
            iin = istart;
        if (info.cutOut == info.end)
            iout = iend;

        /*
        info.start  = 1;
        info.end    = std::abs(iend - istart);
        info.cutIn  = std::abs(iin - istart);
        info.cutOut = std::abs(iout - istart);
        */
        if (fabs(m_vscale->front()) > 1.0 || info.fps != m_fps->front())
        {
            //  invRetimedFrame returns the _smallest_ outFrame such
            //  retimedFram(outFrame) = inFrame
            //  but for the out and end, we want the _largest_
            //  (in abs value)

            int inc = (m_vscale->front() > 0.0) ? 1.0 : -1.0;

            while (retimedFrame(iend + inc) == info.end)
            {
                if (m_warpActive->front()
                    && iend + inc > m_warpedOutToIn.size())
                    break;
                iend += inc;
            }
            while (retimedFrame(iout + inc) == info.cutOut)
            {
                if (m_warpActive->front()
                    && iout + inc > m_warpedOutToIn.size())
                    break;
                iout += inc;
            }
        }

        //
        //  If we reversed the order, swap them
        //
        if (istart > iend)
        {
            int tmp = istart;
            istart = iend;
            iend = tmp;
        }
        if (iin > iout)
        {
            int tmp = iin;
            iin = iout;
            iout = tmp;
        }

        info.start = istart;
        info.end = iend;
        info.cutIn = iin;
        info.cutOut = iout;
        info.fps = m_fps->front();

        return info;
    }

    void RetimeIPNode::propertyChanged(const Property* p)
    {
        propagateRangeChange();
        IPNode::propertyChanged(p);
    }

    size_t RetimeIPNode::audioFillBuffer(const AudioContext& context)
    {
        double fps = m_fps->front();
        double fpsRatio = context.fps / fps;
        double vfactor = fpsRatio / m_vscale->front();

        double factor = vfactor * m_ascale->front();

        //
        //  The vffset has already been accounted for at this point,
        //  perhaps because the frame in the context has been retimed ?
        //  Anyway, don't add it in again here.
        //
        double poffset = -m_aoffset->front() /*- (m_voffset->front() / fps)*/;

        bool reversed = factor < 0;
        factor = factor < 0 ? -factor : factor;

        Time startTime = context.buffer.startTime() + Time(poffset);
        size_t nsamples = context.buffer.size();

        if (reversed)
        {
            startTime = Time(m_inputInfo.end - m_inputInfo.start)
                        / Time(m_inputInfo.fps);
            startTime -= samplesToTime(nsamples, context.buffer.rate());
            startTime -= context.buffer.startTime();
        }

        AudioBuffer buffer(context.buffer, 0, nsamples, startTime);
        AudioContext newContext(buffer, m_inputInfo.fps);

        if (factor == 1.0)
        {
            if (reversed)
            {
                size_t n = IPNode::audioFillBuffer(newContext);
                buffer.reverse();
                return n;
            }
            else
            {
                //
                //  Normal playback -- no stretching or compressing
                //

                return IPNode::audioFillBuffer(newContext);
            }
        }

        const Time rate = buffer.rate();
        const size_t ch = buffer.numChannels();
        const size_t start = buffer.startSample();
        const size_t num = buffer.size();
        const size_t end = start + num;

        const size_t grainSamples =
            timeToSamples(m_grainDuration, buffer.rate());
        const size_t grainMarginSamples =
            timeToSamples(m_grainEnvelope, buffer.rate());
        const size_t grainSyncWidth = grainSamples + grainMarginSamples;

        const size_t sync0 = start / grainSyncWidth;
        const size_t syncN = (start + num) / grainSyncWidth;
        const size_t grainSize = grainSyncWidth + grainMarginSamples;

        float* out = buffer.pointer();

        for (size_t i = sync0; i <= syncN; i++)
        {
            //
            //  Locate the grain we want. t is relative to the output
            //  time.
            //

            Time t = i * samplesToTime(grainSyncWidth, rate)
                     - samplesToTime(grainMarginSamples, rate);
            Time d = samplesToTime(grainSize, rate);

            if (t < 0)
                t = 0;

            AudioBuffer grain(grainSize, buffer.channels(), rate, t * factor);
            AudioContext rcontext(grain, newContext.fps); // !!!! fps???

            if (IPNode::audioFillBuffer(rcontext) == 0)
                break;

            //
            //  render the grain
            //

            const float* in = grain.pointer();
            size_t gpos = timeToSamples(t, rate);

            for (size_t q = 0; q < grainSize; q++, gpos++)
            {
                if (gpos < start)
                    continue;
                if (gpos >= end)
                    break;

                double env = 1.0;

                if (q < grainMarginSamples)
                {
                    env = double(q) / double(grainMarginSamples);
                }
                else if (q > grainSyncWidth)
                {
                    env = 1.0
                          - double(q - grainSyncWidth)
                                / double(grainMarginSamples);
                }

                for (size_t c = 0; c < ch; c++)
                {
                    out[(gpos - start) * ch + c] += env * in[q * ch + c];
                }
            }
        }

        return buffer.size();
    }

    bool RetimeIPNode::explicitPropertiesOK() const
    {
        if (m_explicitInputFrames->empty())
        {
            cerr << "ERROR: retime node '" << name()
                 << "': explicit retiming active, but no input frames set"
                 << endl;
            return false;
        }

        return true;
    }

    void RetimeIPNode::resetFPS()
    {
        if (m_fpsDetected)
        {
            m_inputInfo = IPNode::imageRangeInfo();
            m_fps->front() = m_inputInfo.fps;
        }
    }

} // namespace IPCore
