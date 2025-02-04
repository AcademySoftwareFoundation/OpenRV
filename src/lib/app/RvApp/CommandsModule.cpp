//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvApp/CommandsModule.h>
#include <RvApp/Options.h>
#include <RvApp/RvSession.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPBaseNodes/SequenceIPNode.h>
#include <IPBaseNodes/SourceGroupIPNode.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/IPProperty.h>
#include <IPCore/PropertyEditor.h>
#include <IPCore/NodeManager.h>
#include <IPCore/Profile.h>
#include <IPCore/IPInstanceNode.h>
#include <OCIONodes/OCIOIPNode.h>
#include <IPMu/CommandsModule.h>
#include <TwkDeploy/Deploy.h>
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
#include <MuGL/GLModule.h>
#include <MuGLText/GLTextModule.h>
#include <MuGLU/GLUModule.h>
#include <MuAutoDoc/AutoDocModule.h>
#include <MuIO/IOModule.h>
#include <MuImage/ImageModule.h>
#include <MuEncoding/EncodingModule.h>
#include <MuMathLinear/MathLinearModule.h>
#include <MuSystem/SystemModule.h>
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
#include <TwkDeploy/Deploy.h>
#include <TwkFB/IO.h>
#include <TwkFB/Operations.h>
#include <TwkMovie/Movie.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/Timer.h>
#include <algorithm>
#include <fstream>
#include <half.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stl_ext/string_algo.h>

TwkUtil::Timer theTimer;

namespace Rv
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

    //----------------------------------------------------------------------

    Mu::Function* sessionFunction(const char* name)
    {
        USING_MU_FUNCTION_SYMBOLS;
        MuLangContext* context = TwkApp::muContext();
        Name n = context->internName(name);
        Symbol::SymbolVector symbols = context->findSymbolsOfType<Function>(n);

        for (int i = 0; i < symbols.size(); i++)
        {
            Function* F = static_cast<Function*>(symbols[i]);

            if (F->numArgs() == 0)
            {
                return F;
            }
        }

        return 0;
    }

    //----------------------------------------------------------------------

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

    //
    //  DEPRECATED (RV-ONLY) CORE COMMANDS
    //

    NODE_IMPLEMENTATION(sourceMedia, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        Session* s = Session::currentSession();
        const TupleType* ttype =
            static_cast<const TupleType*>(NODE_THIS.type());
        const StringType* stype =
            static_cast<const StringType*>(ttype->tupleFieldTypes()[0]);
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(ttype->tupleFieldTypes()[1]);
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        if (!name)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil source");
        }

        string n = name->c_str();
        string frame = "";
        string mindex = "";
        size_t p0 = n.find('.');
        size_t p1 = n.find('/');

        struct STuple
        {
            StringType::String* name;
            DynamicArray* layers;
            DynamicArray* views;
        };

        ClassInstance* obj = ClassInstance::allocate(ttype);
        STuple* tuple = reinterpret_cast<STuple*>(obj->structure());

        tuple->views = new DynamicArray(atype, 1);
        tuple->layers = new DynamicArray(atype, 1);

        if (p0 != string::npos)
        {
            n = n.substr(0, p0);

            if (p1 != string::npos)
            {
                mindex = name->utf8().substr(p0 + 1, p1 - p0 - 1).c_str();
                frame = name->utf8().substr(p1 + 1, n.size() - p1 - 1).c_str();
            }
        }

        SourceIPNode* node = dynamic_cast<SourceIPNode*>(s->node(n));

        if (!node)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "bad source name");
        }

        size_t index = atoi(mindex.c_str());
        const MovieInfo& info = node->mediaMovieInfo(index);
        const string& filename = node->mediaName(index);

        tuple->name = stype->allocate(filename);
        const vector<string>& views = info.views;
        const vector<string>& layers = info.layers;

        tuple->views->resize(views.size());
        tuple->layers->resize(layers.size());

        for (unsigned int i = 0; i < views.size(); i++)
        {
            tuple->views->element<ClassInstance*>(i) =
                stype->allocate(views[i]);
        }

        for (unsigned int i = 0; i < layers.size(); i++)
        {
            tuple->layers->element<ClassInstance*>(i) =
                stype->allocate(layers[i]);
        }

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(startTimer, void)
    {
        Session* s = Session::currentSession();
        if (s->userTimer().isRunning())
            s->userTimer().stop();
        s->userTimer().start();
    }

    NODE_IMPLEMENTATION(elapsedTime, float)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(float(s->userTimer().elapsed()));
    }

    NODE_IMPLEMENTATION(theTime, float)
    {
        NODE_RETURN(float(theTimer.elapsed()));
    }

    NODE_IMPLEMENTATION(stopTimer, float)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(float(s->userTimer().stop()));
    }

    NODE_IMPLEMENTATION(isTimerRunning, bool)
    {
        Session* s = Session::currentSession();
        NODE_RETURN(s->userTimer().isRunning());
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

    //----------------------------------------------------------------------
    //
    //  RVSESSION COMMANDS
    //

    NODE_IMPLEMENTATION(readCDL, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* node = NODE_ARG_OBJECT(1, StringType::String);
        const bool activate = NODE_ARG(2, bool);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil name");
        if (!node)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node");

        IPGraph::GraphEdit edit(RvSession::currentRvSession()->graph());
        RvSession::currentRvSession()->readCDL(name->c_str(), node->c_str(),
                                               activate);
    }

    NODE_IMPLEMENTATION(readLUT, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* node = NODE_ARG_OBJECT(1, StringType::String);
        const bool activate = NODE_ARG(2, bool);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil name");
        if (!node)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node");

        IPGraph::GraphEdit edit(RvSession::currentRvSession()->graph());
        RvSession::currentRvSession()->readLUT(name->c_str(), node->c_str(),
                                               activate);
    }

    NODE_IMPLEMENTATION(updateLUT, void)
    {
        // No-Op
    }

    NODE_IMPLEMENTATION(exportCurrentSourceFrame, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const TwkFB::FrameBuffer* fb = s->currentFB();

        const TwkFB::FrameBuffer* outfb = fb;

        if (fb->dataType() >= TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            outfb = convertToLinearRGB709(fb);
        }

        outfb = copyConvert(outfb, TwkFB::FrameBuffer::HALF);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil filename");

        string filename = name->c_str();
        if (extension(filename) == "")
            filename += ".tif";

        TwkFB::FrameBufferIO::WriteRequest request;
        TwkFB::FrameBufferIO::ConstFrameBufferVector fbs(1);
        fbs.front() = outfb;

        try
        {
            TwkFB::GenericIO::writeImages(fbs, filename, request);
        }
        catch (...)
        {
            if (fb != outfb)
                delete outfb;
            throw;
        }
    }

    NODE_IMPLEMENTATION(exportCurrentFrame, void)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        Session* s = Session::currentSession();
        ImageRenderer* renderer = s->renderer();
        const TwkGLF::GLVideoDevice* d = renderer->controlDevice().glDevice;

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil filename");

        string filename = name->c_str();
        if (extension(filename) == "")
            filename += ".tif";

        TwkApp::VideoDevice::Margins m = s->eventVideoDevice()->margins();

        TwkFB::FrameBuffer* fb = new TwkFB::FrameBuffer(
            d->width() - m.left - m.right, d->height() - m.bottom - m.top, 4,
            TwkFB::FrameBuffer::UCHAR);

        const ImageRenderer::GLFBO* fbo =
            renderer->newOutputOnlyImageFBO(GL_RGBA8)->fbo();
        renderer->render(s->currentFrame(), s->displayImage());

        glFinish();

        glReadPixels(m.left, m.bottom, d->width() - m.left - m.right,
                     d->height() - m.bottom - m.top, GL_RGBA, GL_UNSIGNED_BYTE,
                     fb->pixels<unsigned char>());

        glFinish();

        fbo->unbind();
        renderer->releaseImageFBO(fbo);

        TwkFB::FrameBufferIO::WriteRequest request;
        TwkFB::FrameBufferIO::ConstFrameBufferVector fbs(1);
        fbs.front() = fb;
        TwkFB::GenericIO::writeImages(fbs, filename, request);
    }

    NODE_IMPLEMENTATION(newImageSource, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        int width = NODE_ARG(1, int);
        int height = NODE_ARG(2, int);
        int uncropWidth = NODE_ARG(3, int);
        int uncropHeight = NODE_ARG(4, int);
        int uncropX = NODE_ARG(5, int);
        int uncropY = NODE_ARG(6, int);
        float pixelAspect = NODE_ARG(7, float);
        int channels = NODE_ARG(8, int);
        int bits = NODE_ARG(9, int);
        bool fp = NODE_ARG(10, bool);
        int fs = NODE_ARG(11, int);
        int fe = NODE_ARG(12, int);
        float fps = NODE_ARG(13, float);
        DynamicArray* layers = NODE_ARG_OBJECT(14, DynamicArray);
        DynamicArray* views = NODE_ARG_OBJECT(15, DynamicArray);

        if (!name)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source name");

        IPNode::MovieInfo info;
        info.width = width;
        info.height = height;
        info.uncropWidth = uncropWidth;
        info.uncropHeight = uncropHeight;
        info.uncropX = uncropX;
        info.uncropY = uncropY;
        info.pixelAspect = pixelAspect;
        info.numChannels = channels;
        info.fps = fps;
        info.start = fs;
        info.end = fe;
        info.inc = 1;

        switch (bits)
        {
        case 8:
            info.dataType = TwkFB::FrameBuffer::UCHAR;
            break;
        case 16:
            info.dataType =
                fp ? TwkFB::FrameBuffer::HALF : TwkFB::FrameBuffer::USHORT;
            break;
        case 32:
            info.dataType = TwkFB::FrameBuffer::FLOAT;
            break;
        }

        if (layers)
        {
            for (size_t i = 0; i < layers->size(); i++)
            {
                StringType::String* s = layers->element<StringType::String*>(i);
                info.layers.push_back(s->c_str());
            }
        }

        if (views)
        {
            for (size_t i = 0; i < views->size(); i++)
            {
                StringType::String* s = views->element<StringType::String*>(i);
                info.views.push_back(s->c_str());
            }
        }

        IPGraph::GraphEdit edit(s->graph());
        SourceIPNode* node = s->addImageSource(name->c_str(), info);
        StringType::String* nodeName = stype->allocate(node->name());
        NODE_RETURN(nodeName);
    }

    NODE_IMPLEMENTATION(newImageSourcePixels, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();
        StringType::String* source = NODE_ARG_OBJECT(0, StringType::String);
        int frame = NODE_ARG(1, int);
        StringType::String* layer = NODE_ARG_OBJECT(2, StringType::String);
        StringType::String* view = NODE_ARG_OBJECT(3, StringType::String);

        if (!source)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source name");

        if (IPNode* node = s->graph().findNode(source->c_str()))
        {
            if (ImageSourceIPNode* imageNode =
                    dynamic_cast<ImageSourceIPNode*>(node))
            {
                IPGraph::GraphEdit edit(s->graph());

                Property* p = imageNode->findCreatePixels(
                    frame, view ? view->c_str() : "-",
                    layer ? layer->c_str() : "-");
                ostringstream str;
                str << name(node) << ".image." << p->name();
                NODE_RETURN(stype->allocate(str));
            }
        }

        NODE_RETURN(Pointer(0));
    }

    NODE_IMPLEMENTATION(insertCreatePixelBlock, void)
    {
        RvSession* s = RvSession::currentRvSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Event* event = NODE_ARG_OBJECT(0, Event);

        if (const PixelBlockTransferEvent* pe =
                dynamic_cast<const PixelBlockTransferEvent*>(event->event))
        {
            const RvGraph::Sources& sources = s->rvgraph().imageSources();

            for (size_t i = 0; i < sources.size(); i++)
            {
                if (ImageSourceIPNode* node =
                        dynamic_cast<ImageSourceIPNode*>(sources[i]))
                {
                    size_t index = node->mediaIndex(pe->media());

                    if (index != size_t(-1))
                    {
                        try
                        {

#if 0
                        cout << "pixel block x,y = "
                             << pe->x()
                             << ", " << pe->y()
                             << "  w,h = " << pe->width() 
                             << ", " << pe->height() 
                             << "  view = " << pe->view()
                             << endl;
#endif

                            IPGraph::GraphEdit edit(s->graph());

                            node->insertPixels(pe->view(), pe->layer(),
                                               pe->frame(), pe->x(), pe->y(),
                                               pe->width(), pe->height(),
                                               pe->pixels(), pe->size());
                        }
                        catch (PixelBlockSizeMismatchExc& exc)
                        {
                            cerr << "ERROR: " << exc.what() << endl;
                            throwBadArgumentException(
                                NODE_THIS, NODE_THREAD,
                                "bad pixel block recieved");
                        }
                    }
                }
            }
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }
    }

    NODE_IMPLEMENTATION(optionsPlay, int)
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        NODE_RETURN(opts.play);
    }

    NODE_IMPLEMENTATION(optionsPlayReset, void)
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        opts.play = 0;
    }

    NODE_IMPLEMENTATION(optionsNoPackages, int)
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        NODE_RETURN(opts.noPackages);
    }

    NODE_IMPLEMENTATION(optionsProgressiveLoading, int)
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        NODE_RETURN(opts.progressiveSourceLoading);
    }

    NODE_IMPLEMENTATION(loadTotal, int)
    {
        RvSession* s = RvSession::currentRvSession();
        NODE_RETURN(s->loadTotal());
    }

    NODE_IMPLEMENTATION(loadCount, int)
    {
        RvSession* s = RvSession::currentRvSession();
        NODE_RETURN(s->loadCount());
    }

    NODE_IMPLEMENTATION(data, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        if (!s)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "No active session available");
        NODE_RETURN(s->data());
    }

    NODE_IMPLEMENTATION(sources, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
        const TupleType* ttype =
            static_cast<const TupleType*>(type->elementType());

        struct STuple
        {
            StringType::String* name;
            int start;
            int end;
            int inc;
            float fps;
            bool audio;
            bool video;
        };

        const RvSession::Sources& sources = s->rvgraph().imageSources();

        DynamicArray* array = new DynamicArray(type, 1);
        array->resize(sources.size());

        for (int i = 0; i < sources.size(); i++)
        {
            SourceIPNode* source = sources[i];
            ClassInstance* tuple = ClassInstance::allocate(ttype);

            STuple* st = reinterpret_cast<STuple*>(tuple->structure());

            if (source)
            {
                try
                {
                    const MovieInfo& info = source->mediaMovieInfo(0);
                    const string& mediaName = source->mediaName(0);

                    st->name = c->stringType()->allocate(mediaName);
                    st->start = info.start;
                    st->end = info.end;
                    st->inc = info.inc;
                    st->fps = info.fps;
                    st->audio = info.audio;
                    st->video = info.video;

                    array->element<ClassInstance*>(i) = tuple;
                }
                catch (...)
                {
                    array->element<ClassInstance*>(i) = NULL;
                }
            }
            else
            {
                array->element<ClassInstance*>(i) = NULL;
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(getCurrentPixelValue, Mu::Vector4f)
    {
        RvSession* s = RvSession::currentRvSession();

        const Vector2f inp = NODE_ARG(0, Vector2f);
        const float x = inp[0];
        const float y = inp[1];

        if (!s->currentFB())
        {
            NODE_RETURN(Mu::Vector4f());
        }

        Vector4f p;
        const TwkFB::FrameBuffer* fb = s->currentFB();

        const int fbw = fb->width();
        const int fbh = fb->height();

        const float ia = float(fbw) / float(fbh);
        int ix = int(x / ia * fbw);
        int iy = int(y * fbh);

        if (ix < 0)
            ix = 0;
        if (iy < 0)
            iy = 0;
        if (ix >= fbw)
            ix = fbw - 1;
        if (iy >= fbh)
            iy = fbh - 1;

        TwkFB::linearRGBA709pixelValue(fb, ix, iy, &p[0]);

        NODE_RETURN(p);
    }

    NODE_IMPLEMENTATION(getCurrentImageChannelNames, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        RvSession* s = RvSession::currentRvSession();
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!s->currentFB())
        {
            NODE_RETURN(0);
        }

        const TwkFB::FrameBuffer* fb = s->currentFB();

        DynamicArray* array = new DynamicArray(atype, 1);
        size_t nchannels = 0;

        for (const TwkFB::FrameBuffer* f = fb; f; f = f->nextPlane())
        {
            nchannels += f->numChannels();
        }

        array->resize(nchannels);

        int index = 0;

        for (const TwkFB::FrameBuffer* f = fb; f; f = f->nextPlane())
        {
            for (int i = 0; i < f->numChannels(); i++, index++)
            {
                array->element<ClassInstance*>(index) =
                    stype->allocate(f->channelName(i));
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(getCurrentAttributes, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        RvSession* s = RvSession::currentRvSession();
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* ttype = static_cast<const Class*>(atype->elementType());
        const StringType* stype = c->stringType();

        if (!s->currentFB())
        {
            NODE_RETURN(0);
        }

        const TwkFB::FrameBuffer* fb = s->currentFB();

        const TwkFB::FrameBuffer::AttributeVector& attributes =
            fb->attributes();

        struct STuple
        {
            StringType::String* attrName;
            StringType::String* attrValue;
        };

        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(attributes.size());

        for (int i = 0; i < attributes.size(); i++)
        {
            ClassInstance* tuple = ClassInstance::allocate(ttype);
            STuple* st = reinterpret_cast<STuple*>(tuple->structure());
            st->attrName = stype->allocate(attributes[i]->name());
            st->attrValue = stype->allocate(attributes[i]->valueAsString());

            array->element<ClassInstance*>(i) = tuple;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(sourceAttributes, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        RvSession* s = RvSession::currentRvSession();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Class* ttype = static_cast<const Class*>(atype->elementType());
        const StringType* stype = c->stringType();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* media =
            NODE_ARG_OBJECT(1, StringType::String);

        const TwkFB::FrameBuffer* fb =
            name ? s->currentFB(name->c_str()) : s->currentFB();

        //
        //  Even if we found an FB above, this source media may have changed
        //  since the last time it was rendered, in which case the attributes on
        //  the FB will be out-of-date. So if the caller requested specific
        //  media, see if we can find it.
        //

        if (media)
        {
            //
            //  The name doesn't exactly match one of the rendered images
            //

            if (IPNode* node = s->graph().findNode(name->c_str()))
            {
                if (SourceIPNode* snode = dynamic_cast<SourceIPNode*>(node))
                {
                    size_t index = snode->mediaIndex(media->c_str());
                    if (index == size_t(-1))
                        throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                                  "bad source name");
                    fb = &snode->mediaMovieInfo(index).proxy;
                }
            }
        }

        if (!fb)
            NODE_RETURN(0);

        const TwkFB::FrameBuffer::AttributeVector& attributes =
            fb->attributes();

        struct STuple
        {
            const StringType::String* attrName;
            const StringType::String* attrValue;
        };

        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(attributes.size());

        for (int i = 0; i < attributes.size(); i++)
        {
            ClassInstance* tuple = ClassInstance::allocate(ttype);
            STuple* st = reinterpret_cast<STuple*>(tuple->structure());
            st->attrName = stype->allocate(attributes[i]->name());
            st->attrValue = stype->allocate(attributes[i]->valueAsString());

            array->element<ClassInstance*>(i) = tuple;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(sourceDataAttributes, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        RvSession* s = RvSession::currentRvSession();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const TupleType* ttype =
            static_cast<const TupleType*>(atype->elementType());
        const DynamicArrayType* btype =
            static_cast<const DynamicArrayType*>(ttype->tupleFieldTypes()[1]);
        const StringType* stype = c->stringType();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* media =
            NODE_ARG_OBJECT(1, StringType::String);

        const TwkFB::FrameBuffer* fb =
            name ? s->currentFB(name->c_str()) : s->currentFB();

        //
        //  Even if we found an FB above, this source media may have changed
        //  since the last time it was rendered, in which case the attributes on
        //  the FB will be out-of-date. So if the caller requested specific
        //  media, see if we can find it.
        //

        if (media)
        {
            //
            //  The name doesn't exactly match one of the rendered images
            //

            if (IPNode* node = s->graph().findNode(name->c_str()))
            {
                if (SourceIPNode* snode = dynamic_cast<SourceIPNode*>(node))
                {
                    size_t index = snode->mediaIndex(media->c_str());
                    if (index == size_t(-1))
                        throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                                  "bad source name");
                    fb = &snode->mediaMovieInfo(index).proxy;
                }
            }
        }

        if (!fb)
            NODE_RETURN(0);

        const TwkFB::FrameBuffer::AttributeVector& attributes =
            fb->attributes();

        struct STuple
        {
            const StringType::String* attrName;
            const DynamicArray* attrValue;
        };

        int count = 0;

        for (int i = 0; i < attributes.size(); i++)
        {
            if (TwkFB::DataContainerAttribute* da =
                    dynamic_cast<TwkFB::DataContainerAttribute*>(attributes[i]))
            {
                count++;
            }
        }

        DynamicArray* array = new DynamicArray(atype, 1); // 1-dimensional
        array->resize(count);

        for (int i = 0, c = 0; i < attributes.size(); i++)
        {
            if (TwkFB::DataContainerAttribute* da =
                    dynamic_cast<TwkFB::DataContainerAttribute*>(attributes[i]))
            {
                ClassInstance* tuple = ClassInstance::allocate(ttype);
                STuple* st = reinterpret_cast<STuple*>(tuple->structure());
                st->attrName = stype->allocate(da->name());

                DynamicArray* barray =
                    new DynamicArray(btype, 1); // 1-dimensional
                const TwkFB::DataContainer* data = da->dataContainer();
                const size_t n = data->size();

                barray->resize(n);

                for (int q = 0; q < n; q++)
                {
                    barray->element<Mu::byte>(q) = (*data)[q];
                }

                st->attrValue = barray;
                array->element<ClassInstance*>(c) = tuple;
                c++;
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(sourcePixelValue, Mu::Vector4f)
    {
        MuLangContext* c = TwkApp::muContext();
        RvSession* s = RvSession::currentRvSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        float x = NODE_ARG(1, float);
        float y = NODE_ARG(2, float);

        Mu::Vector4f p;
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p[3] = 0;

        const TwkFB::FrameBuffer* fb =
            name ? s->currentFB(name->c_str()) : s->currentFB();

        if (fb && fb->uncrop())
        {
            //
            // Handle the case of an uncropped image. Reorient the coordinate
            // offsets based on the FB orientation. The uncrop x and y values
            // are always measured from the bottom left.
            //

            double offsetX = fb->uncropX();
            double offsetY = fb->uncropY();
            int orientation = fb->orientation();
            if (orientation == TwkFB::FrameBuffer::BOTTOMRIGHT
                || orientation == TwkFB::FrameBuffer::TOPRIGHT)
                offsetX = double(fb->uncropWidth()) - double(fb->width())
                          - double(fb->uncropX());
            if (orientation == TwkFB::FrameBuffer::TOPLEFT
                || orientation == TwkFB::FrameBuffer::TOPRIGHT)
                offsetY = double(fb->uncropHeight()) - double(fb->height())
                          - double(fb->uncropY());

            const double w = double(fb->width());
            const double h = double(fb->height());
            const double cx0 = double(offsetX) / double(fb->uncropWidth());
            const double cy0 = double(offsetY) / double(fb->uncropHeight());
            const double cx1 =
                double(offsetX + fb->width()) / double(fb->uncropWidth());
            const double cy1 =
                double(offsetY + fb->height()) / double(fb->uncropHeight());
            const double cw = cx1 - cx0;
            const double ch = cy1 - cy0;

            x = (double(x) / double(fb->uncropWidth()) - cx0) / cw
                * (fb->width() - 1);
            y = (double(y) / double(fb->uncropHeight()) - cy0) / ch
                * (fb->height() - 1);
        }

        // Because of tiling, we can obtain x/y coordinates that are in the
        // full-FB range. But they are always in the "high range" (ie.:
        // upper-right tile) so converting them to relative coords work.
        x = static_cast<int>(x + 0.5f) % fb->width();
        y = static_cast<int>(y + 0.5f) % fb->height();

        if (!fb || x >= fb->width() || y >= fb->height() || x < 0 || y < 0)
        {
            return p;
        }

        TwkFB::linearRGBA709pixelValue(fb, int(x), int(y), &p[0]);
        NODE_RETURN(p);
    }

    NODE_IMPLEMENTATION(sourceDisplayChannelNames, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = TwkApp::muContext();
        RvSession* s = RvSession::currentRvSession();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        const TwkFB::FrameBuffer* fb =
            name ? s->currentFB(name->c_str()) : s->currentFB();

        if (!fb)
            NODE_RETURN(0);

        DynamicArray* array = new DynamicArray(atype, 1);
        size_t nchannels = 0;

        for (const TwkFB::FrameBuffer* f = fb; f; f = f->nextPlane())
        {
            nchannels += f->numChannels();
        }

        array->resize(nchannels);

        int index = 0;

        for (const TwkFB::FrameBuffer* f = fb; f; f = f->nextPlane())
        {
            for (int i = 0; i < f->numChannels(); i++, index++)
            {
                array->element<ClassInstance*>(index) =
                    stype->allocate(f->channelName(i));
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(addSource, void)
    {
        Session* s = RvSession::currentRvSession();
        const StringType::String* source =
            NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* tag = NODE_ARG_OBJECT(1, StringType::String);

        if (!source)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil source");

        Document::ReadRequest request;
        request.setOption("tag", string(tag ? tag->c_str() : ""));
        request.setOption("append", true);

        IPGraph::GraphEdit edit(s->graph());
        s->read(source->c_str(), request);
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(addSourceMulti, void)
    {
        RvSession* s = RvSession::currentRvSession();
        DynamicArray* array = NODE_ARG_OBJECT(0, DynamicArray);
        StringType::String* muTag = NODE_ARG_OBJECT(1, StringType::String);
        string tag = (muTag) ? muTag->c_str() : "";

        vector<string> files;

        for (size_t i = 0; i < array->size(); i++)
            files.push_back(array->element<StringType::String*>(i)->c_str());

        IPGraph::GraphEdit edit(s->graph());
        s->addSourceWithTag(files, tag);
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(addSourceVerbose, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        DynamicArray* array = NODE_ARG_OBJECT(0, DynamicArray);
        StringType::String* muTag = NODE_ARG_OBJECT(1, StringType::String);
        string tag = (muTag) ? muTag->c_str() : "";
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();

        vector<string> filesAndOptions;
        filesAndOptions.reserve(array->size());
        for (size_t i = 0; i < array->size(); i++)
            filesAndOptions.emplace_back(
                array->element<StringType::String*>(i)->c_str());

        IPGraph::GraphEdit edit(s->graph());

        if (Options::sharedOptions().delaySessionLoading)
        {
            s->userGenericEvent("before-progressive-loading", "");

            // Note: It was decided to trigger the
            // before-progressive-proxy-loading event even when progressive
            // source loading is disabled
            s->userGenericEvent("before-progressive-proxy-loading", "");
        }

        SourceIPNode* node = s->addSourceWithTag(filesAndOptions, tag);

        if (Options::sharedOptions().delaySessionLoading)
        {
            // Note: It was decided to trigger the
            // after-progressive-proxy-loading event even when progressive
            // source loading is disabled
            s->userGenericEvent("after-progressive-proxy-loading", "");

            if (!Options::sharedOptions().progressiveSourceLoading)
            {
                s->userGenericEvent("after-progressive-loading", "");
            }
        }

        s->askForRedraw();

        StringType::String* nodeName = stype->allocate(node->name());
        NODE_RETURN(nodeName);
    }

    NODE_IMPLEMENTATION(addSources, void)
    {
        RvSession* s = RvSession::currentRvSession();
        DynamicArray* array = NODE_ARG_OBJECT(0, DynamicArray);
        StringType::String* muTag = NODE_ARG_OBJECT(1, StringType::String);
        bool processOpts = NODE_ARG(2, bool);
        bool merge = NODE_ARG(3, bool);
        string tag = (muTag) ? muTag->c_str() : "";

        vector<string> sargs;

        for (size_t i = 0; i < array->size(); i++)
        {
            string arg = array->element<StringType::String*>(i)->c_str();
            sargs.push_back(IPCore::Application::mapFromVar(arg));
        }

        //
        //  Note we do not make GraphEdit object here, since this load will be
        //  asynchronous.
        //

        s->readUnorganizedFileList(sargs, processOpts, merge, tag);
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(addSourcesVerbose, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        DynamicArray* array = NODE_ARG_OBJECT(0, DynamicArray);
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        StringType::String* muTag = NODE_ARG_OBJECT(1, StringType::String);
        string tag = (muTag) ? muTag->c_str() : "";
        MuLangContext* c = TwkApp::muContext();
        const StringType* stype = c->stringType();

        DynamicArray* names = new DynamicArray(atype, 1);
        names->resize(array->size());
        IPGraph::GraphEdit edit(s->graph());

        if (Options::sharedOptions().delaySessionLoading)
        {
            s->userGenericEvent("before-progressive-loading", "");

            // Note: It was decided to trigger the
            // before-progressive-proxy-loading event even when progressive
            // source loading is disabled
            s->userGenericEvent("before-progressive-proxy-loading", "");
        }

        for (size_t i = 0; i < array->size(); i++)
        {
            DynamicArray* currentSource = array->element<DynamicArray*>(i);

            vector<string> filesAndOptions;
            filesAndOptions.reserve(currentSource->size());
            for (size_t j = 0; j < currentSource->size(); j++)
            {
                string arg =
                    currentSource->element<StringType::String*>(j)->c_str();
                filesAndOptions.emplace_back(
                    IPCore::Application::mapFromVar(arg));
            }

            string nodeName = s->addSourceWithTag(filesAndOptions, tag)->name();
            names->element<StringType::String*>(i) = stype->allocate(nodeName);
        }

        if (Options::sharedOptions().delaySessionLoading)
        {
            // Note: It was decided to trigger the
            // after-progressive-proxy-loading event even when progressive
            // source loading is disabled
            s->userGenericEvent("after-progressive-proxy-loading", "");

            if (!Options::sharedOptions().progressiveSourceLoading)
            {
                s->userGenericEvent("after-progressive-loading", "");
            }
        }
        s->askForRedraw();
        NODE_RETURN(names);
    }

    NODE_IMPLEMENTATION(addSourceBegin, void)
    {
        RvSession* s = RvSession::currentRvSession();
        s->rvgraph().addSourceBegin();
    }

    NODE_IMPLEMENTATION(addSourceEnd, void)
    {
        RvSession* s = RvSession::currentRvSession();
        IPGraph::GraphEdit edit(s->graph());
        s->rvgraph().addSourceEnd();
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(setProgressiveSourceLoading, void)
    {
        const bool enable = NODE_ARG(0, bool);

        // Note : the settings need to be applied to two separate entities
        // because they correspond to two different levels of abstraction:
        // 1. The RV App high level Rv::Options::sharedOptions()
        // 2. The IPCore lower level of abstraction:
        // IPCore::Application::optionValue I decided to update both instead of
        // relying only on one and risking that the other gets used
        // inadvertantly in the future.
        Rv::Options::sharedOptions().progressiveSourceLoading = enable;
        IPCore::Application::setOptionValue<bool>("progressiveSourceLoading",
                                                  enable);
    }

    NODE_IMPLEMENTATION(progressiveSourceLoading, bool)
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        NODE_RETURN(opts.progressiveSourceLoading != 0);
    }

    NODE_IMPLEMENTATION(addToSource, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* tag = NODE_ARG_OBJECT(1, StringType::String);

        if (!file)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil file arg");

        IPGraph::GraphEdit edit(s->graph());
        s->addToSource("", file->c_str(), string(tag ? tag->c_str() : ""));
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(addToSource2, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* node = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* file = NODE_ARG_OBJECT(1, StringType::String);
        StringType::String* tag = NODE_ARG_OBJECT(2, StringType::String);

        if (!node)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node arg");
        if (!file)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil file arg");

        IPGraph::GraphEdit edit(s->graph());
        s->addToSource(node->c_str(), file->c_str(),
                       string(tag ? tag->c_str() : ""));
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(setSourceMedia, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* node = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* array = NODE_ARG_OBJECT(1, DynamicArray);
        StringType::String* tag = NODE_ARG_OBJECT(2, StringType::String);

        if (!node)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node arg");

        vector<string> files;

        for (size_t i = 0; i < array->size(); i++)
            files.push_back(array->element<StringType::String*>(i)->c_str());

        IPGraph::GraphEdit edit(s->graph());
        s->setSourceMedia(node->c_str(), files,
                          string(tag ? tag->c_str() : ""));
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(addSourceMediaRep, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceNode = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* mediaRepName =
            NODE_ARG_OBJECT(1, StringType::String);
        DynamicArray* mediaRepPathsAndOptionsArg =
            NODE_ARG_OBJECT(2, DynamicArray);
        StringType::String* tag = NODE_ARG_OBJECT(3, StringType::String);
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();

        if (!sourceNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source node arg");
        if (!mediaRepName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil media representation name arg");
        if (!mediaRepPathsAndOptionsArg)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil media representation paths arg");

        vector<string> mediaRepPathsAndOptions;
        mediaRepPathsAndOptions.reserve(mediaRepPathsAndOptionsArg->size());
        for (size_t i = 0; i < mediaRepPathsAndOptionsArg->size(); i++)
            mediaRepPathsAndOptions.emplace_back(
                mediaRepPathsAndOptionsArg->element<StringType::String*>(i)
                    ->c_str());

        IPGraph::GraphEdit edit(s->graph());
        SourceIPNode* node = s->addSourceMediaRep(
            sourceNode->c_str(), mediaRepName->c_str(), mediaRepPathsAndOptions,
            string(tag ? tag->c_str() : ""));
        StringType::String* nodeName =
            stype->allocate(node ? node->name() : "");
        NODE_RETURN(nodeName);
    }

    NODE_IMPLEMENTATION(setActiveSourceMediaRep, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceNode = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* mediaRepName =
            NODE_ARG_OBJECT(1, StringType::String);
        StringType::String* tag = NODE_ARG_OBJECT(2, StringType::String);

        if (!sourceNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source node arg");
        if (!mediaRepName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil media representation name arg");

        IPGraph::GraphEdit edit(s->graph());

        try
        {
            s->setActiveSourceMediaRep(sourceNode->c_str(),
                                       mediaRepName->c_str(),
                                       string(tag ? tag->c_str() : ""));
        }
        catch (TwkExc::Exception& exc)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, exc.what());
        }

        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(sourceMediaRep, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceNode = NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!sourceNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source node arg");

        std::string mediaRepName = s->sourceMediaRep(sourceNode->c_str());
        StringType::String* name = stype->allocate(mediaRepName);

        NODE_RETURN(name);
    }

    NODE_IMPLEMENTATION(sourceMediaReps, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceNode = NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!sourceNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source node arg");

        Session::StringVector mediaReps;
        s->sourceMediaReps(sourceNode->c_str(), mediaReps);
        DynamicArray* names = new DynamicArray(atype, 1);
        names->resize(mediaReps.size());
        for (size_t i = 0; i < mediaReps.size(); i++)
        {
            names->element<StringType::String*>(i) =
                stype->allocate(mediaReps[i]);
        }

        NODE_RETURN(names);
    }

    NODE_IMPLEMENTATION(sourceMediaRepsAndNodes, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceOrSwitchNode =
            NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const TupleType* ttype =
            static_cast<const TupleType*>(atype->elementType());
        const StringType* stype = c->stringType();

        if (!sourceOrSwitchNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source or switch node arg");

        Session::StringVector mediaRepNames;
        Session::StringVector mediaRepSrcNodes;
        s->sourceMediaReps(sourceOrSwitchNode->c_str(), mediaRepNames,
                           &mediaRepSrcNodes);

        if (mediaRepNames.empty()
            || (mediaRepNames.size() != mediaRepSrcNodes.size()))
            NODE_RETURN(0);

        struct STuple
        {
            const StringType::String* mediaRepName;
            const StringType::String* mediaRepSrcNode;
        };

        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(mediaRepNames.size());

        for (int i = 0; i < mediaRepNames.size(); i++)
        {
            ClassInstance* tuple = ClassInstance::allocate(ttype);
            STuple* st = reinterpret_cast<STuple*>(tuple->structure());
            st->mediaRepName = stype->allocate(mediaRepNames[i]);
            st->mediaRepSrcNode = stype->allocate(mediaRepSrcNodes[i]);

            array->element<ClassInstance*>(i) = tuple;
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(sourceMediaRepSwitchNode, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceNode = NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!sourceNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source node arg");

        std::string switchNodeName =
            s->sourceMediaRepSwitchNode(sourceNode->c_str());
        StringType::String* name = stype->allocate(switchNodeName);

        NODE_RETURN(name);
    }

    NODE_IMPLEMENTATION(sourceMediaRepSourceNode, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* sourceNode = NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArrayType* atype =
            reinterpret_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype = c->stringType();

        if (!sourceNode)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "nil source node arg");

        std::string sourceNodeName =
            s->sourceMediaRepSourceNode(sourceNode->c_str());
        StringType::String* name = stype->allocate(sourceNodeName);

        NODE_RETURN(name);
    }

    NODE_IMPLEMENTATION(relocateSource, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* oldName = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* newName = NODE_ARG_OBJECT(1, StringType::String);

        IPGraph::GraphEdit edit(s->graph());
        s->relocateSource(oldName->c_str(), newName->c_str());
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(relocateSource2, void)
    {
        RvSession* s = RvSession::currentRvSession();
        StringType::String* node = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* oldName = NODE_ARG_OBJECT(1, StringType::String);
        StringType::String* newName = NODE_ARG_OBJECT(2, StringType::String);

        if (!node)
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil node arg");
        if (!oldName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "old media name is nil");
        if (!newName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "new media name is nil");

        IPGraph::GraphEdit edit(s->graph());
        s->relocateSource(oldName->c_str(), newName->c_str(), node->c_str());
        s->askForRedraw();
    }

    NODE_IMPLEMENTATION(commandLineFlag, Pointer)
    {
        RvSession* s = RvSession::currentRvSession();
        const StringType* t = static_cast<const StringType*>(NODE_THIS.type());
        const StringType::String* key = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* defaultValue =
            NODE_ARG_OBJECT(1, StringType::String);

        if (!key)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "flag key is nil");

        string val = s->lookupMuFlag(key->c_str());

        if (val.empty())
            NODE_RETURN(Pointer(defaultValue));
        NODE_RETURN(t->allocate(val));
    }

    NODE_IMPLEMENTATION(ocioUpdateConfig, void)
    {
        Session* s = Session::currentSession();
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

        IPNode* node = 0;

        if (name)
        {
            if (!(node = s->graph().findNode(name->c_str())))
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "Bad ocio node name");
            }
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Null ocio node name");
        }

        OCIOIPNode* ocioNode = dynamic_cast<OCIOIPNode*>(node);
        if (!ocioNode)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Not an ocio node");
        }

        try
        {
            ocioNode->updateConfig();
        }
        catch (TwkExc::Exception& exc)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, exc.what());
        }
    }

    NODE_IMPLEMENTATION(shortAppName, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType* stype = c->stringType();

        StringType::String* name = stype->allocate(TWK_DEPLOY_SHORT_APP_NAME());

        NODE_RETURN(name);
    }

    NODE_IMPLEMENTATION(licensingState, int)
    {
        NODE_RETURN(TWK_DEPLOY_GET_LICENSE_STATE());
    }

    //----------------------------------------------------------------------
    //
    //  INIT MODULE
    //

    void initCommands(Mu::MuLangContext* context)
    {
        theTimer.start();

        USING_MU_FUNCTION_SYMBOLS;
        typedef ParameterVariable Param;

        if (!context)
            context = TwkApp::muContext();
        MuLangContext* c = context;
        Symbol* root = context->globalScope();
        Name cname = c->internName("commands");
        Mu::Module* commands = root->findSymbolOfType<Mu::Module>(cname);
        // Mu::GLModule::init("gl", context);
        context->globalScope()->addSymbol(new Mu::GLModule(c, "gl"));
        context->globalScope()->addSymbol(new Mu::GLTextModule(c, "gltext"));
        context->globalScope()->addSymbol(new Mu::GLUModule(c, "glu"));

        Module* autodoc = new Mu::AutoDocModule(context, "autodoc");
        Module* io = new Mu::IOModule(context, "io");
        Module* sys = new Mu::SystemModule(context, "system");
        Module* img = new Mu::ImageModule(context, "image");
        Module* enc = new Mu::EncodingModule(context, "encoding");
        Module* lin = new Mu::MathLinearModule(context, "math_linear");

        root->addSymbols(autodoc, io, sys, img, enc, lin, EndArguments);

        TwkApp::muModuleList().push_back(autodoc);
        TwkApp::muModuleList().push_back(io);
        IPMu::initCommands(c);

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

        Context::NameValuePairs fields;
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

        fields.resize(7);
        fields[0] = make_pair(string("start"), context->intType());
        fields[1] = make_pair(string("end"), context->intType());
        fields[2] = make_pair(string("inc"), context->intType());
        fields[3] = make_pair(string("fps"), context->floatType());
        fields[4] = make_pair(string("cutIn"), context->intType());
        fields[5] = make_pair(string("cutOut"), context->intType());
        fields[6] = make_pair(string("isUndiscovered"), context->boolType());
        context->structType(0, "NodeRangeInfo", fields);

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

        types.clear();
        types.push_back(context->stringType());
        types.push_back(context->arrayType(context->byteType(), 1, 0));
        context->arrayType(context->tupleType(types), 1, 0);

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

        types.clear(); // edl commands use (int,int,int)
        types.push_back(context->intType());
        types.push_back(context->intType());
        types.push_back(context->intType());
        context->tupleType(types);

        types.clear(); // sourceMedia -> (string,string[],string[])
        types.push_back(context->stringType());
        types.push_back(context->arrayType(context->stringType(), 1, 0));
        types.push_back(context->arrayType(context->stringType(), 1, 0));
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

            new SymbolicConstant(c, "SequenceSession", "int",
                                 Value(Session::SequenceSession)),
            new SymbolicConstant(c, "StackSession", "int",
                                 Value(Session::StackSession)),

            new SymbolicConstant(c, "CacheOff", "int",
                                 Value(Session::NeverCache)),
            new SymbolicConstant(c, "CacheBuffer", "int",
                                 Value(Session::BufferCache)),
            new SymbolicConstant(c, "CacheGreedy", "int",
                                 Value(Session::GreedyCache)),

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

            new Function(c, "data", data, None, Return, "object", End),

            new Function(c, "sources", sources, None, Return,
                         "(string,int,int,int,float,bool,bool)[]", End),

            new Function(c, "sourceMedia", sourceMedia, None, Return,
                         "(string,string[],string[])", Parameters,
                         new Param(c, "sourceName", "string"), End),

            new Function(c, "sourcePixelValue", sourcePixelValue, None, Return,
                         "vector float[4]", Parameters,
                         new Param(c, "sourceName", "string"),
                         new Param(c, "px", "float"),
                         new Param(c, "py", "float"), End),

            new Function(c, "sourceAttributes", sourceAttributes, None, Return,
                         "(string,string)[]", Parameters,
                         new Param(c, "sourceName", "string"),
                         new Param(c, "mediaName", "string", Value()), End),

            new Function(c, "sourceDataAttributes", sourceDataAttributes, None,
                         Return, "(string,byte[])[]", Parameters,
                         new Param(c, "sourceName", "string"),
                         new Param(c, "mediaName", "string", Value()), End),

            new Function(c, "sourceDisplayChannelNames",
                         sourceDisplayChannelNames, None, Parameters,
                         new Param(c, "sourceName", "string"), Return,
                         "string[]", End),

            new Function(c, "getCurrentImageSize", getCurrentImageSize, None,
                         Return, "vector float[2]", End),

            new Function(c, "getCurrentPixelValue", getCurrentPixelValue, None,
                         Return, "vector float[4]", Parameters,
                         new Param(c, "point", "vector float[2]"), End),

            new Function(c, "getCurrentAttributes", getCurrentAttributes, None,
                         Return, "(string,string)[]", End),

            new Function(c, "getCurrentImageChannelNames",
                         getCurrentImageChannelNames, None, Return, "string[]",
                         End),

            new Function(c, "getCurrentNodesOfType", getCurrentNodesOfType,
                         None, Parameters, new Param(c, "typeName", "string"),
                         Return, "string[]", End),

            new Function(c, "addSource", addSource, None, Return, "void",
                         Parameters, new Param(c, "fileName", "string"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "addSource", addSourceMulti, None, Return, "void",
                         Parameters, new Param(c, "fileNames", "string[]"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "addSourceVerbose", addSourceVerbose, None, Return,
                         "string", Parameters,
                         new Param(c, "filePathsAndOptions", "string[]"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "addSourceBegin", addSourceBegin, None, Return,
                         "void", End),

            new Function(c, "addSourceEnd", addSourceEnd, None, Return, "void",
                         End),

            new Function(c, "addSources", addSources, None, Return, "void",
                         Parameters, new Param(c, "fileNames", "string[]"),
                         new Param(c, "tag", "string", Value()),
                         new Param(c, "processOpts", "bool", Value(false)),
                         new Param(c, "merge", "bool", Value(false)), End),

            new Function(c, "addSourcesVerbose", addSourcesVerbose, None,
                         Return, "string[]", Parameters,
                         new Param(c, "filePathsAndOptions", "string[][]"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "setProgressiveSourceLoading",
                         setProgressiveSourceLoading, None, Return, "void",
                         Parameters, new Param(c, "enable", "bool"), End),

            new Function(c, "progressiveSourceLoading",
                         progressiveSourceLoading, None, Return, "bool", End),

            new Function(
                c, "newImageSource", newImageSource, None, Return, "string",
                Parameters, new Param(c, "mediaName", "string"),
                new Param(c, "width", "int"), new Param(c, "height", "int"),
                new Param(c, "uncropWidth", "int"),
                new Param(c, "uncropHeight", "int"),
                new Param(c, "uncropX", "int"), new Param(c, "uncropY", "int"),
                new Param(c, "pixelAspect", "float"),
                new Param(c, "channels", "int"),
                new Param(c, "bitsPerChannel", "int"),
                new Param(c, "floatingPoint", "bool"),
                new Param(c, "startFrame", "int"),
                new Param(c, "endFrame", "int"), new Param(c, "fps", "float"),
                new Param(c, "layers", "string[]", Value()),
                new Param(c, "views", "string[]", Value()), End),

            new Function(c, "newImageSourcePixels", newImageSourcePixels, None,
                         Return, "string", Parameters,
                         new Param(c, "sourceName", "string"),
                         new Param(c, "frame", "int"),
                         new Param(c, "layer", "string", Value()),
                         new Param(c, "view", "string", Value()), End),

            new Function(c, "insertCreatePixelBlock", insertCreatePixelBlock,
                         None, Return, "void", Parameters,
                         new Param(c, "event", "Event"), End),

            new Function(c, "addToSource", addToSource, None, Return, "void",
                         Parameters, new Param(c, "fileName", "string"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "addToSource", addToSource2, None, Return, "void",
                         Parameters, new Param(c, "sourceNode", "string"),
                         new Param(c, "fileName", "string"),
                         new Param(c, "tag", "string"), End),

            new Function(c, "setSourceMedia", setSourceMedia, None, Return,
                         "void", Parameters,
                         new Param(c, "sourceNode", "string"),
                         new Param(c, "fileNames", "string[]"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "addSourceMediaRep", addSourceMediaRep, None,
                         Return, "string", Parameters,
                         new Param(c, "sourceNode", "string"),
                         new Param(c, "mediaRepName", "string"),
                         new Param(c, "mediaRepPathsAndOptions", "string[]"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "setActiveSourceMediaRep", setActiveSourceMediaRep,
                         None, Return, "void", Parameters,
                         new Param(c, "sourceNode", "string"),
                         new Param(c, "mediaRepName", "string"),
                         new Param(c, "tag", "string", Value()), End),

            new Function(c, "sourceMediaRep", sourceMediaRep, None, Return,
                         "string", Parameters,
                         new Param(c, "sourceNode", "string"), End),

            new Function(c, "sourceMediaReps", sourceMediaReps, None, Return,
                         "string[]", Parameters,
                         new Param(c, "sourceNode", "string"), End),

            new Function(c, "sourceMediaRepsAndNodes", sourceMediaRepsAndNodes,
                         None, Return, "(string,string)[]", Parameters,
                         new Param(c, "sourceOrSwitchNode", "string"), End),

            new Function(c, "sourceMediaRepSwitchNode",
                         sourceMediaRepSwitchNode, None, Return, "string",
                         Parameters, new Param(c, "sourceNode", "string"), End),

            new Function(c, "sourceMediaRepSourceNode",
                         sourceMediaRepSourceNode, None, Return, "string",
                         Parameters, new Param(c, "sourceNode", "string"), End),

            new Function(c, "relocateSource", relocateSource, None, Return,
                         "void", Parameters,
                         new Param(c, "oldFileName", "string"),
                         new Param(c, "newFileName", "string"), End),

            new Function(c, "relocateSource", relocateSource2, None, Return,
                         "void", Parameters,
                         new Param(c, "sourceNode", "string"),
                         new Param(c, "oldFileName", "string"),
                         new Param(c, "newFileName", "string"), End),

            new Function(
                c, "commandLineFlag", commandLineFlag, None, Return, "string",
                Parameters, new Param(c, "flagName", "string"),
                new Param(c, "defaultValue", "string", Value(Pointer(0))), End),

            new Function(c, "readLUT", readLUT, None, Return, "void",
                         Parameters, new Param(c, "filename", "string"),
                         new Param(c, "nodeName", "string"),
                         new Param(c, "activate", "bool", Value(false)), End),

            new Function(c, "readCDL", readCDL, None, Return, "void",
                         Parameters, new Param(c, "filename", "string"),
                         new Param(c, "nodeName", "string"),
                         new Param(c, "activate", "bool", Value(false)), End),

            new Function(c, "setSessionType", setSessionType, None, Return,
                         "void", Parameters, new Param(c, "sessionType", "int"),
                         End),

            new Function(c, "getSessionType", getSessionType, None, Return,
                         "int", End),

            new Function(c, "startTimer", startTimer, None, Return, "void",
                         End),

            new Function(c, "elapsedTime", elapsedTime, None, Return, "float",
                         End),

            new Function(c, "theTime", theTime, None, Return, "float", End),

            new Function(c, "stopTimer", stopTimer, None, Return, "float", End),

            new Function(c, "isTimerRunning", isTimerRunning, None, Return,
                         "bool", End),

            new Function(c, "updateLUT", updateLUT, None, Return, "void", End),

            new Function(c, "exportCurrentFrame", exportCurrentFrame, None,
                         Return, "void", Parameters,
                         new Param(c, "filename", "string"), End),

            new Function(c, "exportCurrentSourceFrame",
                         exportCurrentSourceFrame, None, Return, "void",
                         Parameters, new Param(c, "filename", "string"), End),

            new Function(c, "optionsPlay", optionsPlay, None, Return, "int",
                         End),

            new Function(c, "optionsPlayReset", optionsPlayReset, None, Return,
                         "void", End),

            new Function(c, "optionsNoPackages", optionsNoPackages, None,
                         Return, "int", End),

            new Function(c, "optionsProgressiveLoading",
                         optionsProgressiveLoading, None, Return, "int", End),

            new Function(c, "loadTotal", loadTotal, None, Return, "int", End),

            new Function(c, "loadCount", loadCount, None, Return, "int", End),

            new Function(c, "shortAppName", shortAppName, None, Return,
                         "string", End),

            new Function(c, "ocioUpdateConfig", ocioUpdateConfig, None, Return,
                         "void", Parameters,
                         new Param(c, "node", "string", Value(Pointer(0))),
                         End),

            new Function(c, "licensingState", licensingState, None, Return,
                         "int", End),

            EndArguments);
    }

} // namespace Rv
