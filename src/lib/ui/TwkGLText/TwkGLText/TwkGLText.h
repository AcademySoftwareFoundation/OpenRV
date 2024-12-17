//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TWKGRAPHTEXT_H__
#define __TWKGRAPHTEXT_H__

#include <TwkExc/TwkExcException.h>
#include <TwkMath/Color.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Box.h>
#include <string>
#include <map>

//
//  This is a static class that is useful for drawing text into an
//  openGL context.  It depends on the FTGL library, and on freetype2.
//  Freetype2 is part of the default install on RedHat 7.2+ and can
//  easily be installed on OSX via fink (it's part of Tweak's default
//  install on alcovitch) See README.FTGL for info on building
//  libftgl.a.
//
//  There are some issues when using this class -- there is thread
//  specific dat required to keep track of the text state between
//  calls -- this may or may not correspond to a GL context. Unless
//  you are using GL contexts with shared textures, you are required
//  to create a new context and set it before calling these functions
//  -- in that case you should have a text context for each gl
//  context.
//

namespace TwkGLText
{

    typedef void* Context;

    class GLtext
    {
    public:
        TWK_EXC_DECLARE(Exception, TwkExc::Exception, "glText::Exception: ");

        static Context newContext();
        static Context getContext();
        static void setContext(Context);
        static void deleteContext(Context);

        static void clear();

        static void init();
        static void init(const char* fontName);
        static void init(const unsigned char* fontData, int fontDataSize);

        static void writeAt(float x, float y, const char* text);
        static void writeAt(const TwkMath::Vec2f& pos, const char* text);

        static void writeAt(float x, float y, std::string text);
        static void writeAt(const TwkMath::Vec2f& pos, std::string text);

        // Pay attention to newlines, returns # of lines written
        static int writeAtNL(float x, float y, const char* text,
                             float mult = 1.0f);
        static int writeAtNL(const TwkMath::Vec2f& pos, const char* text,
                             float mult = 1.0f);

        static int writeAtNL(float x, float y, std::string text,
                             float mult = 1.0f);
        static int writeAtNL(const TwkMath::Vec2f& pos, std::string text,
                             float mult = 1.0f);

        static void size(int s);
        static int size();
        static TwkMath::Box2f bounds(const char* text);
        static TwkMath::Box2f bounds(std::string text);

        static TwkMath::Box2f boundsNL(const char* text, float mult = 1.0f);
        static TwkMath::Box2f boundsNL(std::string text, float mult = 1.0f);

        static void color(float r, float g, float b, float a = 1.0f);
        static void color(TwkMath::Col3f& c);
        static void color(TwkMath::Col4f& c);

        static float globalAscenderHeight();
        static float globalDescenderHeight();
    };

} // namespace TwkGLText

#endif // End #ifdef __TWKGRAPHTEXT_H__
