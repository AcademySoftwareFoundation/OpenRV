//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/BaseDefinitions.h>
#include <IPBaseNodes/AudioAddIPNode.h>
#include <IPBaseNodes/ColorLinearToSRGBIPNode.h>
#include <IPBaseNodes/ColorSRGBToLinearIPNode.h>
#include <IPBaseNodes/CacheLUTIPNode.h>
#include <IPBaseNodes/ChannelMapIPNode.h>
#include <IPBaseNodes/ColorIPNode.h>
#include <IPBaseNodes/CropIPNode.h>
#include <IPBaseNodes/RotateCanvasIPNode.h>
#include <IPBaseNodes/FileOutputGroupIPNode.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/FolderGroupIPNode.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPBaseNodes/LayoutGroupIPNode.h>
#include <IPBaseNodes/LensWarpIPNode.h>
#include <IPBaseNodes/PrimaryConvertIPNode.h>
#include <IPBaseNodes/YCToRGBIPNode.h>
#include <IPBaseNodes/LinearizeIPNode.h>
#include <IPBaseNodes/LookIPNode.h>
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
#include <IPCore/CoreDefinitions.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/NodeManager.h>
#include <IPBaseNodes/ColorExposureIPNode.h>
#include <IPBaseNodes/ColorCurveIPNode.h>
#include <IPBaseNodes/ColorTemperatureIPNode.h>
#include <IPBaseNodes/ColorSaturationIPNode.h>
#include <IPBaseNodes/ColorVibranceIPNode.h>
#include <IPBaseNodes/ColorShadowIPNode.h>
#include <IPBaseNodes/ColorHighlightIPNode.h>
#include <IPBaseNodes/ColorGrayScaleIPNode.h>
#include <IPBaseNodes/NoiseReductionIPNode.h>
#include <IPBaseNodes/UnsharpMaskIPNode.h>
#include <IPBaseNodes/ClarityIPNode.h>
#include <IPBaseNodes/ColorCDLIPNode.h>

namespace IPCore
{
    using namespace std;
    using namespace IPCore;

    typedef TwkContainer::StringProperty StringProperty;
    typedef TwkContainer::IntProperty IntProperty;

    void addBaseNodeDefinitions(IPCore::NodeManager* m)
    {
        NodeDefinition::ByteVector emptyIcon;

        addCoreNodeDefinitions(m);

        {
            NodeDefinition* def = new NodeDefinition(
                "FileOutputGroup", 1, true, "fileOutputGroup",
                newIPNode<FileOutputGroupIPNode>, "File Output Node", "",
                emptyIcon);

            def->declareProperty<StringProperty>("defaults.stereoType",
                                                 "DisplayStereo");
            def->declareProperty<StringProperty>("defaults.pipelineType",
                                                 "DisplayPipelineGroup");
            def->declareProperty<StringProperty>("defaults.resizeType",
                                                 "Resize");
            def->declareProperty<IntProperty>("defaults.autoResize", 1);
            def->declareProperty<StringProperty>("defaults.retimeType",
                                                 "Retime");
            m->addDefinition(def);
        }

        {
            NodeDefinition* def =
                new NodeDefinition("SequenceGroup", 1, true, "sequenceGroup",
                                   newIPNode<SequenceGroupIPNode>,
                                   "Plays back inputs by EDL", "", emptyIcon);

            def->declareProperty<StringProperty>("defaults.sequenceType",
                                                 "Sequence");
            def->declareProperty<StringProperty>("defaults.retimeType",
                                                 "Retime");
            def->declareProperty<StringProperty>("defaults.paintType", "Paint");
            def->declareProperty<StringProperty>("defaults.audioAddType",
                                                 "AudioAdd");
            def->declareProperty<StringProperty>("defaults.audioSourceType",
                                                 "FileSource");
            def->declareProperty<StringProperty>("defaults.autoFPSType", "max");
            def->declareProperty<IntProperty>("defaults.simpleSoundtrack", 1);
            m->addDefinition(def);
        }

        {
            NodeDefinition* def =
                new NodeDefinition("StackGroup", 1, true, "stackGroup",
                                   newIPNode<StackGroupIPNode>,
                                   "Stacks inputs in time", "", emptyIcon);

            def->declareProperty<StringProperty>("defaults.stackType", "Stack");
            def->declareProperty<StringProperty>("defaults.transformType",
                                                 "Transform2D");
            def->declareProperty<StringProperty>("defaults.retimeType",
                                                 "Retime");
            def->declareProperty<StringProperty>("defaults.paintType", "Paint");
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "FolderGroup", 1, true, "folderGroup",
                newIPNode<FolderGroupIPNode>,
                "Multipurpose collection of nodes", "", emptyIcon);

            def->declareProperty<StringProperty>("folder.switch", "Switch");
            def->declareProperty<StringProperty>("folder.stack", "StackGroup");
            def->declareProperty<StringProperty>("folder.layout",
                                                 "LayoutGroup");
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "LayoutGroup", 1, true, "layoutGroup",
                newIPNode<LayoutGroupIPNode>,
                "Visaully layout inputs in various ways", "", emptyIcon);

            def->declareProperty<StringProperty>("defaults.stackType", "Stack");
            def->declareProperty<StringProperty>("defaults.transformType",
                                                 "Transform2D");
            def->declareProperty<StringProperty>("defaults.retimeType",
                                                 "Retime");
            def->declareProperty<StringProperty>("defaults.paintType", "Paint");
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "RetimeGroup", 1, true, "retimeGroup",
                newIPNode<RetimeGroupIPNode>,
                "Change input FPS, duration, or offset", "", emptyIcon);

            def->declareProperty<StringProperty>("defaults.retimeType",
                                                 "Retime");
            def->declareProperty<StringProperty>("defaults.paintType", "Paint");
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "SwitchGroup", 1, true, "switchGroup",
                newIPNode<SwitchGroupIPNode>, "Select one of several inputs",
                "", emptyIcon);

            def->declareProperty<StringProperty>("defaults.switchType",
                                                 "Switch");
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition("AudioAdd", 1, false, "audioAdd",
                                            newIPNode<AudioAddIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition("Retime", 1, false, "retime",
                                            newIPNode<RetimeIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition(
            "Switch", 1, false, "switch", newIPNode<SwitchIPNode>,
            "Select one of several inputs", "", emptyIcon, false));

        {
            NodeDefinition* def = new NodeDefinition(
                "ColorPipelineGroup", 1, true, "pipeline",
                newIPNode<PipelineGroupIPNode>,
                "Managed Single Input Pipeline for Crank Color/Filter", "",
                emptyIcon, false);
            vector<string> crankDefaults;
            crankDefaults.push_back("ColorACESLogCDL");
            crankDefaults.push_back("ColorGrayScale");
            crankDefaults.push_back("ColorLinearToSRGB");
            crankDefaults.push_back("ColorTemperature");
            crankDefaults.push_back("ColorExposure");
            crankDefaults.push_back("ColorCurve");
            crankDefaults.push_back("ColorShadow");
            crankDefaults.push_back("ColorHighlight");
            crankDefaults.push_back("Clarity");
            crankDefaults.push_back("UnsharpMask");
            crankDefaults.push_back("NoiseReduction");
            crankDefaults.push_back("ColorSRGBToLinear");
            crankDefaults.push_back("ColorVibrance");

            def->declareProperty<StringProperty>("defaults.pipeline",
                                                 crankDefaults);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "LookPipelineGroup", 1, true, "pipeline",
                newIPNode<PipelineGroupIPNode>,
                "Managed Single Input Pipeline for Source Look", "", emptyIcon,
                false);

            def->declareProperty<StringProperty>("defaults.pipeline",
                                                 "LookLUT");
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "LinearizePipelineGroup", 1, true, "pipeline",
                newIPNode<PipelineGroupIPNode>,
                "Managed Single Input Pipeline for Source Linearization", "",
                emptyIcon, false);

            vector<string> linearizeDefaults(4);
            linearizeDefaults[0] = "YCToRGB";
            linearizeDefaults[1] = "FileCDL";
            linearizeDefaults[2] = "Linearize";
            linearizeDefaults[3] = "LensWarp";
            def->declareProperty<StringProperty>("defaults.pipeline",
                                                 linearizeDefaults);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "CacheLUT", 1, false, "cacheLUT", newIPNode<CacheLUTIPNode>, "",
                "", emptyIcon, false);

            def->declareProperty<IntProperty>(
                "defaults.progressiveSourceLoading", 1);
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition(
            "ChannelMap", 1, false, "channelMap", newIPNode<ChannelMapIPNode>,
            "", "", emptyIcon, false));

        m->addDefinition(new NodeDefinition("Color", 2, false, "color",
                                            newIPNode<ColorIPNode>, "", "",
                                            emptyIcon, true));

        m->addDefinition(new NodeDefinition("ColorCDL", 2, false, "CDL",
                                            newIPNode<ColorCDLIPNode>, "", "",
                                            emptyIcon, true));

        m->addDefinition(new NodeDefinition("FileCDL", 2, false, "fileCDL",
                                            newIPNode<ColorCDLIPNode>, "", "",
                                            emptyIcon, true));

        {
            NodeDefinition* def = new NodeDefinition(
                "ColorACESLogCDL", 2, false, "acesCDL",
                newIPNode<ColorCDLIPNode>, "", "", emptyIcon, true);
            def->declareProperty<StringProperty>("defaults.colorspace",
                                                 "aceslog");
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition(
            "UnsharpMask", 2, false, "unsharpMask",
            newIPNode<UnsharpMaskIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition("ColorShadow", 2, false, "shadow",
                                            newIPNode<ColorShadowIPNode>, "",
                                            "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorHighlight", 2, false, "highlight",
            newIPNode<ColorHighlightIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorSaturation", 2, false, "saturation",
            newIPNode<ColorSaturationIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorGrayScale", 2, false, "saturation",
            newIPNode<ColorGrayScaleIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorVibrance", 2, false, "vibrance",
            newIPNode<ColorVibranceIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorLinearToSRGB", 2, false, "linearToSRGB",
            newIPNode<ColorLinearToSRGBIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorSRGBToLinear", 2, false, "SRGBToLinear",
            newIPNode<ColorSRGBToLinearIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "NoiseReduction", 2, false, "denoise",
            newIPNode<NoiseReductionIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition("Clarity", 2, false, "clarity",
                                            newIPNode<ClarityIPNode>, "", "",
                                            emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorTemperature", 2, false, "temperature",
            newIPNode<ColorTemperatureIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ColorExposure", 2, false, "exposure",
            newIPNode<ColorExposureIPNode>, "", "", emptyIcon, true));

        m->addDefinition(new NodeDefinition("ColorCurve", 2, false, "curve",
                                            newIPNode<ColorCurveIPNode>, "", "",
                                            emptyIcon, true));

        {
            NodeDefinition* def = new NodeDefinition(
                "FileSource", 1, false, "fileSource",
                newIPNode<FileSourceIPNode>, "", "", emptyIcon, false);

            def->declareProperty<IntProperty>(
                "defaults.progressiveSourceLoading", 1);
            def->declareProperty<StringProperty>("defaults.missingMovieProc",
                                                 "black");
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition(
            "ImageSource", 1, false, "imageSource",
            newIPNode<ImageSourceIPNode>, "", "", emptyIcon, false));

#if 0
    {
        NodeDefinition* def = new NodeDefinition("Format", 1, false,
                                                 "format",
                                                 newIPNode<FormatIPNode>,
                                                 "", "", emptyIcon,
                                                 false);

        def->declareProperty<IntProperty>("defaults.cropLRBT", 1);
        m->addDefinition(def);
    }
#endif

        {
            NodeDefinition* def = new NodeDefinition(
                "Linearize", 1, false, "linearize", newIPNode<LinearizeIPNode>,
                "", "", emptyIcon, false);
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "YCToRGB", 1, false, "ycToRGB", newIPNode<YCToRGBIPNode>, "",
                "", emptyIcon, false);
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition("LensWarp", 1, false, "lensWarp",
                                            newIPNode<LensWarpIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition(
            "PrimaryConvert", 1, false, "primaryConvert",
            newIPNode<PrimaryConvertIPNode>, "", "", emptyIcon, false));

        {
            NodeDefinition* def = new NodeDefinition(
                "LookLUT", 1, false, "look", newIPNode<LookIPNode>, "", "",
                emptyIcon, false);
            def->declareProperty<IntProperty>(
                "defaults.progressiveSourceLoading", 1);
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition("Overlay", 1, false, "overlay",
                                            newIPNode<OverlayIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition("Paint", 3, false, "paint",
                                            newIPNode<PaintIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition("Crop", 3, false, "crop",
                                            newIPNode<CropIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition("RotateCanvas", 3, false, "rotate",
                                            newIPNode<RotateCanvasIPNode>, "",
                                            "", emptyIcon, false));

        m->addDefinition(new NodeDefinition("Sequence", 1, false, "sequence",
                                            newIPNode<SequenceIPNode>, "", "",
                                            emptyIcon, false));

        {
            NodeDefinition* def = new NodeDefinition(
                "SourceGroup", 1, true, "sourceGroup",
                newIPNode<SourceGroupIPNode>, "", "", emptyIcon);

            def->declareProperty<StringProperty>("defaults.preCacheLUTType",
                                                 "CacheLUT");
            def->declareProperty<StringProperty>("defaults.formatType", "");
            def->declareProperty<StringProperty>("defaults.channelMapType",
                                                 "ChannelMap");
            def->declareProperty<StringProperty>("defaults.cacheType", "Cache");
            def->declareProperty<StringProperty>("defaults.paintType", "Paint");
            def->declareProperty<StringProperty>("defaults.overlayType",
                                                 "Overlay");
            def->declareProperty<StringProperty>("defaults.colorPipelineType",
                                                 "ColorPipelineGroup");
            def->declareProperty<StringProperty>(
                "defaults.linearizePipelineType", "LinearizePipelineGroup");
            def->declareProperty<StringProperty>("defaults.lookPipelineType",
                                                 "LookPipelineGroup");
            def->declareProperty<StringProperty>("defaults.stereoType",
                                                 "SourceStereo");
            def->declareProperty<StringProperty>("defaults.transformType",
                                                 "Transform2D");
            def->declareProperty<StringProperty>("defaults.cropType", "Crop");
            def->declareProperty<StringProperty>("defaults.sourceType",
                                                 "FileSource");
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition(
            "SourceStereo", 1, false, "sourceStereo",
            newIPNode<SourceStereoIPNode>, "", "", emptyIcon, false));

        m->addDefinition(new NodeDefinition("Stack", 1, false, "stack",
                                            newIPNode<StackIPNode>, "", "",
                                            emptyIcon, false));
    }

} // namespace IPCore
