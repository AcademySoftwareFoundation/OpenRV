//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkGLText/TwkGLText.h>
#include <TwkGLText/defaultFont.h>
#include <TwkGLText/GLT.h>
#include <FTGL/ftgl.h>
#include <TwkExc/TwkExcException.h>
#include <TwkMath/Box.h>
#include <TwkMath/Color.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec4.h>
#include <iostream>
#include <stl_ext/string_algo.h>

using namespace std;
using namespace TwkMath;

#define CURRENT_FACE (state.back().second)

static GLTStateStack state(1);
static GLTFaceCache cache;

void gltPushFace() { state.push_back(state.back()); }

void gltPopFace() { state.pop_back(); }

void gltFace(const std::string& fontName, size_t fontSize)
{
    GLTFaceDescription d(fontName, fontSize);
    GLTFace* face = cache[d];

    if (!face)
    {
        face = new GLTFace(default_font, 67548);
        face->FaceSize(fontSize);
        cache[d] = face;
    }

    state.back() = GLTState(d, face);
}

void gltFace(const std::string& fontName)
{
    gltFace(fontName, state.back().first.second);
}

void gltSize(size_t fontSize) { gltFace(state.back().first.first, fontSize); }

float gltAscenderHeight() { return CURRENT_FACE->Ascender(); }

float gltDescenderHeight() { return CURRENT_FACE->Descender(); }

void gltBounds(const std::string& text, float& xmin, float& xmax, float& ymin,
               float& ymax)
{
    float zmin, zmax;
    CURRENT_FACE->BBox(text.c_str(), xmin, ymin, zmin, xmax, ymax, zmax);
}

void gltMultipleLineBounds(const std::string& text, float& xmin, float& xmax,
                           float& ymin, float& ymax)
{
    vector<string> lines;
    stl_ext::tokenize(lines, text, "\n");
    float advance = gltAscenderHeight() - (2 * gltDescenderHeight());
    gltBounds(lines.front(), xmin, xmax, ymin, ymax);

    for (int i = 1; i < lines.size(); i++)
    {
        float x1, y1, x2, y2;
        gltBounds(lines[i], x1, x2, y1, y2);
        if (x1 < xmin)
            xmin = x1;
        if (x2 > xmax)
            xmax = x2;
        if (y1 < ymin)
            ymin = y1;
        ymax += (y2 - y1);
    }
}

void gltRender(float x, float y, const std::string& text)
{
    glPushAttrib(GL_PIXEL_MODE_BIT);

    glPushMatrix();
    glTranslatef(x, y, 0);

    glPixelTransferf(GL_RED_SCALE, 1.0f);
    glPixelTransferf(GL_GREEN_SCALE, 1.0f);
    glPixelTransferf(GL_BLUE_SCALE, 1.0f);

#if FTGL_FONT_TYPE == FTGLPixmapFont
    glPixelZoom(1.0f, 1.0f);
#endif
#if FTGL_FONT_TYPE == FTGLPixmapFont
    glRasterPos2d(0, 0);
#endif

    CURRENT_FACE->Render(text.c_str());
    glPopMatrix();
    glPopAttrib();
}

void gltMultipleLineRender(float x, float y, const std::string& text,
                           float mult)
{
    vector<string> lines;
    stl_ext::tokenize(lines, text, "\n");

    float advance = mult * (gltAscenderHeight() - 2.0 * gltDescenderHeight());

    for (int i = 0; i < lines.size(); ++i)
    {
        gltRender(x, y + advance * i, lines[i]);
    }
}
