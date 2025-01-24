//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__SequenceGroupIPNode__h__
#define __IPGraph__SequenceGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class SequenceIPNode;
    class PaintIPNode;
    class FileSourceIPNode;
    class AudioAddIPNode;
    class RetimeIPNode;

    /// SequenceGroupIPNode manages a sub-graph that includes a sequence at the
    /// root

    ///
    /// The sub-graph contains one SequenceIPNode as the root. The rest of
    /// the nodes in the sub-graph are conditioners on the inputs of the
    /// sequence. For example, any kind of "per-cut" operation might be
    /// done this way (e.g. annotation only appearing per-source in a
    /// sequence). By default the group is just the sequence node.
    ///

    class SequenceGroupIPNode : public GroupIPNode
    {
    public:
        //
        //  Types
        //

        //
        //  EDLClipInfo is used to report the contents of the SourceGroupIPNode
        //  in a friendly way for code that converts it into some other form
        //  (e.g. an EDL exporter). The underlying logic of how the EDL is
        //  stored and evaluated should be hidden from code outside of
        //  SequenceGroupIPNode and the underlying SequenceIPNode.
        //

        struct EDLClipInfo
        {
            IPNode* node;
            IPNode::MediaInfoVector media;
            size_t index;
            int in;
            int out; // inclusive
            int inputCutIn;
            int inputCutOut; // inclusive
            float inputFPS;
        };

        typedef std::vector<EDLClipInfo> EDLInfo;

        SequenceGroupIPNode(const std::string& name, const NodeDefinition* def,
                            IPGraph* graph, GroupIPNode* group = 0);

        virtual ~SequenceGroupIPNode();

        virtual void setInputs(const IPNodes&) override;
        virtual IPNode* newSubGraphForInput(size_t, const IPNodes&) override;
        virtual IPNode* modifySubGraphForInput(size_t, const IPNodes&,
                                               IPNode*) override;
        virtual void propertyChanged(const Property*) override;
        void inputMediaChanged(IPNode* srcNode, int srcInputIndex,
                               PropagateTarget target) override;

        SequenceIPNode* sequenceNode() const { return m_sequenceNode; }

        static void setDefaultAutoRetime(bool b) { m_defaultAutoRetime = b; }

        //
        //  Report on EDL contents. NOTE: this function is not "efficient". Its
        //  meant to work even if the underlying sequence implemenation
        //  changes. In order to do that it samples *every* frame of the input
        //  range.
        //

        EDLInfo edlInfo() const;

    private:
        std::string retimeType();
        std::string paintType();
        std::string sequenceType();
        std::string audioAddType();
        std::string audioSourceType();

        void configureSoundTrack();
        void rebuild(int inputIndex = -1);

    private:
        SequenceIPNode* m_sequenceNode;
        IntProperty* m_retimeToOutput;
        RetimeIPNode* m_soundTrackRetime;
        FileSourceIPNode* m_soundTrackNode;
        StringProperty* m_soundTrackFile;
        FloatProperty* m_soundTrackOffset;
        AudioAddIPNode* m_audioAddNode;
        bool m_useMaxFPS;
        float m_maxFPS;

        IntProperty* m_markersIn;
        IntProperty* m_markersOut;
        Vec4fProperty* m_markersColor;
        StringProperty* m_markersName;

        static bool m_defaultAutoRetime;
    };

} // namespace IPCore

#endif // __IPGraph__SequenceGroupIPNode__h__
