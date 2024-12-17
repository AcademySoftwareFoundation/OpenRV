//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RvApp__RvGraph__h__
#define __RvApp__RvGraph__h__
#include <IPCore/IPGraph.h>

#include <atomic>

namespace IPCore
{
    class FileSourceIPNode;
    class ImageSourceIPNode;
    class LayoutGroupIPNode;
    class SequenceGroupIPNode;
    class SourceGroupIPNode;
    class SourceIPNode;
    class StackGroupIPNode;
} // namespace IPCore

namespace Rv
{

    //
    //  RvGraph
    //
    //  This class controls the IPNode graph. It also owns one or more
    //  evaluation threads which are the only threads allowed to evaluate
    //  the graph. The RvGraph can also cache results and guarantee that
    //  pixels will be available for a certain amount of time.
    //

    class RvGraph : public IPCore::IPGraph
    {
    public:
        typedef std::vector<IPCore::SourceIPNode*> Sources;
        typedef boost::signals2::signal<void(bool /*begin*/,
                                             int /*fastAddSourceEnabled*/)>
            FastAddSourceChangedSignal;

        RvGraph(const IPCore::NodeManager*);
        ~RvGraph();

        //
        //  Create a sparse node from the given node. This will share prop
        //  data via referencing
        //

        virtual TwkContainer::PropertyContainer*
        sparseContainer(IPCore::IPNode*);

        virtual void initializeIPTree(const VideoModules&);
        virtual void removeNode(IPCore::IPNode*);
        virtual IPCore::DisplayGroupIPNode*
        newDisplayGroup(const std::string& nodeName,
                        const TwkApp::VideoDevice* d = 0);
        virtual IPCore::OutputGroupIPNode*
        newOutputGroup(const std::string& nodeName,
                       const TwkApp::VideoDevice* d = 0);

        const Sources& imageSources() const { return m_imageSources; }

        IPCore::SourceIPNode*
        addSource(const std::string& nodeType, const std::string& nodeName = "",
                  const std::string& mediaRepName = "",
                  IPCore::SourceIPNode* mediaRepSisterSrcNode = nullptr);

        // Fast add source mechanism which postpones the default views' inputs
        // connection until all the new sources have been added to prevent
        // O(n^2)
        void connectNewSourcesToDefaultViews();

        // Scope of the fast add source mechanism which postpones the default
        // views' inputs connection until all the new sources have been added
        void addSourceBegin()
        {
            int newFastAddSourceEnabled = ++m_fastAddSourceEnabled;

            m_fastAddSourceChangedSignal(true, newFastAddSourceEnabled);
        }

        void addSourceEnd()
        {
            int newFastAddSourceEnabled = --m_fastAddSourceEnabled;

            if (newFastAddSourceEnabled == 0)
                connectNewSourcesToDefaultViews();

            m_fastAddSourceChangedSignal(false, newFastAddSourceEnabled);
        }

        bool isFastAddSourceEnabled() const
        {
            return m_fastAddSourceEnabled > 0;
        }

        // gives access to the signal call when fastAddSourceEnabled changed
        //
        FastAddSourceChangedSignal& fastAddSourceChangedSignal()
        {
            return m_fastAddSourceChangedSignal;
        }

        // Scope guard of the same fast add source mechanism
        class FastAddSourceGuard
        {
        public:
            explicit FastAddSourceGuard(RvGraph& rvgraph)
                : m_rvgraph(rvgraph)
            {
                m_rvgraph.addSourceBegin();
            }

            ~FastAddSourceGuard() { m_rvgraph.addSourceEnd(); }

        private:
            RvGraph& m_rvgraph;
        };

    protected:
        IPCore::SourceGroupIPNode* newSourceGroup(const std::string& nodeName,
                                                  IPCore::SourceIPNode*);
        IPCore::SourceIPNode* newSource(const std::string& nodeName,
                                        const std::string& nodeType,
                                        const std::string& mediaRepName);
        void setupSource(IPCore::SourceIPNode*,
                         IPCore::SourceIPNode* mediaRepSisterSrcNode);

    private:
        Sources m_imageSources;
        IPCore::SequenceGroupIPNode* m_sequenceNode;
        IPCore::StackGroupIPNode* m_stackNode;
        IPCore::LayoutGroupIPNode* m_layoutNode;
        IPCore::IPNode::IPNodes m_newSources;
        std::atomic<int> m_fastAddSourceEnabled{0};
        FastAddSourceChangedSignal m_fastAddSourceChangedSignal;
    };

} // namespace Rv

#endif // __RvApp__RvGraph__h__
