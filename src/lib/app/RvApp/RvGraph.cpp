//
//  Copyright (c) 2012 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <RvApp/Options.h>
#include <RvApp/RvGraph.h>
#include <RvApp/RvNodeDefinitions.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPBaseNodes/LayoutGroupIPNode.h>
#include <IPBaseNodes/SequenceGroupIPNode.h>
#include <IPBaseNodes/SourceGroupIPNode.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/SwitchIPNode.h>
#include <IPBaseNodes/SwitchGroupIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/AudioTextureIPNode.h>
#include <IPCore/CacheIPNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/NodeManager.h>
#include <IPCore/OutputGroupIPNode.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/RootIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Transform2DIPNode.h>
#include <IPCore/ViewGroupIPNode.h>
#include <TwkApp/Event.h>
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <TwkAudio/Filters.h>
#include <TwkAudio/Mix.h>
#include <TwkMath/Function.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/sgcHop.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include <algorithm> 
#include <cmath>

#include <QtCore/QObject>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace Rv {
using namespace IPCore;
using namespace std;
using namespace TwkMath;
using namespace TwkUtil;
using namespace TwkContainer;
using namespace TwkAudio;
using namespace boost;

#ifdef PLATFORM_WINDOWS
#define PAD "%06d"
#else
#define PAD "%06zd"
#endif

// Note that the new fast add source mechanism postpones the default views
// input connection until all the new sources have been added.
// Note that this mechanism was put in place so that we end up with an O(n)
// complexity instead of O(n^2) with respect to the number of added sources
// This mechanism is enabled by default. 
// However the following environment variable can be used if one suspects
// that an issue originates from this specific mechanism.
static ENVVAR_BOOL( evUseFastAddSource, "RV_USE_FAST_ADD_SOURCE", true );

// Defines the default value of the autoEDL mode for the RVSwitchGroup node
static ENVVAR_INT( evMediaRepSwitchAutoEDL, "RV_MEDIA_REP_SWITCH_AUTO_EDL", 0 );

//----------------------------------------------------------------------

RvGraph::RvGraph(const NodeManager* nodeManager)
    : IPGraph(nodeManager),
      m_sequenceNode(0),
      m_stackNode(0),
      m_layoutNode(0)
{
}

RvGraph::~RvGraph()
{
}

void
RvGraph::removeNode(IPNode* n)
{
    IPGraph::removeNode(n);

    if (n == m_sequenceNode) m_sequenceNode = 0;
    if (n == m_stackNode) m_stackNode = 0;
    if (n == m_layoutNode) m_layoutNode = 0;

    Sources::iterator s = find(m_imageSources.begin(), m_imageSources.end(), n);
    if (s != m_imageSources.end()) m_imageSources.erase(s);
}

void
RvGraph::initializeIPTree(const VideoModules& modules)
{
    lockAudioFill();
    finishAudioThread();
    unlockAudioFill();

    m_topologyChanged = true;

    if (m_cacheMode != NeverCache)
    {
        setCachingMode(NeverCache, 1, 2, 1, 2, 1, 1, 24.0);
        finishCachingThread();
    }

    //
    //  Unhook all inputs. This will prevent the need for recursive
    //  deletion -- at least at the top level.
    //

    for (NodeMap::iterator i = m_viewNodeMap.begin();
         i != m_viewNodeMap.end();
         ++i)
    {
        IPNode* n = i->second;
        n->willDelete();
        n->disconnectInputs();
    }

    //
    //  Delete all nodes until none are left, but first set the view node
    //  to the last node.  If we don't do this, once we delete the view node
    //  we will reset it to the next node and repeat that until we reach 
    //  the end. Each time we do this is will re-evaluate the graph, which
    //  is needlessly costly since we are about to delete all nodes.
    //
    if (!m_viewNodeMap.empty())
    {
       setViewNode(m_viewNodeMap.rbegin()->second);
    }
    while (!m_viewNodeMap.empty())
    {
        IPNode* n = m_viewNodeMap.begin()->second;
        delete n;
    }

    m_imageSources.clear();
    m_displayGroups.clear();

    m_rootNode             = 0;
    m_sequenceNode         = 0;
    m_stackNode            = 0;
    m_layoutNode           = 0;
    m_viewNode             = 0;

    m_nodeMap.clear();
    m_viewNodeMap.clear();
    m_defaultViewsMap.clear();

    m_rootNode           = newNode("Root", "root");
    m_sessionNode        = newNodeOfType<SessionIPNode>("RVSession", "rv");
    m_sequenceNode       = newNodeOfType<SequenceGroupIPNode>("RVSequenceGroup", "defaultSequence");
    m_stackNode          = newNodeOfType<StackGroupIPNode>("RVStackGroup", "defaultStack");
    m_layoutNode         = newNodeOfType<LayoutGroupIPNode>("RVLayoutGroup", "defaultLayout");
    m_viewGroupNode      = newNodeOfType<ViewGroupIPNode>("RVViewGroup", "viewGroup");
    m_defaultOutputGroup = newOutputGroup("defaultOutputGroup");

    m_viewNode = m_sequenceNode;

    m_sequenceNode->declareProperty<StringProperty>("ui.name", QObject::tr("Default Sequence").toStdString().c_str());
    m_stackNode->declareProperty<StringProperty>("ui.name", "Default Stack");
    m_layoutNode->declareProperty<StringProperty>("ui.name", "Default Layout");

    m_volume         = m_viewGroupNode->soundtrackNode()->property<FloatProperty>("audio.volume");
    m_balance        = m_viewGroupNode->soundtrackNode()->property<FloatProperty>("audio.balance");
    m_mute           = m_viewGroupNode->soundtrackNode()->property<IntProperty>("audio.mute");
    m_audioSoftClamp = m_viewGroupNode->soundtrackNode()->property<IntProperty>("audio.softClamp");
    m_audioOffset    = m_viewGroupNode->soundtrackNode()->property<FloatProperty>("audio.offset");
    m_audioOffset2   = m_viewGroupNode->soundtrackNode()->property<FloatProperty>("audio.internalOffset");

    m_defaultViewsMap[m_sequenceNode->name()] = m_sequenceNode;
    m_defaultViewsMap[m_stackNode->name()]    = m_stackNode;
    m_defaultViewsMap[m_layoutNode->name()]   = m_layoutNode;

    m_viewNodeMap[m_sequenceNode->name()] = m_sequenceNode;
    m_viewNodeMap[m_stackNode->name()]    = m_stackNode;
    m_viewNodeMap[m_layoutNode->name()]   = m_layoutNode;

    m_viewGroupNode->setInputs1(m_viewNode);

    m_defaultOutputGroup->setInputs1(m_viewGroupNode);
    setPhysicalDevicesInternal(modules);

    m_rootNode->appendInput(m_viewGroupNode->waveformNode());
}


DisplayGroupIPNode* 
RvGraph::newDisplayGroup(const std::string& nodeName, const TwkApp::VideoDevice* device)
{
    const Options& opts = Options::sharedOptions();

    DisplayGroupIPNode* group = newNodeOfType<DisplayGroupIPNode>("RVDisplayGroup", nodeName);

    if (group) 
    {
        group->setPhysicalVideoDevice(device);

        if (IPNode* dispNode = 
            group->displayPipelineNode()->findNodeInPipelineByTypeName("RVDisplayColor"))
        {
            dispNode->setProperty<FloatProperty>("color.gamma", opts.gamma);
            dispNode->setProperty<IntProperty>("color.sRGB", opts.sRGB);
            dispNode->setProperty<IntProperty>("color.Rec709", opts.rec709);
            dispNode->setProperty<FloatProperty>("color.brightness", opts.brightness);
        }
    }

    return group;
}

OutputGroupIPNode* 
RvGraph::newOutputGroup(const std::string& nodeName, const TwkApp::VideoDevice* device)
{
    OutputGroupIPNode* group = newNodeOfType<OutputGroupIPNode>("RVOutputGroup", nodeName);
    if (group) group->setPhysicalVideoDevice(device);
    return group;
}

TwkContainer::PropertyContainer*
RvGraph::sparseContainer(IPNode* node)
{
    if (dynamic_cast<ImageSourceIPNode*>(node))
    {
        return node->shallowCopy();
    }
    return IPGraph::sparseContainer(node);
}

SourceIPNode*
RvGraph::addSource(
    const std::string& nodeType, 
    const std::string& nodeName, 
    const std::string& mediaRepName, 
    IPCore::SourceIPNode* mediaRepSisterSrcNode
)
{
    // Alternating between 2 colors makes it easier to see each individual
    // added source.
    static bool zoneFlip = false;
    zoneFlip = !zoneFlip;
    HOP_ZONE( zoneFlip ? HOP_ZONE_COLOR_10 :HOP_ZONE_COLOR_11 );
    HOP_PROF_DYN_NAME( 
        std::string(std::string("RvGraph::addSource(nodeType=") + nodeType + 
                    std::string(") ") + nodeName).c_str());

    //
    //  Build the IP tree for this source. Make sure that the
    //  caching/evaluation thread is suspended before doing
    //  anything.
    //

    string finalName = nodeName;

    if (finalName.empty())
    {
        char temp[256];
        size_t n = m_imageSources.size();
        sprintf(temp, "sourceGroup" PAD "_source", n);

        finalName = temp;
    }

    if (finalName.find("_source") == string::npos) 
    {
        cerr << "ERROR: malformed source name: " << finalName << endl;
    }

    SourceIPNode* source = newSource(finalName, nodeType, mediaRepName);
    setupSource(source, mediaRepSisterSrcNode);

    return source;
}

// Fast add source mechanism which postpones the default views' inputs 
// connection until all the new sources have been added.
// Note that this mechanism was put in place so that we end up with an O(n)
// complexity instead of O(n^2) with respect to the number of added sources
// This mechanism is enabled by default but can be disabled with an 
// environment variable for emergency/debugging purpose. 
void
RvGraph::connectNewSourcesToDefaultViews()
{
    HOP_PROF_FUNC();

    if ( !evUseFastAddSource.getValue() || m_newSources.empty() )
    {
        return;
    }

    for (NodeMap::iterator i = m_defaultViewsMap.begin();
         i != m_defaultViewsMap.end();
         ++i)
    {
        HOP_PROF_DYN_NAME( 
            std::string(
                std::string("RvGraph::updateDefaultViewsWithNewSources() - setInputs - ") + 
                i->first ).c_str() );

        IPNode* layer = i->second;
        IPNode::IPNodes inputs(layer->inputs().size() + m_newSources.size());
        copy(layer->inputs().begin(), layer->inputs().end(), inputs.begin());
        copy(m_newSources.begin(), m_newSources.end(), inputs.begin()+layer->inputs().size());
        HOP_ZONE( HOP_ZONE_COLOR_12 );
        layer->setInputs(inputs);
    }

    m_newSources.clear();
}

void
RvGraph::setupSource(SourceIPNode* source, IPCore::SourceIPNode* mediaRepSisterSrcNode)
{
    HOP_PROF_FUNC();

    string name = source->name();
    name = name.substr(0, name.find("_source"));

    m_imageSources.push_back(source);

    SourceGroupIPNode* sourceGroup = newSourceGroup(name, source);

    m_viewNodeMap[sourceGroup->name()] = sourceGroup;

    // Are we adding a media representation ?
    SwitchGroupIPNode* switchGroup = nullptr;
    if (mediaRepSisterSrcNode || !source->mediaRepName().empty())
    {
        // Make sure that the media is not loaded by default when adding a media
        // representation as we only want to load it when the media representation
        // is actually used
        if (mediaRepSisterSrcNode)
        {
            source->setMediaActive(false);

            // Is there an already existing switch group already connected to the sister source node ?
            switchGroup = dynamic_cast<SwitchGroupIPNode*>(
                findNodeAssociatedWith(mediaRepSisterSrcNode->group(), "RVSwitchGroup"));
        }
    
        // If there is no existing switch group already then allocate one
        if (!switchGroup)
        {
            const NodeDefinition* def = m_nodeManager->definition("RVSwitchGroup");
            if (!def)
            {
                TWK_THROW_EXC_STREAM("Could not find RVSwitchGroup node definition");
            }

            switchGroup = new SwitchGroupIPNode(name+"_switchGroup", def, this);
            switchGroup->switchNode()->setProperty<IntProperty>(
                "mode.autoEDL", evMediaRepSwitchAutoEDL.getValue() );

            if (mediaRepSisterSrcNode)
            {
                switchGroup->setInputs2(mediaRepSisterSrcNode->group(), sourceGroup);
            }
            else
            {
                switchGroup->setInputs1(sourceGroup);
            }

            m_viewNodeMap[switchGroup->name()] = switchGroup;

            // Adjust the view nodes accordingly
            if (evUseFastAddSource.getValue() && isFastAddSourceEnabled())
            {
                // Fast add source mechanism which postpones the default views' inputs 
                // connection until all the new sources have been added to prevent O(n^2)
                if (mediaRepSisterSrcNode)
                {
                    std::replace(m_newSources.begin(), m_newSources.end(), static_cast<IPNode*>(mediaRepSisterSrcNode->group()), static_cast<IPNode*>(switchGroup));
                }
                else
                {
                    m_newSources.push_back(static_cast<IPNode*>(switchGroup));
                }
            }
            else
            {
                //
                //  Connect all of the default view nodes.
                //

                IPNode::IPNodes inputs(1);
                for (NodeMap::iterator i = m_defaultViewsMap.begin();
                    i != m_defaultViewsMap.end();
                    ++i)
                {
                    HOP_PROF_DYN_NAME( 
                        std::string(
                            std::string("RvGraph::setupSource - setInputs - ") + 
                            i->first ).c_str() );

                    IPNode* layer = i->second;
                    if (mediaRepSisterSrcNode)
                    {
                        inputs.resize(layer->inputs().size());
                        for (size_t i = 0; i < inputs.size(); i++)
                        {
                            inputs[i] = (layer->inputs()[i] == mediaRepSisterSrcNode->group()) ? switchGroup : layer->inputs()[i]; 
                        }
                    }
                    else
                    {
                        inputs.resize(layer->inputs().size() + 1);
                        copy(layer->inputs().begin(), layer->inputs().end(), inputs.begin());
                        inputs.back() = switchGroup;
                    }
                    HOP_ZONE( HOP_ZONE_COLOR_12 );
                    layer->setInputs(inputs);
                }
            }

            m_topologyChanged = true;
        }
        else
        {
            switchGroup->appendInput(sourceGroup);
        }
        
        return;
    }

    if (evUseFastAddSource.getValue() && isFastAddSourceEnabled())
    {
        // Fast add source mechanism which postpones the default views' inputs 
        // connection until all the new sources have been added to prevent O(n^2)
        m_newSources.push_back(sourceGroup);
    }
    else
    {
        //
        //  Connect all of the default view nodes.
        //

        IPNode::IPNodes inputs(1);
        for (NodeMap::iterator i = m_defaultViewsMap.begin();
            i != m_defaultViewsMap.end();
            ++i)
        {
            HOP_PROF_DYN_NAME( 
                std::string(
                    std::string("RvGraph::setupSource - setInputs - ") + 
                    i->first ).c_str() );

            IPNode* layer = i->second;
            inputs.resize(layer->inputs().size() + 1);
            copy(layer->inputs().begin(), layer->inputs().end(), inputs.begin());
            inputs.back() = sourceGroup;
            HOP_ZONE( HOP_ZONE_COLOR_12 );
            layer->setInputs(inputs);
        }
    }

    m_frameCacheInvalid = true;
    m_topologyChanged = true;
}

SourceIPNode* 
RvGraph::newSource(const std::string& nodeName,
                   const std::string& nodeType,
                   const std::string& mediaRepName)
{
    SourceIPNode* source = 0;
    if (const NodeDefinition* def = m_nodeManager->definition(nodeType))
    {
        if (nodeType == "RVImageSource")
        {
            ImageSourceIPNode* sipn = new ImageSourceIPNode(nodeName, def, this, NULL, mediaRepName);
            source = dynamic_cast<SourceIPNode*>(sipn);
        }
        else if (nodeType == "RVFileSource")
        {
            FileSourceIPNode* sipn = new FileSourceIPNode(nodeName, def, this, NULL, mediaRepName);
            source = dynamic_cast<SourceIPNode*>(sipn);
        }
    }

    return source;
}

SourceGroupIPNode*
RvGraph::newSourceGroup(const std::string& nodeName, SourceIPNode* snode)
{
    if (const NodeDefinition* def = m_nodeManager->definition("RVSourceGroup"))
    {
        return new SourceGroupIPNode(nodeName, def, snode, this);
    }
    else
    {
        return 0;
    }
}

} // Rv
