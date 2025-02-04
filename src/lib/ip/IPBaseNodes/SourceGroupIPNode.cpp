//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/CacheLUTIPNode.h>
#include <IPBaseNodes/ChannelMapIPNode.h>
#include <IPBaseNodes/ColorIPNode.h>
#include <IPBaseNodes/CropIPNode.h>
#include <IPBaseNodes/RotateCanvasIPNode.h>
#include <IPBaseNodes/LookIPNode.h>
#include <IPBaseNodes/OverlayIPNode.h>
#include <IPBaseNodes/PaintIPNode.h>
#include <IPBaseNodes/SourceGroupIPNode.h>
#include <IPBaseNodes/SourceStereoIPNode.h>
#include <IPCore/CacheIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/Transform2DIPNode.h>
#include <TwkUtil/sgcHop.h>
#include <TwkContainer/Properties.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/filesystem.hpp>

namespace IPCore
{
    using namespace boost;
    using namespace std;
    using namespace TwkContainer;

    typedef set<string> NameSet;

    static string uniqueName(string name, NameSet& disallowedNames)
    {
        static char buf[24];
        static regex endRE("(.*) \\(([0-9]+)\\)");

        if (disallowedNames.count(name) > 0)
        {
            smatch endMatch;

            if (regex_search(name, endMatch, endRE))
            {
                string finalName;
                istringstream istr(endMatch[1]);
                int n;
                istr >> n;
                do
                {
                    ++n;
                    sprintf(buf, " (%d)", n);
                    ostringstream str;
                    str << endMatch[0] << buf;
                    finalName = str.str();
                } while (disallowedNames.count(finalName) > 0);

                return finalName;
            }

            //
            //  Fallback: add a number to the end and recurse
            //
            ostringstream str;
            str << name << " (2)";
            return uniqueName(str.str(), disallowedNames);
        }

        return name;
    }

    SourceGroupIPNode::SourceGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
        , m_sourceNode(0)
        , m_beforeColorNode(0)
        , m_afterColorNode(0)
        , m_beforeLinearizeNode(0)
        , m_afterLinearizeNode(0)
    {
        init(name, def, 0, graph, group);
    }

    SourceGroupIPNode::SourceGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         SourceIPNode* sourceNode,
                                         IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
        , m_sourceNode(sourceNode)
        , m_beforeColorNode(0)
        , m_afterColorNode(0)
        , m_beforeLinearizeNode(0)
        , m_afterLinearizeNode(0)
    {
        init(name, def, sourceNode, graph, group);
    }

    void SourceGroupIPNode::setUINameFromMedia(int index)
    {
        if (!m_sourceNode)
            return;
        StringProperty* sp = declareProperty<StringProperty>("ui.name", "");

        //
        //  Find suitable unique UI name
        //

        static regex movRE("(^.*)\\.mov$");
        static regex seqRE(
            "(^.*)\\.-?[0-9]*-?-?[0-9]*[@#%]*[0-9]*d?\\.[a-zA-Z0-9]+$");
        static regex extRE("(^.*)\\.[a-zA-Z0-9]+$");

        string uiname =
            (m_sourceNode && m_sourceNode->numMedia() > 0)
                ? boost::filesystem::basename(m_sourceNode->mediaName(index))
                : "";
        smatch movMatch;

        if (regex_search(uiname, movMatch, movRE))
        {
            uiname = movMatch[0];
        }
        else
        {
            smatch seqMatch;

            if (regex_search(uiname, seqMatch, seqRE))
            {
                uiname = seqMatch[0];
            }
            else
            {
                smatch extMatch;
                if (regex_search(uiname, extMatch, extRE))
                {
                    uiname = extMatch[0];
                }
            }
        }

        // Append the media representation's name if any
        if (!m_sourceNode->mediaRepName().empty())
        {
            uiname += " (" + m_sourceNode->mediaRepName() + ")";
        }

        NameSet existingNames;

        for (IPGraph::NodeMap::const_iterator i =
                 graph()->viewableNodes().begin();
             i != graph()->viewableNodes().end(); ++i)
        {
            existingNames.insert((*i).second->uiName());
        }

        sp->front() = uniqueName(uiname, existingNames);
    }

    void SourceGroupIPNode::init(const std::string& name,
                                 const NodeDefinition* def,
                                 SourceIPNode* sourceNode, IPGraph* graph,
                                 GroupIPNode* group)
    {
        HOP_PROF_DYN_NAME(
            std::string(std::string("SourceGroupIPNode::init : ") + name)
                .c_str());

        setMaxInputs(0);
        setMinInputs(0);

        const string& sname = sourceNode ? sourceNode->name() : "";
        size_t n = atoi(sname.c_str() + sname.size() - 6);

        if (sourceNode)
        {
            sourceNode->setGroup(this);
        }

        string cacheLUTType =
            def->stringValue("defaults.preCacheLUTType", "CacheLUT");
        string formatType = def->stringValue("defaults.formatType", "Format");
        string channelMapType =
            def->stringValue("defaults.channelMapType", "ChannelMap");
        string cacheType = def->stringValue("defaults.cacheType", "Cache");
        string paintType = def->stringValue("defaults.paintType", "Paint");
        string overlayType =
            def->stringValue("defaults.overlayType", "Overlay");
        string colorPipelineType =
            def->stringValue("defaults.colorPipelineType", "PipelineGroup");
        string linPipelineType =
            def->stringValue("defaults.linearizePipelineType", "PipelineGroup");
        string lookPipelineType =
            def->stringValue("defaults.lookPipelineType", "PipelineGroup");
        string stereoType =
            def->stringValue("defaults.stereoType", "SourceStereo");
        string rotateType =
            def->stringValue("defaults.rotateType", "RotateCanvas");
        string transformType =
            def->stringValue("defaults.transformType", "Transform2D");
        string cropType = def->stringValue("defaults.cropType", "Crop");
        string sourceType =
            def->stringValue("defaults.sourceType", "FileSource");

        CacheLUTIPNode* cacheLUT =
            newMemberNodeOfType<CacheLUTIPNode>(cacheLUTType, "cacheLUT");
        IPNode* format = 0;
        ChannelMapIPNode* chmap =
            newMemberNodeOfType<ChannelMapIPNode>(channelMapType, "channelMap");
        CacheIPNode* cache =
            newMemberNodeOfType<CacheIPNode>(cacheType, "cache");
        PipelineGroupIPNode* tolinGroup =
            newMemberNodeOfType<PipelineGroupIPNode>(linPipelineType,
                                                     "tolinPipeline");
        PaintIPNode* paint =
            newMemberNodeOfType<PaintIPNode>(paintType, "paint");
        OverlayIPNode* overlay =
            newMemberNodeOfType<OverlayIPNode>(overlayType, "overlay");
        PipelineGroupIPNode* colorGroup =
            newMemberNodeOfType<PipelineGroupIPNode>(colorPipelineType,
                                                     "colorPipeline");
        PipelineGroupIPNode* lookGroup =
            newMemberNodeOfType<PipelineGroupIPNode>(lookPipelineType,
                                                     "lookPipeline");
        SourceStereoIPNode* stereo =
            newMemberNodeOfType<SourceStereoIPNode>(stereoType, "sourceStereo");
        RotateCanvasIPNode* rotate = 0;
        Transform2DIPNode* tform = newMemberNodeOfType<Transform2DIPNode>(
            transformType, "transform2D");
        CropIPNode* crop = 0;
        if (!sourceNode)
            sourceNode = newMemberNodeOfType<SourceIPNode>(
                sourceType, sourceType == "FileSource" ? "file" : "image");

        if (formatType != "")
            format = newMemberNode(formatType, "format");
        if (cropType != "")
            crop = newMemberNodeOfType<CropIPNode>(cropType, "crop");
        if (rotateType != "")
            rotate =
                newMemberNodeOfType<RotateCanvasIPNode>(rotateType, "rotate");

        cache->setSourceNode(sourceNode);
        tform->setAdaptiveResampling(false);

        //
        //  NOTE: the evaluation order is encoded here
        //

        if (sourceNode)
            cacheLUT->setInputs1(sourceNode);

        if (format)
        {
            format->setInputs1(cacheLUT);
            chmap->setInputs1(format);
        }
        else
        {
            chmap->setInputs1(cacheLUT);
        }

        cache->setInputs1(chmap);
        tolinGroup->setInputs1(cache);
        if (rotate)
        {
            rotate->setInputs1(tolinGroup);
            overlay->setInputs1(rotate);
        }
        else
            overlay->setInputs1(tolinGroup);
        paint->setInputs1(overlay);
        colorGroup->setInputs1(paint);
        lookGroup->setInputs1(colorGroup);
        stereo->setInputs1(lookGroup);
        tform->setInputs1(stereo);
        if (crop)
            crop->setInputs1(tform);

        m_sourceNode = sourceNode;
        m_beforeColorNode = colorGroup->inputs()[0];
        m_afterColorNode = colorGroup;
        m_beforeLinearizeNode = tolinGroup->inputs()[0];
        m_afterLinearizeNode = tolinGroup;
        m_linearizePipeline = tolinGroup;
        m_colorPipeline = colorGroup;
        m_lookPipeline = lookGroup;

        setRoot(crop ? (IPNode*)crop : (IPNode*)tform);

        m_markersIn = declareProperty<IntProperty>("markers.in");
        m_markersOut = declareProperty<IntProperty>("markers.out");
        m_markersColor = declareProperty<Vec4fProperty>("markers.color");
        m_markersName = declareProperty<StringProperty>("markers.name");
    }

    SourceGroupIPNode::~SourceGroupIPNode()
    {
        // base class will delete m_root
    }

    void SourceGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        //
        //  We shouldn't have any inputs since SourceGroupIPNode is a leaf
        //

        if (newInputs.size())
        {
            cout << "WARNING: SourceGroupIPNode: setInputs called with other "
                    "than 0 nodes"
                 << endl;
        }

        IPNode::setInputs(newInputs);
    }

    void SourceGroupIPNode::readCompleted(const std::string& type,
                                          unsigned int version)
    {
    }

} // namespace IPCore
