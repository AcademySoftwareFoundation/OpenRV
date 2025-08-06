//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
//  This file contains the Mu commands that operate on IPCore and
//  IPBaseNodes libraries. Other CommandModules exist for example jjjjj
//
#include <IPMu/CommandsModule.h>

#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPBaseNodes/SequenceIPNode.h>
#include <IPBaseNodes/SourceGroupIPNode.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/FBCache.h>
#include <IPCore/IPProperty.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/NodeManager.h>
#include <IPCore/Profile.h>
#include <IPCore/PropertyEditor.h>
#include <IPCore/Session.h>
#include <IPCore/RenderQuery.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/StructType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/HalfType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuSystem/SystemModule.h>
#include <MuTwkApp/EventType.h>
#include <MuTwkApp/MuInterface.h>
#include <TwkApp/Command.h>
#include <TwkApp/Document.h>
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkFB/IO.h>
#include <TwkFB/Operations.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/Timer.h>
#include <TwkDeploy/Deploy.h>

#include <IPMu/RemoteRvCommand.h>

#include <algorithm>
#include <limits>
#include <fstream>
#include <half.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stl_ext/string_algo.h>
#include <unordered_set>

namespace IPMu
{
    using namespace TwkApp;
    using namespace TwkAudio;
    using namespace TwkMath;
    using namespace TwkMovie;
    using namespace Mu;
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkUtil;
    using namespace IPCore;

    typedef Session::PropertyVector PropertyVector;
    typedef TwkApp::EventType::EventInstance Event;

    //----------------------------------------------------------------------

    static void throwBadArgumentException(const Mu::Node& node,
                                          Mu::Thread& thread,
                                          const Mu::String& msg)
    {
        const Mu::MuLangContext* context =
            static_cast<const Mu::MuLangContext*>(thread.context());
        ExceptionType::Exception* e =
            new ExceptionType::Exception(context->exceptionType());

        ostringstream str;

        if (context->debugging())
        {
            const Mu::AnnotatedNode& anode =
                static_cast<const Mu::AnnotatedNode&>(node);
            // When linenum is 0, it indicates that sourceFileName is either
            // empty or not null-terminated. This typically occurs when an
            // exception is raised from a Python plugin. Despite populating the
            // exception object with relevant information, the text does not
            // display in the console for such cases.
            if (anode.linenum() > 0)
            {
                str << anode.sourceFileName() << ", line " << anode.linenum()
                    << ", char " << anode.charnum() << ": ";
            }
        }

        str << "in " << node.symbol()->fullyQualifiedName() << ": " << msg;
        e->string() = str.str().c_str();
        ProgramException exc(thread, e);
        thread.setException(e);
        e->setBackTrace(exc);
        throw exc;
    }

    static void throwBadProperty(Thread& thread, const Mu::Node& node,
                                 const Mu::String& name)
    {
        Process* process = thread.process();
        MuLangContext* context =
            static_cast<MuLangContext*>(process->context());
        ExceptionType::Exception* e =
            new ExceptionType::Exception(context->exceptionType());

        ostringstream str;

        if (context->debugging())
        {
            const Mu::AnnotatedNode& anode =
                static_cast<const Mu::AnnotatedNode&>(node);
            if (anode.linenum() > 0)
            {
                str << anode.sourceFileName() << ", line " << anode.linenum()
                    << ", char " << anode.charnum() << ": ";
            }
        }

        str << "invalid property name " << name.c_str();

        e->string() = str.str().c_str();
        thread.setException(e);
        ProgramException exc(thread, e);
        e->setBackTrace(exc);
        throw exc;
    }

    static void throwBadPropertyType(Thread& thread, const Mu::Node& node,
                                     const Mu::String& name)
    {
        Process* process = thread.process();
        MuLangContext* context =
            static_cast<MuLangContext*>(process->context());
        ExceptionType::Exception* e =
            new ExceptionType::Exception(context->exceptionType());

        ostringstream str;

        if (context->debugging())
        {
            const Mu::AnnotatedNode& anode =
                static_cast<const Mu::AnnotatedNode&>(node);
            if (anode.linenum() > 0)
            {
                str << anode.sourceFileName() << ", line " << anode.linenum()
                    << ", char " << anode.charnum() << ": ";
            }
        }

        str << "wrong property type " << name.c_str();

        e->string() = str.str().c_str();
        thread.setException(e);
        ProgramException exc(thread, e);
        e->setBackTrace(exc);
        throw exc;
    }

    static void getProperty(Session::PropertyVector& props, Mu::Thread& thread,
                            const Mu::Node& node,
                            const Mu::StringType::String* name)
    {
        if (!name)
            throwBadArgumentException(node, thread, "node name is nil");

        Session* session = Session::currentSession();
        session->findProperty(props, name->c_str());
        if (props.empty())
            throwBadProperty(thread, node, name->c_str());
    }

    // Helper function to detect if an image is actually a tile in a
    // tiled-source. For safety, it first checks if source-tiling is enabled,
    // then it checks if the image has the tile token in its name (eg.:
    // sourceGroup000000_source/tile_x0_y1.0//1
    // If the image is a tile, it truncates the sourceName so it does not
    // include the tile name (eg.: sourceGroup000000_source).
    static std::tuple<std::string, bool>
    detectTile(const std::string& sourceName)
    {
        std::string tileName = "";
        bool sourceIsATile = false;

        static bool srcTilingOn =
            getenv("RV_SOURCE_TILING"); // FIXME_DESRUIE : detect tiles!

        if (srcTilingOn)
        {
            const size_t tileToken = sourceName.find("/tile_x");
            if (tileToken != std::string::npos)
            {
                sourceIsATile = true;
                tileName = sourceName.substr(0, tileToken);
            }
        }

        return std::make_tuple(tileName, sourceIsATile);
    }

    // Helper function to adjust tile coordinates so that they take into account
    // the frame-ratio and to make sure they are in the right range.
    // Note that y-values are in a 1-normalized range, whereas x-values are
    // normalized in a range that is based on the frame-ratio.
    void adjustTileCoords(const std::string& sourceName, Vector2f& mp,
                          const RenderQuery& renderQuery,
                          const bool inImageSpace)
    {
        double frameRatio(1.0);

        renderQuery.imageFrameRatio(sourceName, frameRatio);

        if (inImageSpace)
        {
            // normalize x value
            mp[0] /= frameRatio;

            // go from a [-0.5..0.5] range to a [0..1] range.
            mp[0] += 0.5;
            mp[1] += 0.5;

            // denormalize x value
            mp[0] *= frameRatio;
        }
        else
        {
            mp[0] -= (0.5 * frameRatio);
            mp[1] -= 0.5;
        }
    }

    //----------------------------------------------------------------------

    NODE_IMPLEMENTATION(setSessionName, void)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "setSessionName: nil node name");

        if (Session* s = Session::currentSession())
        {
            s->setName(name->c_str());
        }
    }

    NODE_IMPLEMENTATION(sessionName, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        StringType::String* name = 0;

        if (Session* s = Session::currentSession())
        {
            name = stype->allocate(s->name());
        }

        NODE_RETURN(name);
    }

    NODE_IMPLEMENTATION(sessionNames, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        DynamicArrayType* atype = (DynamicArrayType*)NODE_THIS.type();

        const IPCore::Application::Documents& docs = IPCore::App()->documents();
        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(docs.size());

        for (int i = 0; i < docs.size(); i++)
        {
            Session* s = (Session*)docs[i];
            array->element<StringType::String*>(i) = stype->allocate(s->name());
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(setFrame, void)
    {
        int n = NODE_ARG(0, int);

        if (Session* s = Session::currentSession())
        {
            s->setFrame(n);
            s->askForRedraw(true);
        }
    }

    NODE_IMPLEMENTATION(redraw, void)
    {
        if (Session* s = Session::currentSession())
        {
            s->askForRedraw();
        }
    }

    NODE_IMPLEMENTATION(inPoint, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->inPoint());
    }

    NODE_IMPLEMENTATION(outPoint, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->outPoint() - 1);
    }

    NODE_IMPLEMENTATION(setOutPoint, void)
    {
        int f = NODE_ARG(0, int);
        Session* s = Session::currentSession();
        int newOut = f + 1, oldIn = s->inPoint();
        {
            IPGraph::GraphEdit edit(s->graph());
            if (newOut < oldIn)
                s->setInPoint(newOut - (s->outPoint() - oldIn));
            s->setOutPoint(newOut);
        }
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(setInPoint, void)
    {
        int f = NODE_ARG(0, int);
        Session* s = Session::currentSession();
        int oldOut = s->outPoint();
        {
            IPGraph::GraphEdit edit(s->graph());
            if (f > oldOut)
                s->setOutPoint(f + (oldOut - s->inPoint()));
            s->setInPoint(f);
        }
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(setMargins, void)
    {
        Vector4f m = NODE_ARG(0, Vector4f);
        const bool allDevices = NODE_ARG(1, bool);
        Session* s = Session::currentSession();
        s->setMargins(m[0], m[1], m[2], m[3], allDevices);
        s->askForRedraw(true /*force*/);
    }

    NODE_IMPLEMENTATION(margins, Mu::Vector4f)
    {
        Session* s = Session::currentSession();
        const VideoDevice* d = s->eventVideoDevice() ? s->eventVideoDevice()
                                                     : s->outputVideoDevice();
        TwkApp::VideoDevice::Margins m = d->margins();

        Vector4f ret;
        ret[0] = m.left;
        ret[1] = m.right;
        ret[2] = m.top;
        ret[3] = m.bottom;

        NODE_RETURN(ret);
    }

    NODE_IMPLEMENTATION(setPlayMode, void)
    {
        int m = NODE_ARG(0, int);
        Session* s = Session::currentSession();

        switch (m)
        {
        case 0:
            s->setPlayMode(Session::PlayLoop);
            break;
        case 1:
            s->setPlayMode(Session::PlayOnce);
            break;
        case 2:
            s->setPlayMode(Session::PlayPingPong);
            break;
        }
    }

    NODE_IMPLEMENTATION(playMode, int)
    {
        Session* s = Session::currentSession();
        int rval = 0;

        switch (s->playMode())
        {
        case Session::PlayLoop:
            rval = 0;
            break;
        case Session::PlayOnce:
            rval = 1;
            break;
        case Session::PlayPingPong:
            rval = 2;
            break;
        }

        NODE_RETURN(rval);
    }

    NODE_IMPLEMENTATION(clearAllButFrame, void)
    {
        int f = NODE_ARG(0, int);
        Session* s = Session::currentSession();

        s->graph().cache().lock();
        s->graph().cache().clearAllButFrame(f);
        s->graph().cache().unlock();
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(reload, void)
    {
        Session* s = Session::currentSession();
        Session::NodeVector nodes;

        s->findCurrentNodesByTypeName(nodes, "RVFileSource");

        for (int i = 0; i < nodes.size(); ++i)
        {
            if (FileSourceIPNode* fs =
                    dynamic_cast<FileSourceIPNode*>(nodes[i]))
            {
                fs->invalidateFileSystemInfo();
            }
        }

        s->reload(s->currentFrame(), s->currentFrame());
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(reloadRange, void)
    {
        int start = NODE_ARG(0, int);
        int end = NODE_ARG(1, int);
        Session* s = Session::currentSession();
        Session::NodeVector nodes;

        s->findNodesByTypeName(nodes, "RVFileSource");

        for (int i = 0; i < nodes.size(); ++i)
        {
            if (FileSourceIPNode* fs =
                    dynamic_cast<FileSourceIPNode*>(nodes[i]))
            {
                fs->invalidateFileSystemInfo();
            }
        }

        s->reload(start, end);
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(loadChangedFrames, void)
    {
        Session* s = Session::currentSession();
        DynamicArray* inputs = NODE_ARG_OBJECT(0, DynamicArray);

        //  Turn off cache

        Session::CachingMode mode = s->cachingMode();
        s->setCaching(Session::NeverCache);

        //  Clear frame cache.  This causes no deletion of frame buffers.

        s->graph().cache().lock();
        s->graph().cache().clear();
        s->graph().cache().unlock();

        //  Invalidate filesystem data for given  sources.  This will pick up
        //  newly-created files and produce new identifiers for changed files.

        Session::NodeVector nodes;

        for (size_t i = 0; i < inputs->size(); i++)
        {
            StringType::String* n = inputs->element<StringType::String*>(i);

            if (!n)
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "nil sourceName");

            if (IPNode* node = s->graph().findNode(n->c_str()))
            {
                nodes.push_back(node);
            }
            else
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "sourceName does not exist");
        }

        for (int i = 0; i < nodes.size(); ++i)
        {
            if (FileSourceIPNode* fs =
                    dynamic_cast<FileSourceIPNode*>(nodes[i]))
            {
                fs->invalidateFileSystemInfo();
            }
            else
                throwBadArgumentException(
                    NODE_THIS, NODE_THREAD,
                    "given sourceName is not an RVFileSource");
        }

        //  If we had caching on, turn it back on here.  At this point any
        //  newly-required framebuffers for frames that were previously cached
        //  will be cached again.

        s->setCaching(mode);
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(play, void)
    {
        Session* s = Session::currentSession();
        s->play();
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(isPlaying, bool)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(bool(s->isPlaying()));
    }

    NODE_IMPLEMENTATION(stop, void)
    {
        Session* s = Session::currentSession();
        s->stop();
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(frame, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->currentFrame());
    }

    NODE_IMPLEMENTATION(metaEvaluate, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const int frame = NODE_ARG(0, int);
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        const Class* itype = static_cast<const Class*>(dtype->elementType());
        StringType::String* root = NODE_ARG_OBJECT(1, StringType::String);
        StringType::String* leaf = NODE_ARG_OBJECT(2, StringType::String);
        const bool unique = NODE_ARG(3, bool);

        IPNode::MetaEvalInfoVector infos;
        IPNode* rootNode = s->graph().viewNode();

        if (root)
        {
            if (!(rootNode = s->graph().findNode(root->c_str())))
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "Bad node name");
            }
        }

        if (leaf)
        {
            if (IPNode* leafNode = s->graph().findNode(leaf->c_str()))
            {
                IPNode::MetaEvalPath pathfinder(infos, leafNode);
                s->graph().root()->metaEvaluate(
                    s->graph().contextForFrame(frame), pathfinder);
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "Bad node name");
            }
        }
        else
        {
            if (!rootNode)
                rootNode = s->graph().root();
            if (rootNode)
            {
                IPNode::MetaEvalInfoCollector collector(infos);
                rootNode->metaEvaluate(s->graph().contextForFrame(frame),
                                       collector);
            }
        }

        struct Info
        {
            Pointer nodeName;
            Pointer nodeType;
            int frame;
        };

        if (infos.size() > 0)
        {
            IPNode::MetaEvalInfoVector& infosRef = infos;
            IPNode::MetaEvalInfoVector newInfos;

            if (unique)
            {
                //
                //  Construct new list of unique Infos, but with order
                //  preserved.
                //

                set<IPNode::MetaEvalInfo> usedInfos;

                for (int i = 0; i < infos.size(); ++i)
                {
                    if (usedInfos.count(infos[i]) == 0)
                    {
                        newInfos.push_back(infos[i]);
                        usedInfos.insert(infos[i]);
                    }
                }
                infosRef = newInfos;
            }

            array->resize(infosRef.size());

            for (int i = 0; i < infosRef.size(); i++)
            {
                ClassInstance* obj = ClassInstance::allocate(itype);
                array->element<ClassInstance*>(i) = obj;
                Info* info = reinterpret_cast<Info*>(obj->structure());

                info->nodeName =
                    c->stringType()->allocate(infosRef[i].node->name());
                info->nodeType =
                    c->stringType()->allocate(infosRef[i].node->protocol());
                info->frame = infosRef[i].sourceFrame;
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(metaEvaluate2, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const int frame = NODE_ARG(0, int);
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        const Class* itype = static_cast<const Class*>(dtype->elementType());
        StringType::String* typeName = NODE_ARG_OBJECT(1, StringType::String);
        StringType::String* root = NODE_ARG_OBJECT(2, StringType::String);
        const bool unique = NODE_ARG(3, bool);

        IPNode* rootNode = s->graph().root();

        if (root)
        {
            if (!(rootNode = s->graph().findNode(root->c_str())))
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "Bad node name");
            }
        }

        if (!typeName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil type name");

        IPNode::MetaEvalInfoVector infos;
        IPNode::MetaEvalClosestByTypeName collector(infos, typeName->c_str());
        rootNode->metaEvaluate(s->graph().contextForFrame(frame), collector);

        struct Info
        {
            Pointer nodeName;
            Pointer nodeType;
            int frame;
        };

        if (infos.size() > 0)
        {
            IPNode::MetaEvalInfoVector& infosRef = infos;
            IPNode::MetaEvalInfoVector newInfos;

            if (unique)
            {
                //
                //  Construct new list of unique Infos, but with order
                //  preserved.
                //

                set<IPNode::MetaEvalInfo> usedInfos;

                for (int i = 0; i < infos.size(); ++i)
                {
                    if (usedInfos.count(infos[i]) == 0)
                    {
                        newInfos.push_back(infos[i]);
                        usedInfos.insert(infos[i]);
                    }
                }
                infosRef = newInfos;
            }

            array->resize(infosRef.size());

            for (int i = 0; i < infosRef.size(); i++)
            {
                ClassInstance* obj = ClassInstance::allocate(itype);
                array->element<ClassInstance*>(i) = obj;
                Info* info = reinterpret_cast<Info*>(obj->structure());

                info->nodeName =
                    c->stringType()->allocate(infosRef[i].node->name());
                info->nodeType =
                    c->stringType()->allocate(infosRef[i].node->protocol());
                info->frame = infosRef[i].sourceFrame;
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(mapPropertyToGlobalFrames, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int depth = NODE_ARG(1, int);
        StringType::String* root = NODE_ARG_OBJECT(2, StringType::String);
        const Class* atype =
            static_cast<const Class*>(c->arrayType(c->intType(), 1, 0));

        IPNode* rootNode = s->graph().root();

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil name");

        if (root)
        {
            if (!(rootNode = s->graph().findNode(root->c_str())))
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "Bad node name");
            }
        }

        IPNode::PropertyAsFramesVisitor visitor(name->c_str(), depth);
        rootNode->visitRecursive(visitor);

        IPNode::FrameVector frames = visitor.result(rootNode);
        set<int> frameSet;

#if 0
    for (IPNode::NodeFramesMap::const_iterator i = visitor.nodeMap.begin();
         i != visitor.nodeMap.end();
         ++i)
    {
        const IPNode::NodeFramesMap::value_type& x = *i;
        cout << x.first->name() << " ";

        for (size_t i = 0; i < x.second.size(); i++)
        {
            cout << " " << x.second[i];
        }

        cout << endl;
    }
#endif

        for (size_t i = 0; i < frames.size(); i++)
            frameSet.insert(frames[i]);

        DynamicArray* array = new DynamicArray(atype, 1);

        array->resize(frameSet.size());
        size_t count = 0;

        for (set<int>::const_iterator i = frameSet.begin(); i != frameSet.end();
             ++i)
        {
            array->element<int>(count) = *i;
            count++;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(closestNodesOfType, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        StringType::String* typeName = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* root = NODE_ARG_OBJECT(1, StringType::String);
        int depth = NODE_ARG(2, int);
        const StringType* stype = c->stringType();

        IPNode* rootNode = s->graph().root();

        if (root)
        {
            if (!(rootNode = s->graph().findNode(root->c_str())))
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "Bad node name");
            }
        }

        if (!typeName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil type name");

        IPNode::ClosestByTypeNameVisitor visitor(typeName->c_str(), depth);
        rootNode->visitRecursive(visitor);

        if (!visitor.nodes.empty())
        {
            size_t index = 0;
            array->resize(visitor.nodes.size());

            for (set<IPNode*>::iterator i = visitor.nodes.begin();
                 i != visitor.nodes.end(); ++i)
            {
                StringType::String* str = stype->allocate((*i)->name());
                array->element<ClassInstance*>(index++) = str;
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(frameStart, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->rangeStart());
    }

    NODE_IMPLEMENTATION(frameEnd, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->rangeEnd() - 1);
    }

    NODE_IMPLEMENTATION(narrowedFrameStart, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->narrowedRangeStart());
    }

    NODE_IMPLEMENTATION(narrowedFrameEnd, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->narrowedRangeEnd() - 1);
    }

    NODE_IMPLEMENTATION(narrowToRange, void)
    {
        Session* s = Session::currentSession();
        s->setNarrowedRange(NODE_ARG(0, int), NODE_ARG(1, int) + 1);
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(setRealtime, void)
    {
        Session* s = Session::currentSession();
        s->setRealtime(NODE_ARG(0, bool));
    }

    NODE_IMPLEMENTATION(isRealtime, bool)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->realtime());
    }

    NODE_IMPLEMENTATION(skipped, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->skipped());
    }

    NODE_IMPLEMENTATION(isCurrentFrameIncomplete, bool)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->currentStateIsIncomplete());
    }

    NODE_IMPLEMENTATION(isCurrentFrameError, bool)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->currentStateIsError());
    }

    NODE_IMPLEMENTATION(currentFrameStatus, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->currentFrameState());
    }

    NODE_IMPLEMENTATION(setFPS, void)
    {
        Session* s = Session::currentSession();
        float fps = NODE_ARG(0, float);
        s->setFPS(double(fps));
    }

    NODE_IMPLEMENTATION(setFrameStart, void)
    {
        Session* s = Session::currentSession();
        s->setRangeStart(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(setFrameEnd, void)
    {
        Session* s = Session::currentSession();
        s->setRangeEnd(NODE_ARG(0, int) + 1);
    }

    NODE_IMPLEMENTATION(realFPS, float)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(float(s->realFPS()));
    }

    NODE_IMPLEMENTATION(fps, float)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(float(s->fps()));
    }

    NODE_IMPLEMENTATION(mbps, float)
    {
        NODE_RETURN(float(TwkUtil::FileStream::mbps()));
    }

    NODE_IMPLEMENTATION(resetMbps, void) { TwkUtil::FileStream::resetMbps(); }

    NODE_IMPLEMENTATION(setInc, void)
    {
        Session* s = Session::currentSession();
        int inc = NODE_ARG(0, int);
        s->setInc(inc);
    }

    NODE_IMPLEMENTATION(inc, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->inc());
    }

    NODE_IMPLEMENTATION(markFrame, void)
    {
        Session* s = Session::currentSession();
        int frame = NODE_ARG(0, int);
        bool mark = NODE_ARG(1, bool);

        if (mark)
            s->markFrame(frame);
        else
            s->unmarkFrame(frame);
    }

    NODE_IMPLEMENTATION(isMarked, bool)
    {
        Session* s = Session::currentSession();
        int frame = NODE_ARG(0, int);

        NODE_RETURN(s->isMarked(frame));
    }

    NODE_IMPLEMENTATION(markedFrames, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();

        const Session::Frames& frames = s->markedFrames();
        DynamicArray* array = new DynamicArray(type, 1);
        array->resize(frames.size());

        for (int i = 0; i < frames.size(); i++)
        {
            array->element<int>(i) = frames[i];
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(cacheSize, int)
    {
        return IPCore::App()->availableMemory();
    }

    NODE_IMPLEMENTATION(setCacheMode, void)
    {
        Session* s = Session::currentSession();
        Session::CachingMode mode = Session::CachingMode(NODE_ARG(0, int));
        s->setCaching(mode);

        //
        //  If we just turned the cache off, we should clear
        //  the frame-level cache in case when we turn the cache on
        //  something has changed that means we should remap identifiers
        //  to frames.
        //
        if (mode == Session::NeverCache)
        {
            s->graph().cache().lock();
            s->graph().cache().clear();
            s->graph().cache().unlock();
        }
    }

    NODE_IMPLEMENTATION(releaseAllCachedImages, void)
    {
        Session* s = Session::currentSession();
        s->setCaching(Session::NeverCache);

        s->graph().cache().lock();
        s->graph().cache().clearAllButFrame(s->currentFrame(), true);
        s->graph().cache().unlock();
    }

    NODE_IMPLEMENTATION(releaseAllUnusedImages, void)
    {
        Session* s = Session::currentSession();
        FBCache& cache = s->graph().cache();
        cache.lock();
        cache.freeAllTrash();
        cache.unlock();
    }

    NODE_IMPLEMENTATION(setAudioCacheMode, void)
    {
        Session* s = Session::currentSession();
        s->setAudioCaching(Session::CachingMode(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(audioCacheMode, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(int(s->audioCachingMode()));
    }

    NODE_IMPLEMENTATION(cacheMode, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(int(s->cachingMode()));
    }

    NODE_IMPLEMENTATION(cacheOutsideRegion, int)
    {
        NODE_RETURN(bool(FBCache::cacheOutsideRegion()));
    }

    NODE_IMPLEMENTATION(setCacheOutsideRegion, void)
    {
        Session* s = Session::currentSession();
        Session::CachingMode mode = s->cachingMode();
        s->setCaching(Session::NeverCache);

        FBCache::setCacheOutsideRegion(NODE_ARG(0, bool));

        s->setCaching(mode);
        s->askForRedraw(true);
    }

    NODE_IMPLEMENTATION(isCaching, bool)
    {
        bool b = Session::currentSession()->isCaching();
        NODE_RETURN(b);
    }

    NODE_IMPLEMENTATION(isBuffering, bool)
    {
        bool b = Session::currentSession()->isBuffering();
        NODE_RETURN(b);
    }

    NODE_IMPLEMENTATION(cacheInfo, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        Session* s = Session::currentSession();
        const Class* ttype = (const Class*)NODE_THIS.type();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(ttype->fieldType(6));

        struct CTuple
        {
            int64 capacity;
            int64 used;
            int64 usedLookahead;
            float lookahead;
            float lookaheadWaitTime;
            float audio;
            DynamicArray* array;
        };

        ClassInstance* tuple = ClassInstance::allocate(ttype);
        CTuple* cinfo = reinterpret_cast<CTuple*>(tuple->structure());

        Session::CacheStats stats = s->cacheStats();

        cinfo->capacity = stats.capacity;
        cinfo->used = stats.used;

        // Note: usedLookAhead is no longer computed since it isn't used anymore
        // by the app. However usedLookahead placeholder kept in the command for
        // backward compatibility.
        cinfo->usedLookahead = stats.used;

        cinfo->lookahead = stats.lookAheadSeconds;
        cinfo->lookaheadWaitTime = s->maxBufferedWaitTime();
        cinfo->audio = stats.audioSecondsCached;
        cinfo->array = new DynamicArray(atype, 1);

        cinfo->array->resize(stats.cachedRanges.size() * 2);

        for (size_t i = 0; i < stats.cachedRanges.size(); i++)
        {
            cinfo->array->element<int>(i * 2) = stats.cachedRanges[i].first;
            cinfo->array->element<int>(i * 2 + 1) =
                stats.cachedRanges[i].second;
        }

        NODE_RETURN(tuple);
    }

    NODE_IMPLEMENTATION(audioCacheInfo, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        Session* s = Session::currentSession();
        const Class* ttype = (const Class*)NODE_THIS.type();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(ttype->fieldType(1));

        struct AudioCacheInfoTuple
        {
            float audioSecondsCached;
            DynamicArray* audioCachedRanges;
        };

        ClassInstance* tuple = ClassInstance::allocate(ttype);
        AudioCacheInfoTuple* cinfo =
            reinterpret_cast<AudioCacheInfoTuple*>(tuple->structure());

        cinfo->audioSecondsCached =
            s->graph().audioCache().totalSecondsCached();

        TwkAudio::AudioCache::FrameRangeVector cachedRanges;
        s->graph().audioCache().computeCachedRangesStat(s->fps(), cachedRanges);
        cinfo->audioCachedRanges = new DynamicArray(atype, 1);
        cinfo->audioCachedRanges->resize(cachedRanges.size() * 2);
        for (size_t i = 0; i < cachedRanges.size(); i++)
        {
            cinfo->audioCachedRanges->element<int>(i * 2) =
                cachedRanges[i].first;
            cinfo->audioCachedRanges->element<int>(i * 2 + 1) =
                cachedRanges[i].second;
        }

        NODE_RETURN(tuple);
    }

    NODE_IMPLEMENTATION(fullScreenMode, void)
    {
        Session::currentSession()->fullScreenMode(NODE_ARG(0, bool));
    }

    NODE_IMPLEMENTATION(isFullScreen, bool)
    {
        NODE_RETURN(Session::currentSession()->isFullScreen());
    }

    struct FrameBufferFinder
    {
        FrameBufferFinder()
            : fbImage(0)
        {
        }

        const IPImage* fbImage;

        void operator()(IPImage* i)
        {
            if (fbImage)
                return;

            if (i->destination != IPImage::OutputTexture)
            {
                fbImage = i;
            }
        }
    };

    NODE_IMPLEMENTATION(getCurrentImageSize, Mu::Vector2f)
    {
        Session* s = Session::currentSession();
        Vector2f v;

        FrameBufferFinder finder;
        foreach_ip((IPImage*)s->displayImage(), finder);

        if (const IPImage* i = finder.fbImage)
        {
            if (TwkFB::FrameBuffer* fb = i->fb)
            {
                if (fb->uncrop())
                {
                    v[0] = fb->uncropWidth();
                    v[1] = fb->uncropHeight();
                }
                else
                {
                    v[0] = fb->width();
                    v[1] = fb->height();
                }
            }
            else
            {
                v[0] = i->width;
                v[1] = i->height;
            }
        }
        else
        {
            v[0] = 0;
            v[1] = 0;
        }

        NODE_RETURN(v);
    }

    NODE_IMPLEMENTATION(getCurrentNodesOfType, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();
        const StringType::String* tname =
            NODE_ARG_OBJECT(0, StringType::String);

        if (!tname)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil tname");

        Session::NodeVector nodes;
        s->findCurrentNodesByTypeName(nodes, tname->c_str());
        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(nodes.size());

        for (int i = 0; i < nodes.size(); i++)
        {
            array->element<StringType::String*>(i) =
                stype->allocate(nodes[i]->name());
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(nodesOfType, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const StringType::String* tname =
            NODE_ARG_OBJECT(0, StringType::String);
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!tname)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil tname");

        Session::NodeVector nodes;
        s->findNodesByTypeName(nodes, tname->c_str());
        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(nodes.size());

        for (int i = 0; i < nodes.size(); i++)
        {
            array->element<StringType::String*>(i) =
                stype->allocate(nodes[i]->name());
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(event2image, Mu::Vector2f)
    {
        Session* s = Session::currentSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        Vector2f inp = NODE_ARG(1, Vector2f);
        bool normalize = NODE_ARG(2, bool);
        float x = inp[0];
        float y = inp[1];

        if (!name)
            throw NilArgumentException(NODE_THREAD);

        std::string sourceName(name->c_str());

        std::tuple<std::string, bool> tileInfo = detectTile(sourceName);
        const bool sourceIsATile = std::get<1>(tileInfo);

        // Tiles must use normalize coordinates.
        if (sourceIsATile)
        {
            normalize = true;
            sourceName = std::get<0>(tileInfo);
        }

        //
        //  NOTE: OpenGL defines NDC as [-1, 1] in both dimensions. This
        //  is a bit different from RenderMan for example which defines it
        //  as [0, 1]. The glViewport() call maps from pixel space to GL
        //  NDC space. So we need to undo that transform.
        //

        Vector2f mp;
        try
        {
            Box2f vp = s->renderer()->viewport();
            Mat44f M, P, T, O, Pl;
            RenderQuery renderQuery(s->renderer());
            renderQuery.imageTransforms(sourceName.c_str(), M, P, T, O, Pl);

            // Note we disregard orientation i.e. O matrix
            // from the normalization calc because our
            // IP images are all normalized to a single
            // BOTTOM_LEFT orientation in the renderer.
            Mat44f NI = normalize ? Pl.inverted() : Mat44f();

            Mat44f I = P * M * NI;
            I.invert();

            Vec3f p = I
                      * Vec3f((x - vp.min.x) / vp.size().x * 2.0 - 1.0f,
                              (y - vp.min.y) / vp.size().y * 2.0 - 1.0f, 0);

            mp[0] = p.x;
            mp[1] = p.y;

            if (sourceIsATile)
            {
                // It is important to use the full name, not the tile name.
                adjustTileCoords(name->c_str(), mp, renderQuery, true);
            }
        }
        catch (...)
        {
            ostringstream ostr;
            ostr << "bad image name '" << sourceName.c_str() << "'";
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      ostr.str().c_str());
        }

        NODE_RETURN(mp);
    }

    static void computePixelImages(float x, float y,
                                   ImageRenderer::RenderedImagesVector& images,
                                   PixelImageVector& pixelImages)
    {
        for (size_t i = 0; i < images.size(); ++i)
        {
            const ImageRenderer::RenderedImage& image = images[i];
            PixelImage& ps = pixelImages[i];

            //
            //  Sanity check for Nans  (this only checks on field for nans)
            //
            const IPImage::Matrix IP =
                image.projectionMatrix(0, 0) == image.projectionMatrix(0, 0)
                    ? image.projectionMatrix
                    : IPImage::Matrix();
            const IPImage::Matrix IM =
                image.globalMatrix(0, 0) == image.globalMatrix(0, 0)
                    ? image.globalMatrix
                    : IPImage::Matrix();
            const IPImage::Matrix M = IP * IM;

            //
            //  M may have nans, so catch float exc that may occur
            //
            IPImage::Matrix I;

            try
            {
                I = invert(M);
            }
            catch (...)
            {
                I = IPImage::Matrix();
            }

            Vec3f point(x, y, 0);
            const Vec3f p = I * point;

            const float iw = image.width;
            const float ih = image.height;
            const float uw = image.uncropWidth;
            const float uh = image.uncropHeight;
            const float ux = image.uncropX;
            const float uy = image.uncropY;
            const float ua = uw / uh;
            const float af = image.pixelAspect >= 1
                                 ? image.pixelAspect * ua
                                 : image.initPixelAspect * ua;

            const float wmin = ux / uw;
            const float hmin = (uh - uy - ih) / uh;
            const float wmax = (ux + iw) / uw;
            const float hmax = (uh - uy) / uh;

            const float xmin =
                (wmin + (wmax - wmin) * image.stencilBox.min.x) * af;
            const float ymin = hmin + (hmax - hmin) * image.stencilBox.min.y;
            const float xmax =
                (wmin + (wmax - wmin) * image.stencilBox.max.x) * af;
            const float ymax = hmin + (hmax - hmin) * image.stencilBox.max.y;

            const bool inside =
                p.x >= xmin && p.x <= xmax && p.y >= ymin && p.y <= ymax;

            if (inside)
            {
                //
                //  If we're inside the closest point will be in this
                //  image. Start with the x=0 edge being the closest.
                //
                ps.inside = true;
                float iclosest = numeric_limits<float>::max();
                Vec3f p0(0.0f);

                if (p.x - xmin < iclosest)
                {
                    p0.x = xmin;
                    p0.y = p.y;
                    iclosest = p.x - xmin;
                }
                if (xmax - p.x < iclosest)
                {
                    p0.x = xmax;
                    p0.y = p.y;
                    iclosest = xmax - p.x;
                }

                if (p.y - ymin < iclosest)
                {
                    p0.x = p.x;
                    p0.y = ymin;
                    iclosest = p.y - ymin;
                }

                if (ymax - p.y < iclosest)
                {
                    p0.x = p.x;
                    p0.y = ymax;
                    iclosest = ymax - p.y;
                }

                p0 = M * p0;
                ps.x = p0.x;
                ps.y = p0.y;

                Vec3f tp = Vec3f((p.x / ua) * uw, p.y * uh, 0);

                ps.px = tp.x;
                ps.py = tp.y;

                assert(ps.px == ps.px);

                const Vec3f dir = p0 - point;
                const float d = dot(dir, dir);
                ps.edgeDistance = sqrt(d);
            }
            else
            {

                Vec3f p0(0.0f);

                if (p.x < xmin)
                {
                    if (p.y <= ymin)
                        p0 = Vec3f(xmin, ymin, 0);
                    else if (p.y >= ymax)
                        p0 = Vec3f(xmin, ymax, 0);
                    else
                        p0 = Vec3f(xmin, p.y, 0);
                }
                else if (p.x > xmax)
                {
                    if (p.y <= ymin)
                        p0 = Vec3f(xmax, ymin, 0);
                    else if (p.y >= ymax)
                        p0 = Vec3f(xmax, ymax, 0);
                    else
                        p0 = Vec3f(xmax, p.y, 0);
                }
                else
                {
                    p0 = Vec3f(p.x, p.y >= ymax ? ymax : ymin, 0);
                }

                const Vec3f tp = Vec3f((p0.x / ua) * uw, p0.y * uh, 0);

                ps.px = tp.x;
                ps.py = tp.y;

                p0 = M * p0;
                const Vec3f dir = p0 - point;
                const float d = dot(dir, dir);

                ps.inside = false;
                ps.x = p0.x;
                ps.y = p0.y;
                ps.edgeDistance = sqrt(d);
            }
        }
    }

    NODE_IMPLEMENTATION(inputAtPixel, Pointer)
    {
        Vector2f inp = NODE_ARG(0, Vector2f);
        bool strict = NODE_ARG(1, bool);
        Session* session = Session::currentSession();
        Box2f vp = session->renderer()->viewport();
        float x = (inp[0] - vp.min.x) / (vp.size().x - 1.0) * 2.0 - 1.0;
        float y = (inp[1] - vp.min.y) / (vp.size().y - 1.0) * 2.0 - 1.0;
        const StringType* stype = TwkApp::muContext()->stringType();
        StringType::String* inputName = 0;

        IPNode* node = session->graph().viewNode();
        if (!node)
            NODE_RETURN(0);

        IPNode::IPNodes inputs = node->inputs();
        if (inputs.empty())
            NODE_RETURN(0);

        ImageRenderer::RenderedImagesVector images;
        RenderQuery renderQuery(session->renderer());
        renderQuery.imagesByTag(images, "");

        if (images.empty())
            NODE_RETURN(0);

        PixelImageVector pixelImages;
        pixelImages.resize(images.size());
        computePixelImages(x, y, images, pixelImages);

        //
        //  Retime only has one input and does not report it in
        //  imagesAtPixel
        //
        if (inputs.size() == 1 && pixelImages[0].inside)
        {
            inputName = stype->allocate(inputs[0]->name());
            NODE_RETURN(inputName);
        }

        //
        //  Check for image containing point
        //

        for (size_t i = 0; i < images.size(); ++i)
        {
            ImageRenderer::RenderedImage& image = images[i];
            PixelImage& ps = pixelImages[i];

            if (!image.node)
                continue;

            const IPNode* imageNode = image.node;
            while (imageNode->group())
                imageNode = imageNode->group();

            if (ps.inside)
            {
                for (int j = 0; j < inputs.size(); ++j)
                {
                    if (inputs[j] == imageNode)
                    {
                        inputName = stype->allocate(imageNode->name());
                        NODE_RETURN(inputName);
                    }
                }
            }
        }

        if (strict)
            NODE_RETURN(0);

        //
        //  Didn't find image containing point, so find closest
        //

        //  Retime only has one input and does not report it in
        //  imagesAtPixel
        //
        if (inputs.size() == 1)
        {
            inputName = stype->allocate(inputs[0]->name());
            NODE_RETURN(inputName);
        }

        float closestEdge = numeric_limits<float>::max();
        const IPNode* closestInput = 0;

        for (int i = 0; i < images.size(); ++i)
        {
            ImageRenderer::RenderedImage& image = images[i];
            PixelImage& ps = pixelImages[i];

            if (!image.node)
                continue;

            const IPNode* imageNode =
                (image.node->group()) ? image.node->group() : image.node;

            if (ps.edgeDistance < closestEdge)
            {
                for (int j = 0; j < inputs.size(); ++j)
                {
                    if (inputs[j] == imageNode)
                    {
                        closestEdge = ps.edgeDistance;
                        closestInput = imageNode;
                    }
                }
            }
        }

        if (closestInput)
            inputName = stype->allocate(closestInput->name());

        NODE_RETURN(inputName);
    }

    NODE_IMPLEMENTATION(image2event, Mu::Vector2f)
    {
        Session* s = Session::currentSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        Vector2f inp = NODE_ARG(1, Vector2f);
        bool normalize = NODE_ARG(2, bool);
        float x = inp[0];
        float y = inp[1];

        if (!name)
            throw NilArgumentException(NODE_THREAD);

        std::string sourceName(name->c_str());

        std::tuple<std::string, bool> tileInfo = detectTile(sourceName);
        const bool sourceIsATile = std::get<1>(tileInfo);

        // Tiles must use normalize coordinates.
        if (sourceIsATile)
        {
            normalize = true;
            sourceName = std::get<0>(tileInfo);
        }

        //
        //  See event2image for comments (this does the inverse)
        //

        Vector2f mp;
        try
        {
            Box2f vp = s->renderer()->viewport();
            Mat44f M, P, T, O, Pl;
            RenderQuery renderQuery(s->renderer());
            renderQuery.imageTransforms(sourceName.c_str(), M, P, T, O, Pl);

            Mat44f W = P * M;

            if (normalize)
            {
                // Note we disregard orientation i.e. O matrix
                // from the normalization calc because our
                // IP images are all normalized to a single
                // BOTTOM_LEFT orientation in the renderer.
                W *= Pl.inverted();
            }

            if (sourceIsATile)
            {
                Vector2f inputCoords;
                inputCoords[0] = x;
                inputCoords[1] = y;

                // It is important to use the full name, not the tile name.
                adjustTileCoords(name->c_str(), inputCoords, renderQuery,
                                 false);

                x = inputCoords[0];
                y = inputCoords[1];
            }

            Vec3f p = W * Vec3f(x, y, 0);
            p.x = (p.x / 2.0 + 0.5f) * vp.size().x + vp.min.x;
            p.y = (p.y / 2.0 + 0.5f) * vp.size().y + vp.min.y;

            mp[0] = p.x;
            mp[1] = p.y;
        }
        catch (...)
        {
            ostringstream ostr;
            ostr << "bad image name '" << sourceName.c_str() << "'";
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      ostr.str().c_str());
        }

        NODE_RETURN(mp);
    }

    NODE_IMPLEMENTATION(event2camera, Mu::Vector2f)
    {
        Session* s = Session::currentSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        Vector2f inp = NODE_ARG(1, Vector2f);
        float x = inp[0];
        float y = inp[1];

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil name");

        //
        //  NOTE: OpenGL defines "NDC" as [-1, 1] in both dimensions. This
        //  is a bit different from RenderMan for example which defines it
        //  as [0, 1]. The glViewport() call maps from pixel space to GL
        //  NDC space. So we need to undo that transform.
        //

        Vector2f mp;
        try
        {
            Box2f vp = s->renderer()->viewport();
            Mat44f M, P, T, O, Pl;
            RenderQuery renderQuery(s->renderer());
            renderQuery.imageTransforms(name->c_str(), M, P, T, O, Pl);

            Mat44f I = P.inverted();

            Vec3f p = I
                      * Vec3f((x - vp.min.x) / vp.size().x * 2.0 - 1.0f,
                              (y - vp.min.y) / vp.size().y * 2.0 - 1.0f, 0);

            mp[0] = p.x;
            mp[1] = p.y;
        }
        catch (...)
        {
            ostringstream ostr;
            ostr << "bad image name '" << name->c_str() << "'";
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      ostr.str().c_str());
        }
        NODE_RETURN(mp);
    }

    NODE_IMPLEMENTATION(imagesAtPixel, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const StringType* stype = c->stringType();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* rtype = static_cast<const Class*>(atype->elementType());
        Vector2f inp = NODE_ARG(0, Vector2f);
        StringType::String* tagname = NODE_ARG_OBJECT(1, StringType::String);
        bool sourcesOnly = NODE_ARG(2, bool);

        // Note that the viewport is already taking the devicePixelRatio into
        // account (High DPI display) whereas the input coordinates are in pixel
        // space, so we need to adjust the viewport accordingly.
        Box2f vp = s->renderer()->viewport()
                   / s->renderer()->currentDevice()->devicePixelRatio();

        float x = (inp[0] - vp.min.x) / (vp.size().x - 1.0) * 2.0 - 1.0;
        float y = (inp[1] - vp.min.y) / (vp.size().y - 1.0) * 2.0 - 1.0;
        DynamicArray* array = new DynamicArray(atype, 1);
        const FixedArrayType* m44type = static_cast<const FixedArrayType*>(
            c->arrayType(c->floatType(), 2, 4, 4, 0));

        ImageRenderer::RenderedImagesVector images;
        RenderQuery renderQuery(s->renderer());
        renderQuery.imagesByTag(images, tagname ? tagname->c_str() : "");

        PixelImageVector pixelImages;
        pixelImages.resize(images.size());
        computePixelImages(x, y, images, pixelImages);

        std::vector<size_t> wantedOnes;
        if (sourcesOnly)
        {
            int closestIndex = -1;
            float closestEdge = numeric_limits<float>::max();

            for (size_t i = 0; i < images.size(); i++)
            {
                ImageRenderer::RenderedImage& image = images[i];
                PixelImage& ps = pixelImages[i];

                if (!image.node)
                    continue;

                if (!dynamic_cast<const SourceIPNode*>(image.node))
                    continue;

                if (ps.inside)
                    wantedOnes.push_back(i);
                if (ps.edgeDistance < closestEdge)
                {
                    closestEdge = ps.edgeDistance;
                    closestIndex = i;
                }
            }

            if (wantedOnes.empty() && closestIndex != -1)
            {
                // if none inside and found a closest
                wantedOnes.push_back(closestIndex);
            }
        }
        else
        {
            for (size_t i = 0; i < images.size(); ++i)
            {
                wantedOnes.push_back(i);
            }
        }

        array->resize(wantedOnes.size());

        struct TT
        {
            StringType::String* source;
            float x;
            float y;
            float px;
            float py;
            bool inside;
            float edge;
            Pointer M;
            Pointer G;
            Pointer P;
            Pointer T;
            Pointer O;
            Pointer Pl;
            Pointer node;
            Pointer tagArray;
            int index;
            float pixelAspect;
            Pointer device;
            Pointer pdevice;
            int serialNum;
            int imageNum;
            int textureID;
            float initPixelAspect;
        };

        const DynamicArrayType* tarrayType =
            static_cast<const DynamicArrayType*>(
                atype->elementType()->fieldType(14));
        const Class* tsType =
            static_cast<const Class*>(tarrayType->elementType());

        for (size_t i = 0; i < wantedOnes.size(); i++)
        {
            ImageRenderer::RenderedImage& image = images[wantedOnes[i]];
            PixelImage& ps = pixelImages[wantedOnes[i]];

            ClassInstance* o = ClassInstance::allocate(rtype);
            TT* tt = reinterpret_cast<TT*>(o->structure());

            FixedArray* M =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            FixedArray* G =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            FixedArray* P =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            FixedArray* T =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            FixedArray* O =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            FixedArray* Pl =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            *M->data<Mat44f>() = image.modelMatrix;
            *G->data<Mat44f>() = image.globalMatrix;
            *P->data<Mat44f>() = image.projectionMatrix;
            *T->data<Mat44f>() = image.textureMatrix;
            *O->data<Mat44f>() = image.orientationMatrix;
            *Pl->data<Mat44f>() = image.placementMatrix;

            DynamicArray* a = new DynamicArray(tarrayType, 1);
            a->resize(image.tagMap.size());
            int count = 0;

            for (IPImage::TagMap::const_iterator q = image.tagMap.begin();
                 q != image.tagMap.end(); ++q, count++)
            {
                ClassInstance* t = ClassInstance::allocate(tsType);
                *t->data<Pointer>() = stype->allocate(q->first);
                *(t->data<Pointer>() + 1) = stype->allocate(q->second);
                a->element<ClassInstance*>(count) = t;
            }

            tt->tagArray = a;

            string nname = image.node ? image.node->name() : "";
            tt->index = image.index;
            tt->node = stype->allocate(nname);
            tt->source = stype->allocate(image.source);
            tt->x = (ps.x + 1.0) * 0.5 * vp.size().x + vp.min.x;
            tt->y = (ps.y + 1.0) * 0.5 * vp.size().y + vp.min.y;
            tt->px = ps.px;
            tt->py = ps.py;
            tt->inside = ps.inside;
            tt->edge = ps.edgeDistance;
            tt->M = M;
            tt->G = G;
            tt->P = P;
            tt->T = T;
            tt->O = O;
            tt->Pl = Pl;
            tt->pixelAspect = image.pixelAspect;
            tt->device =
                stype->allocate(image.device ? image.device->name() : "");
            tt->pdevice = stype->allocate(
                image.device ? image.device->physicalDevice()->name() : "");
            tt->serialNum = image.serialNum;
            tt->imageNum = image.imageNum;
            tt->textureID = image.textureID;
            tt->initPixelAspect = image.initPixelAspect;

            array->element<ClassInstance*>(i) = o;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(sourcesAtFrame, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const StringType* stype = c->stringType();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const int frame = NODE_ARG(0, int);
        DynamicArray* array = new DynamicArray(atype, 1);

        IPNode::MetaEvalInfoVector infos;
        IPNode::MetaEvalInfoCollectorByType<SourceIPNode> collector(infos);
        s->graph().root()->metaEvaluate(s->graph().contextForFrame(frame),
                                        collector);

        // Add unique sources to the list while making sure to respect the same
        // order
        array->resize(infos.size());
        std::unordered_set<std::string> uniqueSources;
        for (size_t i = 0, size = infos.size(), pos = 0; i < size; ++i)
        {
            if (auto ret = uniqueSources.insert(infos[i].node->name());
                ret.second)
            {
                array->element<StringType::String*>(pos++) =
                    stype->allocate(infos[i].node->name());
            }
        }
        array->resize(uniqueSources.size());

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(renderedImages, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const StringType* stype = c->stringType();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* rtype = static_cast<const Class*>(atype->elementType());
        DynamicArray* array = new DynamicArray(atype, 1);
        const FixedArrayType* m44type = static_cast<const FixedArrayType*>(
            c->arrayType(c->floatType(), 2, 4, 4, 0));

        ImageRenderer::RenderedImagesVector sarray;
        RenderQuery renderQuery(s->renderer());
        renderQuery.renderedImages(sarray);
        array->resize(sarray.size());

        const DynamicArrayType* tarrayType =
            static_cast<const DynamicArrayType*>(
                atype->elementType()->fieldType(20));
        const Class* tsType =
            static_cast<const Class*>(tarrayType->elementType());

        struct TT
        {
            StringType::String* source;
            int index;
            Vector2f imageMin;
            Vector2f imageMax;
            Vector2f stencilMin;
            Vector2f stencilMax;
            Pointer M;
            Pointer G;
            Pointer P;
            Pointer T;
            Pointer O;
            Pointer Pl;
            int w;
            int h;
            float pixelAspect;
            int bitDepth;
            bool floatingPoint;
            int numChannels;
            bool planar;
            Pointer node;
            Pointer tags;
            Pointer device;
            Pointer pdevice;
            int serialNum;
            int imageNum;
            int textureID;
        };

        for (int i = 0; i < sarray.size(); i++)
        {
            ImageRenderer::RenderedImage ps = sarray[i];

            ClassInstance* o = ClassInstance::allocate(rtype);
            TT* tt = reinterpret_cast<TT*>(o->structure());

            FixedArray* M =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));
            FixedArray* P =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));
            FixedArray* T =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));
            FixedArray* G =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));
            FixedArray* O =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));
            FixedArray* Pl =
                static_cast<FixedArray*>(ClassInstance::allocate(m44type));

            *M->data<Mat44f>() = ps.modelMatrix;
            *P->data<Mat44f>() = ps.projectionMatrix;
            *T->data<Mat44f>() = ps.textureMatrix;
            *G->data<Mat44f>() = ps.globalMatrix;
            *O->data<Mat44f>() = ps.orientationMatrix;
            *Pl->data<Mat44f>() = ps.placementMatrix;

            DynamicArray* a = new DynamicArray(tarrayType, 1);
            a->resize(ps.tagMap.size());
            int count = 0;

            for (IPImage::TagMap::const_iterator q = ps.tagMap.begin();
                 q != ps.tagMap.end(); ++q, count++)
            {
                ClassInstance* t = ClassInstance::allocate(tsType);
                *t->data<Pointer>() = stype->allocate(q->first);
                *(t->data<Pointer>() + 1) = stype->allocate(q->second);
                a->element<ClassInstance*>(count) = t;
            }

            string nname = ps.node ? ps.node->name() : "";

            tt->tags = a;

            tt->source = stype->allocate(ps.source);
            tt->index = ps.index;
            tt->imageMin[0] = ps.imageBox.min.x;
            tt->imageMin[1] = ps.imageBox.min.y;
            tt->imageMax[0] = ps.imageBox.max.x;
            tt->imageMax[1] = ps.imageBox.max.y;
            tt->stencilMin[0] = ps.stencilBox.min.x;
            tt->stencilMin[1] = ps.stencilBox.min.y;
            tt->stencilMax[0] = ps.stencilBox.max.x;
            tt->stencilMax[1] = ps.stencilBox.max.y;
            tt->M = M;
            tt->P = P;
            tt->T = T;
            tt->G = G;
            tt->O = O;
            tt->Pl = Pl;
            tt->w = ps.width;
            tt->h = ps.height;
            tt->bitDepth = ps.bitDepth;
            tt->floatingPoint = ps.floatingPoint;
            tt->numChannels = ps.numChannels;
            tt->planar = ps.planar;
            tt->pixelAspect = ps.pixelAspect;
            tt->node = stype->allocate(nname);
            tt->device = stype->allocate(ps.device ? ps.device->name() : "");
            tt->pdevice = stype->allocate(
                ps.device ? ps.device->physicalDevice()->name() : "");
            tt->serialNum = ps.serialNum;
            tt->imageNum = ps.imageNum;
            tt->textureID = ps.textureID;

            array->element<ClassInstance*>(i) = o;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(imageGeometry, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        Box2f vp = s->renderer()->viewport();
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* ttype = static_cast<const Class*>(atype->elementType());
        const StringType::String* source =
            NODE_ARG_OBJECT(0, StringType::String);
        bool useStencil = NODE_ARG(1, bool);

        if (!source)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source name");
        }

        vector<Vec3f> points;
        RenderQuery renderQuery(s->renderer());
        renderQuery.imageCorners(source->c_str(), points, useStencil);
        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(points.size());

        for (int i = 0; i < points.size(); i++)
        {
            Vector2f p;
            p[0] = (points[i].x * 0.5 + 0.5) * vp.size().x + vp.min.x;
            p[1] = (points[i].y * 0.5 + 0.5) * vp.size().y + vp.min.y;

            array->element<Vector2f>(i) = p;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(imageGeometryByIndex, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        Box2f vp = s->renderer()->viewport();
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* ttype = static_cast<const Class*>(atype->elementType());
        int index = NODE_ARG(0, int);
        bool useStencil = NODE_ARG(1, bool);

        try
        {
            vector<Vec3f> points;
            RenderQuery renderQuery(s->renderer());
            renderQuery.imageCorners(index, points, useStencil);
            DynamicArray* array = new DynamicArray(atype, 1);
            array->resize(points.size());

            for (int i = 0; i < points.size(); i++)
            {
                Vector2f p;
                p[0] = (points[i].x * 0.5 + 0.5) * vp.size().x + vp.min.x;
                p[1] = (points[i].y * 0.5 + 0.5) * vp.size().y + vp.min.y;

                array->element<Vector2f>(i) = p;
            }

            NODE_RETURN(array);
        }
        catch (...)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad index");
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(imageGeometryByTag, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        Box2f vp = s->renderer()->viewport();
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* ttype = static_cast<const Class*>(atype->elementType());
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* value =
            NODE_ARG_OBJECT(1, StringType::String);
        bool useStencil = NODE_ARG(2, bool);

        if (!name || !value)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "missing tag name or value");

        try
        {
            vector<Vec3f> points;
            RenderQuery renderQuery(s->renderer());
            renderQuery.imageCornersByTag(name->utf8std(), value->utf8std(),
                                          points, useStencil);
            DynamicArray* array = new DynamicArray(atype, 1);
            array->resize(points.size());

            for (int i = 0; i < points.size(); i++)
            {
                Vector2f p;
                p[0] = (points[i].x * 0.5 + 0.5) * vp.size().x + vp.min.x;
                p[1] = (points[i].y * 0.5 + 0.5) * vp.size().y + vp.min.y;

                array->element<Vector2f>(i) = p;
            }

            NODE_RETURN(array);
        }
        catch (...)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad index");
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(sessionFileName, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        const StringType* t = static_cast<const StringType*>(NODE_THIS.type());
        NODE_RETURN(t->allocate(s->fileName()));
    }

    NODE_IMPLEMENTATION(setSessionFileName, void)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "setSessionFileName: nil file name");

        if (Session* s = Session::currentSession())
        {
            const string filename = pathConform(name->utf8std());
            s->setFileName(filename);
        }
    }

    NODE_IMPLEMENTATION(undoPathSwapVars, Pointer)
    {
        const StringType* t = static_cast<const StringType*>(NODE_THIS.type());
        const StringType::String* inS = NODE_ARG_OBJECT(0, StringType::String);

        if (!inS)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "input string is nil");

        string outS = IPCore::Application::mapFromVar(inS->c_str());

        NODE_RETURN(t->allocate(outS));
    }

    NODE_IMPLEMENTATION(redoPathSwapVars, Pointer)
    {
        const StringType* t = static_cast<const StringType*>(NODE_THIS.type());
        StringType::String* inS = NODE_ARG_OBJECT(0, StringType::String);

        if (!inS)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "input string is nil");

        string outS = IPCore::Application::mapToVar(inS->c_str());

        NODE_RETURN(t->allocate(outS));
    }

    NODE_IMPLEMENTATION(readProfile, void)
    {
        Session* s = Session::currentSession();
        const StringType::String* filename =
            NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* nodename =
            NODE_ARG_OBJECT(1, StringType::String);
        const bool usePath = NODE_ARG(2, bool);
        const StringType::String* tag = NODE_ARG_OBJECT(3, StringType::String);

        if (!nodename)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil node argument");
        if (!filename)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil file name argument");

        IPNode* node = s->graph().findNode(nodename->c_str());

        if (!node)
        {
            ostringstream str;
            string err;
            str << "No node with name \"" << nodename->c_str() << "\"";
            err = str.str();
            throwBadArgumentException(NODE_THIS, NODE_THREAD, err.c_str());
        }

        Document::ReadRequest request;

        if (usePath)
        {
            string tagStr = (tag) ? tag->c_str() : "";
            string profile = profileMatchingNameInPath(filename->c_str(),
                                                       tagStr, &s->graph());

            if (profile != "")
            {
                s->readProfile(profile, node, request);
            }
            else
            {
                ostringstream str;
                string err;
                str << "No profile with name \"" << filename->c_str()
                    << "\" found in profile path";
                err = str.str();
                throwBadArgumentException(NODE_THIS, NODE_THREAD, err.c_str());
            }
        }
        else
        {
            s->readProfile(filename->c_str(), node, request);
        }
    }

    NODE_IMPLEMENTATION(writeProfile, void)
    {
        Session* s = Session::currentSession();
        const StringType::String* filename =
            NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* nodename =
            NODE_ARG_OBJECT(1, StringType::String);
        const StringType::String* comments =
            NODE_ARG_OBJECT(2, StringType::String);

        if (!nodename)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil node argument");
        if (!filename)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil file name argument");

        IPNode* node = s->graph().findNode(nodename->c_str());

        if (!node)
        {
            ostringstream str;
            string err;
            str << "No node with name \"" << nodename->c_str() << "\"";
            err = str.str();
            throwBadArgumentException(NODE_THIS, NODE_THREAD, err.c_str());
        }

        Document::WriteRequest request;
        request.setOption("connections", true);
        request.setOption("membership", true);
        request.setOption("recursive", true);
        request.setOption("sparse", false);
        request.setOption("comments",
                          string(comments ? comments->c_str() : ""));

        try
        {
            s->writeProfile(filename->c_str(), node, request);
        }
        catch (TwkExc::Exception& exc)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, exc.what());
        }
        catch (...)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "failed");
        }
    }

    NODE_IMPLEMENTATION(saveSession, void)
    {
        Session* s = Session::currentSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        bool ascopy = NODE_ARG(1, bool);
        bool compressed = NODE_ARG(2, bool);
        bool sparse = NODE_ARG(3, bool);

        if (getenv("RV_COMPLETE_SESSION_FILES"))
            sparse = false;

        Document::WriteRequest request;
        request.setOption("tag", string(""));
        request.setOption("sparse", sparse);
        request.setOption("compressed", compressed);
        request.setOption("writeAsCopy", ascopy);
        request.setOption("partial", false);

        if (!name)
        {
            s->write(s->filePath().c_str(), request);
        }
        else
        {
            s->write(name->c_str(), request);
        }
    }

    NODE_IMPLEMENTATION(writeNodeDefinition, void)
    {
        Session* s = Session::currentSession();
        const StringType::String* typeName =
            NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* fileName =
            NODE_ARG_OBJECT(1, StringType::String);
        const NodeManager* nodeManager = s->graph().nodeManager();
        bool inlineSourceCode = NODE_ARG(2, bool);

        if (!typeName)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil typeName argument");
        }

        const NodeDefinition* def = nodeManager->definition(typeName->c_str());

        if (!def)
        {
            Mu::String str;
            str = "unknown node type '";
            str += typeName->c_str();
            str += "'";
            throwBadArgumentException(NODE_THIS, NODE_THREAD, str);
        }

        try
        {
            NodeManager::NodeDefinitionVector defs(1);
            defs[0] = def;
            nodeManager->writeDefinitions(fileName->c_str(), defs,
                                          inlineSourceCode);
        }
        catch (TwkExc::Exception& exc)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, exc.what());
        }
    }

    NODE_IMPLEMENTATION(writeAllNodeDefinitions, void)
    {
        Session* s = Session::currentSession();
        const StringType::String* fileName =
            NODE_ARG_OBJECT(0, StringType::String);
        const NodeManager* nodeManager = s->graph().nodeManager();
        bool inlineSource = NODE_ARG(1, bool);

        if (!fileName)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil fileName argument");
        }

        try
        {
            nodeManager->writeAllDefinitions(fileName->c_str(), inlineSource);
        }
        catch (TwkExc::Exception& exc)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, exc.what());
        }
    }

    NODE_IMPLEMENTATION(nodeTypes, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Session* s = Session::currentSession();
        const DynamicArrayType* type =
            (const DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);
        const StringType* stype = c->stringType();
        const bool userVisible = NODE_ARG(0, bool);

        const NodeManager* nodeManager = s->graph().nodeManager();
        const NodeManager::NodeDefinitionMap& map =
            nodeManager->definitionMap();

        std::vector<std::string> keys;
        for (NodeManager::NodeDefinitionMap::const_iterator i = map.begin();
             i != map.end(); ++i)
        {
            if (!userVisible || (userVisible && (*i).second->userVisible()))
            {
                keys.push_back((*i).first);
            }
        }

        array->resize(keys.size());
        for (int k = 0; k < keys.size(); k++)
        {
            array->element<Mu::StringType::String*>(k) =
                stype->allocate(keys[k]);
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(updateNodeDefinition, void)
    {
        Session* s = Session::currentSession();
        const StringType::String* nodeType =
            NODE_ARG_OBJECT(0, StringType::String);
        NodeManager* nodeManager = IPCore::App()->nodeManager();
        const NodeDefinition* def = nodeManager->definition(nodeType->c_str());

        if (!def)
        {
            ostringstream str;
            str << "\"" << nodeType->c_str() << "\" is not a node type";
            string s = str.str(); // windows
            throwBadArgumentException(NODE_THIS, NODE_THREAD, s.c_str());
        }

        try
        {
            if (!def->function() || def->function()->originalSource() == "")
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "bad arg to rebuildShader");
            }

            IPGraph::GraphEdit edit(s->graph());

            if (!nodeManager->updateDefinition(def->name()))
            {
                TWK_THROW_EXC_STREAM(
                    "updateNodeDefinition() failed on: " << def->name());
            }
        }
        catch (TwkExc::Exception& exc)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, exc.what());
        }
    }

    NODE_IMPLEMENTATION(clearSession, void)
    {
        Session* s = Session::currentSession();

        IPMu::RemoteRvCommand remoteRvCommand(s, "clearSession");

        s->clear();
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(newSession, void)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        DynamicArray* newValues = NODE_ARG_OBJECT(0, DynamicArray);
        const Class* stype = c->stringType();

        if (s->multipleVideoDevices())
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "newSessions cannot be created when "
                                      "presentation mode is active");
        }

        vector<string> files;

        if (newValues)
        {
            files.resize(newValues->size());

            for (int i = 0; i < newValues->size(); i++)
            {
                StringType::String* s =
                    newValues->element<StringType::String*>(i);
                files[i] = s->c_str();
            }
        }

        IPCore::App()->createNewSessionFromFiles(files);
    }

    NODE_IMPLEMENTATION(getFloatProperty, Pointer)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int start = NODE_ARG(1, int);
        int num = NODE_ARG(2, int);

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        Session::Property* prop = props.front();
        size_t width = prop->xsizeTrait();

        if (prop->layoutTrait() == Property::FloatLayout)
        {
            float* data =
                prop->empty() ? 0 : reinterpret_cast<float*>(prop->rawData());
            DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
            DynamicArray* array = new DynamicArray(type, 1);
            size_t s = prop->size() * width;
            array->resize(min(s, num * width));
            int start0 = start * width;

            for (int i = start0; i < start0 + array->size(); i++)
            {
                array->element<float>(i - start0) = data[i];
            }

            NODE_RETURN(array);
        }
        else
        {
            throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(setFloatProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        bool allowResize = NODE_ARG(2, bool);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);

        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            Property* prop = props[i];
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();

            if (prop->layoutTrait() != Property::FloatLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (!allowResize && newValues->size() != prop->size() * width)
            {
                throwBadArgumentException(
                    NODE_THIS, NODE_THREAD,
                    "number of values does not match property size");
            }

            if (node)
                node->propertyWillChange(prop);
            if (allowResize)
                prop->resize(newValues->size() / width);

            const float* data = newValues->data<float>();
            float* pdata = reinterpret_cast<float*>(prop->rawData());
            copy(data, data + prop->size() * width, pdata);

            if (node)
                node->propertyChanged(prop);

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(insertFloatProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        int index = NODE_ARG(2, int);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);

        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            FloatProperty* prop = static_cast<FloatProperty*>(props[i]);
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();
            size_t insertIndex =
                std::min(std::max(0, index), int(prop->size())) * width;

            if (prop->layoutTrait() != Property::FloatLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (node)
            {
                node->propertyWillInsert(prop, size_t(insertIndex / width),
                                         newValues->size() / width);
                node->propertyWillChange(prop);
            }

            prop->valueContainer().insert(prop->begin() + insertIndex,
                                          newValues->begin<float>(),
                                          newValues->end<float>());

            if (node)
            {
                node->propertyDidInsert(prop, size_t(insertIndex / width),
                                        newValues->size() / width);

                node->propertyChanged(prop);
            }

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(getHalfProperty, Pointer)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int start = NODE_ARG(1, int);
        int num = NODE_ARG(2, int);

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        Session::Property* prop = props.front();
        size_t width = prop->xsizeTrait();

        if (prop->layoutTrait() == Property::HalfLayout)
        {
            half* data =
                prop->empty() ? 0 : reinterpret_cast<half*>(prop->rawData());
            DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
            DynamicArray* array = new DynamicArray(type, 1);
            size_t s = prop->size() * width;
            array->resize(min(s, num * width));
            int start0 = start * width;

            for (int i = start0; i < start0 + array->size(); i++)
            {
                array->element<half>(i - start0) = data[i];
            }

            NODE_RETURN(array);
        }
        else
        {
            throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(setHalfProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        bool allowResize = NODE_ARG(2, bool);
        Session* session = Session::currentSession();

        PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);

        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            Property* prop = props[i];
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();

            if (prop->layoutTrait() != Property::HalfLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (!allowResize && newValues->size() != prop->size() * width)
            {
                throwBadArgumentException(
                    NODE_THIS, NODE_THREAD,
                    "number of values does not match property size");
            }

            if (node)
                node->propertyWillChange(prop);
            if (allowResize)
                prop->resize(newValues->size() / width);

            const half* data = newValues->data<half>();
            half* pdata = reinterpret_cast<half*>(prop->rawData());
            copy(data, data + prop->size() * width, pdata);

            if (node)
                node->propertyChanged(prop);

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(insertHalfProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        int index = NODE_ARG(2, int);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);

        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            HalfProperty* prop = static_cast<HalfProperty*>(props[i]);
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();
            size_t insertIndex =
                std::min(std::max(0, index), int(prop->size())) * width;

            if (prop->layoutTrait() != Property::HalfLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (node)
            {
                node->propertyWillInsert(prop, size_t(insertIndex / width),
                                         newValues->size() / width);
                node->propertyWillChange(prop);
            }

            prop->valueContainer().insert(prop->begin() + insertIndex,
                                          newValues->begin<half>(),
                                          newValues->end<half>());

            if (node)
            {
                node->propertyDidInsert(prop, size_t(insertIndex / width),
                                        newValues->size() / width);

                node->propertyChanged(prop);
            }

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(getIntProperty, Pointer)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int start = NODE_ARG(1, int);
        int num = NODE_ARG(2, int);

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        Session::Property* prop = props.front();
        size_t width = prop->xsizeTrait();

        if (prop->layoutTrait() == Property::IntLayout)
        {
            int* data =
                prop->empty() ? 0 : reinterpret_cast<int*>(prop->rawData());
            DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
            DynamicArray* array = new DynamicArray(type, 1);
            size_t s = prop->size() * width;
            array->resize(min(s, num * width));
            int start0 = start * width;

            for (int i = start0; i < start0 + array->size(); i++)
            {
                array->element<int>(i - start0) = data[i];
            }

            NODE_RETURN(array);
        }
        else
        {
            throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(setIntProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        bool allowResize = NODE_ARG(2, bool);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            Property* prop = props[i];
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();

            if (prop->layoutTrait() != Property::IntLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (!allowResize && newValues->size() != prop->size() * width)
            {
                throwBadArgumentException(
                    NODE_THIS, NODE_THREAD,
                    "number of values does not match property size");
            }

            if (node)
                node->propertyWillChange(prop);
            if (allowResize)
                prop->resize(newValues->size() / width);

            const int* data = newValues->data<int>();
            int* pdata = reinterpret_cast<int*>(prop->rawData());
            copy(data, data + prop->size() * width, pdata);

            if (node)
                node->propertyChanged(prop);

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(insertIntProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        int index = NODE_ARG(2, int);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            IntProperty* prop = static_cast<IntProperty*>(props[i]);
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();
            size_t insertIndex =
                std::min(std::max(0, index), int(prop->size())) * width;

            if (prop->layoutTrait() != Property::IntLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (node)
            {
                node->propertyWillInsert(prop, size_t(insertIndex / width),
                                         newValues->size() / width);
                node->propertyWillChange(prop);
            }

            prop->valueContainer().insert(prop->begin() + insertIndex,
                                          newValues->begin<int>(),
                                          newValues->end<int>());

            if (node)
            {
                node->propertyDidInsert(prop, size_t(insertIndex / width),
                                        newValues->size() / width);

                node->propertyChanged(prop);
            }

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(getByteProperty, Pointer)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int start = NODE_ARG(1, int);
        int num = NODE_ARG(2, int);

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        Session::Property* prop = props.front();
        size_t width = prop->xsizeTrait();

        if (prop->layoutTrait() == Property::ByteLayout)
        {
            unsigned char* data =
                prop->empty()
                    ? 0
                    : reinterpret_cast<unsigned char*>(prop->rawData());
            DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
            DynamicArray* array = new DynamicArray(type, 1);
            size_t s = prop->size() * width;
            array->resize(min(s, num * width));
            int start0 = start * width;

            for (int i = start0; i < start0 + array->size(); i++)
            {
                array->element<unsigned char>(i - start0) = data[i];
            }

            NODE_RETURN(array);
        }
        else
        {
            throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(setByteProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        bool allowResize = NODE_ARG(2, bool);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            Property* prop = props[i];
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();

            if (prop->layoutTrait() != Property::ByteLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (!allowResize && newValues->size() != prop->size() * width)
            {
                throwBadArgumentException(
                    NODE_THIS, NODE_THREAD,
                    "number of values does not match property size");
            }

            if (node)
                node->propertyWillChange(prop);
            if (allowResize)
                prop->resize(newValues->size() / width);

            const unsigned char* data = newValues->data<unsigned char>();
            unsigned char* pdata =
                reinterpret_cast<unsigned char*>(prop->rawData());
            copy(data, data + prop->size() * width, pdata);

            if (node)
                node->propertyChanged(prop);

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(insertByteProperty, void)
    {
        Process* p = NODE_THREAD.process();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        int index = NODE_ARG(2, int);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            ByteProperty* prop = static_cast<ByteProperty*>(props[i]);
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();
            size_t insertIndex =
                std::min(std::max(0, index), int(prop->size())) * width;

            if (prop->layoutTrait() != Property::ByteLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (node)
            {
                node->propertyWillInsert(prop, size_t(insertIndex / width),
                                         newValues->size() / width);
                node->propertyWillChange(prop);
            }

            prop->valueContainer().insert(prop->begin() + insertIndex,
                                          newValues->begin<unsigned char>(),
                                          newValues->end<unsigned char>());

            if (node)
            {
                node->propertyDidInsert(prop, size_t(insertIndex / width),
                                        newValues->size() / width);

                node->propertyChanged(prop);
            }

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(getStringProperty, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = (MuLangContext*)NODE_THREAD.context();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int start = NODE_ARG(1, int);
        int num = NODE_ARG(2, int);

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        Session::Property* prop = props.front();
        size_t width = prop->xsizeTrait();

        if (prop->layoutTrait() == Property::StringLayout)
        {
            string* data =
                prop->empty() ? 0 : reinterpret_cast<string*>(prop->rawData());
            DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
            DynamicArray* array = new DynamicArray(type, 1);
            size_t s = prop->size() * width;
            array->resize(min(s, num * width));
            int start0 = start * width;

            for (int i = start0; i < start0 + array->size(); i++)
            {
                array->element<StringType::String*>(i - start0) =
                    c->stringType()->allocate(data[i]);
            }

            NODE_RETURN(array);
        }
        else
        {
            throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(setStringProperty, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues = NODE_ARG_OBJECT(1, DynamicArray);
        bool allowResize = NODE_ARG(2, bool);
        Session* session = Session::currentSession();

        PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        IPGraph::GraphEdit edit(session->graph(), props);

        for (int i = 0; i < props.size(); i++)
        {
            Property* prop = props[i];
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();

            if (prop->layoutTrait() != Property::StringLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (!allowResize && newValues->size() != prop->size() * width)
            {
                throwBadArgumentException(
                    NODE_THIS, NODE_THREAD,
                    "number of values does not match property size");
            }

            bool throwBad = false;
            if (node)
                node->propertyWillChange(prop);
            if (allowResize)
                prop->resize(newValues->size());

            if (StringProperty* sp = static_cast<StringProperty*>(prop))
            {
                for (int i = 0; i < newValues->size(); i++)
                {
                    Mu::StringType::String* s =
                        newValues->element<StringType::String*>(i);
                    (*sp)[i] = (s) ? s->c_str() : "<nil>";
                }
            }
            else if (StringPairProperty* spp =
                         static_cast<StringPairProperty*>(prop))
            {
                for (int i = 0; i < newValues->size(); i += 2)
                {
                    Mu::StringType::String* s0 =
                        newValues->element<StringType::String*>(i);
                    Mu::StringType::String* s1 =
                        newValues->element<StringType::String*>(i + 1);

                    string cstr0 = s0 ? s0->c_str() : "<nil>";
                    string cstr1 = s1 ? s1->c_str() : "<nil>";

                    (*spp)[i] = TwkContainer::StringPair(cstr0, cstr1);
                }
            }
            else
            {
                throwBad = true;
            }

            if (node)
                node->propertyChanged(prop);
            if (throwBad)
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }

        session->forceNextEvaluation();
    }

    NODE_IMPLEMENTATION(insertStringProperty, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* newValues0 = NODE_ARG_OBJECT(1, DynamicArray);
        int index = NODE_ARG(2, int);
        Session* session = Session::currentSession();

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        IPGraph::GraphEdit edit(session->graph(), props);

        vector<string> newValues(newValues0->size());
        for (size_t i = 0; i < newValues0->size(); i++)
        {
            newValues[i] = newValues0->element<StringType::String*>(i)->c_str();
        }

        for (int i = 0; i < props.size(); i++)
        {
            StringProperty* prop = static_cast<StringProperty*>(props[i]);
            IPNode* node = static_cast<IPNode*>(prop->container());
            size_t width = prop->xsizeTrait();
            size_t insertIndex =
                std::min(std::max(0, index), int(prop->size())) * width;

            if (prop->layoutTrait() != Property::StringLayout)
            {
                throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
            }

            if (node)
            {
                node->propertyWillInsert(prop, size_t(insertIndex / width),
                                         newValues.size() / width);
                node->propertyWillChange(prop);
            }

            prop->valueContainer().insert(prop->begin() + insertIndex,
                                          newValues.begin(), newValues.end());

            if (node)
            {
                node->propertyDidInsert(prop, size_t(insertIndex / width),
                                        newValues.size() / width);

                node->propertyChanged(prop);
            }

            session->forceNextEvaluation();
        }
    }

    NODE_IMPLEMENTATION(fileKind, int)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil file");

        NODE_RETURN(int(IPCore::App()->fileKind(name->c_str())));
    }

    NODE_IMPLEMENTATION(setFiltering, void)
    {
        Process* p = NODE_THREAD.process();
        Session* s = Session::currentSession();
        int filter = NODE_ARG(0, int);

        if (filter != GL_LINEAR && filter != GL_NEAREST)
        {
            throwBadArgumentException(
                NODE_THIS, NODE_THREAD,
                "setFiltering only accepts GL_LINEAR or GL_NEAREST");
        }

        if (s->renderer())
        {
            s->renderer()->setFiltering(filter);
        }
    }

    NODE_IMPLEMENTATION(getFiltering, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->renderer()->filterType());
    }

    NODE_IMPLEMENTATION(viewSize, Vector2f)
    {
        Session* s = Session::currentSession();
        Vector2f v = {static_cast<float>(s->eventVideoDevice()->width()),
                      static_cast<float>(s->eventVideoDevice()->height())};
        NODE_RETURN(v);
    }

    NODE_IMPLEMENTATION(bgMethod, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const StringType* stype = c->stringType();

        const char* n = "";

        if (s && s->renderer())
        {
            switch (s->renderer()->bgPattern())
            {
            default:
            case ImageRenderer::Solid0:
                n = "black";
                break;
            case ImageRenderer::Solid100:
                n = "white";
                break;
            case ImageRenderer::Solid18:
                n = "grey18";
                break;
            case ImageRenderer::Solid50:
                n = "grey50";
                break;
            case ImageRenderer::Checker:
                n = "checker";
                break;
            case ImageRenderer::CrossHatch:
                n = "crosshatch";
                break;
            }
        }

        NODE_RETURN(stype->allocate(n));
    }

    NODE_IMPLEMENTATION(setBGMethod, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const Class* stype = c->stringType();
        const StringType::String* method =
            NODE_ARG_OBJECT(0, StringType::String);

        if (!method)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil bg method");

        ImageRenderer::BGPattern bg = ImageRenderer::Solid0;

        if (*method == "black")
        {
            bg = ImageRenderer::Solid0;
        }
        else if (*method == "white")
        {
            bg = ImageRenderer::Solid100;
        }
        else if (*method == "grey18")
        {
            bg = ImageRenderer::Solid18;
        }
        else if (*method == "grey50")
        {
            bg = ImageRenderer::Solid50;
        }
        else if (*method == "checker")
        {
            bg = ImageRenderer::Checker;
        }
        else if (*method == "crosshatch")
        {
            bg = ImageRenderer::CrossHatch;
        }

        if (s && s->renderer())
        {
            s->setRendererBGType(bg);
        }
    }

    NODE_IMPLEMENTATION(setRendererType, void)
    {
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        s->setRendererType(name->c_str());
    }

    NODE_IMPLEMENTATION(getRendererType, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();

        StringType::String* n =
            c->stringType()->allocate(s->renderer()->name());

        NODE_RETURN(n);
    }

    NODE_IMPLEMENTATION(setSessionType, void)
    {
        Session* s = Session::currentSession();
        int type = NODE_ARG(0, int);
        s->setSessionType((Session::SessionType)type);
    }

    NODE_IMPLEMENTATION(getSessionType, int)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(int(s->getSessionType()));
    }

    NODE_IMPLEMENTATION(setHardwareStereoMode, void)
    {
        Session* s = Session::currentSession();
        bool on = NODE_ARG(0, bool);

        if (on)
        {
            s->send(Session::stereoHardwareOnMessage());
        }
        else
        {
            s->send(Session::stereoHardwareOffMessage());
        }
    }

    NODE_IMPLEMENTATION(scrubAudio, void)
    {
        Session* s = Session::currentSession();
        bool on = NODE_ARG(0, bool);
        float duration = NODE_ARG(1, float);
        int count = NODE_ARG(2, int);

        s->scrubAudio(on, duration, count);
    }

    namespace
    {

        struct DimTuple
        {
            int x, y, z, w;
        };

        void newProp(const Mu::Node& node, Mu::Thread& thread, Process* p,
                     Session* s, StringType::String* name, int type,
                     const Vec4i& dimensions)
        {
            if (!name)
                throwBadArgumentException(node, thread, "nil property name");

            vector<string> tokens;
            stl_ext::tokenize(tokens, string(name->c_str()), ".");

            if (tokens.size() != 3 || tokens[0].size() < 2
                || tokens[1].size() < 1 || tokens[2].size() < 1)
            {
                throwBadArgumentException(node, thread,
                                          "malformed property name");
            }

            IPGraph& graph = s->graph();
            IPNode* object = graph.findNode(tokens[0]);

            if (!object)
            {
                IPGraph::NodeVector nodes;
                string typeName = tokens[0].substr(1, tokens[0].size());
                graph.findNodesByTypeName(s->currentFrame(), nodes, typeName);

                if (nodes.empty())
                {
                    ostringstream str;
                    str << "no node of type '" << typeName << "' found";
                    string s = str.str();
                    throwBadArgumentException(node, thread, s.c_str());
                }
                else if (nodes.size() == 1)
                {
                    object = nodes.front();
                }
                else
                {
                    ostringstream str;
                    str << "more than one node of type '" << typeName
                        << "' found";
                    string s = str.str();
                    throwBadArgumentException(node, thread, s.c_str());
                }
            }

            if (!object)
            {
                throwBadArgumentException(node, thread, "not an object");
            }

            Property* prop = 0;

            switch (Property::Layout(type))
            {
            case Property::IntLayout:
                switch (dimensions.x)
                {
                case 1:
                    prop = object->createProperty<IntProperty>(tokens[1],
                                                               tokens[2]);
                    break;
                case 2:
                    prop = object->createProperty<Vec2iProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 3:
                    prop = object->createProperty<Vec3iProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 4:
                    prop = object->createProperty<Vec4iProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                }
                break;
            case Property::FloatLayout:
                switch (dimensions.x)
                {
                case 1:
                    prop = object->createProperty<FloatProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 2:
                    prop = object->createProperty<Vec2fProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 3:
                    prop = object->createProperty<Vec3fProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 4:
                    prop = object->createProperty<Vec4fProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                default:
                    break;
                }
                break;
            case Property::HalfLayout:
                switch (dimensions.x)
                {
                case 1:
                    prop = object->createProperty<HalfProperty>(tokens[1],
                                                                tokens[2]);
                    break;
                case 2:
                    prop = object->createProperty<Vec2hProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 3:
                    prop = object->createProperty<Vec3hProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 4:
                    prop = object->createProperty<Vec4hProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                default:
                    break;
                }
                break;
            case Property::StringLayout:
                prop = object->createProperty<StringProperty>(tokens[1],
                                                              tokens[2]);
                break;
            case Property::ByteLayout:
                switch (dimensions.x)
                {
                default:
                case 1:
                    prop = object->createProperty<ByteProperty>(tokens[1],
                                                                tokens[2]);
                    break;
                case 2:
                    prop = object->createProperty<Vec2ucProperty>(tokens[1],
                                                                  tokens[2]);
                    break;
                case 3:
                    prop = object->createProperty<Vec3ucProperty>(tokens[1],
                                                                  tokens[2]);
                    break;
                case 4:
                    prop = object->createProperty<Vec4ucProperty>(tokens[1],
                                                                  tokens[2]);
                    break;
                }
                break;
            case Property::ShortLayout:
                switch (dimensions.x)
                {
                case 1:
                    prop = object->createProperty<ShortProperty>(tokens[1],
                                                                 tokens[2]);
                    break;
                case 2:
                    prop = object->createProperty<Vec2usProperty>(tokens[1],
                                                                  tokens[2]);
                    break;
                case 3:
                    prop = object->createProperty<Vec3usProperty>(tokens[1],
                                                                  tokens[2]);
                    break;
                case 4:
                    prop = object->createProperty<Vec4usProperty>(tokens[1],
                                                                  tokens[2]);
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }

            if (object)
                object->newPropertyCreated(prop);
        }

    } // namespace

    NODE_IMPLEMENTATION(newProperty, void)
    {
        Process* p = NODE_THREAD.process();
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int type = NODE_ARG(1, int);
        int width = NODE_ARG(2, int);

        newProp(NODE_THIS, NODE_THREAD, p, s, name, type,
                Vec4i(width, 0, 0, 0));
    }

    NODE_IMPLEMENTATION(newProperty2, void)
    {
        Process* p = NODE_THREAD.process();
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int type = NODE_ARG(1, int);
        ClassInstance* dims = NODE_ARG_OBJECT(2, ClassInstance);
        DimTuple* tuple = dims->data<DimTuple>();

        newProp(NODE_THIS, NODE_THREAD, p, s, name, type,
                Vec4i(tuple->x, tuple->y, tuple->z, tuple->w));
    }

    NODE_IMPLEMENTATION(deleteProperty, void)
    {
        Process* p = NODE_THREAD.process();
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil property name");

        Session::PropertyVector props;

        vector<string> tokens;
        stl_ext::tokenize(tokens, string(name->c_str()), ".");

        if (tokens.size() != 3 || tokens[0].size() < 2 || tokens[1].size() < 1
            || tokens[2].size() < 1)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "malformed property name");
        }

        IPGraph& graph = s->graph();
        IPNode* object = graph.findNode(tokens[0]);

        if (!object)
        {
            IPGraph::NodeVector nodes;
            graph.findNodesByTypeName(s->currentFrame(), nodes,
                                      tokens[0].substr(1, tokens[0].size()));

            if (nodes.size() == 1)
            {
                object = nodes.front();
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "node name is ambigious");
            }
        }

        if (!object)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "not an object");
        }

        getProperty(props, NODE_THREAD, NODE_THIS, name);

        for (size_t i = 0; i < props.size(); i++)
        {
            Property* p = props[i];
            if (object)
                object->propertyWillBeDeleted(p);

            //  Causes unref which causes deletion if required
            if (p->container())
                p->container()->removeProperty(p);
        }

        if (object)
            object->propertyDeleted(name->c_str());
    }

    namespace
    {

        struct PropInfo
        {
            Pointer name;
            int type;
            Pointer dimensions;
            int size;
            bool userDefined;
            Pointer info;
        };

    } // namespace

    NODE_IMPLEMENTATION(propertyInfo, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const Mu::StringType::String* name =
            NODE_ARG_OBJECT(0, StringType::String);
        const Mu::StringType* stype =
            static_cast<const StringType*>(name->type());
        const Mu::Class* type = static_cast<const Class*>(NODE_THIS.type());

        Session::PropertyVector props;
        getProperty(props, NODE_THREAD, NODE_THIS, name);
        ClassInstance* obj = 0;

        if (props.empty())
        {
            throwBadPropertyType(NODE_THREAD, NODE_THIS, name->c_str());
        }
        else
        {
            Property* p = props.front();

            Context::TypeVector types(4);
            types[0] = c->intType();
            types[1] = c->intType();
            types[2] = c->intType();
            types[3] = c->intType();

            const TupleType* ttype = c->tupleType(types);
            ClassInstance* dims = ClassInstance::allocate(ttype);
            DimTuple* di = dims->data<DimTuple>();
            di->x = p->xsizeTrait();
            di->y = p->ysizeTrait();
            di->z = p->zsizeTrait();
            di->w = p->wsizeTrait();

            obj = ClassInstance::allocate(type);
            PropInfo* pi = obj->data<PropInfo>();

            pi->name = Pointer(name);
            pi->type = p->layoutTrait();
            pi->dimensions = Pointer(dims);
            pi->size = p->size();
            pi->userDefined = false;
            pi->info = 0;
        }

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(nextUIName, Pointer)
    {
        Mu::StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const Mu::StringType* stype =
            static_cast<const StringType*>(name->type());
        const Mu::Class* type = static_cast<const Class*>(NODE_THIS.type());

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil property name");

        Session::PropertyVector props;
        Session* session = Session::currentSession();
        std::string newName = session->nextUIName(name->c_str());

        return stype->allocate(newName);
    }

    NODE_IMPLEMENTATION(propertyExists, bool)
    {
        Mu::StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const Mu::StringType* stype =
            static_cast<const StringType*>(name->type());
        const Mu::Class* type = static_cast<const Class*>(NODE_THIS.type());

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil property name");

        Session::PropertyVector props;
        Session* session = Session::currentSession();
        session->findProperty(props, name->c_str());
        NODE_RETURN(!props.empty());
    }

    NODE_IMPLEMENTATION(properties, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Session* s = Session::currentSession();
        Mu::StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const DynamicArrayType* type =
            (const DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);
        const StringType* stype = c->stringType();

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        typedef const TwkContainer::PropertyContainer::Components Components;
        typedef const TwkContainer::Component::Properties Properties;
        typedef const TwkContainer::Property Property;

        vector<string> allprops;

        IPNode* node = 0;

        if ((node = s->graph().findNode(name->c_str())))
        {
        }
        else
        {
            IPGraph::NodeVector nodes;
            s->graph().findNodesByTypeName(s->currentFrame(), nodes,
                                           name->c_str() + 1);

            if (!nodes.empty())
            {
                if (nodes.size() == 1)
                {
                    node = nodes.front();
                }
                else
                {
                    ostringstream str;
                    str << "ambiguous: too many nodes of type " << name->c_str()
                        << ":";
                    for (size_t i = 0; i < nodes.size(); i++)
                        str << " " << nodes[i]->name();
                    throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                              str.str().c_str());
                }
            }
        }

        if (node)
        {
            const Components& comps = node->components();

            for (size_t i = 0; i < comps.size(); i++)
            {
                Component* comp = comps[i];
                const Properties& props = comp->properties();

                for (size_t q = 0; q < props.size(); q++)
                {
                    Property* p = props[q];
                    ostringstream n;
                    n << node->name() << "." << comp->name() << "."
                      << p->name();
                    allprops.push_back(n.str());
                }
            }
        }
        else
        {
            ostringstream str;
            str << "no nodes found: " << name->c_str() << endl;
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      str.str().c_str());
        }

        array->resize(allprops.size());

        for (size_t i = 0; i < allprops.size(); i++)
        {
            array->element<Mu::StringType::String*>(i) =
                stype->allocate(allprops[i]);
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(nodes, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Session* s = Session::currentSession();
        const DynamicArrayType* type =
            (const DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);
        const StringType* stype = c->stringType();

        const IPGraph::NodeMap& map = s->graph().nodeMap();

        array->resize(map.size());
        size_t count = 0;

        for (IPGraph::NodeMap::const_iterator i = map.begin(); i != map.end();
             ++i, count++)
        {
            array->element<Mu::StringType::String*>(count) =
                stype->allocate((*i).first);
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(nodeType, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Session* s = Session::currentSession();
        StringType::String* node = NODE_ARG_OBJECT(0, StringType::String);
        const StringType* stype = c->stringType();
        StringType::String* type = 0;

        if (!node)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        if (IPNode* n = s->graph().findNode(node->c_str()))
        {
            type = stype->allocate(n->protocol());
        }
        else
        {
            ostringstream str;
            str << "\"" << node->c_str() << "\" is not a node";
            string s = str.str();
            throwBadArgumentException(NODE_THIS, NODE_THREAD, s.c_str());
        }

        NODE_RETURN(type);
    }

    NODE_IMPLEMENTATION(deleteNode, void)
    {
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        if (s->graph().isDefaultView(name->c_str()))
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "can't delete default views");
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            s->deleteNode(node);
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad node name");
        }
    }

    NODE_IMPLEMENTATION(sendInternalEvent, Pointer)
    {
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* contents = NODE_ARG_OBJECT(1, StringType::String);
        StringType::String* sender = NODE_ARG_OBJECT(2, StringType::String);
        const StringType* stype = static_cast<const StringType*>(name->type());

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "no event name specified");
        }

        string r = s->userGenericEvent(name->c_str(),
                                       contents ? contents->c_str() : "",
                                       sender ? sender->c_str() : "");

        NODE_RETURN(stype->allocate(r));
    }

    NODE_IMPLEMENTATION(setFilterLiveReviewEvents, void)
    {
        Session* s = Session::currentSession();
        bool shouldFilterEvents = NODE_ARG(0, bool);

        s->setFilterLiveReviewEvents(shouldFilterEvents);
    }

    NODE_IMPLEMENTATION(filterLiveReviewEvents, bool)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->filterLiveReviewEvents());
    }

    NODE_IMPLEMENTATION(nextViewNode, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        string next = s->nextViewNode();

        if (next.empty())
            NODE_RETURN(0);
        else
            NODE_RETURN(c->stringType()->allocate(next));
    }

    NODE_IMPLEMENTATION(previousViewNode, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        string prev = s->previousViewNode();

        if (prev.empty())
            NODE_RETURN(0);
        else
            NODE_RETURN(c->stringType()->allocate(prev));
    }

    NODE_IMPLEMENTATION(setViewNode, void)
    {
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil view name");

        bool playing = s->isPlaying();
        if (playing)
            s->stop();
        if (!s->setViewNode(name->c_str()))
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad node name");
        }
        if (s->audioCachingMode() == Session::GreedyCache)
        {
            s->play();
            s->stop();
            s->setAudioCaching(Session::BufferCache);
            s->setAudioCaching(Session::GreedyCache);
        }
        if (playing)
            s->play();
    }

    NODE_IMPLEMENTATION(viewNodes, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArrayType* type =
            (const DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);
        const StringType* stype =
            static_cast<const StringType*>(type->elementType());

        const IPGraph::NodeMap& nodes = s->graph().viewableNodes();
        array->resize(nodes.size());
        size_t count = 0;

        for (IPGraph::NodeMap::const_iterator i = nodes.begin();
             i != nodes.end(); ++i)
        {
            array->element<StringType::String*>(count++) =
                stype->allocate(i->second->name());
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(viewNode, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        StringType::String* str = 0;

        if (IPNode* node = s->graph().viewNode())
        {
            str = c->stringType()->allocate(node->name());
        }

        NODE_RETURN(str);
    }

    struct StringArrayPairTuple
    {
        ClassInstance* _0;
        ClassInstance* _1;
    };

    NODE_IMPLEMENTATION(nodeConnections, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const TupleType* type = (const TupleType*)NODE_THIS.type();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        bool traverseGroups = NODE_ARG(1, bool);

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "no node name specified");
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            IPNode::IPNodes nodeIns = node->inputs();
            IPNode::IPNodes nodeOuts = node->outputs();

            if (nodeOuts.empty() && node->group())
            {
                nodeOuts = node->group()->outputs();
            }

            IPNode::IPNodes ins;
            IPNode::IPNodes outs;

            if (traverseGroups)
            {
                for (size_t i = 0; i < nodeIns.size(); i++)
                {
                    if (GroupIPNode* group =
                            dynamic_cast<GroupIPNode*>(nodeIns[i]))
                    {
                        while (dynamic_cast<GroupIPNode*>(group->rootNode()))
                            group =
                                static_cast<GroupIPNode*>(group->rootNode());

                        ins.push_back(group->rootNode());
                    }
                    else
                    {
                        ins.push_back(nodeIns[i]);
                    }
                }

                for (size_t i = 0; i < nodeOuts.size(); i++)
                {
                    if (GroupIPNode* group =
                            dynamic_cast<GroupIPNode*>(nodeOuts[i]))
                    {
                        IPNode::IPNodes onodes;
                        group->internalOutputNodesFor(node, onodes);

                        // cout << "node " << node->name() << " gets outputs
                        // from "
                        //<< group->name()
                        //<< endl;

                        for (size_t q = 0; q < onodes.size(); q++)
                        {
                            outs.push_back(onodes[q]);
                        }
                    }
                    else
                    {
                        outs.push_back(nodeOuts[i]);
                    }
                }
            }
            else
            {
                ins = nodeIns;
                outs = nodeOuts;
            }

            const Class* atype =
                static_cast<const Class*>(c->arrayType(c->stringType(), 1, 0));

            DynamicArray* inarray = new DynamicArray(atype, 1);
            DynamicArray* outarray = new DynamicArray(atype, 1);

            inarray->resize(ins.size());
            outarray->resize(outs.size());

            for (size_t i = 0; i < ins.size(); i++)
            {
                inarray->element<StringType::String*>(i) =
                    c->stringType()->allocate(ins[i]->name());
            }

            for (size_t i = 0; i < outs.size(); i++)
            {
                outarray->element<StringType::String*>(i) =
                    c->stringType()->allocate(outs[i]->name());
            }

            ClassInstance* tobj = ClassInstance::allocate(type);
            StringArrayPairTuple* t = tobj->data<StringArrayPairTuple>();
            t->_0 = inarray;
            t->_1 = outarray;

            NODE_RETURN(tobj);
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(nodesInGroup, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const TupleType* type = (const TupleType*)NODE_THIS.type();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "no node name specified");
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            if (GroupIPNode* group = dynamic_cast<GroupIPNode*>(node))
            {
                const IPNode::IPNodeSet& members = group->members();
                const Class* atype = static_cast<const Class*>(
                    c->arrayType(c->stringType(), 1, 0));
                DynamicArray* outarray = new DynamicArray(atype, 1);

                outarray->resize(members.size());
                int count = 0;

                // Compile a list of the IPNodes sorted by name.
                // Note: 'members' is an std::set of IPNodes which are
                // inherently 'ordered' because an std::set is ordered. However
                // it is ordered by memory addess because this is what the
                // std::set contains. We are sorting them by names here before
                // returning that list

                // Compile the list of node names
                std::vector<std::string> node_names;
                std::for_each(members.cbegin(), members.cend(),
                              [&](const IPNode* node)
                              { node_names.push_back(node->name()); });

                // Sort the node names
                std::sort(node_names.begin(), node_names.end());

                // Initialize an outarray with the sorted node names
                std::for_each(node_names.begin(), node_names.end(),
                              [&](const std::string& node_name)
                              {
                                  outarray->element<StringType::String*>(
                                      count) =
                                      c->stringType()->allocate(node_name);
                                  count++;
                              });

                NODE_RETURN(outarray);
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "node name specified is not a group");
            }
        }

        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(nodeGroup, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const TupleType* type = (const TupleType*)NODE_THIS.type();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "no node name specified");
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            if (GroupIPNode* group = node->group())
            {
                NODE_RETURN(c->stringType()->allocate(group->name()));
            }
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(nodeGroupRoot, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const TupleType* type = (const TupleType*)NODE_THIS.type();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "no node name specified");
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            if (GroupIPNode* group = dynamic_cast<GroupIPNode*>(node))
            {
                NODE_RETURN(
                    c->stringType()->allocate(group->rootNode()->name()));
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "node name specified is not a group");
            }
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(nodeExists, bool)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        bool exists = name && s->graph().findNode(name->c_str());
        NODE_RETURN(exists);
    }

    NODE_IMPLEMENTATION(nodeImageGeometry, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int frame = NODE_ARG(1, int);
        const Class* rtype = static_cast<const Class*>(NODE_THIS.type());

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            struct InfoStruct
            {
                int w;
                int h;
                float pa;
                Pointer M;
            };

            ClassInstance* obj = ClassInstance::allocate(rtype);

            IPNode::ImageStructureInfo info =
                node->imageStructureInfo(s->graph().contextForFrame(frame));
            InfoStruct* s = reinterpret_cast<InfoStruct*>(obj->structure());
            s->w = info.width;
            s->h = info.height;
            s->pa = info.pixelAspect;

            const Class* m44 = static_cast<const Class*>(
                c->arrayType(c->floatType(), 2, 4, 4, 0));
            FixedArray* marray =
                static_cast<FixedArray*>(ClassInstance::allocate(m44));
            memcpy(marray->data<float>(), &info.orientation,
                   sizeof(float) * 16);

            s->M = marray;

            NODE_RETURN(obj);
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "node name not found");
        }

        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(nodeRangeInfo, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const Class* rtype = static_cast<const Class*>(NODE_THIS.type());

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            struct InfoStruct
            {
                int start;
                int end;
                int inc;
                float fps;
                int cutIn;
                int cutOut;
                bool isUndiscovered;
            };

            ClassInstance* obj = ClassInstance::allocate(rtype);

            IPNode::ImageRangeInfo info = node->imageRangeInfo();
            InfoStruct* s = reinterpret_cast<InfoStruct*>(obj->structure());
            s->start = info.start;
            s->end = info.end;
            s->inc = info.inc;
            s->fps = info.fps;
            s->cutIn = info.cutIn;
            s->cutOut = info.cutOut;
            s->isUndiscovered = info.isUndiscovered;

            NODE_RETURN(obj);
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "node name not found");
        }

        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(newNode, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* type = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* name = NODE_ARG_OBJECT(1, StringType::String);

        if (!type)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node type");

        string nodeName = "";
        if (name)
            nodeName = name->c_str();

        s->graph().beginGraphEdit();
        if (IPNode* node = s->newNode(type->c_str(), nodeName))
        {
            s->graph().endGraphEdit();
            NODE_RETURN(c->stringType()->allocate(node->name()));
        }
        else
        {
            s->graph().endGraphEdit();
            ostringstream ostr;
            ostr << "can't build node of type '" << type->c_str() << "'";
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      ostr.str().c_str());
        }

        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(setNodeInputs, void)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* inputs = NODE_ARG_OBJECT(1, DynamicArray);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        IPGraph::IPNodes inputNodes;

        for (size_t i = 0; i < inputs->size(); i++)
        {
            StringType::String* n = inputs->element<StringType::String*>(i);

            if (!n)
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "nil in input array");

            if (IPNode* node = s->graph().findNode(n->c_str()))
            {
                inputNodes.push_back(node);
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "can't find a node");
            }
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            if (node->group())
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "top level nodes only");
            }

            try
            {
                if (node->compareToInputs(inputNodes)
                    != IPNode::IdenticalResult)
                {
                    s->graph().beginGraphEdit();
                    ostringstream msg;

                    if (node->testInputs(inputNodes, msg))
                    {
                        node->setInputs(inputNodes);
                    }
                    else
                    {
                        s->graph().endGraphEdit();
                        throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                                  msg.str().c_str());
                    }

                    s->graph().endGraphEdit();
                }
            }
            catch (...)
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD, "failed");
            }
        }
    }

    NODE_IMPLEMENTATION(testNodeInputs, Pointer)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* inputs = NODE_ARG_OBJECT(1, DynamicArray);
        const StringType* stype = static_cast<const StringType*>(name->type());

        IPGraph::IPNodes inputNodes;

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        for (size_t i = 0; i < inputs->size(); i++)
        {
            StringType::String* n = inputs->element<StringType::String*>(i);

            if (!n)
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "nil in inputs array");

            if (IPNode* node = s->graph().findNode(n->c_str()))
            {
                inputNodes.push_back(node);
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "can't find a node");
            }
        }

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            if (node->group())
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "top level nodes only");
            }

            try
            {
                ostringstream msg;

                if (node->testInputs(inputNodes, msg))
                {
                    NODE_RETURN(Pointer(0));
                }
                else
                {
                    NODE_RETURN(stype->allocate(msg.str()));
                }
            }
            catch (...)
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD, "failed");
            }
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(flushCachedNode, void)
    {
        Session* s = Session::currentSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        bool fileDataAlso = NODE_ARG(1, bool);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node name");

        if (IPNode* node = s->graph().findNode(name->c_str()))
        {
            GroupIPNode* group = dynamic_cast<GroupIPNode*>(node);
            if (!group)
                group = node->group();

            if (group)
            {
                group->flushIDsOfGroup();

                if (fileDataAlso)
                {
                    if (SourceGroupIPNode* sgipn =
                            dynamic_cast<SourceGroupIPNode*>(group))
                    {
                        if (FileSourceIPNode* fsipn =
                                dynamic_cast<FileSourceIPNode*>(
                                    sgipn->sourceNode()))
                        {
                            fsipn->invalidateFileSystemInfo();
                        }
                    }
                }
            }
        }
    }

    NODE_IMPLEMENTATION(existingFilesInSequence, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        StringType::String* seq = NODE_ARG_OBJECT(0, StringType::String);
        const StringType* stype = c->stringType();

        if (!seq)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil sequence string");

        ExistingFileList list =
            TwkUtil::existingFilesInSequence(seq->c_str(), false);

        int existingCount = 0;
        for (int i = 0; i < list.size(); ++i)
            if (list[i].exists)
                ++existingCount;

        if (existingCount)
        {
            int index = 0;
            array->resize(existingCount);

            for (int i = 0; i < list.size(); ++i)
            {
                if (list[i].exists)
                {
                    StringType::String* str = stype->allocate(list[i].name);
                    array->element<ClassInstance*>(index++) = str;
                }
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(existingFramesInSequence, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        StringType::String* seq = NODE_ARG_OBJECT(0, StringType::String);

        if (!seq)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil sequence string");

        ExistingFileList list =
            TwkUtil::existingFilesInSequence(seq->c_str(), false);

        int existingCount = 0;
        for (int i = 0; i < list.size(); ++i)
            if (list[i].exists)
                ++existingCount;

        if (existingCount)
        {
            int index = 0;
            array->resize(existingCount);

            for (int i = 0; i < list.size(); ++i)
            {
                if (list[i].exists)
                {
                    array->element<int>(index++) = list[i].frame;
                }
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(ioFormats, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        const Class* itype = static_cast<const Class*>(dtype->elementType());
        const DynamicArrayType* starrayType =
            static_cast<const DynamicArrayType*>(itype->fieldType(3));
        const Class* stupleType =
            static_cast<const Class*>(starrayType->elementType());

        typedef TwkMovie::GenericIO::Plugins Plugins;

        struct STuple
        {
            Pointer name;
            Pointer desc;
        };

        struct IOFormat
        {
            Pointer ext;
            Pointer desc;
            int caps;
            Pointer icodecs;
            Pointer acodecs;
        };

        const Plugins& plugins = TwkMovie::GenericIO::allPlugins();
        size_t count = 0;

        for (Plugins::const_iterator i = plugins.begin(); i != plugins.end();
             ++i)
        {
            const MovieIO* io = *i;
            const MovieIO::MovieTypeInfos& infos = io->extensionsSupported();

            if (infos.size() > 0)
            {
                array->resize(array->size() + infos.size());

                for (int i = 0; i < infos.size(); i++)
                {
                    const MovieIO::MovieTypeInfo& info = infos[i];

                    ClassInstance* obj = ClassInstance::allocate(itype);
                    array->element<ClassInstance*>(count++) = obj;
                    IOFormat* format =
                        reinterpret_cast<IOFormat*>(obj->structure());

                    format->ext = c->stringType()->allocate(info.extension);
                    format->desc = c->stringType()->allocate(info.description);
                    format->caps = info.capabilities;

                    DynamicArray* iarray = static_cast<DynamicArray*>(
                        ClassInstance::allocate(starrayType));
                    DynamicArray* aarray = static_cast<DynamicArray*>(
                        ClassInstance::allocate(starrayType));

                    iarray->resize(info.codecs.size());
                    aarray->resize(info.audioCodecs.size());

                    format->icodecs = iarray;
                    format->acodecs = aarray;

                    for (size_t q = 0; q < info.codecs.size(); q++)
                    {
                        ClassInstance* obj =
                            ClassInstance::allocate(stupleType);
                        iarray->element<ClassInstance*>(q) = obj;
                        STuple* tuple =
                            reinterpret_cast<STuple*>(obj->structure());

                        tuple->name =
                            c->stringType()->allocate(info.codecs[q].first);
                        tuple->desc =
                            c->stringType()->allocate(info.codecs[q].second);
                    }

                    for (size_t q = 0; q < info.audioCodecs.size(); q++)
                    {
                        ClassInstance* obj =
                            ClassInstance::allocate(stupleType);
                        aarray->element<ClassInstance*>(q) = obj;
                        STuple* tuple =
                            reinterpret_cast<STuple*>(obj->structure());

                        tuple->name = c->stringType()->allocate(
                            info.audioCodecs[q].first);
                        tuple->desc = c->stringType()->allocate(
                            info.audioCodecs[q].second);
                    }
                }
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(ioParameters, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const DynamicArrayType* dtype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(dtype, 1);
        const Class* itype = static_cast<const Class*>(dtype->elementType());
        const StringType::String* ext = NODE_ARG_OBJECT(0, StringType::String);
        bool encode = NODE_ARG(1, bool);
        const StringType::String* codec =
            NODE_ARG_OBJECT(2, StringType::String);

        typedef TwkMovie::GenericIO::Plugins Plugins;

        struct IOParameter
        {
            Pointer name;
            Pointer desc;
            Pointer uitype;
        };

        if (!ext)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil extension");
        if (!codec)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil codec");

        const Plugins& plugins = TwkMovie::GenericIO::allPlugins();

        for (Plugins::const_iterator i = plugins.begin(); i != plugins.end();
             ++i)
        {
            const MovieIO* io = *i;
            const MovieIO::MovieTypeInfos& infos = io->extensionsSupported();

            for (int i = 0; i < infos.size(); i++)
            {
                const MovieIO::MovieTypeInfo& info = infos[i];
                if (info.extension != ext->c_str())
                    continue;

                for (int q = 0; q < info.encodeParameters.size(); q++)
                {
                    const MovieIO::Parameter& p = info.encodeParameters[q];
                    if (p.codec != codec->c_str())
                        continue;

                    array->resize(array->size() + 1);
                    ClassInstance* obj = ClassInstance::allocate(itype);
                    array->element<ClassInstance*>(array->size() - 1) = obj;
                    IOParameter* param =
                        reinterpret_cast<IOParameter*>(obj->structure());

                    param->name = c->stringType()->allocate(p.name);
                    param->desc = c->stringType()->allocate(p.description);
                    param->uitype =
                        c->stringType()->allocate(""); // not used yet
                }
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(videoState, Pointer)
    {
        MuLangContext* c = TwkApp::muContext();
        Session* session = Session::currentSession();
        const Class* vtype = static_cast<const Class*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!session->multipleVideoDevices())
            NODE_RETURN(Pointer(0));

        ClassInstance* obj = ClassInstance::allocate(vtype);

        struct VState
        {
            Pointer module;
            Pointer device;
            Pointer videoFormat;
            float rate;
            int width;
            int height;
            Pointer dataFormat;
            Pointer sync;
            Pointer syncSource;
            Pointer audioFormat;
            float audioRate;
            int audioBits;
            int audioChannels;
            bool audioFloat;
            int displayMode;
        };

        VState* s = reinterpret_cast<VState*>(obj->structure());

        const VideoDevice* d = session->outputVideoDevice();

        VideoDevice::VideoFormat f =
            d->numVideoFormats()
                ? d->videoFormatAtIndex(d->currentVideoFormat())
                : VideoDevice::VideoFormat();
        VideoDevice::DataFormat df =
            d->numDataFormats() ? d->dataFormatAtIndex(d->currentDataFormat())
                                : VideoDevice::DataFormat();
        VideoDevice::SyncMode sm =
            d->numSyncModes() ? d->syncModeAtIndex(d->currentSyncMode())
                              : VideoDevice::SyncMode();
        VideoDevice::SyncSource ss =
            d->numSyncSources() ? d->syncSourceAtIndex(d->currentSyncSource())
                                : VideoDevice::SyncSource();
        VideoDevice::AudioFormat af =
            d->numAudioFormats()
                ? d->audioFormatAtIndex(d->currentAudioFormat())
                : VideoDevice::AudioFormat();

        s->module = stype->allocate(d->module()->name());
        s->device = stype->allocate(d->name());
        s->videoFormat = stype->allocate(f.description);
        s->rate = f.hz;
        s->width = f.width;
        s->height = f.height;
        s->dataFormat = stype->allocate(df.description);
        s->sync = stype->allocate(sm.description);
        s->syncSource = stype->allocate(ss.description);
        s->audioFormat = stype->allocate(af.description);
        s->audioRate = af.hz;
        s->audioChannels = af.numChannels;
        s->audioFloat = false;
        s->displayMode = d->displayMode();

        switch (af.format)
        {
        Float32Format:
            s->audioBits = 32;
            s->audioFloat = true;
            break;
        Int16Format:
            s->audioBits = 16;
            break;
        Int8Format:
            s->audioBits = 8;
            break;
        UInt32_SMPTE272M_20Format:
            s->audioBits = 20;
            break;
        UInt32_SMPTE299M_24Format:
            s->audioBits = 24;
            break;
        default:
            break;
        }

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(videoDeviceIDString, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        const StringType::String* moduleName =
            NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* deviceName =
            NODE_ARG_OBJECT(1, StringType::String);
        int index = NODE_ARG(2, int);

        const IPCore::Application::VideoModules& modules =
            IPCore::App()->videoModules();

        string idstr;

        for (size_t i = 0; i < modules.size(); i++)
        {
            if (modules[i]->name() == moduleName->c_str())
            {
                const TwkApp::VideoModule::VideoDevices& devices =
                    modules[i]->devices();

                for (size_t q = 0; q < devices.size(); q++)
                {
                    const TwkApp::VideoDevice* d = devices[q];

                    if (d->name() == deviceName->c_str())
                    {
                        idstr = d->humanReadableID(VideoDevice::IDType(index));
                    }
                }
            }
        }

        NODE_RETURN(stype->allocate(idstr));
    }

    NODE_IMPLEMENTATION(audioTextureID, int)
    {
        Session* s = Session::currentSession();
        ImageRenderer* renderer = s->renderer();

        IPCore::RenderQuery::TaggedTextureImagesMap taggedTextures;
        RenderQuery renderQuery(s->renderer());
        renderQuery.taggedWaveformImages(taggedTextures);
        IPCore::RenderQuery::TaggedTextureImagesMap::iterator it =
            taggedTextures.find(IPImage::waveformTagValue());
        if (it != taggedTextures.end())
        {
            NODE_RETURN(it->second.textureID);
        }
        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(audioTextureComplete, float)
    {
        Session::NodeVector nodes;
        Session* s = Session::currentSession();
        s->findNodesByTypeName(nodes, "RVSoundTrack");

        float complete = 0.0;
        if (nodes.size() > 0)
        {
            if (SoundTrackIPNode* sndTrk =
                    dynamic_cast<SoundTrackIPNode*>(nodes[0]))
            {
                complete = sndTrk->audioTextureComplete();
            }
        }

        NODE_RETURN(complete);
    }

    NODE_IMPLEMENTATION(licensingState, int)
    {
        NODE_RETURN(TWK_DEPLOY_GET_LICENSE_STATE());
    }

    namespace
    {
        DynamicArray*
        makeChannelArray(const DynamicArrayType* type, const StringType* stype,
                         const vector<TwkFB::FBInfo::ChannelInfo>& infos)
        {
            struct ChannelInfoStruct
            {
                Pointer name;
                int type;
            };

            const Class* channelType =
                static_cast<const Class*>(type->elementType());
            DynamicArray* array = new DynamicArray(type, 1);
            array->resize(infos.size());

            for (size_t i = 0; i < infos.size(); i++)
            {
                const TwkFB::FBInfo::ChannelInfo& info = infos[i];
                ClassInstance* obj = ClassInstance::allocate(channelType);
                ChannelInfoStruct* s =
                    reinterpret_cast<ChannelInfoStruct*>(obj->structure());
                s->name = stype->allocate(info.name.c_str());
                s->type = info.type;

                array->element<ClassInstance*>(i) = obj;
            }

            return array;
        }
    } // namespace

    NODE_IMPLEMENTATION(sourceMediaInfo, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* media =
            NODE_ARG_OBJECT(1, StringType::String);
        const MovieInfo* info = 0;

        StringType::String* mfile = 0;

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil source");
        const StringType* stype = static_cast<const StringType*>(name->type());

        // for campatibility with rv, scrub the incoming name
        vector<string> tokens;
        stl_ext::tokenize(tokens, name->c_str(), "./");

        if (!tokens.empty())
        {
            if (SourceIPNode* snode =
                    s->graph().findNodeOfType<SourceIPNode>(name->c_str()))
            {
                // First make sure that the media is active
                // Note that a source might not be active if it is one of many
                // multiple media representations and that it hasn't been
                // activated yet.
                if (!snode->isMediaActive() && snode->numMedia() == 0)
                {
                    throwBadArgumentException(
                        NODE_THIS, NODE_THREAD,
                        "Source is not active. A source can be activated by "
                        "the Multiple Media Representations drop down menu or "
                        "via the setActiveSourceMediaRep() command.");
                }

                if (media)
                {
                    size_t index = snode->mediaIndex(media->c_str());
                    if (index == size_t(-1))
                        throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                                  "bad source name");
                    mfile = stype->allocate(snode->mediaName(index));
                    info = &snode->mediaMovieInfo(index);
                }
                else
                {
                    info = &snode->mediaMovieInfo(0);
                    mfile = stype->allocate(snode->mediaName(0));
                }
            }
        }

        if (!info)
            NODE_RETURN(0);

        const Class* ttype = (const Class*)NODE_THIS.type();
        const DynamicArrayType* viArrayType =
            static_cast<const DynamicArrayType*>(ttype->fieldType(16));
        const Class* viewType =
            static_cast<const Class*>(viArrayType->elementType());
        const DynamicArrayType* larrayType =
            static_cast<const DynamicArrayType*>(viewType->fieldType(1));
        const Class* layerType =
            static_cast<const Class*>(larrayType->elementType());
        const DynamicArrayType* cniArrayType =
            static_cast<const DynamicArrayType*>(viewType->fieldType(2));
        const DynamicArrayType* cpiArrayType =
            static_cast<const DynamicArrayType*>(ttype->fieldType(21));
        const Class* chapterType =
            static_cast<const DynamicArrayType*>(cpiArrayType->elementType());

        struct LayerInfoStruct
        {
            Pointer name;
            Pointer channels;
        };

        struct ViewInfoStruct
        {
            Pointer name;
            Pointer layers;
            Pointer noLayerChannels;
        };

        struct ChapterInfoStruct
        {
            Pointer title;
            int startFrame;
            int endFrame;
        };

        struct STuple
        {
            int w, h, uw, uh, ux, uy, bits, ch;
            bool f;
            int planes;
            int start, end;
            float fps;
            int ach;
            float arate;
            Pointer channels;
            Pointer views;
            Pointer defaultView;
            Pointer file;
            bool hasAudio, hasVideo;
            Pointer chapters;
            float pa;
        };

        ClassInstance* obj = ClassInstance::allocate(ttype);
        STuple* st = reinterpret_cast<STuple*>(obj->structure());

        if (info)
        {
            DynamicArray* viArray = new DynamicArray(viArrayType, 1);
            DynamicArray* cpiArray = new DynamicArray(cpiArrayType, 1);

            st->w = info->width;
            st->h = info->height;
            st->uw = info->uncropWidth;
            st->uh = info->uncropHeight;
            st->ux = info->uncropX;
            st->uy = info->uncropY;
            st->ch = info->numChannels;
            st->f = false;
            st->planes = 1;
            st->start = info->start;
            st->end = info->end;
            st->fps = info->fps;
            st->ach = info->audio ? info->audioChannels.size() : 0;
            st->arate = info->audio ? info->audioSampleRate : 0.0;
            st->channels =
                makeChannelArray(cniArrayType, stype, info->channelInfos);
            st->views = viArray;
            st->chapters = cpiArray;
            st->file = mfile;
            st->defaultView = stype->allocate(info->defaultView);
            st->hasAudio = info->audio;
            st->hasVideo = info->video;
            st->pa = info->pixelAspect;

            switch (info->dataType)
            {
            case TwkFB::FrameBuffer::FLOAT:
                st->f = true;
                st->bits = 32;
                break;
            case TwkFB::FrameBuffer::HALF:
                st->f = true; // no break;
            case TwkFB::FrameBuffer::USHORT:
                st->bits = 16;
                break;
            case TwkFB::FrameBuffer::DOUBLE:
                st->f = true;
                st->bits = 64;
                break;
            default:
            case TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
            case TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
            case TwkFB::FrameBuffer::UCHAR:
                st->bits = 8;
                break;
            case TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2:
            case TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10:
                st->bits = 10;
                break;
            }

            viArray->resize(info->viewInfos.size());

            for (size_t i = 0; i < info->viewInfos.size(); i++)
            {
                const TwkFB::FBInfo::ViewInfo& vi = info->viewInfos[i];
                ClassInstance* viobj = ClassInstance::allocate(viewType);
                ViewInfoStruct* v =
                    reinterpret_cast<ViewInfoStruct*>(viobj->structure());
                v->name = stype->allocate(vi.name.c_str());
                viArray->element<ClassInstance*>(i) = viobj;

                v->noLayerChannels =
                    makeChannelArray(cniArrayType, stype, vi.otherChannels);

                DynamicArray* layerArray = new DynamicArray(larrayType, 1);
                v->layers = layerArray;
                layerArray->resize(vi.layers.size());

                for (size_t q = 0; q < vi.layers.size(); q++)
                {
                    const TwkFB::FBInfo::LayerInfo& li = vi.layers[q];
                    ClassInstance* liobj = ClassInstance::allocate(layerType);
                    LayerInfoStruct* c =
                        reinterpret_cast<LayerInfoStruct*>(liobj->structure());
                    c->name = stype->allocate(li.name.c_str());
                    c->channels =
                        makeChannelArray(cniArrayType, stype, li.channels);
                    layerArray->element<ClassInstance*>(q) = liobj;
                }
            }

            cpiArray->resize(info->chapters.size());

            for (size_t c = 0; c < info->chapters.size(); c++)
            {
                const TwkMovie::ChapterInfo& cpi = info->chapters[c];
                ClassInstance* cpobj = ClassInstance::allocate(chapterType);
                ChapterInfoStruct* cp =
                    reinterpret_cast<ChapterInfoStruct*>(cpobj->structure());
                cp->title = stype->allocate(cpi.title.c_str());
                cp->startFrame = cpi.startFrame;
                cp->endFrame = cpi.endFrame;
                cpiArray->element<ClassInstance*>(c) = cpobj;
            }
        }
        else
        {
        }

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(hopProfDynName, void)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "hopProfDynName: nil node name");

        HOP_PROF_DYN_NAME(name->c_str());
    }

    NODE_IMPLEMENTATION(logMetrics, void)
    {
        // Note: This command is no longer relevant in RV Open Source but has
        // been kept to maintain backward compatibility.
    }

    NODE_IMPLEMENTATION(logMetricsWithProperties, void)
    {
        // Note: This command is no longer relevant in RV Open Source but has
        // been kept to maintain backward compatibility.
    }

    NODE_IMPLEMENTATION(getVersion, Pointer)
    {
        DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);
        array->resize(3);

        array->element<int>(0) = TWK_DEPLOY_MAJOR_VERSION();
        array->element<int>(1) = TWK_DEPLOY_MINOR_VERSION();
        array->element<int>(2) = TWK_DEPLOY_PATCH_LEVEL();

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(getReleaseVariant, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        static const std::string rd = RELEASE_VARIANT;
        NODE_RETURN(stype->allocate(rd));
    }

    NODE_IMPLEMENTATION(getApplicationType, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        static const std::string at = APPLICATION_TYPE;
        NODE_RETURN(stype->allocate(at));
    }

    NODE_IMPLEMENTATION(isDebug, bool)
    {
#ifdef NDEBUG
        NODE_RETURN(false);
#else
        NODE_RETURN(true);
#endif
    }

    // Helper command to crash so that the breakpad minidump can be tested
    // Note: DO NOT DOCUMENT in commands.mud
    NODE_IMPLEMENTATION(crash, void)
    {
        volatile int* a = (int*)(NULL);
        *a = 1;
    }

    //----------------------------------------------------------------------
    //
    //  DEFINITIONS, MODULE INIT
    //

    void initCommands(Mu::MuLangContext* context)
    {
        USING_MU_FUNCTION_SYMBOLS;
        typedef ParameterVariable Param;

        if (!context)
            context = TwkApp::muContext();
        MuLangContext* c = context;
        Symbol* root = context->globalScope();
        Name cname = c->internName("commands");
        Mu::Module* commands = root->findSymbolOfType<Mu::Module>(cname);

        //
        // construct (string,string)[] type
        //
        Context::TypeVector types;
        types.push_back(context->stringType());
        types.push_back(context->stringType());
        TupleType* st = context->tupleType(types);
        const Type* stArray = context->arrayType(st, 1, 0);

        context->arrayType(context->halfType(), 1, 0);

        const Type* sarray = context->arrayType(context->stringType(), 1, 0);

        //
        //  Construct (int,int,int,int)
        //

        types.resize(4);
        types[0] = context->intType();
        types[1] = context->intType();
        types[2] = context->intType();
        types[3] = context->intType();
        const Type* i4tupleType = context->tupleType(types);

        // float[4,4]
        const Type* m44 = context->arrayType(context->floatType(), 2, 4, 4, 0);

        //
        //  WARNING: if you change the order here fix imagesAtPixel. its
        //  looking up the type of a specific field
        //
        Context::NameValuePairs fields(23);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("x"), context->floatType());
        fields[2] = make_pair(string("y"), context->floatType());
        fields[3] = make_pair(string("px"), context->floatType());
        fields[4] = make_pair(string("py"), context->floatType());
        fields[5] = make_pair(string("inside"), context->boolType());
        fields[6] = make_pair(string("edge"), context->floatType());
        fields[7] = make_pair(string("modelMatrix"), m44);
        fields[8] = make_pair(string("globalMatrix"), m44);
        fields[9] = make_pair(string("projectionMatrix"), m44);
        fields[10] = make_pair(string("textureMatrix"), m44);
        fields[11] = make_pair(string("orientationMatrix"), m44);
        fields[12] = make_pair(string("placementMatrix"), m44);
        fields[13] = make_pair(string("node"), context->stringType());
        fields[14] = make_pair(string("tags"), stArray);
        fields[15] = make_pair(string("index"), context->intType());
        fields[16] = make_pair(string("pixelAspect"), context->floatType());
        fields[17] = make_pair(string("device"), context->stringType());
        fields[18] = make_pair(string("physicalDevice"), context->stringType());
        fields[19] = make_pair(string("serialNumber"), context->intType());
        fields[20] = make_pair(string("imageNum"), context->intType());
        fields[21] = make_pair(string("textureID"), context->intType());
        fields[22] = make_pair(string("initPixelAspect"), context->floatType());
        context->arrayType(context->structType(0, "PixelImageInfo", fields), 1,
                           0);

        fields.resize(15);
        fields[0] = make_pair(string("module"), context->stringType());
        fields[1] = make_pair(string("device"), context->stringType());
        fields[2] = make_pair(string("videoFormat"), context->stringType());
        fields[3] = make_pair(string("rate"), context->floatType());
        fields[4] = make_pair(string("width"), context->intType());
        fields[5] = make_pair(string("height"), context->intType());
        fields[6] = make_pair(string("dataFormat"), context->stringType());
        fields[7] = make_pair(string("sync"), context->stringType());
        fields[8] = make_pair(string("syncSource"), context->stringType());
        fields[9] = make_pair(string("audioFormat"), context->stringType());
        fields[10] = make_pair(string("audioRate"), context->floatType());
        fields[11] = make_pair(string("audioBits"), context->intType());
        fields[12] = make_pair(string("audioChannels"), context->intType());
        fields[13] = make_pair(string("audioFloat"), context->boolType());
        fields[14] = make_pair(string("displayMode"), context->intType());
        context->arrayType(context->structType(0, "VideoDeviceState", fields),
                           1, 0);

        fields.resize(3);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("description"), context->stringType());
        fields[2] = make_pair(string("type"), context->stringType());
        context->arrayType(context->structType(0, "IOParameter", fields), 1, 0);

        fields.resize(4);
        fields[0] = make_pair(string("width"), context->intType());
        fields[1] = make_pair(string("height"), context->intType());
        fields[2] = make_pair(string("pixelAspect"), context->floatType());
        fields[3] = make_pair(string("orientation"), m44);
        context->structType(0, "NodeImageGeometry", fields);

        fields.resize(7);
        fields[0] = make_pair(string("start"), context->intType());
        fields[1] = make_pair(string("end"), context->intType());
        fields[2] = make_pair(string("inc"), context->intType());
        fields[3] = make_pair(string("fps"), context->floatType());
        fields[4] = make_pair(string("cutIn"), context->intType());
        fields[5] = make_pair(string("cutOut"), context->intType());
        fields[6] = make_pair(string("isUndiscovered"), context->boolType());
        context->structType(0, "NodeRangeInfo", fields);

        //
        //  WARNING: if you change the order here fix renderedImages. its
        //  looking up the type of a specific field
        //
        fields.resize(26);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("index"), context->intType());
        fields[2] = make_pair(string("imageMin"), context->vec2fType());
        fields[3] = make_pair(string("imageMax"), context->vec2fType());
        fields[4] = make_pair(string("stencilMin"), context->vec2fType());
        fields[5] = make_pair(string("stencilMax"), context->vec2fType());
        fields[6] = make_pair(string("modelMatrix"), m44);
        fields[7] = make_pair(string("globalMatrix"), m44);
        fields[8] = make_pair(string("projectionMatrix"), m44);
        fields[9] = make_pair(string("textureMatrix"), m44);
        fields[10] = make_pair(string("orientationMatrix"), m44);
        fields[11] = make_pair(string("placementMatrix"), m44);
        fields[12] = make_pair(string("width"), context->intType());
        fields[13] = make_pair(string("height"), context->intType());
        fields[14] = make_pair(string("pixelAspect"), context->floatType());
        fields[15] = make_pair(string("bitDepth"), context->intType());
        fields[16] = make_pair(string("floatingPoint"), context->boolType());
        fields[17] = make_pair(string("numChannels"), context->intType());
        fields[18] = make_pair(string("planar"), context->boolType());
        fields[19] = make_pair(string("node"), context->stringType());
        fields[20] = make_pair(string("tags"), stArray);
        fields[21] = make_pair(string("device"), context->stringType());
        fields[22] = make_pair(string("physicalDevice"), context->stringType());
        fields[23] = make_pair(string("serialNumber"), context->intType());
        fields[24] = make_pair(string("imageNum"), context->intType());
        fields[25] = make_pair(string("textureID"), context->intType());
        context->arrayType(context->structType(0, "RenderedImageInfo", fields),
                           1, 0);

        fields.resize(6);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("type"), context->intType());
        fields[2] = make_pair(string("dimensions"), i4tupleType);
        fields[3] = make_pair(string("size"), context->intType());
        fields[4] = make_pair(string("userDefined"), context->boolType());
        fields[5] = make_pair(string("info"), context->stringType());
        context->arrayType(context->structType(0, "PropertyInfo", fields), 1,
                           0);

        fields.resize(3);
        fields[0] = make_pair(string("node"), context->stringType());
        fields[1] = make_pair(string("nodeType"), context->stringType());
        fields[2] = make_pair(string("frame"), context->intType());
        context->arrayType(context->structType(0, "MetaEvalInfo", fields), 1,
                           0);

        fields.resize(5);
        fields[0] = make_pair(string("extension"), context->stringType());
        fields[1] = make_pair(string("description"), context->stringType());
        fields[2] = make_pair(string("capabilities"), context->intType());
        fields[3] = make_pair(string("imageCodecs"), stArray);
        fields[4] = make_pair(string("audioCodecs"), stArray);
        context->arrayType(context->structType(0, "IOFormat", fields), 1, 0);

        fields.resize(2);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("dataType"), context->intType());
        const Type* channelInfoType =
            context->structType(0, "ChannelInfo", fields);
        const Type* cniArrayType = context->arrayType(channelInfoType, 1, 0);

        fields.resize(2);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("channels"), cniArrayType);
        const Type* liArrayType = context->arrayType(
            context->structType(0, "LayerInfo", fields), 1, 0);

        fields.resize(3);
        fields[0] = make_pair(string("name"), context->stringType());
        fields[1] = make_pair(string("layers"), liArrayType);
        fields[2] = make_pair(string("noLayerChannels"), cniArrayType);
        const Type* viArrayType = context->arrayType(
            context->structType(0, "ViewInfo", fields), 1, 0);

        fields.resize(3);
        fields[0] = make_pair(string("title"), context->stringType());
        fields[1] = make_pair(string("startFrame"), context->intType());
        fields[2] = make_pair(string("endFrame"), context->intType());
        const Type* cpiArrayType = context->arrayType(
            context->structType(0, "ChapterInfo", fields), 1, 0);

        //
        //  WARNING: sourceMediaInfo() uses fieldType(16) to access the view
        //  info array, so if you change this struct you potentially need to
        //  modify that code
        //

        fields.resize(23);
        fields[0] = make_pair(string("width"), context->intType());
        fields[1] = make_pair(string("height"), context->intType());
        fields[2] = make_pair(string("uncropWidth"), context->intType());
        fields[3] = make_pair(string("uncropHeight"), context->intType());
        fields[4] = make_pair(string("uncropX"), context->intType());
        fields[5] = make_pair(string("uncropY"), context->intType());
        fields[6] = make_pair(string("bitsPerChannel"), context->intType());
        fields[7] = make_pair(string("channels"), context->intType());
        fields[8] = make_pair(string("isFloat"), context->boolType());
        fields[9] = make_pair(string("planes"), context->intType());
        fields[10] = make_pair(string("startFrame"), context->intType());
        fields[11] = make_pair(string("endFrame"), context->intType());
        fields[12] = make_pair(string("fps"), context->floatType());
        fields[13] = make_pair(string("audioChannels"), context->intType());
        fields[14] = make_pair(string("audioRate"), context->floatType());
        fields[15] = make_pair(string("channelInfos"), cniArrayType);
        fields[16] = make_pair(string("viewInfos"), viArrayType);
        fields[17] = make_pair(string("defaultView"), context->stringType());
        fields[18] = make_pair(string("file"), context->stringType());
        fields[19] = make_pair(string("hasAudio"), context->boolType());
        fields[20] = make_pair(string("hasVideo"), context->boolType());
        fields[21] = make_pair(string("chapterInfos"), cpiArrayType);
        fields[22] = make_pair(string("pixelAspect"), context->floatType());
        context->structType(0, "SourceMediaInfo", fields);

        types.clear();
        types.push_back(context->stringType()); // source name
        types.push_back(context->intType());    // start frame
        types.push_back(context->intType());    // end frame
        types.push_back(context->intType());    // inc
        types.push_back(context->floatType());  // fps
        types.push_back(context->boolType());   // audio
        types.push_back(context->boolType());   // video
        st = context->tupleType(types);
        context->arrayType(st, 1, 0);

        types.clear();                         // cacheInfo() return tuple
        types.push_back(context->int64Type()); // capacity in bytes
        types.push_back(context->int64Type()); // used in bytes
        types.push_back(context->int64Type()); // used in look ahead in bytes
        types.push_back(context->floatType()); // seconds of look ahead cache
        types.push_back(context->floatType()); // min seconds of look ahead
                                               // cache requested by user
        types.push_back(context->floatType()); // seconds of audio cached
        types.push_back(context->arrayType(context->intType(), 1,
                                           0)); // cached frame ranges
        context->tupleType(types);

        types.clear();                         // audioCacheInfo() return tuple
        types.push_back(context->floatType()); // seconds of audio cached
        types.push_back(context->arrayType(context->intType(), 1,
                                           0)); // cached audio samples ranges
        context->tupleType(types);

        types.clear(); // edl commands use (int,int,int)
        types.push_back(context->intType());
        types.push_back(context->intType());
        types.push_back(context->intType());
        context->tupleType(types);

        context->arrayType(context->intType(), 1, 0);

        types.erase(types.begin());
        context->tupleType(types); // (string[],string[])

        commands->addSymbols(
            new SymbolicConstant(c, "RGB709", "int", Value(0)),
            new SymbolicConstant(c, "CIEXYZ", "int", Value(1)),

            new SymbolicConstant(c, "PlayLoop", "int", Value(0)),
            new SymbolicConstant(c, "PlayOnce", "int", Value(1)),
            new SymbolicConstant(c, "PlayPingPong", "int", Value(2)),

            new SymbolicConstant(c, "OkImageStatus", "int",
                                 Value(Session::OkStatus)),
            new SymbolicConstant(c, "ErrorImageStatus", "int",
                                 Value(Session::ErrorStatus)),
            new SymbolicConstant(c, "WarningImageStatus", "int",
                                 Value(Session::WarningStatus)),
            new SymbolicConstant(c, "NoImageStatus", "int",
                                 Value(Session::NoImageStatus)),
            new SymbolicConstant(c, "PartialImageStatus", "int",
                                 Value(Session::PartialStatus)),
            new SymbolicConstant(c, "LoadingImageStatus", "int",
                                 Value(Session::LoadingStatus)),

            new SymbolicConstant(c, "CacheOff", "int",
                                 Value(Session::NeverCache)),
            new SymbolicConstant(c, "CacheBuffer", "int",
                                 Value(Session::BufferCache)),
            new SymbolicConstant(c, "CacheGreedy", "int",
                                 Value(Session::GreedyCache)),

            new SymbolicConstant(c, "ImageFileKind", "int",
                                 Value(IPCore::Application::ImageFileKind)),
            new SymbolicConstant(c, "MovieFileKind", "int",
                                 Value(IPCore::Application::MovieFileKind)),
            new SymbolicConstant(c, "CDLFileKind", "int",
                                 Value(IPCore::Application::CDLFileKind)),
            new SymbolicConstant(c, "LUTFileKind", "int",
                                 Value(IPCore::Application::LUTFileKind)),
            new SymbolicConstant(c, "DirectoryFileKind", "int",
                                 Value(IPCore::Application::DirectoryFileKind)),
            new SymbolicConstant(c, "RVFileKind", "int",
                                 Value(IPCore::Application::RVFileKind)),
            new SymbolicConstant(c, "EDLFileKind", "int",
                                 Value(IPCore::Application::EDLFileKind)),
            new SymbolicConstant(c, "UnknownFileKind", "int",
                                 Value(IPCore::Application::UnknownFileKind)),

            new SymbolicConstant(c, "IntType", "int",
                                 Value(Property::IntLayout)),
            new SymbolicConstant(c, "FloatType", "int",
                                 Value(Property::FloatLayout)),
            new SymbolicConstant(c, "HalfType", "int",
                                 Value(Property::HalfLayout)),
            new SymbolicConstant(c, "StringType", "int",
                                 Value(Property::StringLayout)),
            new SymbolicConstant(c, "ByteType", "int",
                                 Value(Property::ByteLayout)),
            new SymbolicConstant(c, "ShortType", "int",
                                 Value(Property::ShortLayout)),

            new SymbolicConstant(c, "IndependentDisplayMode", "int",
                                 Value(VideoDevice::IndependentDisplayMode)),
            new SymbolicConstant(c, "MirrorDisplayMode", "int",
                                 Value(VideoDevice::MirrorDisplayMode)),
            new SymbolicConstant(c, "NotADisplayMode", "int",
                                 Value(VideoDevice::NotADisplayMode)),

            new SymbolicConstant(c, "VideoAndDataFormatID", "int",
                                 Value(VideoDevice::VideoAndDataFormatID)),
            new SymbolicConstant(c, "DeviceNameID", "int",
                                 Value(VideoDevice::DeviceNameID)),
            new SymbolicConstant(c, "ModuleNameID", "int",
                                 Value(VideoDevice::ModuleNameID)),

            EndArguments);

        commands->addSymbols(
            new Function(c, "sessionName", sessionName, None, Return, "string",
                         End),

            new Function(c, "sessionNames", sessionNames, None, Return,
                         "string[]", End),

            new Function(c, "setSessionName", setSessionName, None, Return,
                         "void", Parameters, new Param(c, "name", "string"),
                         End),

            new Function(c, "setFrame", setFrame, None, Return, "void",
                         Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "markFrame", markFrame, None, Return, "void",
                         Parameters, new Param(c, "frame", "int"),
                         new Param(c, "mark", "bool"), End),

            new Function(c, "isMarked", isMarked, None, Return, "bool",
                         Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "markedFrames", markedFrames, None, Return, "int[]",
                         End),

            new Function(c, "scrubAudio", scrubAudio, None, Return, "void",
                         Parameters, new Param(c, "on", "bool"),
                         new Param(c, "chunkDuration", "float", Value(0.0f)),
                         new Param(c, "loopCount", "int", Value(int(0))), End),

            new Function(c, "play", play, None, Return, "void", End),

            new Function(c, "redraw", redraw, None, Return, "void", End),

            new Function(c, "clearAllButFrame", clearAllButFrame, None, Return,
                         "void", Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "reload", reload, None, Return, "void", End),

            new Function(c, "loadChangedFrames", loadChangedFrames, None,
                         Return, "void", Parameters,
                         new Param(c, "sourceNodes", "string[]"), End),

            new Function(c, "reload", reloadRange, None, Return, "void",
                         Parameters, new Param(c, "startFrame", "int"),
                         new Param(c, "endFrame", "int"), End),

            new Function(c, "isPlaying", isPlaying, None, Return, "bool", End),

            new Function(c, "stop", stop, None, Return, "void", End),

            new Function(c, "frame", frame, None, Return, "int", End),

            new Function(c, "metaEvaluate", metaEvaluate, None, Return,
                         "MetaEvalInfo[]", Parameters,
                         new Param(c, "frame", "int"),
                         new Param(c, "root", "string", Value(Pointer(0))),
                         new Param(c, "leaf", "string", Value(Pointer(0))),
                         new Param(c, "unique", "bool", Value(true)), End),

            new Function(c, "metaEvaluateClosestByType", metaEvaluate2, None,
                         Return, "MetaEvalInfo[]", Parameters,
                         new Param(c, "frame", "int"),
                         new Param(c, "typeName", "string"),
                         new Param(c, "root", "string", Value(Pointer(0))),
                         new Param(c, "unique", "bool", Value(true)), End),

            new Function(
                c, "mapPropertyToGlobalFrames", mapPropertyToGlobalFrames, None,
                Return, "int[]", Parameters, new Param(c, "propName", "string"),
                new Param(c, "maxDepth", "int"),
                new Param(c, "root", "string", Value(Pointer(0))), End),

            new Function(c, "closestNodesOfType", closestNodesOfType, None,
                         Return, "string[]", Parameters,
                         new Param(c, "typeName", "string"),
                         new Param(c, "root", "string", Value(Pointer(0))),
                         new Param(c, "depth", "int", Value(int(0))), End),

            new Function(c, "frameStart", frameStart, None, Return, "int", End),

            new Function(c, "frameEnd", frameEnd, None, Return, "int", End),

            new Function(c, "narrowedFrameStart", narrowedFrameStart, None,
                         Return, "int", End),

            new Function(c, "narrowedFrameEnd", narrowedFrameEnd, None, Return,
                         "int", End),

            new Function(c, "narrowToRange", narrowToRange, None, Return,
                         "void", Parameters, new Param(c, "frameStart", "int"),
                         new Param(c, "frameEnd", "int"), End),

            new Function(c, "setRealtime", setRealtime, None, Return, "void",
                         Parameters, new Param(c, "realtime", "bool"), End),

            new Function(c, "isRealtime", isRealtime, None, Return, "bool",
                         End),

            new Function(c, "skipped", skipped, None, Return, "int", End),

            new Function(c, "isCurrentFrameIncomplete",
                         isCurrentFrameIncomplete, None, Return, "bool", End),

            new Function(c, "isCurrentFrameError", isCurrentFrameError, None,
                         Return, "bool", End),

            new Function(c, "currentFrameStatus", currentFrameStatus, None,
                         Return, "int", End),

            new Function(c, "inPoint", inPoint, None, Return, "int", End),

            new Function(c, "outPoint", outPoint, None, Return, "int", End),

            new Function(c, "setInPoint", setInPoint, None, Return, "void",
                         Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "setOutPoint", setOutPoint, None, Return, "void",
                         Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "setMargins", setMargins, None, Return, "void",
                         Parameters, new Param(c, "margins", "vector float[4]"),
                         new Param(c, "allDevices", "bool", Value(false)), End),

            new Function(c, "margins", margins, None, Return, "vector float[4]",
                         End),

            new Function(c, "setPlayMode", setPlayMode, None, Return, "void",
                         Parameters, new Param(c, "mode", "int"), End),

            new Function(c, "playMode", playMode, None, Return, "int", End),

            new Function(c, "setFPS", setFPS, None, Return, "void", Parameters,
                         new Param(c, "fps", "float"), End),

            new Function(c, "setFrameStart", setFrameStart, None, Return,
                         "void", Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "setFrameEnd", setFrameEnd, None, Return, "void",
                         Parameters, new Param(c, "frame", "int"), End),

            new Function(c, "realFPS", realFPS, None, Return, "float", End),

            new Function(c, "fps", fps, None, Return, "float", End),

            new Function(c, "mbps", mbps, None, Return, "float", End),

            new Function(c, "resetMbps", resetMbps, None, Return, "void", End),

            new Function(c, "setInc", setInc, None, Return, "void", Parameters,
                         new Param(c, "inc", "int"), End),

            new Function(c, "setFiltering", setFiltering, None, Return, "void",
                         Parameters, new Param(c, "filterType", "int"), End),

            new Function(c, "getFiltering", getFiltering, None, Return, "int",
                         End),

            new Function(c, "setCacheMode", setCacheMode, None, Return, "void",
                         Parameters, new Param(c, "mode", "int"), End),

            new Function(c, "releaseAllCachedImages", releaseAllCachedImages,
                         None, Return, "void", End),

            new Function(c, "releaseAllUnusedImages", releaseAllUnusedImages,
                         None, Return, "void", End),

            new Function(c, "setAudioCacheMode", setAudioCacheMode, None,
                         Return, "void", Parameters,
                         new Param(c, "mode", "int"), End),

            new Function(c, "audioCacheMode", audioCacheMode, None, Return,
                         "int", End),

            new Function(c, "cacheMode", cacheMode, None, Return, "int", End),

            new Function(c, "cacheOutsideRegion", cacheOutsideRegion, None,
                         Return, "bool", End),

            new Function(c, "setCacheOutsideRegion", setCacheOutsideRegion,
                         None, Return, "void", Parameters,
                         new Param(c, "cacheOutside", "bool"), End),

            new Function(c, "isCaching", isCaching, None, Return, "bool", End),

            new Function(c, "cacheInfo", cacheInfo, None, Return,
                         "(int64,int64,int64,float,float,float,int[])", End),

            new Function(c, "audioCacheInfo", audioCacheInfo, None, Return,
                         "(float,int[])", End),

            new Function(c, "isBuffering", isBuffering, None, Return, "bool",
                         End),

            new Function(c, "inc", inc, None, Return, "int", End),

            new Function(c, "cacheSize", cacheSize, None, Return, "int", End),

            new Function(c, "fullScreenMode", fullScreenMode, None, Return,
                         "void", Parameters, new Param(c, "active", "bool"),
                         End),

            new Function(c, "isFullScreen", isFullScreen, None, Return, "bool",
                         End),

            new Function(c, "inputAtPixel", inputAtPixel, None, Return,
                         "string", Parameters,
                         new Param(c, "point", "vector float[2]"),
                         new Param(c, "strict", "bool", Value(true)), End),

            new Function(
                c, "eventToImageSpace", event2image, None, Return,
                "vector float[2]", Parameters,
                new Param(c, "sourceName", "string"),
                new Param(c, "point", "vector float[2]"),
                new Param(c, "normalizedImageOrigin", "bool", Value(false)),
                End),

            new Function(
                c, "imageToEventSpace", image2event, None, Return,
                "vector float[2]", Parameters,
                new Param(c, "sourceName", "string"),
                new Param(c, "point", "vector float[2]"),
                new Param(c, "normalizedImageOrigin", "bool", Value(false)),
                End),

            new Function(c, "eventToCameraSpace", event2camera, None, Return,
                         "vector float[2]", Parameters,
                         new Param(c, "sourceName", "string"),
                         new Param(c, "point", "vector float[2]"), End),

            new Function(c, "imagesAtPixel", imagesAtPixel, None, Return,
                         "PixelImageInfo[]", Parameters,
                         new Param(c, "point", "vector float[2]"),
                         new Param(c, "tag", "string", Value(Pointer(0))),
                         new Param(c, "sourcesOnly", "bool", Value(false)),
                         End),

            new Function(c, "sourceMediaInfo", sourceMediaInfo, None, Return,
                         "SourceMediaInfo", Parameters,
                         new Param(c, "sourceName", "string"),
                         new Param(c, "mediaName", "string", Value()), End),

            new Function(c, "sourcesAtFrame", sourcesAtFrame, None, Return,
                         "string[]", Parameters, new Param(c, "frame", "int"),
                         End),

            new Function(c, "renderedImages", renderedImages, None, Return,
                         "RenderedImageInfo[]", End),

            new Function(c, "imageGeometry", imageGeometry, None, Return,
                         "(vector float[2])[]", Parameters,
                         new Param(c, "name", "string"),
                         new Param(c, "useStencil", "bool", Value(true)), End),

            new Function(c, "imageGeometryByIndex", imageGeometryByIndex, None,
                         Return, "(vector float[2])[]", Parameters,
                         new Param(c, "index", "int"),
                         new Param(c, "useStencil", "bool", Value(true)), End),

            new Function(c, "imageGeometryByTag", imageGeometryByTag, None,
                         Return, "(vector float[2])[]", Parameters,
                         new Param(c, "name", "string"),
                         new Param(c, "value", "string"),
                         new Param(c, "useStencil", "bool", Value(true)), End),

            new Function(c, "nodesOfType", nodesOfType, None, Parameters,
                         new Param(c, "typeName", "string"), Return, "string[]",
                         End),

            new Function(c, "sessionFileName", sessionFileName, None, Return,
                         "string", End),

            new Function(c, "setSessionFileName", setSessionFileName, None,
                         Return, "void", Parameters,
                         new Param(c, "name", "string"), End),

            new Function(c, "undoPathSwapVars", undoPathSwapVars, None, Return,
                         "string", Parameters,
                         new Param(c, "pathWithVars", "string"), End),

            new Function(c, "redoPathSwapVars", redoPathSwapVars, None, Return,
                         "string", Parameters,
                         new Param(c, "pathWithoutVars", "string"), End),

            new Function(c, "readProfile", readProfile, None, Return, "void",
                         Parameters, new Param(c, "fileName", "string"),
                         new Param(c, "node", "string"),
                         new Param(c, "usePath", "bool", Value(true)),
                         new Param(c, "tag", "string", Value(Pointer(0))), End),

            new Function(c, "writeProfile", writeProfile, None, Return, "void",
                         Parameters, new Param(c, "fileName", "string"),
                         new Param(c, "node", "string"),
                         new Param(c, "comments", "string", Value(Pointer(0))),
                         End),

            new Function(c, "saveSession", saveSession, None, Return, "void",
                         Parameters, new Param(c, "fileName", "string"),
                         new Param(c, "asACopy", "bool", Value(false)),
                         new Param(c, "compressed", "bool", Value(false)),
                         new Param(c, "sparse", "bool", Value(false)), End),

            // NODE NEFINITION API

            new Function(c, "updateNodeDefinition_", updateNodeDefinition, None,
                         Return, "void", Parameters,
                         new Param(c, "defitionName", "string"), End),

            new Function(
                c, "writeNodeDefinition", writeNodeDefinition, None, Return,
                "void", Parameters, new Param(c, "nodeName", "string"),
                new Param(c, "fileName", "string"),
                new Param(c, "inlineSourceCode", "bool", Value(true)), End),

            new Function(
                c, "writeAllNodeDefinitions", writeAllNodeDefinitions, None,
                Return, "void", Parameters, new Param(c, "fileName", "string"),
                new Param(c, "inlineSourceCode", "bool", Value(true)), End),

            new Function(
                c, "nodeTypes", nodeTypes, None, Return, "string[]", Parameters,
                new Param(c, "userVisibleOnly", "bool", Value(false)), End),

            // --

            new Function(c, "newSession", newSession, None, Return, "void",
                         Parameters, new Param(c, "files", "string[]"), End),

            new Function(c, "clearSession", clearSession, None, Return, "void",
                         End),

            //----------------------------------------

            new Function(
                c, "getFloatProperty", getFloatProperty, None, Return,
                "float[]", Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "start", "int", Value(int(0))),
                new Param(c, "num", "int", Value(numeric_limits<int>::max())),
                End),

            new Function(
                c, "setFloatProperty", setFloatProperty, None, Return, "void",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "value", "float[]"),
                new Param(c, "allowResize", "bool", Value(false)), End),

            new Function(c, "insertFloatProperty", insertFloatProperty, None,
                         Return, "void", Parameters,
                         new Param(c, "propertyName", "string"),
                         new Param(c, "value", "float[]"),
                         new Param(c, "beforeIndex", "int",
                                   Value(numeric_limits<int>::max())),
                         End),

            new Function(
                c, "getHalfProperty", getHalfProperty, None, Return, "half[]",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "start", "int", Value(int(0))),
                new Param(c, "num", "int", Value(numeric_limits<int>::max())),
                End),

            new Function(
                c, "setHalfProperty", setHalfProperty, None, Return, "void",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "value", "half[]"),
                new Param(c, "allowResize", "bool", Value(false)), End),

            new Function(c, "insertHalfProperty", insertHalfProperty, None,
                         Return, "void", Parameters,
                         new Param(c, "propertyName", "string"),
                         new Param(c, "value", "half[]"),
                         new Param(c, "beforeIndex", "int",
                                   Value(numeric_limits<int>::max())),
                         End),

            new Function(
                c, "getIntProperty", getIntProperty, None, Return, "int[]",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "start", "int", Value(int(0))),
                new Param(c, "num", "int", Value(numeric_limits<int>::max())),
                End),

            new Function(
                c, "setIntProperty", setIntProperty, None, Return, "void",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "value", "int[]"),
                new Param(c, "allowResize", "bool", Value(false)), End),

            new Function(c, "insertIntProperty", insertIntProperty, None,
                         Return, "void", Parameters,
                         new Param(c, "propertyName", "string"),
                         new Param(c, "value", "int[]"),
                         new Param(c, "beforeIndex", "int",
                                   Value(numeric_limits<int>::max())),
                         End),

            new Function(
                c, "getByteProperty", getByteProperty, None, Return, "byte[]",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "start", "int", Value(int(0))),
                new Param(c, "num", "int", Value(numeric_limits<int>::max())),
                End),

            new Function(
                c, "setByteProperty", setByteProperty, None, Return, "void",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "value", "byte[]"),
                new Param(c, "allowResize", "bool", Value(false)), End),

            new Function(c, "insertByteProperty", insertByteProperty, None,
                         Return, "void", Parameters,
                         new Param(c, "propertyName", "string"),
                         new Param(c, "value", "byte[]"),
                         new Param(c, "beforeIndex", "int",
                                   Value(numeric_limits<int>::max())),
                         End),

            new Function(
                c, "getStringProperty", getStringProperty, None, Return,
                "string[]", Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "start", "int", Value(int(0))),
                new Param(c, "num", "int", Value(numeric_limits<int>::max())),
                End),

            new Function(
                c, "setStringProperty", setStringProperty, None, Return, "void",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "value", "string[]"),
                new Param(c, "allowResize", "bool", Value(false)), End),

            new Function(c, "insertStringProperty", insertStringProperty, None,
                         Return, "void", Parameters,
                         new Param(c, "propertyName", "string"),
                         new Param(c, "value", "string[]"),
                         new Param(c, "beforeIndex", "int",
                                   Value(numeric_limits<int>::max())),
                         End),

            new Function(
                c, "newNDProperty", newProperty2, None, Return, "void",
                Parameters, new Param(c, "propertyName", "string"),
                new Param(c, "propertyType", "int"),
                new Param(c, "propertyDimensions", "(int,int,int,int)"), End),

            new Function(c, "newProperty", newProperty, None, Return, "void",
                         Parameters, new Param(c, "propertyName", "string"),
                         new Param(c, "propertyType", "int"),
                         new Param(c, "propertyWidth", "int"), End),

            new Function(c, "deleteProperty", deleteProperty, None, Return,
                         "void", Parameters,
                         new Param(c, "propertyName", "string"), End),

            new Function(c, "nodes", nodes, None, Return, "string[]", End),

            new Function(c, "nodeType", nodeType, None, Return, "string",
                         Parameters, new Param(c, "nodeName", "string"), End),

            new Function(c, "deleteNode", deleteNode, None, Return, "void",
                         Parameters, new Param(c, "nodeName", "string"), End),

            new Function(c, "properties", properties, None, Return, "string[]",
                         Parameters, new Param(c, "nodeName", "string"), End),

            new Function(c, "propertyInfo", propertyInfo, None, Return,
                         "PropertyInfo", Parameters,
                         new Param(c, "propertyName", "string"), End),

            new Function(c, "nextUIName", nextUIName, None, Return, "string",
                         Parameters, new Param(c, "uiName", "string"), End),

            new Function(c, "propertyExists", propertyExists, None, Return,
                         "bool", Parameters,
                         new Param(c, "propertyName", "string"), End),

            new Function(c, "viewSize", viewSize, None, Return,
                         "vector float[2]", End),

            new Function(c, "bgMethod", bgMethod, None, Return, "string", End),

            new Function(c, "setBGMethod", setBGMethod, None, Return, "void",
                         Parameters, new Param(c, "methodName", "string"), End),

            new Function(c, "setRendererType", setRendererType, None, Return,
                         "void", Parameters, new Param(c, "name", "string"),
                         End),

            new Function(c, "getRendererType", getRendererType, None, Return,
                         "string", End),

            new Function(c, "setHardwareStereoMode", setHardwareStereoMode,
                         None, Return, "void", Parameters,
                         new Param(c, "active", "bool"), End),

            new Function(c, "fileKind", fileKind, None, Return, "int",
                         Parameters, new Param(c, "filename", "string"), End),

            new Function(c, "audioTextureID", audioTextureID, None, Return,
                         "int", End),

            new Function(c, "audioTextureComplete", audioTextureComplete, None,
                         Return, "float", End),

            new Function(
                c, "sendInternalEvent", sendInternalEvent, None, Return,
                "string", Parameters, new Param(c, "eventName", "string"),
                new Param(c, "contents", "string", Value(Pointer(0))),
                new Param(c, "senderName", "string", Value(Pointer(0))), End),

            new Function(c, "setFilterLiveReviewEvents",
                         setFilterLiveReviewEvents, None, Return, "void",
                         Parameters, new Param(c, "shouldFilterEvents", "bool"),
                         End),

            new Function(c, "filterLiveReviewEvents", filterLiveReviewEvents,
                         None, Return, "bool", End),

            new Function(c, "previousViewNode", previousViewNode, None, Return,
                         "string", End),

            new Function(c, "nextViewNode", nextViewNode, None, Return,
                         "string", End),

            new Function(c, "setViewNode", setViewNode, None, Return, "void",
                         Parameters, new Param(c, "nodeName", "string"), End),

            new Function(c, "viewNodes", viewNodes, None, Return, "string[]",
                         End),

            new Function(c, "viewNode", viewNode, None, Return, "string", End),

            new Function(c, "nodeImageGeometry", nodeImageGeometry, None,
                         Return, "NodeImageGeometry", Parameters,
                         new Param(c, "nodeName", "string"),
                         new Param(c, "frame", "int"), End),

            new Function(c, "nodeRangeInfo", nodeRangeInfo, None, Return,
                         "NodeRangeInfo", Parameters,
                         new Param(c, "nodeName", "string"), End),

            new Function(c, "newNode", newNode, None, Return, "string",
                         Parameters, new Param(c, "nodeType", "string"),
                         new Param(c, "nodeName", "string", Value(Pointer(0))),
                         End),

            new Function(c, "nodeConnections", nodeConnections, None, Return,
                         "(string[],string[])", Parameters,
                         new Param(c, "nodeName", "string"),
                         new Param(c, "traverseGroups", "bool", Value(false)),
                         End),

            new Function(c, "nodesInGroup", nodesInGroup, None, Return,
                         "string[]", Parameters,
                         new Param(c, "nodeName", "string"), End),

            new Function(c, "nodeGroup", nodeGroup, None, Return, "string",
                         Parameters, new Param(c, "nodeName", "string"), End),

            new Function(c, "nodeGroupRoot", nodeGroupRoot, None, Return,
                         "string", Parameters,
                         new Param(c, "nodeName", "string"), End),

            new Function(c, "nodeExists", nodeExists, None, Return, "bool",
                         Parameters, new Param(c, "nodeName", "string"), End),

            new Function(c, "setNodeInputs", setNodeInputs, None, Return,
                         "void", Parameters, new Param(c, "nodeName", "string"),
                         new Param(c, "inputNodes", "string[]"), End),

            new Function(c, "testNodeInputs", testNodeInputs, None, Return,
                         "string", Parameters,
                         new Param(c, "nodeName", "string"),
                         new Param(c, "inputNodes", "string[]"), End),

            new Function(c, "flushCachedNode", flushCachedNode, None, Return,
                         "void", Parameters, new Param(c, "nodeName", "string"),
                         new Param(c, "fileDataAlso", "bool", Value(false)),
                         End),

            new Function(c, "existingFilesInSequence", existingFilesInSequence,
                         None, Return, "string[]", Parameters,
                         new Param(c, "sequence", "string"), End),

            new Function(c, "existingFramesInSequence",
                         existingFramesInSequence, None, Return, "int[]",
                         Parameters, new Param(c, "sequence", "string"), End),

            new Function(c, "ioFormats", ioFormats, None, Return, "IOFormat[]",
                         End),

            new Function(
                c, "ioParameters", ioParameters, None, Return, "IOParameter[]",
                Parameters, new Param(c, "extension", "string"),
                new Param(c, "forEncode", "bool"),
                new Param(c, "codec", "string", Value(Pointer(0))), End),

            new Function(c, "videoState", videoState, None, Return,
                         "VideoDeviceState", End),

            new Function(c, "videoDeviceIDString", videoDeviceIDString, None,
                         Return, "string", Parameters,
                         new Param(c, "moduleName", "string"),
                         new Param(c, "deviceName", "string"),
                         new Param(c, "idtype", "int"), End),

            new Function(c, "licensingState", licensingState, None, Return,
                         "int", End),

            new Function(c, "hopProfDynName", hopProfDynName, None, Return,
                         "void", Parameters, new Param(c, "name", "string"),
                         End),

            new Function(c, "logMetrics", logMetrics, None, Return, "void",
                         Parameters, new Param(c, "event", "string"), End),

            new Function(c, "logMetricsWithProperties",
                         logMetricsWithProperties, None, Return, "void",
                         Parameters, new Param(c, "event", "string"),
                         new Param(c, "properties", "string"), End),

            new Function(c, "getVersion", getVersion, None, Return, "int[]",
                         End),

            new Function(c, "getReleaseVariant", getReleaseVariant, None,
                         Return, "string", End),

            new Function(c, "getApplicationType", getApplicationType, None,
                         Return, "string", End),

            new Function(c, "isDebug", isDebug, None, Return, "bool", End),

            new Function(c, "crash", crash, None, Return, "void", Parameters,
                         End),

            EndArguments);
    }

} // namespace IPMu
