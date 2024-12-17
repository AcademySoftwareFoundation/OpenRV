//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RVApp__RvSession__h__
#define __RVApp__RvSession__h__
#include <RvApp/RvGraph.h>
#include <IPCore/IPImage.h>
#include <IPCore/IPNode.h>
#include <IPCore/Session.h>
#include <Mu/Object.h>
#include <Mu/Value.h>
#include <RvApp/Options.h>

namespace Mu
{
    class CallEnvironment;
}

namespace IPCore
{
    class IPNode;
    class SourceIPNode;
    class SequenceIPNode;
    class SwitchIPNode;
} // namespace IPCore

namespace Rv
{
    class LoadState;

    //
    //  class Session
    //
    //  This object represents the "document" in as much as Rv has a
    //  "document". The Session can be a single image, a movie, an edit, a
    //  composite, etc, etc.
    //

    class RvSession : public IPCore::Session
    {
    public:
        //
        //  Types
        //

        typedef std::vector<IPCore::SourceIPNode*> Sources;

        //
        //  Constructors
        //

        RvSession();
        virtual ~RvSession();

        void postInitialize();

        const RvGraph& rvgraph() const
        {
            return *static_cast<RvGraph*>(m_graph);
        }

        RvGraph& rvgraph() { return *static_cast<RvGraph*>(m_graph); }

        //
        //  Takes a file list and calls read with sequences created from
        //  it. Calls read() below with addContents=true. It will throw if
        //  any file in files is unreadable, but only AFTER trying to read
        //  all of them.
        //
        //  If a set of files in the file list is bracket by files named
        //  '[' and ']' (without nesting) the files are "grouped". In that
        //  case all of the file sources in the group are put into an
        //  input source together. This is used, for example, to make
        //  stereo pairs.
        //
        //  Its an error to have unmatched '[' or ']' names. If that
        //  happens, the function will throw a read error.
        //
        //  throws AllReadFailedExc (if none of the files worked)
        //         ReadFailedExc (if some of the files worked)
        //

        void readUnorganizedFileList(const StringVector& files,
                                     bool doProcessOpts = false,
                                     bool merge = false,
                                     const std::string& tag = "");

        //
        //  Replaces contents with filename or if addContents==true then
        //  add the contents of the file instead of replacing. You should
        //  throw out of here if the read failed.
        //

        virtual void read(const std::string& filename, const ReadRequest&);

        //
        //  Write contents to filename or if partial==true, then write
        //  only some of the data. (This is interpreted by the document
        //  sub-class.) Normally this would mean: write selection
        //  only. you should throw out of here if the write fails.
        //

        virtual void write(const std::string& filename, const WriteRequest&);

        //
        //  Similar, but does not change the filename of the session
        //

        void saveACopyAs(const char* filename, bool partial = false,
                         bool compressed = false, bool sparse = true);

        virtual void makeActive();
        virtual void clear();
        virtual void render();

        bool isEmpty() const;

        //
        //  If the user has set the view size (from Mu) we may not want
        //  to do it ourselves.
        //
        void setUserHasSetViewSize(bool v) { m_userHasSetViewSize = v; };

        bool userHasSetViewSize() { return m_userHasSetViewSize; };

        //
        //  Source material.  Some methods add a bunch of media paths as a
        //  single source. This is useful for making stereo sources for example.
        //  Set the name if you are reading the source from a file, otherwise
        //  it'll be created programmatically.
        //

        IPCore::SourceIPNode* addSourceWithTag(const StringVector& files,
                                               std::string tag,
                                               std::string nodeName = "",
                                               std::string mediaRepName = "");
        IPCore::SourceIPNode* addSource(std::string, Options::SourceArgs& sargs,
                                        std::string nodeName = "");
        IPCore::SourceIPNode* addSource(const StringVector&,
                                        Options::SourceArgs& sargs,
                                        std::string nodeName = "",
                                        std::string nodeType = "RVFileSource",
                                        std::string mediaRepName = "",
                                        std::string mediaRepSource = "");
        IPCore::SourceIPNode* addImageSource(const std::string&,
                                             const TwkMovie::MovieInfo&);

        void newMediaLoaded(IPCore::SourceIPNode*);

        //
        //  Delete a node. This can be a source (group) node or whatever
        //

        virtual void deleteNode(IPCore::IPNode*);

        //
        //  Add another media path to the current source (for stereo, etc) you
        //  can pass "" for sourceName to use the "current" source ala pre 3.8.7
        //

        IPCore::SourceIPNode* addToSourceHelper(const std::string&,
                                                StringVector files,
                                                Options::SourceArgs& sargs);

        void addToSource(const std::string& sourceName,
                         const std::string& filename,
                         const std::string& tag = "");

        //
        //  Relocate lost media files.
        //

        void relocateSource(const std::string& oldFilename,
                            const std::string& newFilename,
                            const std::string& sourceNode = "");

        //
        //  Set all the media in a particular source
        //

        void setSourceMedia(const std::string& sourceNode,
                            const StringVector& files, const std::string& tag);

        //
        //  Add a media representation to an existing source specified by
        //  srcNodeName
        //

        IPCore::SourceIPNode* addSourceMediaRep(
            const std::string& srcNodeName, const std::string& mediaRepName,
            const StringVector& mediaRepPaths, const std::string& tag);

        //
        //  Set the active input of the Switch node specified or the ones
        //  associated with the specified source node to the given media
        //  representation specified by name.
        //

        void setActiveSourceMediaRep(const std::string& srcNodeOrSwitchNodeName,
                                     const std::string& mediaRepName,
                                     const std::string& tag);

        //
        //  Returns the name of the media representation currently selected by
        //  the Switch Group corresponding to the given RVFileSource node.
        //

        std::string sourceMediaRep(const std::string& srcNodeOrSwitchNodeName);

        //
        //  Returns the names of the media representations available for the
        //  Switch Group corresponding to the given RVFileSource node or
        //  RVSwitch node. Also optionally returns the source nodes associated
        //  with these media reps.
        //

        void sourceMediaReps(const std::string& srcNodeOrSwitchNodeName,
                             StringVector& sourceMediaReps,
                             StringVector* sourceNodes = nullptr);

        //
        // Returns the source media rep switch node associated with the source
        // if any
        //

        std::string sourceMediaRepSwitchNode(const std::string& srcNodeName);

        //
        // Returns the source media rep's switch node's first input associated
        // with the source if any
        //

        std::string sourceMediaRepSourceNode(const std::string& srcNodeName);

        //
        //  Mu code to run on initialization
        //

        static void setInitEval(const std::string& s) { m_initEval = s; }

        static std::string initEval() { return m_initEval; }

        static void setPyInitEval(const std::string& s) { m_pyInitEval = s; }

        static std::string pyInitEval() { return m_pyInitEval; }

        //
        //  Sources / Nodes
        //

        const Sources& sources() const { return rvgraph().imageSources(); }

        std::string currentSourceName() const;

        //
        //  "Top" FB -- not really useful anymore
        //

        const TwkFB::FrameBuffer* currentFB() const;

        //
        //  FB for one of the current sources
        //

        const TwkFB::FrameBuffer* currentFB(const std::string&) const;

        //
        //  Same syntax as currentFB but returns a SourceNode
        //

        IPCore::SourceIPNode* sourceNode(const std::string&) const;

        //
        //  Same syntax as currentFB but returns a SwitchNode
        //

        IPCore::SwitchIPNode* switchNode(const std::string&) const;

        //
        //  Data
        //

        Mu::Object* data() const { return m_data; }

        void* pyData() const { return m_pydata; }

        static RvSession* currentRvSession()
        {
            return static_cast<RvSession*>(IPCore::Session::currentSession());
        }

        //
        //  Set scaling on all transform nodes. Used by linux
        //  version command line argument.
        //

        void setScaleOnAll(float scale);

        //
        //  LUTs
        //

        void readCDL(std::string, const std::string& node,
                     bool activate = false);
        void readLUT(std::string, const std::string& node,
                     bool activate = false);
        void readLUTOnAll(std::string, const std::string& nodeType,
                          bool activate = true);

        std::string lookupMuFlag(std::string key);

        //
        //  Loading status
        //

        int loadCount();
        int loadTotal();

        //
        //  Override
        //

        virtual void findProperty(PropertyVector& props,
                                  const std::string& name);
        virtual void findCurrentNodesByTypeName(NodeVector& nodes,
                                                const std::string& typeName);
        virtual void findNodesByTypeName(NodeVector& nodes,
                                         const std::string& typeName);

        virtual IPCore::IPNode* newNode(const std::string& typeName,
                                        const std::string& nodeName);

        virtual void setRendererType(const std::string&);

        virtual void readProfile(const std::string& filename,
                                 IPCore::IPNode* node, const WriteRequest&);

    protected:
        virtual void readGTO(const std::string& filename, bool merge,
                             Options::SourceArgs& sargs);
        void readEDL(const char* filename);
        void readGTOSessionContainer(PropertyContainer*);

        void applyRvSessionSpecificOptions();

        void checkForStereoPaths(const std::string&, StringVector&);

        void readSource(const std::string& filename, Options::SourceArgs& sargs,
                        bool addContents, bool addToExistingSource,
                        const std::string& tag = "", bool merge = false);

        void readSourceHelper(StringVector files, Options::SourceArgs& sargs,
                              bool addContents, bool addToExistingSource);

        void continueLoading();
        void applySingleSourceArgs(Options::SourceArgs& sargs,
                                   IPCore::SourceIPNode* node);

        void processOptionsAfterSourcesLoaded(bool hadLoadError = false);

        void buildFileList(const StringVector& files, std::string tag,
                           StringVector& collected);
        std::vector<IPCore::FileSourceIPNode*>
        findSourcesByMedia(const std::string& filename,
                           const std::string& sourceNode = "");
        std::vector<IPCore::SwitchIPNode*>
        findSwitchIPNodes(const std::string& srcNodeOrSwitchNodeName);

        void setSequenceEvents();
        void unsetSequenceEvents();
        void setSequenceIPNode(IPCore::SequenceIPNode* newSequenceIPNode);
        void onSequenceChanging();
        void onSequenceChanged();
        void onGraphFastAddSourceChanged(bool begin,
                                         int newFastAddSourceEnabled);
        void onGraphMediaSetEmpty();
        void onGraphNodeWillRemove(IPCore::IPNode* node);

    private:
        Mu::Object* m_data;
        void* m_pydata;
        bool m_loadingError;
        std::string m_loadingErrorString;
        Mu::CallEnvironment* m_callEnv;
        StringMap m_muFlags;
        LoadState* m_loadState;
        int m_gtoSourceTotal;
        int m_gtoSourceCount;
        bool m_userHasSetViewSize;

        // The source node which gives its fps to the graph.
        // Normally the first source inserted in the graph.
        IPCore::SourceIPNode* m_conductorSource;

        /// sequence IP node of the graph
        IPCore::SequenceIPNode* m_sequenceIPNode{nullptr};

        /// source IP node that contains the play head when the sequence
        /// node starts its layout
        int m_currentSourceIndex{-1};

        /// the range info of the source that contains the play head
        /// at the beginning of the layout
        IPCore::IPNode::ImageRangeInfo m_currentSourceRangeInfo;

        /// offset of the source that contains the playhead at the beginning
        /// of the layout
        int m_currentSourceOffset{0};

        /// next sequence layout is the first one
        bool m_firstSequenceChange{true};

        boost::signals2::connection m_fastAddSourceChangedConnection;
        boost::signals2::connection m_mediaLoadingSetEmptyConnection;
        boost::signals2::connection m_nodeWillRemoveConnection;

        static std::string m_initEval;
        static std::string m_pyInitEval;

    public:
        static Session* m_currentSession;
    };

} // namespace Rv

#endif // __RVApp__RvSession__h__
