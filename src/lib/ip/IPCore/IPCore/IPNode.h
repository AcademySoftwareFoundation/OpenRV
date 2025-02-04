//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__IPNode__h__
#define __IPCore__IPNode__h__
#include <IPCore/FBCache.h>
#include <IPCore/IPImage.h>
#include <TwkAudio/Audio.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkContainer/PropertyContainer.h>
#include <TwkContainer/Properties.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <TwkMovie/Movie.h>
#include <limits>
#include <string>
#include <vector>
#include <boost/signals2.hpp>

namespace TwkMovie
{
    class MovieInfo;
};

/* ajg - min/max !!! MIGHT NOT BE NECC ANYMORE?*/
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define IPGRAPH_CONSTRUCTOR_DECLARATION(NAME)                           \
    static IPNode* NAME(const std::string& name, const NodeDefinition*, \
                        IPGraph*, GroupIPNode*)

namespace IPCore
{
    class IPGraph;
    class GroupIPNode;
    class NodeDefinition;

    ///
    /// Image and Audio processing node base class for the IPGraph
    ///
    /// Evaluation on an IPNode can occur independently for audio and images.
    ///
    /// There is also a third evaluation type for image identifiers. An image
    /// identifier represents the contents of an image without actually reading
    /// it. The identifier can be used as a cache key.
    ///

    class IPNode : public TwkContainer::PropertyContainer
    {
    public:
        //
        //  Types
        //

        typedef TwkContainer::PropertyContainer PropertyContainer;
        typedef std::vector<IPNode*> IPNodes;
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef TwkFB::FBInfo FBInfo;
        typedef std::vector<FrameBuffer*> FrameBuffers;
        typedef TwkContainer::Vec4fProperty LUTProperty;
        typedef TwkAudio::AudioBuffer AudioBuffer;
        typedef TwkMovie::MovieInfo MovieInfo;
        typedef std::vector<std::string> StringVector;
        typedef TwkContainer::Property Property;
        typedef TwkContainer::Component Component;
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::HalfProperty HalfProperty;
        typedef TwkContainer::Vec2fProperty Vec2fProperty;
        typedef TwkContainer::Vec3fProperty Vec3fProperty;
        typedef TwkContainer::Vec4fProperty Vec4fProperty;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkContainer::Vec2iProperty Vec2iProperty;
        typedef TwkContainer::Vec3iProperty Vec3iProperty;
        typedef TwkContainer::Vec4iProperty Vec4iProperty;
        typedef TwkContainer::Mat44fProperty Mat44fProperty;
        typedef TwkContainer::Mat33fProperty Mat33fProperty;
        typedef TwkContainer::StringProperty StringProperty;
        typedef TwkContainer::ByteProperty ByteProperty;
        typedef TwkContainer::StringPair StringPair;
        typedef TwkContainer::StringPairProperty StringPairProperty;
        typedef TwkMath::Vec4f Vec4;
        typedef TwkMath::Vec3f Vec3;
        typedef TwkMath::Vec2f Vec2;
        typedef TwkMath::Box2f Box2;
        typedef std::set<IPNode*> IPNodeSet;
        typedef std::vector<int> IndexArray;
        typedef IPImage::Matrix Matrix;
        typedef std::vector<int> FrameVector;

        //
        //  Signal types mimic the notification virtual functions
        //

        typedef boost::signals2::signal<void()> VoidSignal;
        typedef boost::signals2::signal<void(const Property*)> PropertySignal;
        typedef boost::signals2::signal<void(const Property*, size_t, size_t)>
            PropertyInsertSignal;
        typedef boost::signals2::signal<void(const std::string&)>
            PropertyNameSignal;
        typedef boost::signals2::signal<void(const std::string&, unsigned int)>
            ReadCompletedSignal;

        //
        //  For some applications (EG session writing), we want to maintain a
        //  sorted set.  Note that the use of the name() method here _requires_
        //  that the node not be deleted before it is removed from the set. Also
        //  the name of a node should not change while it is in the set.
        //
        struct NodeComp
        {
            bool operator()(const IPNode* a, const IPNode* b) const
            {
                if (a && b)
                    return (a->name() < b->name());
                else
                    return (a < b);
            }
        };

        typedef std::set<IPNode*, NodeComp> SortedIPNodeSet;

        enum ThreadType
        {
            CacheLockedThread = (1 << 0),
            CacheUnlockedThread = (1 << 1),

            DisplayNoEvalThread = (1 << 2),
            DisplayEvalThread = (1 << 3),
            DisplayPassThroughThread = (1 << 4),
            DisplayMaybeEvalThread = (1 << 5),
            DisplayCacheEvalThread = (1 << 6),
            DisplayThreadMarker = (1 << 7),

            CacheThreadMarker = (1 << 8),
            CacheEvalThread = (1 << 9),

            DisplayThread = DisplayNoEvalThread | DisplayEvalThread
                            | DisplayPassThroughThread | DisplayMaybeEvalThread
                            | DisplayCacheEvalThread
        };

        //
        //  Image views, layers, and channels, (maybe parts in the future)
        //  are represented as an ImageComponent. Only a single
        //  ImageComponent other than the default can be asked for per
        //  evaluation.
        //

        enum ComponentType
        {
            NoComponent,
            ViewComponent,
            LayerComponent,
            ChannelComponent
        };

        enum PropagateTarget : int
        {
            GroupPropagateTarget = 0x0001,  // propagates event in group
            GraphPropagateTarget = 0x0002,  // propagates event in graph
            OutputPropagateTarget = 0x0004, // propagates event throw ouputs
            MemberPropagateTarget =
                0x0008, // propages event in members of group
            GroupAndGraphInOutputPropagateTarget =
                0x1000, // allow graph and graph event propagation in outpus

            // same event propagation behaviour then RV 7.6
            LegacyPropagateTarget = GroupPropagateTarget | GraphPropagateTarget
                                    | OutputPropagateTarget
                                    | MemberPropagateTarget
                                    | GroupAndGraphInOutputPropagateTarget,

            // new fast event propagation behaviour
            // we avoid to propagate in graph and group after an output is
            // crossed
            FastPropagateTarget = GroupPropagateTarget | GraphPropagateTarget
                                  | OutputPropagateTarget
        };

        struct ImageComponent
        {
            ImageComponent(ComponentType t = NoComponent)
                : type(t)
            {
            }

            ImageComponent(ComponentType t, const std::string& a)
                : type(t)
                , name(1)
            {
                name[0] = a;
            }

            ImageComponent(ComponentType t, const std::string& a,
                           const std::string& b)
                : type(t)
                , name(2)
            {
                name[0] = a;
                name[1] = b;
            }

            ImageComponent(ComponentType t, const std::string& a,
                           const std::string& b, const std::string& c)
                : type(t)
                , name(3)
            {
                name[0] = a;
                name[1] = b;
                name[2] = c;
            }

            bool isValid() const
            {
                return (type == ViewComponent && name.size() == 1)
                       || (type == LayerComponent && name.size() == 2)
                       || (type == ChannelComponent && name.size() == 3);
            }

            ComponentType type;
            StringVector name;
        };

        struct StereoContext
        {
            //
            //  All these vars refer to bits of state set by the evaluate()
            //  caller, so that stereo evaluators can do the right thing.
            //  Typical case is that vars are set by DisplayStereoIPNode, so
            //  that SourceStereoIPNode can respond appropriately.
            //

            StereoContext()
                : active(false)
                , swap(false)
                , flip(false)
                , flop(false)
                , offset(0.0)
                , roffset(0.0)
                , rrotate(0.0)
                , rtranslate(0.0, 0.0)
            {
            }

            bool active;  //  StereoContext is ignored unless active is true
            bool swap;    //  Swap will happen somewhere closer to the root
            bool flip;    //  Right-eye flip will happen somewhere closer to the
                          //  root
            bool flop;    //  Right-eye flop will happen somewhere closer to the
                          //  root
            float offset; //  Additional stereo offset
            float roffset; //  Additional stereo right-eye-only offset
            float
                rrotate; //  Additional right-eye rotation **CURRENTLY UNUSED**
            Vec2 rtranslate; //  Additional right-eye translation **CURRENTLY
                             //  UNUSED**
        };

        /// Image evaluation context

        struct Context
        {
            Context(int evalFrame_, int baseFrame_, float fps_, int w, int h,
                    ThreadType thread_, size_t threadNum_, FBCache& cache_,
                    bool stereo_ = false)
                : frame(evalFrame_)
                , baseFrame(baseFrame_)
                , thread(thread_)
                , threadNum(threadNum_)
                , cache(cache_)
                , cacheNode(0)
                , stereo(stereo_)
                , allowInteractiveResize(true)
                , eye(0)
                , missing(false)
                , viewWidth(w)
                , viewHeight(h)
                , viewXOrigin(0)
                , viewYOrigin(0)
                , deviceWidth(0)
                , deviceHeight(0)
                , fps(fps_)
            {
            }

            int frame;     /// frame to evaluate for
            int baseFrame; /// frame being rendered
            int viewWidth;
            int viewHeight;
            int viewXOrigin; /// necessary for scanline and
            int viewYOrigin; /// checker modes
            int deviceWidth;
            int deviceHeight;
            float fps;
            ThreadType thread;
            size_t threadNum;
            FBCache& cache;
            const IPNode* cacheNode;     /// For per-node caching
            bool allowInteractiveResize; /// if false don't do it
            bool stereo;                 /// render is in stereo mode
            bool missing;                /// find a proxy for this frame
            mutable StereoContext
                stereoContext; /// set at display level to inform source-level
                               /// stereo eval.
            size_t eye; /// request a specific eye (0=left, 1=right, 2=whatever)
            ImageComponent component; /// defaults to NoComponent
        };

        typedef Context FlushContext;

        /// Audio evaluation context

        struct AudioContext
        {
            AudioContext(AudioBuffer& b, float f)
                : buffer(b)
                , fps(f)
            {
            }

            AudioBuffer& buffer;
            float fps;
        };

        struct ProtocolP
        {
            ProtocolP(const char* p)
                : protocol(p)
            {
            }

            bool operator()(const IPNode* n)
            {
                return n->protocol() == protocol;
            }

            const char* protocol;
        };

        //
        //  For use in conjuction with foreach_ip<>() when flushing caches
        //

        struct FlushFBs
        {
            FlushFBs(const FlushContext& c_)
                : c(c_)
            {
            }

            const FlushContext& c;

            void operator()(IPImageID* i)
            {
                c.cache.flush(i->id);
                //
                //  The above will flush frame related structures, but
                //  the actual fb remains in the Cache TrashCan until we
                //  do the below.
                //
                c.cache.TwkFB::Cache::flush(i->id);
            }
        };

        //
        //  see testEvaluate() below
        //

        struct TestEvaluationResult
        {
            TestEvaluationResult()
                : poorRandomAccessPerformance(false)
            {
            }

            bool poorRandomAccessPerformance;
        };

        //
        //  Constructors
        //

        IPNode(const std::string& name, const NodeDefinition* definition,
               IPGraph*, GroupIPNode* group = 0);

        virtual ~IPNode();

        //
        //  SIGNALS
        //
        //  External code should connect to these to get various info
        //  about events occuring on this node or the graph above it.
        //

        VoidSignal& willDeleteSignal() { return m_willDeleteSignal; }

        VoidSignal& inputsChangedSignal() { return m_inputsChangedSignal; }

        VoidSignal& outputsChangedSignal() { return m_outputsChangedSignal; }

        VoidSignal& graphStateChangedSignal()
        {
            return m_graphStateChangedSignal;
        }

        VoidSignal& imageStructureChangedSignal()
        {
            return m_imageStructureChangedSignal;
        }

        VoidSignal& mediaChangedSignal() { return m_mediaChangedSignal; }

        VoidSignal& rangeChangedSignal() { return m_rangeChangedSignal; }

        PropertySignal& propertyWillBeDeletedSignal()
        {
            return m_propertyWillBeDeletedSignal;
        }

        PropertyNameSignal& propertyDeletedSignal()
        {
            return m_propertyDeletedSignal;
        }

        PropertySignal& newPropertySignal() { return m_newPropertySignal; }

        PropertySignal& propertyChangedSignal()
        {
            return m_propertyChangedSignal;
        }

        PropertySignal& propertyWillChangeSignal()
        {
            return m_propertyWillChangeSignal;
        }

        PropertyInsertSignal& propertyWillInsertSignal()
        {
            return m_propertyWillInsertSignal;
        }

        PropertyInsertSignal& propertyDidInsertSignal()
        {
            return m_propertyDidInsertSignal;
        }

        //
        //  Node Name and type
        //

        const std::string uiName() const;

        //
        //  Definition object
        //

        const NodeDefinition* definition() const { return m_definition; }

        //
        //  Graph that owns the node
        //

        IPGraph* graph() { return m_graph; }

        const IPGraph* graph() const { return m_graph; }

        void setGraph(IPGraph* graph);

        //
        //  From PropertyContainer
        //

        virtual void copy(const PropertyContainer*);
        virtual void copyNode(const IPNode*);

        //
        //  Group --
        //

        GroupIPNode* group() const { return m_group; }

        void setGroup(GroupIPNode*);

        //
        //  Does this node produce audio or video ?  These only make sense for
        //  sources at the moment, but might apply to any new node type.
        //

        bool hasAudio() const { return m_hasAudio; }

        bool hasVideo() const { return m_hasVideo; }

        void setHasAudio(bool b) { m_hasAudio = b; }

        void setHasVideo(bool b) { m_hasVideo = b; }

        //
        //  An IPNode may do work in the background in order become fully
        //  constructed. E.g. read files in another thread. If a node is
        //  still "working" on another thread isReady() should return
        //  false.
        //

        virtual bool isReady() const;

        //
        //  Should this node be written to a session file. Make it writable
        //  with setWritable() function.
        //

        bool isWritable() const;

        void setWritable(bool b) { m_writable = b; }

        //
        //  Should be searched by metaEvaluate
        //

        bool isMetaSearchable() const { return m_metaSearchable; }

        void setMetaSearchable(bool b) { m_metaSearchable = b; }

        //
        //  Setting this allows this node to take any node as an input
        //  even if that node is not a member of the same group as this
        //  one. This is mostly to support the RootIPNode and nodes which
        //  inspect the output members of other groups.
        //
        //  Outputs are never tested.
        //

        void setUnconstrainedInputs(bool b) { m_unconstrainedInputs = b; }

        //
        //  Before deletion -- if this function is called the node is about to
        //  be deleted. In some cases a faster shutdown can occur if you know
        //  this.
        //

        virtual void willDelete();

        //
        //  Inputs/Outputs
        //
        //  setInputs() can be overriden to update internal state when
        //  inputs change. Throw on invalid input.
        //
        //  testInputs() should detect if the inputs would cause a failure
        //  in setInputs() and return false + a human readable
        //  explanation.
        //
        //  maximumNumberOfInputs() returns the result of
        //  setMaxInputs(). If the value == -1 then there are no
        //  restrictions on the number of allowed inputs.
        //

        const IPNodes& inputs() const { return m_inputs; }

        const IPNodes& outputs() const { return m_outputs; }

        virtual void setInputs(const IPNodes&);

        //
        //  These set *all* of the inputs to whatever the args are. You
        //  can't overload "setInput" with these for some obscure C++ reason.
        //

        void setInputs1(IPNode* node0);
        void setInputs2(IPNode* node0, IPNode* node1);
        void setInputs3(IPNode* node0, IPNode* node1, IPNode* node2);
        void setInputs4(IPNode* node0, IPNode* node1, IPNode* node2,
                        IPNode* node3);

        //
        //  Returns either 0 or the numbered input (first, second, etc)
        //

        IPNode* inputs1() const;
        IPNode* inputs2() const;
        IPNode* inputs3() const;
        IPNode* inputs4() const;

        virtual bool testInputs(const IPNodes&, std::ostringstream&) const;

        int maximumNumberOfInputs() const { return m_maxInputs; }

        int minimumNumberOfInputs() const { return m_minInputs; }

        int indexOfChild(IPNode*); // -1 if not there
        bool isInput(IPNode*) const;
        void removeInput(IPNode*);
        void appendInput(IPNode*);
        void insertInput(IPNode*, size_t index = 0);
        void disconnectInputs();
        void disconnectOutputs();
        void deleteInputs(); // NOTE: actually deletes them not just setting to
                             // nothing

        enum InputComparisonResult
        {
            IdenticalResult,
            AppendedResult,
            ReorderedResult,
            AddedResult,
            DiffersResult
        };

        InputComparisonResult compareToInputs(const IPNodes&) const;

        //
        //  fills the IndexArray with the new ordering based on the current
        //  inputs. The IPNodes should produce IdenticalResult if passed to
        //  compareToInputs above.
        //
        //  reorder() will change the input ording according to indices
        //

        void inputReordering(const IPNodes& newOrder,
                             IndexArray& indices) const;
        void reorder(const IndexArray& indices);

        void inputPartialReordering(
            const IPNodes& newInputs,
            IndexArray& removeIndices,         // inputs to be removed
            IndexArray& reorderIndices) const; // size same as newInputs

        void findInputs(const IndexArray& indices, IPNodes& nodes) const;

        //
        //  Information on this and upstream images.
        //
        //  NOTE: the width and height here are probably the uncropped
        //  width and height. Its not clear that information about the
        //  data window are needed by functions that get the
        //  ImageStructureInfo. In all likelyhood, they are concerned with
        //  the "logical" image structure.
        //
        //  The orientation matrix is 4x4, but should really be a 2x2
        //  scale matrix in practice.
        //

        struct ImageStructureInfo
        {
            ImageStructureInfo(int w = 0, int h = 0, float pa = 1.0,
                               const Matrix& O = Matrix())
                : width(w)
                , height(h)
                , pixelAspect(pa)
                , orientation(O)
            {
            }

            int width;
            int height;
            float pixelAspect;
            Matrix orientation;
        };

        struct ImageRangeInfo
        {
            ImageRangeInfo(int s = 0, int e = 0, int i = 0, float f = 0,
                           int ci = 0, int co = 0, int b = false)
                : start(s)
                , end(e)
                , inc(i)
                , fps(f)
                , cutIn(ci)
                , cutOut(co)
                , isUndiscovered(b)
            {
            }

            int start; //  FIRST frame
            int end;   //  LAST frame, _not_  LAST frame + 1
            int inc;
            float fps;
            int cutIn;
            int cutOut;
            bool isUndiscovered;
        };

        struct MediaInfo
        {
            MediaInfo(const std::string& n = "",
                      const TwkMovie::MovieInfo* i = 0, IPNode* nd = NULL)
                : name(n)
                , info(i)
                , node(nd)
            {
            }

            MediaInfo(const std::string& n, const TwkMovie::MovieInfo& i,
                      IPNode* nd)
                : name(n)
                , info(&i)
                , node(nd)
            {
            }

            std::string name;
            const TwkMovie::MovieInfo* info;
            IPNode* node;
        };

        typedef std::vector<MediaInfo> MediaInfoVector;

        //
        //  Information about timing
        //

        virtual ImageRangeInfo imageRangeInfo() const;

        //
        //  Information about image structure.
        //

        virtual ImageStructureInfo imageStructureInfo(const Context&) const;

        //
        //  Information about upstream media
        //

        virtual void mediaInfo(const Context&, MediaInfoVector&) const;
        virtual bool isMediaActive() const;
        virtual void setMediaActive(bool state);

        //
        //  This will call IPGraph to build a "default" context the same
        //  way it does. (Mostly here to avoid IPGraph.h being included)
        //

        Context contextForFrame(int frame);

        //
        //  Evaluate at frame into a FrameBuffer. The IPImage returned is
        //  expected to be a linked list of IPImages with possible planes
        //  for each one.
        //

        virtual IPImage* evaluate(const Context&);

        //
        //  Get the identifier strings as if evaluation were being run.
        //  Each entry in the IDTree will have the ids of all FrameBuffers
        //  that could potentially be cached for each image. The IDTree
        //  topology will be identical to the IPImage topology
        //

        virtual IPImageID* evaluateIdentifier(const Context&);

        //
        //  If the node applies transforms hasTransforms() should return
        //  true. Incorrectly reporting the transform will result in a big
        //  mess. setHasLinearTransform() should be called by the IPNode
        //  constructor but can be called later if required.
        //
        //  localMatrix() returns the linear transforms associated with
        //  the node in the given evaluation context. NOTE:
        //  localMatrix()'s return value can vary with the passed in
        //  Context. This could be the frame or any other member of
        //  Context.
        //

        bool hasLinearTransforms() const { return m_hasLinearTransforms; }

        virtual Matrix localMatrix(const Context&) const;

        //
        //  Transform the frame(s) at input index to output. E.g. if
        //  retiming is occuring in this node then the supplied frame
        //  number and input should be inverted to give the global frame
        //  number that would call it. if its not invertable give the
        //  *first* such frame.
        //
        //  The default is to append inframes to outframes
        //

        virtual void mapInputToEvalFrames(size_t inputIndex,
                                          const FrameVector& inframes,
                                          FrameVector& outframes) const;

        //
        //  Find nodes by predicate function in evaluation
        //  nodes
        //

        template <typename Predicate>
        void findInEvaluationPath(int frame, Predicate, IPNodes&,
                                  bool ascendingOrder = true);

        //
        //  Meta evaluation returns the nodes which would be evaluated at
        //  frame in either ascending or descending order. No evaluation
        //  occurs.  You can create a subclass of MetaEvalFilter to find
        //  specific nodes or make special evaluation paths. E.g., find
        //  all nodes of a certain type on all branches but never two that
        //  live in the same evaluation path.
        //

        class MetaEvalVisitor
        {
        public:
            virtual void enter(const Context&, IPNode*) {}

            virtual void leave(const Context&, IPNode*) {}

            virtual bool traverseChild(const Context&, size_t index, IPNode*,
                                       IPNode*)
            {
                return true;
            }
        };

        struct MetaEvalInfo
        {
            MetaEvalInfo(int f, IPNode* n)
                : sourceFrame(f)
                , node(n)
            {
            }

            MetaEvalInfo()
                : sourceFrame(0)
                , node(0)
            {
            }

            bool operator<(const MetaEvalInfo& i) const
            {
                return (node < i.node
                        || (node == i.node && sourceFrame < i.sourceFrame));
            }

            int sourceFrame;
            IPNode* node;
        };

        typedef std::vector<MetaEvalInfo> MetaEvalInfoVector;

        class MetaEvalInfoCollector : public MetaEvalVisitor
        {
        public:
            MetaEvalInfoCollector(MetaEvalInfoVector& i)
                : info(i)
            {
            }

            virtual void enter(const Context& c, IPNode* node)
            {
                info.push_back(MetaEvalInfo(c.frame, node));
            }

            MetaEvalInfoVector& info;
        };

        template <class T>
        class MetaEvalInfoCollectorByType : public MetaEvalVisitor
        {
        public:
            MetaEvalInfoCollectorByType(MetaEvalInfoVector& i)
                : info(i)
            {
            }

            MetaEvalInfoVector& info;

            virtual void enter(const Context& c, IPNode* n)
            {
                if (dynamic_cast<T*>(n))
                    info.push_back(MetaEvalInfo(c.frame, n));
            }
        };

        class MetaEvalPath : public MetaEvalVisitor
        {
        public:
            MetaEvalPath(MetaEvalInfoVector& i, IPNode* l)
                : info(i)
                , leaf(l)
                , found(false)
            {
            }

            virtual void enter(const Context&, IPNode* n);
            virtual void leave(const Context&, IPNode* n);
            virtual bool traverseChild(const Context&, size_t index,
                                       IPNode* parent, IPNode* child);

            MetaEvalInfoVector& info;
            IPNode* leaf;
            bool found;
        };

        class MetaEvalClosestByTypeName : public MetaEvalVisitor
        {
        public:
            MetaEvalClosestByTypeName(MetaEvalInfoVector& i,
                                      const std::string& type)
                : info(i)
                , typeName(type)
            {
            }

            virtual void enter(const Context&, IPNode* n);
            virtual bool traverseChild(const Context&, size_t index,
                                       IPNode* parent, IPNode* child);

            MetaEvalInfoVector& info;
            std::string typeName;
        };

        class MetaEvalFirstClosestByTypeName : public MetaEvalVisitor
        {
        public:
            MetaEvalFirstClosestByTypeName(MetaEvalInfoVector& i,
                                           const std::string& type)
                : info(i)
                , typeName(type)
            {
            }

            virtual void enter(const Context&, IPNode* n);
            virtual bool traverseChild(const Context&, size_t index,
                                       IPNode* parent, IPNode* child);

            MetaEvalInfoVector& info;
            std::string typeName;
        };

        //
        //  Metaevaluate probs the structure of the graph. You only need
        //  to override it if e.g. your node conditionally evaluates
        //  inputs at a frame.
        //

        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);

        //
        //  Using visitRecursive you can visit each node using NodeVisitor
        //

        class NodeVisitor
        {
        public:
            virtual void enter(IPNode*) {}

            virtual void leave(IPNode*) {}

            virtual bool traverseChild(size_t index, IPNode*, IPNode*)
            {
                return true;
            }
        };

        typedef std::map<const IPNode*, FrameVector> NodeFramesMap;

        class PropertyAsFramesVisitor : public NodeVisitor
        {
        public:
            PropertyAsFramesVisitor(const std::string& propfullname,
                                    size_t depth_)
                : propName(propfullname)
                , depth(depth_)
                , currentDepth(0)
            {
            }

            std::string propName;
            size_t depth;
            NodeFramesMap nodeMap;

            const FrameVector& result(const IPNode* n) { return nodeMap[n]; }

            virtual bool traverseChild(size_t index, IPNode* parent,
                                       IPNode* child);
            virtual void leave(IPNode* n);
            virtual void enter(IPNode* n);

        private:
            int currentDepth;
        };

        //
        //  This is similar to the MetaEvalClosestByTypeName except it
        //  searches only by topology not in evaluation order
        //

        class ClosestByTypeNameVisitor : public NodeVisitor
        {
        public:
            ClosestByTypeNameVisitor(const std::string& typeName,
                                     size_t depth = 1)
                : name(typeName)
                , currentDepth(0)
                , maxDepth(depth)
            {
            }

            std::string name;
            std::set<IPNode*> nodes;
            size_t maxDepth;

            virtual bool traverseChild(size_t index, IPNode* parent,
                                       IPNode* child);
            virtual void leave(IPNode* n);
            virtual void enter(IPNode* n);

        private:
            int currentDepth;
        };

        virtual void visitRecursive(NodeVisitor&);

        //
        //  Test evaluation returns information which may aid in
        //  evaluation. For example if multiple threads are a detriment,
        //  test evaluation will indicate that.
        //

        virtual void testEvaluate(const Context&, TestEvaluationResult&);

        //
        //  This function should flush any caching occuring per node
        //  The propagate functions call flushAllCaches() either up
        //  or down the node tree.
        //

        virtual void flushAllCaches(const FlushContext&);

        virtual void propagateFlushToInputs(const FlushContext&);
        virtual void propagateFlushToOutputs(const FlushContext&);

        //
        //  Inputs changing upstream should call this to propagate that back to
        //  to the root.

        virtual void propagateInputChange();

        //
        //  If a node changes state, propagateStateChange() should be
        //  called to propagate that fact back to the root.
        //

        virtual void propagateStateChange();

        //
        //  Same idea as above, but indicates that this nodes time range
        //  has changed.
        //

        virtual void
        propagateRangeChange(PropagateTarget target = LegacyPropagateTarget);

        //
        //  Same idea as above, but indicates that this nodes image struct
        //  has changed.
        //

        virtual void propagateImageStructureChange(
            PropagateTarget target = LegacyPropagateTarget);

        //
        //  Same idea as above, but indicates that this nodes upstream
        //  media has changed.
        //

        virtual void
        propagateMediaChange(PropagateTarget target = LegacyPropagateTarget);

        //
        //  The audio analog to evaluate
        //

        virtual size_t audioFillBuffer(const AudioContext&);

        //
        //  Recursively calls audioConfigure() right before audio will
        //  play. Progagates information about the audio renderer up the node
        //  tree
        //

        struct AudioConfiguration
        {
            AudioConfiguration()
                : layout(TwkAudio::UnknownLayout)
                , rate(0.0)
                , samples(0)
            {
            }

            AudioConfiguration(double deviceRate, TwkAudio::Layout layout,
                               size_t numSamples)
                : rate(deviceRate)
                , layout(layout)
                , samples(numSamples)
            {
            }

            bool operator==(const AudioConfiguration& b) const
            {
                return layout == b.layout && rate == b.rate
                       && samples == b.samples;
            }

            TwkAudio::Layout layout;
            double rate;
            size_t samples;
        };

        //
        //  NOTE: you should not supply any other arg here other than the
        //  one originally passed in by IPGraph.
        //

        virtual void propagateAudioConfigToInputs(const AudioConfiguration&);

        //
        //  Recursively call graphConfigure() with new values.
        //

        struct GraphConfiguration
        {
            GraphConfiguration()
                : minFrame(0)
                , maxFrame(0)
                , inFrame(0)
                , outFrame(0)
                , fps(0.0)
            {
            }

            GraphConfiguration(int min, int max, int in, int out, float f)
                : minFrame(min)
                , maxFrame(max)
                , inFrame(in)
                , outFrame(out)
                , fps(f)
            {
            }

            int minFrame;
            int maxFrame;
            int inFrame;
            int outFrame;
            float fps;
        };

        //
        //  NOTE: you should not supply any other arg here other than the
        //  one originally passed in by IPGraph.
        //

        virtual void propagateGraphConfigToInputs(const GraphConfiguration&);

        //
        //  Collect all inputs -- used before deletion. The nodes will be
        //  ordered starting with the leaves and working towards the root. So
        //  its possible (but not necessary) to delete them in that order if
        //  you want a "clean" graph change.
        //

        void collectInputs(IPNodes&);

        //
        //  Called just prior to writing
        //  and just after writing
        //

        virtual void prepareForWrite();
        virtual void writeCompleted();

        //
        //  Called after reading but before first eval calls occur. The
        //  passed in type and version are from the object that held the
        //  node state in the file -- so it could differ from the name and
        //  version that are instantiated by the application (e.g. older
        //  version file or renamed type)
        //
        //  NOTE: this is *also* called when reading a profile (sub-graph)
        //  which is applied to a group node. The reading procedure is
        //  basically the same as a full session file read; notification
        //  is avoided (propertyChanged()) for similar reasons.
        //

        virtual void readCompleted(const std::string& type,
                                   unsigned int version);

        //
        //  Notification of property changes
        //

        virtual void propertyWillBeDeleted(const Property*);
        virtual void propertyDeleted(const std::string&);
        virtual void newPropertyCreated(const Property*);
        virtual void propertyChanged(const Property*);
        virtual void propertyWillChange(const Property*);
        virtual void propertyWillInsert(const Property*, size_t index,
                                        size_t size);
        virtual void propertyDidInsert(const Property*, size_t index,
                                       size_t size);

        virtual void propagateInputChangeInternal();
        virtual void propagateStateChangeInternal();
        virtual void propagateRangeChangeInternal(
            PropagateTarget target = LegacyPropagateTarget);
        virtual void propagateImageStructureChangeInternal(
            PropagateTarget target = LegacyPropagateTarget);
        virtual void propagateMediaChangeInternal(
            PropagateTarget target = LegacyPropagateTarget);

        virtual void collectMemberNodes(IPNodeSet&, size_t depth = 0);

        //
        //  For undo/redo
        //

        virtual void isolate();
        virtual void restore();
        bool isIsolated() const;

        void undoRef() { m_undoRefCount++; }

        void undoDeref();

    protected:
        void disconnectInputsAtomic();

        //
        //  Derived classes can call this to let the base class handle
        //  setInputs() for limited input nodes. testInputs() will throw
        //  if max inputs is exceeded. Since any node can have zero inputs
        //  (while editing the graph) it will not throw in the case of min
        //  inputs, but during eval it may return a NoImage. The default
        //  min inputs is 1
        //

        void setMaxInputs(int n) { m_maxInputs = n; }

        void setMinInputs(int n) { m_minInputs = n; }

        //
        //  Indicate whether or not this node applies transforms to the
        //  output images. The node should not apply them directly, only
        //  return its transform from transform()
        //

        void setHasLinearTransform(bool b) { m_hasLinearTransforms = b; }

        //
        //  Called to (possibly) reconfigure audio
        //

        virtual void audioConfigure(const AudioConfiguration&);

        //
        //  Called to reconfigure graph range, etc. If range is not
        //  interesting you can use the graph() function to get the
        //  IPGraph object and get data specifically. The
        //  GraphConfiguration is there to limit locks on the fbcache for
        //  commonly desired data.
        //

        virtual void graphConfigure(const GraphConfiguration&);

        //
        //  Called when an upstream node signals that its inputs have
        //  changed. Note that the input that changed could be anywhere up the
        //  graph.
        //

        virtual void inputChanged(int inputIndex);

        //
        //  Called when an upstream node signals that its state (property)
        //  has changed
        //

        virtual void inputStateChanged(int inputIndex);

        //
        //  Called when an upstream node signals that its ImageRangeInfo
        //  has changed.
        //

        virtual void inputRangeChanged(int inputIndex, PropagateTarget target);

        //
        //  Called when an upstream node signals that its
        //  ImageStructureInfo has changed.
        //

        virtual void inputImageStructureChanged(int inputIndex,
                                                PropagateTarget target);

        //
        //  Called when an upstream node changes its media
        //

        virtual void inputMediaChanged(IPNode* srcNode, int srcOutputIndex,
                                       PropagateTarget target);

        //
        //  Called when output disconnection is about to occur
        //

        virtual void outputDisconnect(IPNode*);

        //

        virtual void addOutput(IPNode*);

        bool testInputsInternal(const IPNode*) const;

        bool isDeleting() const { return m_deleting; }

        static IPImage::ResourceUsage accumulate(const IPImageVector&);
        static IPImage::ResourceUsage filterAccumulate(const IPImageVector&);

        typedef IPImage::ResourceUsage (*UsageFunction)(const IPImageVector&);

        static bool willConvertToIntermediate(IPImage*);
        static bool convertBlendRenderTypeToIntermediate(IPImage*);
        static bool
        convertBlendRenderTypeToIntermediate(const IPImageVector& inImages,
                                             IPImageSet& modifiedImages);

        static std::string getUnqualifiedName(const std::string& name);

        void assembleMergeExpressions(IPImage* root, IPImageVector& inImages,
                                      const IPImageSet& imagesNotToBeModified,
                                      bool isFilter,
                                      Shader::ExpressionVector& exprs);

        void balanceResourceUsage(UsageFunction accumFunc,
                                  IPImageVector& inImages,
                                  IPImageSet& modifiedImages, size_t maxBuffers,
                                  size_t maxCoords, size_t maxFetches,
                                  size_t incomingSamplers = 0) const;

        IPImage*
        insertIntermediateRendersForPaint(IPImage* root,
                                          const Context& context) const;

        static void filterLimits(const IPImageVector& inImages,
                                 IPImageSet& modifiedImages);

        /** maps a source node to an input
         * @param srcNode the source node
         * @param srcOutIndex the output index of the source index
         * @returns the index of the current node. -1 if it is not found
         */
        int mapToInputIndex(IPNode* srcNode, int srcOutIndex) const;

        struct CoverageInfo
        {
            Vec2 points[4];
            float width;
            float height;
        };

    private:
        const NodeDefinition* m_definition;
        IPGraph* m_graph;
        IPNodes m_inputs;
        IPNodes m_outputs;
        GroupIPNode* m_group;
        int m_maxInputs;
        int m_minInputs;
        VoidSignal m_willDeleteSignal;
        VoidSignal m_inputsChangedSignal;
        VoidSignal m_outputsChangedSignal;
        VoidSignal m_graphStateChangedSignal;
        VoidSignal m_imageStructureChangedSignal;
        VoidSignal m_mediaChangedSignal;
        VoidSignal m_rangeChangedSignal;
        PropertySignal m_propertyWillBeDeletedSignal;
        PropertyNameSignal m_propertyDeletedSignal;
        PropertySignal m_newPropertySignal;
        PropertySignal m_propertyChangedSignal;
        PropertySignal m_propertyWillChangeSignal;
        PropertyInsertSignal m_propertyWillInsertSignal;
        PropertyInsertSignal m_propertyDidInsertSignal;
        size_t m_undoRefCount;
        bool m_deleting : 1;
        bool m_writable : 1;
        bool m_unconstrainedInputs : 1;
        bool m_hasLinearTransforms : 1;
        bool m_metaSearchable : 1;
        bool m_hasVideo : 1;
        bool m_hasAudio : 1;
    };

    template <typename Predicate>
    void IPNode::findInEvaluationPath(int frame, Predicate P, IPNodes& nodes,
                                      bool ascendingOrder)
    {
        MetaEvalInfoVector infos;
        MetaEvalInfoCollector collector(infos);
        metaEvaluate(contextForFrame(frame), collector);

        for (int i = 0; i < infos.size(); i++)
        {
            if (P(infos[i].node))
                nodes.push_back(infos[i].node);
        }
    }

    //
    //  A generic constructor function
    //

    template <class T>
    IPNode* newIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph* graph, GroupIPNode* group)
    {
        return new T(name, def, graph, group);
    }

    std::ostream& operator<<(std::ostream& o, const IPNode::ImageComponent& c);

} // namespace IPCore

#endif // __IPCore__IPNode__h__
