//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MovieMuDraw/MovieMuDraw.h>
#include <Mu/FunctionType.h>
#include <Mu/List.h>
#include <Mu/Module.h>
#include <Mu/TupleType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/MuLangContext.h>
#include <TwkAudio/Audio.h>
#include <TwkExc/Exception.h>
#include <TwkFB/Exception.h>
#include <TwkFB/IO.h>
#include <TwkGLText/TwkGLText.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLState.h>
#include <iostream>

#if defined(PLATFORM_WINDOWS)
#define SERIALIZE
#endif

namespace TwkMovie
{
    using namespace std;
    using namespace TwkMath;
    using namespace TwkAudio;
    using namespace TwkGLF;
    using namespace Mu;

    static bool init = true;
    static pthread_mutex_t lock;

    MovieMuDraw::MovieMuDraw(Movie* movie, MuLangContext* context,
                             Process* process, TwkGLF::GLVideoDevice* d)
        : Movie()
        , m_context(context)
        , m_process(process)
        , m_main(0)
        , m_extraArg(false)
        , m_movie(movie)
        , m_device(d)
    {
#ifdef SERIALIZE
        if (true)
        {
            init = false;
            pthread_mutex_init(&lock, 0);
        }
#endif

        m_info = movie->info();
    }

    MovieMuDraw::~MovieMuDraw()
    {
        // delete m_process;
    }

    Movie* MovieMuDraw::clone() const { return 0; }

    void MovieMuDraw::setFunction(const string& moduleName,
                                  const StringVector& args)
    {
        static const char* funcSig = "(void;int,int,int,int,int,int,"
                                     "bool,bool,int,[string])";

        static const char* funcSig2 =
            "(void;int,int,int,int,int,int,"
            "bool,bool,int,[string],[(string,string)])";

        Name name = m_context->internName(moduleName.c_str());
        Module* module = Module::load(name, m_process, m_context);

        if (!module)
        {
            TWK_THROW_EXC_STREAM("can't find " << moduleName);
        }

        if (m_main = module->findSymbolOfType<Mu::Function>(
                m_context->internName("main")))
        {
            bool sig1 =
                m_main->type() == m_context->parseType(funcSig, m_process);
            bool sig2 =
                m_main->type() == m_context->parseType(funcSig2, m_process);

            if (!sig1 && !sig2)
            {
                TWK_THROW_EXC_STREAM(moduleName
                                     << ".main has wrong function signature");
            }

            m_extraArg = sig2;
        }
        else
        {
            TWK_THROW_EXC_STREAM(moduleName << ".main not found");
        }

        if (m_init = module->findSymbolOfType<Mu::Function>(
                m_context->internName("init")))
        {
            if (m_init->type() != m_context->parseType(funcSig, m_process))
            {
                TWK_THROW_EXC_STREAM(moduleName
                                     << ".init has wrong function signature");
            }
        }
        else
        {
            // init doesn't have to be there
            // TWK_THROW_EXC_STREAM(moduleName << ".init not found");
        }

        m_argv = args;
    }

    struct StringTuple
    {
        Pointer _1;
        Pointer _2;
    };

    void MovieMuDraw::imagesAtFrame(const ReadRequest& request,
                                    FrameBufferVector& fbs)
    {
        int frame = request.frame;
        bool stereo = request.stereo;

        m_movie->imagesAtFrame(request, fbs);
        if (!m_main)
            return;

#ifdef SERIALIZE
        pthread_mutex_lock(&lock);
#endif
        Thread* t = m_process->newApplicationThread();

        const Class* stype = m_context->stringType();
        Context::TypeVector types(2);
        types[0] = stype;
        types[1] = stype;

        const Class* tupleType = m_context->tupleType(types);
        const ListType* listType = m_context->listType(tupleType);

        for (size_t i = 0; i < fbs.size(); i++)
        {
            TwkFB::FrameBuffer* fb = fbs[i];
            TwkGLF::GLState* glstate =
                fb->attribute<TwkGLF::GLState*>("glState");

            if (TwkGLF::GLVideoDevice* d =
                    fb->attribute<TwkGLF::GLVideoDevice*>("videoDevice"))
            {
                d->setDefaultFBOIndex(i);
                d->makeCurrent(fb);
            }
            else if (m_device)
            {
                m_device->makeCurrent(fb);
            }

            //
            //  The safest way to deal with these allocated objects is on
            //  the stack. The garbage collector is sure to find them
            //  here. The downside is we need to rebuild every frame, but
            //  this is typically not an expensive process (relatively
            //  speaking).
            //

            ArgumentVector args;

            //
            //  There are only two possibilities for m_main->numArgs
            //  (10 and 11).  See setFunction, above.
            //

            args.resize(10 + (m_extraArg ? 1 : 0));

            args[0]._int = m_movie->info().uncropWidth;
            args[1]._int = m_movie->info().uncropHeight;
            args[2]._int = 0;
            args[3]._int = 0;
            args[4]._int = m_movie->info().uncropWidth;
            args[5]._int = m_movie->info().uncropHeight;
            args[6]._bool = stereo; // stereo
            args[7]._bool = i == 1; // right eye
            args[8]._int = frame;   // frame

            //
            //  Create the argv list argument
            //

            const StringType* stype = m_context->stringType();
            const ListType* listType = m_context->listType(stype);

            List arglist(m_process, listType);

            for (size_t j = 0; j < m_argv.size(); j++)
            {
                StringType::String* s = stype->allocate(m_argv[j].c_str());
                arglist.append((ClassInstance*)s);
            }

            args[9]._Pointer = arglist.head();

            //
            //  Extra args
            //

            if (m_extraArg)
            {
                Context::TypeVector types;
                types.push_back(stype);
                types.push_back(stype);
                const TupleType* ttype = m_context->tupleType(types);
                const ListType* listType = m_context->listType(ttype);

                List arglist2(m_process, listType);

                const TwkFB::FrameBuffer::AttributeVector& attrs =
                    fb->attributes();

                for (size_t j = 0; j < attrs.size(); j++)
                {
                    if (attrs[j]->name() == "glfb")
                        continue;
                    ClassInstance* tupleObj = ClassInstance::allocate(ttype);
                    TT* tt = tupleObj->data<TT>();
                    tt->name = stype->allocate(attrs[j]->name());
                    tt->value = stype->allocate(attrs[j]->valueAsString());
                    arglist2.append(tupleObj);
                }

                args[10]._Pointer = arglist2.head();
            }

            //
            //  Call the function
            //

            GLState::FixedFunctionPipeline FFP(glstate);

            glPushAttrib(GL_ALL_ATTRIB_BITS);
            FFP.setViewport(0, 0, m_movie->info().uncropWidth,
                            m_movie->info().uncropHeight);

            if (fb->attribute<string>("renderer") == "sw" && fbs.size() > 1)
            {
                GLenum dtype = GL_FLOAT;
                GLenum ctype = fb->numChannels() == 3 ? GL_RGB : GL_RGBA;

                switch (fb->dataType())
                {
                case FrameBuffer::FLOAT:
                    dtype = GL_FLOAT;
                    break;
                case FrameBuffer::HALF:
                    dtype = GL_HALF_FLOAT_ARB;
                    break;
                case FrameBuffer::UCHAR:
                    dtype = GL_UNSIGNED_INT_8_8_8_8;
                    break;
                case FrameBuffer::USHORT:
                    dtype = GL_UNSIGNED_SHORT;
                    break;
                default:
                    break;
                }

                glDrawPixels(fb->width(), fb->height(), ctype, dtype,
                             fb->pixels<GLvoid>());
            }

            if (m_init)
            {
                m_process->call(t, m_init, args);
                m_init = 0;
            }

            Value v = m_process->call(t, m_main, args);
            TWK_GLDEBUG;
            glPopAttrib();

            if (t->uncaughtException())
            {
                cerr << "ERROR: caught exception from ";
                m_main->output(cerr);
                cerr << endl;
            }

            if (fb->attribute<string>("renderer") == "hw")
            {
                GLenum dtype = GL_FLOAT;
                GLenum ctype = fb->numChannels() == 3 ? GL_RGB : GL_RGBA;

                switch (fb->dataType())
                {
                case FrameBuffer::FLOAT:
                    dtype = GL_FLOAT;
                    break;
                case FrameBuffer::HALF:
                    dtype = GL_HALF_FLOAT_ARB;
                    break;
                case FrameBuffer::UCHAR:
                    dtype = GL_UNSIGNED_INT_8_8_8_8;
                    break;
                case FrameBuffer::USHORT:
                    dtype = GL_UNSIGNED_SHORT;
                    break;
                default:
                    break;
                }

                glReadPixels(0, 0, fb->width(), fb->height(), ctype, dtype,
                             fb->pixels<GLvoid>());
            }

            glFlush();
        }

        m_process->releaseApplicationThread(t);
#ifdef SERIALIZE
        pthread_mutex_unlock(&lock);
#endif
    }

    void MovieMuDraw::identifiersAtFrame(const ReadRequest& request,
                                         IdentifierVector& ids)
    {
        m_movie->identifiersAtFrame(request, ids);
    }

    size_t MovieMuDraw::audioFillBuffer(const AudioReadRequest& request,
                                        AudioBuffer& buffer)
    {
        return m_movie->audioFillBuffer(request, buffer);
    }

    void MovieMuDraw::flush() { m_movie->flush(); }

} // namespace TwkMovie
