//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the qt2mu.py script.
//            If it is not possible, manual editing is ok but it could be lost in future generations.

#include <MuQt6/qtUtils.h>
#include <MuQt6/QRegionType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QWidgetType.h>
#include <Mu/Alias.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <QtNetwork/QtNetwork>
#include <MuQt6/QRectType.h>
#include <MuQt6/QBitmapType.h>
#include <MuQt6/QPointType.h>

namespace Mu {
using namespace std;

QRegionType::Instance::Instance(const Class* c) : ClassInstance(c)
{
}

QRegionType::QRegionType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QRegionType::~QRegionType()
{
}

static NODE_IMPLEMENTATION(__allocate, Pointer)
{
    QRegionType::Instance* i = new QRegionType::Instance((Class*)NODE_THIS.type());
    QRegionType::registerFinalizer(i);
    NODE_RETURN(i);
}

void 
QRegionType::registerFinalizer (void* o)
{
    GC_register_finalizer(o, QRegionType::finalizer, 0, 0, 0);
}

void 
QRegionType::finalizer (void* obj, void* data)
{
    QRegionType::Instance* i = reinterpret_cast<QRegionType::Instance*>(obj);
    delete i;
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

Pointer qt_QRegion_QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    setqtype<QRegionType>(param_this,QRegion());
    return param_this;
}

Pointer qt_QRegion_QRegion_QRegion_QRegion_int_int_int_int_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_x, int param_y, int param_w, int param_h, int param_t)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    int arg1 = (int)(param_x);
    int arg2 = (int)(param_y);
    int arg3 = (int)(param_w);
    int arg4 = (int)(param_h);
    QRegion::RegionType arg5 = (QRegion::RegionType)(param_t);
    setqtype<QRegionType>(param_this,QRegion(arg1, arg2, arg3, arg4, arg5));
    return param_this;
}

Pointer qt_QRegion_QRegion_QRegion_QRegion_QRect_int(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r, int param_t)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRect  arg1 = getqtype<QRectType>(param_r);
    QRegion::RegionType arg2 = (QRegion::RegionType)(param_t);
    setqtype<QRegionType>(param_this,QRegion(arg1, arg2));
    return param_this;
}

Pointer qt_QRegion_QRegion_QRegion_QRegion_QBitmap(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_bm)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QBitmap  arg1 = getqtype<QBitmapType>(param_bm);
    setqtype<QRegionType>(param_this,QRegion(arg1));
    return param_this;
}

Pointer qt_QRegion_boundingRect_QRect_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    return makeqtype<QRectType>(c,arg0.boundingRect(),"qt.QRect");
}

bool qt_QRegion_contains_bool_QRegion_QPoint(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_p)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QPoint  arg1 = getqtype<QPointType>(param_p);
    return arg0.contains(arg1);
}

bool qt_QRegion_contains_bool_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_r);
    return arg0.contains(arg1);
}

Pointer qt_QRegion_intersected_QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.intersected(arg1),"qt.QRegion");
}

Pointer qt_QRegion_intersected_QRegion_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_rect)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_rect);
    return makeqtype<QRegionType>(c,arg0.intersected(arg1),"qt.QRegion");
}

bool qt_QRegion_intersects_bool_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_region)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_region);
    return arg0.intersects(arg1);
}

bool qt_QRegion_intersects_bool_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_rect)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_rect);
    return arg0.intersects(arg1);
}

bool qt_QRegion_isEmpty_bool_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    return arg0.isEmpty();
}

bool qt_QRegion_isNull_bool_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    return arg0.isNull();
}

int qt_QRegion_rectCount_int_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    return arg0.rectCount();
}

Pointer qt_QRegion_subtracted_QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.subtracted(arg1),"qt.QRegion");
}

void qt_QRegion_swap_void_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_other)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    QRegion  arg1 = getqtype<QRegionType>(param_other);
    arg0.swap(arg1);
    setqtype<QRegionType>(param_this,arg0);
}

void qt_QRegion_translate_void_QRegion_int_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_dx, int param_dy)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    int arg1 = (int)(param_dx);
    int arg2 = (int)(param_dy);
    arg0.translate(arg1, arg2);
    setqtype<QRegionType>(param_this,arg0);
}

void qt_QRegion_translate_void_QRegion_QPoint(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_point)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QPoint  arg1 = getqtype<QPointType>(param_point);
    arg0.translate(arg1);
    setqtype<QRegionType>(param_this,arg0);
}

Pointer qt_QRegion_translated_QRegion_QRegion_int_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_dx, int param_dy)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    int arg1 = (int)(param_dx);
    int arg2 = (int)(param_dy);
    return makeqtype<QRegionType>(c,arg0.translated(arg1, arg2),"qt.QRegion");
}

Pointer qt_QRegion_translated_QRegion_QRegion_QPoint(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_p)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QPoint  arg1 = getqtype<QPointType>(param_p);
    return makeqtype<QRegionType>(c,arg0.translated(arg1),"qt.QRegion");
}

Pointer qt_QRegion_united_QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.united(arg1),"qt.QRegion");
}

Pointer qt_QRegion_united_QRegion_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_rect)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_rect);
    return makeqtype<QRegionType>(c,arg0.united(arg1),"qt.QRegion");
}

Pointer qt_QRegion_xored_QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.xored(arg1),"qt.QRegion");
}

bool qt_QRegion_operatorBang_EQ__bool_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_other)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_other);
    return arg0.operator!=(arg1);
}

Pointer qt_QRegion_operatorAmp__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator&(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorAmp__QRegion_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator&(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorPlus__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator+(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorPlus__QRegion_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator+(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorPlus_EQ__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator+=(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorPlus_EQ__QRegion_QRegion_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_rect)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_rect);
    return makeqtype<QRegionType>(c,arg0.operator+=(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorMinus__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator-(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorMinus_EQ__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator-=(arg1),"qt.QRegion");
}

bool qt_QRegion_operatorEQ_EQ__bool_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return arg0.operator==(arg1);
}

Pointer qt_QRegion_operatorCaret__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator^(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorCaret_EQ__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator^=(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorPipe__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator|(arg1),"qt.QRegion");
}

Pointer qt_QRegion_operatorPipe_EQ__QRegion_QRegion_QRegion(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_r)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QRegion& arg0 = getqtype<QRegionType>(param_this);
    const QRegion  arg1 = getqtype<QRegionType>(param_r);
    return makeqtype<QRegionType>(c,arg0.operator|=(arg1),"qt.QRegion");
}


static NODE_IMPLEMENTATION(_n_QRegion0, Pointer)
{
    NODE_RETURN(qt_QRegion_QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_QRegion1, Pointer)
{
    NODE_RETURN(qt_QRegion_QRegion_QRegion_QRegion_int_int_int_int_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, int), NODE_ARG(3, int), NODE_ARG(4, int), NODE_ARG(5, int)));
}

static NODE_IMPLEMENTATION(_n_QRegion2, Pointer)
{
    NODE_RETURN(qt_QRegion_QRegion_QRegion_QRegion_QRect_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, int)));
}

static NODE_IMPLEMENTATION(_n_QRegion4, Pointer)
{
    NODE_RETURN(qt_QRegion_QRegion_QRegion_QRegion_QBitmap(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_boundingRect0, Pointer)
{
    NODE_RETURN(qt_QRegion_boundingRect_QRect_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_contains0, bool)
{
    NODE_RETURN(qt_QRegion_contains_bool_QRegion_QPoint(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_contains1, bool)
{
    NODE_RETURN(qt_QRegion_contains_bool_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_intersected0, Pointer)
{
    NODE_RETURN(qt_QRegion_intersected_QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_intersected1, Pointer)
{
    NODE_RETURN(qt_QRegion_intersected_QRegion_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_intersects0, bool)
{
    NODE_RETURN(qt_QRegion_intersects_bool_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_intersects1, bool)
{
    NODE_RETURN(qt_QRegion_intersects_bool_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isEmpty0, bool)
{
    NODE_RETURN(qt_QRegion_isEmpty_bool_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isNull0, bool)
{
    NODE_RETURN(qt_QRegion_isNull_bool_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_rectCount0, int)
{
    NODE_RETURN(qt_QRegion_rectCount_int_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_subtracted0, Pointer)
{
    NODE_RETURN(qt_QRegion_subtracted_QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_swap0, void)
{
    qt_QRegion_swap_void_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_translate0, void)
{
    qt_QRegion_translate_void_QRegion_int_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, int));
}

static NODE_IMPLEMENTATION(_n_translate1, void)
{
    qt_QRegion_translate_void_QRegion_QPoint(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_translated0, Pointer)
{
    NODE_RETURN(qt_QRegion_translated_QRegion_QRegion_int_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, int)));
}

static NODE_IMPLEMENTATION(_n_translated1, Pointer)
{
    NODE_RETURN(qt_QRegion_translated_QRegion_QRegion_QPoint(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_united0, Pointer)
{
    NODE_RETURN(qt_QRegion_united_QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_united1, Pointer)
{
    NODE_RETURN(qt_QRegion_united_QRegion_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_xored0, Pointer)
{
    NODE_RETURN(qt_QRegion_xored_QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorBang_EQ_0, bool)
{
    NODE_RETURN(qt_QRegion_operatorBang_EQ__bool_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorAmp_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorAmp__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorAmp_1, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorAmp__QRegion_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorPlus_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorPlus__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorPlus_1, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorPlus__QRegion_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorPlus_EQ_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorPlus_EQ__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorPlus_EQ_1, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorPlus_EQ__QRegion_QRegion_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorMinus_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorMinus__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorMinus_EQ_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorMinus_EQ__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorEQ_EQ_0, bool)
{
    NODE_RETURN(qt_QRegion_operatorEQ_EQ__bool_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorCaret_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorCaret__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorCaret_EQ_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorCaret_EQ__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorPipe_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorPipe__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorPipe_EQ_0, Pointer)
{
    NODE_RETURN(qt_QRegion_operatorPipe_EQ__QRegion_QRegion_QRegion(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}



void
QRegionType::load()
{
    USING_MU_FUNCTION_SYMBOLS;
    MuLangContext* c = static_cast<MuLangContext*>(context());
    Module* global = globalModule();
    
    const string typeName        = name();
    const string refTypeName     = typeName + "&";
    const string fullTypeName    = fullyQualifiedName();
    const string fullRefTypeName = fullTypeName + "&";
    const char*  tn              = typeName.c_str();
    const char*  ftn             = fullTypeName.c_str();
    const char*  rtn             = refTypeName.c_str();
    const char*  frtn            = fullRefTypeName.c_str();

    scope()->addSymbols(new ReferenceType(c, rtn, this),

                        new Function(c, tn, BaseFunctions::dereference, Cast,
                                     Return, ftn,
                                     Args, frtn, End),

                        EndArguments);
    
    addSymbols(new Function(c, "__allocate", __allocate, None,
                            Return, ftn,
                            End),

               EndArguments );

addSymbols(
    new Alias(c, "RegionType", "int"),
      new SymbolicConstant(c, "Rectangle", "int", Value(int(QRegion::Rectangle))),
      new SymbolicConstant(c, "Ellipse", "int", Value(int(QRegion::Ellipse))),
    EndArguments);

addSymbols(
    // enums
    // member functions
    new Function(c, "QRegion", _n_QRegion0, None, Compiled, qt_QRegion_QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), End),
    new Function(c, "QRegion", _n_QRegion1, None, Compiled, qt_QRegion_QRegion_QRegion_QRegion_int_int_int_int_int, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "x", "int"), new Param(c, "y", "int"), new Param(c, "w", "int"), new Param(c, "h", "int"), new Param(c, "t", "int", Value((int)QRegion::Rectangle)), End),
    new Function(c, "QRegion", _n_QRegion2, None, Compiled, qt_QRegion_QRegion_QRegion_QRegion_QRect_int, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRect"), new Param(c, "t", "int", Value((int)QRegion::Rectangle)), End),
    // MISSING: QRegion (QRegion; QRegion this, "const QPolygon &" a, flags Qt::FillRule fillRule)
    new Function(c, "QRegion", _n_QRegion4, None, Compiled, qt_QRegion_QRegion_QRegion_QRegion_QBitmap, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "bm", "qt.QBitmap"), End),
    // MISSING: QRegion (QRegion; QRegion this, QRegion r)
    // MISSING: QRegion (QRegion; QRegion this, "QRegion & &" other)
    // MISSING: begin ("QRegion::const_iterator"; QRegion this)
    new Function(c, "boundingRect", _n_boundingRect0, None, Compiled, qt_QRegion_boundingRect_QRect_QRegion, Return, "qt.QRect", Parameters, new Param(c, "this", "qt.QRegion"), End),
    // MISSING: cbegin ("QRegion::const_iterator"; QRegion this)
    // MISSING: cend ("QRegion::const_iterator"; QRegion this)
    new Function(c, "contains", _n_contains0, None, Compiled, qt_QRegion_contains_bool_QRegion_QPoint, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "p", "qt.QPoint"), End),
    new Function(c, "contains", _n_contains1, None, Compiled, qt_QRegion_contains_bool_QRegion_QRect, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRect"), End),
    // MISSING: crbegin ("QRegion::const_reverse_iterator"; QRegion this)
    // MISSING: crend ("QRegion::const_reverse_iterator"; QRegion this)
    // MISSING: end ("QRegion::const_iterator"; QRegion this)
    new Function(c, "intersected", _n_intersected0, None, Compiled, qt_QRegion_intersected_QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "intersected", _n_intersected1, None, Compiled, qt_QRegion_intersected_QRegion_QRegion_QRect, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "rect", "qt.QRect"), End),
    new Function(c, "intersects", _n_intersects0, None, Compiled, qt_QRegion_intersects_bool_QRegion_QRegion, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "region", "qt.QRegion"), End),
    new Function(c, "intersects", _n_intersects1, None, Compiled, qt_QRegion_intersects_bool_QRegion_QRect, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "rect", "qt.QRect"), End),
    new Function(c, "isEmpty", _n_isEmpty0, None, Compiled, qt_QRegion_isEmpty_bool_QRegion, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), End),
    new Function(c, "isNull", _n_isNull0, None, Compiled, qt_QRegion_isNull_bool_QRegion, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), End),
    // MISSING: rbegin ("QRegion::const_reverse_iterator"; QRegion this)
    new Function(c, "rectCount", _n_rectCount0, None, Compiled, qt_QRegion_rectCount_int_QRegion, Return, "int", Parameters, new Param(c, "this", "qt.QRegion"), End),
    // MISSING: rend ("QRegion::const_reverse_iterator"; QRegion this)
    new Function(c, "subtracted", _n_subtracted0, None, Compiled, qt_QRegion_subtracted_QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "swap", _n_swap0, None, Compiled, qt_QRegion_swap_void_QRegion_QRegion, Return, "void", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "other", "qt.QRegion"), End),
    // MISSING: toHRGN ("HRGN"; QRegion this)
    new Function(c, "translate", _n_translate0, None, Compiled, qt_QRegion_translate_void_QRegion_int_int, Return, "void", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "dx", "int"), new Param(c, "dy", "int"), End),
    new Function(c, "translate", _n_translate1, None, Compiled, qt_QRegion_translate_void_QRegion_QPoint, Return, "void", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "point", "qt.QPoint"), End),
    new Function(c, "translated", _n_translated0, None, Compiled, qt_QRegion_translated_QRegion_QRegion_int_int, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "dx", "int"), new Param(c, "dy", "int"), End),
    new Function(c, "translated", _n_translated1, None, Compiled, qt_QRegion_translated_QRegion_QRegion_QPoint, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "p", "qt.QPoint"), End),
    new Function(c, "united", _n_united0, None, Compiled, qt_QRegion_united_QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "united", _n_united1, None, Compiled, qt_QRegion_united_QRegion_QRegion_QRect, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "rect", "qt.QRect"), End),
    new Function(c, "xored", _n_xored0, None, Compiled, qt_QRegion_xored_QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    // MISSING: QVariant ("QVariant operator"; QRegion this)
    // MISSING: = ("QRegion & operator&"; QRegion this, QRegion r)
    // MISSING: = ("QRegion & operator&"; QRegion this, QRect r)
    // static functions
    // MISSING: fromHRGN (QRegion; "HRGN" hrgn)
    EndArguments);
globalScope()->addSymbols(
    new Function(c, "!=", _n_operatorBang_EQ_0, Op, Compiled, qt_QRegion_operatorBang_EQ__bool_QRegion_QRegion, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "other", "qt.QRegion"), End),
    new Function(c, "&", _n_operatorAmp_0, Op, Compiled, qt_QRegion_operatorAmp__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "&", _n_operatorAmp_1, Op, Compiled, qt_QRegion_operatorAmp__QRegion_QRegion_QRect, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRect"), End),
    new Function(c, "+", _n_operatorPlus_0, Op, Compiled, qt_QRegion_operatorPlus__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "+", _n_operatorPlus_1, Op, Compiled, qt_QRegion_operatorPlus__QRegion_QRegion_QRect, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRect"), End),
    new Function(c, "+=", _n_operatorPlus_EQ_0, Op, Compiled, qt_QRegion_operatorPlus_EQ__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "+=", _n_operatorPlus_EQ_1, Op, Compiled, qt_QRegion_operatorPlus_EQ__QRegion_QRegion_QRect, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "rect", "qt.QRect"), End),
    new Function(c, "-", _n_operatorMinus_0, Op, Compiled, qt_QRegion_operatorMinus__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "-=", _n_operatorMinus_EQ_0, Op, Compiled, qt_QRegion_operatorMinus_EQ__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    // MISSING: = (QRegion; QRegion this, QRegion r)
    // MISSING: = (QRegion; QRegion this, "QRegion & &" other)
    new Function(c, "==", _n_operatorEQ_EQ_0, Op, Compiled, qt_QRegion_operatorEQ_EQ__bool_QRegion_QRegion, Return, "bool", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "^", _n_operatorCaret_0, Op, Compiled, qt_QRegion_operatorCaret__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "^=", _n_operatorCaret_EQ_0, Op, Compiled, qt_QRegion_operatorCaret_EQ__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "|", _n_operatorPipe_0, Op, Compiled, qt_QRegion_operatorPipe__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    new Function(c, "|=", _n_operatorPipe_EQ_0, Op, Compiled, qt_QRegion_operatorPipe_EQ__QRegion_QRegion_QRegion, Return, "qt.QRegion", Parameters, new Param(c, "this", "qt.QRegion"), new Param(c, "r", "qt.QRegion"), End),
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
