//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkGLText__GLT__h__
#define __TwkGLText__GLT__h__
#include <TwkGLF/GL.h>
#include <FTGL/ftgl.h>
#include <string>
#include <vector>
#include <map>

#ifndef FTGL_FONT_TYPE
// #define FTGL_FONT_TYPE FTGLPixmapFont
#define FTGL_FONT_TYPE FTTextureFont
#endif

//
//  GLT
//
//  This is a high level "extension" API for text rendering
//

//
//  Types
//
//  A Face is a font+size
//

typedef FTGL_FONT_TYPE GLTFace;
typedef std::pair<std::string, size_t> GLTFaceDescription;
typedef std::pair<GLTFaceDescription, GLTFace*> GLTState;
typedef std::vector<GLTState> GLTStateStack;
typedef std::map<GLTFaceDescription, GLTFace*> GLTFaceCache;

//
//  glt is a state machine much like GL itself
//  The state is composed of:
//
//  > The current Face
//

//
//  State setting functions
//

void gltPushFace();
void gltPopFace();

void gltFace(const std::string& fontName, size_t fontSize);
void gltFace(const std::string& fontName);
void gltSize(size_t fontSize);

//
//  Rendering strings
//

void gltRender(float x, float y, const std::string& text);
void gltMultipleLineRender(float x, float y, const std::string& text);

//
//  Measuring strings and the current face
//

void gltBounds(const std::string& text, float& xmin, float& xmax, float& ymin,
               float& ymax);

void gltMultipleLineBounds(const std::string& text, float& xmin, float& xmax,
                           float& ymin, float& ymax);

float gltAscenderHeight();
float gltDescenderHeight();

#endif // __TwkGLText__GLT__h__
