//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/CoreDefinitions.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/AudioTextureIPNode.h>
#include <IPCore/CacheIPNode.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/ViewGroupIPNode.h>
#include <IPCore/DisplayIPNode.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/DynamicIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeManager.h>
#include <IPCore/OutputGroupIPNode.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/ResizeIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/TextureOutputGroupIPNode.h>
#include <IPCore/Transform2DIPNode.h>
#include <IPCore/RootIPNode.h>
#include <IPCore/HistogramIPNode.h>

namespace IPCore
{
    using namespace std;

    typedef TwkContainer::StringProperty StringProperty;
    typedef TwkContainer::IntProperty IntProperty;

    void addCoreNodeDefinitions(NodeManager* m)
    {
        NodeDefinition::ByteVector emptyIcon;
        m->addDefinition(new NodeDefinition("Adaptor", 1, false, "adaptor",
                                            newIPNode<AdaptorIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition("Root", 1, false, "root",
                                            newIPNode<RootIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition(
            "Session", 2, false, "session", newIPNode<SessionIPNode>,
            "Session container", "", emptyIcon, false));

        m->addDefinition(new NodeDefinition(
            "PipelineGroup", 1, true, "pipeline",
            newIPNode<PipelineGroupIPNode>, "Managed Single Input Pipeline", "",
            emptyIcon, true));

        m->addDefinition(new NodeDefinition(
            "ViewPipelineGroup", 1, true, "viewPipelineGroup",
            newIPNode<PipelineGroupIPNode>, "Managed Single Input Pipeline", "",
            emptyIcon, true));

        {
            NodeDefinition* def = new NodeDefinition(
                "DisplayPipelineGroup", 1, true, "displayPipelineGroup",
                newIPNode<PipelineGroupIPNode>, "Managed Single Input Pipeline",
                "", emptyIcon, true);

            def->declareProperty<StringProperty>("defaults.pipeline",
                                                 "DisplayColor");
            m->addDefinition(def);
        }
        m->addDefinition(new NodeDefinition("Cache", 1, false, "cache",
                                            newIPNode<CacheIPNode>, "", "",
                                            emptyIcon, false));

        m->addDefinition(new NodeDefinition(
            "DispTransform2D", 1, false, "dispTransform2D",
            newIPNode<DispTransform2DIPNode>, "", "", emptyIcon, false));

        {
            NodeDefinition* def = new NodeDefinition(
                "DisplayColor", 1, false, "displayColor",
                newIPNode<DisplayIPNode>, "", "", emptyIcon, false);

            def->declareProperty<IntProperty>(
                "defaults.progressiveSourceLoading", 1);
            def->declareProperty<IntProperty>("defaults.overridableColorspace",
                                              1);
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "DisplayGroup", 1, true, "displayGroup",
                newIPNode<DisplayGroupIPNode>, "", "", emptyIcon, false);

            def->declareProperty<StringProperty>("defaults.stereoType",
                                                 "DisplayStereo");
            def->declareProperty<StringProperty>("defaults.pipelineType",
                                                 "DisplayPipelineGroup");
            // no resize by default
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "OutputGroup", 1, true, "outputGroup",
                newIPNode<OutputGroupIPNode>, "", "", emptyIcon, false);

            def->declareProperty<StringProperty>("defaults.stereoType",
                                                 "DisplayStereo");
            def->declareProperty<StringProperty>("defaults.pipelineType",
                                                 "DisplayPipelineGroup");
            def->declareProperty<StringProperty>("defaults.resizeType",
                                                 "Resize");
            def->declareProperty<IntProperty>("defaults.autoResize", 1);
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "TextureOutputGroup", 1, true, "textureOutputGroup",
                newIPNode<TextureOutputGroupIPNode>, "", "", emptyIcon, false);

            def->declareProperty<StringProperty>("defaults.stereoType", "");
            def->declareProperty<StringProperty>("defaults.pipelineType",
                                                 "DisplayPipelineGroup");

            //
            // For automatic resampling of UI textures uncomment the
            // following 2 lines
            //
            // def->declareProperty<StringProperty>("defaults.resizeType",
            // "Resize");
            // def->declareProperty<IntProperty>("defaults.autoResize", 1);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "Resize", 1, true, "resize", newIPNode<ResizeIPNode>, "", "",
                emptyIcon, false);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "Histogram", 1, true, "histogram", newIPNode<HistogramIPNode>,
                "", "", emptyIcon, false);

            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "ViewGroup", 1, true, "viewGroup", newIPNode<ViewGroupIPNode>,
                "", "", emptyIcon, false);

            def->declareProperty<StringProperty>("defaults.soundtrackType",
                                                 "SoundTrack");
            def->declareProperty<StringProperty>("defaults.dispTransformType",
                                                 "DispTransform2D");
            def->declareProperty<StringProperty>("defaults.waveformType",
                                                 "AudioWaveform");
            def->declareProperty<StringProperty>("defaults.pipelineType",
                                                 "ViewPipelineGroup");
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition(
            "DisplayStereo", 1, false, "displayStereo",
            newIPNode<DisplayStereoIPNode>, "", "", emptyIcon, false));

        {
            NodeDefinition* def = new NodeDefinition(
                "SoundTrack", 1, false, "soundTrack",
                newIPNode<SoundTrackIPNode>, "", "", emptyIcon, false);

            def->declareProperty<IntProperty>("defaults.softClamp", 0);
            m->addDefinition(def);
        }

        {
            NodeDefinition* def = new NodeDefinition(
                "Transform2D", 1, false, "transform2D",
                newIPNode<Transform2DIPNode>, "", "", emptyIcon, false);

            def->declareProperty<IntProperty>("defaults.newStyleVisibleBox", 1);
            m->addDefinition(def);
        }

        m->addDefinition(new NodeDefinition(
            "AudioWaveform", 1, false, "audioWaveform",
            newIPNode<AudioTextureIPNode>, "", "", emptyIcon, false));

        m->addDefinition(new NodeDefinition("Dynamic", 1, true, "dynamic",
                                            newIPNode<DynamicIPNode>, "", "",
                                            emptyIcon));
    }

} // namespace IPCore
