//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <RvApp/Options.h>
#include <RvApp/RvNodeDefinitions.h>
#include <RvApp/FormatIPNode.h>
#include <RvApp/FileSpaceLinearizeIPNode.h>
#include <RvApp/RvViewGroupIPNode.h>
#include <IPBaseNodes/CacheLUTIPNode.h>
#include <IPBaseNodes/ChannelMapIPNode.h>
#include <IPBaseNodes/ColorIPNode.h>
#include <IPBaseNodes/ColorCDLIPNode.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/FolderGroupIPNode.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPBaseNodes/LayoutGroupIPNode.h>
#include <IPBaseNodes/LensWarpIPNode.h>
#include <IPBaseNodes/PrimaryConvertIPNode.h>
#include <IPBaseNodes/OverlayIPNode.h>
#include <IPBaseNodes/PaintIPNode.h>
#include <IPBaseNodes/RetimeGroupIPNode.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPBaseNodes/SequenceGroupIPNode.h>
#include <IPBaseNodes/SequenceIPNode.h>
#include <IPBaseNodes/SourceGroupIPNode.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPBaseNodes/SourceStereoIPNode.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/StackIPNode.h>
#include <IPBaseNodes/SwitchGroupIPNode.h>
#include <IPBaseNodes/SwitchIPNode.h>
#include <IPBaseNodes/UnsharpMaskIPNode.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/AudioTextureIPNode.h>
#include <IPCore/CacheIPNode.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/DisplayIPNode.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/DynamicIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/LUTIPNode.h>
#include <IPCore/RootIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/NodeManager.h>
#include <OCIONodes/OCIOIPNode.h>
#include <IPCore/OutputGroupIPNode.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Transform2DIPNode.h>
#include <ICCNodes/ICCDefinitions.h>

namespace Rv {
using namespace std;
using namespace IPCore;

namespace {
typedef TwkContainer::StringProperty StringProperty;
typedef IPGraph::FloatProperty FloatProperty;
typedef IPGraph::IntProperty IntProperty;

int compiledPreLUTSize = 0;
int getCompiledPreLUTSize()
{
    if (compiledPreLUTSize == 0)
    {
        compiledPreLUTSize = 2048;
        if (const char* var = getenv("RV_COMPILED_PRELUT_SIZE"))
        {
            compiledPreLUTSize = atoi(var);
            cerr << "INFO: custom preLUT size: " << compiledPreLUTSize << endl;
        }
    }
    return compiledPreLUTSize;
}

} // close empty namespace

void addRvNodeDefinitions(NodeManager* m)
{
    NodeDefinition::ByteVector emptyIcon;


    addICCNodeDefinitions(m);
    bool usingICC = false;
    char* ignoreICC = getenv("RV_IGNORE_ICC_PROFILE");
    if (ignoreICC) usingICC = (strcmp(ignoreICC,"0") == 0);

    m->addDefinition(new NodeDefinition("Adaptor", 1, false,
                                        "adaptor",
                                        newIPNode<AdaptorIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("Root", 1, false,
                                        "root",
                                        newIPNode<RootIPNode>,
                                        "", "", emptyIcon,
                                        false));


    // RvSession protocolVersion is overwritten in the session write request
    m->addDefinition(new NodeDefinition("RVSession", 4, false,
                                        "session",
                                        newIPNode<SessionIPNode>,
                                        "Session container",
                                        "",
                                        emptyIcon,
                                        false));

    {
        NodeDefinition* def = new NodeDefinition("RVSequenceGroup", 1, true,
                                                 "sequenceGroup",
                                                 newIPNode<SequenceGroupIPNode>,
                                                 "Plays back inputs by EDL",
                                                 "",
                                                 emptyIcon);

        def->declareProperty<StringProperty>("defaults.sequenceType", "RVSequence");
        def->declareProperty<StringProperty>("defaults.retimeType", "RVRetime");
        def->declareProperty<StringProperty>("defaults.paintType", "RVPaint");
        m->addDefinition(def);
    }


    {
        NodeDefinition* def = new NodeDefinition("RVStackGroup", 1, true,
                                                 "stackGroup",
                                                 newIPNode<StackGroupIPNode>,
                                                 "Stacks inputs in time",
                                                 "",
                                                 emptyIcon);

        def->declareProperty<StringProperty>("defaults.stackType", "RVStack");
        def->declareProperty<StringProperty>("defaults.transformType", "RVTransform2D");
        def->declareProperty<StringProperty>("defaults.retimeType", "RVRetime");
        def->declareProperty<StringProperty>("defaults.paintType", "RVPaint");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVFolderGroup", 1, true,
                                                 "folderGroup",
                                                 newIPNode<FolderGroupIPNode>,
                                                 "Multipurpose collection of nodes",
                                                 "",
                                                 emptyIcon);

        def->declareProperty<StringProperty>("folder.switch", "RVSwitch");
        def->declareProperty<StringProperty>("folder.stack", "RVStackGroup");
        def->declareProperty<StringProperty>("folder.layout", "RVLayoutGroup");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVLayoutGroup", 1, true,
                                                 "layoutGroup",
                                                 newIPNode<LayoutGroupIPNode>,
                                                 "Visaully layout inputs in various ways",
                                                 "",
                                                 emptyIcon);

        def->declareProperty<StringProperty>("defaults.stackType", "RVStack");
        def->declareProperty<StringProperty>("defaults.transformType", "RVTransform2D");
        def->declareProperty<StringProperty>("defaults.retimeType", "RVRetime");
        def->declareProperty<StringProperty>("defaults.paintType", "RVPaint");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVRetimeGroup", 1, true,
                                                 "retimeGroup",
                                                 newIPNode<RetimeGroupIPNode>,
                                                 "Change input FPS, duration, or offset",
                                                 "",
                                                 emptyIcon);

        def->declareProperty<StringProperty>("defaults.retimeType", "RVRetime");
        def->declareProperty<StringProperty>("defaults.paintType", "RVPaint");
        m->addDefinition(def);
    }


    {
        NodeDefinition* def = new NodeDefinition("RVSwitchGroup", 1, true,
                                                 "switchGroup",
                                                 newIPNode<SwitchGroupIPNode>,
                                                 "Select one of several inputs",
                                                 "",
                                                 emptyIcon);

        def->declareProperty<StringProperty>("defaults.switchType", "RVSwitch");
        m->addDefinition(def);
    }

    m->addDefinition(new NodeDefinition("RVRetime", 1, false,
                                        "retime",
                                        newIPNode<RetimeIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVSwitch", 1, false,
                                        "switch",
                                        newIPNode<SwitchIPNode>,
                                        "Select one of several inputs",
                                        "",
                                        emptyIcon,
                                        false));

    {
        NodeDefinition* def = new NodeDefinition("RVColorPipelineGroup", 1, true,
                                                 "pipeline",
                                                 newIPNode<PipelineGroupIPNode>,
                                                 "Managed Single Input Pipeline for Source Color",
                                                 "",
                                                 emptyIcon,
                                                 false);

        def->declareProperty<StringProperty>("defaults.pipeline", "RVColor");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVLookPipelineGroup", 1, true,
                                                 "pipeline",
                                                 newIPNode<PipelineGroupIPNode>,
                                                 "Managed Single Input Pipeline for Source Look",
                                                 "",
                                                 emptyIcon,
                                                 false);

        def->declareProperty<StringProperty>("defaults.pipeline", "RVLookLUT");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVLinearizePipelineGroup", 1, true,
                                                 "pipeline",
                                                 newIPNode<PipelineGroupIPNode>,
                                                 "Managed Single Input Pipeline for Source Linearization",
                                                 "",
                                                 emptyIcon,
                                                 false);

        vector<string> linearizeDefaults;
        linearizeDefaults.push_back("RVLinearize");
        if (usingICC) linearizeDefaults.push_back("ICCLinearizeTransform");
        linearizeDefaults.push_back("RVLensWarp");
        def->declareProperty<StringProperty>("defaults.pipeline", linearizeDefaults);
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVDisplayPipelineGroup", 1, true,
                                                 "pipeline",
                                                 newIPNode<PipelineGroupIPNode>,
                                                 "Managed Single Input Pipeline for Display",
                                                 "",
                                                 emptyIcon,
                                                 false);

        vector<string> displayDefaults;
        if (usingICC) displayDefaults.push_back("ICCDisplayTransform");
        displayDefaults.push_back("RVDisplayColor");
        def->declareProperty<StringProperty>("defaults.pipeline", displayDefaults);
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVViewPipelineGroup", 1, true,
                                                 "pipeline",
                                                 newIPNode<PipelineGroupIPNode>,
                                                 "Managed Single Input Pipeline for View Group",
                                                 "",
                                                 emptyIcon,
                                                 false);

        //
        //  View pipeline is empty by default.
        //
        def->declareProperty<StringProperty>("defaults.pipeline");
        m->addDefinition(def);
    }

    m->addDefinition(new NodeDefinition("PipelineGroup", 1, true,
                                        "pipeline",
                                        newIPNode<PipelineGroupIPNode>,
                                        "Managed Single Input Pipeline",
                                        "",
                                        emptyIcon,
                                        true));

    m->addDefinition(new NodeDefinition("RVCache", 1, false,
                                        "cache",
                                        newIPNode<CacheIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVCacheLUT", 1, false,
                                        "cacheLUT",
                                        newIPNode<CacheLUTIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVChannelMap", 1, false,
                                        "channelMap",
                                        newIPNode<ChannelMapIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVColor", 2, false,
                                        "color",
                                        newIPNode<ColorIPNode>,
                                        "", "", emptyIcon,
                                        true));

    m->addDefinition(new NodeDefinition("RVCDL", 1, false,
                                        "color",
                                        newIPNode<ColorCDLIPNode>,
                                        "", "", emptyIcon,
                                        true));

    m->addDefinition(new NodeDefinition("RVDispTransform2D", 1, false,
                                        "dispTransform2D",
                                        newIPNode<DispTransform2DIPNode>,
                                        "", "", emptyIcon,
                                        false));

    {
        NodeDefinition* def = new NodeDefinition("RVDisplayColor", 1, false,
                                                 "displayColor",
                                                 newIPNode<DisplayIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        def->declareProperty<IntProperty>("defaults.preLUTSize", getCompiledPreLUTSize());
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVDisplayGroup", 1, true,
                                                 "displayGroup",
                                                 newIPNode<DisplayGroupIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        def->declareProperty<StringProperty>("defaults.stereoType", "RVDisplayStereo");
        def->declareProperty<StringProperty>("defaults.pipelineType", "RVDisplayPipelineGroup");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVOutputGroup", 1, true,
                                                 "outputGroup",
                                                 newIPNode<OutputGroupIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        def->declareProperty<StringProperty>("defaults.stereoType", "RVDisplayStereo");
        def->declareProperty<StringProperty>("defaults.pipelineType", "RVDisplayPipelineGroup");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVViewGroup", 1, true,
                                                 "viewGroup",
                                                 newIPNode<RvViewGroupIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        def->declareProperty<StringProperty>("defaults.soundtrackType",    "RVSoundTrack");
        def->declareProperty<StringProperty>("defaults.dispTransformType", "RVDispTransform2D");
        def->declareProperty<StringProperty>("defaults.waveformType",      "AudioWaveform");
        def->declareProperty<StringProperty>("defaults.pipelineType",      "RVViewPipelineGroup");
        m->addDefinition(def);
    }


    m->addDefinition(new NodeDefinition("RVDisplayStereo", 1, false,
                                        "displayStereo",
                                        newIPNode<DisplayStereoIPNode>,
                                        "", "", emptyIcon,
                                        false));

    {
        NodeDefinition* def = new NodeDefinition("RVFileSource", 1, false,
                                                 "fileSource",
                                                 newIPNode<FileSourceIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        Rv::Options& opts = Rv::Options::sharedOptions();
        def->declareProperty<IntProperty>("defaults.progressiveSourceLoading", opts.progressiveSourceLoading);
        def->declareProperty<StringProperty>("defaults.missingMovieProc", "error");
        m->addDefinition(def);
    }

    m->addDefinition(new NodeDefinition("RVImageSource", 1, false,
                                        "imageSource",
                                        newIPNode<ImageSourceIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVFormat", 1, false,
                                        "format",
                                        newIPNode<FormatIPNode>,
                                        "", "", emptyIcon,
                                        false));

    {
        NodeDefinition* def = new NodeDefinition("RVLinearize", 1, false,
                                                 "linearize",
                                                 newIPNode<FileSpaceLinearizeIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        def->declareProperty<IntProperty>("defaults.preLUTSize", getCompiledPreLUTSize());
        m->addDefinition(def);
    }

    m->addDefinition(new NodeDefinition("RVLensWarp", 1, false,
                                        "lensWarp",
                                        newIPNode<LensWarpIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVUnsharpMask", 1, false,
                                        "unsharpMask",
                                        newIPNode<UnsharpMaskIPNode>,
                                        "", "", emptyIcon,
                                        true));

    m->addDefinition(new NodeDefinition("RVLookLUT", 1, false,
                                        "look",
                                        newIPNode<LUTIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVPrimaryConvert", 1, false,
                                        "primaryConvert",
                                        newIPNode<PrimaryConvertIPNode>,
                                        "", "", emptyIcon,
                                        true));


    {
        NodeDefinition* def = new NodeDefinition("OCIO", 1, false,
                                                 "OCIO",
                                                 newIPNode<OCIOIPNode>,
                                                 "", "", emptyIcon,
                                                 true);

        def->declareProperty<StringProperty>("defaults.function", "color");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("OCIODisplay", 1, false,
                                                 "OCIODisplay",
                                                 newIPNode<OCIOIPNode>,
                                                 "", "", emptyIcon,
                                                 true);

        def->declareProperty<StringProperty>("defaults.function", "display");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("OCIOLook", 1, false,
                                                 "OCIOLook",
                                                 newIPNode<OCIOIPNode>,
                                                 "", "", emptyIcon,
                                                 true);

        def->declareProperty<StringProperty>("defaults.function", "look");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("OCIOFile", 1, false,
                                                 "OCIOFile",
                                                 newIPNode<OCIOIPNode>,
                                                 "", "", emptyIcon,
                                                 true);

        def->declareProperty<StringProperty>("defaults.function", "color");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("SYNLinearize", 1, false,
                                                 "sclin",
                                                 newIPNode<OCIOIPNode>,
                                                 "", "", emptyIcon,
                                                 true);

        def->declareProperty<StringProperty>("defaults.function", "synlinearize");
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("SYNDisplay", 1, false,
                                                 "scdsp",
                                                 newIPNode<OCIOIPNode>,
                                                 "", "", emptyIcon,
                                                 true);

        def->declareProperty<StringProperty>("defaults.function", "syndisplay");
        m->addDefinition(def);
    }

    m->addDefinition(new NodeDefinition("RVOverlay", 1, false,
                                        "overlay",
                                        newIPNode<OverlayIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVPaint", 3, false,
                                        "paint",
                                        newIPNode<PaintIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVSequence", 1, false,
                                        "sequence",
                                        newIPNode<SequenceIPNode>,
                                        "", "", emptyIcon,
                                        false));

    {
        NodeDefinition* def = new NodeDefinition("RVSoundTrack", 1, false,
                                                 "soundTrack",
                                                 newIPNode<SoundTrackIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        // NOTE: differs from core SoundTrack
        def->declareProperty<IntProperty>("defaults.softClamp", 1);
        m->addDefinition(def);
    }

    {
        NodeDefinition* def = new NodeDefinition("RVSourceGroup", 1, true,
                                                 "sourceGroup",
                                                 newIPNode<SourceGroupIPNode>,
                                                 "", "", emptyIcon);

        def->declareProperty<StringProperty>("defaults.preCacheLUTType", "RVCacheLUT");
        def->declareProperty<StringProperty>("defaults.formatType", "RVFormat");
        def->declareProperty<StringProperty>("defaults.channelMapType", "RVChannelMap");
        def->declareProperty<StringProperty>("defaults.cacheType", "RVCache");
        def->declareProperty<StringProperty>("defaults.paintType", "RVPaint");
        def->declareProperty<StringProperty>("defaults.overlayType", "RVOverlay");
        def->declareProperty<StringProperty>("defaults.colorPipelineType", "RVColorPipelineGroup");
        def->declareProperty<StringProperty>("defaults.linearizePipelineType", "RVLinearizePipelineGroup");
        def->declareProperty<StringProperty>("defaults.lookPipelineType", "RVLookPipelineGroup");
        def->declareProperty<StringProperty>("defaults.stereoType", "RVSourceStereo");
        def->declareProperty<StringProperty>("defaults.transformType", "RVTransform2D");
        def->declareProperty<StringProperty>("defaults.cropType", "");
        m->addDefinition(def);
    }

    m->addDefinition(new NodeDefinition("RVSourceStereo", 1, false,
                                        "sourceStereo",
                                        newIPNode<SourceStereoIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVStack", 1, false,
                                        "stack",
                                        newIPNode<StackIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("RVTransform2D", 1, false,
                                        "transform2D",
                                        newIPNode<Transform2DIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("AudioWaveform", 1, false,
                                        "audioWaveform",
                                        newIPNode<AudioTextureIPNode>,
                                        "", "", emptyIcon,
                                        false));

    m->addDefinition(new NodeDefinition("Dynamic", 1, true,
                                        "dynamic",
                                        newIPNode<DynamicIPNode>,
                                        "", "", emptyIcon));

}

void
setScaleOnAll(const IPGraph& graph, float scale)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FormatIPNode* node = dynamic_cast<FormatIPNode*>((*i).second))
        {
            if (FloatProperty* fp =
                node->property<FloatProperty>("geometry", "scale"))
            {
                fp->front() = scale;
            }
        }
    }
}

void
setLogLinOnAll(const IPGraph& graph, bool val, int type)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FileSpaceLinearizeIPNode* node = dynamic_cast<FileSpaceLinearizeIPNode*>((*i).second))
        {
            node->setLogLin(val ? type : 0);
        }
    }
}

void
setSRGBLinOnAll(const IPGraph& graph, bool val)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FileSpaceLinearizeIPNode* node = dynamic_cast<FileSpaceLinearizeIPNode*>((*i).second))
        {
            node->setSRGB(1);
        }
    }
}

void
setRec709LinOnAll(const IPGraph& graph, bool val)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FileSpaceLinearizeIPNode* node = dynamic_cast<FileSpaceLinearizeIPNode*>((*i).second))
        {
            node->setRec709(1);
        }
    }
}

void
setGammaOnAll(const IPGraph& graph, float gamma)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (ColorIPNode* node = dynamic_cast<ColorIPNode*>((*i).second))
        {
            node->setGamma(gamma);
        }
    }
}

void
setFileGammaOnAll(const IPGraph& graph, float gamma)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FileSpaceLinearizeIPNode* node = dynamic_cast<FileSpaceLinearizeIPNode*>((*i).second))
        {
            node->setFileGamma(gamma);
        }
    }
}

void
setFileExposureOnAll(const IPGraph& graph, float e)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (ColorIPNode* node = dynamic_cast<ColorIPNode*>((*i).second))
        {
            node->setExposure(e);
        }
    }
}

void
setFlipFlopOnAll(const IPGraph& graph, bool setFlip, bool flip,
                 bool setFlop, bool flop)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (Transform2DIPNode* node = dynamic_cast<Transform2DIPNode*>((*i).second))
        {
            if (setFlip) node->setFlip(flip);
            if (setFlop) node->setFlop(flop);
        }
    }
}

void
setChannelMapOnAll(const IPGraph& graph, const vector<string>& chmap)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (ChannelMapIPNode* node = dynamic_cast<ChannelMapIPNode*>((*i).second))
        {
            node->setChannelMap(chmap);
        }

        if (FileSourceIPNode* node = dynamic_cast<FileSourceIPNode*>((*i).second))
        {
            node->setReadAllChannels(true);
        }
    }
}


void
fitAllInputs(const IPGraph& graph, int w, int h)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FormatIPNode* node = dynamic_cast<FormatIPNode*>((*i).second))
        {
            node->setFitResolution(w, h);
        }
    }
}

void
resizeAllInputs(const IPGraph& graph, int w, int h)
{
    const IPGraph::NodeMap& nodeMap = graph.nodeMap();

    for (IPGraph::NodeMap::const_iterator i = nodeMap.begin();
         i != nodeMap.end();
         ++i)
    {
        if (FormatIPNode* node = dynamic_cast<FormatIPNode*>((*i).second))
        {
            node->setResizeResolution(w, h);
        }
    }
}

} // Rv
