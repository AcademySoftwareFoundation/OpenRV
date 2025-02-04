//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <TwkGLF/GL.h>
#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#endif

#include <FTGL/ftgl.h>
#include <TwkGLText/TwkGLText.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <utf8/unchecked.h>
#include <string>
#include <vector>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <TwkGLText/defaultFont.h>
#include <iterator>
#include <pthread.h>

#ifdef WIN32
#define utf8toWChar(T, W) \
    utf8::unchecked::utf8to16(T, T + strlen(T), std::back_inserter(W))
#else
#define utf8toWChar(T, W) \
    utf8::unchecked::utf8to32(T, T + strlen(T), std::back_inserter(W))
#endif

namespace TwkGLText
{
    using namespace std;

    struct GLTextContext
    {
        GLTextContext()
            : color(0.0, 0.0, 0.0, 1.0)
            , initialized(false)
            , size(12)
            , fontName("")
        {
            // cout << "GLTextContext -> " << this << endl;
        }

        ~GLTextContext()
        {
            // cout << "~GLTextContext -> " << this << endl;
            if (initialized)
            {
                clear();
            }
        }

        void clear()
        {
            for (std::map<string, std::vector<FTFont*>*>::iterator it =
                     fonts.begin();
                 it != fonts.end(); it++)
            {
                for (int i = 0; i < it->second->size(); i++)
                {
                    delete (*it->second)[i];
                }
                delete it->second;
            }
            fonts.clear();
            initialized = false;
            fontName = "";
        }

        string fontName;
        TwkMath::Col4f color;
        bool initialized;
        int size;
        std::map<string, std::vector<FTFont*>*> fonts;
    };

    static bool usePixmaps = false;
    static pthread_once_t threadInit = PTHREAD_ONCE_INIT;
    static pthread_key_t threadKey;

    static void thread_once(void)
    {
        if (pthread_key_create(&threadKey, NULL) != 0)
        {
            cout << "ERRRO: pthread_key_create failed: in " << __FUNCTION__
                 << ", " << __FILE__ << ", line " << __LINE__ << endl;
        }
    }

    static FTFont* newFont(const unsigned char* data, size_t size)
    {
        if (usePixmaps)
            return new FTPixmapFont(data, size);
        else
            return new FTTextureFont(data, size);
    }

    static FTFont* newFont(const char* name)
    {
        if (usePixmaps)
            return new FTPixmapFont(name);
        else
            return new FTTextureFont(name);
    }

    Context GLtext::getContext()
    {
        if (Context existing_ctx = pthread_getspecific(threadKey))
        {
            return existing_ctx;
        }
        else
        {
            Context ctx = newContext();
            setContext(ctx);
            return ctx;
        }
    }

    Context GLtext::newContext()
    {
        pthread_once(&threadInit, thread_once);

        usePixmaps = false;

        GLTextContext* ctx = new GLTextContext();

        if (ctx->fonts.size() == 0)
        {
            std::vector<FTFont*>* v_font = new std::vector<FTFont*>();
            v_font->resize(ctx->size + 1);
            ctx->fonts.insert(make_pair(ctx->fontName, v_font));
        }

        if ((*ctx->fonts[ctx->fontName]).size() <= ctx->size)
            (*ctx->fonts[ctx->fontName]).resize(ctx->size + 1);
        if (!(*ctx->fonts[ctx->fontName])[ctx->size])
            (*ctx->fonts[ctx->fontName])[ctx->size] =
                newFont(default_font, 67548);
        (*ctx->fonts[ctx->fontName])[ctx->size]->FaceSize(ctx->size);
        ctx->initialized = true;

        return ctx;
    }

    void GLtext::deleteContext(Context c) { delete (GLTextContext*)c; }

    void GLtext::setContext(Context c)
    {
        GLTextContext* ctx = (GLTextContext*)c;

        if (pthread_getspecific(threadKey) != (void*)ctx)
        {
            // cout << "setting " << c << endl;
            pthread_setspecific(threadKey, c);
        }
    }

    void GLtext::clear()
    {
        if (GLTextContext* ctx = (GLTextContext*)getContext())
        {
            ctx->clear();
        }
    }

    void GLtext::init()
    {
        if (GLTextContext* ctx = (GLTextContext*)getContext())
        {
            ctx->fontName = "";
            if (ctx->fonts.count(ctx->fontName) == 0)
            {
                std::vector<FTFont*>* v_font = new std::vector<FTFont*>();
                v_font->resize(ctx->size + 1);
                ctx->fonts.insert(make_pair(ctx->fontName, v_font));
            }

            if ((*ctx->fonts[ctx->fontName]).size() <= ctx->size)
            {
                (*ctx->fonts[ctx->fontName]).resize(ctx->size + 1);
            }
            if (!(*ctx->fonts[ctx->fontName])[ctx->size])
            {
                (*ctx->fonts[ctx->fontName])[ctx->size] =
                    newFont(default_font, 67548);
                (*ctx->fonts[ctx->fontName])[ctx->size]->FaceSize(ctx->size);
            }
            ctx->initialized = true;
        }
    }

    void GLtext::init(const char* fontName)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();

        ctx->fontName = TwkUtil::pathConform(fontName);

        if (ctx->fonts.count(ctx->fontName) == 0)
        {
            std::vector<FTFont*>* v_font = new std::vector<FTFont*>();
            v_font->resize(ctx->size + 1);
            ctx->fonts.insert(make_pair(ctx->fontName, v_font));
        }

        if (ctx->fonts[ctx->fontName]->size() <= ctx->size)
        {
            ctx->fonts[ctx->fontName]->resize(ctx->size + 1);
        }
        else if ((*ctx->fonts[ctx->fontName])[ctx->size])
        {
            return;
        }

        if (!TwkUtil::fileExists(ctx->fontName.c_str()))
        {
            cerr << "WARNING: can't open '" << fontName
                 << "'. Using default font." << endl;
            init();
            return;
        }

        (*ctx->fonts[ctx->fontName])[ctx->size] =
            newFont(ctx->fontName.c_str());

        if (!(*ctx->fonts[ctx->fontName])[ctx->size])
        {
            cerr << "ERROR: can't open " << fontName << endl;
            string str("Failed to open '");
            str += fontName;
            str += "'";
            TWK_EXC_THROW_WHAT(Exception, str);
        }

        (*ctx->fonts[ctx->fontName])[ctx->size]->FaceSize(ctx->size);
        (*ctx->fonts[ctx->fontName])[ctx->size]->Depth(20);
        (*ctx->fonts[ctx->fontName])[ctx->size]->CharMap(ft_encoding_unicode);
        ctx->initialized = true;
    }

    void GLtext::init(const unsigned char* fontData, int fontDataSize)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();

        if (ctx->fonts[ctx->fontName]->size() <= ctx->size)
            ctx->fonts[ctx->fontName]->resize(ctx->size + 1);

        if (!(*ctx->fonts[ctx->fontName])[ctx->size])
        {
            (*ctx->fonts[ctx->fontName])[ctx->size] =
                newFont(fontData, fontDataSize);
            (*ctx->fonts[ctx->fontName])[ctx->size]->FaceSize(ctx->size);
        }
    }

    void GLtext::size(int s)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        ctx->size = s;

        if (ctx->fonts.count(ctx->fontName) == 0)
        {
            init();
        }
        else if (ctx->fonts[ctx->fontName]->size() <= s
                 || !(*ctx->fonts[ctx->fontName])[s])
        {
            if (ctx->fontName != "")
            {
                init(ctx->fontName.c_str());
            }
            else
            {
                init();
            }
        }

#if 0
    if (usePixmaps)
    {
        (*ctx->fonts[ctx->fontName])[ctx->size]->FaceSize( ctx->size );
        (*ctx->fonts[ctx->fontName])[ctx->size]->Render( " " );
    }
#endif
    }

    int GLtext::size()
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        return ctx->size;
    }

    TwkMath::Box2f GLtext::bounds(const char* text)
    {
        if (!text)
            return TwkMath::Box2f();
        GLTextContext* ctx = (GLTextContext*)getContext();

        if (ctx->fonts.count(ctx->fontName) == 0
            || ctx->fonts[ctx->fontName]->size() <= ctx->size
            || !(*ctx->fonts[ctx->fontName])[ctx->size])
        {
            return TwkMath::Box2f();
        }

        wstring w;
        utf8toWChar(text, w);

        FTBBox b = (*ctx->fonts[ctx->fontName])[ctx->size]->BBox(w.c_str());
        FTPoint p0 = b.Lower();
        FTPoint p1 = b.Upper();

        return TwkMath::Box2f(TwkMath::Vec2f(p0.X(), p0.Y()),
                              TwkMath::Vec2f(p1.X(), p1.Y()));
    }

    TwkMath::Box2f GLtext::bounds(string text)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        return bounds(text.c_str());
    }

    TwkMath::Box2f GLtext::boundsNL(string text, float mult)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        vector<string> lines;
        stl_ext::tokenize(lines, text, "\n");
        float advance =
            (*ctx->fonts[ctx->fontName])[ctx->size]->Ascender()
            - (2 * (*ctx->fonts[ctx->fontName])[ctx->size]->Descender());
        advance *= mult;

        TwkMath::Box2f bounds;

        bounds.makeEmpty();
        for (int i = 0; i < lines.size(); ++i)
        {
            TwkMath::Box2f b = GLtext::bounds(lines[i]);
            b.min.y -= (advance * i);
            bounds.extendBy(b);
        }
        return bounds;
    }

    TwkMath::Box2f GLtext::boundsNL(const char* text, float mult)
    {
        if (!text)
            return TwkMath::Box2f();
        return boundsNL(std::string(text), mult);
    }

    void GLtext::writeAt(float x, float y, const char* text)
    {
        if (!text)
            return;
        GLTextContext* ctx = (GLTextContext*)getContext();
        if (ctx->fonts.count(ctx->fontName) == 0
            || ctx->fonts[ctx->fontName]->size() <= ctx->size
            || !(*ctx->fonts[ctx->fontName])[ctx->size])
        {
            init();
        }

        glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT
                     | GL_PIXEL_MODE_BIT);
        glPushMatrix();
        glTranslatef(x, y, 0);

        glPixelTransferf(GL_RED_SCALE, 1.0f);
        glPixelTransferf(GL_GREEN_SCALE, 1.0f);
        glPixelTransferf(GL_BLUE_SCALE, 1.0f);

        // #if FTGL_FONT_TYPE == FTGLPixmapFont
        //     abort();
        //     glPixelZoom( 1.0f, 1.0f );
        // #endif
        glColor4f(ctx->color.x, ctx->color.y, ctx->color.z, ctx->color.w);
        // #if FTGL_FONT_TYPE == FTGLPixmapFont
        //     ctx->font[ctx->size]->FaceSize( ctx->size );
        //     glRasterPos2d( 0, 0 );
        // #endif

        wstring w;
        utf8toWChar(text, w);
        (*ctx->fonts[ctx->fontName])[ctx->size]->Render(w.c_str());
        glPopMatrix();
        glPopAttrib();
    }

    int GLtext::writeAtNL(float x, float y, std::string text, float mult)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        vector<string> lines;
        stl_ext::tokenize(lines, text, "\n");
        // ctx->font[ctx->size]->FaceSize( ctx->size );

        float advance =
            mult
            * ((*ctx->fonts[ctx->fontName])[ctx->size]->Ascender()
               - (2.0 * (*ctx->fonts[ctx->fontName])[ctx->size]->Descender()));
        for (int i = 0; i < lines.size(); ++i)
        {
            GLtext::writeAt(x, y + (advance * i), lines[i]);
        }
        return lines.size();
    }

    int GLtext::writeAtNL(float x, float y, const char* text, float mult)
    {
        if (!text)
            return 0;
        return GLtext::writeAtNL(x, y, std::string(text), mult);
    }

    int GLtext::writeAtNL(const TwkMath::Vec2f& pos, const char* text,
                          float mult)
    {
        if (!text)
            return 0;
        return GLtext::writeAtNL(pos.x, pos.y, std::string(text), mult);
    }

    int GLtext::writeAtNL(const TwkMath::Vec2f& pos, std::string text,
                          float mult)
    {
        return GLtext::writeAtNL(pos.x, pos.y, text, mult);
    }

    void GLtext::writeAt(const TwkMath::Vec2f& pos, const char* text)
    {
        if (!text)
            return;
        writeAt(pos.x, pos.y, text);
    }

    void GLtext::writeAt(float x, float y, std::string text)
    {
        writeAt(x, y, text.c_str());
    }

    void GLtext::writeAt(const TwkMath::Vec2f& pos, string text)
    {
        writeAt(pos.x, pos.y, text.c_str());
    }

    void GLtext::color(float r, float g, float b, float a)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        ctx->color = TwkMath::Vec4f(r, g, b, a);
    }

    void GLtext::color(TwkMath::Col3f& c)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        ctx->color = TwkMath::Col4f(c.x, c.y, c.z, 1.0f);
    }

    void GLtext::color(TwkMath::Col4f& c)
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        ctx->color = c;
    }

    float GLtext::globalAscenderHeight()
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        if (ctx->fonts.count(ctx->fontName) == 0
            || ctx->fonts[ctx->fontName]->size() <= ctx->size
            || !(*ctx->fonts[ctx->fontName])[ctx->size])
            init();
        return (*ctx->fonts[ctx->fontName])[ctx->size]->Ascender();
    }

    float GLtext::globalDescenderHeight()
    {
        GLTextContext* ctx = (GLTextContext*)getContext();
        if (ctx->fonts.count(ctx->fontName) == 0
            || ctx->fonts[ctx->fontName]->size() <= ctx->size
            || !(*ctx->fonts[ctx->fontName])[ctx->size])
            init();
        return (*ctx->fonts[ctx->fontName])[ctx->size]->Descender();
    }

} // namespace TwkGLText
