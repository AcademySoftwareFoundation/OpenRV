//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/SequenceGroupIPNode.h>
#include <IPBaseNodes/SequenceIPNode.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/AudioAddIPNode.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/PropertyEditor.h>
#include <TwkUtil/File.h>
#include <TwkUtil/sgcHop.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;

    bool SequenceGroupIPNode::m_defaultAutoRetime = true;

    SequenceGroupIPNode::SequenceGroupIPNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
        , m_maxFPS(0.0)
        , m_soundTrackNode(0)
        , m_audioAddNode(0)
        , m_soundTrackRetime(0)
    {
        declareProperty<StringProperty>("ui.name", name);

        m_useMaxFPS = def->stringValue("defaults.autoFPSType", "") == "max";

        m_soundTrackFile =
            declareProperty<StringProperty>("soundtrack.file", "");
        m_soundTrackOffset =
            declareProperty<FloatProperty>("soundtrack.offset", 0.0f);

        m_sequenceNode =
            newMemberNodeOfType<SequenceIPNode>(sequenceType(), "sequence");
        m_sequenceNode->setVolatileInputs(m_useMaxFPS);
        setRoot(m_sequenceNode);

        m_retimeToOutput = declareProperty<IntProperty>(
            "timing.retimeInputs", m_defaultAutoRetime ? 1 : 0);

        m_markersIn = declareProperty<IntProperty>("markers.in");
        m_markersOut = declareProperty<IntProperty>("markers.out");
        m_markersColor = declareProperty<Vec4fProperty>("markers.color");
        m_markersName = declareProperty<StringProperty>("markers.name");
    }

    SequenceGroupIPNode::~SequenceGroupIPNode()
    {
        //
        //  The GroupIPNode will delete m_root
        //

        m_sequenceNode = 0;
        m_audioAddNode = 0;
        m_soundTrackNode = 0;
    }

    string SequenceGroupIPNode::sequenceType()
    {
        return definition()->stringValue("defaults.sequenceType", "Sequence");
    }

    string SequenceGroupIPNode::retimeType()
    {
        return definition()->stringValue("defaults.retimeType", "Retime");
    }

    string SequenceGroupIPNode::paintType()
    {
        return definition()->stringValue("defaults.paintType", "Paint");
    }

    string SequenceGroupIPNode::audioAddType()
    {
        return definition()->stringValue("defaults.audioAddType", "AudioAdd");
    }

    string SequenceGroupIPNode::audioSourceType()
    {
        return definition()->stringValue("defaults.audioSourceType",
                                         "FileSource");
    }

    IPNode* SequenceGroupIPNode::newSubGraphForInput(size_t index,
                                                     const IPNodes& newInputs)
    {
        float fps = m_sequenceNode->imageRangeInfo().fps;
        if (fps == 0.0 && !newInputs.empty())
        {
            auto const imageRangeInfo = newInputs[index]->imageRangeInfo();
            if (!imageRangeInfo.isUndiscovered)
                fps = imageRangeInfo.fps;
        }

        bool retimeInputs = m_retimeToOutput->front() ? true : false;

        if (m_useMaxFPS)
        {
            if (m_sequenceNode->outputFPSProperty()->front() != m_maxFPS)
            {
                m_sequenceNode->outputFPSProperty()->front() = m_maxFPS;
            }
        }

        AdaptorIPNode* anode = newAdaptorForInput(newInputs[index]);
        IPNode* paintNode =
            newMemberNodeForInput(paintType(), newInputs[index], "p");

        paintNode->setInputs1(anode);

        IPNode* node = paintNode;

        //
        //  Always add a retime node, since otherwise we have timing issues with
        //  setting properties on nodes that don't exist when reading session
        //  files, etc.
        //

        if (fps != 0.0f && retimeInputs || m_useMaxFPS)
        {
            IPNode* retimer =
                newMemberNodeForInput(retimeType(), newInputs[index], "rt");
            retimer->setInputs1(node);
            retimer->setProperty<FloatProperty>("output.fps",
                                                m_useMaxFPS ? m_maxFPS : fps);
            node = retimer;
        }

        return node;
    }

    IPNode* SequenceGroupIPNode::modifySubGraphForInput(
        size_t index, const IPNodes& newInputs, IPNode* subgraph)
    {
        bool retimeInputs = m_retimeToOutput->front() ? true : false;

        if (retimeInputs && subgraph->protocol() != retimeType())
        {
            auto const inputRangeInfo = newInputs[index]->imageRangeInfo();
            if (!inputRangeInfo.isUndiscovered)
            {
                IPNode* innode = newInputs[index];
                IPNode* retimer =
                    newMemberNodeForInput(retimeType(), innode, "rt");
                retimer->setInputs1(subgraph);
                retimer->setProperty<FloatProperty>("output.fps",
                                                    inputRangeInfo.fps);
                subgraph = retimer;
            }
        }

        if ((retimeInputs || m_useMaxFPS)
            && subgraph->protocol() != retimeType())
        {
            auto const inputRangeInfo = newInputs[index]->imageRangeInfo();
            if (!inputRangeInfo.isUndiscovered)
            {
                IPNode* innode = newInputs[index];
                IPNode* retimer =
                    newMemberNodeForInput(retimeType(), innode, "rt");
                retimer->setInputs1(subgraph);
                retimer->setProperty<FloatProperty>("output.fps",
                                                    inputRangeInfo.fps);
                subgraph = retimer;
            }
        }

        if (subgraph->protocol() == retimeType())
        {
            float retimeFPS = 0.0f;
            if (retimeInputs)
                retimeFPS = m_useMaxFPS ? m_maxFPS
                                        : m_sequenceNode->imageRangeInfo().fps;
            else
                retimeFPS = newInputs[index]->imageRangeInfo().fps;

            subgraph->setProperty<FloatProperty>("output.fps", retimeFPS);
            m_sequenceNode->invalidate();
        }

        return subgraph;
    }

    void SequenceGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        HOP_PROF_FUNC();
        if (m_sequenceNode && !isDeleting())
        {
            if (m_useMaxFPS)
            {
                m_maxFPS = 0;

                for (size_t i = 0; i < newInputs.size(); i++)
                {
                    m_maxFPS =
                        std::max(newInputs[i]->imageRangeInfo().fps, m_maxFPS);
                }

                m_sequenceNode->outputFPSProperty()->front() = m_maxFPS;
            }

            setInputsWithReordering(newInputs, m_sequenceNode);
        }
        else
        {
            IPNode::setInputs(newInputs);
        }
    }

    void SequenceGroupIPNode::rebuild(int inputIndex)
    {
        if (!isDeleting())
        {
            graph()->beginGraphEdit();
            IPNodes cachedInputs = inputs();
            if (inputIndex >= 0)
                setInputsWithReordering(cachedInputs, m_sequenceNode,
                                        inputIndex);
            else
                setInputs(cachedInputs);
            configureSoundTrack();
            graph()->endGraphEdit();
        }
    }

    void SequenceGroupIPNode::configureSoundTrack()
    {
        const string filename =
            propertyValue<StringProperty>(m_soundTrackFile, "");
        const float offset =
            propertyValue<FloatProperty>(m_soundTrackOffset, 0.0f);

        if (filename == "")
        {
            if (m_audioAddNode)
            {
                m_sequenceNode->disconnectOutputs();

                setRoot(m_sequenceNode);
                delete m_audioAddNode;

                m_audioAddNode = 0;
                m_soundTrackNode = 0;
                m_soundTrackRetime = 0;
            }
        }
        else
        {
            if (!m_audioAddNode)
            {
                m_audioAddNode = newMemberNodeOfType<AudioAddIPNode>(
                    audioAddType(), "audioAdd");
                m_soundTrackRetime = newMemberNodeOfType<RetimeIPNode>(
                    retimeType(), "soundtrackRetime");
                m_soundTrackNode = newMemberNodeOfType<FileSourceIPNode>(
                    audioSourceType(), "soundtrackSource");

                m_soundTrackNode->setProgressiveSourceLoading(true);
                m_soundTrackRetime->setInputs1(m_soundTrackNode);
                m_audioAddNode->setInputs2(m_sequenceNode, m_soundTrackRetime);

                setRoot(m_audioAddNode);
            }

            if (m_soundTrackNode->propertyValue<StringProperty>("media.movie",
                                                                "")
                != filename)
            {
                m_soundTrackNode->setProperty<FloatProperty>(
                    "group.audioOffset", offset);
                float fps = m_useMaxFPS ? m_maxFPS
                                        : m_sequenceNode->imageRangeInfo().fps;
                m_soundTrackRetime->setProperty<FloatProperty>("output.fps",
                                                               fps);

                PropertyEditor<StringProperty> edit(m_soundTrackNode,
                                                    "media.movie");
                edit.setValue(filename);
            }
        }
    }

    void SequenceGroupIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_retimeToOutput
                || p == m_sequenceNode->outputFPSProperty())
            {
                rebuild();
                propagateRangeChange();
                if (p == m_sequenceNode->outputFPSProperty())
                    return;
            }
            else if (p == m_sequenceNode->outputSizeProperty()
                     || p == m_sequenceNode->autoSizeProperty())
            {
                return;
            }
            else if (p == m_soundTrackFile)
            {
                rebuild();
            }
            else if (p == m_soundTrackOffset)
            {
            }
        }

        GroupIPNode::propertyChanged(p);
    }

    namespace
    {
        typedef SequenceIPNode::EvalPoint EvalPoint;
    }

    SequenceGroupIPNode::EDLInfo SequenceGroupIPNode::edlInfo() const
    {
        const ImageRangeInfo range = imageRangeInfo();
        EDLInfo edl;

        //
        //  Loop over the frame range and sample
        //

        EvalPoint p0(-1, 0, -1);

        for (int frame = range.start; frame <= range.end; frame++)
        {
            EvalPoint p = m_sequenceNode->evaluationPoint(frame);

            if (p0.sourceIndex != p.sourceIndex)
            {
                EDLClipInfo clipinfo;

                clipinfo.index = p.sourceIndex;
                clipinfo.node = inputs()[p.sourceIndex];
                clipinfo.in = frame;
                clipinfo.inputCutIn = p.sourceFrame;
                clipinfo.out = frame;
                clipinfo.inputCutOut = p.sourceFrame;

                clipinfo.node->mediaInfo(
                    graph()->contextForFrame(clipinfo.inputCutIn),
                    clipinfo.media);

                ImageRangeInfo iinfo = clipinfo.node->imageRangeInfo();
                clipinfo.inputFPS = iinfo.fps;

                edl.push_back(clipinfo);

                p0 = p;
            }
            else
            {
                edl.back().out = frame;
                edl.back().inputCutOut = p.sourceFrame;
            }
        }

        return edl;
    }

    void SequenceGroupIPNode::inputMediaChanged(IPNode* srcNode,
                                                int srcOutIndex,
                                                PropagateTarget target)
    {
        IPNode::inputMediaChanged(srcNode, srcOutIndex, target);

        auto inputIndex = mapToInputIndex(srcNode, srcOutIndex);

        if (inputIndex < 0)
        {
            return;
        }

        auto input = m_sequenceNode->inputs()[inputIndex];

        if (input->protocol() == "RVRetime")
        {
            if (auto retime = dynamic_cast<RetimeIPNode*>(input))
                retime->resetFPS();
        }

        rebuild(inputIndex);
    }

} // namespace IPCore
